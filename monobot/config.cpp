#include <Arduino.h>
#include "config.h"

Config::Config(int servo_center,
        int servo_amplitude_low,
        int servo_amplitude_high,
        int servo_step,
        int servo_delay,
        int turn_left,
        int turn_right,
        float twist_compensation,
        bool debug,
        int debug_step_time,
        bool only_log_movement,
        LogLevel log_level)
  : SERVO_CENTER(servo_center),
    SERVO_AMPLITUDE_LOW(servo_amplitude_low),
    SERVO_AMPLITUDE_HIGH(servo_amplitude_high),
    SERVO_STEP(servo_step),
    SERVO_DELAY(servo_delay),
    TURN_LEFT(turn_left),
    TURN_RIGHT(turn_right),
    TWIST_COMPENSATION(twist_compensation),
    DEBUG(debug),
    DEBUG_STEP_TIME(debug_step_time),
    ONLY_LOG_MOVEMENT(only_log_movement),
    LOG_LEVEL(log_level)
{ }

Config* Config::createFromJson(const char* JSONConfig) {
    // JSON-Dokument mit ausreichend Puffergröße
    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, JSONConfig);
    if (error) {
        Serial.println("JSON Parsing failed. Using default values.");
    }

    // Extrahiere Werte aus dem JSON-Objekt
    int servo_center         = doc["servo_center"] | 100;
    int servo_amplitude_low  = doc["servo_amplitude_low"] | 20;
    int servo_amplitude_high = doc["servo_amplitude_high"] | 75;
    int servo_step           = doc["servo_step"] | 10;
    int servo_delay          = doc["servo_delay"] | 20;
    int turn_left            = doc["turn_left"] | 1;
    int turn_right           = doc["turn_right"] | -1;
    float twist_compensation = doc["twist_compensation"] | 1.0;
    bool debug               = doc["debug"] | false;
    int debug_step_time      = doc["debug_step_time"] | 500;
    bool only_log_movement   = doc["only_log_movement"] | false;
    LogLevel log_level       = static_cast<LogLevel>(doc["log_level"] | 0);

    // Erzeuge und gebe ein Config-Objekt zurück
    return new Config(servo_center, servo_amplitude_low, servo_amplitude_high, servo_step, servo_delay,
                  turn_left, turn_right, twist_compensation, debug, debug_step_time, only_log_movement, log_level);
}

String Config::toString() const {
        String result = "";
        result += "SERVO_CENTER: " + String(SERVO_CENTER) + "\n";
        result += "SERVO_AMPLITUDE_LOW: " + String(SERVO_AMPLITUDE_LOW) + "\n";
        result += "SERVO_AMPLITUDE_HIGH: " + String(SERVO_AMPLITUDE_HIGH) + "\n";
        result += "SERVO_STEP: " + String(SERVO_STEP) + "\n";
        result += "SERVO_DELAY: " + String(SERVO_DELAY) + "\n";
        result += "TURN_LEFT: " + String(TURN_LEFT) + "\n";
        result += "TURN_RIGHT: " + String(TURN_RIGHT) + "\n";
        result += "LOG_LEVEL: " + String((int)LOG_LEVEL) + "\n";  // Enum als int ausgeben
        result += "DEBUG: " + String(DEBUG ? "true" : "false") + "\n";
        result += "DEBUG_STEP_TIME: " + String(DEBUG_STEP_TIME) + "\n";
        result += "ONLY_LOG_MOVEMENT: " + String(ONLY_LOG_MOVEMENT ? "true" : "false") + "\n";
        return result;
}


