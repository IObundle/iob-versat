#include "unitVCD.hpp"

#include "utils.hpp"

static bool initTracing = false;
static VerilatedVcdC* vcdFiles[128];
static int openedVcdFiles = 0;

static void CloseOpenedVcdFiles(){
   //printf("Closing vcd files\n");

   for(int i = 0; i < openedVcdFiles; i++){
      vcdFiles[i]->close();
   }
}

static void InitVCD(){
   if(!initTracing){
      Verilated::traceEverOn(true);
      initTracing = true;
      atexit(CloseOpenedVcdFiles);
   }
}

VCDData::VCDData():vcd(){
   InitVCD();

   timesDumped = 0;
}

void VCDData::dump()
{
   vcd.dump(5 * timesDumped++);
}

void VCDData::open(const char* filepath){
   vcd.open(filepath);

   Assert(openedVcdFiles < 128);

   vcdFiles[openedVcdFiles++] = &vcd;
}
