#ifndef INCLUDED_LOGGER
#define INCLUDED_LOGGER

#define ENABLE_LOG_MODULE 0b1111
#define ENABLE_LOG_TYPE   0b001
#define MINIMUM_LOG_LEVEL 0

enum LogModule{
   FIRMWARE    = 0x1,
   MEMORY      = 0x2,
   ACCELERATOR = 0x4,
   PARSER      = 0x8
};

enum LogLevel{
   INFO    = 0,
   DEBUG   = 1,
   WARN,
   ERROR,
   FATAL
};

void Log(LogModule module,LogLevel level,const char* format, ...) __attribute__ ((format (printf, 3, 4)));

#define LogOnce(...) \
   do { \
   static bool firstTimeLog_ = true; \
   if(firstTime){ \
      Log(__VA_ARGS__); \
      firstTime = false; \
   } while(0)

#endif // INCLUDED_LOGGER
