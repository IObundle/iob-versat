#include "accel.hpp"

#include "versat.hpp"
#include "accelerator.hpp"

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

#define IMPLEMENT_VERILOG_UNITS
#include "wrapper.inc"

static Versat* versat = nullptr;
static Accelerator* accel = nullptr;

volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

void versat_init(int base){
   versat = InitVersat(base,1,false);

   RegisterAllVerilogUnitsVerilog(versat);

   SetDebug(versat,VersatDebugFlags::OUTPUT_VCD,1);

   accel = CreateAccelerator(versat);
   FUDeclaration* type = GetTypeByName(versat,STRING(acceleratorTypeName));
   CreateFUInstance(accel,type,STRING("TOP"));

   //ParseVersatSpecification(versat,"testVersatSpecification.txt");

   // accel = CreateAccelerator(versat);
   // FUDeclaration* type = GetTypeByName(versat,STRING(acceleratorTypeName));
   // CreateFUInstance(accel,type,STRING("TOP"));

   /*
      If we do a verilator pass over the versat_instance, we end up with an interface which cannot be mapped to a pointer like data access.
      Probably the best approach would be to separate the versat_instance into a "Top" level module unit, which has as input and output the config, state and external memory ports.
         The problem would then be that the external memory ports would have to span the entire memory address.

   */

   accelConfig = (volatile AcceleratorConfig*) accel->configAlloc.ptr;
   accelState = (volatile AcceleratorState*) accel->stateAlloc.ptr;
}

void RunAccelerator(int times){
   AcceleratorRun(accel,times);
}
