#include "communication.h"
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <vector>

Servo servo;
#define SERVOPIN 12
#define SERIAL_TIMEOUT 150

// Variables
int currentAngle;
int servoStepVar = 0;
int state = 0;
int dir = 0;
long t, tlastcommand, tlastdata = 0;
long lastLogTime = 0;
bool goForLog = false;

Communication com;
const Config* config = nullptr;

String logMsg = "";

void setup() {
    Serial.begin(9600);
    delay(5000);
    com.setupWIFI();

    // Wait for a valid configuration
    while (config == nullptr) {
        com.MDNSUpdate();
        char* jsonConfig = com.recieveData();
        if (jsonConfig != nullptr && strstr(jsonConfig, "CONFIG:")) {
            config = Config::createFromJson(jsonConfig + strlen("CONFIG:"));
        }
        // Short delay to avoid unnecessary CPU load
        delay(100);
    }
    delay(100);
    com.MDNSUpdate();
    com.sendLog(LogMsg(LogLevel::INFO, "GO"), config);
    com.sendLog(LogMsg(LogLevel::DEBUG, config->toString()), config);

    currentAngle = config->SERVO_CENTER;

    // Configuration successfully loaded
    Serial.println("Configuration successfully loaded!");

    servo.attach(SERVOPIN);
    servo.write(config->SERVO_CENTER);
    delay(500);
}

void loop() {
    // Update MDNS
    com.MDNSUpdate();

    goForLog = false;
    t = millis();

    // Read a new state (0,1,2,3) if available
    int newState = com.receiveState();
    if (newState != -1) {
        state = newState;
        tlastdata = t;
    }

    // Check WiFi command timeout
    if (t - tlastdata >= SERIAL_TIMEOUT) {
        // If too much time has passed since the last valid state,
        // assume no movement.
        state = 0;
        dir = 0;
    }

    // Calculate the servo angle based on the current state
    switch (state) {
        case 1: // forward
            currentAngle = moveForward();
            break;
        case 2: // turn left
            currentAngle = turnLeft();
            break;
        case 3: // turn right
            currentAngle = turnRight();
            break;
        default:
            currentAngle = noMovement();
            break;
    }
    
    // Log message for debugging
    logMsg = "state:" + decodeState(state) +
             "time: " + String(t) +
             ", last data: " + String(tlastdata) +
             ", servo angle: " + String(currentAngle) +
             ", direction: " + String(dir);

    // Send the log if debug is enabled and goForLog is true
    if (com.isClientAvailable() && config->DEBUG && goForLog) {
        com.sendLog(LogMsg(LogLevel::SYSTEM, logMsg), config);
    }

    // Update servo
    servo.write(currentAngle);
}

int moveForward() {
    // If enough time hasn't passed, do nothing
    if (t - tlastcommand < config->SERVO_DELAY) {
        return currentAngle;
    }

    if (t - lastLogTime >= config->DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    // Move in one direction
    if (dir == 1) {
        currentAngle += config->SERVO_STEP;
        if (currentAngle > (config->SERVO_CENTER + config->SERVO_AMPLITUDE_LOW)) {
            dir = 0;
        }
    }
    // Change direction
    else if (dir == 0) {
        currentAngle -= config->SERVO_STEP;
        if (currentAngle < (config->SERVO_CENTER - config->SERVO_AMPLITUDE_LOW)) {
            dir = 1;
        }
    }

    tlastdata = t;
    tlastcommand = t;
    return currentAngle;
}

int turnRight() {
  return moveDirection(config->TURN_RIGHT);
}

int turnLeft() {
    return moveDirection(config->TURN_LEFT);
}

int moveDirection(int directionFactor) {
    // directionFactor = config->TURN_LEFT or config->TURN_RIGHT

    if (t - tlastcommand < config->SERVO_DELAY) {
        return currentAngle;
    }

    if (t - lastLogTime >= config->DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    // Move in the current direction
    if (dir == 1) {
        currentAngle += servoStepVar * directionFactor;
        servoStepVar++;

        directionLog(directionFactor);

        // Check the limit for maximum amplitude
        if ((directionFactor == config->TURN_RIGHT &&
             currentAngle < (config->SERVO_CENTER - config->SERVO_AMPLITUDE_HIGH)) ||
            (directionFactor == config->TURN_LEFT &&
             currentAngle > (config->SERVO_CENTER + config->SERVO_AMPLITUDE_HIGH))) {
            // Change direction
            dir = 0;
        }
    }
    // Move back towards the center
    else if (dir == 0) {
        currentAngle -= directionFactor * servoStepVar;
        servoStepVar--;

        directionLog(directionFactor);

        // Limit the step size
        if (servoStepVar < 2) {
            servoStepVar = 2;
        }
        // Change direction once the center is reached
        if ((directionFactor == config->TURN_RIGHT && currentAngle >= config->SERVO_CENTER) ||
            (directionFactor == config->TURN_LEFT && currentAngle <= config->SERVO_CENTER)) {
            dir = 1;
            servoStepVar = 1;
        }
    }

    tlastdata = t;
    tlastcommand = t;
    return currentAngle;
}

// No movement
int noMovement() {
    if (!config->ONLY_LOG_MOVEMENT && (t - lastLogTime >= config->DEBUG_STEP_TIME)) {
        goForLog = true;
        lastLogTime = t;
    }
    // Return the neutral servo position
    return config->SERVO_CENTER;
}

void directionLog(int directionFactor) {
    if (goForLog) {
        bool left = (directionFactor == config->TURN_LEFT);
        logMsg += ", servo " + String(left ? "left" : "right") + ", " + 
                  String("lower bound: ") + String(config->SERVO_CENTER - config->SERVO_AMPLITUDE_HIGH) + 
                  String(", upper bound: ") + String(config->SERVO_CENTER + config->SERVO_AMPLITUDE_HIGH);
    }
}

void setGoForLog() {
    if (config->DEBUG && (t - lastLogTime >= config->DEBUG_STEP_TIME)) {
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