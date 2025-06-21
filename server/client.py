import websocket
import threading
import sys

def on_message(ws, message):
    print("Received:", message)

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

def on_open(ws):
    print("Connected to the server. You can now send messages.")
    # Keep the connection open and allow interactive input

def send_messages(ws):
    try:
        while True:
            message = input("Enter message: ")
            if message.lower() == "exit":
                print("Closing connection...")
                ws.close()
                break
            ws.send(message)
    except KeyboardInterrupt:
        print("\nClosing connection...")
        ws.close()

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "8080"

    ws = websocket.WebSocketApp("ws://localhost:" + port + "/",
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)

    # Run the WebSocket in a separate thread to allow interactive input
    thread = threading.Thread(target=ws.run_forever)
    thread.daemon = True
    thread.start()

    send_messages(ws)