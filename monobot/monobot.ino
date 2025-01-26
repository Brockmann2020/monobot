#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// Create a WiFi server on port 80
WiFiServer server(80);
WiFiClient client;

// Function prototypes
bool checkSerialTimeout();
void setGoForLog();
int moveForward(int command);
int noMovement(int command);
int moveDirection(int directionFactor);
String decodeState();
void directionLog(int directionFactor);
int turnLeft(int command);
int turnRight(int command);

Servo servo;
#define SERVOPIN 12
#define SERIAL_TIMEOUT 150

// Debug Settings
const bool DEBUG = true;
const int DEBUG_STEP_TIME = 500; // Controls how often the logging occurs
const bool ONLY_LOG_MOVEMENT = false;
bool goForLog = false;

// WiFi Settings
const char ssid[] = "Mi Note 10";
const char pass[] = "brockmann";

// Programm Parameters
const int SERVO_CENTER = 103; //servo center fine tuning
const int SERVO_AMPLITUDE_LOW = 20, SERVO_AMPLITUDE_HIGH = 75;
const int SERVO_STEP = 10;
const int SERVO_DELAY = 20;
const int TURN_LEFT = 1;
const int TURN_RIGHT = -1;

// Variables
int currentAngle = SERVO_CENTER;
int servoStepVar = 0;
int state = 0;
int dir = 0;
long t, tlastcommand, tlastdata = 0;
long lastLogTime = 0;

String logMsg = "";

IPAddress ipadrr;

void setup() {
    Serial.begin(9600);

    // Connect to WiFi
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    ipadrr = WiFi.localIP();
    Serial.print("ESP8266 IP address: ");
    Serial.println(ipadrr);

    server.begin();

    // Start the MDNS responder
    if (!MDNS.begin("monobot")) { // Use your preferred hostname
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("MDNS responder started. Access via http://monobot.local/");
    }

    ipadrr = WiFi.localIP();
    Serial.print("Arduino IP address: ");
    Serial.println(ipadrr);

    servo.attach(SERVOPIN);
    servo.write(SERVO_CENTER);
    delay(500);
}

void loop() {
    // Check for incoming client connections
    if (!client || !client.connected()) {
      // If no client or client disconnected, check for new clients
      WiFiClient newClient = server.available();
      if (newClient) {
        client = newClient;
      }
    }
    MDNS.update();
    //Serial.println(ipadrr);

    goForLog = false;
    char command = 0;
    t = millis();

    // Read command from client if available
    if (client && client.connected() && client.available() > 0) {
        command = client.read();
    }

    logMsg = "state:" + decodeState(state) + "time: " + String(t) + ", last data: " + String(tlastdata) + ", servo angle: " + currentAngle + ", direction: " + String(dir);

    // Check WiFi command timeout 
    if (t - tlastdata >= SERIAL_TIMEOUT) {
        state = 0;
        dir = 0;
    }

    // Calculate Angle
    switch (state) {
        case 1: //forward
            currentAngle = moveForward(command);
            break;
        case 2: //turn left
            currentAngle = turnLeft(command);
            break;
        case 3: //turn right
            currentAngle = turnRight(command);
            break;
        default:
            currentAngle = noMovement(command);
            break;
    }

    // Send The Log if DEBUG and goForLog
    if (client && client.connected() && DEBUG && goForLog) {
      client.println(logMsg);
    }

    servo.write(currentAngle);
}

int moveForward(int command) {
    int servoAngle = currentAngle;

    if (command != 'w') {
        // Wrong command - no movement
        return currentAngle;
    }

    tlastdata = t;

    if (t - tlastcommand < SERVO_DELAY) {
      return currentAngle;
    }

    if (t - lastLogTime >= DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    // move in one direction
    if (dir == 1) {
        servoAngle += SERVO_STEP;
        if (servoAngle > (SERVO_CENTER + SERVO_AMPLITUDE_LOW)) {
            dir = 0;
        }
    }
    // change direction
    else if (dir == 0) {
        servoAngle -= SERVO_STEP;
        if (servoAngle < (SERVO_CENTER - SERVO_AMPLITUDE_LOW)) {
            dir = 1;
        }
    }

    tlastcommand = t;
    return servoAngle;
}

int moveDirection(int directionFactor) {
    int servoAngle = currentAngle;
    tlastdata = t;

    if (t - tlastcommand < SERVO_DELAY) {
      return currentAngle;
    }

    if (t - lastLogTime >= DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    if (dir == 1) {
        // Move in the current direction
        servoAngle += servoStepVar * directionFactor;
        servoStepVar++;

        directionLog(directionFactor);

        // Check the limit for maximum amplitude
        if ((directionFactor == TURN_RIGHT && servoAngle < (SERVO_CENTER - SERVO_AMPLITUDE_HIGH)) ||
            (directionFactor == TURN_LEFT  && servoAngle > (SERVO_CENTER + SERVO_AMPLITUDE_HIGH))) {
            dir = 0; // Change direction
        }
    } else if (dir == 0) {
        // Move back towards the center
        servoAngle -= directionFactor * servoStepVar;
        servoStepVar--;

        directionLog(directionFactor);

        // Limit the step size
        if (servoStepVar < 2) {
            servoStepVar = 2;
        }

        // Change direction once the center is reached
        if ((directionFactor == TURN_RIGHT  && servoAngle >= SERVO_CENTER) ||
            (directionFactor == TURN_LEFT && servoAngle <= SERVO_CENTER)) {
            dir = 1;
            servoStepVar = 1;
        }
    }

    tlastcommand = t;
    return servoAngle;
}

int turnRight(int command) {
    if (command == 'd') {
        tlastdata = t;
        return moveDirection(TURN_RIGHT);
    }
    return currentAngle;
}

int turnLeft(int command) {
    if (command == 'a') {
        tlastdata = t;
        return moveDirection(TURN_LEFT);
    }
    return currentAngle;
}

int noMovement(int command) {    
  if (!ONLY_LOG_MOVEMENT && (t - lastLogTime >= DEBUG_STEP_TIME)) {
      goForLog = true;
      lastLogTime = t;
  }

  switch (command) {
      case 'w':
          tlastdata = t;
          state = 1;
          break;
      case 'a':
          tlastdata = t;
          dir = 1;
          servoStepVar = 1;
          state = 2;
          break;
      case 'd':
          tlastdata = t;
          dir = 1;
          servoStepVar = 1;
          state = 3;
          break;
      default:
          break;
  }
  
  return SERVO_CENTER;
}

void directionLog(int directionFactor) {
  if (goForLog) {
    bool left = directionFactor == TURN_LEFT;
    logMsg += ", servo " + String(left ? "left" : "right") + ", " + 
          String("lower bound: ") + String(SERVO_CENTER - SERVO_AMPLITUDE_HIGH) + 
          String(", upper bound: ") + String(SERVO_CENTER + SERVO_AMPLITUDE_HIGH);
  }
}

void setGoForLog() {
    if (DEBUG && (t - lastLogTime >= DEBUG_STEP_TIME)) {
        goForLog = true;
        lastLogTime = t;
    } else {
        goForLog = false;
    }
}

String decodeState(int s) {
  switch(s) {
    case 0: return "0={noMovement},\t";
    case 1: return "1={forward},\t\t";
    case 2: return "2={left},\t\t";
    case 3: return "3={right},\t\t";
    default: return "undefined state";
  }
}
