#include "debug.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

#include <algorithm>

#include "type.hpp"
#include "textualRepresentation.hpp"

void CheckMemory(Accelerator* topLevel){
   Arena* temp = &topLevel->temp;
   ArenaMarker marker(temp);

   AcceleratorIterator iter = {};
   iter.Start(topLevel,temp,true);

   CheckMemory(iter);
}

void CheckMemory(Accelerator* topLevel,MemType type){
   Arena* temp = &topLevel->temp;
   ArenaMarker marker(temp);

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

static void PrintRange(int pos,int delta,bool isComposite,FUDeclaration* decl){
   if(delta == 1){
      printf("%d [%d]",pos,delta);
   } else if(delta > 0 || isComposite){
      printf("%d-%d [%d]",pos,pos + delta - 1,delta);
   }
}

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
   ARENA_MARKER(arena);

   Byte* mark = MarkArena(arena);

   Accelerator* topLevel = iter.topLevel;
   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Next()){
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
   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Next()){
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
   Arena* temp = &topLevel->temp;
   ArenaMarker marker(temp);

   AcceleratorIterator iter = {};
   iter.Start(topLevel,temp,true);
   DisplayUnitConfiguration(iter);
}

void DisplayUnitConfiguration(AcceleratorIterator iter){
   Accelerator* accel = iter.topLevel;
   Assert(accel);

   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Next()){
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

void EnterDebugTerminal(Versat* versat){
   DebugTerminal(MakeValue(versat,"Versat"));
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
   Arena* temp = &accel->temp;
   ArenaMarker marker(temp);

   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel,temp,false); inst; inst = iter.Next()){
      for(int ii = 0; ii < iter.level; ii++){
         printf("  ");
      }
      printf("%.*s\n",UNPACK_SS(inst->name));
   }
}

bool IsGraphValid(AcceleratorView view){
   Assert(view.graphData.size);

   InstanceMap map;

   for(ComplexFUInstance** instPtr : view.nodes){
      ComplexFUInstance* inst = *instPtr;

      inst->tag = 0;
      map.insert({inst,inst});
   }

   for(EdgeView* edgeView : view.edges){
      Edge* edge = edgeView->edge;

      for(int i = 0; i < 2; i++){
         auto res = map.find(edge->units[i].inst);

         Assert(res != map.end());

         res->first->tag = 1;
      }

      Assert(edge->units[0].port < edge->units[0].inst->declaration->outputLatencies.size && edge->units[0].port >= 0);
      Assert(edge->units[1].port < edge->units[1].inst->declaration->inputDelays.size && edge->units[1].port >= 0);
   }

   if(view.edges.Size()){
      for(ComplexFUInstance** instPtr : view.nodes){
         ComplexFUInstance* inst = *instPtr;

         Assert(inst->tag == 1 || inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED);
      }
   }

   return true;
}

void OutputGraphDotFile_(SimpleGraph graph,bool collapseSameEdges,FILE* file){
   fprintf(file,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");

   for(int i = 0; i < graph.nodes.size; i++){
      fprintf(file,"%d [label=\"%.*s\"]\n",i,UNPACK_SS(graph.nodes[i].decl->name));
   }
   for(int i = 0; i < graph.edges.size; i++){
      SimpleEdge e = graph.edges[i];
      fprintf(file,"%d:%d -> %d:%d\n",e.out,e.outPort,e.in,e.inPort);
   }
   fprintf(file,"}\n");
}

static void OutputGraphDotFile_(Versat* versat,AcceleratorView& view,bool collapseSameEdges,ComplexFUInstance* highlighInstance,FILE* outputFile){
   Arena* arena = &versat->temp;

   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(ComplexFUInstance** instPtr : view.nodes){
      ComplexFUInstance* inst = *instPtr;
      String id = UniqueRepr(inst,arena);
      String name = Repr(inst,versat->debug.dotFormat,arena);

      if(inst == highlighInstance){
         fprintf(outputFile,"\t\"%.*s\" [color=blue,label=\"%.*s\"];\n",UNPACK_SS(id),UNPACK_SS(name));
      } else {
         fprintf(outputFile,"\t\"%.*s\" [label=\"%.*s\"];\n",UNPACK_SS(id),UNPACK_SS(name));
      }
   }

   std::set<std::pair<ComplexFUInstance*,ComplexFUInstance*>> sameEdgeCounter;

   // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
   for(EdgeView* edgeView : view.edges){
      Edge* edge = edgeView->edge;
      if(collapseSameEdges){
         std::pair<ComplexFUInstance*,ComplexFUInstance*> key{edge->units[0].inst,edge->units[1].inst};

         if(sameEdgeCounter.count(key) == 1){
            continue;
         }

         sameEdgeCounter.insert(key);
      }

      String first = UniqueRepr(edge->units[0].inst,arena);
      String second = UniqueRepr(edge->units[1].inst,arena);
      PortInstance start = edge->units[0];
      PortInstance end = edge->units[1];
      String label = Repr(start,end,versat->debug.dotFormat,arena);
      int calculatedDelay = edgeView->delay;

      bool highlight = (start.inst == highlighInstance || end.inst == highlighInstance);

      fprintf(outputFile,"\t\"%.*s\" -> ",UNPACK_SS(first));
      fprintf(outputFile,"\"%.*s\"",UNPACK_SS(second));

      if(highlight){
         fprintf(outputFile,"[color=blue,label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),edge->delay,calculatedDelay);
      } else {
         fprintf(outputFile,"[label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),edge->delay,calculatedDelay);
      }
   }

   fprintf(outputFile,"}\n");
}

void OutputGraphDotFile(Versat* versat,AcceleratorView& view,bool collapseSameEdges,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(versat,view,collapseSameEdges,nullptr,file);
   fclose(file);

   va_end(args);
}

void OutputGraphDotFile(SimpleGraph graph,bool collapseSameEdges,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(graph,collapseSameEdges,file);
   fclose(file);

   va_end(args);
}

void OutputGraphDotFile(Versat* versat,AcceleratorView& view,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = OpenFileAndCreateDirectories(buffer,"w");
   OutputGraphDotFile_(versat,view,collapseSameEdges,highlighInstance,file);
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

static const int VCD_MAPPING_SIZE = 5;
static std::array<char,VCD_MAPPING_SIZE+1> currentMapping = {'a','a','a','a','a','\0'};
static int mappingIncrements = 0;
static void ResetMapping(){
   for(int i = 0; i < VCD_MAPPING_SIZE; i++){
      currentMapping[i] = 'a';
   }
   mappingIncrements = 0;
}

static void IncrementMapping(){
   for(int i = VCD_MAPPING_SIZE-1; i >= 0; i--){
      mappingIncrements += 1;
      currentMapping[i] += 1;
      if(currentMapping[i] == 'z' + 1){
         currentMapping[i] = 'a';
      } else {
         return;
      }
   }
   Assert(false && "Increase mapping space");
}

static void PrintVCDDefinitions_(FILE* accelOutputFile,Accelerator* accel){
   #if 0
   for(StaticInfo* info : accel->staticInfo){
      for(Wire& wire : info->configs){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s_%.*s $end\n",wire.bitsize,currentMapping.data(),UNPACK_SS(info->id.name),UNPACK_SS(wire.name));
         IncrementMapping();
      }
   }
   #endif

   FOREACH_LIST(inst,accel->instances){
      fprintf(accelOutputFile,"$scope module %.*s_%d $end\n",UNPACK_SS(inst->name),inst->id);

      for(int i = 0; i < inst->graphData->singleInputs.size; i++){
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_in%d $end\n",currentMapping.data(),UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->graphData->outputs; i++){
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_out%d $end\n",currentMapping.data(),UNPACK_SS(inst->name),i);
         IncrementMapping();
         fprintf(accelOutputFile,"$var wire  32 %s %.*s_stored_out%d $end\n",currentMapping.data(),UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(Wire& wire : inst->declaration->configs){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s $end\n",wire.bitsize,currentMapping.data(),UNPACK_SS(wire.name));
         IncrementMapping();
      }

      for(Wire& wire : inst->declaration->states){
         fprintf(accelOutputFile,"$var wire  %d %s %.*s $end\n",wire.bitsize,currentMapping.data(),UNPACK_SS(wire.name));
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"$var wire 32 %s delay%d $end\n",currentMapping.data(),i);
         IncrementMapping();
      }

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"$var wire  1 %s done $end\n",currentMapping.data());
         IncrementMapping();
      }

      if(inst->declaration->fixedDelayCircuit){
         PrintVCDDefinitions_(accelOutputFile,inst->declaration->fixedDelayCircuit);
      }

      fprintf(accelOutputFile,"$upscope $end\n");
   }
}

Array<int> PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel,Arena* tempSave){
   ResetMapping();

   fprintf(accelOutputFile,"$timescale   1ns $end\n");
   fprintf(accelOutputFile,"$scope module TOP $end\n");
   fprintf(accelOutputFile,"$var wire  1 a clk $end\n");
   fprintf(accelOutputFile,"$var wire  32 b counter $end\n");
   PrintVCDDefinitions_(accelOutputFile,accel);
   fprintf(accelOutputFile,"$upscope $end\n");
   fprintf(accelOutputFile,"$enddefinitions $end\n");

   Array<int> array = PushArray<int>(tempSave,mappingIncrements);
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

static void PrintVCD_(FILE* accelOutputFile,AcceleratorIterator iter,int time,Array<int> sameValueCheckSpace){
   #if 0
   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->configs.size; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %s\n",Bin(info->ptr[i]),currentMapping.data());
         }
         IncrementMapping();
      }
   }
   #endif

   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Skip()){
      for(int i = 0; i < inst->graphData->singleInputs.size; i++){
         if(inst->graphData->singleInputs[i].inst == nullptr){
            if(time == 0){
               fprintf(accelOutputFile,"bx %s\n",currentMapping.data());
            }
         } else {
            int value = GetInputValue(inst,i);

            if(time == 0 || (value != sameValueCheckSpace[mappingIncrements])){
               fprintf(accelOutputFile,"b%s %s\n",Bin(value),currentMapping.data());
               sameValueCheckSpace[mappingIncrements] = value;
            }
         }

         IncrementMapping();
      }

      for(int i = 0; i < inst->graphData->outputs; i++){
         if(time == 0 || (inst->outputs[i] != sameValueCheckSpace[mappingIncrements])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->outputs[i]),currentMapping.data());
            sameValueCheckSpace[mappingIncrements] = inst->outputs[i];
         }
         IncrementMapping();
         if(time == 0 || (inst->storedOutputs[i] != sameValueCheckSpace[mappingIncrements])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->storedOutputs[i]),currentMapping.data());
            sameValueCheckSpace[mappingIncrements] = inst->storedOutputs[i];
         }
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->configs.size; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->config[i]),currentMapping.data());
         }
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->states.size; i++){
         if(time == 0 || (inst->state[i] != sameValueCheckSpace[mappingIncrements])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->state[i]),currentMapping.data());
            sameValueCheckSpace[mappingIncrements] = inst->state[i];
         }
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         if(time == 0 || (inst->delay[i] != sameValueCheckSpace[mappingIncrements])){
            fprintf(accelOutputFile,"b%s %s\n",Bin(inst->delay[i]),currentMapping.data());
            sameValueCheckSpace[mappingIncrements] = inst->delay[i];
         }

         IncrementMapping();
      }

      if(inst->declaration->implementsDone){
         if(time == 0 || (inst->done != sameValueCheckSpace[mappingIncrements])){
            fprintf(accelOutputFile,"%d%s\n",inst->done ? 1 : 0,currentMapping.data());
            sameValueCheckSpace[mappingIncrements] = inst->done;
         }
         IncrementMapping();
      }

      if(inst->declaration->fixedDelayCircuit){
         AcceleratorIterator it = iter.LevelBelowIterator();

         PrintVCD_(accelOutputFile,it,time,sameValueCheckSpace);
      }
   }
}

void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock,Array<int> sameValueCheckSpace){ // Need to put some clock signal
   Arena* temp = &accel->temp;
   ArenaMarker marker(temp);
   ResetMapping();

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock ? 1 : 0);
   fprintf(accelOutputFile,"b%s b\n",Bin((time + 1) / 2));

   AcceleratorIterator iter = {};
   iter.Start(accel,temp,true);
   PrintVCD_(accelOutputFile,iter,time,sameValueCheckSpace);
}

void SetDebugSignalHandler(SignalHandler func){
   signal(SIGUSR1, func);
   signal(SIGSEGV, func);
   signal(SIGABRT, func);
}

#if 0
#define NCURSES
#endif

#ifdef NCURSES
#include <ncurses.h>
#include <signal.h>

struct PanelState{
   String name;
   Value valueLooking;
   int cursorPosition;
   Byte* arenaMark;
};

#define MAX_FRAMES 99
#define MAX_PANELS 10

static PanelState globalPanels[MAX_PANELS][MAX_FRAMES] = {};
static int globalCurrentPanels[MAX_PANELS] = {};
static int globalCurrentPanel;

template<typename T>
int ListCount(T* listHead){
   int count = 0;

   for(T* ptr = listHead; ptr != nullptr; ptr = ptr->next){
      count += 1;
   }

   return count;
}

template<typename T>
T* GetListByIndex(T* listHead,int index){
   T* ptr = listHead;

   while(index > 0){
      ptr = ptr->next;
      index -= 1;
   }

   return ptr;
}

// Only allow one input
enum Input{
   INPUT_NONE,
   INPUT_BACKSPACE,
   INPUT_ENTER,
   INPUT_UP,
   INPUT_DOWN,
   INPUT_LEFT,
   INPUT_RIGHT
};

static bool WatchableObject(Value object){
   Value val = CollapsePtrIntoStruct(object);

   Type* type = val.type;

   if(IsIndexable(type)){
      return true;
   } else if(type->type == Type::STRUCT){
      return true;
   }

   return false;
}

void StructPanel(WINDOW* w,Value val,int cursorPosition,int xStart,bool displayOnly,Arena* arena){
   Type* type = val.type;

   Assert(type->type == Type::STRUCT);

   const int panelWidth = 32;

   int menuIndex = 0;
   int startPos = 2;

   if(IsEmbeddedListKind(type)){
      mvaddnstr(1,0,"List kind",9);
   }

   for(Member& member : type->members){
      int yPos = startPos + menuIndex;

      if(!displayOnly && menuIndex == cursorPosition){
         wattron( w, A_STANDOUT );
      }

      mvaddnstr(yPos, xStart, member.name.data,member.name.size);

      Value memberVal = AccessStruct(val,&member);
      String repr = GetValueRepresentation(memberVal,arena);

      move(yPos, xStart + panelWidth);
      addnstr(repr.data,repr.size);

      move(yPos, xStart + panelWidth + panelWidth / 2);
      addnstr(member.type->name.data,member.type->name.size);

      if(!displayOnly && menuIndex == cursorPosition){
         wattroff( w, A_STANDOUT );
      }

      menuIndex += 1;
   }
}

void TerminalIteration(int panelIndex,WINDOW* w,Input input,Arena* arena,Arena* temp){
   PanelState* panel = globalPanels[panelIndex];
   int& currentPanel = globalCurrentPanels[panelIndex];

   ArenaMarker mark(temp);
   clear();
   // Before panel decision
   {
   PanelState* currentState = &panel[currentPanel];
   Value val = currentState->valueLooking;
   Type* type = val.type;

   if(input == INPUT_BACKSPACE || input == INPUT_LEFT){
      if(currentPanel > 0){
         PopMark(arena,currentState->arenaMark);

         currentPanel -= 1;
      }
   } else if(input == INPUT_ENTER){
      // Assume new state will be created and fill with default parameters
      PanelState* newState = &panel[currentPanel + 1];
      newState->cursorPosition = 0;
      newState->arenaMark = MarkArena(arena);

      if(type->type == Type::STRUCT){
         Member* member = &type->members[currentState->cursorPosition];

         Value selectedValue = AccessStruct(val,currentState->cursorPosition);

         selectedValue = CollapsePtrIntoStruct(selectedValue);

         if(selectedValue.type->type == Type::STRUCT || IsIndexable(selectedValue.type)){
            currentPanel += 1;

            newState->name = member->name;
            newState->valueLooking = selectedValue;
         }
      } else if(IsIndexable(type)){
         if(!IsIndexableOfBasicType(type)){
            Iterator iter = Iterate(val);
            for(int i = 0; HasNext(iter); i += 1,Advance(&iter)){
               if(i == currentState->cursorPosition){
                  Value val = GetValue(iter);

                  currentPanel += 1;

                  newState->name = PushString(arena,"%d\n",i);
                  newState->valueLooking = val;
                  break;
               }
            }
         }
      }
   } else if(input == INPUT_RIGHT){
      if(type->type == Type::STRUCT && IsEmbeddedListKind(type)){
         PanelState* newState = &panel[currentPanel + 1];
         newState->cursorPosition = 0;
         newState->arenaMark = MarkArena(arena);

         Value selectedValue = AccessStruct(val,STRING("next"));

         selectedValue = CollapsePtrIntoStruct(selectedValue);

         if(selectedValue.type->type == Type::STRUCT || IsIndexable(selectedValue.type)){
            currentPanel += 1;

            newState->name = STRING("next");
            newState->valueLooking = selectedValue;
         }
      }
   }
   }

   currentPanel = std::max(0,currentPanel);

   // At this point, panel is locked in
   PanelState* state = &panel[currentPanel];

   Value val = state->valueLooking;
   Type* type = val.type;

   String panelNumber = PushString(temp,"%d",currentPanel);
   mvaddnstr(0,0, panelNumber.data,panelNumber.size);
   mvaddnstr(0,3, val.type->name.data, val.type->name.size);

   if(state->name.size){
      addnstr(":",1);
      addnstr(state->name.data,state->name.size);
   }

   int numberElements = 0;
   if(type->type == Type::STRUCT){
      numberElements = type->members.size;
   } else if(IsIndexable(type)){
      numberElements = IndexableSize(val);
   }

   if(input == INPUT_DOWN){
      state->cursorPosition += 1;
   } else if(input == INPUT_UP){
      state->cursorPosition -= 1;
   }

   state->cursorPosition = RolloverRange(0,state->cursorPosition,numberElements - 1);

   if(type->type == Type::STRUCT){
      StructPanel(w,state->valueLooking,state->cursorPosition,0,false,arena);
   } else if(IsIndexable(type)){
      if(IsIndexableOfBasicType(type)){
         Iterator iter = Iterate(val);

         int maxSize = 0;
         for(; HasNext(iter); Advance(&iter)){
            String repr = GetValueRepresentation(GetValue(iter),temp);
            maxSize = std::max(maxSize,repr.size);
         }

         int maxElementsPerLine = 10; // For now, hardcode it
         int spacing = 2;

         for(int i = 0; i < 10; i++){
            String integer = PushString(temp,"%d\n",i);

            mvaddnstr(1,(maxSize - 1) + 2+i*(maxSize+spacing),integer.data,integer.size);
         }

         iter = Iterate(val);
         int y = 2;
         int x = 2;
         for(int index = 0; HasNext(iter); index += 1,Advance(&iter)){
            if(index % 10 == 0 && index != 0){
               y += 1;
               x = 2;
            }

            String repr = GetValueRepresentation(GetValue(iter),temp);
            int alignLeft = x + (maxSize - repr.size);
            mvaddnstr(y,alignLeft,repr.data,repr.size);
            x += maxSize + spacing;
         }
      } else {
         Iterator iter = Iterate(val);

         Value currentSelected = {};

         for(int index = 0; HasNext(iter); index += 1,Advance(&iter)){
            if(index == state->cursorPosition){
               wattron( w, A_STANDOUT );
               currentSelected = GetValue(iter);
            }

            move(index + 2,0);
            String integer = PushString(temp,"%d\n",index);
            addnstr(integer.data,integer.size);

            if(index == state->cursorPosition){
               wattroff( w, A_STANDOUT );
            }
         }

         if(numberElements){
            if(currentSelected.type->type == Type::STRUCT){
               StructPanel(w,currentSelected,0,8,true,arena);
            } else {
               String repr = GetValueRepresentation(currentSelected,temp);
               mvaddnstr(2,8,repr.data,repr.size);
            }
         }
      }
   } else {
      String typeName = PushString(temp,"%.*s",UNPACK_SS(type->name));
      mvaddnstr(1,0,typeName.data,typeName.size);
   }

   refresh();
}

#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>

static Value storedDebugValue;
static bool validStoredDebugValue = false;

void StartDebugTerminal(){
   if(validStoredDebugValue){
      DebugTerminal(storedDebugValue);
   }
}

static void debug_sig_handler(int sig){
   if(sig == SIGUSR1){
      StartDebugTerminal();
   }
   if(sig == SIGSEGV){
      StartDebugTerminal();
      signal(SIGSEGV, SIG_DFL);
      raise(SIGSEGV);
   }
   if(sig == SIGABRT){
      StartDebugTerminal();
      signal(SIGABRT, SIG_DFL);
      raise(SIGABRT);
   }
}

void SetDebuggingValue(Value val){
   static bool init = false;

   if(!init){
      signal(SIGUSR1, debug_sig_handler);
      signal(SIGSEGV, debug_sig_handler);
      signal(SIGABRT, debug_sig_handler);
      init = true;
   }

   storedDebugValue = val;
   validStoredDebugValue = true;
}

static int terminalWidth,terminalHeight;
static bool resized;

static void terminal_sig_handler(int sig){
   if (sig == SIGWINCH) {
      winsize winsz;

      ioctl(0, TIOCGWINSZ, &winsz);

      terminalWidth = winsz.ws_row;
      terminalHeight = winsz.ws_col;
      resized = true;
  }
}

void DebugTerminal(Value initialValue){
   Assert(WatchableObject(initialValue));

   Arena arena = {};
   Arena temp = {};
   InitArena(&arena,Megabyte(1));
   InitArena(&temp,Megabyte(1));

   for(int i = 0; i < MAX_PANELS; i++){
      globalCurrentPanels[i] = 0;
   }

   globalPanels[0][0].name = STRING("TOP");
   globalPanels[0][0].cursorPosition = 0;
   globalPanels[0][0].valueLooking = CollapsePtrIntoStruct(initialValue);

	WINDOW *w  = initscr();

   cbreak();
   noecho();
   keypad(stdscr, TRUE);
   nodelay(w,true);

   curs_set(0);

   static bool init = false;
   if(!init){
      getmaxyx(w, terminalHeight, terminalWidth);
      resized = false;
      signal(SIGWINCH, terminal_sig_handler);
   }

   TerminalIteration(0,w,INPUT_NONE,&arena,&temp);

   int c;
   while((c = getch()) != 'q'){
      Input input = {};

      switch(c){
      case KEY_BACKSPACE:{
         input = INPUT_BACKSPACE;
      }break;
      case 10:
      case KEY_ENTER:{
         input = INPUT_ENTER;
      }break;
      case KEY_LEFT:{
         input = INPUT_LEFT;
      }break;
      case KEY_RIGHT:{
         input = INPUT_RIGHT;
      }break;
      case KEY_DOWN:{
         input = INPUT_DOWN;
      }break;
      case KEY_UP:{
         input = INPUT_UP;
      }break;
      }

      bool iterate = false;
      if(resized){
         resizeterm(terminalWidth,terminalHeight);
         resized = false;
         iterate = true;
      }

      if(input != INPUT_NONE || iterate){
         TerminalIteration(0,w,input,&arena,&temp);
      }
	}

	endwin();

	Free(&arena);
}
#else
void DebugTerminal(Value initialValue){
   LogOnce(LogModule::DEBUG_SYS,LogLevel::DEBUG,"No ncurses support, no debug terminal");
   return;
}
#endif
