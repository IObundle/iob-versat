#ifndef INCLUDED_VERILOG_DATA
#define INCLUDED_VERILOG_DATA

#{for module modules}
#{if module.nConfigs}

struct @{module.name}Config{
#{for i module.nConfigs}
#{set wire module.configs[i]}
int @{wire.name};
#{end}
};

#{end}

#{if module.nStates}

struct @{module.name}State{
#{for i module.nStates}
#{set wire module.states[i]}
int @{wire.name};
#{end}
};

#{end}
#{end}

#ifdef IMPLEMENT_VERILOG_UNITS
#include <new>

#include "versatPrivate.hpp"
#include "utils.hpp"

#{for module modules}
#include "V@{module.name}.h"
#{end}


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

#define PREAMBLE(type) \
   type* self = &data->unit; \
   VCDData* vcd = &data->vcd;

template<typename T>
static int32_t MemoryAccessNoAddress(ComplexFUInstance* inst,int address,int value,int write){
   T* self = (T*) inst->extraData;

   if(write){
      self->valid = 1;
      self->wstrb = 0xf;
      self->wdata = value;

      self->eval();

      while(!self->ready){
         UPDATE(self);
      }

      self->valid = 0;
      self->wstrb = 0x00;
      self->wdata = 0x00000000;

      UPDATE(self);

      return 0;
   } else {
      self->valid = 1;
      self->wstrb = 0x0;

      self->eval();

      while(!self->ready){
         UPDATE(self);
      }

      int32_t res = self->rdata;

      self->valid = 0;

      UPDATE(self);

      return res;
   }
}

template<typename T>
static int32_t MemoryAccess(ComplexFUInstance* inst,int address,int value,int write){
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

struct DatabusAccess{
   int counter;
   int latencyCounter;
};

#define INITIAL_MEMORY_LATENCY 5
#define MEMORY_LATENCY 2

#{for module modules}
static int32_t* @{module.name}_InitializeFunction(ComplexFUInstance* inst){
   V@{module.name}* self = new (inst->extraData) V@{module.name}();

   INIT(self);

#{for i module.nInputs}
   self->in@{i} = 0;
#{end}

   RESET(self);

   return NULL;
}

static int32_t* @{module.name}_StartFunction(ComplexFUInstance* inst){
#{if module.nOutputs}
   static int32_t out[@{module.nOutputs}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

#{if module.nDelays}
#{for i module.nDelays}
   self->delay@{i} = inst->delay[@{i}];
#{end}
#{end}

#{if module.nConfigs}
@{module.name}Config* config = (@{module.name}Config*) inst->config;
#{for i module.nConfigs}
   self->@{module.configs[i].name} = config->@{module.configs[i].name};
#{end}
#{end}


#{if module.doesIO}
   DatabusAccess* memoryLatency = (DatabusAccess*) &self[1];

   memoryLatency->counter = 0;
   memoryLatency->latencyCounter = INITIAL_MEMORY_LATENCY;
#{end}

   START_RUN(self);

#{if module.hasDone}
   inst->done = self->done;
#{end}

#{if module.nOutputs}
   #{for i module.nOutputs}
      out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

static int32_t* @{module.name}_UpdateFunction(ComplexFUInstance* inst){
#{if module.nOutputs}
   static int32_t out[@{module.nOutputs}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

#{for i module.nInputs}
   self->in@{i} = GetInputValue(inst,@{i}); 
#{end}

#{if module.doesIO}
   DatabusAccess* access = (DatabusAccess*) &self[1];

   if(self->databus_valid){
      if(access->latencyCounter > 0){
         access->latencyCounter -= 1;
      } else {
         int* ptr = (int*) (self->databus_addr);
         
         if(self->databus_wstrb == 0){
            if(ptr == nullptr){
               self->databus_rdata = 0xfeeffeef; // Feed bad data if not set (in pc-emul is needed otherwise segfault)
            } else {
               self->databus_rdata = ptr[access->counter];
            }            
         } else { // self->databus_wstrb != 0
            if(ptr != nullptr){
               ptr[access->counter] = self->databus_wdata;
            }
         }
         self->databus_ready = 1;
         
         if(access->counter == self->databus_len){
            access->counter = 0;
            self->databus_last = 1;
         } else {
            access->counter += 1;            
         }

         access->latencyCounter = MEMORY_LATENCY;
      }
   }
#{end}

   UPDATE(self);

#{if module.doesIO}
   self->databus_ready = 0;
   self->databus_last = 0;
#{end}

#{if module.nStates}
@{module.name}State* state = (@{module.name}State*) inst->state;
#{for i module.nStates}
   state->@{module.states[i].name} = self->@{module.states[i].name};
#{end}
#{end}

#{if module.hasDone}
   inst->done = self->done;
#{end}

#{if module.nOutputs}
   #{for i module.nOutputs}
      out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

static int32_t* @{module.name}_DestroyFunction(ComplexFUInstance* inst){
   V@{module.name}* self = (V@{module.name}*) inst->extraData;

   self->~V@{module.name}();

   return nullptr;
}

static FUDeclaration* @{module.name}_Register(Versat* versat){
   FUDeclaration decl = {};

   #{if module.nInputs}
   decl.nInputs = @{module.nInputs};
   static int inputDelays[] =  {#{join "," for i module.nInputs}@{module.inputDelays[i]}#{end}};
   decl.inputDelays = inputDelays;
   #{end}

   #{if module.nOutputs}
   decl.nOutputs = @{module.nOutputs};
   static int latencies[] = {#{join "," for i module.nOutputs}@{module.outputLatencies[i]}#{end}};
   decl.latencies = latencies;
   #{end}

   decl.name = MakeSizedString("@{module.name}");

   #{if module.doesIO}
   decl.extraDataSize = sizeof(V@{module.name}) + sizeof(DatabusAccess);
   #{else}
   decl.extraDataSize = sizeof(V@{module.name});
   #{end}

   decl.initializeFunction = @{module.name}_InitializeFunction;
   decl.startFunction = @{module.name}_StartFunction;
   decl.updateFunction = @{module.name}_UpdateFunction;
   decl.destroyFunction = @{module.name}_DestroyFunction;

   #{if module.nConfigs}
   static Wire @{module.name}ConfigWires[] = {#{join "," for i module.nConfigs} {MakeSizedString("@{module.configs[i].name}"),@{module.configs[i].bitsize}} #{end}};
   decl.nConfigs = @{module.nConfigs};
   decl.configWires = @{module.name}ConfigWires;
   #{end}

   #{if module.nStates}
   static Wire @{module.name}StateWires[] = {#{join "," for i module.nStates} {MakeSizedString("@{module.states[i].name}"),@{module.states[i].bitsize}} #{end}};
   decl.nStates = @{module.nStates};
   decl.stateWires = @{module.name}StateWires;
   #{end}

   #{if module.hasDone}
   decl.implementsDone = true;
   #{end}

   #{if module.isSource}
   decl.delayType = decl.delayType | DelayType::DELAY_TYPE_SINK_DELAY;
   #{end}

   #{if module.doesIO}
   decl.nIOs = 1;
   #{end}

   #{if module.memoryMapped}
   decl.isMemoryMapped = true;
   decl.memoryMapBits = @{module.memoryMappedBits};
   #{if module.memoryMappedBits == 0}
   decl.memAccessFunction = MemoryAccessNoAddress<V@{module.name}>;
   #{else}
   decl.memAccessFunction = MemoryAccess<V@{module.name}>;
   #{end}
   #{end}

   decl.nDelays = @{module.nDelays};

   return RegisterFU(versat,decl);
}

#{end}

static void RegisterAllVerilogUnits(Versat* versat){
   #{for module modules}
   @{module.name}_Register(versat);
   #{end}
}

#endif

#endif // INCLUDED_VERILOG_DATA