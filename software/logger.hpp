#ifndef INCLUDED_LOGGER
#define INCLUDED_LOGGER

//                          DPAVM
#define ENABLE_LOG_MODULE 0b11111
#define MINIMUM_LOG_LEVEL 0

enum LogModule{
   MEMORY      = 0x1,
   TOP_SYS     = 0x2,
   ACCELERATOR = 0x4,
   PARSER      = 0x8,
   DEBUG_SYS   = 0x10
};

enum LogLevel{
   DEBUG   = 0, // Info that user doesn't care about
   INFO    = 1, // General info that every user might want to know
   WARN    = 2, // Not a problem, but can lead to poor results (performance, unexpected results, etc..)
   ERROR   = 3, // Problem but can continue
   FATAL   = 4  // Problem that cannot continue
};

#define Log(MODULE,LEVEL,...) Log_(MODULE,LEVEL,__LINE__,__FILE__,__PRETTY_FUNCTION__,__VA_ARGS__)
void Log_(LogModule module,LogLevel level,int line,const char* filename,const char* funcName,const char* format, ...) __attribute__ ((format (printf, 6, 7)));

#define LogOnce(...) \
   do { \
      static bool firstTimeLog_ = true; \
      if(firstTimeLog_){ \
         Log(__VA_ARGS__); \
         firstTimeLog_ = false; \
      } \
   } while(0)

#endif // INCLUDED_LOGGER
