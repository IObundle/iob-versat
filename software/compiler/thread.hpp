#pragma once

#include "memory.hpp"

typedef void (*TaskFunction)(int id,void* args);

#define TASK_FUNCTION (TaskFunction*)

struct Task{
   TaskFunction function;
   int order;
   void* args;
};

struct WorkGroup{
   TaskFunction function;
   Array<Task> tasks;
};

void InitThreadPool(int threads);
void TerminatePool(bool force = false);

void WaitCompletion();

int NumberThreads();

bool FullTasks();
void AddTask(Task task);

WorkGroup* PushWorkGroup(Arena* out,int numberWork);

void DoWork(WorkGroup* work);

#define MemoryBarrier() __asm__ __volatile__("":::"memory"); __sync_synchronize() // Gcc specific
