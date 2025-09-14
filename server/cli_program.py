import threading
import asyncio
import pickle
import os
import json
from getpass import getpass
import requests
import socketio
import websockets
import time

# Constants
BASE_URL = "http://localhost:8080"
COOKIE_FILE = "cookies.pkl"


def save_cookies(session: requests.Session):
    with open(COOKIE_FILE, "wb") as f:
        pickle.dump(session.cookies, f)


def load_cookies(session: requests.Session):
    if os.path.exists(COOKIE_FILE):
        with open(COOKIE_FILE, "rb") as f:
            session.cookies.update(pickle.load(f))


def get_session() -> requests.Session:
    session = requests.Session()
    load_cookies(session)
    return session


# REST functions

def do_upload_picture(session):
    print("-- Upload Picture --")
    file_path = input("Path to picture: ").strip()
    if not os.path.exists(file_path):
        print("File does not exist.")
        return

    try:
        with open(file_path, "rb") as f:
            files = {"file": (os.path.basename(file_path), f, "image/jpeg")}
            resp = session.post(f"{BASE_URL}/api/pics/insert", files=files)
            print("Sent headers:", resp.request.headers)
            print(resp.status_code, resp.text)
    except Exception as e:
        print(f"Upload failed: {e}")


def do_get_picture(session):
    print("-- Get Picture --")
    pic_id = input("Picture ID: ").strip()
    if not pic_id.isdigit():
        print("Invalid picture ID.")
        return

    resp = session.get(f"{BASE_URL}/api/pics/get/?id={pic_id}")
    if resp.status_code == 200:
        filename = f"downloaded_{pic_id}.jpg"
        with open(filename, "wb") as f:
            f.write(resp.content)
        print(f"Picture saved as {filename}")
    else:
        print(resp.status_code, resp.text)


def do_register(session):
    print("-- Register new user --")
    username = input("Username: ").strip()
    email = input("Email: ").strip()
    password = getpass("Password: ")
    first_name = input("First name: ").strip()
    last_name = input("Last name: ").strip()
    payload = {"username": username, "email": email,
               "password": password, "first_name": first_name, "last_name": last_name}
    resp = session.post(f"{BASE_URL}/api/register", json=payload)
    print(resp.status_code, resp.text)


def do_get_chat_messages(session):
    print("-- Get Chat Messages --")
    chat_id = input("Chat ID: ").strip()
    resp = session.get(f"{BASE_URL}/api/chat/messages/?id={chat_id}")
    print(resp.status_code, resp.text)

def do_get_insert_tags(session):
    print("-- Insert Tags --")
    tags = input("Tags (comma-separated): ").strip()

    try:
        tags_list = [int(tag.strip()) for tag in tags.split(",") if tag.strip().isdigit()]
    except ValueError:
        print("Invalid input. Please enter only numeric tags.")
        return

    resp = session.post(f"{BASE_URL}/api/tags/insert", json={"tags": tags_list})
    print(resp.status_code, resp.text)

def do_get_user_profile(session):
    print("-- Get User Profile --")
    user_id = input("User ID: ").strip()
    start_time = time.time()  # Record the start time
    if user_id.isdigit() and int(user_id) > 0:
        resp = session.get(f"{BASE_URL}/api/profile/get/?id={user_id}")
    else:
        resp = session.get(f"{BASE_URL}/api/profile/get")
    end_time = time.time()  # Record the end time
    elapsed_time = end_time - start_time  # Calculate the elapsed time
    print(f"Response time: {elapsed_time:.2f} seconds")  # Print the response time
    print(resp.status_code, resp.text)

def do_login(session):
    print("-- Login --")
    email = input("Email: ").strip()
    password = getpass("Password: ")
    payload = {"email": email, "password": password}
    resp = session.post(f"{BASE_URL}/api/login", json=payload)
    print(resp.status_code, resp.text)
    if resp.status_code == 200:
        save_cookies(session)


def do_validate(session):
    print("-- Validate session --")
    resp = session.get(f"{BASE_URL}/api/validate")
    print(resp.status_code, resp.text)


def do_logout(session):
    print("-- Logout --")
    resp = session.post(f"{BASE_URL}/api/logout")
    print(resp.status_code, resp.text)
    if resp.status_code == 200:
        if os.path.exists(COOKIE_FILE):
            os.remove(COOKIE_FILE)
        session.cookies.clear()


# WebSocket handler
async def ws_listener(uri):
    try:
        async with websockets.connect(uri) as ws:
            print(f"[WS] Connected to {uri}. Press Ctrl+C to quit.")
            while True:
                msg = await ws.recv()
                print(f"[WS] Received: {msg}")
    except Exception as e:
        print(f"[WS] Connection closed: {e}")


def ws_mode():
    uri = input("WebSocket URI (default ws://localhost:8000/ws): ").strip() or f"ws://localhost:8000/ws"
    loop = asyncio.new_event_loop()
    t = threading.Thread(target=lambda: loop.run_until_complete(ws_listener(uri)), daemon=True)
    t.start()
    input("Press Enter to stop WebSocket listener and return to menu...\n")
    loop.stop()


# Socket.IO handler and thread-safe emit queue
sio_client = None
emit_queue = []
emit_lock = threading.Lock()

def start_sio(uri):
    global sio_client
    sio_client = socketio.Client()

    # Load session cookies and prepare Cookie header
    session = get_session()
    cookies = session.cookies.get_dict()
    if cookies:
        cookie_header = "; ".join(f"{k}={v}" for k, v in cookies.items())
        headers = {'Cookie': cookie_header}
    else:
        headers = {}

    @sio_client.event
    def connect():
        print("[SIO] Connected.")

    @sio_client.event
    def disconnect():
        print("[SIO] Disconnected.")

    @sio_client.on("*")
    def catch_all(event, data):
        print(f"[SIO] Event '{event}': {data}")

    # Connect with cookie header for auth
    sio_client.connect(uri, transports=['websocket'], headers=headers)

    try:
        while sio_client.connected:
            with emit_lock:
                while emit_queue:
                    event, data = emit_queue.pop(0)
                    try:
                        sio_client.emit(event, data)
                        print(f"[SIO] Emitted event '{event}' with {data}")
                    except Exception as e:
                        print(f"[SIO] Emit error: {e}")
            sio_client.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        sio_client.disconnect()


def sio_mode():
    uri = input("Socket.IO server URL (default http://localhost:8080): ").strip() or "http://localhost:8080"
    t = threading.Thread(target=start_sio, args=(uri,), daemon=True)
    t.start()
    print("[SIO] Listener started in background.")
    while True:
        cmd = input("SIO> ").strip()
        if cmd.lower() in ("exit", "quit"): break
        if cmd.startswith("emit "):
            try:
                _, event, data = cmd.split(" ", 2)
                # data = json.loads(json_data)
                with emit_lock:
                    emit_queue.append((event, data))
            except Exception as e:
                print(f"Invalid emit command or JSON: {e}")
        else:
            print("Commands: emit <event> <json_data>, exit")
    print("Stopping Socket.IO...")


def print_menu():
    print("\n=== anon-reports CLI ===")
    print("1) Register (/api/register)")
    print("2) Login (/api/login)")
    print("3) Validate (/api/validate)")
    print("4) Logout (/api/logout)")
    print("5) Connect WebSocket (/ws)")
    print("6) Connect Socket.IO (/socket.io)")
    print("7) Get Chat Messages (/api/chat/messages/?id=<chat_id>)")
    print("8) Get User Profile (/api/profile/get/?id=<user_id>)")
    print("9) Insert Tags (/api/tags/insert)")
    print("10) Upload Picture (/api/pics/insert)")
    print("11) Get Picture (/api/pics/get/?id=<pic_id>)")
    print("0) Exit")


def main():
    session = get_session()
    actions = {
        "1": lambda: do_register(session),
        "2": lambda: do_login(session),
        "3": lambda: do_validate(session),
        "4": lambda: do_logout(session),
        "5": ws_mode,
        "6": sio_mode,
        "7": lambda: do_get_chat_messages(session),
        "8": lambda: do_get_user_profile(session),
        "9": lambda: do_get_insert_tags(session),
        "10": lambda: do_upload_picture(session),
        "11": lambda: do_get_picture(session),
    }
    while True:
        print_menu()
        choice = input("Select an option: ").strip()
        if choice == "0":
            print("Goodbye!")
            break
        action = actions.get(choice)
        if action:
            action()
        else:
            print("Invalid choice, try again.")

if __name__ == "__main__":
    main()
