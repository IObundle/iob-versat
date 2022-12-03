#include "debug.hpp"

#include <cstdlib>

#include "type.hpp"

static void AssertValidRandomGraphOptions(RandomGraphOptions* options){
   Assert(options->maxNodes >= options->minNodes);
   Assert(options->maxDepth >= options->minDepth);

   float totalProbability = 0.0f;
   for(int i = 0; i < options->numberTypes; i++){
      RandomGraphTypeOption* type = &options->types[i];

      totalProbability += type->probability;
   }

   Assert(std::abs(totalProbability - 1.0f) < 0.01f);
}

struct TypeData{
   int numberNodes;
};

Accelerator* GenerateRandomGraph(Versat* versat,RandomGraphOptions* options,unsigned int seed){
   AssertValidRandomGraphOptions(options);

   srand(seed);

   int nodes = RandomNumberBetween(options->minNodes,options->maxNodes,rand());
   int depth = RandomNumberBetween(options->minDepth,options->maxDepth,rand());

   NOT_IMPLEMENTED;

   return nullptr;
}

void CheckMemory(Accelerator* topLevel,Accelerator* accel){
   //AcceleratorIterator iter = {};
   //for(FUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){

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

static void DisplayIntList(int* ptr, int size){
   for(int i = 0; i < size; i++){
      printf("%d\n",ptr[i]);
   }
}

void DisplayAcceleratorMemory(Accelerator* topLevel){
   printf("Config:\n");
   DisplayIntList(topLevel->configAlloc.ptr,topLevel->configAlloc.size);

   printf("Static:\n");
   DisplayIntList(topLevel->staticAlloc.ptr,topLevel->staticAlloc.size);

   printf("Delay:\n");
   DisplayIntList(topLevel->delayAlloc.ptr,topLevel->delayAlloc.size);
}

static char GetHex(int value){
   if(value < 10){
      return '0' + value;
   } else{
      return 'a' + (value - 10);
   }
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

static void OutputSimpleIntegers(int* mem,int size){
   for(int i = 0; i < size; i++){
      printf("%d ",mem[i]);
   }
}

void DisplayUnitConfiguration(Accelerator* topLevel){
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(topLevel); inst; inst = iter.Next()){
      UnitValues val = CalculateIndividualUnitValues(inst);

      FUDeclaration* type = inst->declaration;

      if(val.configs | val.states | val.delays){
         printf("[%.*s] %.*s:",UNPACK_SS(type->name),UNPACK_SS(inst->name));
         if(val.configs){
            if(IsConfigStatic(topLevel,inst)){
               printf("\nStatic [%d]: ",inst->config - topLevel->staticAlloc.ptr);
            } else {
               printf("\nConfig [%d]: ",inst->config - topLevel->configAlloc.ptr);
            }

            OutputSimpleIntegers(inst->config,val.configs);
         }
         if(val.states){
            printf("\nState [%d]: ",inst->state - topLevel->stateAlloc.ptr);
            OutputSimpleIntegers(inst->state,val.states);
         }
         if(val.delays){
            printf("\nDelay [%d]: ",inst->delay - topLevel->delayAlloc.ptr);
            OutputSimpleIntegers(inst->delay,val.delays);
         }
         printf("\n\n");
      }
   }
}

#if 1
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

static int RolloverRange(int min,int val,int max){
   if(val < min){
      val = max;
   } else if(val > max){
      val = min;
   }
}

static int Clamp(int min,int val,int max){
   if(val < min){
      return min;
   }
   if(val >= max){
      return max;
   }

   return val;
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

   int width,height;
   getmaxyx(w, height, width);

   const int panelWidth = 32;

   int menuIndex = 0;
   int startPos = 2;

   for(Member* member = val.type->members; member != nullptr; member = member->next){
      int yPos = startPos + menuIndex;

      if(menuIndex == state->cursorPosition){
         wattron( w, A_STANDOUT );
      }

      mvaddnstr(yPos, 0, member->name.str,member->name.size);

      // Ugly hack, somethings are char* despite not pointing to strings but to areas of memory
      // Type system cannot distinguish them, and therefore we kinda hack it away
      if(member->type != GetType(MakeSizedString("char*"))){
         //printf("%.*s\n",UNPACK_SS(member->name));
         Value memberVal = AccessStruct(val,member);
         SizedString repr = GetValueRepresentation(memberVal,arena);

         move(yPos, panelWidth);
         addnstr(repr.str,repr.size);
      }

      move(yPos, panelWidth + panelWidth / 2);
      addnstr(member->type->name.str,member->type->name.size);

      if(menuIndex == state->cursorPosition){
         wattroff( w, A_STANDOUT );
      }

      menuIndex += 1;
   }
}

void TerminalIteration(WINDOW* w,Input input,Arena* arena){
   // Before panel decision
   if(input == INPUT_BACKSPACE){
      currentPanel -= 1;

      PanelState* currentState = &panels[currentPanel];
      //PopMark(arena,currentState->arenaMark);
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

   mvaddnstr(0, 0,  val.type->name.str, val.type->name.size);

   if(state->name.size){
      addnstr(":",1);
      addnstr(state->name.str,state->name.size);
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
         Value val = GetValue(iter);
         Type* type = val.type;

         if(index == state->cursorPosition){
            wattron( w, A_STANDOUT );
         }

         move(index + 2,0);
         {
            ArenaMarker marker(arena);
            SizedString integer = PushString(arena,"%d\n",index);
            addnstr(integer.str,integer.size);
         }

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
   InitArena(&arena,Megabyte(1));

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

   TerminalIteration(w,INPUT_NONE,&arena);

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
         TerminalIteration(w,input,&arena);
         refresh();
      }
	}

	endwin();

	Free(&arena);
}

#endif
