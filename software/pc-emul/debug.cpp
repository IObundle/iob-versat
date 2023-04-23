#include "debug.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

#include <algorithm>

#include "type.hpp"
#include "textualRepresentation.hpp"

bool debugFlag = false;
Arena debugArenaInst = {};
Arena* debugArena = &debugArenaInst;

void InitDebug(){
   static bool init = false;
   if(init){
      return;
   }
   init = true;

   debugArenaInst = InitArena(Megabyte(64));
}

void CheckMemory(Accelerator* topLevel){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   AcceleratorIterator iter = {};
   iter.Start(topLevel,temp,true);

   CheckMemory(iter);
}

void CheckMemory(Accelerator* topLevel,MemType type){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   AcceleratorIterator iter = {};
   iter.Start(topLevel,temp,true);

   CheckMemory(iter,type,temp);
}

static void CheckMemoryPrint(const char* level,const char* name,int pos,int delta,ComplexFUInstance* inst = nullptr){ // inst is for total outputs, if the unit is composite
   bool isComposite = (inst && inst->declaration->type == FUDeclaration::COMPOSITE);

   if(delta == 1 || (isComposite && delta == 0)){
      printf("%s%s:%d [%d",level,name,pos,delta);
   } else if(delta > 0 || isComposite){
      printf("%s%s:%d-%d [%d",level,name,pos,pos + delta - 1,delta);
   } else {
      return;
   }

   if(isComposite){
      printf("+%d]\n",inst->declaration->totalOutputs);
   } else {
      printf("]\n");
   }
}

#if 0
static void PrintRange(int pos,int delta,bool isComposite,FUDeclaration* decl){
   if(delta == 1){
      printf("%d [%d]",pos,delta);
   } else if(delta > 0 || isComposite){
      printf("%d-%d [%d]",pos,pos + delta - 1,delta);
   }
}
#endif

static void PrintLevel(int level){
   for(int i = 0; i < level; i++){
      printf("  ");
   }
}

struct MemoryRange{
   int start;
   int delta;
   FUDeclaration* decl;
   String name;
   int level;
   bool isComposite;
};

void CheckMemory(AcceleratorIterator iter,MemType type,Arena* arena){
   BLOCK_REGION(arena);

   Byte* mark = MarkArena(arena);

   Accelerator* topLevel = iter.topLevel;
   for(InstanceNode* node = iter.Current(); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;

      bool isComposite = (decl->type == FUDeclaration::COMPOSITE);

      MemoryRange* t = nullptr;

      switch(type){
      case MemType::CONFIG:{
         if(decl->nDelays){
            t->start = inst->delay - topLevel->delayAlloc.ptr;
            t->delta = decl->nDelays;
         }
      }break;
      case MemType::DELAY:{
         if(decl->nDelays){
            t->start = inst->delay - topLevel->delayAlloc.ptr;
            t->delta = decl->nDelays;
         }
      }break;
      case MemType::EXTRA:{
         if(decl->extraDataSize){
            t = PushStruct<MemoryRange>(arena);
            t->start = inst->extraData - topLevel->extraDataAlloc.ptr;
            t->delta = decl->extraDataSize;
         }
      }break;
      case MemType::OUTPUT:{
      }break;
      case MemType::STATE:{
      }break;
      case MemType::STATIC:{
      }break;
      case MemType::STORED_OUTPUT:{
      }break;
      }

      if(t){
         t->decl = decl;
         t->level = iter.level;
         t->name = inst->name;
         t->isComposite = isComposite;
      }
   }

   Array<MemoryRange> array = PointArray<MemoryRange>(arena,mark);

   qsort(array.data,array.size,sizeof(MemoryRange),[](const void* v1,const void* v2){
            MemoryRange* m1 = (MemoryRange*) v1;
            MemoryRange* m2 = (MemoryRange*) v2;
            return m1->start - m2->start;
         });

   mark = MarkArena(arena);

   for(MemoryRange& r : array){
      Range* range = PushStruct<Range>(arena);

      range->start = r.start;

      if(!r.isComposite){
         PrintLevel(r.level);
         printf("[%.*s] %.*s : %d-%d [%d]\n",UNPACK_SS(r.decl->name),UNPACK_SS(r.name),r.start,r.start + r.delta,r.delta);
         range->end = r.start + r.delta;
      } else {
         range->end = r.start;
      }
   }

   Array<Range> ranges = PointArray<Range>(arena,mark);

   SortRanges(ranges);

   Assert(CheckNoGaps(ranges).result);
   Assert(CheckNoOverlap(ranges).result);
}

void SortRanges(Array<Range> ranges){
   qsort(ranges.data,ranges.size,sizeof(Range),[](const void* v1,const void* v2){
            Range* r1 = (Range*) v1;
            Range* r2 = (Range*) v2;
            return r1->start - r2->start;
         });
}

static bool IsSorted(Array<Range> ranges){
   for(int i = 0; i < ranges.size - 1; i++){
      if(ranges[i + 1].start < ranges[i].start){
         DEBUG_BREAK_IF_DEBUGGING();
         return false;
      }
   }
   return true;
}

CheckRangeResult CheckNoOverlap(Array<Range> ranges){
   CheckRangeResult res = {};

   Assert(IsSorted(ranges));

   for(int i = 0; i < ranges.size - 1; i++){
      Range cur = ranges[i];
      Range next = ranges[i+1];

      if(cur.end > next.start){
         res.result = false;
         res.problemIndex = i;

         return res;
      }
   }

   res.result = true;
   return res;
}

CheckRangeResult CheckNoGaps(Array<Range> ranges){
   CheckRangeResult res = {};

   Assert(IsSorted(ranges));

   for(int i = 0; i < ranges.size - 1; i++){
      Range cur = ranges[i];
      Range next = ranges[i+1];

      if(cur.end != next.start){
         res.result = false;
         res.problemIndex = i;

         return res;
      }
   }

   res.result = true;
   return res;
}

void CheckMemory(AcceleratorIterator iter){
   char levelBuffer[] = "| | | | | | | | | | | | ";

   Accelerator* topLevel = iter.topLevel;
   for(InstanceNode* node = iter.Current(); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      levelBuffer[iter.level * 2] = '\0';

      printf("%s\n",levelBuffer);

      printf("%s[%.*s] %.*s:\n",levelBuffer,UNPACK_SS(inst->declaration->name),UNPACK_SS(inst->name));

      if(IsConfigStatic(topLevel,inst)){
         CheckMemoryPrint(levelBuffer,"c",inst->config - topLevel->staticAlloc.ptr,decl->configs.size);
      } else {
         CheckMemoryPrint(levelBuffer,"C",inst->config - topLevel->configAlloc.ptr,decl->configs.size);
      }

      UnitValues val = CalculateIndividualUnitValues(inst);

      CheckMemoryPrint(levelBuffer,"S",inst->state - topLevel->stateAlloc.ptr,decl->states.size);
      CheckMemoryPrint(levelBuffer,"D",inst->delay - topLevel->delayAlloc.ptr,decl->nDelays);
      CheckMemoryPrint(levelBuffer,"E",inst->extraData - topLevel->extraDataAlloc.ptr,decl->extraDataSize);
      CheckMemoryPrint(levelBuffer,"O",inst->outputs - topLevel->outputAlloc.ptr,val.outputs,inst);
      CheckMemoryPrint(levelBuffer,"o",inst->storedOutputs - topLevel->storedOutputAlloc.ptr,val.outputs,inst);

      levelBuffer[iter.level * 2] = '|';
   }
}

void DisplayInstanceMemory(ComplexFUInstance* inst){
   printf("%.*s\n",UNPACK_SS(inst->name));
   printf("  C: %p\n",inst->config);
   printf("  S: %p\n",inst->state);
   printf("  D: %p\n",inst->delay);
   printf("  O: %p\n",inst->outputs);
   printf("  o: %p\n",inst->storedOutputs);
   printf("  E: %p\n",inst->extraData);
}

static void OutputSimpleIntegers(int* mem,int size){
   for(int i = 0; i < size; i++){
      printf("%d ",mem[i]);
   }
}

void DisplayAcceleratorMemory(Accelerator* topLevel){
   printf("Config:\n");
   OutputSimpleIntegers(topLevel->configAlloc.ptr,topLevel->configAlloc.size);

   printf("\nStatic:\n");
   OutputSimpleIntegers(topLevel->staticAlloc.ptr,topLevel->staticAlloc.size);

   printf("\nDelay:\n");
   OutputSimpleIntegers(topLevel->delayAlloc.ptr,topLevel->delayAlloc.size);
   printf("\n");
}

void DisplayUnitConfiguration(Accelerator* topLevel){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   AcceleratorIterator iter = {};
   iter.Start(topLevel,temp,true);
   DisplayUnitConfiguration(iter);
}

void DisplayUnitConfiguration(AcceleratorIterator iter){
   Accelerator* accel = iter.topLevel;
   Assert(accel);

   for(InstanceNode* node = iter.Current(); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      UnitValues val = CalculateIndividualUnitValues(inst);

      FUDeclaration* type = inst->declaration;

      if(val.configs | val.states | val.delays){
         printf("[%.*s] %.*s:",UNPACK_SS(type->name),UNPACK_SS(inst->name));
         if(val.configs){
            if(IsConfigStatic(accel,inst)){
               printf("\nStatic [%ld]: ",inst->config - accel->staticAlloc.ptr);
            } else {
               printf("\nConfig [%ld]: ",inst->config - accel->configAlloc.ptr);
            }

            OutputSimpleIntegers(inst->config,val.configs);
         }
         if(val.states){
            printf("\nState [%ld]: ",inst->state - accel->stateAlloc.ptr);
            OutputSimpleIntegers(inst->state,val.states);
         }
         if(val.delays){
            printf("\nDelay [%ld]: ",inst->delay - accel->delayAlloc.ptr);
            OutputSimpleIntegers(inst->delay,val.delays);
         }
         printf("\n\n");
      }
   }
}

bool CheckInputAndOutputNumber(FUDeclaration* type,int inputs,int outputs){
   if(inputs > type->inputDelays.size){
      return false;
   } else if(outputs > type->outputLatencies.size){
      return false;
   }

   return true;
}

void PrintAcceleratorInstances(Accelerator* accel){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,temp,false); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      for(int ii = 0; ii < iter.level; ii++){
         printf("  ");
      }
      printf("%.*s\n",UNPACK_SS(inst->name));
   }
}

static void OutputGraphDotFile_(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<ComplexFUInstance*>* highlighInstance,FILE* outputFile){
   Arena* arena = &versat->temp;
   BLOCK_REGION(arena);

   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      String id = UniqueRepr(inst,arena);
      String name = Repr(inst,versat->debug.dotFormat,arena);

      String color = STRING("darkblue");

      if(ptr->type == InstanceNode::TAG_SOURCE || ptr->type == InstanceNode::TAG_SOURCE_AND_SINK){
         color = STRING("darkgreen");
      } else if(ptr->type == InstanceNode::TAG_SINK){
         color = STRING("dark");
      }

      bool doHighligh = (highlighInstance ? highlighInstance->Exists(inst) : false);

      if(doHighligh){
         fprintf(outputFile,"\t\"%.*s\" [color=darkred,label=\"%.*s\"];\n",UNPACK_SS(id),UNPACK_SS(name));
      } else {
         fprintf(outputFile,"\t\"%.*s\" [color=%.*s label=\"%.*s\"];\n",UNPACK_SS(id),UNPACK_SS(color),UNPACK_SS(name));
      }
   }

   int size = Size(accel->edges);
   Hashmap<Pair<InstanceNode*,InstanceNode*>,int>* seen = PushHashmap<Pair<InstanceNode*,InstanceNode*>,int>(arena,size);

   // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* out = ptr->inst;

      FOREACH_LIST(con,ptr->allOutputs){
         ComplexFUInstance* in = con->instConnectedTo.node->inst;

         if(collapseSameEdges){
            Pair<InstanceNode*,InstanceNode*> nodeEdge = {};
            nodeEdge.first = ptr;
            nodeEdge.second = con->instConnectedTo.node;

            GetOrAllocateResult res = seen->GetOrAllocate(nodeEdge);
            if(res.alreadyExisted){
               continue;
            }
         }

         int inPort = con->instConnectedTo.port;
         int outPort = con->port;

         String first = UniqueRepr(out,arena);
         String second = UniqueRepr(in,arena);
         PortInstance start = {out,outPort};
         PortInstance end = {in,inPort};
         String label = Repr(start,end,versat->debug.dotFormat,arena);
         int calculatedDelay = con->delay ? *con->delay : 0;

         bool highlighStart = (highlighInstance ? highlighInstance->Exists(start.inst) : false);
         bool highlighEnd = (highlighInstance ? highlighInstance->Exists(end.inst) : false);

         bool highlight = highlighStart && highlighEnd;

         fprintf(outputFile,"\t\"%.*s\" -> ",UNPACK_SS(first));
         fprintf(outputFile,"\"%.*s\"",UNPACK_SS(second));

         if(highlight){
            fprintf(outputFile,"[color=darkred,label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),con->edgeDelay,calculatedDelay);
         } else {
            fprintf(outputFile,"[label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),con->edgeDelay,calculatedDelay);
         }
      }
   }
   fprintf(outputFile,"}\n");
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...){
   char buffer[1024];

   if(!versat->debug.outputGraphs){
      return;
   }

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(versat,accel,collapseSameEdges,nullptr,file);
   fclose(file);

   va_end(args);
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...){
   char buffer[1024];

   if(!versat->debug.outputGraphs){
      return;
   }

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   Arena* temp = &versat->temp;
   BLOCK_REGION(temp);

   Set<ComplexFUInstance*>* highlight = PushSet<ComplexFUInstance*>(temp,1);
   highlight->Insert(highlighInstance);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(versat,accel,collapseSameEdges,highlight,file);
   fclose(file);

   va_end(args);
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<ComplexFUInstance*>* highlight,const char* filenameFormat,...){
   char buffer[1024];

   if(!versat->debug.outputGraphs){
      return;
   }

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(versat,accel,collapseSameEdges,highlight,file);
   fclose(file);

   va_end(args);
}

String PushMemoryHex(Arena* arena,void* memory,int size){
   Byte* mark = MarkArena(arena);

   unsigned char* view = (unsigned char*) memory;

   for(int i = 0; i < size; i++){
      int low = view[i] % 16;
      int high = view[i] / 16;

      PushString(arena,"%c%c ",GetHex(high),GetHex(low));
   }

   return PointArena(arena,mark);
}

void OutputMemoryHex(void* memory,int size){
   unsigned char* view = (unsigned char*) memory;

   for(int i = 0; i < size; i++){
      if(i % 16 == 0 && i != 0){
         printf("\n");
      }

      int low = view[i] % 16;
      int high = view[i] / 16;

      printf("%c%c ",GetHex(high),GetHex(low));

   }

   printf("\n");
}

static void PrintVCDDefinitions_(FILE* accelOutputFile,VCDMapping& currentMapping,Accelerator* accel){
   #if 0
   for(StaticInfo* info : accel->staticInfo){
      for(Wire& wire : info->configs){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s_%.*s $end\n",wire.bitsize,currentMapping.Get(),UNPACK_SS(info->id.name),UNPACK_SS(wire.name));
         currentMapping.Increment();
      }
   }
   #endif

   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      fprintf(accelOutputFile,"$scope module %.*s_%d $end\n",UNPACK_SS(inst->name),inst->id);

      for(int i = 0; i < ptr->inputs.size; i++){
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_in%d $end\n",currentMapping.Get(),UNPACK_SS(inst->name),i);
         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->outputLatencies.size; i++){
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_out%d $end\n",currentMapping.Get(),UNPACK_SS(inst->name),i);
         currentMapping.Increment();
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_stored_out%d $end\n",currentMapping.Get(),UNPACK_SS(inst->name),i);
         currentMapping.Increment();
      }

      for(Wire& wire : inst->declaration->configs){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s $end\n",wire.bitsize,currentMapping.Get(),UNPACK_SS(wire.name));
         currentMapping.Increment();
      }

      for(Wire& wire : inst->declaration->states){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s $end\n",wire.bitsize,currentMapping.Get(),UNPACK_SS(wire.name));
         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"$var wire 32 %s delay%d $end\n",currentMapping.Get(),i);
         currentMapping.Increment();
      }

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"$var wire  1 %s done $end\n",currentMapping.Get());
         currentMapping.Increment();
      }

      if(inst->declaration->printVCD){
         inst->declaration->printVCD(inst,accelOutputFile,currentMapping,{},false,true);
      }

      if(inst->declaration->fixedDelayCircuit){
         PrintVCDDefinitions_(accelOutputFile,currentMapping,inst->declaration->fixedDelayCircuit);
      }

      fprintf(accelOutputFile,"$upscope $end\n");
   }
}

Array<int> PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel,Arena* tempSave){
   VCDMapping mapping;

   fprintf(accelOutputFile,"$timescale   1ns $end\n");
   fprintf(accelOutputFile,"$scope module TOP $end\n");
   fprintf(accelOutputFile,"$var wire  1 a clk $end\n");
   fprintf(accelOutputFile,"$var wire  32 b counter $end\n");
   fprintf(accelOutputFile,"$var wire  1 c run $end\n");
   PrintVCDDefinitions_(accelOutputFile,mapping,accel);
   fprintf(accelOutputFile,"$upscope $end\n");
   fprintf(accelOutputFile,"$enddefinitions $end\n");

   Array<int> array = PushArray<int>(tempSave,mapping.increments);
   return array;
}

static char* Bin(unsigned int value){
   static char buffer[33];
   buffer[32] = '\0';

   unsigned int val = value;
   for(int i = 31; i >= 0; i--){
      buffer[i] = '0' + (val & 0x1);
      val >>= 1;
   }

   return buffer;
}

static void PrintVCD_(FILE* accelOutputFile,VCDMapping& currentMapping,AcceleratorIterator iter,bool firstTime,Array<int> sameValueCheckSpace){
   #if 0
   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->configs.size; i++){
         if(firstTime){
            fprintf(accelOutputFile,"b%s %s\n",Bin(info->ptr[i]),currentMapping.Get());
         }
         currentMapping.Increment();
      }
   }
   #endif

   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      ComplexFUInstance* inst = node->inst;
      for(int i = 0; i < node->inputs.size; i++){
         if(node->inputs[i].node == nullptr){
            if(firstTime){
               fprintf(accelOutputFile,"bx %s\n",currentMapping.Get());
            }
         } else {
            int value = GetInputValue(inst,i);

            if(firstTime || (value != sameValueCheckSpace[currentMapping.increments])){
               fprintf(accelOutputFile,"b%s %s\n",Bin(value),currentMapping.Get());
               sameValueCheckSpace[currentMapping.increments] = value;
            }
         }

         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->outputLatencies.size; i++){
         if(firstTime || (inst->outputs[i] != sameValueCheckSpace[currentMapping.increments])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->outputs[i]),currentMapping.Get());
            sameValueCheckSpace[currentMapping.increments] = inst->outputs[i];
         }
         currentMapping.Increment();
         if(firstTime || (inst->storedOutputs[i] != sameValueCheckSpace[currentMapping.increments])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->storedOutputs[i]),currentMapping.Get());
            sameValueCheckSpace[currentMapping.increments] = inst->storedOutputs[i];
         }
         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->configs.size; i++){
         if(firstTime){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->config[i]),currentMapping.Get());
         }
         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->states.size; i++){
         if(firstTime || (inst->state[i] != sameValueCheckSpace[currentMapping.increments])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->state[i]),currentMapping.Get());
            sameValueCheckSpace[currentMapping.increments] = inst->state[i];
         }
         currentMapping.Increment();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         if(firstTime || (inst->delay[i] != sameValueCheckSpace[currentMapping.increments])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->delay[i]),currentMapping.Get());
            sameValueCheckSpace[currentMapping.increments] = inst->delay[i];
         }

         currentMapping.Increment();
      }

      if(inst->declaration->implementsDone){
         if(firstTime || (inst->done != sameValueCheckSpace[currentMapping.increments])){
            fprintf(accelOutputFile,"%d%s\n",inst->done ? 1 : 0,currentMapping.Get());
            sameValueCheckSpace[currentMapping.increments] = inst->done;
         }
         currentMapping.Increment();
      }

      if(inst->declaration->printVCD){
         inst->declaration->printVCD(inst,accelOutputFile,currentMapping,sameValueCheckSpace,firstTime,false);
      }

      if(inst->declaration->fixedDelayCircuit){
         AcceleratorIterator it = iter.LevelBelowIterator();

         PrintVCD_(accelOutputFile,currentMapping,it,firstTime,sameValueCheckSpace);
      }
   }
}

void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock,int run,Array<int> sameValueCheckSpace,Arena* arena){ // Need to put some clock signal
   BLOCK_REGION(arena);

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock ? 1 : 0);

   int counter = time - 1;

   if(counter < 0){
      fprintf(accelOutputFile,"bx b\n");
   } else {
      fprintf(accelOutputFile,"b%s b\n",Bin(counter / 2));
   }

   fprintf(accelOutputFile,"%dc\n",run ? 1 : 0);

   AcceleratorIterator iter = {};
   iter.Start(accel,arena,true);
   VCDMapping mapping;

   PrintVCD_(accelOutputFile,mapping,iter,time == 0,sameValueCheckSpace);
}

#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>

void SetDebugSignalHandler(SignalHandler func){
   signal(SIGUSR1, func);
   signal(SIGSEGV, func);
   signal(SIGABRT, func);
}

