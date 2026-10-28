import socketio
import threading
import time

# DOES NOT WORK AS IT NEEDS COOKIE TO WORK
NUM_CLIENTS = 1  # Modify this value to change the number of clients

def create_client(client_id):
    sio = socketio.Client()

    @sio.event
    def connect():
        print(f"Client {client_id} connected to the server.")

    @sio.event
    def disconnect():
        print(f"Client {client_id} disconnected from the server.")

    @sio.on('message')
    def on_message(data):
        print(f"Client {client_id} received: {data}")

    try:
        sio.connect('http://localhost:8080', transports=['websocket'])
        print(f"Client {client_id} connecting to the server...")
    except socketio.exceptions.ConnectionError as e:
        print(f"Client {client_id} failed to connect: {e}")
        return  # Exit the function gracefully if connection fails

    try:
        while True:
            message = input(f"Client {client_id} - Enter message: ")
            if message.lower() == "exit":
                print(f"Client {client_id} closing connection...")
                sio.disconnect()
                break
            sio.send(message)
    except KeyboardInterrupt:
        print(f"\nClient {client_id} closing connection...")
        sio.disconnect()

if __name__ == "__main__":
    threads = []

    for i in range(NUM_CLIENTS):
        thread = threading.Thread(target=create_client, args=(i,))
        thread.start()
        threads.append(thread)
        time.sleep(0.1)  # Slight delay to avoid overwhelming the server

    for thread in threads:
        thread.join()