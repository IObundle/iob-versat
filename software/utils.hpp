#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

#include <iosfwd>
#include <string.h>
#include <new>
#include <functional>
#include <stdint.h>

#include "signal.h"
#include "assert.h"

#define CI(x) (x - '0')
#define TWO_DIGIT(x,y) (CI(x) * 10 + CI(y))
#define COMPILE_TIME (TWO_DIGIT(__TIME__[0],__TIME__[1]) * 3600 + TWO_DIGIT(__TIME__[3],__TIME__[4]) * 60 + TWO_DIGIT(__TIME__[6],__TIME__[7]))

#define ALIGN_4(val) ((val + 3) & ~0x3)
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define PrintFileAndLine() printf("%s:%d\n",__FILE__,__LINE__)

void FlushStdout();
#if defined(PC) && defined(VERSAT_DEBUG)
#define Assert(EXPR) \
   do { \
   int _ = (int) (EXPR);   \
   if(!_){ \
      FlushStdout(); \
      assert(_ && (EXPR)); \
   } \
   } while(0)
#else
#define Assert(EXPR) do {} while(0)
#endif

#define DEBUG_BREAK do{ FlushStdout(); raise(SIGTRAP); } while(0)

#define NOT_IMPLEMENTED do{ FlushStdout(); Assert(false); } while(0) // Doesn't mean that something is necessarily future planned
#define NOT_POSSIBLE DEBUG_BREAK
#define UNHANDLED_ERROR do{ FlushStdout(); Assert(false); } while(0) // Know it's an error, but only exit, for now
#define USER_ERROR do{ FlushStdout(); exit(0); } while(0) // User error and program does not try to repair or keep going (report error before and exit)

#ifdef VERSAT
#include <cstdio>
extern "C"{
#include "iob-timer.h"
}
#define TIME_FUNCTION timer_get_count
#else
#define TIME_FUNCTION clock
#endif

// Automatically times a block in number of counts
class TimeIt{
public:
	unsigned long long start;
   const char* id;

   TimeIt(const char* id){this->id = id; start = TIME_FUNCTION();};
   ~TimeIt(){unsigned long long end = TIME_FUNCTION();printf("%s: %llu\n",id,end - start);}
};
#define TIME_IT(ID) TimeIt timer_##__LINE__(ID)

typedef char Byte;
#ifdef PC
typedef uint32_t uint;
#endif

template<typename T>
class ArrayIterator{
public:
   T* ptr;

   inline bool operator!=(const ArrayIterator<T>& iter){return ptr != iter.ptr;};
   inline ArrayIterator<T>& operator++(){++ptr; return *this;};
   inline T& operator*(){return *ptr;};
};

template<typename T>
struct Array{
   T* data;
   int size;

   inline T& operator[](int index) const {Assert(index < size); return data[index];} // Assert + function call could trash performance really bad. Eventually profile and replace with direct access if the case
   ArrayIterator<T> begin(){return ArrayIterator<T>{data};};
   ArrayIterator<T> end(){return ArrayIterator<T>{data + size};};
};

typedef Array<const char> SizedString;

#define UNPACK_SS(STR) (STR).size,(STR).data
#define UNPACK_SS_REVERSE(STR) (STR).data,(STR).size

#define MakeSizedString1(STR) ((SizedString){STR,(int) strlen(STR)})
#define MakeSizedString2(STR,LEN) ((SizedString){STR,(int) (LEN)})

#define GET_MACRO(_1,_2,NAME,...) NAME
#define MakeSizedString(...) GET_MACRO(__VA_ARGS__, MakeSizedString2, MakeSizedString1)(__VA_ARGS__)

struct Range{
   int high;
   int low;
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
};

#define BIT_MASK(BIT) (1 << BIT)
#define GET_BIT(VAL,INDEX) (VAL & (BIT_MASK(INDEX)))
#define SET_BIT(VAL,INDEX) (VAL | (BIT_MASK(INDEX)))
#define COND_BIT_MASK(BIT,COND) (COND ? BIT_MASK(BIT) : 0)
#define FULL_MASK(BITS) (BIT_MASK(BITS) - 1)
#define MASK_VALUE(VAL,BITS) (VAL & FULL_MASK(BITS))

// Misc
int RoundUpDiv(int dividend,int divisor);
int log2i(int value); // Log function customized to calculating bits needed for a number of possible addresses (ex: log2i(1024) = 10)
int AlignNextPower2(int val);
bool IsPowerOf2(int val);
int RandomNumberBetween(int minimum,int maximum,int randomValue);

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

int NumberDigitsRepresentation(int number); // Number of digits if printed (negative includes - sign )
char GetHex(int value);

// Weak random generator but produces same results in pc-emul and in simulation
void SeedRandomNumber(uint seed);
uint GetRandomNumber();

long int GetFileSize(FILE* file);
char* GetCurrentDirectory();
void MakeDirectory(const char* path);

void FixedStringCpy(char* dest,SizedString src);

// Compare strings regardless of implementation
bool CompareString(SizedString str1,SizedString str2);
bool CompareString(const char* str1,SizedString str2);
bool CompareString(SizedString str1,const char* str2);
bool CompareString(const char* str1,const char* str2);

char GetHexadecimalChar(int value);
unsigned char* GetHexadecimal(const unsigned char* text, int str_size); // Helper function to display result

bool IsAlpha(char ch);

SizedString PathGoUp(char* pathBuffer);

template<typename T>
void Memset(T* buffer,T elem,int bufferSize){
   for(int i = 0; i < bufferSize; i++){
      buffer[i] = elem;
   }
}

template<typename T>
void Memset(Array<T> buffer,T elem){
   for(int i = 0; i < buffer.size; i++){
      buffer.data[i] = elem;
   }
}

template<typename T>
void Memcpy(T* dest,T* scr,int numberElements){
   for(int i = 0; i < numberElements; i++){
      dest[i] = scr[i];
   }
}

// Mainly change return meaning (true means equal)
template<typename T>
bool Memcmp(T* a,T* b,int numberElements){
   bool res = (memcmp(a,b,numberElements * sizeof(T)) == 0);

   return res;
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

#endif // INCLUDED_UTILS_HPP












