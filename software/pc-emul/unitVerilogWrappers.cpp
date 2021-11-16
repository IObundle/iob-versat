#include "unitVerilogWrappers.h"
#include "stdio.h"
#include "math.h"

#include "xadd.h"
#include "xreg.h"
#include "xmem.h"
#include "vread.h"
#include "vwrite.h"

#define INSTANTIATE_ARRAY
#include "unitData.h"
#undef INSTANTIATE_ARRAY

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

// For now it is a name changed copy of GetInputValue, in order to compile
static int32_t GetInput(FUInstance* instance,int index){
   FUInput input = instance->inputs[index];
   FUInstance* inst = input.instance;

   if(inst){
      return inst->outputs[input.index];   
   }
   else{
      return 0;
   }   
}

#define INIT(unit) \
   self->run = 0; \
   self->clk = 0; \
   self->rst = 0;

#define UPDATE(unit) \
   self->clk = 1; \
   self->eval(); \
   self->clk = 0; \
   self->eval();

#define RESET(unit) \
   self->rst = 1; \
   UPDATE(unit); \
   self->rst = 0;

#define START_RUN(unit) \
   self->run = 1; \
   UPDATE(unit); \
   self->run = 0;

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

   self->in0 = GetInput(inst,0);
   self->in1 = GetInput(inst,1);

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
                                    AddUpdateFunction);

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
   self->initialValue = config->initialValue;
   self->writeDelay = config->writeDelay;

   START_RUN(self);

   return NULL;
}

static int32_t* RegUpdateFunction(FUInstance* inst){
   static int32_t out;
   Vxreg* self = (Vxreg*) inst->extraData;
   RegState* state = (RegState*) inst->state;

   self->in0 = GetInput(inst,0);

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
                                    0, // MemoryMapped
                                    false, // IO
                                    sizeof(Vxreg), // Extra memory
                                    RegInitializeFunction,
                                    RegStartFunction,
                                    RegUpdateFunction);

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

   self->in0 = GetInput(inst,0);

   UPDATE(self);

   // Update out
   out[0] = self->out0;
   out[1] = self->out1;

   return out;
}

EXPORT FU_Type RegisterMem(Versat* versat,int addr_w){
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

   FU_Type type = RegisterFU(versat,buffer,
                                    2, // n inputs
                                    2, // n outputs
                                    ARRAY_SIZE(memConfigWires), // Config
                                    instanceWires, 
                                    0, // State
                                    NULL,
                                    (1 << addr_w) * 4, // MemoryMapped
                                    false, // IO
                                    sizeof(Vxmem), // Extra memory
                                    MemInitializeFunction,
                                    MemStartFunction,
                                    MemUpdateFunction);

   return type;
}

static int32_t* VReadInitializeFunction(FUInstance* inst){
   Vvread* self = new (inst->extraData) Vvread();

   INIT(self);
   
   RESET(self);

   return NULL;
}

static int32_t* VReadStartFunction(FUInstance* inst){
   Vvread* self = (Vvread*) inst->extraData;
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
   Vvread* self = (Vvread*) inst->extraData;

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
                                    sizeof(Vvread), // Extra memory
                                    VReadInitializeFunction,
                                    VReadStartFunction,
                                    VReadUpdateFunction);

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

   self->in0 = GetInput(inst,0);

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
                                    VWriteUpdateFunction);

   return type;
}