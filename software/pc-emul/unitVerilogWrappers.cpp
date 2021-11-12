#include "unitVerilogWrappers.h"
#include "stdio.h"

#include "xadd.h"

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
   self->eval(); \
   self->run = 0;

EXPORT int AddExtraSize(){
   return sizeof(Vxadd);
}

EXPORT int32_t* AddInitializeFunction(FUInstance* inst){
   Vxadd* self = new (inst->extraData) Vxadd();

   self->run = 0;
   self->in0 = 0;
   self->in1 = 0;

   RESET(self);

   return NULL;
}

EXPORT int32_t* AddStartFunction(FUInstance* inst){
   Vxadd* self = (Vxadd*) inst->extraData;

   START_RUN(self);

   return NULL;
}

EXPORT int32_t* AddUpdateFunction(FUInstance* inst){
   static int32_t out;

   Vxadd* self = (Vxadd*) inst->extraData;

   self->in0 = GetInput(inst,0);
   self->in1 = GetInput(inst,1);

   UPDATE(self);

   out = self->out0;

   return &out;
}