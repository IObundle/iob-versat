#include "utils.hpp"

#include <cinttypes>
#include <limits>

extern "C"{
int printf_(const char* format, ...);
}

#ifndef PC
#define printf printf_
#else
#include <cstdio>
#endif

static char* GetNumberRepr(uint64 number){
   static char buffer[32];

   if(number == 0){
      buffer[0] = '0';
      buffer[1] = '\0';
      return buffer;
   }

   uint64 val = number;
   buffer[31] = '\0';
   int index = 30;
   while(val){
      buffer[index] = '0' + (val % 10);
      val /= 10;
      index -= 1;
   }

   return &buffer[index+1];
}

void TimeIt::Output(){
   // Cannot use floating point because of embedded
   NanoSecond end = GetTime();

   uint64 diff = end.time - start.time;
   int digits = NumberDigitsRepresentation(diff);
   //sprintf(buffer,"%" PRId64,diff);
   char* buffer = GetNumberRepr(diff);

   printf("[TimeIt] %s: ",id);

   int charSeen = 0;
   for(int i = std::max(6,digits); i >= 3; i--){
      if(i < digits){
         printf("%c",buffer[charSeen++]);
      } else if(i <= 6){
         printf("0");
      }

      if(i == 6){
         printf(".");
      }
   }
   printf("\n");
}

NanoSecond operator-(const NanoSecond& s1,const NanoSecond& s2){
   NanoSecond res = {};
   res.time = s1.time - s2.time;
   return res;
}

bool operator>(const NanoSecond& s1,const NanoSecond& s2){
   bool res = s1.time > s2.time;
   return res;
}

// Misc
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

   value -= 1; // If value matches a power of 2, we still want to return the previous value

   // Change the order of this if to after subtracting one.
   // Any weird bug check this first
   if(value <= 0)
      return 0;

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

int Align(int val,int alignment){
   Assert(alignment != 0);
   int remainder = val % alignment;
   if(remainder == 0){
      return val;
   }

   int res = val + (alignment - remainder);
   return res;
}

unsigned int AlignBitBoundary(unsigned int val,int numberBits){ // Align value so the lower numberBits are all zeros
   if(numberBits == 0){
      return val;
   }

   Assert(((size_t) numberBits) < sizeof(val) * 8);

   unsigned int lowerVal = MASK_VALUE(val,numberBits);
   if(lowerVal == 0){
      return val;
   }

   unsigned int lowerRemoved = val & ~FULL_MASK(numberBits);
   unsigned int res = lowerRemoved + BIT_MASK(numberBits);

   return res;
}

bool IsPowerOf2(int val){
   bool res = (val != 0) && ((val & (val - 1)) == 0);
   return res;
}

int RandomNumberBetween(int minimum,int maximum,int randomValue){
   int delta = maximum - minimum;

   if(delta <= 0){
      return minimum;
   }

   int res = minimum + Abs(randomValue % (delta + 1));
   return res;
}

int GetMaxDigitSize(Array<int> array){
   int maxReprSize = 0;
   for(int val : array){
      maxReprSize = std::max(maxReprSize,NumberDigitsRepresentation(val));
   }

   return maxReprSize;
}

void Print(Array<int> array,int digitSize){
   if(digitSize == 0){
      digitSize = GetMaxDigitSize(array);
   }

   for(int val : array){
      printf("%*d ",digitSize,val);
   }
}

int RolloverRange(int min,int val,int max){
   if(val < min){
      val = max;
   } else if(val > max){
      val = min;
   }

   return val;
}

int Clamp(int min,int val,int max){
   if(val < min){
      return min;
   }
   if(val >= max){
      return max;
   }

   return val;
}

float Sqrt(float n){
   float high = 1.8446734e+19;
   float low = 0.0f;

   float middle;
   for(int i = 0; i < 256; i++){
      middle = (high / 2.0f) + (low / 2.0f); // Divide individually otherwise risk of overflowing the addition

      float squared = middle * middle;

      if(FloatEqual(squared,n)){
         break;
      } else if(squared > n){
         high = middle;
      } else {
         low = middle;
      }
   }

   return middle;
}

float Abs(float val){
   float res = val;
   if(val < 0.0f){
      res = -val;
   }
   return res;
}

double Abs(double val){
   double res = val;
   if(val < 0.0){
      res = -val;
   }
   return res;
}

int Abs(int val){
   int res = val;
   if(val < 0){
      res = -val;
   }
   return res;
}

int Abs(unsigned int val){
   int conv = (int) val;
   int res = Abs(conv);
   return res;
}

bool FloatEqual(float f0,float f1,float epsilon){
   if(f0 == f1){
      return true;
   }

   float norm = Abs(f0) + Abs(f1);
   norm = std::min(norm,std::numeric_limits<float>::max());
   float diff = Abs(f0 - f1);

   bool equal = diff < norm * epsilon;

   return equal;
}

bool FloatEqual(double f0,double f1,double epsilon){
   if(f0 == f1){
      return true;
   }

   float norm = Abs(f0) + Abs(f1);
   norm = std::min(norm,std::numeric_limits<float>::max());
   float diff = Abs(f0 - f1);

   bool equal = diff < norm * epsilon;

   return equal;
}

int PackInt(float val){
   Conversion conv = {};
   conv.f = val;

   return conv.i;
}

float PackFloat(int val){
   Conversion conv = {};
   conv.i = val;

   return conv.f;
}

float PackFloat(Byte sign,Byte exponent,int mantissa){
   Conversion conv = {};

   conv.ui = COND_BIT_MASK(31,sign) |
             (exponent << 23) |
             MASK_VALUE(mantissa,23);

   return conv.f;
}

int SwapEndianess(int val){
   unsigned char* view = (unsigned char*) &val;

   int res = (view[0] << 24) |
             (view[1] << 16) |
             (view[2] << 8)  |
             (view[3]);

   return res;
}

int NumberDigitsRepresentation(int64 number){
   int nDigits = 0;

   if(number == 0){
      return 1;
   }

   int64 num = number;
   if(num < 0){
      nDigits += 1; // minus sign
      num *= -1;
   }

   while(num > 0){
      num /= 10;
      nDigits += 1;
   }

   return nDigits;
}

char GetHex(int value){
   if(value < 10){
      return '0' + value;
   } else{
      return 'a' + (value - 10);
   }
}

Byte HexCharToNumber(char ch){
   if(ch >= '0' && ch <= '9'){
      return ch - '0';
   }
   if(ch >= 'A' && ch <= 'F'){
      return (ch - 'A') + 10;
   }
   if(ch >= 'a' && ch <= 'f'){
      return (ch - 'a') + 10;
   }

   Assert(false);
   return '\0';
}

void HexStringToHex(unsigned char* buffer,String str){
   Assert(str.size % 2 == 0);

   for(int i = 0; i < str.size; i += 2){
      char high = str.data[i];
      char low  = str.data[i+1];

      Byte highV = HexCharToNumber(high);
      Byte lowV = HexCharToNumber(low);

      buffer[i / 2] = highV * 16 + lowV;
   }
}

static unsigned int randomSeed = 1;
void SeedRandomNumber(unsigned int val){
   if(val == 0){
      randomSeed = 1;
   } else {
      randomSeed = val;
   }
}

unsigned int GetRandomNumber(){
   // Xorshift
   randomSeed ^= randomSeed << 13;
	randomSeed ^= randomSeed >> 17;
	randomSeed ^= randomSeed << 5;
   return randomSeed;
}

void FixedStringCpy(char* dest,String src){
   int i = 0;
   for(i = 0; i < src.size; i++){
      dest[i] = src[i];
   }
   dest[i] = '\0';
}

bool CompareString(String str1,String str2){
   if(str1.size != str2.size){
      return false;
   }

   bool res = (memcmp(str1.data,str2.data,str1.size) == 0);

   #if 0
   if(str1.data == str2.data){
      return true;
   }

   for(int i = 0; i < str1.size; i++){
      if(str1[i] != str2[i]){
         return false;
      }
   }
   #endif

   return res;
}

bool CompareString(const char* str1,String str2){
   // Slower but do not care for now
   bool res = CompareString(STRING(str1),str2);
   return res;
}

bool CompareString(String str1,const char* str2){
   bool res = CompareString(str2,str1);
   return res;
}

bool CompareString(const char* str1,const char* str2){
   bool res = (strcmp(str1,str2) == 0);
   return res;
}

int CompareStringOrdered(String str1,String str2){
   int minSize = std::min(str1.size,str2.size);
   for(int i = 0; i < minSize; i++){
      if(str1[i] == str2[i]){
         continue;
      }

      return (str2[i] - str1[i]);
   }

   if(str1.size == str2.size){
      return 0;
   } else {
      return (str2.size - str1.size);
   }
}

char GetHexadecimalChar(int value){
   if(value < 10){
      return '0' + value;
   } else{
      return 'a' + (value - 10);
   }
}

unsigned char* GetHexadecimal(const unsigned char* text, int str_size){
   static unsigned char buffer[2048+1];
   int i;

   for(i = 0; i< str_size; i++){
      Assert(i * 2 < 2048);

      int ch = (int) ((unsigned char) text[i]);

      buffer[i*2] = GetHexadecimalChar(ch / 16);
      buffer[i*2+1] = GetHexadecimalChar(ch % 16);
   }

   buffer[(i)*2] = '\0';

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
