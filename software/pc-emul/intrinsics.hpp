#ifndef INCLUDED_INTRINSICS
#define INCLUDED_INTRINSICS

#include <immintrin.h>
#include <mmintrin.h>

inline int PopCount(uint32 val){return __builtin_popcount(val);};
inline int LeadingZerosCount(uint32 val){return __builtin_clz(val);};
inline int TrailingZerosCount(uint32 val){return __builtin_ctz(val);};

// Could potently be used in the BitArray class
// This is a optimization, so only do that if we can test at full -O2 or -O3 the code and check if it improves.
// Might need to use a macro otherwise the function might not perform the bit set and test for a variable inside a register
// Might fetch memory when we do not want that. (The instruction used can work on registers, but the intrinsic only accepts pointers)
//inline Byte BitTestAndClear(int32* bitListStart, int32 index){;};

#endif // INCLUDED_INTRINSICS
