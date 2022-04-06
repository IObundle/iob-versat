#ifndef INCLUDED_UTILS_H
#define INCLUDED_UTILS_H

#include "assert.h"

#define ALIGN_4(val) ((val + 3) & ~0x3)
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// Misc

int mini(int a1,int a2);
int maxi(int a1,int a2);
int RoundUpDiv(int dividend,int divisor);

void FixedStringCpy(char* dest,const char* src,int size);

void FlushStdout();
#define Assert(EXPR) \
   { \
   int val = (int) (EXPR);   \
   if(!val){ \
      FlushStdout(); \
      assert(val && EXPR); \
   } \
   }

void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);

#define MAX_NAME_SIZE 16

// Hierarchical naming scheme
typedef struct HierarchyName_t{
   char str[MAX_NAME_SIZE];
   struct HierarchyName_t* parent;
} HierarchyName;

char* GetHierarchyNameRepr(HierarchyName name); // Uses statically allocaded memory, care

#endif // INCLUDED_UTILS_H
