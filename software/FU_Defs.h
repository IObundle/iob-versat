#define INSTANTIATE_CLASS
#include "unitData.h"
#undef INSTANTIATE_CLASS
#define INSTANTIATE_ARRAY
#include "unitData.h"
#undef INSTANTIATE_ARRAY

//versat-io
#define IO_ADDR_W 32
#define IO_SIZE_W 11
#define nIO (nVI+nVO)
#define VI_CONF_EXT_ADDR 0
#define VI_CONF_INT_ADDR 1
#define VI_CONF_SIZE 2
#define VI_CONF_ITER_A 3
#define VI_CONF_PER_A 4
#define VI_CONF_DUTY_A 5
#define VI_CONF_SHIFT_A 6
#define VI_CONF_INCR_A 7
#define VI_CONF_ITER_B 8
#define VI_CONF_PER_B 9
#define VI_CONF_DUTY_B 10
#define VI_CONF_START_B 11
#define VI_CONF_SHIFT_B 12
#define VI_CONF_INCR_B 13
#define VI_CONF_DELAY_B 14
#define VI_CONF_RVRS_B 15
#define VI_CONF_EXT_B 16
#define VI_CONF_ITER2_B 17
#define VI_CONF_PER2_B 18
#define VI_CONF_SHIFT2_B 19
#define VI_CONF_INCR2_B 20
#define VI_CONF_OFFSET 21
#define VO_CONF_EXT_ADDR 0
#define VO_CONF_INT_ADDR 1
#define VO_CONF_SIZE 2
#define VO_CONF_ITER_A 3
#define VO_CONF_PER_A 4
#define VO_CONF_DUTY_A 5
#define VO_CONF_SHIFT_A 6
#define VO_CONF_INCR_A 7
#define VO_CONF_ITER_B 8
#define VO_CONF_PER_B 9
#define VO_CONF_DUTY_B 10
#define VO_CONF_SEL_B 11
#define VO_CONF_START_B 12
#define VO_CONF_SHIFT_B 13
#define VO_CONF_INCR_B 14
#define VO_CONF_DELAY_B 15
#define VO_CONF_RVRS_B 16
#define VO_CONF_EXT_B 17
#define VO_CONF_ITER2_B 18
#define VO_CONF_PER2_B 19
#define VO_CONF_SHIFT2_B 20
#define VO_CONF_INCR2_B 21
#define VO_CONF_OFFSET 22

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

//xconfdefs
#define CONF_MEM0A 0
#define CONF_VI0 (CONF_MEM0A + 2*nMEM*MEMP_CONF_OFFSET)
#define CONF_VO0 (CONF_VI0 + nVI*VI_CONF_OFFSET)
#define CONF_ALU0 (CONF_VO0 + nVO*VO_CONF_OFFSET)
#define CONF_ALULITE0 (CONF_ALU0 + nALU*ALU_CONF_OFFSET)
#define CONF_MUL0 (CONF_ALULITE0 + nALULITE*ALULITE_CONF_OFFSET)
#define CONF_MULADD0 (CONF_MUL0 + nMUL*MUL_CONF_OFFSET)
#define CONF_BS0 (CONF_MULADD0 + nMULADD*MULADD_CONF_OFFSET)
#define CONF_REG_ADDR_W (clog2(CONF_BS0 + nBS*BS_CONF_OFFSET))
#define CONF_CLEAR (1<<CONF_REG_ADDR_W)
#define GLOBAL_CONF_CLEAR (CONF_CLEAR+1)
#define CONF_MEM (CONF_CLEAR + (1<<(CONF_REG_ADDR_W-1)))

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
#define MULADD_LAT 4
#endif

//xbsdefs
#define BS_SHR_A 0
#define BS_SHR_L 1
#define BS_SHL 2
#define BS_CONF_SELD 0
#define BS_CONF_SELS 1
#define BS_CONF_FNS 2
#define BS_CONF_OFFSET 3
#define BS_LAT 2

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
#define MEMP_LAT 3

//xmuldefs
#define MUL_HI 1
#define MUL_DIV2_LO 2
#define MUL_DIV2_HI 3
#define MUL_CONF_SELA 0
#define MUL_CONF_SELB 1
#define MUL_CONF_FNS 2
#define MUL_CONF_OFFSET 3
#define MUL_LAT 3

#undef B

typedef struct {
   int done,pos,pos2,run_delay,loop1,loop2,loop3,loop4,duty_cnt,enable,next_half;
   int iaddr,eaddr,ext_loop1,ext_loop2;
   int mem[2048];
} VWriteExtra;

typedef struct {
   int done,pos,pos2,run_delay,loop1,loop2,loop3,loop4,duty_cnt,enable,next_half;
   int iaddr,eaddr,ext_loop1,ext_loop2,memoryDelay;
   unsigned int stored[MEMP_LAT];
   int mem[2048];
} VReadExtra;

typedef struct MuladdExtra_t{
    int acc, acc_w;
    int done, duty;
    int duty_cnt, enable;
    int shift_addr, incr, aux, pos;
    int loop2, loop1, cnt_addr;
    int run_delay;
    int debug;
    unsigned int stored[MULADD_LAT];
} MuladdExtra;

typedef struct MemExtra_t{
    int done;
    int run_delay;
    int enable;
    int loop1, loop2, loop3, loop4;
    unsigned int pos;
    unsigned int pos2;
    int duty_cnt;
    int debug;
    unsigned int stored[MEMP_LAT];
} MemExtra;

/*
int32_t* VReadStartFunction(FUInstance* instance){
   static int32_t result = 0;

   VReadConfig* c = (VReadConfig*) instance->config;
   VReadExtra*  e = (VReadExtra*) instance->extraData;

   e->done = 0;
   e->pos = c->int_addr;
   e->pos2 = c->int_addr;
   if (c->dutyA == 0)
      c->dutyA = c->perA;
   
   //set run_delay
   e->run_delay = 0;
   e->loop1 = 0;
   e->loop2 = 0;
   e->loop3 = 0;
   e->loop4 = 0;
   e->duty_cnt = 0;
   e->enable = 0;
   e->iaddr = c->int_addr;
   e->eaddr = 0;
   e->memoryDelay = 3;

   for (int i = 0; i < MEMP_LAT; i++)
      e->stored[i] = 0;

   return &result;
}

int32_t* VReadUpdateFunction(FUInstance* instance){
   static int32_t out = 0;

   VReadConfig* c = (VReadConfig*) instance->config;
   VReadExtra*  e = (VReadExtra*) instance->extraData;
   int* mem = e->mem;

   out = 0;

   for(int i = 0; i < MEMP_LAT - 1; i++){
      e->stored[i] = e->stored[i+1]; 
   }

   out = e->stored[0];

   // Memory read side
   if(e->memoryDelay > 0)
      e->memoryDelay -= 1;
   else{
      int* FPGA_mem = (int*) instance->state;

      if(e->ext_loop1 < c->iterA)
      {
         if(e->ext_loop2 < c->perA)
         {
            int data = FPGA_mem[e->eaddr];
            mem[e->iaddr++] = data;
            e->eaddr += c->incrA;
            e->ext_loop2++;
         }
         if(e->ext_loop2 == c->perA)
         {
            e->eaddr += c->shiftA;
            e->ext_loop2 = 0;        
            e->ext_loop1 += 1;
         }
      }
   }

   //check for delay
   if (e->run_delay > 0)
   {
      e->run_delay--;
      return &out;
   }
   
   if(e->done){
      return &out;
   }

   // Output side selection
   uint32_t aux = 0;
   if (c->B.iter2 == 0 && c->B.per2 == 0)
   {
      if (e->loop2 < c->B.iter)
      {
         if (e->loop1 < c->B.per)
         {
            e->loop1++;
            e->enable = 0;
            if (e->duty_cnt < c->B.duty)
            {
               e->enable = 1;
               aux = e->pos;
               e->duty_cnt++;
               e->pos += c->B.incr;
            }
         }
         if (e->loop1 == c->B.per)
         {
            e->loop1 = 0;
            e->duty_cnt = 0;
            e->loop2++;
            e->pos += c->B.shift;
         }
      }
      if (e->loop2 == c->B.iter)
      {
         e->loop2 = 0;
         e->done = 1;
      }
   }
   else
   {
      if (e->loop4 < c->B.iter2)
      {
         if (e->loop3 < c->B.per2)
         {
            if (e->loop2 < c->B.iter)
            {
               if (e->loop1 < c->B.per)
               {
                  e->loop1++;
                  e->enable = 0;
                  if (e->duty_cnt < c->B.duty)
                  {
                     e->enable = 1;
                     aux = e->pos;
                     e->duty_cnt++;
                     e->pos += c->B.incr;
                  }
               }
               if (e->loop1 == c->B.per)
               {
                  e->loop1 = 0;
                  e->loop2++;
                  e->duty_cnt = 0;
                  e->pos += c->B.shift;
               }
            }
            if (e->loop2 == c->B.iter)
            {
               e->loop2 = 0;
               e->pos2 += c->B.incr2;
               e->pos = e->pos2;
               e->loop3++;
            }
         }
         if (e->loop3 == c->B.per2)
         {
            e->pos2 += c->B.shift2;
            e->pos = e->pos2;
            e->loop3 = 0;
            e->loop4++;
         }
      }
      if (e->loop4 == c->B.iter2)
      {
         e->done = 1;
      }
   }

   e->stored[MEMP_LAT-1] = mem[aux];

   return &out;
}

int32_t* VWriteStartFunction(FUInstance* instance){
   VWriteConfig* c = (VWriteConfig*) instance->config;
   VWriteExtra*  e = (VWriteExtra*) instance->extraData;

   e->done = 0;
   e->pos = c->int_addr;
   e->pos2 = c->int_addr;
   if (c->dutyA == 0)
      c->dutyA = c->perA;
   
   //set run_delay
   e->run_delay = c->B.delay;
   e->loop1 = 0;
   e->loop2 = 0;
   e->loop3 = 0;
   e->loop4 = 0;
   e->duty_cnt = 0;
   e->enable = 0;
   e->ext_loop1 = 0;
   e->ext_loop2 = 0;
   e->iaddr = c->int_addr;
   e->eaddr = 0;

   return NULL;
}

int32_t* VWriteUpdateFunction(FUInstance* instance){

   VWriteConfig* c = (VWriteConfig*) instance->config;
   VWriteExtra*  e = (VWriteExtra*) instance->extraData;
   int* mem = e->mem;

   // Read from memory
   int* memory = (int*) instance->state;
   if(e->ext_loop1 < c->iterA)
   {
      if(e->ext_loop2 < c->perA)
      {
         memory[e->eaddr++] = mem[e->iaddr];
         e->iaddr += c->incrA;
         e->ext_loop2++;
      }
      if(e->ext_loop2 == c->perA)
      {
         e->iaddr += c->shiftA;
         e->ext_loop2 = 0;        
         e->ext_loop1 += 1;
      }
   }

   //check for delay
   if (e->run_delay > 0)
   {
      e->run_delay--;
      return NULL;
   }
   
   if(e->done){
      return NULL;
   }

   // Memory write side
   uint32_t aux = 0;
   if (c->B.iter2 == 0 && c->B.per2 == 0)
   {
      if (e->loop2 < c->B.iter)
      {
         if (e->loop1 < c->B.per)
         {
            e->loop1++;
            e->enable = 0;
            if (e->duty_cnt < c->B.duty)
            {
               e->enable = 1;
               aux = e->pos;
               e->duty_cnt++;
               e->pos += c->B.incr;
            }
         }
         if (e->loop1 == c->B.per)
         {
            e->loop1 = 0;
            e->duty_cnt = 0;
            e->loop2++;
            e->pos += c->B.shift;
         }
      }
      if (e->loop2 == c->B.iter)
      {
         e->loop2 = 0;
         e->done = 1;
      }
   }
   else
   {
      if (e->loop4 < c->B.iter2)
      {
         if (e->loop3 < c->B.per2)
         {
            if (e->loop2 < c->B.iter)
            {
               if (e->loop1 < c->B.per)
               {
                  e->loop1++;
                  e->enable = 0;
                  if (e->duty_cnt < c->B.duty)
                  {
                     e->enable = 1;
                     aux = e->pos;
                     e->duty_cnt++;
                     e->pos += c->B.incr;
                  }
               }
               if (e->loop1 == c->B.per)
               {
                  e->loop1 = 0;
                  e->loop2++;
                  e->duty_cnt = 0;
                  e->pos += c->B.shift;
               }
            }
            if (e->loop2 == c->B.iter)
            {
               e->loop2 = 0;
               e->pos2 += c->B.incr2;
               e->pos = e->pos2;
               e->loop3++;
            }
         }
         if (e->loop3 == c->B.per2)
         {
            e->pos2 += c->B.shift2;
            e->pos = e->pos2;
            e->loop3 = 0;
            e->loop4++;
         }
      }
      if (e->loop4 == c->B.iter2)
      {
         e->done = 1;
      }
   }

   mem[aux] = GetInputValue(instance,0);

   return NULL;
}

int32_t* AluliteUpdateFunction(FUInstance* instance){
   static int32_t out;

   int32_t a = GetInputValue(instance,0);
	int32_t b = GetInputValue(instance,1);

   AluliteConfig* c = (AluliteConfig*) instance->config;

   int32_t* storedDelay = (int32_t*) instance->extraData;

   out = *storedDelay;

   int res = 0;
   switch (c->fns)
   {
   case ALULITE_OR:
     res = a | b;
     break;
   case ALULITE_AND:
     res = a & b;
     break;
   case ALULITE_CMP_SIG:
     break;
   case ALULITE_MUX:
     res = a < 0 ? b : c->self_loop == 1 ? out : 0;
     break;
   case ALULITE_SUB:
     res = a < 0 ? b : a - b;
     break;
   case ALULITE_ADD:
     res =  a + b;
     if (c->self_loop)
         res =  a < 0 ? b : out;
     break;
   case ALULITE_MAX:
     res =  (a > b ? a : b);
     break;
   case ALULITE_MIN:
     res = (a < b ? a : b);
     break;
   default:
     break;
   }

   *storedDelay = res;

	return &out;
}

int32_t* MemStartFunction(FUInstance* instance){
   MemConfig* c = (MemConfig*) instance->config;
   MemExtra*  e = (MemExtra*) instance->extraData;

   e->done = 0;
   e->pos = c->A.start;
   e->pos2 = c->A.start;
   if (c->A.duty == 0)
      c->A.duty = c->A.per;
   e->run_delay = c->A.delay;

   return 0;
}

int32_t* MemUpdateFunction(FUInstance* instance){
   static int32_t out[2];

   MemConfig* c = (MemConfig*) instance->config;
   MemExtra*  e = (MemExtra*) instance->extraData;
   int* mem = (int*) instance->memMapped;

   out[0] = 0;

   for(int i = 0; i < MEMP_LAT - 1; i++){
      e->stored[i] = e->stored[i+1]; 
   }

   out[0] = e->stored[0];

	if(e->run_delay){
		e->run_delay--;
		return out;
	}

	if(e->done){
		return out;
	}

	uint32_t aux = 0;
	if (c->A.iter2 == 0 && c->A.per2 == 0)
    {
        if (e->loop2 < c->A.iter)
        {
            if (e->loop1 < c->A.per)
            {
                e->loop1++;
                e->enable = 0;
                if (e->duty_cnt < c->A.duty)
                {
                    e->enable = 1;
                    aux = e->pos;
                    e->duty_cnt++;
                    e->pos += c->A.incr;
                }
            }
            if (e->loop1 == c->A.per)
            {
                e->loop1 = 0;
                e->duty_cnt = 0;
                e->loop2++;
                e->pos += c->A.shift;
            }
        }
        if (e->loop2 == c->A.iter)
        {
            e->loop2 = 0;
            e->done = 1;
        }
    }
    else
    {
        if (e->loop4 < c->A.iter2)
        {
            if (e->loop3 < c->A.per2)
            {
                if (e->loop2 < c->A.iter)
                {
                    if (e->loop1 < c->A.per)
                    {
                        e->loop1++;
                        e->enable = 0;
                        if (e->duty_cnt < c->A.duty)
                        {
                            e->enable = 1;
                            aux = e->pos;
                            e->duty_cnt++;
                            e->pos += c->A.incr;
                        }
                    }
                    if (e->loop1 == c->A.per)
                    {
                        e->loop1 = 0;
                        e->loop2++;
                        e->duty_cnt = 0;
                        e->pos += c->A.shift;
                    }
                }
                if (e->loop2 == c->A.iter)
                {
                    e->loop2 = 0;
                    e->pos2 += c->A.incr2;
                    e->pos = e->pos2;
                    e->loop3++;
                }
            }
            if (e->loop3 == c->A.per2)
            {
                e->pos2 += c->A.shift2;
                e->pos = e->pos2;
                e->loop3 = 0;
                e->loop4++;
            }
        }
        if (e->loop4 == c->A.iter2)
        {
            e->done = 1;
        }
    }
    
    if(c->A.in_wr){
         if(e->enable){
            mem[aux] = GetInputValue(instance,0);
         }
    } else {
         e->stored[MEMP_LAT-1] = mem[aux];
    }

    if(e->debug){
        printf("[mem] %d:%d\n",aux,out[0]);
    }

    return out;
}

int32_t* MulAddStartFunction(FUInstance* instance){
    MuladdConfig* c = (MuladdConfig*) instance->config;
    MuladdExtra* e = (MuladdExtra*) instance->extraData;

    e->run_delay = c->delay;

    //set addrgen counter variables
    e->incr = 1;
    e->loop1 = 0;
    e->duty = c->per;
    e->loop2 = 0;
    e->pos = 0;
    e->shift_addr = -c->per;
    e->duty_cnt = 0;
    e->cnt_addr = 0;
    e->done = 0;

    return 0;
}

int32_t* MulAddUpdateFunction(FUInstance* instance){
    static int32_t out;

    int32_t opa = GetInputValue(instance,0);
    int32_t opb = GetInputValue(instance,1);

    MuladdConfig* c = (MuladdConfig*) instance->config;
    MuladdExtra* e = (MuladdExtra*) instance->extraData;

    out = 0;

    for(int i = 0; i < MULADD_LAT-1; i++){
      e->stored[i] = e->stored[i+1]; 
    }

    out = e->stored[0];

    //check for delay
    if (e->run_delay > 0)
    {
        e->run_delay--;
        return &out;
    }

    if (e->loop2 < c->iter)
    {
        if (e->loop1 < c->per)
        {
            e->loop1++;
            e->enable = 0;
            if (e->duty_cnt < e->duty)
            {
                e->enable = 1;
                e->aux = e->pos;
                e->duty_cnt++;
                e->pos += e->incr;
            }
        }
        if (e->loop1 == c->per)
        {
            e->loop1 = 0;
            e->duty_cnt = 0;
            e->loop2++;
            e->pos += e->shift_addr;
        }
    }
    if (e->loop2 == c->iter)
    {
        e->loop2 = 0;
        e->done = 1;
    }
    e->cnt_addr = e->aux;

    //select acc_w value
    e->acc_w = (e->cnt_addr == 0) ? 0 : e->acc;

    //perform MAC operation
    int32_t result_mult = opa * opb;
    if (c->fns == MULADD_MACC)
    {
        e->acc = e->acc_w + result_mult;
    }
    else
    {
        e->acc = e->acc_w - result_mult;
    }
    e->stored[MULADD_LAT-1] = (int32_t)(e->acc >> c->shift);

    if(e->debug)
        printf("[muladd] %d %d %d: %d\n",e->acc,opa,opb,e->stored[MULADD_LAT-1]);

    return &out;
}

int32_t* RegStartFunction(FUInstance* inst){
   static int32_t startValue;

   RegConfig* config = (RegConfig*) inst->config;
   int32_t* view = (int32_t*) inst->extraData;

   view[0] = config->writeDelay;
   
   startValue = inst->outputs[0]; // Kinda sketchy

   if(!view[1]){
      startValue = config->initialValue;
      inst->state[0] = startValue;
      view[1] = 1;
   }

   return &startValue;
}

int32_t* RegUpdateFunction(FUInstance* inst){
   static int32_t out;

   out = inst->outputs[0]; // By default, keep same output
   int32_t* delayView = (int32_t*) inst->extraData;

   if((*delayView) > 0){
      if(*delayView == 1){
         out = GetInputValue(inst,0);
      }
      *delayView -= 1;
   }

   inst->state[0] = out;

   return &out;
}

int32_t* AddFunction(FUInstance* inst){
   static int32_t out;

   out = (GetInputValue(inst,0)) + (GetInputValue(inst,1));

   return &out;
}
*/