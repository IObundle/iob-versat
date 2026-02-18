#if 0
#include "thread.hpp"

#include <pthread.h>
#include <semaphore.h>

#include <unistd.h>

#include "utils.hpp"

static pthread_mutex_t poolMutex;
static sem_t poolSemaphore;
static pthread_t threads[64]; // Limit to 64, for now
static int numberThreads;

inline void LockMutex(){pthread_mutex_lock(&poolMutex);}
inline void UnlockMutex(){pthread_mutex_unlock(&poolMutex);}
inline void IncSem(){sem_post(&poolSemaphore);}
inline void DecSem(){sem_wait(&poolSemaphore);}

static const int MAX_TASKS = 256;

// Pool mutex locked
static Task tasks[MAX_TASKS];
static int taskWrite;
static int taskRead;
static bool stop;

static int jobsAdded;
static int jobsFinished;

void* ThreadFunction(void* arg){
   union {
      void* ptr;
      struct{
         int i0;
         int i1;
      };
   } conv;

   conv.ptr = arg;
   int id = conv.i0; // For now, embed id as the argument

   while(1){
      DecSem();

      if(stop){
         break;
      }

      LockMutex();
      Task task = tasks[taskRead];
      taskRead = (taskRead + 1) % MAX_TASKS;
      UnlockMutex();

      task.function(id,task.args);

      LockMutex();
      jobsFinished += 1; // Should be an atomic increment
      UnlockMutex();
   }

   return nullptr;
}

void InitThreadPool(int nThreads){
   static bool init = false;
   Assert(!init);
   init = true;

   Assert(nThreads < 64);

   numberThreads = nThreads;
   taskWrite = 0;
   taskRead = 0;
   stop = false;

   int res = 0;
   res |= pthread_mutex_init(&poolMutex,NULL);
   res |= sem_init(&poolSemaphore,0,0);

   for(iptr i = 0; i < nThreads; i++){
      res |= pthread_create(&threads[i],NULL,ThreadFunction,(void*) i);
   }

   Assert(res == 0);
}

bool FullTasks(){
   LockMutex();
   bool full = ((taskWrite + 1) % MAX_TASKS == taskRead);
   UnlockMutex();
   return full;
}

void AddTask(Task task){
   while(FullTasks()) asm("");

   MemoryBarrier();

   LockMutex();

   Assert((taskWrite + 1) % MAX_TASKS != taskRead); // Could also have just a delay, waiting for more free space, but could lock if AddTask called by the tasks themselves

   tasks[taskWrite] = task;
   taskWrite = (taskWrite + 1) % MAX_TASKS;

   UnlockMutex();

   IncSem();

   jobsAdded += 1;
}

int NumberThreads(){
   return numberThreads;
}

void WaitCompletion(){
   while(1){
      LockMutex();
      bool end = (jobsAdded == jobsFinished);
      UnlockMutex();

      if(end){
         break;
      } else {
         //sleep(1);
      }
   }

   return;
}

void TerminatePool(bool force){
   if(!force){
      // Wait for threads to terminate the remaining tasks
      while(1){
         LockMutex();
         bool end = (taskWrite == taskRead);
         UnlockMutex();

         if(end){
            break;
         } else {
            //sleep(1);
         }
      }
   }

   LockMutex();
   stop = true;
   UnlockMutex();

   for(int i = 0; i < numberThreads; i++){ // Make sure that threads can see the stop and get out
      IncSem();
   }

   for(int i = 0; i < numberThreads; i++){
      pthread_join(threads[i],NULL);
   }
}

WorkGroup* PushWorkGroup(Arena* out,int numberWork){
   WorkGroup* work = PushStruct<WorkGroup>(out);
   work->tasks = PushArray<Task>(out,numberWork);

   return work;
}
#endif
