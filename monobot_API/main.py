import socket
import time
from pynput import keyboard
from threading import Thread

# Set the Arduino's IP and port
ARDUINO_IP = "monobot.local"  # Replace with your Arduino's IP or mDNS name
ARDUINO_PORT = 80             # Must match the Arduino server port

# Global variables
current_key = None       # Currently pressed key
config_received = False  # Flag indicating that the GO signal has been received
abort = False            # Flag indicating that the user wants to abort (by pressing Esc)

# --------------------------------------------------
# Attempt connection to the Arduino repeatedly until
# a connection is established or abort is signaled.
# --------------------------------------------------
def on_press_connect(key):
    global abort
    if key == keyboard.Key.esc:
        print("ESC pressed - aborting connection attempts.")
        abort = True
        return False  # Stop the listener

# Start a listener for ESC during connection attempts
connect_listener = keyboard.Listener(on_press=on_press_connect)
connect_listener.start()

arduino = None
while arduino is None and not abort:
    try:
        print(f"Attempting to connect to {ARDUINO_IP}:{ARDUINO_PORT}...")
        arduino = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        arduino.settimeout(5)  # Timeout for connection attempts
        arduino.connect((ARDUINO_IP, ARDUINO_PORT))
        arduino.settimeout(None)  # Remove timeout after successful connection
        print("Connection established!")
    except Exception as e:
        print("Connection failed, retrying in 2 seconds...")
        time.sleep(2)
        arduino = None

connect_listener.stop()

if abort:
    print("User aborted. Exiting program.")
    exit(0)

# --------------------------------------------------
# Rest of the program
# --------------------------------------------------
def send_config():
    """
    Reads the configuration file (config.json), adds start and end markers,
    and sends the resulting string to the robot.
    """
    try:
        with open("config.json", "r") as f:
            config_text = f.read()
    except FileNotFoundError:
        print("config.json not found!")
        return

    # Append markers to indicate the start and end of the configuration data
    config_message = "CONFIG:" + config_text + chr(0x1E)

    # Send the configuration data to the Arduino
    arduino.sendall(config_message.encode())
    print("Configuration sent.")

def wait_for_go():
    """
    Waits until the GO signal is received from the robot.
    The signal is expected to be sent as a log message containing the string "GO".
    """
    global config_received
    buffer = ""
    print("Waiting for GO signal from Arduino...")
    while not config_received:
        try:
            data = arduino.recv(1024)
            if not data:
                # Connection closed by Arduino
                break
            buffer += data.decode("utf-8", errors="replace")
            # Process complete lines
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip('\r')
                if "GO" in line:
                    config_received = True
                    print("GO signal received from Arduino.")
                    return
        except (socket.timeout, OSError):
            pass
        time.sleep(0.01)

def send_command():
    """
    Continuously sends the currently pressed key (w, a, or d)
    to the robot. Commands are only sent after the GO signal has been received.
    """
    global current_key
    while True:
        if config_received and current_key:
            try:
                arduino.sendall(current_key.encode())
            except (BrokenPipeError, OSError):
                # Connection lost
                break
        time.sleep(0.001)  # Adjust delay for the desired command repetition rate

def read_logs():
    """
    Continuously reads log messages from the robot and prints them to the console.
    """
    buffer = ""
    while True:
        try:
            data = arduino.recv(1024)
            if not data:
                # Connection closed by Arduino
                break
            buffer += data.decode("utf-8", errors="replace")
            # Process complete lines
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip('\r')
                if line:
                    print(f"Arduino Log: {line}")
        except (BrokenPipeError, OSError):
            break
        except socket.timeout:
            pass
        time.sleep(0.01)

def on_press(key):
    """
    Called when a key is pressed.
    Only the keys 'w', 'a', and 'd' are considered valid commands.
    """
    global current_key
    try:
        if key.char in ['w', 'a', 'd']:
            current_key = key.char
    except AttributeError:
        # Ignore special keys (e.g., arrow keys, shift, ctrl)
        pass

def on_release(key):
    """
    Called when a key is released.
    If a valid command key is released, reset current_key.
    Pressing Esc will close the socket and exit the script.
    """
    global current_key
    if hasattr(key, 'char') and key.char in ['w', 'a', 'd']:
        current_key = None
    if key == keyboard.Key.esc:
        arduino.close()
        return False

# -----------
# Main Program Flow
# -----------

# 1. Send the configuration file to the robot
send_config()

# 2. Wait for the GO signal from the robot
wait_for_go()

# 3. Start the thread to read log messages
log_thread = Thread(target=read_logs, daemon=True)
log_thread.start()

# 4. Start the thread to send commands
send_thread = Thread(target=send_command, daemon=True)
send_thread.start()

# 5. Start the keyboard listener
with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
    listener.join()
