#include "versat.h"

#include "stdlib.h"

static int maxi(int a1,int a2){
   int res = (a1 > a2 ? a1 : a2);

   return res;
}

static int mini(int a1,int a2){
   int res = (a1 < a2 ? a1 : a2);

   return res;
}

int UnitDelays(FUInstance* inst){
   if((inst->declaration->type & VERSAT_TYPE_IMPLEMENTS_DELAY)){
      if(inst->declaration->type & VERSAT_TYPE_SOURCE_DELAY){
         return mini(inst->declaration->nOutputs,2);
      } else {
         return 1;
      }
   } else {
      return 0;
   }
}

void CalculateNodesOutputs(Accelerator* accel){
   int size = 0;
   for(int i = 0; i < accel->nInstances; i++){
      size += accel->instances[i].declaration->nOutputs;
   }

   if(accel->nodeOutputsAuxiliary){
      free(accel->nodeOutputsAuxiliary);
   }

   accel->nodeOutputsAuxiliary = (FUInstance**) malloc(sizeof(FUInstance*) * 1024); // TODO: calculate based on need, no hardcoding

   int sortedOutputsIndex = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];

      inst->outputInstances = &accel->nodeOutputsAuxiliary[sortedOutputsIndex];

      int numberOutputs = 0;
      // Iterate over every other node
      for(int ii = 0; ii < accel->nInstances; ii++){
         FUInstance* other = &accel->instances[ii];

         if(other == inst){
            continue;
         }

         // Iterate over their inputs
         for(int iii = 0; iii < other->declaration->nInputs; iii++){
            if(other->inputs[iii].instance == inst){
               numberOutputs += 1;
               accel->nodeOutputsAuxiliary[sortedOutputsIndex++] = other;
            }
         }
      }

      inst->numberOutputs = numberOutputs;
   }
}

#define CHECK_TYPE(inst,T) ((inst->declaration->type & T) == T)

#define TAG_UNCONNECTED 0
#define TAG_SOURCE      1
#define TAG_SINK        2
#define TAG_COMPUTE     3
#define TAG_SOURCE_AND_SINK 4

// Gives latency as seen by unit, taking into account existing delay
static int CalculateUnitFullLatency(FUInstance* inst,int port,bool seenSourceAndSink){
   if(UnitDelays(inst) < 2){
      port = 0;
   }

   if(inst->tag == TAG_SOURCE)
      return inst->delays[port] + inst->declaration->latency;

   if(inst->tag == TAG_SOURCE_AND_SINK){
      if(seenSourceAndSink){
         if(CHECK_TYPE(inst,VERSAT_TYPE_SOURCE_DELAY)){
            return inst->delays[port] + inst->declaration->latency;
         } else {
            return inst->declaration->latency;
         }
      } else {
         seenSourceAndSink = true;
      }
   }

   int subLatency = 0;
   for(int i = 0; i < inst->declaration->nInputs; i++){
      if(inst->inputs[i].instance){
         subLatency = maxi(subLatency,CalculateUnitFullLatency(inst->inputs[i].instance,inst->inputs[i].index,seenSourceAndSink));
      }
   }

   return subLatency + inst->declaration->latency;
}

// Gives latency as seen by unit, taking into account existing delay
static int CalculateUnitInputLatency(FUInstance* instance,bool seenSourceAndSink){
   int latency = CalculateUnitFullLatency(instance,0,seenSourceAndSink) - instance->declaration->latency;

   return latency;
}

static void SetPathLatency(FUInstance* inst,int amount,int port){
   amount -= inst->declaration->latency;

   if(UnitDelays(inst) < 2){
      port = 0;
   }

   if(inst->tag == TAG_SOURCE){
      if(CHECK_TYPE(inst,VERSAT_TYPE_IMPLEMENTS_DELAY)){
         inst->delays[port] = amount;
      }
   } else if(inst->tag == TAG_SOURCE_AND_SINK){
      if(CHECK_TYPE(inst,VERSAT_TYPE_IMPLEMENTS_DELAY)
      && CHECK_TYPE(inst,VERSAT_TYPE_SOURCE_DELAY)){
         inst->delays[port] = amount;
      }
   }else if(inst->tag != TAG_SOURCE){
      for(int i = 0; i < inst->declaration->nInputs; i++){
         if(inst->inputs[i].instance){
            SetPathLatency(inst->inputs[i].instance,amount,inst->inputs[i].index);
         }
      }
   }
}

static int PropagateDelay(FUInstance* inst,bool seenSourceAndSink){
   int latencies[16];

   if(inst->tag == TAG_SOURCE)
      return inst->declaration->latency;

   if(inst->tag == TAG_SOURCE_AND_SINK){
      if(seenSourceAndSink){
         return inst->declaration->latency;
      } else {
         seenSourceAndSink = true;
      }
   }

   int minLatency = 0;
   for(int i = 0; i < inst->declaration->nInputs; i++){
      if(inst->inputs[i].instance){
         int latency = PropagateDelay(inst->inputs[i].instance,seenSourceAndSink);
         latencies[i] = latency;
         minLatency = maxi(minLatency,latency);
      } else {
         latencies[i] = -1;
      }
   }

   for(int i = 0; i < inst->declaration->nInputs; i++){
      if(latencies[i] != -1 && latencies[i] != minLatency){
         SetPathLatency(inst->inputs[i].instance,minLatency,inst->inputs[i].index);

         // TODO:
         // Multiple paths convergence detection can be detected here
         // if we check the latency of any other path change after calling SetPathLatency in any other path
      }
   }

   return minLatency + inst->declaration->latency;
}

EXPORT UnitInfo CalculateUnitInfo(FUInstance* inst){
   UnitInfo info = {};

   FUDeclaration decl = *inst->declaration;

   info.numberDelays = UnitDelays(inst);
   info.implementsDelay = (info.numberDelays > 0 ? true : false);
   info.nConfigsWithDelay = decl.nConfigs + info.numberDelays;

   if(decl.nConfigs){
      for(int ii = 0; ii < decl.nConfigs; ii++){
         info.configBitSize += decl.configWires[ii].bitsize;
      }
   }
   info.configBitSize += DELAY_BIT_SIZE * info.numberDelays;

   info.nStates = decl.nStates;
   if(decl.nStates){
      for(int ii = 0; ii < decl.nStates; ii++){
         info.stateBitSize += decl.stateWires[ii].bitsize;
      }
   }

   info.memoryMappedBytes = decl.memoryMapBytes;
   info.implementsDone = decl.doesIO;
   info.doesIO = decl.doesIO;

   return info;
}

void CalculateDelay(Accelerator* accel){
   // Reset delay to zero globally
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];

      inst->delays[0] = 0;
      inst->delays[1] = 0;
   }

   // Calculate output of nodes
   CalculateNodesOutputs(accel);

   // Tag units with the values we care for the delay computation
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];

      inst->tag = TAG_UNCONNECTED;

      bool hasInput = false;
      for(int ii = 0; ii < inst->declaration->nInputs; ii++){
         if(inst->inputs[ii].instance){
            hasInput = true;
            break;
         }
      }

      bool hasOutput = (inst->numberOutputs > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(CHECK_TYPE(inst,VERSAT_TYPE_SINK) && CHECK_TYPE(inst,VERSAT_TYPE_SOURCE)){
         if(hasInput && hasOutput){
            inst->tag = TAG_SOURCE_AND_SINK;
         } else if(hasInput){
            inst->tag = TAG_SINK;
         } else if(hasOutput){
            inst->tag = TAG_SOURCE;
         }
      }
      else if(CHECK_TYPE(inst,VERSAT_TYPE_SINK) && hasInput){
         inst->tag = TAG_SINK;
      }
      else if(CHECK_TYPE(inst,VERSAT_TYPE_SOURCE) && hasOutput){
         inst->tag = TAG_SOURCE;
      }
      else if(hasOutput && hasInput){
         inst->tag = TAG_COMPUTE;
      }

      //if(inst->tag == TAG_UNCONNECTED)
      //   assert(false); // For now, ignore units that have absolutely no connection or purpose, might affect the algorithm
   }

   // Compute delay to add to source units
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];

      // Compute delay starting from sinks
      if(inst->tag == TAG_SINK){
         PropagateDelay(inst,true);
      } else if(inst->tag == TAG_SOURCE_AND_SINK){ // Special compute if tag is both source and sink
         PropagateDelay(inst,false);
      }
   }

   // Compute delay to computation and sink units
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];

      // If it does not implement delay, ignore
      if(!CHECK_TYPE(inst,VERSAT_TYPE_IMPLEMENTS_DELAY))
         continue;

      if(inst->tag == TAG_SOURCE_AND_SINK && (!CHECK_TYPE(inst,VERSAT_TYPE_SOURCE_DELAY))){
         int delay = CalculateUnitInputLatency(inst,false);

         if(UnitDelays(inst) == 1){
            inst->delays[0] = delay;
         } else {
            for(int ii = 0; ii < inst->declaration->nOutputs; ii++)
               inst->delays[ii] = delay;
         }
      }

      if((inst->tag == TAG_SINK) || (inst->tag == TAG_COMPUTE)){
         int delay = CalculateUnitInputLatency(inst,true);
         if(UnitDelays(inst) == 1){
            inst->delays[0] = delay;
         } else {
            for(int ii = 0; ii < inst->declaration->nOutputs; ii++)
               inst->delays[ii] = delay;
         }
      }
   }
}
