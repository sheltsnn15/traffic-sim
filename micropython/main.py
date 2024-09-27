import gc
import usocket as socket
from ubinascii import b2a_base64
import hashlib
import network
import ujson
import time
from machine import UART

WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
uart = UART(1, baudrate=115200, tx=17, rx=16)

def log(message):
    print(f"[INFO] {message}")


def read_config(filename='secrets.json'):
    """Reads configuration from a JSON file."""
    try:
        with open(filename, "r") as file:
            return ujson.load(file)
    except (OSError, ValueError) as e:
        log(f"Failed to load configuration: {e}")
        return {}


def connect_wifi(ssid, password):
    """Connects to the Wi-Fi network with the provided SSID and password."""
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)

    timeout = 10  # Wait up to 10 seconds
    while not wlan.isconnected() and timeout > 0:
        log("Attempting to connect...")
        time.sleep(1)
        timeout -= 1

    if wlan.isconnected():
        log(f"Connected to {ssid}")
        log(f"Network config: {wlan.ifconfig()}")
    else:
        log("Failed to connect to Wi-Fi")


def html_response():
    """Generates a basic HTML response."""
    return b"""
        <html>
            <head><title>ESP32 Web Server</title></head>
            <body><h1>Hello from ESP32</h1></body>
        </html>
    """


def read_uart_data():
    """Reads data from UART and returns it."""
    if uart.any():  # Check if there's any data to read
        data = uart.read().decode('utf-8').strip()  # Read and decode the data
        return data  # Return the data as a string
    return None  # If no data, return None


def get_traffic_state():
    """Return a JSON with the current state of the traffic lights."""
    traffic_data = read_uart_data()  # Get the latest traffic data from UART
    if traffic_data:
        state = {
            'Y_Junction': traffic_data,  # For now, all data comes for Y-Junction
        }
    else:
        state = {'error': 'No data available from UART'}
    return ujson.dumps(state)


def start_webserver(port):
    """Starts a basic HTTP server."""
    addr = socket.getaddrinfo("0.0.0.0", port)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)  # Limit to one connection at a time

    log(f"Web server listening on {addr}")

    while True:
        cl, addr = s.accept()
        log(f"Client connected from {addr}")
        handle_client(cl)


def generate_accept_key(websocket_key):
    """Generates the WebSocket accept key based on the provided websocket_key."""
    key = websocket_key + WEBSOCKET_GUID
    sha1_hash = hashlib.sha1(key.encode()).digest()
    return b2a_base64(sha1_hash).decode('utf-8').strip()


def handshake(client):
    """Performs the WebSocket handshake."""
    request = client.recv(1024).decode('utf-8')
    headers = request.split("\r\n")
    websocket_key = None
    for header in headers:
        if header.startswith("Sec-WebSocket-Key"):
            websocket_key = header.split(": ")[1]
            break

    if websocket_key:
        websocket_accept = generate_accept_key(websocket_key)
        response = (
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Accept: {websocket_accept}\r\n\r\n"
        )
        client.send(response.encode('utf-8'))
    else:
        log("Failed to perform WebSocket handshake")


def start_websocket_server(port):
    """Starts a WebSocket server."""
    addr = socket.getaddrinfo("0.0.0.0", port)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)
    log(f"WebSocket Server listening on {addr}")

    while True:
        client, addr = s.accept()
        log(f"Client connected from {addr}")
        handshake(client)

        try:
            while True:
                data = client.recv(1024)
                if not data:
                    break
                log(f"Received: {data}")
                client.send(data)  # Echo back the received data
        except OSError:
            log("Connection closed")
        client.close()


def handle_client(cl):
    """Handle incoming client connections."""
    request = cl.recv(1024).decode('utf-8')

    if request.startswith('GET /traffic_state'):
        response = get_traffic_state()
        cl.send(b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n")
        cl.sendall(response.encode('utf-8'))
    else:
        cl.send(b"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n")
        cl.sendall(html_response())

    cl.close()


def main():
    config = read_config()
    ssid = config.get('SSID')
    password = config.get('PASSWORD')
    port = config.get('PORT', 80)

    if ssid and password:
        connect_wifi(ssid, password)
        start_webserver(port)
        # To use WebSocket server instead, comment out start_webserver() and uncomment the next line:
        # start_websocket_server(port)
    else:
        log("Wi-Fi credentials not found or incomplete.")


if __name__ == "__main__":
    main()
    gc.collect()  # Run garbage collector
