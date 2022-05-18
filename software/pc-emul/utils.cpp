#include "utils.hpp"

#include <unistd.h>
#include <sys/mman.h>

#include "signal.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"

int GetPageSize(){
   static int pageSize = 0;

   if(pageSize == 0){
      pageSize = getpagesize();
   }

   return pageSize;
}

static int pagesAllocated = 0;
static int pagesDeallocated = 0;

void* AllocatePages(int pages){
   pagesAllocated += pages;
   void* res = mmap(0, pages * GetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   Assert(res != MAP_FAILED);

   return res;
}

void DeallocatePages(void* ptr,int pages){
   pagesDeallocated += pages;
   munmap(ptr,pages * GetPageSize());
}

// Misc

int mini(int a1,int a2){
   int res = (a1 < a2 ? a1 : a2);

   return res;
}

int maxi(int a1,int a2){
   int res = (a1 > a2 ? a1 : a2);

   return res;
}

int RoundUpDiv(int dividend,int divisor){
   int div = dividend / divisor;

   if(dividend % divisor == 0){
      return div;
   } else {
      return (div + 1);
   }
}

int AlignNextPower2(int val){
   if(val <= 0){
      return 0;
   }

   int res = 1;
   while(res < val){
      res *= 2;
   }

   return res;
}

SizedString MakeSizedString(const char* str,size_t size){
   SizedString res = {};

   res.str = str;

   if(size != 0){
      res.size = size;
   }else{
      res.size = strlen(str);
   }

   return res;
}

void* ZeroOutRealloc(void* ptr,int newSize,int oldSize){
   int extraMem = newSize - oldSize;

   void* res = realloc(ptr,newSize);

   if(extraMem > 0){
      char* view = (char*) res;
      memset(&view[oldSize],0,extraMem);
   }

   return res;
}

void FixedStringCpy(char* dest,SizedString src){
   int i = 0;
   for(i = 0; i < src.size; i++){
      dest[i] = src.str[i];
   }
   dest[i] = '\0';
}

bool CompareString(SizedString str1,SizedString str2){
   if(str1.size != str2.size){
      return false;
   }

   for(int i = 0; i < str1.size; i++){
      if(str1.str[i] != str2.str[i]){
         return false;
      }
   }

   return true;
}

bool CompareString(const char* str1,SizedString str2){
   // Slower but do not care for now
   bool res = CompareString(MakeSizedString(str1),str2);
   return res;
}

bool CompareString(SizedString str1,const char* str2){
   bool res = CompareString(str2,str1);
   return res;
}

bool CompareString(const char* str1,const char* str2){
   bool res = (strcmp(str1,str2) == 0);
   return res;
}

void FlushStdout(){
   fflush(stdout);
}

static char* GetHierarchyNameRepr_(HierarchyName name, char* buffer,int first){
   if(name.parent){
      buffer = GetHierarchyNameRepr_(*name.parent,buffer,0);
   }

   for(int i = 0; i < MAX_NAME_SIZE - 1 && name.str[i] != '\0'; i++){
      *(buffer++) = name.str[i];
   }
   if(!first){
      *(buffer++) = '_';
   }
   *buffer = '\0';

   return buffer;
}

char* GetHierarchyNameRepr(HierarchyName name){
   static int slot = 0;
   static char buffer[MAX_NAME_SIZE * 16 * 8];

   char* ptr = &buffer[MAX_NAME_SIZE * 16 * slot];
   GetHierarchyNameRepr_(name,ptr,1);

   slot = (slot + 1) % 8;

   return ptr;
}
