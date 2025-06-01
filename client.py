import websocket
import ssl
import sys

def on_message(ws, message):
    print("Received:", message)

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

def on_open(ws):
    print("Connected, sending message...")
    ws.send("Hello from Python client!")
    ws.close()

if __name__ == "__main__":
    # Disable certificate validation for testing self-signed certs

    port = sys.argv[1] if len(sys.argv) > 1 else "8080"
    ssl_opts = {
        "cert_reqs": ssl.CERT_NONE
    }

    ws = websocket.WebSocketApp("wss://localhost:" + port + "/",
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)

    ws.run_forever(sslopt=ssl_opts)
