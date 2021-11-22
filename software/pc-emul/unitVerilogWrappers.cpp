#include <new>

#include "unitVerilogWrappers.h"
#include "stdio.h"
#include "math.h"

#include "Vxadd.h"
#include "Vxreg.h"
#include "Vxmem.h"
#include "Vvread.h"
#include "Vvwrite.h"

#define INSTANTIATE_ARRAY
#include "unitData.h"
#undef INSTANTIATE_ARRAY

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define INIT(unit) \
   self->run = 0; \
   self->clk = 0; \
   self->rst = 0;

#define UPDATE(unit) \
   self->clk = 0; \
   self->eval(); \
   self->clk = 1; \
   self->eval();

#define RESET(unit) \
   self->rst = 1; \
   UPDATE(unit); \
   self->rst = 0;

#define START_RUN(unit) \
   self->run = 1; \
   UPDATE(unit); \
   self->run = 0;

#define DEBUG_PRINT(unit) \
   printf("%d %d %02x %08x %08x\n",unit->valid,unit->ready,unit->wstrb,unit->wdata,unit->rdata);

template<typename T>
static int32_t MemoryAccess(FUInstance* inst,int address,int value,int write){
   T* self = (T*) inst->extraData;

   if(write){
      self->valid = 1;
      self->wstrb = 0xf;
      self->addr = address;
      self->wdata = value;

      self->eval();

      while(!self->ready){
         UPDATE(self);
      }

      self->valid = 0;
      self->wstrb = 0x00;
      self->addr = 0x00000000;
      self->wdata = 0x00000000;

      UPDATE(self);

      return 0;
   } else {
      self->valid = 1;
      self->wstrb = 0x0;
      self->addr = address;

      self->eval();

      while(!self->ready){
         UPDATE(self);
      }

      int32_t res = self->rdata;

      self->valid = 0;
      self->addr = 0;

      UPDATE(self);

      return res;
   }
}

static int32_t* AddInitializeFunction(FUInstance* inst){
   Vxadd* self = new (inst->extraData) Vxadd();

   INIT(self);
   
   self->in0 = 0;
   self->in1 = 0;

   RESET(self);

   return NULL;
}

static int32_t* AddStartFunction(FUInstance* inst){
   Vxadd* self = (Vxadd*) inst->extraData;

   START_RUN(self);

   return NULL;
}

static int32_t* AddUpdateFunction(FUInstance* inst){
   static int32_t out;

   Vxadd* self = (Vxadd*) inst->extraData;

   self->in0 = GetInputValue(inst,0);
   self->in1 = GetInputValue(inst,1);

   UPDATE(self);

   out = self->out0;

   return &out;
}

EXPORT FU_Type RegisterAdd(Versat* versat){
   FU_Type type = RegisterFU(versat,"xadd",
                                    2, // n inputs
                                    1, // n outputs
                                    0, // Config
                                    NULL,
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    false, // IO
                                    sizeof(Vxadd), // Extra memory
                                    AddInitializeFunction,
                                    AddStartFunction,
                                    AddUpdateFunction,
                                    NULL);

   return type;
}

static int32_t* RegInitializeFunction(FUInstance* inst){
   Vxreg* self = new (inst->extraData) Vxreg();

   INIT(self);

   self->in0 = 0;

   RESET(self);

   return NULL;
}

static int32_t* RegStartFunction(FUInstance* inst){
   Vxreg* self = (Vxreg*) inst->extraData;
   RegConfig* config = (RegConfig*) inst->config;

   // Update config
   self->writeDelay = config->writeDelay;

   START_RUN(self);

   return NULL;
}

static int32_t* RegUpdateFunction(FUInstance* inst){
   static int32_t out;
   Vxreg* self = (Vxreg*) inst->extraData;
   RegState* state = (RegState*) inst->state;

   self->in0 = GetInputValue(inst,0);

   UPDATE(self);

   // Update state
   state->currentValue = self->currentValue;

   // Update out
   out = self->out0;

   return &out;
}

EXPORT FU_Type RegisterReg(Versat* versat){
   FU_Type type = RegisterFU(versat,"xreg",
                                    1, // n inputs
                                    1, // n outputs
                                    ARRAY_SIZE(regConfigWires), // Config
                                    regConfigWires,
                                    ARRAY_SIZE(regStateWires), // State
                                    regStateWires,
                                    4, // MemoryMapped
                                    false, // IO
                                    sizeof(Vxreg), // Extra memory
                                    RegInitializeFunction,
                                    RegStartFunction,
                                    RegUpdateFunction,
                                    MemoryAccess<Vxreg>);

   return type;
}

static int32_t* MemInitializeFunction(FUInstance* inst){
   Vxmem* self = new (inst->extraData) Vxmem();

   INIT(self);

   self->in0 = 0;
   self->in1 = 0;

   RESET(self);

   return NULL;
}

static int32_t* MemStartFunction(FUInstance* inst){
   Vxmem* self = (Vxmem*) inst->extraData;
   MemConfig* config = (MemConfig*) inst->config;

   // Update config
   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->startA = config->startA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   self->delayA = config->delayA;
   self->reverseA = config->reverseA;
   self->extA = config->extA;
   self->in0_wr = config->in0_wr;
   self->iter2A = config->iter2A;
   self->per2A = config->per2A;
   self->shift2A = config->shift2A;
   self->incr2A = config->incr2A;
   self->iterB = config->iterB;
   self->perB = config->perB;
   self->dutyB = config->dutyB;
   self->startB = config->startB;
   self->shiftB = config->shiftB;
   self->incrB = config->incrB;
   self->delayB = config->delayB;
   self->reverseB = config->reverseB;
   self->extB = config->extB;
   self->in1_wr = config->in1_wr;
   self->iter2B = config->iter2B;
   self->per2B = config->per2B;
   self->shift2B = config->shift2B;
   self->incr2B = config->incr2B;

   START_RUN(self);

   return NULL;
}

static int32_t* MemUpdateFunction(FUInstance* inst){
   static int32_t out[2];
   Vxmem* self = (Vxmem*) inst->extraData;

   self->in0 = GetInputValue(inst,0);

   UPDATE(self);

   // Update out
   out[0] = self->out0;
   out[1] = self->out1;

   return out;
}

EXPORT FU_Type RegisterMem(Versat* versat,int addr_w){
   #if 0
   char* buffer = (char*) malloc(128 * sizeof(char)); // For now this memory is leaked.
   Wire* instanceWires = (Wire*) malloc(sizeof(Wire) * ARRAY_SIZE(memConfigWires)); // For now this memory is leaked.

   assert(addr_w < 20);

   sprintf(buffer,"xmem #(.ADDR_W(%d))",addr_w);

   memcpy(instanceWires,memConfigWires,sizeof(Wire) * ARRAY_SIZE(memConfigWires));

   for(int i = 0; i < ARRAY_SIZE(memConfigWires); i++){
      Wire* wire = &instanceWires[i];

      if(wire->bitsize == MEM_SUBSTITUTE_ADDR_TYPE)
         wire->bitsize = addr_w;
   }
   #endif

   FU_Type type = RegisterFU(versat,"xmem #(.ADDR_W(10))",
                                    2, // n inputs
                                    2, // n outputs
                                    ARRAY_SIZE(memConfigWires), // Config
                                    memConfigWires, 
                                    0, // State
                                    NULL,
                                    (1 << 10) * 4, // MemoryMapped
                                    false, // IO
                                    sizeof(Vxmem), // Extra memory
                                    MemInitializeFunction,
                                    MemStartFunction,
                                    MemUpdateFunction,
                                    MemoryAccess<Vxmem>);

   return type;
}

// Since it is very likely that memory accesses will stall on a board, 
// delay by a couple of seconds, in order to provide a closer to the expected experience
#define INITIAL_MEMORY_LATENCY 5
#define MEMORY_LATENCY 2

struct VReadExtra{
   Vvread vvread;
   int memoryAccessCounter;
};

static int32_t* VReadInitializeFunction(FUInstance* inst){
   VReadExtra* extra = new (inst->extraData) VReadExtra();
   Vvread* self = &extra->vvread;

   extra->memoryAccessCounter = INITIAL_MEMORY_LATENCY;

   INIT(self);
   
   RESET(self);

   return NULL;
}

static int32_t* VReadStartFunction(FUInstance* inst){
   VReadExtra* extra = (VReadExtra*) inst->extraData;
   Vvread* self = &extra->vvread;
   VReadConfig* config = (VReadConfig*) inst->config;

   // Update config
   self->ext_addr = config->ext_addr;
   self->int_addr = config->int_addr;
   self->size = config->size;
   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   self->iterB = config->iterB;
   self->perB = config->perB;
   self->dutyB = config->dutyB;
   self->startB = config->startB;
   self->shiftB = config->shiftB;
   self->incrB = config->incrB;
   self->delayB = config->delayB;
   self->reverseB = config->reverseB;
   self->extB = config->extB;
   self->iter2B = config->iter2B;
   self->per2B = config->per2B;
   self->shift2B = config->shift2B;
   self->incr2B = config->incr2B;

   START_RUN(self);

   return NULL;
}

static int32_t* VReadUpdateFunction(FUInstance* inst){
   static int32_t out;
   VReadExtra* extra = (VReadExtra*) inst->extraData;
   Vvread* self = &extra->vvread;

   self->databus_ready = 0;

   if(self->databus_valid){
      if(extra->memoryAccessCounter > 0){
         extra->memoryAccessCounter -= 1;
      } else {
         int* ptr = (int*) (self->databus_addr);
         self->databus_rdata = *ptr;
         self->databus_ready = 1;
         extra->memoryAccessCounter = MEMORY_LATENCY;
      }
   }

   UPDATE(self);

   // Update out
   out = self->out0;

   return &out;
}

EXPORT FU_Type RegisterVRead(Versat* versat){
   FU_Type type = RegisterFU(versat,"vread",
                                    0, // n inputs
                                    1, // n outputs
                                    ARRAY_SIZE(vreadConfigWires), // Config
                                    vreadConfigWires, 
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    true, // IO
                                    sizeof(VReadExtra), // Extra memory
                                    VReadInitializeFunction,
                                    VReadStartFunction,
                                    VReadUpdateFunction,
                                    NULL);

   return type;
}

static int32_t* VWriteInitializeFunction(FUInstance* inst){
   Vvwrite* self = new (inst->extraData) Vvwrite();

   INIT(self);
   
   RESET(self);

   return NULL;
}

static int32_t* VWriteStartFunction(FUInstance* inst){
   Vvwrite* self = (Vvwrite*) inst->extraData;
   VWriteConfig* config = (VWriteConfig*) inst->config;

   // Update config
   self->ext_addr = config->ext_addr;
   self->int_addr = config->int_addr;
   self->size = config->size;
   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   self->iterB = config->iterB;
   self->perB = config->perB;
   self->dutyB = config->dutyB;
   self->startB = config->startB;
   self->shiftB = config->shiftB;
   self->incrB = config->incrB;
   self->delayB = config->delayB;
   self->reverseB = config->reverseB;
   self->extB = config->extB;
   self->iter2B = config->iter2B;
   self->per2B = config->per2B;
   self->shift2B = config->shift2B;
   self->incr2B = config->incr2B;

   START_RUN(self);

   return NULL;
}

static int32_t* VWriteUpdateFunction(FUInstance* inst){
   static int32_t out;
   Vvwrite* self = (Vvwrite*) inst->extraData;

   self->in0 = GetInputValue(inst,0);

   UPDATE(self);

   // Update out
   out = self->out0;

   return &out;
}

EXPORT FU_Type RegisterVWrite(Versat* versat){
   FU_Type type = RegisterFU(versat,"vwrite",
                                    1, // n inputs
                                    0, // n outputs
                                    ARRAY_SIZE(vwriteConfigWires), // Config
                                    vwriteConfigWires, 
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    true, // IO
                                    sizeof(Vvwrite), // Extra memory
                                    VWriteInitializeFunction,
                                    VWriteStartFunction,
                                    VWriteUpdateFunction,
                                    NULL);

   return type;
}

static int32_t* DebugStartFunction(FUInstance* inst){
   int* extra = (int*) inst->extraData;

   *extra = 0;

   return NULL;
}

static int32_t* DebugUpdateFunction(FUInstance* inst){
   int* extra = (int*) inst->extraData;

   int32_t val = GetInputValue(inst,0);

   printf("[Debug %d]: 0x%x\n",*extra,val);

   (*extra) += 1;

   return NULL;
}

EXPORT FU_Type RegisterDebug(Versat* versat){
   FU_Type type = RegisterFU(versat,"",
                                    1, // n inputs
                                    0, // n outputs
                                    0, // Config
                                    NULL,
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    false, // IO
                                    sizeof(int), // Extra memory
                                    NULL,
                                    DebugStartFunction,
                                    DebugUpdateFunction,
                                    NULL
                                    );

   return type;
}
