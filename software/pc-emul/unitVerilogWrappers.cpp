#include <new>

#include "versat.hpp"

#include "stdio.h"
#include "math.h"
#include "string.h"

#include "unitVCD.hpp"
#include "unitVerilogWrappers.hpp"

#include "Vxadd.h"
#include "Vxreg.h"
#include "Vxmem.h"
#include "Vvread.h"
#include "Vvwrite.h"
#include "Vpipeline_register.h"

#define INSTANTIATE_ARRAY
#include "unitData.hpp"
#undef INSTANTIATE_ARRAY

//#define TRACE
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define INIT(unit) \
   unit->run = 0; \
   unit->clk = 0; \
   unit->rst = 0;

#define UPDATE(unit) \
   unit->clk = 0; \
   unit->eval(); \
   unit->clk = 1; \
   unit->eval();

#define RESET(unit) \
   unit->rst = 1; \
   UPDATE(unit); \
   unit->rst = 0;

#define START_RUN(unit) \
   unit->run = 1; \
   UPDATE(unit); \
   unit->run = 0;

#ifdef TRACE
#define VCD_UPDATE(unit) \
   unit->clk = 0; \
   unit->eval(); \
   vcd->dump(); \
   unit->clk = 1; \
   unit->eval(); \
   vcd->dump();

#define VCD_RESET(unit) \
   unit->rst = 1; \
   VCD_UPDATE(unit); \
   unit->rst = 0;

#define VCD_START_RUN(unit) \
   VCD_UPDATE(unit); \
   unit->run = 1; \
   VCD_UPDATE(unit); \
   unit->run = 0;
#else
#define VCD_UPDATE(unit) UPDATE(unit)
#define VCD_RESET(unit) RESET(unit)
#define VCD_START_RUN(unit) START_RUN(unit)
#endif

#define PREAMBLE(type) \
   type* self = &data->unit; \
   VCDData* vcd = &data->vcd;

#define DEBUG_PRINT(unit) \
   printf("%d %d %02x %08x %08x\n",unit->valid,unit->ready,unit->wstrb,unit->wdata,unit->rdata);

static int zeros[100] = {};
static int ones[] = {1,1};
static int threes[] = {3,3};

template<typename T>
static int MemoryAccess(FUInstance* inst,int address,int value,int write){
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

      int res = self->rdata;

      self->valid = 0;
      self->addr = 0;

      UPDATE(self);

      return res;
   }
}

static int* AddInitializeFunction(FUInstance* inst){
   Vxadd* self = new (inst->extraData) Vxadd();

   INIT(self);

   self->in0 = 0;
   self->in1 = 0;

   RESET(self);

   inst->done = true;

   return NULL;
}

static int* AddStartFunction(FUInstance* inst){
   Vxadd* self = (Vxadd*) inst->extraData;

   START_RUN(self);

   return NULL;
}

static int* AddUpdateFunction(FUInstance* inst){
   static int out;

   Vxadd* self = (Vxadd*) inst->extraData;

   self->in0 = GetInputValue(inst,0);
   self->in1 = GetInputValue(inst,1);

   UPDATE(self);

   out = self->out0;

   return &out;
}

FUDeclaration* RegisterAdd(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"xadd");
   decl.nInputs = 2;
   decl.nOutputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = ones;
   decl.extraDataSize = sizeof(Vxadd);
   decl.initializeFunction = AddInitializeFunction;
   decl.startFunction = AddStartFunction;
   decl.updateFunction = AddUpdateFunction;

   return RegisterFU(versat,decl);
}

struct RegData{
   Vxreg unit;
   VCDData vcd;
};

static int* RegInitializeFunction(FUInstance* inst){
   static int regCounter;
   char buffer[256];
   RegData* data = new (inst->extraData) RegData();
   PREAMBLE(Vxreg);

   #ifdef TRACE
   self->trace(&data->vcd.vcd,99);
   sprintf(buffer,"./trace_out/reg%d.vcd",regCounter++);
   data->vcd.open(buffer);
   #endif

   INIT(self);

   self->in0 = 0;

   VCD_RESET(self);

   return NULL;
}

static int* RegStartFunction(FUInstance* inst){
   static int out;

   RegData* data = (RegData*) inst->extraData;
   PREAMBLE(Vxreg);

   // Update config
   self->delay0 = inst->delay[0];

   VCD_START_RUN(self);

   out = self->out0;

   return &out;
}

static int* RegUpdateFunction(FUInstance* inst){
   static int out;
   RegData* data = (RegData*) inst->extraData;
   RegState* state = (RegState*) inst->state;

   PREAMBLE(Vxreg);

   self->in0 = GetInputValue(inst,0);

   VCD_UPDATE(self);

   // Update state
   state->currentValue = self->currentValue;
   inst->done = self->done;

   // Update out
   out = self->out0;

   return &out;
}

FUDeclaration* RegisterReg(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"xreg");
   decl.nInputs = 1;
   decl.nOutputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.nStates = ARRAY_SIZE(regStateWires);
   decl.stateWires = regStateWires;
   decl.memoryMapBits = 0;
   decl.isMemoryMapped = true;
   decl.extraDataSize = sizeof(RegData);
   decl.initializeFunction = RegInitializeFunction;
   decl.startFunction = RegStartFunction;
   decl.updateFunction = RegUpdateFunction;
   decl.memAccessFunction = MemoryAccess<Vxreg>;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
   decl.nDelays = decl.nInputs;

   return RegisterFU(versat,decl);
}

static int* ConstStartFunction(FUInstance* inst){
   static int out;

   // Update config
   out = *inst->config;

   inst->done = true;

   return &out;
}

static int* ConstUpdateFunction(FUInstance* inst){
   static int out;

   out = *inst->config;

   return &out;
}

FUDeclaration* RegisterConst(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"xconst");
   decl.nOutputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.nConfigs = ARRAY_SIZE(constConfigWires);
   decl.configWires = constConfigWires;
   decl.startFunction = ConstStartFunction;
   decl.updateFunction = ConstUpdateFunction;

   return RegisterFU(versat,decl);
}

static int* MemInitializeFunction(FUInstance* inst){
   Vxmem* self = new (inst->extraData) Vxmem();

   INIT(self);

   self->in0 = 0;
   self->in1 = 0;

   RESET(self);

   return NULL;
}

static int* MemStartFunction(FUInstance* inst){
   Vxmem* self = (Vxmem*) inst->extraData;
   MemConfig* config = (MemConfig*) inst->config;

   // Update config
   self->delay0 = inst->delay[0];
   self->delay1 = inst->delay[0];

   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->startA = config->startA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   //self->delayA = config->delayA;
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
   //self->delayB = config->delayB;
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

static int* MemUpdateFunction(FUInstance* inst){
   static int out[2];
   Vxmem* self = (Vxmem*) inst->extraData;

   self->in0 = GetInputValue(inst,0);

   UPDATE(self);

   inst->done = self->done;

   // Update out
   out[0] = self->out0;
   out[1] = self->out1;

   return out;
}

FUDeclaration* RegisterMem(Versat* versat,int addr_w){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"xmem");
   decl.nInputs = 2;
   decl.nOutputs = 2;
   decl.inputDelays = zeros;
   decl.latencies = threes;
   decl.nConfigs = ARRAY_SIZE(memConfigWires);
   decl.configWires = memConfigWires;
   decl.memoryMapBits = 10;
   decl.isMemoryMapped = true;
   decl.extraDataSize = sizeof(Vxmem);
   decl.initializeFunction = MemInitializeFunction;
   decl.startFunction = MemStartFunction;
   decl.updateFunction = MemUpdateFunction;
   decl.memAccessFunction = MemoryAccess<Vxmem>;
   decl.delayType = (enum DelayType)(DELAY_TYPE_SOURCE_DELAY | DELAY_TYPE_IMPLEMENTS_DONE);
   decl.nDelays = decl.nInputs;

   return RegisterFU(versat,decl);
}

// Since it is very likely that memory accesses will stall on a board,
// delay by a couple of seconds, in order to provide a closer to the expected experience
#define INITIAL_MEMORY_LATENCY 5
#define MEMORY_LATENCY 2

struct VReadExtra{
   Vvread unit;
   VCDData vcd;
   int memoryAccessCounter;
};

static int vreadCounter = 0;

static int* VReadInitializeFunction(FUInstance* inst){
   char buffer[256];

   VReadExtra* data = new (inst->extraData) VReadExtra();
   PREAMBLE(Vvread);

   #ifdef TRACE
   self->trace(&vcd->vcd,99);
   sprintf(buffer,"./trace_out/vread%d.vcd",vreadCounter++);
   vcd->open(buffer);
   #endif

   data->memoryAccessCounter = INITIAL_MEMORY_LATENCY;

   INIT(self);

   VCD_RESET(self);

   return NULL;
}

static int* VReadStartFunction(FUInstance* inst){
   VReadExtra* data = (VReadExtra*) inst->extraData;
   VReadConfig* config = (VReadConfig*) inst->config;
   PREAMBLE(Vvread);

   // Update config
   self->delay0 = inst->delay[0];

   self->ext_addr = config->ext_addr;
   self->int_addr = config->int_addr;
   self->size = config->size;
   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   self->pingPong = config->pingPong;
   self->iterB = config->iterB;
   self->perB = config->perB;
   self->dutyB = config->dutyB;
   self->startB = config->startB;
   self->shiftB = config->shiftB;
   self->incrB = config->incrB;
   //self->delayB = config->delayB;
   self->reverseB = config->reverseB;
   self->extB = config->extB;
   self->iter2B = config->iter2B;
   self->per2B = config->per2B;
   self->shift2B = config->shift2B;
   self->incr2B = config->incr2B;

   VCD_START_RUN(self);

   return NULL;
}

static int* VReadUpdateFunction(FUInstance* inst){
   static int out;
   VReadExtra* data = (VReadExtra*) inst->extraData;
   PREAMBLE(Vvread);

   self->databus_ready = 0;

   if(self->databus_valid){
      if(data->memoryAccessCounter > 0){
         data->memoryAccessCounter -= 1;
      } else {
         int* ptr = (int*) (self->databus_addr);
         self->databus_rdata = *ptr;
         self->databus_ready = 1;
         data->memoryAccessCounter = MEMORY_LATENCY;
      }
   }

   VCD_UPDATE(self);

   // Update out
   out = self->out0;

   inst->done = self->done;

   return &out;
}

FUDeclaration* RegisterVRead(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"vread");
   decl.nInputs = 0;
   decl.nOutputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = ones;
   decl.nConfigs = ARRAY_SIZE(vreadConfigWires);
   decl.configWires = vreadConfigWires;
   decl.nIOs = 1;
   decl.extraDataSize = sizeof(VReadExtra);
   decl.initializeFunction = VReadInitializeFunction;
   decl.startFunction = VReadStartFunction;
   decl.updateFunction = VReadUpdateFunction;
   decl.delayType = (enum DelayType)(DELAY_TYPE_SOURCE_DELAY | DELAY_TYPE_IMPLEMENTS_DONE);
   decl.nDelays = 1;

   return RegisterFU(versat,decl);
}

static int* VWriteInitializeFunction(FUInstance* inst){
   Vvwrite* self = new (inst->extraData) Vvwrite();

   INIT(self);

   RESET(self);

   return NULL;
}

static int* VWriteStartFunction(FUInstance* inst){
   Vvwrite* self = (Vvwrite*) inst->extraData;
   VWriteConfig* config = (VWriteConfig*) inst->config;

   // Update config
   self->delay0 = inst->delay[0];

   self->ext_addr = config->ext_addr;
   self->int_addr = config->int_addr;
   self->size = config->size;
   self->iterA = config->iterA;
   self->perA = config->perA;
   self->dutyA = config->dutyA;
   self->shiftA = config->shiftA;
   self->incrA = config->incrA;
   self->pingPong = config->pingPong;
   self->iterB = config->iterB;
   self->perB = config->perB;
   self->dutyB = config->dutyB;
   self->startB = config->startB;
   self->shiftB = config->shiftB;
   self->incrB = config->incrB;
   //self->delayB = config->delayB;
   self->reverseB = config->reverseB;
   self->extB = config->extB;
   self->iter2B = config->iter2B;
   self->per2B = config->per2B;
   self->shift2B = config->shift2B;
   self->incr2B = config->incr2B;

   START_RUN(self);

   return NULL;
}

static int* VWriteUpdateFunction(FUInstance* inst){
   static int out;
   Vvwrite* self = (Vvwrite*) inst->extraData;

   self->in0 = GetInputValue(inst,0);

   UPDATE(self);

   // Update out
   out = self->out0;

   inst->done = self->done;

   return &out;
}

FUDeclaration* RegisterVWrite(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"vwrite");
   decl.nInputs = 1;
   decl.nOutputs = 0;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.nConfigs = ARRAY_SIZE(vwriteConfigWires);
   decl.configWires = vwriteConfigWires;
   decl.nIOs = 1;
   decl.extraDataSize = sizeof(Vvwrite);
   decl.initializeFunction = VWriteInitializeFunction;
   decl.startFunction = VWriteStartFunction;
   decl.updateFunction = VWriteUpdateFunction;
   decl.delayType = (enum DelayType)(DELAY_TYPE_SINK_DELAY | DELAY_TYPE_IMPLEMENTS_DONE);
   decl.nDelays = decl.nInputs;

   return RegisterFU(versat,decl);
}

static int* DebugStartFunction(FUInstance* inst){
   int* extra = (int*) inst->extraData;

   *extra = 0;

   return NULL;
}

static int* DebugUpdateFunction(FUInstance* inst){
   int* extra = (int*) inst->extraData;

   int val = GetInputValue(inst,0);

   printf("[Debug %d]: 0x%x\n",*extra,val);

   (*extra) += 1;

   return NULL;
}

FUDeclaration* RegisterDebug(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"debug");
   decl.nInputs = 99;
   decl.nOutputs = 0;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.extraDataSize = sizeof(int);
   decl.startFunction = DebugStartFunction;
   decl.updateFunction = DebugUpdateFunction;
   decl.delayType = DELAY_TYPE_SINK_DELAY;
   decl.nDelays = decl.nInputs;

   return RegisterFU(versat,decl);
}

struct PipeRegData{
   Vpipeline_register unit;
   VCDData vcd;
};

static int* PipelineRegisterInitializeFunction(FUInstance* inst){
   static int pipeRegCounter = 0;
   char buffer[256];
   PipeRegData* data = new (inst->extraData) PipeRegData();
   PREAMBLE(Vpipeline_register);

   #ifdef TRACE
   self->trace(&data->vcd.vcd,99);

   sprintf(buffer,"./trace_out/pipeReg%d.vcd",pipeRegCounter++);
   data->vcd.open(buffer);
   #endif

   INIT(self);

   self->in0 = 0;

   VCD_RESET(self);

   return NULL;
}

static int* PipelineRegisterStartFunction(FUInstance* inst){
   static int out;
   PipeRegData* data = (PipeRegData*) inst->extraData;
   PREAMBLE(Vpipeline_register);

   VCD_START_RUN(self);

   out = self->out0;
   inst->done = true;

   return &out;
}

static int* PipelineRegisterUpdateFunction(FUInstance* inst){
   static int out;
   PipeRegData* data = (PipeRegData*) inst->extraData;
   PREAMBLE(Vpipeline_register);

   self->in0 = GetInputValue(inst,0);

   VCD_UPDATE(self);

   // Update out
   out = self->out0;

   inst->done = true;

   return &out;
}

FUDeclaration* RegisterPipelineRegister(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"pipeline_register");
   decl.nInputs = 1;
   decl.nOutputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = ones;
   decl.extraDataSize = sizeof(PipeRegData);
   decl.initializeFunction = PipelineRegisterInitializeFunction;
   decl.startFunction = PipelineRegisterStartFunction;
   decl.updateFunction = PipelineRegisterUpdateFunction;
   decl.type = FUDeclaration::SINGLE;

   return RegisterFU(versat,decl);
}

int* MergeStartFunction(FUInstance* inst){
   static int out;

   int* extraData = (int*) inst->extraData;

   extraData[0] = inst->delay[0];
   extraData[1] = 0;

   out = GetInputValue(inst,0);

   return &out;
}

int* MergeUpdateFunction(FUInstance* inst){
   static int out;

   int* extraData = (int*) inst->extraData; // 0 - delay, 1 - counter

   out = GetInputValue(inst,extraData[1]);

   if(extraData[0] == 0){
      extraData[1] += 1;
   } else {
      extraData[0] -= 1;
   }

   return &out;
}

FUDeclaration* RegisterMerge(Versat* versat){
   static int inputDelays[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
   FUDeclaration decl = {};

   strcpy(decl.name.str,"Merge");
   decl.nInputs = 16;
   decl.nOutputs = 1;
   decl.inputDelays = inputDelays;
   decl.latencies = ones;
   decl.extraDataSize = sizeof(int) * 2;
   decl.startFunction = MergeStartFunction;
   decl.updateFunction = MergeUpdateFunction;
   decl.type = FUDeclaration::SINGLE;
   decl.nDelays = 1;

   return RegisterFU(versat,decl);
}


