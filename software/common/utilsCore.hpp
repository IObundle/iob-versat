#pragma once

// Utilities that do not depend on anything else from versat (like memory.hpp, it's fine to depend on standard library)

#include <cstdio>
#include <initializer_list>
#include <iosfwd>
#include <string.h>
#include <new>
#include <functional>
#include <stdint.h>
#include <optional>

#include "signal.h"
#include "assert.h"

#include "debug.hpp"

#define ALWAYS_INLINE __attribute__((always_inline)) inline

#define CI(x) (x - '0')
#define TWO_DIGIT(x,y) (CI(x) * 10 + CI(y))
#define COMPILE_TIME (TWO_DIGIT(__TIME__[0],__TIME__[1]) * 3600 + TWO_DIGIT(__TIME__[3],__TIME__[4]) * 60 + TWO_DIGIT(__TIME__[6],__TIME__[7]))

#define ALIGN_DOWN(val,size) (val & (~(size - 1)))

// Helper functions named after bitsize. If using byte size, use the X(val,size) functions
#define ALIGN_DOWN_16(val) (val & (~1))
#define ALIGN_DOWN_32(val) (val & (~3))
#define ALIGN_DOWN_64(val) (val & (~7))
#define ALIGN_DOWN_128(val) (val & (~15))
#define ALIGN_DOWN_256(val) (val & (~31))
#define ALIGN_DOWN_512(val) (val & (~63))

#define ALIGN_UP(val,size) (((val) + (size - 1)) & ~(size - 1))

#define ALIGN_UP_16(val) (((val) + 1) & ~1)
#define ALIGN_UP_32(val) (((val) + 3) & ~3)
#define ALIGN_UP_64(val) (((val) + 7) & ~7)
#define ALIGN_UP_128(val) (((val) + 15) & ~15)
#define ALIGN_UP_256(val) (((val) + 31) & ~31)
#define ALIGN_UP_512(val) (((val) + 63) & ~63)

#define IS_ALIGNED(val,size) ((((uptr) val) & (size-1)) == 0x0)

#define IS_ALIGNED_16(val) ((((uptr) val) & 1) == 0x0)
#define IS_ALIGNED_32(val) ((((uptr) val) & 3) == 0x0)
#define IS_ALIGNED_64(val) ((((uptr) val) & 7) == 0x0)
#define IS_ALIGNED_128(val) ((((uptr) val) & 15) == 0x0)
#define IS_ALIGNED_256(val) ((((uptr) val) & 31) == 0x0)
#define IS_ALIGNED_512(val) ((((uptr) val) & 63) == 0x0)

#undef  ARRAY_SIZE
#define ARRAY_SIZE(array) ((int) (sizeof(array) / sizeof(array[0])))

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define CAST_AND_DEREF(EXPR,TYPE) *((TYPE*) &EXPR)

#define PrintFileAndLine() printf("%s:%d\n",__FILE__,__LINE__)

template<typename F>
class _Defer{
public:
   F f;
   _Defer(F f) : f(f){};
   ~_Defer(){f();}
};

struct _DeferTag{};
template<typename F>
ALWAYS_INLINE _Defer<F> operator+(_DeferTag,F&& f){
   return _Defer<F>(f);
}

#define TEMP__defer(LINE) TEMPdefer_ ## LINE
#define TEMP_defer(LINE) TEMP__defer( LINE )
#define defer auto TEMP_defer(__LINE__) = _DeferTag() + [&]() // Only executes at the end of the code block

#define DEFER_CLOSE_FILE(NAME) defer{ \
  if(NAME){  \
    int res = fclose(NAME); \
    if(res){ \
      printf("Error closing file: %d\n",errno); \
    }\
    NAME = nullptr; \
  };\
}

struct _Once{};
struct _OnceTag{};
template<typename F>
ALWAYS_INLINE _Once operator+(_OnceTag,F&& f){
  f();
  return _Once{};
}

#define TEMP__once(LINE) TEMPonce_ ## LINE
#define TEMP_once(LINE) TEMP__once( LINE )
#define once static _Once TEMP_once(__LINE__) = _OnceTag() + [&] // Executes once even if called multiple times

void PrintStacktrace();

const char* GetFilename(const char* fullpath);

// Quick print functions used when doing "print" debugging
#define NEWLINE() do{ printf("\n"); fflush(stdout);} while(0)
#define LOCATION() do{ printf("%s:%d\n",GetFilename(__FILE__),__LINE__); fflush(stdout);} while(0)
#define PRINTF_WITH_LOCATION(...) do{ printf("%s:%d-",__FILE__,__LINE__); printf(__VA_ARGS__); fflush(stdout);} while(0)
#define PRINT_STRING(STR) do{ printf("%.*s\n",UNPACK_SS((STR))); fflush(stdout);} while(0)

// Use when debugging, easier to search due to the 'd' at the beginning, less confusion with non-debugging printfs
int dprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

static void Terminate(){fflush(stdout); exit(-1);}
//#if defined(VERSAT_DEBUG)
#define Assert(EXPR) \
  do { \
    bool _ = !(EXPR);   \
if(_){ \
      NEWLINE(); \
      NEWLINE(); \
      NEWLINE(); \
      fprintf(stderr,"Assertion failed: %s\n",#EXPR); \
      fprintf(stderr,"At: "); LOCATION(); \
      fflush(stderr); \
      NEWLINE(); \
      NEWLINE(); \
      NEWLINE(); \
      __builtin_trap(); \
    } \
  } while(0)
//#else
//#define Assert(EXPR) do {} while(0)
//#endif

#define DEBUG_BREAK_IF(COND) do{ \
  if(currentlyDebugging) { \
    if(COND){ \
      fflush(stdout); \
      __asm__("int3");} \
  } else { \
    once{ \
      printf("Old debug break point still active:\n");  \
      LOCATION(); \
      printf("\n");  \
    }; \
  } \
  } while(0)

#define DEBUG_BREAK() DEBUG_BREAK_IF(true)

// If possible use argument to put a simple string indicating the error that must likely occured
// We do not print it for now but useful for long lasting code.
#define NOT_IMPLEMENTED(...) do{ printf("%s:%d:1: error: Not implemented: %s",__FILE__,__LINE__,__PRETTY_FUNCTION__); fflush(stdout); Assert(false); } while(0) // Doesn't mean that something is necessarily future planned
#define NOT_POSSIBLE(...) DEBUG_BREAK()
#define UNHANDLED_ERROR(...) do{ fflush(stdout); Assert(false); } while(0) // Know it's an error, but only exit, for now
#define USER_ERROR(...) do{ fflush(stdout); exit(0); } while(0) // User error and program does not try to repair or keep going (report error before and exit)
#define UNREACHABLE(...) do{Assert(false); __builtin_unreachable();} while(0)

#define FOREACH_LIST(TYPE,ITER,START) for(TYPE ITER = START; ITER; ITER = ITER->next)
#define FOREACH_LIST_INDEXED(TYPE,ITER,START,INDEX) for(TYPE ITER = START; ITER; ITER = ITER->next,INDEX += 1)

#define SWAP(A,B) do { \
   auto TEMP = A; \
   A = B; \
   B = TEMP; \
   } while(0)

typedef uint8_t Byte;
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef intptr_t iptr;
typedef uintptr_t uptr;
typedef unsigned int uint;

// Using std::optional is a pain when debugging 
// This simple class basically does the same and much easier to debug
template<typename T>
struct Opt{
  T val;
  bool hasVal;

  Opt():val{},hasVal(false){};
  Opt(std::nullopt_t):val{},hasVal(false){};
  Opt(T t):val(t),hasVal(true){};
  
  Opt<T>& operator=(T& t){
    hasVal = true;
    val = t;
    return *this;
  };

  Opt<T>& operator=(std::nullopt_t){
    hasVal = false;
    return *this;
  };

  bool operator!(){return !hasVal;};

  // Match the std interface to make it easier to replace if needed
  bool has_value(){return hasVal;};
  T& value() & {assert(hasVal);return val;};
  T&& value() && {assert(hasVal);return std::move(val);};
  T value_or(T other){return hasVal ? val : other;};
}; 

#if 0
// Optimized Opt for pointers, but not tested yet and test benchs not currently working.
// 
template<typename T>
struct Opt<T*>{
  T* val;

  Opt():val(nullptr){};
  Opt(std::nullopt_t):val(nullptr){};
  Opt(T* t):val(t){};

  Opt<T>& operator=(T& t){
    val = t;
    return *this;
  };

  Opt<T>& operator=(std::nullopt_t){
    val = nullptr;
    return *this;
  };

  bool operator!(){return !val;};

  // Match the std interface to make it easier to replace if needed
  bool has_value(){return (val != nullptr);};
  T& value() & {assert(val);return val;};
  T&& value() && {assert(val);return std::move(val);};
  T value_or(T other){return this->has_value() ? val : other;};
};
#endif

//template<typename T>
//using Opt = std::optional<T>;
#define PROPAGATE(OPTIONAL) if(!(OPTIONAL).has_value()){return {};}
#define PROPAGATE_POINTER(PTR) if((PTR) == nullptr){return (nullptr);}
#define PROPAGATE_POINTER_OPT(PTR) if((PTR) == nullptr){return {};}

template<typename T>
using BracketList = std::initializer_list<T>;

// TODO: Time needs to be rewritted because we no longer need to support firmware code
struct Time{
   u64 microSeconds;
   u64 seconds;
};

Time GetTime();
void PrintTime(Time time,const char* id);
Time operator-(const Time& s1,const Time& s2);
Time operator+(const Time& s1,const Time& s2);
bool operator>(const Time& s1,const Time& s2);
bool operator==(const Time& s1,const Time& s2);

static constexpr Time Seconds(u64 seconds){Time t = {}; t.seconds = seconds; return t;};
static constexpr Time MilliSeconds(u64 milli){Time t = {}; t.microSeconds = milli * 1000; return t;};

// Automatically times a block in number of counts
class TimeIt{
public:
	Time start;
   const char* id;
   bool endedEarly;

   TimeIt(const char* id){this->id = id; start = GetTime(); endedEarly = false;};
   TimeIt(const char* id, bool activated){this->id = id; start = GetTime(); endedEarly = !activated;};
   ~TimeIt(){if(!endedEarly) Output();};
   void Output();
   void End(){Output(); endedEarly = true;}
   operator bool(){return true;}; // For the timeRegion trick
};
#define TIME_IT(ID) TimeIt timer_##__LINE__(ID)
#define TIME_IT_IF(ID,COND) TimeIt timer_##__LINE__(ID,COND)

#define __timeRegion(LINE) timeRegion_ ## LINE
#define _timeRegion(LINE) __timeRegion( LINE )
#define timeRegion(ID) if(TimeIt _timeRegion(__LINE__){ID})

#define timeRegionIf(ID,COND) if(TimeIt _timeRegion(__LINE__){ID,COND})

// Can use break to end time region early. Not sure if useful, for now use the if trick
//#define timeRegion(ID) for(struct{TimeIt m;bool flag;} _timeRegion(__LINE__) = {ID,true}; _timeRegion(__LINE__).flag; _timeRegion(__LINE__).flag = false)

/*
template<typename F>
ALWAYS_INLINE void operator+(TimeIt&& timer,F&& f){
   f();
}
#define timeRegion(ID) TimeIt(ID) + [&]()
*/

template<typename T>
class ArrayIterator{
public:
   T* ptr;

   inline bool operator!=(const ArrayIterator<T>& iter){return ptr != iter.ptr;};
   inline ArrayIterator<T>& operator++(){++ptr; return *this;};
   inline T& operator*(){return *ptr;};
};

template<typename T> struct Array;

template<typename T>
struct Array{
  T* data;
  int size;
  
  inline T& operator[](int index) const {Assert(index >= 0);Assert(index < size); return data[index];}
  ArrayIterator<T> begin() const{return ArrayIterator<T>{data};};
  ArrayIterator<T> end() const{return ArrayIterator<T>{data + size};};
};

typedef Array<const char> String;

#define C_ARRAY_TO_ARRAY(C_ARRAY) {C_ARRAY,sizeof(C_ARRAY) / sizeof(C_ARRAY[0])}

#define UNPACK_SS(STR) (STR).size,(STR).data
#define UNPACK_SS_REVERSE(STR) (STR).data,(STR).size

inline String STRING(const char* str){return (String){str,(int) strlen(str)};}
inline String STRING(const char* str,int size){return (String){str,size};}
inline String STRING(const unsigned char* str){return (String){(const char*)str,(int) strlen((const char*) str)};}
inline String STRING(const unsigned char* str,int size){return (String){(const char*)str,size};}

String Offset(String base,int amount);

template<> class std::hash<String>{
public:
   std::size_t operator()(String const& s) const noexcept{
   std::size_t res = 0;

   std::size_t prime = 5;
   for(int i = 0; i < s.size; i++){
      res += (std::size_t) s[i] * prime;
      res <<= 4;
      prime += 6; // Some not prime, but will find most of them
   }

   return res;
   }
};

template<typename T>
bool operator==(Array<T> first,Array<T> second){
   if(first.size != second.size){
      return false;
   }

   for(int i = 0; i < first.size; i++){
      if(!(first.data[i] == second.data[i])){
         return false;
      }
   }
   return true;
}

template<typename T>
struct Range{
  union{
    T high;
    T start;
    T top;
  };

  union{
    T low;
    T end;
 	T bottom;
  };
};

union Conversion{
   float f;
   int i;
   uint ui;
};

template<typename First,typename Second>
struct Pair{
   union{
      First key;
      First first;
   };
   union{
      Second data;
      Second second;
   };
} /* __attribute__((packed)) */; // TODO: Check if type info works correctly without this. It technically should after we added align info to the type system

template<typename F,typename S>
static bool operator==(const Pair<F,S>& p1,const Pair<F,S>& p2){
   bool res = (p1.first == p2.first && p1.second == p2.second);
   return res;
}

#define BIT_MASK(BIT) (1 << (BIT))
#define GET_BIT(VAL,INDEX) (VAL & (BIT_MASK(INDEX)))
#define SET_BIT(VAL,INDEX) (VAL | (BIT_MASK(INDEX)))
#define CLEAR_BIT(VAL,INDEX) (VAL & ~(BIT_MASK(INDEX)))
#define COND_BIT_MASK(BIT,COND) (COND ? BIT_MASK(BIT) : 0)
#define FULL_MASK(BITS) (BIT_MASK(BITS) - 1)
#define MASK_VALUE(VAL,BITS) (VAL & FULL_MASK(BITS))

// Returns a statically allocated string, instead of implementing varg for everything
// Returned string uses statically allocated memory. Intended to be used to create quick strings for other functions, instead of having to implement them as variadic. Can also be used to temporarely transform a String into a C-String
// NOTE: Extra care when using this function. Use it mainly to interface with C code style strings otherwise real risk of overwriting contents with other data
char* StaticFormat(const char* format,...);

// Array useful functions
int CountNonZeros(Array<int> arr);

// Misc
int RoundUpDiv(int dividend,int divisor);
int log2i(int value); // Log function customized to calculating bits needed for a number of possible addresses (ex: log2i(1024) = 10)
int AlignNextPower2(int val);
int Align(int val,int alignment);
unsigned int AlignBitBoundary(unsigned int val,int numberBits); // Align value so the lower numberBits are all zeros
bool IsPowerOf2(int val);
int RandomNumberBetween(int minimum,int maximum); // Maximum not included

void Print(Array<int> array,int digitSize = 0);
void Print(Array<float> array,int digitSize = 0);
int GetMaxDigitSize(Array<int> array);
int GetMaxDigitSize(Array<float> array);

// Math related functions
int RolloverRange(int min,int val,int max);
int Clamp(int min,int val,int max);

float Sqrt(float n);
float Abs(float val);
double Abs(double val);
int Abs(int val);
int Abs(uint val); // Convert to signed and perform abs, otherwise no reason to call a Abs function for an unsigned number
bool FloatEqual(float f0,float f1,float epsilon = 0.00001f);
bool FloatEqual(double f0,double f1,double epsilon = 0.00001);

int PackInt(float val);
float PackFloat(int val);
float PackFloat(Byte sign,Byte exponent,int mantissa);
int SwapEndianess(int val);
u64 SwapEndianess(u64 val);

int NumberDigitsRepresentation(i64 number); // Number of digits if printed (negative includes - sign )
char GetHex(int value);
Byte HexCharToNumber(char ch);

void HexStringToHex(unsigned char* buffer,String str);

// Weak random generator but produces same results in pc-emul and in simulation
void SeedRandomNumber(uint seed);
uint GetRandomNumber();

bool RemoveDirectory(const char* path);
//bool CheckIfFileExists(const char* file);
long int GetFileSize(FILE* file);
char* GetCurrentDirectory();
void MakeDirectory(const char* path);
void CreateDirectories(const char* path);
String ExtractFilenameOnly(String filepath);
String PathGoUp(char* pathBuffer);

void FixedStringCpy(char* dest,String src);

// Compare strings regardless of implementation
bool CompareString(String str1,String str2);
bool CompareString(const char* str1,String str2);
bool CompareString(String str1,const char* str2);
bool CompareString(const char* str1,const char* str2);

bool Empty(String str);

// If strings are equal up until a given size, returns a order so that smallest comes first
int CompareStringOrdered(String str1,String str2);

char GetHexadecimalChar(int value);
unsigned char* GetHexadecimal(const unsigned char* text, int str_size); // Helper function to display result

bool IsAlpha(char ch);

template<typename T>
inline void Memset(T* buffer,T elem,int bufferSize){
   for(int i = 0; i < bufferSize; i++){
      buffer[i] = elem;
   }
}

template<typename T>
inline void Memset(Array<T> buffer,T elem){
   for(int i = 0; i < buffer.size; i++){
      buffer.data[i] = elem;
   }
}

template<typename T>
inline void Memcpy(T* dest,T* src,int numberElements){
   memcpy(dest,src,numberElements * sizeof(T));
}

template<typename T>
inline void Memcpy(Array<T> dest,Array<T> src){
  Assert(src.size <= dest.size);
  memcpy(dest.data,src.data,src.size * sizeof(T));
}

// Mainly change return meaning (true means equal)
template<typename T>
inline bool Memcmp(T* a,T* b,int numberElements){
   return (memcmp(a,b,numberElements * sizeof(T)) == 0);
}

inline unsigned char SelectByte(int val,int index){
   union{
      int val;
      unsigned char asChar;
   } conv;

   int i = (index & 3) * 8;
   conv.val = ((val >> i) & 0x000000ff);

   return conv.asChar;
}

template<typename T>
Array<T> Offset(Array<T> in,int amount){
  Array<T> res = in;
  if(amount < in.size){
    in.data += amount;
    in.size -= amount;
  } else {
    return {};
  }
}

static inline bool Contains(String str,char ch){
   for(int i = 0; i < str.size; i++){
      if(str.data[i] == ch){
         return true;
      }
   }
   return false;
}


template<typename T>
inline void Reverse(Array<T>& arr){
  for(int i = 0; i < arr.size / 2; i++){
    SWAP(arr[i],arr[arr.size - 1 - i]);
  }
}

template<typename T>
bool Contains(Array<T> array,T toCheck){
   for(int i = 0; i < array.size; i++){
      if(array.data[i] == toCheck){
         return true;
      }
   }
   return false;
}

template<>
inline bool Contains(Array<String> array,String toCheck){
   for(int i = 0; i < array.size; i++){
      if(CompareString(array.data[i],toCheck)){
         return true;
      }
   }
   return false;
}
