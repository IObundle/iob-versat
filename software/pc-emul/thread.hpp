#ifndef INCLUDED_THREADS
#define INCLUDED_THREADS

typedef void (*TaskFunction)(int id,void* args);

struct Task{
   TaskFunction function;
   void* args;
};

void InitThreadPool(int threads);
void TerminatePool(bool force = false);

bool IsTaskWaiting();

void AddTask(Task task);

#endif // INCLUDED_THREADS
