#ifndef INCLUDED_INTRINSICS
#define INCLUDED_INTRINSICS

#include <immintrin.h>
#include <mmintrin.h>

inline int PopCount(uint32 val){return __builtin_popcount(val);};
inline int LeadingZerosCount(uint32 val){return __builtin_clz(val);};
inline int TrailingZerosCount(uint32 val){return __builtin_ctz(val);};

inline uint64 RDTSC(){
   uint32 high;
   uint32 low;
   asm("rdtsc" : "=a"(low), "=d"(high));
   uint64 res = (((uint64) high) << 32) | ((uint64)low);
   return res;
};

// Still need to test if correct and/or faster
inline uint32 BitTestAndReset(uint32 val,uint32 index){
   uint32 wasSet;
   asm ("btr %1,%0" : "=r"(wasSet) : "r"(index) : "cc");
   return wasSet;
}

#endif // INCLUDED_INTRINSICS
