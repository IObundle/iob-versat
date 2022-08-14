#ifndef INCLUDED_LOGGER
#define INCLUDED_LOGGER

#define ENABLE_LOG_MODULE 0b1111
#define MINIMUM_LOG_LEVEL 0

enum LogModule{
   MEMORY      = 0x1,
   VERSAT      = 0x2,
   ACCELERATOR = 0x4,
   PARSER      = 0x8
};

enum LogLevel{
   INFO    = 0,
   DEBUG   = 1,
   WARN    = 2,
   ERROR   = 3,
   FATAL   = 4
};

void Log(LogModule module,LogLevel level,const char* format, ...) __attribute__ ((format (printf, 3, 4)));

#define LogOnce(...) \
   do { \
      static bool firstTimeLog_ = true; \
      if(firstTimeLog_){ \
         Log(__VA_ARGS__); \
         firstTimeLog_ = false; \
      } \
   } while(0)

#endif // INCLUDED_LOGGER
