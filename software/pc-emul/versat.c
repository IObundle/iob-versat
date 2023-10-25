#include "versat_accel.h"

#ifdef __cplusplus
#include <cstdlib>
#include <cstring>
#else
#include <string.h>
#include <stdbool.h>
#endif

#define IMPLEMENT_VERILOG_UNITS
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define nullptr 0

typedef struct Versat Versat;
typedef struct Accelerator Accelerator;
typedef struct FUDeclaration FUDeclaration;

static Versat* versat = nullptr;
static Accelerator* accel = nullptr;
volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

int versat_base;

#ifdef __cplusplus
extern "C"{
#endif

void InitializeVerilator();
Versat* InitVersatC(int base,int numberConfigurations,bool initUnits);
void DebugAcceleratorC(Accelerator* accel);
void RegisterAllVerilogUnitsVerilog(Versat* versat);
Accelerator* CreateSimulableAccelerator(Versat* versat,const char* typeName);
void AcceleratorRunC(Accelerator* accel,int times);
void UnitWrite(Versat* versat,Accelerator* accel,int addrArg,int val);
int UnitRead(Versat* versat,Accelerator* accel,int addr);
void SignalLoopC(Accelerator* accel);

void* GetStartOfConfig(Accelerator* accel);
void* GetStartOfState(Accelerator* accel);

#ifdef __cplusplus
}
#endif

void Debug(){
   DebugAcceleratorC(accel);
}

bool CreateVCD;
bool SimulateDatabus;

void ConfigCreateVCD(bool value){
   CreateVCD = value;
}

void ConfigSimulateDatabus(bool value){
   SimulateDatabus = value;
}

void versat_init(int base){
   CreateVCD = true;
   SimulateDatabus = true;
   
   InitializeVerilator();

   versat_base = base;
   versat = InitVersatC(base,1,0);

   RegisterAllVerilogUnitsVerilog(versat);

   accel = CreateSimulableAccelerator(versat,acceleratorTypeName);

   char* configView = (char*) GetStartOfConfig(accel);

   accelConfig = (volatile AcceleratorConfig*) GetStartOfConfig(accel);
   accelState = (volatile AcceleratorState*) GetStartOfState(accel);

   iptr* delayPtr = (iptr*) (configView + (delayStart - configStart));
   iptr* staticPtr = (iptr*) (configView + (staticStart - configStart));

   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayPtr[i] = delayBuffer[i];
   }

   for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){
      staticPtr[i] = staticBuffer[i];
   }   
}

void StartAccelerator(){
  RunAccelerator(1);
}

void EndAccelerator(){
   // Do nothing. Start accelerator does everything, for now
}

void RunAccelerator(int times){
   AcceleratorRunC(accel,times);
}

void VersatMemoryCopy(void* dest,void* data,int size){
   memcpy(dest,data,size);
}

void VersatUnitWrite(int baseaddr,int index,int val){
  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based address
  UnitWrite(versat,accel,addr,val);
}

int VersatUnitRead(int baseaddr,int index){
  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
   return UnitRead(versat,accel,addr);
}

float VersatUnitReadFloat(int base,int index){
   int addr = base + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
   int value = UnitRead(versat,accel,addr);
   float* view = (float*) &value;
   return *view;
}

void SignalLoop(){
   SignalLoopC(accel);
   // Do nothing, for now
}
