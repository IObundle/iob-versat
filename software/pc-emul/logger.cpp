#include "logger.hpp"

#include "utils.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

const char* LogLevelToName[] = { "DEBUG",
                                 " INFO",
                                 " WARN",
                                 "ERROR",
                                 "FATAL"};

void Log_(LogModule module,LogLevel level,int line,const char* filename,const char* funcName,const char* format, ...){
   va_list args;
   va_start(args,format);

   bool enabledModule = ENABLE_LOG_MODULE & (int) module;
   bool enabledLevel = ((int) level) >= MINIMUM_LOG_LEVEL;

   if(enabledModule && enabledLevel){
      if(level == LogLevel::FATAL){
         printf("[%s:%d] %s\n\t",filename,line,funcName);
      }

      printf("[%s] ",LogLevelToName[(int) level]);
      vprintf(format,args);
      printf("\n");
   }

   va_end(args);

   if(level == LogLevel::FATAL){
      DEBUG_BREAK;
   }
}
