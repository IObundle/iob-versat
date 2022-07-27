#include <new>

#include "versat.hpp"

#include "stdio.h"
#include "math.h"
#include "string.h"

#include "unitVCD.hpp"

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

#define PREAMBLE(type) \
   type* self = &data->unit; \
   VCDData* vcd = &data->vcd;

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

#{for module modules}
static init32_t* @{module.name}_InitializeFunction(FUInstance* inst){
   V@{module.name} self = new (inst->extraData) V@{module.name}();

   INIT(self);

#{for i module.nInputs}
   self->in@{i} = 0;
#{end}

   RESET(self);

   inst->done = self->done;

   return NULL;
}

static init32_t* @{module.name}_StartFunction(FUInstance* inst){
#{if module.nOutputs}
   static int32_t out[@{module.nOutputs}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

   START_RUN(self);

#{if module.nOutputs}
   #{for i module.nOutputs}
      out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

static init32_t* @{module.name}_UpdateFunction(FUInstance* inst){
#{if module.nOutputs}
   static int32_t out[@{module.nOutputs}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

#{for i module.nInputs}
   self->in@{i} = GetInputValue(inst,@{i}); 
#{end}

   UPDATE(self);

#{if module.nOutputs}
   #{for i module.nOutputs}
      out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

static int32_t* @{module.name}_Register(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"@{module.name}");
   decl.nInputs = @{module.nInputs};
   decl.nOutputs = @{module.nOutputs};
   decl.inputDelays = {#{join "," for i module.nInputs}@{module.inputDelays[i]}#{end}};
   decl.latencies = {#{join "," for i module.nOutputs}@{module.latencies[i]}#{end}};
   decl.extraDataSize = sizeof(V@{module.name});
   decl.initializeFunction = @{module.name}_InitializeFunction;
   decl.startFunction = @{module.name}_StartFunction;
   decl.updateFunction = @{module.name}_UpdateFunction;

   #{if module.doesIO}
   decl.memAccessFunction = MemoryAccess<V@{module.name}>;
   #{end}


   return RegisterFU(versat,decl);
}


#{end}