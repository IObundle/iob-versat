#ifndef UNIT_VCD_INCLUDED
#define UNIT_VCD_INCLUDED

#include <verilated_vcd_c.h>

class VCDData{
public:
   VerilatedVcdC vcd;
   int timesDumped;

   VCDData();

   void dump();
   void open(const char* filepath);
};

#define ENABLE_TRACE(unitPtr,vcdDataPtr) unitPtr->trace(&vcdDataPtr->vcd, 99);  // Trace 99 levels of hierarchy

#endif // UNIT_VCD_INCLUDED