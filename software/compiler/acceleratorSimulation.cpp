#include "acceleratorSimulation.hpp"

#include "versat.hpp"
#include "debug.hpp"

void AcceleratorRunStart(Accelerator* accel){
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,&accel->versat->temp,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* type = inst->declaration;

      if(type->startFunction){
         int* startingOutputs = type->startFunction(inst);

         if(startingOutputs){
            memcpy(inst->outputs,startingOutputs,inst->declaration->outputLatencies.size * sizeof(int));
            memcpy(inst->storedOutputs,startingOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         }
      }
   }
}

bool AcceleratorDone(Accelerator* accel){
   FOREACH_LIST(ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
      if(IsTypeHierarchical(inst->declaration) && !IsTypeSimulatable(inst->declaration)){
         bool subDone = AcceleratorDone(inst->declaration->fixedDelayCircuit);
         if(subDone){
            continue;
         } else {
            return false;
         }
      } else if(inst->declaration->implementsDone){
         if(inst->done){
            continue;
         } else {
            return false;
         }
      }
   }

   return true;
}

void AcceleratorEvalIter(AcceleratorIterator iter){
   // Set accelerator input to instance input
   InstanceNode* compositeNode = iter.Current();
   FUInstance* compositeInst = compositeNode->inst;
   Assert(IsTypeHierarchical(compositeInst->declaration));
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   // Order is very important. Need to populate before setting input values
   /* AcceleratorIterator it = */ iter.LevelBelowIterator();

   for(int i = 0; i < compositeNode->inputs.size; i++){
      if(compositeNode->inputs[i].node == nullptr){
         continue;
      }

      InstanceNode* inputNode = GetInputNode(accel->allocated,i);
      Assert(inputNode);
      FUInstance* input = inputNode->inst;
      int val = GetInputValue(compositeNode,i);
      for(int ii = 0; ii < input->declaration->outputLatencies.size; ii++){
         input->outputs[ii] = val;
         input->storedOutputs[ii] = val;
      }
   }

   // Set output instance value to accelerator output
   InstanceNode* output = GetOutputNode(accel->allocated);
   if(output){
      for(int ii = 0; ii < output->inputs.size; ii++){
         if(output->inputs[ii].node == nullptr){
            continue;
         }

         int val = GetInputValue(output,ii);
         compositeInst->outputs[ii] = val;
         compositeInst->storedOutputs[ii] = val;
      }
   }
}

void AcceleratorEval(AcceleratorIterator iter){
   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      FUInstance* inst = node->inst;
      if(IsTypeHierarchical(inst->declaration)){
         AcceleratorEvalIter(iter);
      }
   }
}

void AcceleratorRunComposite(AcceleratorIterator iter){
   // Set accelerator input to instance input
   InstanceNode* compositeNode = iter.Current();
   FUInstance* compositeInst = compositeNode->inst;
   Assert(IsTypeHierarchical(compositeInst->declaration));
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   // Order is very important. Need to populate before setting input values
   AcceleratorIterator it = iter.LevelBelowIterator();

   for(int i = 0; i < compositeNode->inputs.size; i++){
      if(compositeNode->inputs[i].node == nullptr){
         continue;
      }

      InstanceNode* inputNode = GetInputNode(accel->allocated,i);
      Assert(inputNode);
      FUInstance* input = inputNode->inst;
      int val = GetInputValue(compositeNode,i);
      for(int ii = 0; ii < input->declaration->outputLatencies.size; ii++){
         input->outputs[ii] = val;
         input->storedOutputs[ii] = val;
      }
   }

   AcceleratorRunIter(it);

   // Set output instance value to accelerator output
   InstanceNode* output = GetOutputNode(accel->allocated);
   if(output){
      for(int ii = 0; ii < output->inputs.size; ii++){
         if(output->inputs[ii].node == nullptr){
            continue;
         }

         int val = GetInputValue(output,ii);
         compositeInst->outputs[ii] = val;
         compositeInst->storedOutputs[ii] = val;
      }
   }
}

void AcceleratorRunIter(AcceleratorIterator iter){
   int buffer[99];
   Array<int> bufferArray = {buffer,99};

   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      FUInstance* inst = node->inst;
      if(IsTypeSimulatable(inst->declaration)){
         bufferArray.size = node->inputs.size;
         for(int i = 0; i < node->inputs.size; i++){
            PortNode& other = node->inputs[i];

            if(other.node){
               buffer[i] = other.node->inst->outputs[other.port];
            } else {
               buffer[i] = 0;
            }
         }

         int* newOutputs = inst->declaration->updateFunction(inst,bufferArray);

         if(inst->declaration->outputLatencies.size && inst->declaration->outputLatencies[0] == 0 && node->type != InstanceNode::TAG_SOURCE){
            memcpy(inst->outputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         } else {
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         }
      } else if(IsTypeHierarchical(inst->declaration)){
         AcceleratorRunComposite(iter);
      } else if(inst->declaration->type != FUDeclaration::SPECIAL){
         Assert(false && "Type is neither simulatable or hiearchical");
      }
   }
}

void AcceleratorRunOnce(Accelerator* accel){
   static int numberRuns = 0;
   int time = 0;

   Arena* arena = &accel->versat->temp;
   ArenaMarker marker(arena);

   // TODO: Eventually retire this portion. Make the Accelerator structure a map, maybe a std::unordered_map
   #if 1
   FUDeclaration base = {};
   base.name = STRING("Top");
   accel->subtype = &base;

   ReorganizeAccelerator(accel,arena);

   /* Hashmap<EdgeNode,int>* delay = */ CalculateDelay(accel->versat,accel,arena);
   SetDelayRecursive(accel,arena);

   if(!accel->ordered){
      ReorganizeAccelerator(accel,arena);
   }
   #endif

   FILE* accelOutputFile = nullptr;
   Array<int> vcdSameCheckSpace = {};
   if(accel->versat->debug.outputVCD){
      char buffer[128];
      sprintf(buffer,"debug/accelRun%d.vcd",numberRuns++);
      accelOutputFile = OpenFileAndCreateDirectories(buffer,"w");
      Assert(accelOutputFile);

      vcdSameCheckSpace = PrintVCDDefinitions(accelOutputFile,accel,arena);
   }
   defer{if(accelOutputFile) fclose(accelOutputFile);};

   Assert(accel->outputAlloc.size == accel->storedOutputAlloc.size);

   AcceleratorIterator iter = {};

   if(accel->versat->debug.outputVCD){
      iter.StartOrdered(accel,arena,true); // Probably overkill, but guarantees that print vcd gets the correct result
      AcceleratorEval(iter);
      PrintVCD(accelOutputFile,accel,time++,1,1,vcdSameCheckSpace,arena); // Print starting values
   }

   AcceleratorRunStart(accel);
   memcpy(accel->outputAlloc.ptr,accel->storedOutputAlloc.ptr,accel->outputAlloc.size * sizeof(int));

   if(accel->versat->debug.outputVCD){
      iter.StartOrdered(accel,arena,true);
      AcceleratorEval(iter);
      PrintVCD(accelOutputFile,accel,time++,0,1,vcdSameCheckSpace,arena);
   }

   for(int cycle = 0; cycle < 10000; cycle++){ // Max amount of iterations
      Assert(accel->outputAlloc.size == accel->storedOutputAlloc.size);

      iter.StartOrdered(accel,arena,true);
      AcceleratorRunIter(iter);

      if(accel->versat->debug.outputVCD){
         iter.StartOrdered(accel,arena,true);
         AcceleratorEval(iter);
         PrintVCD(accelOutputFile,accel,time++,1,0,vcdSameCheckSpace,arena);
         PrintVCD(accelOutputFile,accel,time++,0,0,vcdSameCheckSpace,arena);
      }

      memcpy(accel->outputAlloc.ptr,accel->storedOutputAlloc.ptr,accel->outputAlloc.size * sizeof(int));

      if(AcceleratorDone(accel)){
         break;
      }
   }

   // A couple of iterations to let units finish writing data to memories and stuff
   // "Not correct" but follows more closely with what we expected the accelerator to do in embedded
   for(int cycle = 0; cycle < 3; cycle++){
      #if 0
      if(accel->versat->debug.outputVCD){
         PrintVCD(accelOutputFile,accel,time++,0,0,vcdSameCheckSpace,arena);
      }
      #endif

      iter.StartOrdered(accel,arena,true);
      AcceleratorRunIter(iter);

      #if 0
      // For now, do not print vcd, because we are only repeating the last values and it's probably more confusing than helpful
      if(accel->versat->debug.outputVCD){
         PrintVCD(accelOutputFile,accel,time++,1,0,vcdSameCheckSpace,arena);
      }
      #endif
   }

   if(accel->versat->debug.outputVCD){
      PrintVCD(accelOutputFile,accel,time++,0,0,vcdSameCheckSpace,arena);
   }
}

void AcceleratorRun(Accelerator* accel,int times){
   for(int i = 0; i < times; i++){
      AcceleratorRunOnce(accel);
   }
}

extern "C" void AcceleratorRunC(Accelerator* accel,int times){
   for(int i = 0; i < times; i++){
      AcceleratorRunOnce(accel);
   }
}

