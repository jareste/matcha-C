"""
Regression tests for the route contract and the transport.

Every bug these cover was found by running the server, not by reading it:
a GET on a POST-only route used to abort() the process, ~25% of concurrent TLS
requests used to get no response at all, and a websocket payload of 126 bytes
used to corrupt the frame. They are grouped by the mechanism they protect.

Run against either mode:
    make all  && pytest tests/
    make tls  && MATCHA_BASE_URL=https://localhost:8443 pytest tests/
"""
import base64
import json
import os
import random
import socket
import ssl
import string
import struct
from concurrent.futures import ThreadPoolExecutor

import pytest
import requests

BASE_URL = os.environ.get("MATCHA_BASE_URL", "http://localhost:8080")
VERIFY_TLS = os.environ.get("MATCHA_VERIFY_TLS", "0") == "1"
IS_TLS = BASE_URL.startswith("https")

if IS_TLS and not VERIFY_TLS:
    import urllib3
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


def _rand(n=8):
    return "".join(random.choice(string.ascii_lowercase) for _ in range(n))


# Every request carries a timeout: a test for dropped connections must fail,
# never hang, when a connection is actually dropped.
HTTP_TIMEOUT = 10


class _TimeoutSession(requests.Session):
    def request(self, *a, **kw):
        kw.setdefault("timeout", HTTP_TIMEOUT)
        return super().request(*a, **kw)


def new_session():
    s = _TimeoutSession()
    s.verify = VERIFY_TLS
    return s


@pytest.fixture(scope="module")
def creds():
    """A freshly registered, logged-in user."""
    u = "ct_" + _rand()
    s = new_session()
    r = s.post(f"{BASE_URL}/api/register", json={
        "username": u, "email": f"{u}@example.com", "password": "C0ntract!",
        "first_name": "Con", "last_name": "Tract"})
    assert r.status_code == 201, r.text
    r = s.post(f"{BASE_URL}/api/login", json={"email": f"{u}@example.com", "password": "C0ntract!"})
    assert r.status_code == 200, r.text
    return {"user": u, "email": f"{u}@example.com", "password": "C0ntract!",
            "cookies": dict(s.cookies)}


@pytest.fixture
def auth(creds):
    s = new_session()
    s.cookies.update(creds["cookies"])
    return s


# --------------------------------------------------------------------------
# Method enforcement. A GET to /api/tags/insert used to kill the process via
# ASSERT(ctx->parsed_request.body) -> abort(), so any logged-in user could take
# the server down with one request.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("method,path", [
    ("GET",    "/api/login"),
    ("GET",    "/api/logout"),
    ("POST",   "/api/validate"),
    ("GET",    "/api/tags/insert"),
    ("DELETE", "/api/tags/insert"),
    ("GET",    "/api/profile/update"),
    ("POST",   "/api/profile/get"),
    ("POST",   "/api/chat"),
    ("GET",    "/api/pics/insert"),
])
def test_wrong_method_is_405_and_server_survives(auth, method, path):
    r = auth.request(method, f"{BASE_URL}{path}")
    assert r.status_code == 405, f"{method} {path} returned {r.status_code}"
    # The process must still be answering afterwards.
    assert auth.get(f"{BASE_URL}/api/validate").status_code == 200


# --------------------------------------------------------------------------
# Body preconditions, enforced by the router before a handler is entered.
# --------------------------------------------------------------------------
def test_missing_body_is_rejected():
    r = new_session().post(f"{BASE_URL}/api/login")
    assert r.status_code == 400
    assert "body" in r.text.lower()


def test_malformed_json_is_rejected():
    r = new_session().post(f"{BASE_URL}/api/login", data="{not json",
                           headers={"Content-Type": "application/json"})
    assert r.status_code == 400
    assert "json" in r.text.lower()


@pytest.mark.parametrize("body,missing", [
    ({},                      "email"),
    ({"email": "a@b.c"},      "password"),
    ({"password": "x"},       "email"),
])
def test_missing_field_names_the_field(body, missing):
    r = new_session().post(f"{BASE_URL}/api/login", json=body)
    assert r.status_code == 400
    assert missing in r.text


def test_tags_requires_an_array(auth):
    assert auth.post(f"{BASE_URL}/api/tags/insert", json={"tags": "nope"}).status_code == 400
    assert auth.post(f"{BASE_URL}/api/tags/insert", json={}).status_code == 400
    assert auth.post(f"{BASE_URL}/api/tags/insert", json={"tags": [1, 2]}).status_code == 200


# --------------------------------------------------------------------------
# Auth and query preconditions.
# --------------------------------------------------------------------------
def test_unauthenticated_is_rejected():
    for path in ("/api/validate", "/api/profile/get", "/api/chat"):
        assert new_session().get(f"{BASE_URL}{path}").status_code in (401, 403), path


def test_query_needed(auth):
    assert auth.get(f"{BASE_URL}/api/chat/messages").status_code == 400


def test_uid_is_not_mistaken_for_id(auth):
    """strstr(query, "id=") also matched the tail of "uid=7"."""
    assert auth.get(f"{BASE_URL}/api/chat/messages/?uid=7").status_code == 400
    assert auth.get(f"{BASE_URL}/api/chat/messages/?uid=7&id=9").status_code == 200


# --------------------------------------------------------------------------
# Passwords. Stored values must be PBKDF2, never the plaintext, and a bad
# password must be indistinguishable from an unknown account.
# --------------------------------------------------------------------------
def test_wrong_password_and_unknown_user_are_indistinguishable(creds):
    bad = new_session().post(f"{BASE_URL}/api/login",
                             json={"email": creds["email"], "password": "wrong"})
    unknown = new_session().post(f"{BASE_URL}/api/login",
                                 json={"email": f"{_rand()}@nope.com", "password": "wrong"})
    assert bad.status_code == unknown.status_code == 400
    assert bad.json() == unknown.json(), "error bodies differ: accounts can be enumerated"


def test_login_sets_an_httponly_cookie():
    """
    Registers its own user on purpose: the users row stores a single token, so a
    successful login invalidates every other session for that account. Sharing
    the module user here would silently 401 every later authenticated test.
    """
    u = "ck_" + _rand()
    s = new_session()
    assert s.post(f"{BASE_URL}/api/register", json={
        "username": u, "email": f"{u}@example.com", "password": "C0ntract!",
        "first_name": "Co", "last_name": "Ok"}).status_code == 201

    r = new_session().post(f"{BASE_URL}/api/login",
                           json={"email": f"{u}@example.com", "password": "C0ntract!"})
    assert r.status_code == 200
    cookie = r.headers.get("Set-Cookie", "")
    assert "token=" in cookie
    assert "HttpOnly" in cookie


def test_cors_origin_is_echoed_not_hardcoded():
    origin = "http://localhost:3000"
    r = new_session().post(f"{BASE_URL}/api/login", json={}, headers={"Origin": origin})
    assert r.headers.get("Access-Control-Allow-Origin") == origin


def test_options_preflight():
    r = new_session().options(f"{BASE_URL}/api/login",
                              headers={"Origin": "http://localhost:3000"})
    assert r.status_code in (200, 204)
    assert r.headers.get("Access-Control-Allow-Origin") == "http://localhost:3000"


# --------------------------------------------------------------------------
# Concurrency. ~25% of parallel TLS requests used to get no response: the
# handshake and the request arrived in one burst, and a buffered request was
# then discarded when the read hit EOF.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("parallel", [10, 30])
def test_concurrent_requests_all_get_answered(creds, parallel):
    n = parallel * 4

    # Log in afresh: only one token per account is valid at a time.
    s0 = new_session()
    r = s0.post(f"{BASE_URL}/api/login",
                json={"email": creds["email"], "password": creds["password"]})
    assert r.status_code == 200, r.text
    cookies = dict(s0.cookies)

    def one(i):
        s = new_session()
        s.cookies.update(cookies)
        try:
            if i % 3 == 0:
                return s.get(f"{BASE_URL}/api/validate").status_code
            if i % 3 == 1:
                return s.get(f"{BASE_URL}/api/tags/insert").status_code
            return s.post(f"{BASE_URL}/api/login", json={}).status_code
        except Exception as e:  # a dropped connection is the failure we are testing for
            return type(e).__name__

    with ThreadPoolExecutor(parallel) as ex:
        results = list(ex.map(one, range(n)))

    dropped = [r for r in results if not isinstance(r, int)]
    assert not dropped, f"{len(dropped)}/{n} connections got no response: {set(dropped)}"
    assert set(results) == {200, 405, 400}, set(results)


# --------------------------------------------------------------------------
# WebSocket framing, spoken directly over a socket so payload sizes are exact.
# A 126-byte payload used to collide with the 16-bit length escape.
# --------------------------------------------------------------------------
def _ws_connect(path="/ws", cookie=None):
    host = BASE_URL.split("://", 1)[1].split(":")[0]
    port = int(BASE_URL.rsplit(":", 1)[1])
    raw = socket.create_connection((host, port), timeout=10)
    if IS_TLS:
        ctx = ssl.create_default_context()
        if not VERIFY_TLS:
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
        raw = ctx.wrap_socket(raw, server_hostname=host)

    key = base64.b64encode(os.urandom(16)).decode()
    req = (f"GET {path} HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\n"
           f"Connection: Upgrade\r\nSec-WebSocket-Key: {key}\r\n"
           f"Sec-WebSocket-Version: 13\r\n")
    if cookie:
        req += f"Cookie: {cookie}\r\n"
    raw.sendall((req + "\r\n").encode())

    resp = b""
    while b"\r\n\r\n" not in resp:
        chunk = raw.recv(4096)
        if not chunk:
            break
        resp += chunk
    assert b"101" in resp, resp[:120]
    return raw, resp.split(b"\r\n\r\n", 1)[1]


def _frame(payload, opcode=0x1):
    """A client frame: masked, with the correct length encoding."""
    mask = os.urandom(4)
    body = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
    n = len(payload)
    if n <= 125:
        hdr = struct.pack("!BB", 0x80 | opcode, 0x80 | n)
    elif n <= 0xFFFF:
        hdr = struct.pack("!BBH", 0x80 | opcode, 0x80 | 126, n)
    else:
        hdr = struct.pack("!BBQ", 0x80 | opcode, 0x80 | 127, n)
    return hdr + mask + body


def _read_frame(sock):
    def need(k):
        buf = b""
        while len(buf) < k:
            c = sock.recv(k - len(buf))
            if not c:
                raise AssertionError("socket closed mid-frame")
            buf += c
        return buf

    h = need(2)
    opcode = h[0] & 0x0F
    n = h[1] & 0x7F
    if n == 126:
        n = struct.unpack("!H", need(2))[0]
    elif n == 127:
        n = struct.unpack("!Q", need(8))[0]
    assert not (h[1] & 0x80), "server frames must not be masked"
    return opcode, need(n) if n else b""


@pytest.mark.parametrize("size", [0, 1, 125, 126, 127, 1000, 0x10000])
def test_server_frames_use_the_right_length_encoding(size):
    """A ping is echoed as a pong carrying the same payload, at every boundary."""
    sock, _ = _ws_connect()
    try:
        sock.sendall(_frame(b"x" * size, opcode=0x9))
        opcode, payload = _read_frame(sock)
        assert opcode == 0xA, f"expected pong, got opcode {opcode:#x}"
        assert len(payload) == size, f"echoed {len(payload)} bytes for a {size}-byte ping"
    finally:
        sock.close()


def test_several_frames_in_one_segment_are_all_handled():
    """The old reader took one frame per epoll wakeup; the rest stalled."""
    sock, _ = _ws_connect()
    try:
        sock.sendall(b"".join(_frame(bytes([i]) * 10, opcode=0x9) for i in range(4)))
        seen = [_read_frame(sock) for _ in range(4)]
        assert [op for op, _ in seen] == [0xA] * 4, seen
    finally:
        sock.close()


def test_partial_frame_does_not_stall_the_server():
    """
    A half-written header used to park the event loop in a 5s blocking read,
    freezing every other connection.
    """
    sock, _ = _ws_connect()
    try:
        sock.sendall(b"\x89")  # ping opcode, no length byte
        s = new_session()
        r = s.post(f"{BASE_URL}/api/login", json={}, timeout=3)
        assert r.status_code == 400
    finally:
        sock.close()


def test_oversized_frame_is_refused_not_allocated():
    sock, _ = _ws_connect()
    try:
        # Claim 2 GiB without sending it.
        sock.sendall(struct.pack("!BBQ", 0x81, 0x80 | 127, 1 << 31) + os.urandom(4))
        sock.settimeout(5)
        try:
            data = sock.recv(16)
            assert data == b"", "expected the connection to be closed"
        except (ssl.SSLError, OSError):
            pass  # connection torn down, which is the point
    finally:
        sock.close()
