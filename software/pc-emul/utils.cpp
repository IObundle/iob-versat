#include "utils.hpp"

#include <unistd.h>
#include <limits.h>

#include "stdio.h"
#include "string.h"

int SimpleHash(SizedString hashing)
{
   int res = 0;

   int prime = 5;
   for(int i = 0; i < hashing.size; i++){
      res += (int) hashing.str[i] * prime;
      res <<= 4;
      prime += 6; // Some not prime, but will find most of them
   }

   return res;
}

char* SizedStringToCStr(SizedString s){
   static char buffer[1024];

   Assert(s.size < 1023);

   memcpy(buffer,s.str,s.size);
   buffer[s.size] = '\0';

   return buffer;
}

bool operator<(const SizedString& lhs,const SizedString& rhs){
   for(int i = 0; i < mini(lhs.size,rhs.size); i++){
      if(lhs.str[i] < rhs.str[i]){
         return true;
      }
      if(lhs.str[i] > rhs.str[i]){
         return false;
      }
   }

   if(lhs.size < rhs.size){
      return true;
   }

   return false;
}

bool operator==(const SizedString& lhs,const SizedString& rhs){
   bool res = CompareString(lhs,rhs);

   return res;
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

int log2i(int value){
   int res = 0;

   if(value <= 0)
      return 0;

   value -= 1; // If value matches a power of 2, we still want to return the previous value

   while(value > 1){
      value /= 2;
      res++;
   }

   res += 1;

   return res;
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

int NumberDigitsRepresentation(int number){
   int nDigits = 0;

   int num = number;
   if(num < 0){
      nDigits += 1;
      num *= -1;
   }

   while(num > 0){
      num /= 10;
      nDigits += 1;
   }

   return nDigits;
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

long int GetFileSize(FILE* file){
   long int mark = ftell(file);

   fseek(file,0,SEEK_END);
   long int size = ftell(file);

   fseek(file,mark,SEEK_SET);

   return size;
}

char* GetCurrentDirectory(){
   static char buffer[PATH_MAX];
   buffer[0] = '\0';
   getcwd(buffer,PATH_MAX);
   return buffer;
}

bool IsAlpha(char ch){
   if(ch >= 'a' && ch < 'z')
      return true;

   if(ch >= 'A' && ch <= 'Z')
      return true;

   if(ch >= '0' && ch <= '9')
      return true;

   return false;
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

SizedString PathGoUp(char* pathBuffer){
   SizedString content = MakeSizedString(pathBuffer);

   int i = content.size - 1;
   for(; i >= 0; i--){
      if(content.str[i] == '/'){
         break;
      }
   }

   if(content.str[i] != '/'){
      return content;
   }

   pathBuffer[i] = '\0';
   content.size = i;

   return content;
}












