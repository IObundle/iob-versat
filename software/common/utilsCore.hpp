#ifndef VERSAT_INCLUDED_UTILS_CORE
#define VERSAT_INCLUDED_UTILS_CORE

// Utilities that do not depend on anything else (like memory.hpp)

#include <cstdio>
#include <iosfwd>
#include <string.h>
#include <new>
#include <functional>
#include <stdint.h>

#include "signal.h"
#include "assert.h"

#define ALWAYS_INLINE __attribute__((always_inline)) inline

#define CI(x) (x - '0')
#define TWO_DIGIT(x,y) (CI(x) * 10 + CI(y))
#define COMPILE_TIME (TWO_DIGIT(__TIME__[0],__TIME__[1]) * 3600 + TWO_DIGIT(__TIME__[3],__TIME__[4]) * 60 + TWO_DIGIT(__TIME__[6],__TIME__[7]))

#define ALIGN_2(val) (((val) + 1) & ~1)
#define ALIGN_4(val) (((val) + 3) & ~3)
#define ALIGN_8(val) (((val) + 7) & ~7)
#define ALIGN_16(val) (((val) + 15) & ~15)
#define ALIGN_32(val) (((val) + 31) & ~31)
#define ALIGN_64(val) (((val) + 63) & ~63) // Usually a cache line

#undef  ARRAY_SIZE
#define ARRAY_SIZE(array) ((int) (sizeof(array) / sizeof(array[0])))

#define IS_ALIGNED_2(val) ((((uptr) val) & 1) == 0x0)
#define IS_ALIGNED_4(val) ((((uptr) val) & 3) == 0x0)
#define IS_ALIGNED_8(val) ((((uptr) val) & 7) == 0x0)
#define IS_ALIGNED_16(val) ((((uptr) val) & 15) == 0x0)
#define IS_ALIGNED_32(val) ((((uptr) val) & 31) == 0x0)
#define IS_ALIGNED_64(val) ((((uptr) val) & 63) == 0x0)

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define CAST_AND_DEREF(EXPR,TYPE) *((TYPE*) &EXPR)

#define PrintFileAndLine() printf("%s:%d\n",__FILE__,__LINE__)

template<typename F>
class Defer{
public:
   F f;
   Defer(F f) : f(f){};
   ~Defer(){f();}
};

struct _DeferTag{};
template<typename F>
ALWAYS_INLINE Defer<F> operator+(_DeferTag,F&& f){
   return Defer<F>(f);
}

#define TEMP__defer(LINE) TEMPdefer_ ## LINE
#define TEMP_defer(LINE) TEMP__defer( LINE )
#define defer auto TEMP_defer(__LINE__) = _DeferTag() + [&]()

void FlushStdout();
#if defined(VERSAT_DEBUG)
#define Assert(EXPR) \
   do { \
   bool _ = !(EXPR);   \
   if(_){ \
      FlushStdout(); \
      assert(_ && (EXPR)); \
   } \
   } while(0)
#else
#define Assert(EXPR) do {} while(0)
#endif

// Assert that gets replaced by the expression if debug is disabled (instead of simply disappearing like a normal assert)
#if defined(VERSAT_DEBUG)
#define AssertAndDo(EXPR) Assert(EXPR)
#else
#define AssertAndDo(EXPR) EXPR
#endif

#define DEBUG_BREAK() do{ FlushStdout(); raise(SIGTRAP); } while(0)
#define DEBUG_BREAK_IF(COND) if(COND){FlushStdout(); raise(SIGTRAP);}

#define DEBUG_BREAK_IF_DEBUGGING() DEBUG_BREAK_IF(CurrentlyDebugging())

#define debugRegion if(CurrentlyDebugging())

bool CurrentlyDebugging();

#define NOT_IMPLEMENTED do{ printf("%s:%d:1: error: Not implemented: %s",__FILE__,__LINE__,__PRETTY_FUNCTION__); FlushStdout(); Assert(false); } while(0) // Doesn't mean that something is necessarily future planned
#define NOT_POSSIBLE DEBUG_BREAK()
#define UNHANDLED_ERROR do{ FlushStdout(); Assert(false); } while(0) // Know it's an error, but only exit, for now
#define USER_ERROR do{ FlushStdout(); exit(0); } while(0) // User error and program does not try to repair or keep going (report error before and exit)
#define UNREACHABLE do{Assert(false); __builtin_unreachable();} while(0)

#define FOREACH_LIST(ITER,START) for(auto* ITER = START; ITER; ITER = ITER->next)
#define FOREACH_LIST_INDEXED(ITER,START,INDEX) for(auto* ITER = START; ITER; ITER = ITER->next,INDEX += 1)
#define FOREACH_LIST_BOUNDED(ITER,START,END) for(auto* ITER = START; ITER != END; ITER = ITER->next)
#define FOREACH_SUBLIST(ITER,SUB) for(auto* ITER = SUB.start; ITER != SUB.end; ITER = ITER->next)

#define SWAP(A,B) do { \
   auto TEMP = A; \
   A = B; \
   B = TEMP; \
   } while(0)

#define TIME_FUNCTION clock

#if 0
#ifdef VERSAT
//#include <cstdio>
extern "C"{
#include "iob-timer.h"
}
#define TIME_FUNCTION timer_get_count
#else
#define TIME_FUNCTION clock
#endif
#endif

#ifndef SIM // #ifdef PC TODO: I do not think these will be needed anymore
typedef intptr_t iptr;
typedef uintptr_t uptr;
typedef uint32_t uint;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int8_t int8;
typedef uint8_t uint8;
typedef uint8_t Byte;
#else
typedef unsigned char Byte;
typedef unsigned int uint;
typedef intptr_t iptr;
typedef int64_t int64;
typedef uint64_t uint64;
//typedef long long int int64; // Long long is at least 64, so
//typedef long long unsigned int uint64;
#endif

struct Time{
   uint64 seconds;
   uint64 nanoSeconds;
};

Time GetTime();
Time operator-(const Time& s1,const Time& s2);
bool operator>(const Time& s1,const Time& s2);
bool operator==(const Time& s1,const Time& s2);

static constexpr Time Seconds(uint64 seconds){Time t = {}; t.seconds = seconds; return t;};

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

// This struct is associated to a gdb pretty printer.
template<typename T>
struct Array{
   T* data;
   int size;

   inline T& operator[](int index) const {Assert(index < size); return data[index];}
   ArrayIterator<T> begin(){return ArrayIterator<T>{data};};
   ArrayIterator<T> end(){return ArrayIterator<T>{data + size};};
};

typedef Array<const char> String;

#define C_ARRAY_TO_ARRAY(C_ARRAY) {C_ARRAY,sizeof(C_ARRAY) / sizeof(C_ARRAY[0])}

#define UNPACK_SS(STR) (STR).size,(STR).data
#define UNPACK_SS_REVERSE(STR) (STR).data,(STR).size

inline String STRING(const char* str){return (String){str,(int) strlen(str)};}
inline String STRING(const char* str,int size){return (String){str,size};}
inline String STRING(const unsigned char* str){return (String){(const char*)str,(int) strlen((const char*) str)};}
inline String STRING(const unsigned char* str,int size){return (String){(const char*)str,size};}

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

struct Range{
   union{
      int high;
      int start;
   };

   union{
      int low;
      int end;
   };
};

struct CheckRangeResult{
   bool result;
   int problemIndex;
};

void SortRanges(Array<Range> ranges);

// These functions require sorted ranges
CheckRangeResult CheckNoOverlap(Array<Range> ranges);
CheckRangeResult CheckNoGaps(Array<Range> ranges);

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
} /* __attribute__((packed)) */;

template<typename F,typename S>
static bool operator==(const Pair<F,S>& p1,const Pair<F,S>& p2){
   bool res = (p1.first == p2.first && p1.second == p2.second);
   return res;
}

#define BIT_MASK(BIT) (1 << BIT)
#define GET_BIT(VAL,INDEX) (VAL & (BIT_MASK(INDEX)))
#define SET_BIT(VAL,INDEX) (VAL | (BIT_MASK(INDEX)))
#define CLEAR_BIT(VAL,INDEX) (VAL & ~(BIT_MASK(INDEX)))
#define COND_BIT_MASK(BIT,COND) (COND ? BIT_MASK(BIT) : 0)
#define FULL_MASK(BITS) (BIT_MASK(BITS) - 1)
#define MASK_VALUE(VAL,BITS) (VAL & FULL_MASK(BITS))

// Returns a statically allocated string, instead of implementing varg for everything
// Returned string uses statically allocated memory. Intended to be used to create quick strings for other functions, instead of having to implement them as variadic
char* StaticFormat(const char* format,...);

// Misc
int RoundUpDiv(int dividend,int divisor);
int log2i(int value); // Log function customized to calculating bits needed for a number of possible addresses (ex: log2i(1024) = 10)
int AlignNextPower2(int val);
int Align(int val,int alignment);
unsigned int AlignBitBoundary(unsigned int val,int numberBits); // Align value so the lower numberBits are all zeros
bool IsPowerOf2(int val);
int RandomNumberBetween(int minimum,int maximum,int randomValue); // Maximum not included

void Print(Array<int> array,int digitSize = 0);
int GetMaxDigitSize(Array<int> array);

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

int NumberDigitsRepresentation(int64 number); // Number of digits if printed (negative includes - sign )
char GetHex(int value);
Byte HexCharToNumber(char ch);

void HexStringToHex(unsigned char* buffer,String str);

// Weak random generator but produces same results in pc-emul and in simulation
void SeedRandomNumber(uint seed);
uint GetRandomNumber();

bool CheckIfFileExists(const char* file);
long int GetFileSize(FILE* file);
char* GetCurrentDirectory();
void MakeDirectory(const char* path);
FILE* OpenFileAndCreateDirectories(const char* path,const char* format);
void CreateDirectories(const char* path);
String ExtractFilenameOnly(String filepath);

void FixedStringCpy(char* dest,String src);

// Compare strings regardless of implementation
bool CompareString(String str1,String str2);
bool CompareString(const char* str1,String str2);
bool CompareString(String str1,const char* str2);
bool CompareString(const char* str1,const char* str2);

// If strings are equal up until a given size, returns a order so that smallest comes first
int CompareStringOrdered(String str1,String str2);

char GetHexadecimalChar(int value);
unsigned char* GetHexadecimal(const unsigned char* text, int str_size); // Helper function to display result

bool IsAlpha(char ch);

String PathGoUp(char* pathBuffer);

#if __cplusplus >= 201703L
#include <optional>

template<typename T>
using Optional = std::optional<T>;

// Simulate c++23 feature
template<typename T>
Optional<T> OrElse(Optional<T> first,Optional<T> elseOpt){
   if(first){
      return first;
   } else {
      return elseOpt;
   }
}
#endif

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

static inline bool Contains(String str,char ch){
   for(int i = 0; i < str.size; i++){
      if(str.data[i] == ch){
         return true;
      }
   }
   return false;
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

template<typename T>
struct ListedStruct : public T{
   ListedStruct<T>* next;
};

// Generic list manipulation, as long as the structure has a next pointer of equal type
template<typename T>
T* ListGet(T* start,int index){
   T* ptr = start;
   for(int i = 0; i < index; i++){
      if(ptr){
         ptr = ptr->next;
      }
   }
   return ptr;
}

template<typename T>
int ListIndex(T* start,T* toFind){
   int i = 0;
   FOREACH_LIST(ptr,start){
      if(ptr == toFind){
         break;
      }
      i += 1;
   }
   return i;
}

template<typename T>
T* ListRemove(T* start,T* toRemove){ // Returns start of new list. ToRemove is still valid afterwards
   if(start == toRemove){
      return start->next;
   } else {
      T* previous = nullptr;
      FOREACH_LIST(ptr,start){
         if(ptr == toRemove){
            previous->next = ptr->next;
         }
         previous = ptr;
      }

      return start;
   }
}

// For now, we are not returning the "deleted" node.
// TODO: add a free node list and change this function
template<typename T,typename Func>
T* ListRemoveOne(T* start,Func compareFunction){ // Only removes one and returns.
   if(compareFunction(start)){
      return start->next;
   } else {
      T* previous = nullptr;
      FOREACH_LIST(ptr,start){
         if(compareFunction(ptr)){
            previous->next = ptr->next;
         }
         previous = ptr;
      }

      return start;
   }
}

// TODO: This function leaks memory, because it does not return the free nodes
template<typename T,typename Func>
T* ListRemoveAll(T* start,Func compareFunction){
   #if 0
   T* freeListHead = nullptr;
   T* freeListPtr = nullptr;
   #endif

   T* head = nullptr;
   T* listPtr = nullptr;
   for(T* ptr = start; ptr;){
      T* next = ptr->next;
      ptr->next = nullptr;
      bool comp = compareFunction(ptr);

      if(comp){ // Add to free list
         #if 0
         if(freeListPtr){
            freeListPtr->next = ptr;
            freeListPtr = ptr;
         } else {
            freeListHead = ptr;
            freeListPtr = ptr;
         }
         #endif
      } else { // "Add" to return list
         if(listPtr){
            listPtr->next = ptr;
            listPtr = ptr;
         } else {
            head = ptr;
            listPtr = ptr;
         }
      }

      ptr = next;
   }

   return head;
}

template<typename T>
T* ReverseList(T* head){
   if(head == nullptr){
      return head;
   }

   T* ptr = nullptr;
   T* next = head;

   while(next != nullptr){
      T* nextNext = next->next;
      next->next = ptr;
      ptr = next;
      next = nextNext;
   }

   return ptr;
}

template<typename T>
T* ListInsertEnd(T* head,T* toAdd){
   T* last = nullptr;
   FOREACH_LIST(ptr,head){
      last = ptr;
   }
   Assert(last->next == nullptr);
   last->next = toAdd;
}

template<typename T>
T* ListInsert(T* head,T* toAdd){
   if(!head){
      return toAdd;
   }

   toAdd->next = head;
   return toAdd;
}

template<typename T>
int Size(T* start){
   int size = 0;
   FOREACH_LIST(ptr,start){
      size += 1;
   }
   return size;
}

#endif // VERSAT_INCLUDED_UTILS_CORE
