#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#include "logging.h"

class Config {
private:
  char* JSONConfig;

public:
  // Programm Parameters
  const int SERVO_CENTER;
  const int SERVO_AMPLITUDE_LOW;
  const int SERVO_AMPLITUDE_HIGH;
  const int SERVO_STEP;
  const int SERVO_DELAY;
  const int TURN_LEFT;
  const int TURN_RIGHT;
  const float TWIST_COMPENSATION;
  const LogLevel LOG_LEVEL;

  // Debug Parameters
  const bool DEBUG;
  const int DEBUG_STEP_TIME;
  const bool ONLY_LOG_MOVEMENT;

  explicit Config(int servo_center,
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
           LogLevel log_level);

  String toString() const;

  static Config* createFromJson(const char* JSONConfig);
};


#endif // CONFIG_H