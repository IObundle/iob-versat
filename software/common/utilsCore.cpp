#include "filesystem.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cinttypes>
#include <limits>
#include <time.h>

#include <filesystem>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>

namespace fs = std::filesystem;

int dprintf(const char *format, ...){
  va_list args;
  va_start (args, format);
  int result = vprintf (format, args);
  va_end (args);  
  return result;
}

String Offset(String base,int amount){
  String res = base;
  res.data += amount;
  res.size -= amount;

  return res;
}

char* StaticFormat(const char* format,...){
  static const int BUFFER_SIZE = 1024*4;
  static char buffer[BUFFER_SIZE];

  va_list args;
  va_start(args,format);

  int written = vsprintf(buffer,format,args);

  va_end(args);

  Assert(written < BUFFER_SIZE);
  buffer[written] = '\0';
  
  return buffer;
}

int CountNonZeros(Array<int> arr){
  int count = 0;
  for(int val : arr){
    if(val) count++;
  }
  return count;
}

Time GetTime(){
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  Time t = {};
  t.seconds = time.tv_sec;
  t.microSeconds = time.tv_nsec / 1000;

  return t;
}

// Misc
const char* GetFilename(const char* fullpath){
  const char* lastGood = fullpath;
  const char* ptr = fullpath;

  while(*ptr != '\0'){
    if(*ptr == '/'){
      lastGood = ptr + 1;
    }
    ptr += 1;
  }

  return lastGood;
}

void FlushStdout(){
  fflush(stdout);
}

bool RemoveDirectory(const char* path){
  return fs::remove_all(path) > 0;
}

String ExtractFilenameOnly(String filepath){
  int i;
  for(i = filepath.size - 1; i >= 0; i--){
    if(filepath[i] == '/'){
      break;
    }
  }

  String res = {};
  res.data = &filepath.data[i + 1];
  res.size = filepath.size - (i + 1);
  return res;
}

long int GetFileSize(FILE* file){
  long int mark = ftell(file);

  fseek(file,0,SEEK_END);
  long int size = ftell(file);

  fseek(file,mark,SEEK_SET);

  return size;
}

char* GetCurrentDirectory(){
  // TODO: Maybe receive arena and remove use of static buffer
  static char buffer[PATH_MAX];
  buffer[0] = '\0';
  char* res = getcwd(buffer,PATH_MAX);

  return res;
}

void MakeDirectory(const char* path){
  mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void CreateDirectories(const char* path){
  char buffer[PATH_MAX];
  memset(buffer,0,PATH_MAX);

 for(int i = 0; path[i]; i++){
    buffer[i] = path[i];

    if(path[i] == '/'){
      DIR* dir = opendir(buffer);
      if(!dir && errno == ENOENT){
        MakeDirectory(buffer);
      }
      if(dir){
        closedir(dir);
      }
    }
  }

  DIR* dir = opendir(buffer);
  if(!dir && errno == ENOENT){
    MakeDirectory(buffer);
  }
  if(dir){
    closedir(dir);
  }
}

String PathGoUp(char* pathBuffer){
  String content = STRING(pathBuffer);

  int i = content.size - 1;
  for(; i >= 0; i--){
    if(content[i] == '/'){
      break;
    }
  }

  if(content[i] != '/'){
    return content;
  }

  pathBuffer[i] = '\0';
  content.size = i;

  return content;
}

static char* GetNumberRepr(u64 number){
  static char buffer[32];

  if(number == 0){
    buffer[0] = '0';
    buffer[1] = '\0';
    return buffer;
  }

  u64 val = number;
  buffer[31] = '\0';
  int index = 30;
  while(val){
    buffer[index] = '0' + (val % 10);
    val /= 10;
    index -= 1;
  }

  return &buffer[index+1];
}

void PrintTime(Time time,const char* id){
  printf("[TimeIt] %15s: ",id);

  {
    int digits = NumberDigitsRepresentation(time.seconds);
    char* secondsRepr = GetNumberRepr(time.seconds);

    for(int i = 0; i < digits; i++){
      printf("%c",secondsRepr[i]);
    }
  }
  printf(".");
  {
    int digits = NumberDigitsRepresentation(time.microSeconds);
    char* nanoRepr = GetNumberRepr(time.microSeconds);

    for(int i = 0; i < (6 - digits); i++){
      printf("0");
    }

    for(int i = 0; i < digits; i++){
      printf("%c",nanoRepr[i]);
    }
  }

  Time temp = time;
  temp.seconds = SwapEndianess(time.seconds);
  temp.microSeconds = SwapEndianess(time.microSeconds);

  unsigned char* hexVal = GetHexadecimal((const unsigned char*) &temp,sizeof(Time)); // Helper function to display result
  printf(" %s ",hexVal);

  printf("\n");
}

void TimeIt::Output(){
  // Cannot use floating point because of embedded
  Time end = GetTime();

  Time diff = {};
  diff.seconds = end.seconds - start.seconds;
  diff.microSeconds = end.microSeconds - start.microSeconds;

  Assert(end > start);

  PrintTime(diff,id);
}

Time operator-(const Time& s1,const Time& s2){
  Time res = {};

  Assert(s1 > s2 || s1 == s2);

  int S1 = s1.seconds;
  int S2 = s2.seconds;
  int M1 = s1.microSeconds;
  int M2 = s2.microSeconds;
  
  while(M1 < M2){
    M1 += 1000000;
    S1 -= 1;
  }

  Assert(S1 >= S2);
  
  res.seconds = S1 - S2;
  res.microSeconds = M1 - M2;
  
  return res;
}

Time operator+(const Time& s1,const Time& s2){
  Time res = {};
  
  res.seconds = s1.seconds + s2.seconds;
  res.microSeconds = s1.microSeconds + s2.microSeconds;

  if(res.microSeconds > 1000000){
    res.seconds += res.microSeconds / 1000000;
    res.microSeconds %= res.microSeconds;
  }
  
  return res;
}

bool operator>(const Time& s1,const Time& s2){
  bool res = (s1.seconds == s2.seconds ? s1.microSeconds > s2.microSeconds : s1.seconds > s2.seconds);
  return res;
}

bool operator==(const Time& s1,const Time& s2){
  bool res = (s1.seconds == s2.seconds && s1.microSeconds == s2.microSeconds);
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

int RandomNumberBetween(int minimum,int maximum){
  int randomValue = GetRandomNumber();
  int delta = maximum - minimum;

  if(delta <= 0){
    return minimum;
  }

  int res = minimum + Abs(randomValue % delta);
  return res;
}

int GetMaxDigitSize(Array<int> array){
  int maxReprSize = 0;
  for(int val : array){
    maxReprSize = std::max(maxReprSize,NumberDigitsRepresentation(val));
  }

  return maxReprSize;
}

int GetMaxDigitSize(Array<float> array){
  return 2; // Floating points is hard to figure out how many digits. 2 should be enough
}

void Print(Array<float> array,int digitSize){
  if(digitSize == 0){
    digitSize = GetMaxDigitSize(array);
  }

  for(float val : array){
    printf("%.*f ",digitSize,val);
  }
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

u64 SwapEndianess(u64 val){
  unsigned char* view = (unsigned char*) &val;

  u64 res = ((u64) view[0] << 56) |
             ((u64) view[1] << 48) |
             ((u64) view[2] << 40) |
             ((u64) view[3] << 32) |
             ((u64) view[4] << 24) |
             ((u64) view[5] << 16) |
             ((u64) view[6] << 8)  |
             ((u64) view[7]);

  return res;
}

int NumberDigitsRepresentation(i64 number){
  int nDigits = 0;

  if(number == 0){
    return 1;
  }

  i64 num = number;
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
    return 'A' + (value - 10);
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

bool Empty(String str){
  return (str.size == 0);
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

