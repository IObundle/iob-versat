#ifndef INCLUDED_THREADS
#define INCLUDED_THREADS

typedef void (*TaskFunction)(int id,void* args);

struct Task{
   TaskFunction function;
   void* args;
};

void InitThreadPool(int threads);
void TerminatePool(bool force = false);

void WaitCompletion();

int NumberThreads();

bool FullTasks();
void AddTask(Task task);

#define MemoryBarrier() __asm__ __volatile__("":::"memory"); __sync_synchronize() // Gcc specific

#endif // INCLUDED_THREADS
