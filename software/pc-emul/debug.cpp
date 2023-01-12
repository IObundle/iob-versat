#include "debug.hpp"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <set>

#include "type.hpp"
#include "textualRepresentation.hpp"

void CheckMemory(Accelerator* topLevel){
   STACK_ARENA(temp,Kilobyte(64));

   AcceleratorIterator iter = {};
   iter.Start(topLevel,&temp,true);

   CheckMemory(iter);
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
   STACK_ARENA(temp,Kilobyte(64));
   AcceleratorIterator iter = {};
   iter.Start(topLevel,&temp,true);
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
               printf("\nStatic [%d]: ",inst->config - accel->staticAlloc.ptr);
            } else {
               printf("\nConfig [%d]: ",inst->config - accel->configAlloc.ptr);
            }

            OutputSimpleIntegers(inst->config,val.configs);
         }
         if(val.states){
            printf("\nState [%d]: ",inst->state - accel->stateAlloc.ptr);
            OutputSimpleIntegers(inst->state,val.states);
         }
         if(val.delays){
            printf("\nDelay [%d]: ",inst->delay - accel->delayAlloc.ptr);
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
   STACK_ARENA(temp,Kilobyte(64));

   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel,&temp,false); inst; inst = iter.Next()){
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

static void OutputGraphDotFile_(Versat* versat,AcceleratorView& view,bool collapseSameEdges,ComplexFUInstance* highlighInstance,FILE* outputFile){
   Arena* arena = &versat->temp;

   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(ComplexFUInstance** instPtr : view.nodes){
      ComplexFUInstance* inst = *instPtr;
      SizedString id = UniqueRepr(inst,arena);
      SizedString name = Repr(inst,versat->debug.dotFormat,arena);

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

      SizedString first = UniqueRepr(edge->units[0].inst,arena);
      SizedString second = UniqueRepr(edge->units[1].inst,arena);
      PortInstance start = edge->units[0];
      PortInstance end = edge->units[1];
      SizedString label = Repr(start,end,versat->debug.dotFormat,arena);
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

   FILE* file = fopen(buffer,"w");
   OutputGraphDotFile_(versat,view,collapseSameEdges,nullptr,file);
   fclose(file);

   va_end(args);
}

void OutputGraphDotFile(Versat* versat,AcceleratorView& view,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = fopen(buffer,"w");
   OutputGraphDotFile_(versat,view,collapseSameEdges,highlighInstance,file);
   fclose(file);

   va_end(args);
}

SizedString PushMemoryHex(Arena* arena,void* memory,int size){
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

   for(ComplexFUInstance* inst : accel->instances){
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
   STACK_ARENA(temp,Kilobyte(64));
   ResetMapping();

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock ? 1 : 0);
   fprintf(accelOutputFile,"b%s b\n",Bin((time + 1) / 2));

   AcceleratorIterator iter = {};
   iter.Start(accel,&temp,true);
   PrintVCD_(accelOutputFile,iter,time,sameValueCheckSpace);
}

#if 0
#define NCURSES
#endif

#ifdef NCURSES
#include <ncurses.h>
#include <signal.h>

struct PanelState{
   SizedString name;
   Value valueLooking;
   int cursorPosition;
   Byte* arenaMark;
};

static PanelState panels[99] = {};
static int currentPanel = 0;

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
   INPUT_DOWN,
   INPUT_UP
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

   for(Member& member : type->members){
      int yPos = startPos + menuIndex;

      if(!displayOnly && menuIndex == cursorPosition){
         wattron( w, A_STANDOUT );
      }

      mvaddnstr(yPos, xStart, member.name.data,member.name.size);

      Value memberVal = AccessStruct(val,&member);
      SizedString repr = GetValueRepresentation(memberVal,arena);

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

void TerminalIteration(WINDOW* w,Input input,Arena* arena,Arena* temp){
   ArenaMarker mark(temp);
   clear();
   // Before panel decision
   if(input == INPUT_BACKSPACE){
      if(currentPanel > 0){
         PanelState* currentState = &panels[currentPanel];
         PopMark(arena,currentState->arenaMark);

         currentPanel -= 1;
      }
   } else if(input == INPUT_ENTER){
      PanelState* currentState = &panels[currentPanel];

      Value val = currentState->valueLooking;
      Type* type = val.type;

      // Assume new state will be created and fill with default parameters
      PanelState* newState = &panels[currentPanel + 1];
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
         Iterator iter = Iterate(val);
         for(int i = 0; HasNext(iter); i += 1,Advance(&iter)){
            if(i == currentState->cursorPosition){
               Value val = GetValue(iter);

               currentPanel += 1;

               newState->name = PushString(arena,"%d\n",i);;
               newState->valueLooking = val;
               break;
            }
         }
      }
   }

   currentPanel = std::max(0,currentPanel);

   // At this point, panel is locked in
   PanelState* state = &panels[currentPanel];

   Value val = state->valueLooking;
   Type* type = val.type;

   SizedString panelNumber = PushString(temp,"%d",currentPanel);
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
      Iterator iter = Iterate(val);

      Value currentSelected = {};

      for(int index = 0; HasNext(iter); index += 1,Advance(&iter)){
         if(index == state->cursorPosition){
            wattron( w, A_STANDOUT );
            currentSelected = GetValue(iter);
         }

         move(index + 2,0);
         SizedString integer = PushString(temp,"%d\n",index);
         addnstr(integer.data,integer.size);

         if(index == state->cursorPosition){
            wattroff( w, A_STANDOUT );
         }
      }

      if(numberElements){
         if(currentSelected.type->type == Type::STRUCT){
            StructPanel(w,currentSelected,0,8,true,arena);
         } else {
            SizedString repr = GetValueRepresentation(currentSelected,temp);
            mvaddnstr(2,8,repr.data,repr.size);
         }
      }
   } else {
      SizedString typeName = PushString(temp,"%.*s",UNPACK_SS(type->name));
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

   currentPanel = 0;
   panels[0].name = MakeSizedString("TOP");
   panels[0].cursorPosition = 0;
   panels[0].valueLooking = CollapsePtrIntoStruct(initialValue);

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

   TerminalIteration(w,INPUT_NONE,&arena,&temp);

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
         TerminalIteration(w,input,&arena,&temp);
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
