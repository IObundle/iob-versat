#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

#include <iosfwd>
#include <string.h>
#include <new>

#include "assert.h"

#define ALIGN_4(val) ((val + 3) & ~0x3)
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define DebugSignal() printf("%s:%d\n",__FILE__,__LINE__);

#if 1
#define Assert(EXPR) \
   { \
   int _ = (int) (EXPR);   \
   if(!_){ \
      FlushStdout(); \
      assert(_ && (EXPR)); \
   } \
   }
#else
#define Assert(EXPR) do {} while(0)
#endif

struct SizedString{
   const char* str;
   int size;
};

bool operator<(const SizedString& lhs,const SizedString& rhs);

#define UNPACK_SS(STR) STR.size,STR.str

#define MAX_NAME_SIZE 64
// Hierarchical naming scheme
struct HierarchyName{
   char str[MAX_NAME_SIZE];
   HierarchyName* parent;
};

struct Range{
   int high;
   int low;
};

// Misc
int mini(int a1,int a2);
int maxi(int a1,int a2);
int RoundUpDiv(int dividend,int divisor);

int AlignNextPower2(int val);

void FlushStdout();

long int GetFileSize(FILE* file);
char* GetCurrentDirectory();

SizedString MakeSizedString(const char* str, size_t size = 0);
#define MAKE_SIZED_STRING(STR) (MakeSizedString(STR,strlen(STR))) // Should compute length at compile time

void FixedStringCpy(char* dest,SizedString src);

// Compare strings regardless of implementation
bool CompareString(SizedString str1,SizedString str2);
bool CompareString(const char* str1,SizedString str2);
bool CompareString(SizedString str1,const char* str2);
bool CompareString(const char* str1,const char* str2);

char* GetHierarchyNameRepr(HierarchyName name); // Uses statically allocaded memory, take care

SizedString PathGoUp(char* pathBuffer);

#endif // INCLUDED_UTILS_HPP












