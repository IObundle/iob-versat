#include "versat_accel.hpp"

#define IMPLEMENT_VERILOG_UNITS

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define nullptr 0

typedef struct Versat Versat;
typedef struct Accelerator Accelerator;
typedef struct FUDeclaration FUDeclaration;

typedef unsigned char bool;

static Versat* versat = nullptr;
static Accelerator* accel = nullptr;
volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

int versat_base;

void InitializeVerilator();
Versat* InitVersatC(int base,int numberConfigurations,bool initUnits);
void RegisterAllVerilogUnitsVerilog(Versat* versat);
Accelerator* CreateSimulableAccelerator(Versat* versat,const char* typename);
void AcceleratorRunC(Accelerator* accel,int times);
void UnitWrite(Versat* versat,Accelerator* accel,int addrArg,int val);
int UnitRead(Versat* versat,Accelerator* accel,int addr);

void* GetStartOfConfig(Accelerator* accel);
void* GetStartOfState(Accelerator* accel);

void Debug(){
   //DebugAccelerator(accel);
}

void versat_init(int base){
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

void RunAccelerator(int times){
   AcceleratorRunC(accel,times);
}

void VersatMemoryCopy(iptr* dest,iptr* data,int size){
   memcpy(dest,data,sizeof(iptr) * size);
}

void VersatUnitWrite(int addrArg,int val){
   int addr = addrArg - (versat_base + memMappedStart); // Convert back to zero based address
   UnitWrite(versat,accel,addr,val);
}

int VersatUnitRead(int base,int index){
   int addr = base + index - (versat_base + memMappedStart); // Convert back to zero based address
   return UnitRead(versat,accel,addr);
}
