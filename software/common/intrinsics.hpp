#pragma once

#include "utils.hpp"

inline int PopCount(u32 val){return __builtin_popcount(val);};
inline int LeadingZerosCount(u32 val){return __builtin_clz(val);};
inline int TrailingZerosCount(u32 val){return __builtin_ctz(val);};

inline u64 RDTSC(){
   u32 high;
   u32 low;
   asm("rdtsc" : "=a"(low), "=d"(high));
   u64 res = (((u64) high) << 32) | ((u64)low);
   return res;
};

// Still need to test if correct and/or faster
inline u32 BitTestAndReset(u32 val,u32 index){
   u32 wasSet;
   asm ("btr %1,%0" : "=r"(wasSet) : "r"(index) : "cc");
   return wasSet;
}
