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

// Function declarations
int moveForward(int command);
int turnLeft(int command);
int turnRight(int command);
int noMovement(int command);
int moveDirection(int directionFactor);
void directionLog(int directionFactor);
void setGoForLog();
String decodeState(int s);
void parseInput(char* input);

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
    char command = 0;
    t = millis();

    // Read command from client if available
    command = com.recieveDataStream();

    logMsg = "state:" + decodeState(state) + 
             "time: " + String(t) + 
             ", last data: " + String(tlastdata) + 
             ", servo angle: " + String(currentAngle) + 
             ", direction: " + String(dir);

    // Check WiFi command timeout 
    if (t - tlastdata >= SERIAL_TIMEOUT) {
        state = 0;
        dir = 0;
    }

    // Calculate angle
    switch (state) {
        case 1: // forward
            currentAngle = moveForward(command);
            break;
        case 2: // turn left
            currentAngle = turnLeft(command);
            break;
        case 3: // turn right
            currentAngle = turnRight(command);
            break;
        default:
            currentAngle = noMovement(command);
            break;
    }

    // Send the log if debug is enabled and goForLog is true
    if (com.isClientAvailable() && config->DEBUG && goForLog) {
        com.sendLog(LogMsg(LogLevel::SYSTEM, logMsg), config);
    }

    servo.write(currentAngle);
}

// Move forward
int moveForward(int command) {
    int servoAngle = currentAngle;

    // Wrong command - no movement
    if (command != 'w') {
									  
        return currentAngle;
    }

    tlastdata = t;

    if (t - tlastcommand < config->SERVO_DELAY) {
        return currentAngle;
    }

    if (t - lastLogTime >= config->DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    // Move in one direction
    if (dir == 1) {
        servoAngle += config->SERVO_STEP;
        if (servoAngle > (config->SERVO_CENTER + config->SERVO_AMPLITUDE_LOW)) {
            dir = 0;
        }
    }
    // Change direction
    else if (dir == 0) {
        servoAngle -= config->SERVO_STEP;
        if (servoAngle < (config->SERVO_CENTER - config->SERVO_AMPLITUDE_LOW)) {
            dir = 1;
        }
    }

    tlastcommand = t;
    return servoAngle;
}

int moveDirection(int directionFactor) {
    int servoAngle = currentAngle;
    tlastdata = t;

    if (t - tlastcommand < config->SERVO_DELAY) {
        return currentAngle;
    }

    if (t - lastLogTime >= config->DEBUG_STEP_TIME) {
        goForLog = true;
        lastLogTime = t;
    }

    // Move in the current direction
    if (dir == 1) {
										
        servoAngle += servoStepVar * directionFactor;
        servoStepVar++;

        directionLog(directionFactor);

        // Check the limit for maximum amplitude
        if ((directionFactor == config->TURN_RIGHT && servoAngle < (config->SERVO_CENTER - config->SERVO_AMPLITUDE_HIGH)) ||
            (directionFactor == config->TURN_LEFT  && servoAngle > (config->SERVO_CENTER + config->SERVO_AMPLITUDE_HIGH))) {
            dir = 0; // Change direction
        }
    }
    // Move back towards the center
    else if (dir == 0) {
									   
        servoAngle -= directionFactor * servoStepVar;
        servoStepVar--;

        directionLog(directionFactor);

        // Limit the step size
        if (servoStepVar < 2) {
            servoStepVar = 2;
        }

        // Change direction once the center is reached
        if ((directionFactor == config->TURN_RIGHT && servoAngle >= config->SERVO_CENTER) ||
            (directionFactor == config->TURN_LEFT && servoAngle <= config->SERVO_CENTER)) {
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
        return moveDirection(config->TURN_RIGHT);
    }
    return currentAngle;
}

int turnLeft(int command) {
    if (command == 'a') {
        tlastdata = t;
        return moveDirection(config->TURN_LEFT);
    }
    return currentAngle;
}

// No movement
int noMovement(int command) {
    if (!config->ONLY_LOG_MOVEMENT && (t - lastLogTime >= config->DEBUG_STEP_TIME)) {
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