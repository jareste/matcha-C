import os
import random
import string
import struct
import zlib

import pytest
import requests

BASE_URL = os.environ.get("MATCHA_BASE_URL", "http://localhost:8080")
VERIFY_TLS = os.environ.get("MATCHA_VERIFY_TLS", "0") == "1"

if BASE_URL.startswith("https") and not VERIFY_TLS:
    import urllib3
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

HTTP_TIMEOUT = 30

MAX_PICS_PER_USER = 5
MAX_PIC_BYTES = 5 * 1024 * 1024
HTTP_MAX_BODY = 8 * 1024 * 1024


class _TimeoutSession(requests.Session):
    def request(self, *a, **kw):
        kw.setdefault("timeout", HTTP_TIMEOUT)
        return super().request(*a, **kw)


def _rand(n=8):
    return "".join(random.choice(string.ascii_lowercase) for _ in range(n))


# --------------------------------------------------------------------------
# Image fixtures. The server identifies the format from the leading bytes, so
# each of these is only as valid as it needs to be -- except PNG_REAL, which is
# a genuine 1x1 image, used to check the bytes survive a round trip intact.
# --------------------------------------------------------------------------
def _png_1x1():
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"\x00\xff\x00\x00"))
            + chunk(b"IEND", b""))


PNG_REAL = _png_1x1()
PNG = b"\x89PNG\r\n\x1a\n" + b"padding" * 20
JPEG = b"\xff\xd8\xff\xe0" + b"padding" * 20
GIF = b"GIF89a" + b"padding" * 20
WEBP = b"RIFF" + struct.pack("<I", 100) + b"WEBP" + b"padding" * 20


def new_session():
    s = _TimeoutSession()
    s.verify = VERIFY_TLS
    return s


def register_and_login():
    """A user with an empty gallery, so the 5-picture cap never leaks across tests."""
    s = new_session()
    u = "pics_" + _rand()
    r = s.post(f"{BASE_URL}/api/register", json={
        "username": u, "email": f"{u}@example.com", "password": "C0ntract!",
        "first_name": "Pic", "last_name": "Test"})
    assert r.status_code == 201, r.text
    r = s.post(f"{BASE_URL}/api/login", json={
        "email": f"{u}@example.com", "password": "C0ntract!"})
    assert r.status_code == 200, r.text
    return s


@pytest.fixture
def auth():
    return register_and_login()


@pytest.fixture
def other():
    return register_and_login()


def upload(session, content=PNG, filename="pic.png", content_type="image/png"):
    return session.post(f"{BASE_URL}/api/pics/insert",
                        files={"file": (filename, content, content_type)})


def upload_ok(session, **kw):
    r = upload(session, **kw)
    assert r.status_code == 201, r.text
    return r.json()["id"]


# --------------------------------------------------------------------------
# The upload has to actually store something. It used to answer
# 200 "Picture uploaded successfully" while writing nothing at all, and the
# following GET then said 500 "File missing on disk".
# --------------------------------------------------------------------------
def test_upload_then_fetch_returns_the_same_bytes(auth):
    pic_id = upload_ok(auth, content=PNG_REAL, filename="cat.png")

    r = auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}")
    assert r.status_code == 200, r.text
    assert r.content == PNG_REAL


def test_upload_answers_201_with_the_new_id(auth):
    r = upload(auth)
    assert r.status_code == 201, r.text
    body = r.json()
    assert isinstance(body["id"], int) and body["id"] > 0


@pytest.mark.parametrize("content,mime", [
    (PNG, "image/png"),
    (JPEG, "image/jpeg"),
    (GIF, "image/gif"),
    (WEBP, "image/webp"),
])
def test_served_content_type_follows_the_real_format(auth, content, mime):
    # Every picture used to be served as image/jpeg regardless of its format.
    pic_id = upload_ok(auth, content=content, filename="x.bin")
    r = auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}")
    assert r.status_code == 200, r.text
    assert r.headers["Content-Type"] == mime


def test_get_carries_cors_headers(auth):
    # pics_get hand-rolled its header block and omitted these, so the frontend
    # on :3000 could not read the bytes from JS.
    pic_id = upload_ok(auth)
    r = auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}",
                 headers={"Origin": "http://localhost:3000"})
    assert r.status_code == 200
    assert r.headers.get("Access-Control-Allow-Origin") == "http://localhost:3000"
    assert r.headers.get("Access-Control-Allow-Credentials") == "true"
    assert r.headers.get("X-Content-Type-Options") == "nosniff"


# --------------------------------------------------------------------------
# Path traversal. The client's filename went straight into the destination
# path, so this overwrote files outside the upload directory as root.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("filename", [
    "../../../../tmp/pwned.png",
    "..%2f..%2fpwned.png",
    "/etc/pwned.png",
    "....//....//pwned.png",
    "a/b/c.png",
])
def test_client_filename_cannot_steer_the_destination(auth, filename):
    # The name is accepted but ignored: the server picks its own. What matters
    # is that the upload is still retrievable and nothing leaks the path.
    pic_id = upload_ok(auth, content=PNG_REAL, filename=filename)
    r = auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}")
    assert r.status_code == 200
    assert r.content == PNG_REAL


def test_profile_exposes_ids_not_server_paths(auth):
    pic_id = upload_ok(auth)
    r = auth.get(f"{BASE_URL}/api/profile/get")
    assert r.status_code == 200, r.text

    pictures = r.json()["pictures"]
    assert [p["id"] for p in pictures] == [pic_id]
    # A filesystem path anywhere in here is an information leak.
    assert "/" not in str(pictures)


def test_same_filename_from_two_users_stays_separate(other, auth):
    # Both used to land on one file, so a GET of user A's row returned B's bytes.
    a_bytes = b"\x89PNG\r\n\x1a\n" + b"AAAA-user-a"
    b_bytes = b"\x89PNG\r\n\x1a\n" + b"BBBB-user-b"

    a_id = upload_ok(auth, content=a_bytes, filename="cat.png")
    b_id = upload_ok(other, content=b_bytes, filename="cat.png")

    assert a_id != b_id
    assert auth.get(f"{BASE_URL}/api/pics/get?id={a_id}").content == a_bytes
    assert other.get(f"{BASE_URL}/api/pics/get?id={b_id}").content == b_bytes


# --------------------------------------------------------------------------
# Content validation. The spec lists "permitting the upload of unauthorized
# content" among the vulnerabilities that score 0, and all of these were
# accepted with a 200.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("content,filename,mime", [
    (b"<script>alert(document.cookie)</script>", "evil.html", "text/html"),
    (b"<?php system($_GET['c']); ?>", "shell.php", "application/x-php"),
    (b"#!/bin/sh\nrm -rf /\n", "x.sh", "application/x-sh"),
    (b"not an image at all " * 15, "fake.jpg", "image/jpeg"),
    (b"%PDF-1.4 fake pdf", "doc.pdf", "application/pdf"),
    (b"\x89PNG\r\n", "truncated.png", "image/png"),
])
def test_non_image_content_is_refused(auth, content, filename, mime):
    r = upload(auth, content=content, filename=filename, content_type=mime)
    assert r.status_code == 415, r.text


def test_declared_mime_cannot_override_the_real_bytes(auth):
    # Claiming image/png for a shell script must not help.
    r = upload(auth, content=b"#!/bin/sh\necho hi\n", filename="a.png",
               content_type="image/png")
    assert r.status_code == 415, r.text


# --------------------------------------------------------------------------
# Malformed requests must be 4xx, not 500: the spec requires appropriate
# status codes, and multipart_parser_execute's return value was ignored.
# --------------------------------------------------------------------------
def _raw_post(session, body, content_type):
    return session.post(f"{BASE_URL}/api/pics/insert", data=body,
                        headers={"Content-Type": content_type})


def test_json_body_is_400(auth):
    r = auth.post(f"{BASE_URL}/api/pics/insert", json={"file": "nope"})
    assert r.status_code == 400, r.text


def test_garbage_multipart_body_is_400(auth):
    r = _raw_post(auth, b"total garbage not multipart at all",
                  "multipart/form-data; boundary=xyz")
    assert r.status_code == 400, r.text


def test_missing_boundary_is_400(auth):
    r = _raw_post(auth, b"--xyz\r\n\r\n--xyz--\r\n", "multipart/form-data")
    assert r.status_code == 400, r.text


def test_part_without_a_filename_is_400(auth):
    body = (b'--xyz\r\nContent-Disposition: form-data; name="caption"\r\n\r\n'
            b'hello\r\n--xyz--\r\n')
    r = _raw_post(auth, body, "multipart/form-data; boundary=xyz")
    assert r.status_code == 400, r.text


def test_empty_file_part_is_400(auth):
    body = (b'--xyz\r\nContent-Disposition: form-data; name="file"; filename="a.png"\r\n'
            b'Content-Type: image/png\r\n\r\n\r\n--xyz--\r\n')
    r = _raw_post(auth, body, "multipart/form-data; boundary=xyz")
    assert r.status_code == 400, r.text


# The empty filename is the one that used to kill the process: it recorded the
# upload directory as the file, and the next GET malloc'd LONG_MAX.
def test_empty_filename_is_rejected_and_server_survives(auth):
    body = (b'--xyz\r\nContent-Disposition: form-data; name="file"; filename=""\r\n'
            b'Content-Type: image/png\r\n\r\ndata\r\n--xyz--\r\n')
    r = _raw_post(auth, body, "multipart/form-data; boundary=xyz")
    assert r.status_code in (400, 415), r.text

    # Nothing recorded, and the process is still answering. The old code stored
    # a row naming the upload directory itself, and the next GET of that id did
    # malloc(LONG_MAX) and took the server down.
    assert auth.get(f"{BASE_URL}/api/profile/get").json()["pictures"] == []
    assert auth.get(f"{BASE_URL}/api/validate").status_code == 200


@pytest.mark.parametrize("content_type", [
    'multipart/form-data; boundary=xyz',
    'multipart/form-data; boundary="xyz"',
    'multipart/form-data; boundary=xyz; charset=utf-8',
    'multipart/form-data;boundary=xyz',
])
def test_boundary_spellings_all_work(auth, content_type):
    # Only the bare unquoted form used to parse; the rest failed with a 500.
    body = (b'--xyz\r\nContent-Disposition: form-data; name="file"; filename="a.png"\r\n'
            b'Content-Type: image/png\r\n\r\n' + PNG + b'\r\n--xyz--\r\n')
    r = _raw_post(auth, body, content_type)
    assert r.status_code == 201, f"{content_type} -> {r.status_code} {r.text}"


# --------------------------------------------------------------------------
# Size limits. An oversized body used to reset the connection with no response.
# --------------------------------------------------------------------------
def test_picture_over_the_cap_is_413(auth):
    big = b"\x89PNG\r\n\x1a\n" + b"A" * (MAX_PIC_BYTES + 1024)
    r = upload(auth, content=big, filename="big.png")
    assert r.status_code == 413, r.text


def test_body_over_the_transport_cap_is_413_not_a_reset(auth):
    huge = b"\x89PNG\r\n\x1a\n" + b"A" * (HTTP_MAX_BODY + 1024)
    r = upload(auth, content=huge, filename="huge.png")
    assert r.status_code == 413, r.text


def test_picture_just_under_the_cap_is_accepted(auth):
    ok = b"\x89PNG\r\n\x1a\n" + b"A" * (MAX_PIC_BYTES - 4096)
    r = upload(auth, content=ok, filename="nearly.png")
    assert r.status_code == 201, r.text


def test_large_picture_survives_the_round_trip(auth):
    # A response larger than the socket send buffer used to lose its tail: the
    # server never requested EPOLLOUT after answering, so whatever the conn
    # layer had left queued was never written. Random bytes so a truncation
    # cannot accidentally compare equal.
    big = b"\x89PNG\r\n\x1a\n" + os.urandom(MAX_PIC_BYTES - 65536)
    pic_id = upload_ok(auth, content=big, filename="big.png")

    r = auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}")
    assert r.status_code == 200, r.text
    assert len(r.content) == len(big)
    assert r.content == big


# --------------------------------------------------------------------------
# Spec IV.2: "Up to 5 pictures, including one designated as the profile
# picture." Neither half was enforced or implemented.
# --------------------------------------------------------------------------
def test_sixth_picture_is_refused(auth):
    for i in range(MAX_PICS_PER_USER):
        assert upload(auth, filename=f"p{i}.png").status_code == 201

    r = upload(auth, filename="sixth.png")
    assert r.status_code == 409, r.text
    assert "5" in r.text


def test_cap_frees_up_after_a_delete(auth):
    ids = [upload_ok(auth, filename=f"p{i}.png") for i in range(MAX_PICS_PER_USER)]
    assert upload(auth).status_code == 409

    assert auth.post(f"{BASE_URL}/api/pics/delete", json={"id": ids[0]}).status_code == 200
    assert upload(auth).status_code == 201


def test_first_upload_becomes_the_profile_picture(auth):
    # Spec IV.5 blocks liking anyone without a profile picture, and nothing in
    # the codebase ever set the flag, so every user was stuck.
    first = upload_ok(auth)
    second = upload_ok(auth)

    pictures = {p["id"]: p["is_profile"]
                for p in auth.get(f"{BASE_URL}/api/profile/get").json()["pictures"]}
    assert pictures[first] is True
    assert pictures[second] is False


def test_set_profile_moves_the_flag(auth):
    first = upload_ok(auth)
    second = upload_ok(auth)

    r = auth.post(f"{BASE_URL}/api/pics/set_profile", json={"id": second})
    assert r.status_code == 200, r.text

    pictures = {p["id"]: p["is_profile"]
                for p in auth.get(f"{BASE_URL}/api/profile/get").json()["pictures"]}
    assert pictures[second] is True
    assert pictures[first] is False
    assert sum(pictures.values()) == 1


def test_deleting_the_profile_picture_promotes_another(auth):
    first = upload_ok(auth)
    second = upload_ok(auth)

    assert auth.post(f"{BASE_URL}/api/pics/delete", json={"id": first}).status_code == 200

    pictures = auth.get(f"{BASE_URL}/api/profile/get").json()["pictures"]
    assert [p["id"] for p in pictures] == [second]
    assert pictures[0]["is_profile"] is True


def test_delete_removes_the_picture(auth):
    pic_id = upload_ok(auth)
    assert auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}").status_code == 200

    r = auth.post(f"{BASE_URL}/api/pics/delete", json={"id": pic_id})
    assert r.status_code == 200, r.text

    assert auth.get(f"{BASE_URL}/api/pics/get?id={pic_id}").status_code == 404
    assert auth.get(f"{BASE_URL}/api/profile/get").json()["pictures"] == []


# --------------------------------------------------------------------------
# Ownership. Answering 404 rather than 403 for someone else's picture keeps the
# id space from being enumerable.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("route", ["/api/pics/set_profile", "/api/pics/delete"])
def test_cannot_touch_another_users_picture(auth, other, route):
    victim_id = upload_ok(other, content=PNG_REAL)

    r = auth.post(f"{BASE_URL}{route}", json={"id": victim_id})
    assert r.status_code == 404, r.text

    # Still there, still the victim's.
    assert other.get(f"{BASE_URL}/api/pics/get?id={victim_id}").content == PNG_REAL


@pytest.mark.parametrize("route", ["/api/pics/set_profile", "/api/pics/delete"])
@pytest.mark.parametrize("payload", [{}, {"id": "abc"}, {"id": -1}, {"id": 0}, {"id": None}])
def test_bad_id_payload_is_400(auth, route, payload):
    r = auth.post(f"{BASE_URL}{route}", json=payload)
    assert r.status_code == 400, r.text


@pytest.mark.parametrize("route", ["/api/pics/set_profile", "/api/pics/delete"])
def test_unknown_id_is_404(auth, route):
    r = auth.post(f"{BASE_URL}{route}", json={"id": 2_000_000_000})
    assert r.status_code == 404, r.text


# --------------------------------------------------------------------------
# Route contract for the two new endpoints.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("method,path", [
    ("GET", "/api/pics/set_profile"),
    ("GET", "/api/pics/delete"),
])
def test_wrong_method_is_405(auth, method, path):
    assert auth.request(method, f"{BASE_URL}{path}").status_code == 405
    assert auth.get(f"{BASE_URL}/api/validate").status_code == 200


@pytest.mark.parametrize("path", [
    "/api/pics/insert",
    "/api/pics/get?id=1",
    "/api/pics/set_profile",
    "/api/pics/delete",
])
def test_routes_require_authentication(path):
    s = new_session()
    method = "GET" if "get?" in path else "POST"
    r = s.request(method, f"{BASE_URL}{path}", json={"id": 1})
    assert r.status_code in (401, 403), f"{path} -> {r.status_code}"


def test_get_requires_a_usable_id(auth):
    assert auth.get(f"{BASE_URL}/api/pics/get?id=abc").status_code == 400
    assert auth.get(f"{BASE_URL}/api/pics/get?id=-1").status_code == 400
    assert auth.get(f"{BASE_URL}/api/pics/get?other=1").status_code == 400
