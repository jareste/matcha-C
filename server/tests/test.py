import os
import pickle
import pytest
import requests
import time
import warnings

BASE_URL = "http://localhost:8080"
COOKIE_FILE = "cookies.pkl"

def save_cookies(session: requests.Session):
    with open(COOKIE_FILE, "wb") as f:
        pickle.dump(session.cookies, f)

def load_cookies(session: requests.Session):
    if os.path.exists(COOKIE_FILE):
        with open(COOKIE_FILE, "rb") as f:
            session.cookies.update(pickle.load(f))

@pytest.fixture(scope="session")
def session():
    s = requests.Session()
    # load_cookies(s)
    return s

payload = {
        "username": "pytest_user",
        "email": "pytest_user@example.com",
        "password": "pytest_password",
        "first_name": "Py",
        "last_name": "Test"
    }


def aux_login(session):
    login_payload = {"email": payload["email"], "password": payload["password"]}
    headers = {"Connection": "close"}
    resp = session.post(f"{BASE_URL}/api/login", json=login_payload, headers=headers)
    print(resp.status_code, resp.text)
    assert resp.status_code == 200
    time.sleep(0.1)
    # save_cookies(session)

def test_register_and_login(session):
    time.sleep(0.1)
    max_retries = 5
    retry_delay = 0.2

    for attempt in range(max_retries):
        try:
            resp = session.post(f"{BASE_URL}/api/register", json=payload)
            print(resp.status_code, resp.text)
            if resp.status_code == 400:
                assert "already exists" in resp.text
            else:
                assert resp.status_code == 200

            time.sleep(0.2)

            login_payload = {"email": payload["email"], "password": payload["password"]}
            resp = session.post(f"{BASE_URL}/api/login", json=login_payload)
            assert resp.status_code == 200

            break
        except requests.exceptions.ConnectionError as e:
            print(f"ConnectionError occurred: {e}. Retrying in {retry_delay} seconds...")
            time.sleep(retry_delay)
    else:
        warnings.warn("Max retries reached. Unable to confirm if the server is reachable.")

def test_validate(session):
    time.sleep(0.1)
    aux_login(session)

    resp = session.get(f"{BASE_URL}/api/validate")
    print(resp.status_code, resp.text)
    assert resp.status_code == 200
    data = resp.json()
    assert "message" in data and "User is valid" in data["message"]
    assert "username" in data and data["username"] == "pytest_user"

def test_get_user_profile(session):
    # Get self profile (no id)
    time.sleep(0.1)
    resp = session.get(f"{BASE_URL}/api/profile/get")
    assert resp.status_code == 200
    data = resp.json()
    assert "id" in data and "username" in data

def test_insert_tags(session):
    time.sleep(0.1)
    aux_login(session)

    payload = {"tags": [1, 2, 3]}
    resp = session.post(f"{BASE_URL}/api/tags/insert", json=payload)
    print(resp.status_code, resp.text)
    assert resp.status_code in (200, 201)
    data = resp.json()
    assert "message" in data and data["message"] == "Tags successfully added"

def test_forbidden(session):
    time.sleep(0.1)
    payload = {"tags": [1, 2, 3]}
    #ensure logout
    resp = session.post(f"{BASE_URL}/api/logout")
    #now try to insert tags without being logged in
    resp = session.post(f"{BASE_URL}/api/tags/insert", json=payload)
    print(resp.status_code, resp.text)
    assert resp.status_code == 403

def test_chat_messages(session):
    time.sleep(0.1)
    aux_login(session)
    resp = session.get(f"{BASE_URL}/api/chat/messages/?id=1")
    assert resp.status_code in (200, 404)
    if resp.status_code == 200:
        data = resp.json()
        print(data)
        assert "messages" in data
        assert isinstance(data["messages"], list)

def test_logout(session):
    time.sleep(0.1)
    aux_login(session)
    resp = session.post(f"{BASE_URL}/api/logout")
    assert resp.status_code == 200
    if os.path.exists(COOKIE_FILE):
        os.remove(COOKIE_FILE)
    session.cookies.clear()


