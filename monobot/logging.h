enum class LogLevel {
  SYSTEM = 0,
  DEBUG = 1,
  INFO = 2,
  LEVEL = 3
};

struct LogMsg {
  LogLevel level{LogLevel::SYSTEM};
  String message{0};

  LogMsg(LogLevel lvl = LogLevel::SYSTEM, const String& msg = "") 
    : level(lvl), message(msg) {}
};
