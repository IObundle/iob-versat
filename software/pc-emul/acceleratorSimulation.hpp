#ifndef INCLUDED_VERSAT_ACCELERATOR_SIMULATION
#define INCLUDED_VERSAT_ACCELERATOR_SIMULATION

#include "accelerator.hpp"

void AcceleratorRunStart(Accelerator* accel);
bool AcceleratorDone(Accelerator* accel);

void AcceleratorEvalIter(AcceleratorIterator iter);
void AcceleratorEval(AcceleratorIterator iter);

void AcceleratorRunComposite(AcceleratorIterator iter);
void AcceleratorRunIter(AcceleratorIterator iter);
void AcceleratorRunOnce(Accelerator* accel);
void AcceleratorRun(Accelerator* accel,int times);

#endif // INCLUDED_VERSAT_ACCELERATOR_SIMULATION
