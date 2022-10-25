#include "debug.hpp"

#include "type.hpp"

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

#if 0
#include <ncurses.h>

struct PanelState{
   Value valueLooking;
   int cursorPosition;
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

static int clamp(int min,int val,int max){
   if(val < min){
      return min;
   }
   if(val >= max){
      return max;
   }

   return val;
}

void DebugTerminal(Value initialValue){
   panels[0].valueLooking = initialValue;

	WINDOW *w  = initscr();

   cbreak();
   noecho();
   keypad(stdscr, TRUE);
   nodelay(w,true);

   curs_set(0);

   int panelWidth = 32;

   mvaddnstr(0, 0, "Top:", 4);
   int c;
   while((c = getch()) != 'q'){
      // Before panel decision
      switch(c){
      case KEY_BACKSPACE:{
         currentPanel -= 1;
      }break;
      case KEY_ENTER:{
         // currentPanel += 1;
         // TODO: Have to fetch the selected value, if structure or equivalent
      }break;
      }
      currentPanel = std::max(0,currentPanel);

      // At this point, panel is locked in,
      PanelState* state = &panels[currentPanel];

      Value val = state->valueLooking;
      Type* type = val.type;

      if(type->type != Type::STRUCT){
         mvaddstr(2,0,"NOT A STRUCT");
         continue;
      }

      int numberMembers = 0;

      #if 1
      if(type->type == Type::STRUCT){
         numberMembers = ListCount(type->members);
      }
      #endif

      switch(c){
      case KEY_DOWN:{
         state->cursorPosition += 1;
      } break;
      case KEY_UP:{
         state->cursorPosition -= 1;
      }break;
		}
      state->cursorPosition = clamp(0,state->cursorPosition,numberMembers - 1);

		int menuIndex = 0;
      for(Member* member = val.type->members; member != nullptr; member = member->next){
         int yPos = 2 + menuIndex;
         if(menuIndex == state->cursorPosition){
            wattron( w, A_STANDOUT );
         }

         mvaddnstr(yPos, 0, UNPACK_SS_REVERSE(member->name));

         if(menuIndex == state->cursorPosition){
            wattroff( w, A_STANDOUT );
         }

         #if 1
         move(yPos, panelWidth);
         //printw("%d ",member->offset);
         switch(member->type->type){
         case Type::STRUCT:{
            addstr("STRUCT:");
            addnstr(UNPACK_SS_REVERSE(member->name));
         }break;
         case Type::POINTER:{
            addnstr(UNPACK_SS_REVERSE(member->name));
            addstr(":");
            Value ptr = AccessStruct(val,member);

            printw("%x",ptr.isTemp);
         }break;
         case Type::BASE:{
            Value number = AccessStruct(val,member);

            addnstr(UNPACK_SS_REVERSE(member->name));
            addstr(":");

            printw("%x",number.number);
         }break;
         }
         #endif

         menuIndex += 1;
      }

      refresh();
	}

	endwin();
}

#endif
