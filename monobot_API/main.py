import socket
import time
from pynput import keyboard
from threading import Thread

# Set the Arduino's IP and port
ARDUINO_IP = "monobot.local"  # Replace with your Arduino's IP
#ARDUINO_IP = "192.168.91.122"  # Replace with your Arduino's IP
ARDUINO_PORT = 80  # Replace if you changed the server port on Arduino

current_key = None  # Store the currently pressed key

# Create a TCP/IP socket
arduino = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
arduino.settimeout(5)  # Timeout for connection attempts
arduino.connect((ARDUINO_IP, ARDUINO_PORT))
arduino.settimeout(None)  # Once connected, can set no timeout or a read timeout


def send_command():
    global current_key
    while True:
        if current_key:  # Only send if a key is currently held down
            # Send the key to Arduino
            # Arduino expects single-character commands: 'w', 'a', 'd'
            try:
                arduino.sendall(current_key.encode())
            except (BrokenPipeError, OSError):
                # Connection lost
                break
        time.sleep(0.001)  # Adjust delay for command repetition rate


def read_logs():
    buffer = ""  # Buffer for partial lines
    while True:
        # Try to receive data from Arduino
        try:
            data = arduino.recv(1024)
            if not data:
                # Connection closed by Arduino
                break
            buffer += data.decode('utf-8', errors='replace')
            # Parse complete lines
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip('\r')
                if line:
                    print(f"Arduino Log: {line}")
        except (BrokenPipeError, OSError):
            # Connection lost or read error
            break
        except socket.timeout:
            # No data available, continue
            pass

        time.sleep(0.01)  # Small delay to reduce busy-waiting


def on_press(key):
    global current_key
    try:
        if key.char in ['w', 'a', 'd']:  # Only allow valid commands
            current_key = key.char  # Set the current key
    except AttributeError:
        # Special keys (like arrow keys, shift, ctrl) are ignored
        pass


def on_release(key):
    global current_key
    if hasattr(key, 'char') and key.char in ['w', 'a', 'd']:
        current_key = None  # Stop sending commands when the key is released
    if key == keyboard.Key.esc:  # Exit the script with Esc
        # Close the socket
        arduino.close()
        return False


# Start the thread to read logs
log_thread = Thread(target=read_logs, daemon=True)
log_thread.start()

# Start the thread to send commands
send_thread = Thread(target=send_command, daemon=True)
send_thread.start()

# Listen for keyboard events
with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
    listener.join()
