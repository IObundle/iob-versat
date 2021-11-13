#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "assert.h"

#include <ncurses.h>

#include "../versat.h"
#include "versat_instance_template.h"

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static int versat_base;

typedef struct Range_t{
   int high,low;
} Range;

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat,int base){
   versat->accelerators = (Accelerator*) malloc(10 * sizeof(Accelerator));
   versat->declarations = (FUDeclaration*) malloc(10 * sizeof(FUDeclaration));
   versat_base = base;
}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int nInputs,int nOutputs,int nConfigs,const Wire* configWires,int nStates,const Wire* stateWires,int memoryMapBytes,bool doesIO,int extraDataSize,FUFunction initializeFunction,FUFunction startFunction,FUFunction updateFunction){
   FUDeclaration decl = {};
   FU_Type type = {};

   decl.nInputs = nInputs;
   decl.nOutputs = nOutputs;
   decl.name = declarationName;
   decl.nStates = nStates;
   decl.stateWires = stateWires;
   decl.nConfigs = nConfigs;
   decl.configWires = configWires;
   decl.memoryMapBytes = memoryMapBytes;
   decl.doesIO = doesIO;
   decl.extraDataSize = extraDataSize;
   decl.initializeFunction = initializeFunction;
   decl.startFunction = startFunction;
   decl.updateFunction = updateFunction;

   type.type = versat->nDeclarations;
   versat->declarations[versat->nDeclarations++] = decl;

   return type;
}

static void InitAccelerator(Accelerator* accel){
   accel->instances = (FUInstance*) malloc(100 * sizeof(FUInstance));
}

Accelerator* CreateAccelerator(Versat* versat){
   Accelerator accel = {};

   InitAccelerator(&accel);

   accel.versat = versat;
   versat->accelerators[versat->nAccelerators] = accel;

   return &versat->accelerators[versat->nAccelerators++];
}

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type){
   FUInstance instance = {};
   FUDeclaration decl = accel->versat->declarations[type.type];

   instance.declaration = type;
   
   if(decl.nInputs)
      instance.inputs = (FUInput*) malloc(decl.nInputs * sizeof(FUInput));
   if(decl.nOutputs){
      instance.outputs = (int32_t*) malloc(decl.nOutputs * sizeof(int32_t));
      instance.storedOutputs = (int32_t*) malloc(decl.nOutputs * sizeof(int32_t));
   }
   if(decl.nStates)
      instance.state = (volatile int*) malloc(decl.nStates * sizeof(int));
   if(decl.nConfigs)
      instance.config = (volatile int*) malloc(decl.nConfigs * sizeof(int));
   if(decl.memoryMapBytes)
      instance.memMapped = (volatile int*) malloc(decl.memoryMapBytes);
   if(decl.extraDataSize)
      instance.extraData = calloc(decl.extraDataSize,sizeof(int));

   accel->instances[accel->nInstances] = instance;

   FUInstance* ptr = &accel->instances[accel->nInstances++];

   if(decl.initializeFunction)
      decl.initializeFunction(ptr);

   return ptr;
}

FUDeclaration* GetDeclaration(Versat* versat,FUInstance* instance){
   FU_Type type = instance->declaration;
   FUDeclaration* decl = &versat->declarations[type.type];   

   return decl;
}

static void AcceleratorRunStart(Versat* versat,Accelerator* accel){
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* inst = &accel->instances[i];
      FUDeclaration* decl = GetDeclaration(versat,inst);
      FUFunction startFunction = decl->startFunction;

      if(startFunction){
         int32_t* startingOutputs = startFunction(inst);
         
         if(startingOutputs)
            memcpy(inst->outputs,startingOutputs,decl->nOutputs * sizeof(int32_t));
      }
   }   
}

static void AcceleratorRunIteration(Versat* versat,Accelerator* accel){
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];
      FUDeclaration* decl = GetDeclaration(versat,instance);

      int32_t* newOutputs = decl->updateFunction(instance);

      memcpy(instance->storedOutputs,newOutputs,decl->nOutputs * sizeof(int32_t));
   }

   for(int i = 0; i < accel->nInstances; i++){        
      FUDeclaration* decl = GetDeclaration(versat,&accel->instances[i]);
      memcpy(accel->instances[i].outputs,accel->instances[i].storedOutputs,decl->nOutputs * sizeof(int32_t));
   }
}

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction){
   AcceleratorRunStart(versat,accel);

   while(true){
      AcceleratorRunIteration(versat,accel);

      // For now, the end is kinda hardcoded.
      if(terminateFunction(endRoot))
         break;
   }
}

static int maxi(int a1,int a2){
   int res = (a1 > a2 ? a1 : a2);

   return res;
}

// Log function customized to calculating bits needed for a number of possible addresses (ex: log2i(1024) = 10)
static int log2i(int value){
   int res = 0;

   if(value <= 0)
      return 0;

   value -= 1; // If value matches a power of 2, we still want to return the previous value

   while(value > 1){
      value /= 2;
      res++;
   }

   res += 1;

   return res;
}

typedef struct VersatComputedValues_t{
   int memoryMapped;
   int unitsMapped;
   int nConfigs;
   int nStates;
   int nUnitsIO;
   int configurationBits;
   int stateBits;
   int lowerAddressSize;
   int addressSize;
   int configurationAddressBits;
   int stateAddressBits;
   int unitsMappedAddressBits;
   int maxMemoryMapBytes;
   int memoryAddressBits;
   int stateConfigurationAddressBits;
   int memoryMappingAddressBits;

   // Auxiliary for versat generation;
   int memoryConfigDecisionBit;
   
   // Address
   Range memoryMappedUnitAddressRange;
   Range memoryAddressRange;
   Range configAddressRange;
   Range stateAddressRange;

   // Bits
   Range configBitsRange;
   Range stateBitsRange;
} VersatComputedValues;

static VersatComputedValues ComputeVersatValues(Versat* versat){
   VersatComputedValues res = {};

   // Only dealing with one accelerator for now
   Accelerator* accel = &versat->accelerators[0];

   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];
      FUDeclaration* decl = GetDeclaration(versat,instance);

      if(decl->memoryMapBytes > res.maxMemoryMapBytes)
         res.maxMemoryMapBytes = decl->memoryMapBytes;

      res.memoryMapped += decl->memoryMapBytes;
      if(decl->memoryMapBytes)
         res.unitsMapped += 1;

      res.nConfigs += decl->nConfigs;

      if(decl->nConfigs){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            res.configurationBits += decl->configWires[ii].bitsize;
         }
      }

      res.nStates += decl->nStates;
      if(decl->nStates){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            res.stateBits += decl->stateWires[ii].bitsize;
         }
      }

      if(decl->doesIO)
         res.nUnitsIO += 1;
   }
   
   // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register 
   res.nConfigs += 1;
   res.nStates += 1;

   res.memoryAddressBits = log2i(res.maxMemoryMapBytes);
   if(!versat->byteAddressable)
      res.memoryAddressBits -= 2;

   res.unitsMappedAddressBits = log2i(res.unitsMapped);
   res.memoryMappingAddressBits = res.unitsMappedAddressBits + res.memoryAddressBits;
   res.configurationAddressBits = log2i(res.nConfigs);
   res.stateAddressBits = log2i(res.nStates);
   res.stateConfigurationAddressBits = maxi(res.configurationAddressBits,res.stateAddressBits);

   res.lowerAddressSize = maxi(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);
   res.addressSize = res.lowerAddressSize + 1; // One bit to select between memory or config/state
   
   res.memoryConfigDecisionBit = res.addressSize - 1;
   res.memoryMappedUnitAddressRange.high = res.addressSize - 2;
   res.memoryMappedUnitAddressRange.low = res.memoryMappedUnitAddressRange.high - res.unitsMappedAddressBits + 1;
   res.memoryAddressRange.high = res.memoryAddressBits - 1;
   res.memoryAddressRange.low = 0;
   res.configAddressRange.high = res.configurationAddressBits - 1;
   res.configAddressRange.low = 0;
   res.stateAddressRange.high = res.stateAddressBits - 1;
   res.stateAddressRange.low = 0;

   res.configBitsRange.high = res.configurationBits - 1;
   res.configBitsRange.low = 0;
   res.stateBitsRange.high = res.stateBits - 1;
   res.stateBitsRange.low = 0;

   return res;
}

void OutputMemoryMap(Versat* versat){
   VersatComputedValues val = ComputeVersatValues(versat);

   printf("\n");
   printf("Total Memory mapped: %d\n",val.memoryMapped);
   printf("Maximum memory mapped by a unit: %d\n",val.maxMemoryMapBytes);
   printf("Memory address bits: %d\n",val.memoryAddressBits);
   printf("Units mapped: %d\n",val.unitsMapped);
   printf("Units address bits: %d\n",val.unitsMappedAddressBits);
   printf("Memory mapping address bits: %d\n",val.memoryMappingAddressBits);
   printf("\n");
   printf("Configuration registers: %d (including versat reg)\n",val.nConfigs);
   printf("Configuration address bits: %d\n",val.configurationAddressBits);
   if(versat->useShadowRegisters)
      printf("Configuration + shadow bits used: %d\n",val.configurationBits * 2);
   else
      printf("Configuration bits used: %d\n",val.configurationBits);
   printf("\n");
   printf("State registers: %d (including versat reg)\n",val.nStates);
   printf("State address bits: %d\n",val.stateAddressBits);
   printf("State bits used: %d\n",val.stateBits);
   printf("\n");
   printf("IO connections: %d\n",val.nUnitsIO);

   printf("\n");
   printf("Number units: %d\n",versat->accelerators[0].nInstances);
   printf("Versat address size: %d\n",val.addressSize);
   printf("\n");

   #define ALIGN_FORMAT "%-14s"
   #define ALIGN_SIZE 14

   int bitsNeeded = val.lowerAddressSize;

   printf(ALIGN_FORMAT,"Address:");
   for(int i = bitsNeeded; i >= 10; i--)
      printf("%d ",i/10);
   printf("\n");
   printf(ALIGN_FORMAT," ");
   for(int i = bitsNeeded; i >= 0; i--)
      printf("%d ",i%10);
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Memory mapped
   printf(ALIGN_FORMAT,"MemoryMapped:");
   printf("1 ");
   for(int i = bitsNeeded - 1; i >= 0; i--)
      if(i < val.memoryAddressBits)
         printf("X ");
      else if(i < (val.memoryAddressBits + val.unitsMappedAddressBits))
         printf("U ");
      else
         printf("0 ");
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Versat registers
   printf(ALIGN_FORMAT,"Versat Regs:");
   for(int i = bitsNeeded - 0; i >= 0; i--)
      printf("0 ");
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Config/State
   printf(ALIGN_FORMAT,"Config/State:");
   printf("0 ");
   for(int i = bitsNeeded - 1; i >= 0; i--){
      if(i < val.configurationAddressBits && i < val.stateAddressBits)  
         printf("B ");
      else if(i < val.configurationAddressBits)
         printf("C ");
      else if(i < val.stateAddressBits)
         printf("S ");
      else
         printf("0 ");
   }
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   printf("\n");
   printf("U - Unit selection\n");
   printf("X - Any value\n");
   printf("C - Used only by Config\n");
   printf("S - Used only by State\n");
   printf("B - Used by both Config and State\n");
   printf("\n");
   printf("Memory/Config bit: %d\n",val.memoryConfigDecisionBit);
   printf("Unit range: [%d:%d]\n",val.memoryMappedUnitAddressRange.high,val.memoryMappedUnitAddressRange.low);
   printf("Memory range: [%d:%d]\n",val.memoryAddressRange.high,val.memoryAddressRange.low);
   printf("Config range: [%d:%d]\n",val.configAddressRange.high,val.configAddressRange.low);
   printf("State range: [%d:%d]\n",val.stateAddressRange.high,val.stateAddressRange.low);

   printf("\n");
}

int32_t GetInputValue(FUInstance* instance,int index){
   FUInput input = instance->inputs[index];
   FUInstance* inst = input.instance;

   if(inst){
      return inst->outputs[input.index];   
   }
   else{
      return 0;
   }   
}

// Connects out -> in
void ConnectUnits(Versat* versat,FUInstance* out,int outIndex,FUInstance* in,int inIndex){
   FUDeclaration* inDecl = GetDeclaration(versat,in);
   FUDeclaration* outDecl = GetDeclaration(versat,out);

   assert(inIndex < inDecl->nInputs);
   assert(outIndex < outDecl->nOutputs);

   in->inputs[inIndex].instance = out;
   in->inputs[inIndex].index = outIndex;
}

#define TAB "   "

void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath){
   FILE* s = fopen(sourceFilepath,"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      return;
   }

   FILE* d = fopen(definitionFilepath,"w");

   if(!d){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      fclose(s);
      return;
   }

   // Only dealing with 1 accelerator, for now
   Accelerator* accel = &versat->accelerators[0];

   VersatComputedValues val = ComputeVersatValues(versat);

   fprintf(d,"`define NUMBER_UNITS %d\n",accel->nInstances);
   fprintf(d,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(d,"`define STATE_W %d\n",val.stateBits);
   fprintf(d,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(d,"`define nIO %d\n",val.nUnitsIO);
   fprintf(d,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);

   if(versat->useShadowRegisters)
      fprintf(d,"`define SHADOW_REGISTERS 1\n");
   if(val.nUnitsIO)
      fprintf(d,"`define IO 1\n");

   fprintf(s,versat_instance_template_str);

   bool wroteFirst = false;
   fprintf(s,"wire [31:0] ");
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nOutputs; ii++){
         if(wroteFirst)
            fprintf(s,",");

         fprintf(s,"output_%d_%d",i,ii);
         wroteFirst = true;
      }
   }
   fprintf(s,";\n\n");

   if(val.memoryMapped){
      fprintf(s,"wire [31:0] ");
      for(int i = 0; i < val.unitsMapped; i++){
         if(i != 0)
            fprintf(s,",");
         fprintf(s,"rdata_%d",i);
      }
      fprintf(s,";\n\n");
   }

   // Memory mapping   
   if(val.memoryMapped){
      fprintf(s,"assign unitRdataFinal = (unitRData[0]");
      
      for(int i = 1; i < val.unitsMapped; i++){
         fprintf(s," | unitRData[%d]",i);
      }
      fprintf(s,");\n\n");
   } else {
      fprintf(s,"assign unitRdataFinal = 32'h0;\n");
      fprintf(s,"assign unitReady = 0;\n");
   }

   if(val.memoryMapped){
      fprintf(s,"always @*\n");
      fprintf(s,"begin\n");
      fprintf(s,"%smemoryMappedEnable = {%d{1'b0}};\n",TAB,val.unitsMapped);

      for(int i = 0; i < val.unitsMapped; i++){
         fprintf(s,"%sif(valid & memoryMappedAddr & addr[%d:%d] == %d'd%d)\n",TAB,val.memoryMappedUnitAddressRange.high,val.memoryMappedUnitAddressRange.low,val.memoryMappedUnitAddressRange.high - val.memoryMappedUnitAddressRange.low + 1,i);
         fprintf(s,"%s%smemoryMappedEnable[%d] = 1'b1;\n",TAB,TAB,i);
      }
      fprintf(s,"end\n\n");
   }

   // Config data decoding
   fprintf(s,"always @(posedge clk,posedge rst_int)\n");
   fprintf(s,"%sif(rst_int) begin\n",TAB);
   if(versat->useShadowRegisters)
      fprintf(s,"%s%sconfigdata_shadow <= {`CONFIG_W{1'b0}};\n",TAB,TAB);
   else
      fprintf(s,"%s%sconfigdata <= {`CONFIG_W{1'b0}};\n",TAB,TAB);

   fprintf(s,"%send else if(valid & we & !memoryMappedAddr) begin\n",TAB);
   
   int index = 1; // 0 is reserved for versat registers
   int bitCount = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nConfigs; ii++){
         int bitSize = decl->configWires[ii].bitsize;

         fprintf(s,"%s%sif(addr[%d:%d] == %d'd%d)\n",TAB,TAB,val.configAddressRange.high,val.configAddressRange.low,val.configAddressRange.high - val.configAddressRange.low + 1,index++);
         if(versat->useShadowRegisters)
            fprintf(s,"%s%s%sconfigdata_shadow[%d+:%d] <= wdata[%d:0];\n",TAB,TAB,TAB,bitCount,bitSize,bitSize-1);
         else
            fprintf(s,"%s%s%sconfigdata[%d+:%d] <= wdata[%d:0];\n",TAB,TAB,TAB,bitCount,bitSize,bitSize-1);
         bitCount += bitSize;
      }
   }
   fprintf(s,"%send\n\n",TAB);

   // State data decoding
   index = 1; // 0 is reserved for versat registers
   bitCount = 0;
   fprintf(s,"always @*\n");
   fprintf(s,"begin\n");
   fprintf(s,"%sstateRead = 32'h0;\n",TAB);
   fprintf(s,"%sif(valid & !memoryMappedAddr) begin\n",TAB);

   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nStates; ii++){
         int bitSize = decl->stateWires[ii].bitsize;

         fprintf(s,"%s%sif(addr[%d:%d] == %d'd%d)\n",TAB,TAB,val.stateAddressRange.high,val.stateAddressRange.low,val.stateAddressRange.high - val.stateAddressRange.low + 1,index++);
         fprintf(s,"%s%s%sstateRead = statedata[%d+:%d];\n",TAB,TAB,TAB,bitCount,bitSize);
         bitCount += bitSize;
      }      
   }
   fprintf(s,"%send\n",TAB);
   fprintf(s,"end\n\n");

   // Unit instantiation
   int configDataIndex = 0;
   int stateDataIndex = 0;
   int memoryMappedIndex = 0;
   int ioIndex = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      /*
      int configBits = 0;
      for(int ii = 0; ii < decl->nConfigs; ii++){
         configBits += decl->configWires[ii].bitsize;
      }

      int stateBits = 0;
      for(int ii = 0; ii < decl->nStates; ii++){
         stateBits += decl->stateWires[ii].bitsize;
      }
      */

      fprintf(s,"%s%s unit%d(\n",TAB,decl->name,i);
      
      for(int ii = 0; ii < decl->nOutputs; ii++){
         fprintf(s,"%s.out%d(output_%d_%d),\n",TAB,ii,i,ii);
      }   

      for(int ii = 0; ii < decl->nInputs; ii++){
         if(instance->inputs[ii].instance){
            fprintf(s,"%s.in%d(output_%d_%d),\n",TAB,ii,(int) (instance->inputs[ii].instance - accel->instances),instance->inputs[ii].index);
         } else {
            fprintf(s,"%s.in%d(0),\n",TAB,ii);
         }
      }

      // Config data
      //if(decl->nConfigs){
      //   fprintf(s,"%s.configdata(configdata[%d:%d]),\n",TAB,configDataIndex + configBits - 1,configDataIndex);
      //}

      for(int ii = 0; ii < decl->nConfigs; ii++){
         Wire wire = decl->configWires[ii];
         fprintf(s,"%s.%s(configdata[%d:%d]),\n",TAB,wire.name,configDataIndex + wire.bitsize - 1,configDataIndex);
         configDataIndex += wire.bitsize;
      }
      
      for(int ii = 0; ii < decl->nStates; ii++){
         Wire wire = decl->stateWires[ii];
         fprintf(s,"%s.%s(statedata[%d:%d]),\n",TAB,wire.name,stateDataIndex + wire.bitsize - 1,stateDataIndex);
         stateDataIndex += wire.bitsize;
      }

      // State
      //if(decl->nStates){
      //   fprintf(s,"%s.statedata(statedata[%d:%d]),\n",TAB,stateDataIndex + stateBits - 1,stateDataIndex);
      //}

      if(decl->doesIO){
         fprintf(s,"%s.databus_ready(m_databus_ready[%d]),\n",TAB,ioIndex);
         fprintf(s,"%s.databus_valid(m_databus_valid[%d]),\n",TAB,ioIndex);
         fprintf(s,"%s.databus_addr(m_databus_addr[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_rdata(m_databus_rdata[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_wdata(m_databus_wdata[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_wstrb(m_databus_wstrb[%d+:4]),\n",TAB,ioIndex * 4);
         ioIndex += 1;
      }

      // Memory mapped
      if(decl->memoryMapBytes){
         fprintf(s,"%s.valid(memoryMappedEnable[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.wstrb(wstrb),\n",TAB);
         fprintf(s,"%s.addr(addr[%d:0]),\n",TAB,log2i(decl->memoryMapBytes / 4) - 1);
         fprintf(s,"%s.rdata(unitRData[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.ready(unitReady[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.wdata(wdata),\n\n",TAB);
         memoryMappedIndex += 1;
      }
      fprintf(s,"%s.run(run),\n",TAB);
      fprintf(s,"%s.done(unitDone[%d]),\n",TAB,i);
      fprintf(s,"%s.clk(clk),\n",TAB);
      fprintf(s,"%s.rst(rst_int)\n",TAB);
      fprintf(s,"%s);\n\n",TAB);
   }

   fprintf(s,"endmodule");

   fclose(s);
   fclose(d);
}

#define MAX_CHARS 64

bool IsPrefix(char* prefix,char* str){
   for(int i = 0; prefix[i] != '\0'; i++){
      if(prefix[i] != str[i])
         return false;
   }

   return true;
}

#define EMPTY_STATE 0
#define OUTPUT_STATE 1

#define RESET_BUFFER strcpy(lastAction,actionBuffer); actionBuffer[0] = '\0'; actionIndex = 0;
void IterativeAcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction){
   char lastAction[MAX_CHARS];
   char actionBuffer[MAX_CHARS];
   int actionIndex = 0;
   char* errorMsg = NULL;
   char buffer[128];

   lastAction[0] = '\0';
   buffer[0] = '\0';
   RESET_BUFFER;

   int state;
   FUInstance* inst;

   AcceleratorRunStart(versat,accel);

   WINDOW* window = initscr();
   cbreak();
   noecho();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);
   
   halfdelay(2);

   state = 0;
   int steps = 0;
   int lastValidRes = 0;
   while(true){
      int maxX,maxY;
      getmaxyx(window,maxY,maxX);

      int res = getch();

      // Execute command
      if(res == '\n'){
         if(IsPrefix("step",actionBuffer)){
            int cycles;

            int res = sscanf(actionBuffer,"step %d",&cycles);
            if(res == EOF){
               cycles = 1;
            }

            for(int i = 0; i < cycles; i++){
               AcceleratorRunIteration(versat,accel);
               if(terminateFunction(endRoot))
                  break;
            }
            steps += cycles;
            RESET_BUFFER;
         }
         if(IsPrefix("output",actionBuffer)){
            int index;
            sscanf(actionBuffer,"output %d",&index);

            if(index < accel->nInstances){
               inst = &accel->instances[index];
               state = OUTPUT_STATE;
            } else {
               errorMsg = "Index provided is bigger than number of instances that exist";
            }
            RESET_BUFFER;
         }
         if(IsPrefix("exit",actionBuffer)){
            break;
         }
      } else if(res == KEY_BACKSPACE){
         if(actionIndex > 0)
            actionBuffer[--actionIndex] = '\0';
      } else if(res == KEY_UP){
         strcpy(actionBuffer,lastAction);
         actionIndex = strlen(actionBuffer);
      } else if(res != ERR && ((res >= '0' && res <= 'z') || res == ' ') && actionIndex < (MAX_CHARS - 1)){
         actionBuffer[actionIndex++] = res;
         actionBuffer[actionIndex] = '\0';
      }

      clear();

      if(errorMsg)
         mvaddstr((maxY-2),0,errorMsg);

      switch(state){
         case EMPTY_STATE:{

         } break;
         case OUTPUT_STATE:{
            FUDeclaration* decl = GetDeclaration(versat,inst);

            int yCoord = 2;
            mvaddstr(yCoord,0,decl->name);

            yCoord += 2;
            mvaddstr(yCoord,0,"inputs:");

            for(int i = 0; i < decl->nInputs; i++){
               sprintf(buffer,"%x",GetInputValue(inst,i));
               mvaddstr(yCoord,0,buffer);
               yCoord += 1;
            }

            yCoord += 1;
            mvaddstr(yCoord++,0,"outputs:");

            for(int i = 0; i < decl->nOutputs; i++){
               sprintf(buffer,"%x",inst->outputs[i]);
               mvaddstr(yCoord,0,buffer);
               yCoord += 1;
            }


            yCoord += 1;
            mvaddstr(yCoord++,0,"extraData:");

            for(int i = 0; i < (decl->extraDataSize / 4); i++){
               sprintf(buffer,"%x", ((int*) inst->extraData)[i]);
               mvaddstr(yCoord,0,buffer);
               yCoord += 1;
            }            
         } break;
      }

      sprintf(buffer,"Steps: %d",steps);
      mvaddstr(0,0,buffer);

      // Helping debug, temporary for now
      {
      if(res != ERR)
         lastValidRes = res;
      sprintf(buffer,"%x",lastValidRes);
      mvaddstr((maxY-3),0,buffer);
      }

      mvaddstr((maxY-1),0,actionBuffer);

      refresh();
   }

   endwin();
}
