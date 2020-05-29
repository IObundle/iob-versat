//Versat include file
#ifndef VERSAT_h // include guard
#define VERSAT_h
#include <math.h>

//MACRO to calculate ceil of log2
#define clog2(x) ((int)ceil(log2(x)))

//xconfdefs
#define CONF_MEM0A 0
#define CONF_ALU0 (CONF_MEM0A + 2 * nMEM * MEMP_CONF_OFFSET)
#define CONF_ALULITE0 (CONF_ALU0 + nALU * ALU_CONF_OFFSET)
#define CONF_MUL0 (CONF_ALULITE0 + nALULITE * ALULITE_CONF_OFFSET)
#define CONF_MULADD0 (CONF_MUL0 + nMUL * MUL_CONF_OFFSET)
#define CONF_BS0 (CONF_MULADD0 + nMULADD * MULADD_CONF_OFFSET)
#define CONF_REG_ADDR_W (clog2(CONF_BS0 + nBS * BS_CONF_OFFSET))
#define CONF_MEM_ADDR_W 3
#define CONF_CLEAR (1 << CONF_REG_ADDR_W)
#define GLOBAL_CONF_CLEAR (CONF_CLEAR + 1)
#define CONF_MEM (CONF_CLEAR + (1 << (CONF_REG_ADDR_W - 1)))

//xdefs
#define nSTAGE_W (clog2(nSTAGE))
#define nMEM_W (clog2(nMEM))
#define CTR_ADDR_W (nSTAGE_W + 2 + nMEM_W + MEM_ADDR_W)
#define N (2 * nMEM + nALU + nALULITE + nMUL + nMULADD + nBS)
#define N_W (clog2(N) + 1)

//xmemdefs
#define MEMP_CONF_ITER 0
#define MEMP_CONF_PER 1
#define MEMP_CONF_DUTY 2
#define MEMP_CONF_SEL 3
#define MEMP_CONF_START 4
#define MEMP_CONF_SHIFT 5
#define MEMP_CONF_INCR 6
#define MEMP_CONF_DELAY 7
#define MEMP_CONF_RVRS 8
#define MEMP_CONF_EXT 9
#define MEMP_CONF_IN_WR 10
#define MEMP_CONF_ITER2 11
#define MEMP_CONF_PER2 12
#define MEMP_CONF_SHIFT2 13
#define MEMP_CONF_INCR2 14
#define MEMP_CONF_OFFSET 15
#define MEMP_LAT 1

//xmuladddefs
#define MULADD_MACC 0
#define MULADD_MSUB 1
#define MULADD_CONF_SELA 0
#define MULADD_CONF_SELB 1
#define MULADD_CONF_FNS 2
#define MULADD_CONF_ITER 3
#define MULADD_CONF_PER 4
#define MULADD_CONF_DELAY 5
#define MULADD_CONF_SHIFT 6
#define MULADD_CONF_OFFSET 7
#ifdef MULADD_COMB
#define MULADD_LAT 1
#else
#define MULADD_LAT 3
#endif

//xaludefs
#define ALU_ADD 0
#define ALU_SUB 1
#define ALU_CMP_SIG 2
#define ALU_MUX 3
#define ALU_MAX 4
#define ALU_MIN 5
#define ALU_OR 6
#define ALU_AND 7
#define ALU_CMP_UNS 8
#define ALU_XOR 9
#define ALU_SEXT8 10
#define ALU_SEXT16 11
#define ALU_SHIFTR_ARTH 12
#define ALU_SHIFTR_LOG 13
#define ALU_CLZ 14
#define ALU_ABS 15
#define ALU_CONF_SELA 0
#define ALU_CONF_SELB 1
#define ALU_CONF_FNS 2
#define ALU_CONF_OFFSET 3
#define ALU_LAT 2

//xbsdefs
#define BS_SHR_A 0
#define BS_SHR_L 1
#define BS_SHL 2
#define BS_CONF_SELD 0
#define BS_CONF_SELS 1
#define BS_CONF_FNS 2
#define BS_CONF_OFFSET 3
#define BS_LAT 2

//xalulitedefs
#define ALULITE_ADD 0
#define ALULITE_SUB 1
#define ALULITE_CMP_SIG 2
#define ALULITE_MUX 3
#define ALULITE_MAX 4
#define ALULITE_MIN 5
#define ALULITE_OR 6
#define ALULITE_AND 7
#define ALULITE_CONF_SELA 0
#define ALULITE_CONF_SELB 1
#define ALULITE_CONF_FNS 2
#define ALULITE_CONF_OFFSET 3
#define ALULITE_LAT 2

//xmuldefs
#define MUL_HI 1
#define MUL_DIV2_LO 2
#define MUL_DIV2_HI 3
#define MUL_CONF_SELA 0
#define MUL_CONF_SELB 1
#define MUL_CONF_FNS 2
#define MUL_CONF_OFFSET 3
#define MUL_LAT 3

//xversat
#define nSTAGE 5
#define nMEM 4
#define nALU 2
#define nALULITE 1
#define nMUL 2
#define nMULADD 2
#define nBS 1
#define nMULADDLITE 0
#define MEM_ADDR_W 5
#define CONF_MEM_ADDR_W 3

//End of Versat include file
#endif