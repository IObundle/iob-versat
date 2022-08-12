#include "logger.hpp"

#include <cstdio>
#include <cstdarg>

void Log(LogModule module,LogLevel level,const char* format, ...){
   va_list args;
   va_start(args,format);

   bool enabledModule = ENABLE_LOG_MODULE & (int) module;
   bool enabledLevel = ((int) level) >= MINIMUM_LOG_LEVEL;

   if(enabledModule && enabledLevel){
      vprintf(format,args);
   }

   va_end(args);
}
