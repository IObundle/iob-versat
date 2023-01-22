#include "merge.hpp"

#include <pthread.h>

#include "thread.hpp"
//#include "debug.hpp"
//#include "textualRepresentation.hpp"

static pthread_mutex_t bestCliqueMutex;

#define MAX_CLIQUE_TIME 10.0f

inline float MyClock(){
   timespec out = {};
   clock_gettime(CLOCK_MONOTONIC,&out);
   //printf("%d.%d\n",out.tv_sec,out.tv_nsec);

   int milli = out.tv_nsec / 1000000;
   float res = ((float) out.tv_sec) + (((float) milli) / 1000.0f);

   return res;
}

struct IterativeCliqueFrame{
   ConsolidationGraph inputGraph;
   ConsolidationGraph tempGraph;
   IndexRecord record;
   Byte* frameMark;
   int lastI;
   bool insideLoop;
   int size;
   int frameNumber;
   IterativeCliqueFrame* previousFrame;
};

struct IterativeState{
   Arena* arena; // Each state will have it's own arena
   IterativeCliqueFrame* stack;
   IterativeCliqueFrame* current;
   IterativeCliqueFrame firstFrame;
   ConsolidationGraph bestGraphFound;
   int max;
   bool runFully;
};

void Init(IterativeState* state,Arena* arena,int index,ConsolidationGraph graph,BitArray* neighbors){
   state->arena = arena;

   InitArena(arena,Megabyte(1));

   state->bestGraphFound = Copy(graph,arena);
   state->bestGraphFound.validNodes.Fill(0);
   state->firstFrame.record.index = index;
   state->firstFrame.size = 1;

   state->firstFrame.inputGraph = Copy(graph,arena);
   for(int j = index; j < graph.nodes.size; j++){
      state->firstFrame.inputGraph.validNodes.Set(j,1);
   }
   state->firstFrame.inputGraph.validNodes &= neighbors[index];

   state->firstFrame.frameMark = MarkArena(arena);
   state->current = &state->firstFrame;
}

bool IterativeClique(IterativeState* state,BitArray* neighbors,int* table){
   // Unpack state
   IterativeCliqueFrame*& current = state->current;
   Arena*& arena = state->arena;

   while(current){
      //printf("Frame: %d Stack: %p\n",current->size,current->frameMark);

      PopMark(arena,current->frameMark);
      if(!current->insideLoop && current->inputGraph.validNodes.GetNumberBitsSet() == 0){
            if(current->size > state->max){ // Basically, if for index - 1 we found a clique of size N, then for index, the maximum that we can find is N + 1. So if we find a N + 1 clique, we can terminate earlier
               pthread_mutex_lock(&bestCliqueMutex);
               // Record best clique found so far
               state->max = current->size;

               state->bestGraphFound.validNodes.Fill(0);
               for(IndexRecord* ptr = &current->record; ptr != nullptr; ptr = ptr->next){
                  state->bestGraphFound.validNodes.Set(ptr->index,1);
               }
               pthread_mutex_unlock(&bestCliqueMutex);
               current = current->previousFrame;
               return true; // Found the best possible, end early
            }
            current = current->previousFrame;
            continue;
      } else {
         #if 0
         auto end = MyClock();
         float elapsed = (end - state->start);
         if(elapsed > MAX_CLIQUE_TIME){
            state->found = true;
            return;
         }
         #endif

         int num = current->inputGraph.validNodes.GetNumberBitsSet();
         if(num == 0){
            current = current->previousFrame;
            continue;
         }

         #if 1
         if(current->size + num <= state->max){
            current = current->previousFrame;
            continue;
         }
         #endif

         // Should replace with a counting leading zeros or something
         int i = current->inputGraph.validNodes.FirstBitSetIndex(); // current->lastI;

         #if 1
         if(current->size + table[i] <= state->max){
            current = current->previousFrame;
            continue;
         }
         #endif

         current->insideLoop = true;
         current->inputGraph.validNodes.Set(i,0);
         current->tempGraph = Copy(current->inputGraph,arena);
         current->tempGraph.validNodes &= neighbors[i];
         current->lastI = i;

         //printf("%d %d %d %d\n",i,current->tempGraph.validNodes.GetNumberBitsSet(),current->inputGraph.validNodes.GetNumberBitsSet(),num);

         printf("%d\n",i);
         IterativeCliqueFrame* newFrame = PushStruct<IterativeCliqueFrame>(arena);

         newFrame->frameMark = MarkArena(arena);
         newFrame->inputGraph = current->tempGraph;
         newFrame->previousFrame = current;
         newFrame->record.index = i;
         newFrame->record.next = &current->record;
         newFrame->size = current->size + 1;

         current = newFrame;
         continue;
      }
   }
   if(!current){
      state->runFully = true;
   }
   return false;
}

ConsolidationGraph ParallelIterativeMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena){
   ConsolidationGraph cliqueOut = Copy(graph,arena);
   ArenaMarker marker(arena);

   int numberNodes = graph.nodes.size;

   Array<Arena> arenas = PushArray<Arena>(arena,numberNodes);
   Array<IterativeState> states = PushArray<IterativeState>(arena,numberNodes);
   Array<int> table = PushArray<int>(arena,numberNodes);
   BitArray* neighbors = graph.edges.data;

   graph.validNodes.Fill(0);
   for(int i = 0; i < numberNodes; i++){
      Init(&states[i],&arenas[i],i,graph,neighbors);
   }

   for(int j = 0; j < numberNodes; j++){
      bool anyImproved = false;
      for(int i = numberNodes - 1; i >= 0; i--){
         IterativeState* state = &states[i];

         state->max = j;
         bool improved = IterativeClique(state,neighbors,table.data);
         anyImproved |= improved;

         if(state->max > table[i] && improved){ // Table gives the minimum bound. If N - 1 can reach K nodes, then N can have at a minimum K nodes as well. The question is whether it can reach K + 1 or not
            table[i] = state->max;
         }

         if(state->max == upperBound){
            break;
         }

         #if 0
         if(max > table[i]){
            table[i] = max;
         }
         #endif
      }
   }

   for(int i = 0; i < numberNodes; i++){
      printf("%d ",table[i]);
   }
   printf("\n");

   for(int i = 0; i < numberNodes; i++){
      printf("%d",states[i].runFully ? 1 : 0);
   }
   printf("\n");

   cliqueOut.validNodes.Copy(states[0].bestGraphFound.validNodes);

   Assert(IsClique(cliqueOut).result);

   return cliqueOut;
}

void IterativeClique(ParallelCliqueState* state,ConsolidationGraph graphArg,int firstIndex,BitArray* neighbors,Arena* arena){
   ArenaMarker marker(arena);

   IterativeCliqueFrame first = {};
   first.inputGraph = Copy(graphArg,arena);
   first.record.index = firstIndex;
   first.size = 1;

   first.frameMark = MarkArena(arena);

   printf("C: %d\n",firstIndex);
   IterativeCliqueFrame* current = &first;
   while(current){
      //printf("Frame: %d Stack: %p\n",current->size,current->frameMark);

      PopMark(arena,current->frameMark);
      if(!current->insideLoop && current->inputGraph.validNodes.GetNumberBitsSet() == 0){
            if(current->size > *state->max){ // Basically, if for index - 1 we found a clique of size N, then for index, the maximum that we can find is N + 1. So if we find a N + 1 clique, we can terminate earlier
               pthread_mutex_lock(&bestCliqueMutex);
               // Record best clique found so far
               *state->max = current->size;

               state->cliqueOut->validNodes.Fill(0);
               for(IndexRecord* ptr = &current->record; ptr != nullptr; ptr = ptr->next){
                  state->cliqueOut->validNodes.Set(ptr->index,1);
               }
               pthread_mutex_unlock(&bestCliqueMutex);
               return; // Found the best possible, end early
            }
            current = current->previousFrame;
            continue;
      } else {
         #if 0
         auto end = MyClock();
         float elapsed = (end - state->start);
         if(elapsed > MAX_CLIQUE_TIME){
            state->found = true;
            return;
         }
         #endif

         int num = current->inputGraph.validNodes.GetNumberBitsSet();
         if(num == 0){
            current = current->previousFrame;
            continue;
         }

         #if 1
         if(current->size + num <= *state->max){
            current = current->previousFrame;
            continue;
         }
         #endif

         int i = current->inputGraph.validNodes.FirstBitSetIndex(); // current->lastI;

         #if 1
         if(current->size + state->table[i] <= *state->max){
            current = current->previousFrame;
            continue;
         }
         #endif

         current->insideLoop = true;
         current->inputGraph.validNodes.Set(i,0);
         current->tempGraph = Copy(current->inputGraph,arena);
         current->tempGraph.validNodes &= neighbors[i];
         current->lastI = i;

         //printf("%d %d %d %d\n",i,current->tempGraph.validNodes.GetNumberBitsSet(),current->inputGraph.validNodes.GetNumberBitsSet(),num);

         printf("%d\n",i);
         IterativeCliqueFrame* newFrame = PushStruct<IterativeCliqueFrame>(arena);

         newFrame->frameMark = MarkArena(arena);
         newFrame->inputGraph = current->tempGraph;
         newFrame->previousFrame = current;
         newFrame->record.index = i;
         newFrame->record.next = &current->record;
         newFrame->size = current->size + 1;

         current = newFrame;
         continue;
      }
   }
}

#if 0
void IterativeMaxClique(int id,void* args){
   RefParallelTask* task = (RefParallelTask*) args;

   Arena* arena = &task->state->arenas[id];
   ArenaMarker marker(arena);
   int index = task->index;

   ParallelCliqueState state = {};
   state.max = task->max;
   state.table = task->table;
   state.cliqueOut = task->cliqueOut;
   state.found = false;
   state.thisMax = 0; //*task->max;
   state.start = task->start;

   bool timeout = false;
   state.timeout = &timeout;

   ConsolidationGraph graph = Copy(task->originalGraph,arena);
   for(int j = index; j < graph.nodes.size; j++){
      graph.validNodes.Set(j,1);
   }
   graph.validNodes &= task->neighbors[index];

   IndexRecord record = {};
   record.index = index;
   //ParallelClique(&state,graph,&record,1,task->neighbors,arena);

   IterativeClique(&state,graph,index,task->neighbors,arena);

   if(!state.timeout){
      pthread_mutex_lock(&bestCliqueMutex);
      task->table[task->index] = *task->max;
      //task->table[task->index] = state.thisMax;
      pthread_mutex_unlock(&bestCliqueMutex);
   }
}
#endif

void ParallelClique(ParallelCliqueState* state,ConsolidationGraph graphArg,IndexRecord* record,int size,Arena* arena){
   ConsolidationGraph graph = Copy(graphArg,arena);

   if(graph.validNodes.GetNumberBitsSet() == 0){
      if(size > state->thisMax){
         state->thisMax = size;
      }

      if(size > *state->max){
         pthread_mutex_lock(&bestCliqueMutex);

         // Just to make sure, because we read earlier
         MemoryBarrier();
         if(size > *state->max){
            *state->max = size;

            state->cliqueOut->validNodes.Fill(0);
            for(IndexRecord* ptr = record; ptr != nullptr; ptr = ptr->next){
               state->cliqueOut->validNodes.Set(ptr->index,1);
            }

            state->table[state->index] = size;
         }
         pthread_mutex_unlock(&bestCliqueMutex);
      }
      return;
   }

   state->counter += 1;
   if(state->counter >= 1000){
      state->counter = 0;

      auto end = MyClock(); // Calling clock is kinda of expensive every iteration
      float elapsed = (end - state->start);
      if(elapsed > MAX_CLIQUE_TIME){
         state->found = true;
         state->timeout = true;
         return;
      }
   }

   int num = 0;
   while((num = graph.validNodes.GetNumberBitsSet()) != 0){
      #if 1
      if(size + num <= *state->max){
         return;
      }
      #endif

      int i = graph.validNodes.FirstBitSetIndex();

      #if 1
      if(size + state->table[i] <= *state->max){
         return;
      }
      #endif

      graph.validNodes.Set(i,0);

      ArenaMarker marker(arena);
      ConsolidationGraph tempGraph = Copy(graph,arena);

      BitArray* neighbors = graph.edges.data;
      tempGraph.validNodes &= neighbors[i];

      IndexRecord newRecord = {};
      newRecord.index = i;
      newRecord.next = record;

      ParallelClique(state,tempGraph,&newRecord,size + 1,arena);

      if(state->found){
         return;
      }
   }
}

void TaskMaxClique(int id,void* args){
   RefParallelTask* task = (RefParallelTask*) args;

   Arena* arena = &task->state->threadArenas[id];
   ArenaMarker marker(arena);
   int index = task->index;

   ParallelCliqueState state = {};
   state.max = &task->state->max;
   state.table = task->state->table;
   state.cliqueOut = &task->state->result;
   state.found = false;
   state.thisMax = 0;
   state.start = task->state->start; //MyClock();
   state.timeout = false;
   state.index = index;
   state.counter = 0;

   clock_t taskStart = MyClock();
   #if 0
   printf("%d\n",task->index);
   #endif

   ConsolidationGraph graph = Copy(task->state->graphCopy,arena);
   #if 1
   for(int j = index; j < graph.nodes.size; j++){
      graph.validNodes.Set(j,1);
   }
   #endif
   graph.validNodes &= task->state->graphCopy.edges.data[index];

   IndexRecord record = {};
   record.index = index;
   ParallelClique(&state,graph,&record,1,arena);

   clock_t taskEnd = MyClock();

   //printf("%d: %3f\n",task->index,taskEnd - taskStart);

   #if 1
   // No mutex needed.
   int max = state.thisMax;
   for(int i = index; i < task->state->graphCopy.nodes.size; i++){
      if(task->state->table[i] != 9999){
         max = std::max(max,task->state->table[i]);
      }
   }
   //printf("%d %d\n",index,max);
   task->state->table[index] = max;
   #endif
}

void Init(RefParallelState* state,ConsolidationGraph graph,int upperBound,Arena* arena,TaskFunction function){
   *state = {};

   state->result = Copy(graph,arena); // Return value, allocated firs before arena mark
   state->arenaMark = MarkArena(arena);
   state->graphCopy = Copy(graph,arena);
   state->table = PushArray<int>(arena,graph.nodes.size);
   Memset(state->table,9999);
   state->graphCopy.validNodes.Fill(0);
   state->nThreads = NumberThreads();
   state->threadArenas = PushArray<Arena>(arena,state->nThreads);
   state->taskFunction = function;
   for(Arena& ar : state->threadArenas){
      InitArena(&ar,Megabyte(4)); // Allocate one per thread
   }
   state->i = graph.nodes.size - 1;
   state->start = MyClock();
}

ConsolidationGraph AdvanceAll(RefParallelState* parallelState,Arena* arena){
   ArenaMarker marker(arena);

   Task task = {};
   task.function = parallelState->taskFunction;
   int i = parallelState->i;
   for(; i >= 0; i--){
      RefParallelTask* state = PushStruct<RefParallelTask>(arena);
      state->state = parallelState;
      state->index = i;

      task.args = (void*) state;

      while(FullTasks());
      AddTask(task);
   }
   WaitCompletion();

   float end = MyClock();
   if(end - parallelState->start > MAX_CLIQUE_TIME){
      parallelState->timeout = true;
   }

   parallelState->i = i;
   Assert(IsClique(parallelState->result).result);

   return parallelState->result;
}

void PrintTable(Array<int> table){
   int maximum = 0;
   for(int& val : table){
      maximum = std::max(maximum,val);
   }

   int nDigits = NumberDigitsRepresentation(maximum);

   char formatString[5];
   formatString[0] = '\%';
   formatString[1] = '0' + nDigits;
   formatString[2] = 'd';
   formatString[3] = ' ';
   formatString[4] = '\0';

   int x = 0;
   for(int& val : table){
      printf(formatString,val);
      x += (nDigits + 1);
      if(x > 160){
         x = 0;
         printf("\n");
      }
   }
}

ConsolidationGraph ParallelMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena){
   static bool init = false;
   if(!init){
      InitThreadPool(16);
      pthread_mutex_init(&bestCliqueMutex,NULL);
      init = true;
   }

   ConsolidationGraph res = {};
   #if 1
      RefParallelState state = {};
      Init(&state,graph,upperBound,arena,TaskMaxClique);

      PrintTable(state.table);
      printf("\n");

      ConsolidationGraph res0 = AdvanceAll(&state,arena);
      MemoryBarrier();

      printf("\n");
      PrintTable(state.table);
      printf("\n");

      if(state.timeout){
         for(int i = state.table.size - 1; i >= 0; i--){
            if(state.table[i] == 9999){
               printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
               break;
            }
         }
      }

      res = res0;
   #endif
   #if 0
      ConsolidationGraph res1 = ParallelIterativeMaxClique(graph,upperBound,arena);
      res = res1;
   #endif
   #if 0
      RefParallelState state = {};
      Init(&state,graph,upperBound,arena,IterativeMaxClique);
      ConsolidationGraph res0 = AdvanceAll(&state);
      res = res2;
   #endif

   return res;
}



