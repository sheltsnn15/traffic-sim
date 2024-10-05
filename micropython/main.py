import gc
import usocket as socket
from ubinascii import b2a_base64
import hashlib
import network
import ujson
import time
from machine import UART

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

    while not wlan.isconnected():
        time.sleep(0.5)
        log("Connecting to Wi-Fi...")


def preprocess_template(template_str, context):
    """Replaces placeholders in in the template with actual values"""
    for key, value in context.items():
        template_str = template_str.replace('{{key}}', value)
    return template_str.encode('utf8')


def preprocess_html(esp32_ip):
    """Reads index.html file, replaces placeholders, and caches the result."""
    try:
        with open('index.html', 'r') as file:
            html = file.read()

            # Prepare context for placeholders
            context = {
                'ESP32-IP': esp32_ip,
            }

            # Replace placeholders and return the result
            return preprocess_template(html, context)
    except OSError:
        return b"Error: index.html file not found."


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


def start_webserver(port, cached_html):
    """Starts a basic HTTP server."""
    addr = socket.getaddrinfo("0.0.0.0", port)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)  # Limit to one connection at a time

    log(f"Web server listening on {addr}")

    while True:
        cl, addr = s.accept()
        log(f"Client connected from {addr}")
        handle_client(cl, cached_html)


def generate_accept_key(websocket_key, websocket_guid):
    """Generates the WebSocket accept key based on the provided websocket_key and guid."""
    key = websocket_key + websocket_guid
    sha1_hash = hashlib.sha1(key.encode()).digest()
    return b2a_base64(sha1_hash).decode('utf-8').strip()


def handshake(client, websocket_guid):
    """Performs the WebSocket handshake."""
    request = client.recv(1024).decode('utf-8')
    headers = request.split("\r\n")
    websocket_key = None
    for header in headers:
        if header.startswith("Sec-WebSocket-Key"):
            websocket_key = header.split(": ")[1]
            break

    if websocket_key:
        websocket_accept = generate_accept_key(websocket_key, websocket_guid)
        response = (
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Accept: {websocket_accept}\r\n\r\n"
        )
        client.send(response.encode('utf-8'))
    else:
        log("Failed to perform WebSocket handshake")


def start_websocket_server(port, websocket_guid):
    """Starts a WebSocket server."""
    addr = socket.getaddrinfo("0.0.0.0", port)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)
    log(f"WebSocket Server listening on {addr}")

    while True:
        client, addr = s.accept()
        log(f"Client connected from {addr}")
        handshake(client, websocket_guid)

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


def handle_client(cl, cached_html):
    """Handle incoming client connections."""
    request = cl.recv(1024).decode('utf-8')

    if request.startswith('GET /traffic_state'):
        response = get_traffic_state()
        cl.send(b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n")
        cl.sendall(response.encode('utf-8'))
    else:
        cl.send(b"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n")
        cl.sendall(cached_html)

    cl.close()


def main():
    config = read_config()
    ssid = config.get('SSID')
    password = config.get('PASSWORD')
    port = config.get('PORT', 80)
    esp32_ip = config.get('ESP_IP')
    websocket_guid = config.get('WEBSOCKET_GUID')

    if ssid and password:
        connect_wifi(ssid, password)
                # Preprocess the HTML once, injecting the IP address
        cached_html = preprocess_html(esp32_ip)
        start_webserver(port, cached_html)
        # To use WebSocket server instead, comment out start_webserver() and uncomment the next line:
        # start_websocket_server(port, websocket_guid)
    else:
        log("Wi-Fi credentials not found or incomplete.")


if __name__ == "__main__":
    main()
    gc.collect()  # Run garbage collector
