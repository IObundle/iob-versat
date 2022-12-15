#include "debug.hpp"

#include <stdarg.h>
#include <cstdlib>
#include <set>

#include "type.hpp"
#include "textualRepresentation.hpp"

void CheckMemory(Accelerator* topLevel,Accelerator* accel){
   for(ComplexFUInstance* inst : accel->instances){
      printf("[%.*s] %.*s:\n",UNPACK_SS(inst->declaration->name),UNPACK_SS(inst->name));
      if(inst->isStatic){
         printf("C:%d\n",inst->config ? inst->config - topLevel->staticAlloc.ptr : -1);
      } else {
         printf("C:%d\n",inst->config ? inst->config - topLevel->configAlloc.ptr : -1);
      }

      printf("S:%d\n",inst->state ? inst->state - topLevel->stateAlloc.ptr : -1);
      printf("D:%d\n",inst->delay ? inst->delay - topLevel->delayAlloc.ptr : -1);
      printf("O:%d\n",inst->outputs ? inst->outputs - topLevel->outputAlloc.ptr : -1);
      printf("o:%d\n",inst->storedOutputs ? inst->storedOutputs - topLevel->storedOutputAlloc.ptr : -1);
      printf("E:%d\n",inst->extraData ? inst->extraData - topLevel->extraDataAlloc.ptr : -1);
      printf("\n");
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
   Arena* arena = &topLevel->versat->temp;
   ArenaMarker marker(arena);
   AcceleratorView view = CreateAcceleratorView(topLevel,arena);
   DisplayUnitConfiguration(view);
}

void DisplayUnitConfiguration(AcceleratorView topLevel){
   for(int i = 0; i < topLevel.nodes.size; i++){
      ComplexFUInstance* inst = topLevel.nodes[i];
      UnitValues val = CalculateIndividualUnitValues(inst);

      FUDeclaration* type = inst->declaration;

      if(val.configs | val.states | val.delays){
         printf("[%.*s] %.*s:",UNPACK_SS(type->name),UNPACK_SS(inst->name));
         if(val.configs){
            if(IsConfigStatic(topLevel.accel,inst)){
               printf("\nStatic [%d]: ",inst->config - topLevel.accel->staticAlloc.ptr);
            } else {
               printf("\nConfig [%d]: ",inst->config - topLevel.accel->configAlloc.ptr);
            }

            OutputSimpleIntegers(inst->config,val.configs);
         }
         if(val.states){
            printf("\nState [%d]: ",inst->state - topLevel.accel->stateAlloc.ptr);
            OutputSimpleIntegers(inst->state,val.states);
         }
         if(val.delays){
            printf("\nDelay [%d]: ",inst->delay - topLevel.accel->delayAlloc.ptr);
            OutputSimpleIntegers(inst->delay,val.delays);
         }
         printf("\n\n");
      }
   }
}

bool CheckInputAndOutputNumber(FUDeclaration* type,int inputs,int outputs){
   if(inputs > type->nInputs){
      return false;
   } else if(outputs > type->nOutputs){
      return false;
   }

   return true;
}

bool IsGraphValid(AcceleratorView view){
   Assert(view.graphData);

   InstanceMap map;

   for(int i = 0; i < view.nodes.size; i++){
      ComplexFUInstance* inst = view.nodes[i];

      inst->tag = 0;
      map.insert({inst,inst});
   }

   for(int i = 0; i < view.edges.size; i++){
      Edge* edge = view.edges[i].edge;

      for(int i = 0; i < 2; i++){
         auto res = map.find(edge->units[i].inst);

         Assert(res != map.end());

         res->first->tag = 1;
      }

      Assert(edge->units[0].port < edge->units[0].inst->declaration->nOutputs && edge->units[0].port >= 0);
      Assert(edge->units[1].port < edge->units[1].inst->declaration->nInputs && edge->units[1].port >= 0);
   }

   if(view.edges.size){
      for(int i = 0; i < view.nodes.size; i++){
         ComplexFUInstance* inst = view.nodes[i];

         Assert(inst->tag == 1 || inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED);
      }
   }

   return true;
}

static void OutputGraphDotFile_(Versat* versat,AcceleratorView view,bool collapseSameEdges,FILE* outputFile){
   Arena* arena = &versat->temp;

   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(int i = 0; i < view.nodes.size; i++){
      ComplexFUInstance* inst = view.nodes[i];
      SizedString name = Repr(inst,arena,false);
      SizedString type = Repr(inst->declaration,arena);

      fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(name));
      //fprintf(outputFile,"\t\"%.*s\" [label=\"%.*s\"];\n",UNPACK_SS(name),UNPACK_SS(type));
   }

   std::set<std::pair<ComplexFUInstance*,ComplexFUInstance*>> sameEdgeCounter;

   for(int i = 0; i < view.edges.size; i++){
      Edge* edge = view.edges[i].edge;
      if(collapseSameEdges){
         std::pair<ComplexFUInstance*,ComplexFUInstance*> key{edge->units[0].inst,edge->units[1].inst};

         if(sameEdgeCounter.count(key) == 1){
            continue;
         }

         sameEdgeCounter.insert(key);
      }

      SizedString first = Repr(edge->units[0].inst,arena,false);
      SizedString second = Repr(edge->units[1].inst,arena,false);

      fprintf(outputFile,"\t\"%.*s\" -> ",UNPACK_SS(first));
      fprintf(outputFile,"\"%.*s\"",UNPACK_SS(second));

      #if 0
      ComplexFUInstance* outputInst = edge->units[0].inst;
      int delay = 0;
      for(int i = 0; i < outputInst->graphData->numberOutputs; i++){
         if(edge->units[1].inst == outputInst->graphData->outputs[i].instConnectedTo.inst && edge->units[1].port == outputInst->graphData->outputs[i].instConnectedTo.port){
            delay = outputInst->graphData->outputs[i].delay;
            break;
         }
      }

      fprintf(outputFile,"[label=\"%d->%d:%d\"]",edge->units[0].port,edge->units[1].port,delay);
      #endif

      fprintf(outputFile,";\n");
   }

   fprintf(outputFile,"}\n");
}

void OutputGraphDotFile(Versat* versat,AcceleratorView view,bool collapseSameEdges,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = fopen(buffer,"w");
   OutputGraphDotFile_(versat,view,collapseSameEdges,file);
   fclose(file);

   va_end(args);
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

static std::array<char,4> currentMapping = {'a','a','a','a'};
static int mappingIncrements = 0;
static void ResetMapping(){
   currentMapping[0] = 'a';
   currentMapping[1] = 'a';
   currentMapping[2] = 'a';
   currentMapping[3] = 'a';
   mappingIncrements = 0;
}

static void IncrementMapping(){
   for(int i = 3; i >= 0; i--){
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
   Arena* arena = &accel->versat->temp;
   AcceleratorView view = CreateAcceleratorView(accel,arena);
   view.CalculateGraphData(arena);

   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->nConfigs; i++){
         Wire* wire = &info->wires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s_%.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(info->name),UNPACK_SS(wire->name));
         IncrementMapping();
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      fprintf(accelOutputFile,"$scope module %.*s_%d $end\n",UNPACK_SS(inst->name),inst->id);

      for(int i = 0; i < inst->graphData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_in%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->graphData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_out%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_stored_out%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nConfigs; i++){
         Wire* wire = &inst->declaration->configWires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(wire->name));
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nStates; i++){
         Wire* wire = &inst->declaration->stateWires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(wire->name));
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"$var wire 32 %c%c%c%c delay%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],i);
         IncrementMapping();
      }

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"$var wire  1 %c%c%c%c done $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      if(inst->compositeAccel){
         PrintVCDDefinitions_(accelOutputFile,inst->compositeAccel);
      }

      fprintf(accelOutputFile,"$upscope $end\n");
   }
}

void PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel){
   ResetMapping();

   fprintf(accelOutputFile,"$timescale   1ns $end\n");
   fprintf(accelOutputFile,"$scope module TOP $end\n");
   fprintf(accelOutputFile,"$var wire  1 a clk $end\n");
   PrintVCDDefinitions_(accelOutputFile,accel);
   fprintf(accelOutputFile,"$upscope $end\n");
   fprintf(accelOutputFile,"$enddefinitions $end\n");
}

static char* Bin(unsigned int val){
   static char buffer[33];
   buffer[32] = '\0';

   for(int i = 0; i < 32; i++){
      if(val - (1 << (31 - i)) < val){
         val = val - (1 << (31 - i));
         buffer[i] = '1';
      } else {
         buffer[i] = '0';
      }
   }
   return buffer;
}

static void PrintVCD_(FILE* accelOutputFile,Accelerator* accel,int time){
   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->nConfigs; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(info->ptr[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         }
         IncrementMapping();
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      for(int i = 0; i < inst->graphData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(GetInputValue(inst,i)),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->graphData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->outputs[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->storedOutputs[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nConfigs; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->config[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         }
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nStates; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->state[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->delay[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"%d%c%c%c%c\n",inst->done ? 1 : 0,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      if(inst->compositeAccel){
         PrintVCD_(accelOutputFile,inst->compositeAccel,time);
      }
   }
}

void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock){ // Need to put some clock signal
   ResetMapping();

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock ? 1 : 0);
   PrintVCD_(accelOutputFile,accel,time);
}

#if 0
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

void StructPanel(WINDOW* w,PanelState* state,Input input,Arena* arena){
   Value val = state->valueLooking;
   Type* type = val.type;

   const int panelWidth = 32;

   int menuIndex = 0;
   int startPos = 2;

   for(Member* member = type->members; member != nullptr; member = member->next){
      int yPos = startPos + menuIndex;

      if(menuIndex == state->cursorPosition){
         wattron( w, A_STANDOUT );
      }

      mvaddnstr(yPos, 0, member->name.data,member->name.size);

      // Ugly hack, somethings are char* despite not pointing to strings but to areas of memory
      // Type system cannot distinguish them, and therefore we kinda hack it away
      if(member->type != GetType(MakeSizedString("char*"))){
         //printf("%.*s\n",UNPACK_SS(member->name));
         Value memberVal = AccessStruct(val,member);
         SizedString repr = GetValueRepresentation(memberVal,arena);

         move(yPos, panelWidth);
         addnstr(repr.data,repr.size);
      }

      move(yPos, panelWidth + panelWidth / 2);
      addnstr(member->type->name.data,member->type->name.size);

      if(menuIndex == state->cursorPosition){
         wattroff( w, A_STANDOUT );
      }

      menuIndex += 1;
   }
}

void TerminalIteration(WINDOW* w,Input input,Arena* arena,Arena* temp){
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

      // Switch on type
      if(type->type == Type::STRUCT){
         Member* member = GetListByIndex(type->members,currentState->cursorPosition);

         Value selectedValue = AccessStruct(val,currentState->cursorPosition);

         selectedValue = CollapsePtrIntoStruct(selectedValue);

         if(selectedValue.type->type == Type::STRUCT || (selectedValue.type->type == Type::TEMPLATED_INSTANCE && selectedValue.type->templateBase == ValueType::POOL)){
            currentPanel += 1;

            newState->name = member->name;
            newState->valueLooking = selectedValue;
         }
      } else if(type->type == Type::TEMPLATED_INSTANCE && type->templateBase == ValueType::POOL){
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

   // At this point, panel is locked in,
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
      numberElements = ListCount(type->members);
   } else if(type->type == Type::TEMPLATED_INSTANCE && type->templateBase == ValueType::POOL){
      Iterator iter = Iterate(val);
      for(; HasNext(iter); Advance(&iter)){
         numberElements += 1;
      }
   }

   if(input == INPUT_DOWN){
      state->cursorPosition += 1;
   } else if(input == INPUT_UP){
      state->cursorPosition -= 1;
   }

   state->cursorPosition = RolloverRange(0,state->cursorPosition,numberElements - 1);

   if(type->type == Type::STRUCT){
      StructPanel(w,state,input,arena);
      return;
   } else if(type->type == Type::TEMPLATED_INSTANCE && type->templateBase == ValueType::POOL){
      Iterator iter = Iterate(val);
      for(int index = 0; HasNext(iter); index += 1,Advance(&iter)){
         //Value val = GetValue(iter);

         if(index == state->cursorPosition){
            wattron( w, A_STANDOUT );
         }

         move(index + 2,0);
         SizedString integer = PushString(temp,"%d\n",index);
         addnstr(integer.data,integer.size);

         if(index == state->cursorPosition){
            wattroff( w, A_STANDOUT );
         }
      }
   } else {
      mvaddnstr(1,0,"NO TYPE",7);
   }
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
   if (sig == SIGUSR1){
      StartDebugTerminal();
   }
}

void SetDebuggingValue(Value val){
   static bool init = false;

   if(!init){
      signal(SIGUSR1, debug_sig_handler);
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
   Arena arena = {};
   Arena temp = {};
   InitArena(&arena,Megabyte(1));
   InitArena(&temp,Megabyte(1));

   panels[0].name = MakeSizedString("TOP");
   panels[0].cursorPosition = 0;
   panels[0].valueLooking = initialValue;

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
         clear();
         ArenaMarker mark(&temp);
         TerminalIteration(w,input,&arena,&temp);
         refresh();
      }
	}

	endwin();

	Free(&arena);
}

#endif
