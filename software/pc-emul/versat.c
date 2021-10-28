#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "assert.h"

#include <ncurses.h>

#include "../versat.h"
#include "versat_instance_template.h"

static int versat_base;

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat,int base){
	versat->accelerators = (Accelerator*) malloc(10 * sizeof(Accelerator));
	versat->declarations = (FUDeclaration*) malloc(10 * sizeof(FUDeclaration));
   versat_base = base;
}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int nInputs,int nOutputs,int nConfigs,const int const* configBits,int nStates,const int const* stateBits,int memoryMapBytes,bool doesIO,int extraDataSize,FUFunction startFunction,FUFunction updateFunction){
	FUDeclaration decl = {};
	FU_Type type = {};

   decl.nInputs = nInputs;
   decl.nOutputs = nOutputs;
	decl.name = declarationName;
	decl.nStates = nStates;
   decl.stateBits = stateBits;
   decl.nConfigs = nConfigs;
   decl.configBits = configBits;
   decl.memoryMapBytes = memoryMapBytes;
   decl.doesIO = doesIO;
   decl.extraDataSize = extraDataSize;
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
   if(decl.stateBits)
		instance.state = (volatile int*) malloc(decl.nStates * sizeof(int));
   if(decl.configBits)
      instance.config = (volatile int*) malloc(decl.nConfigs * sizeof(int));
   if(decl.memoryMapBytes)
      instance.memMapped = (volatile int*) malloc(decl.memoryMapBytes);
   if(decl.extraDataSize)
      instance.extraData = calloc(decl.extraDataSize,sizeof(int));

	accel->instances[accel->nInstances] = instance;

	return &accel->instances[accel->nInstances++];
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

static int log2i(int value){
   int res = 0;
   while(value != 1){
      value /= 2;
      res++;
   }

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
} VersatComputedValues;

static VersatComputedValues ComputeVersatValues(Versat* versat){
   VersatComputedValues res = {};

   // Only dealing with one accelerator for now
   Accelerator* accel = &versat->accelerators[0];

   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];
      FUDeclaration* decl = GetDeclaration(versat,instance);

      res.memoryMapped += decl->memoryMapBytes;
      if(decl->memoryMapBytes)
         res.unitsMapped += 1;

      res.nConfigs += decl->nConfigs;

      if(decl->nConfigs){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            res.configurationBits += decl->configBits[ii];
         }
      }

      res.nStates += decl->nStates;
      if(decl->nStates){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            res.stateBits += decl->stateBits[ii];
         }      
      }

      if(decl->doesIO)
         res.nUnitsIO += 1;
   }

   res.lowerAddressSize = log2i(maxi(res.nStates,maxi(res.memoryMapped,res.nConfigs)));
   res.addressSize = res.lowerAddressSize + 1; // One bit to select between memory or config/state

   return res;
}

void OutputMemoryMap(Versat* versat){
   VersatComputedValues val = ComputeVersatValues(versat);

   printf("Memory mapped: %d\n",val.memoryMapped);
   printf("Units mapped: %d\n",val.unitsMapped);
   printf("Configuration registers: %d\n",val.nConfigs);
   printf("Configuration bits needed: %d\n",val.configurationBits);
   printf("State registers: %d\n",val.nStates);
   printf("State bits needed: %d\n",val.stateBits);
   printf("Number units: %d\n",versat->accelerators[0].nInstances);

   printf("\n");
   printf("Mem/Config bit size:%d\n",val.lowerAddressSize);
   printf("Unit Selection bit size:%d\n",val.addressSize);
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

   int memoryMapped = 0;
   int nUnitsMapped = 0;

   int nConfigs = 0;
   int configurationBits = 0;
   int nStates = 0;
   int stateBits = 0;

   int nUnitsIO = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];   

      memoryMapped += decl->memoryMapBytes;
      if(decl->memoryMapBytes)
         nUnitsMapped += 1;

      nConfigs += decl->nConfigs;

      if(decl->nConfigs){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            configurationBits += decl->configBits[ii];
         }
      }

      nStates += decl->nStates;
      if(decl->nStates){
         for(int ii = 0; ii < decl->nConfigs; ii++){
            stateBits += decl->stateBits[ii];
         }      
      }

      if(decl->doesIO)
         nUnitsIO += 1;
   }

   int configBits = 0;
   int nIOS = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nConfigs; ii++){
         configBits += decl->configBits[ii];
      }
      nIOS += decl->doesIO;
   }

   fprintf(d,"`define CONFIG_W %d\n",configBits);
   if(nIOS)
      fprintf(d,"`define IO\n");
   fprintf(d,"`define nIO %d\n",nIOS);

   fprintf(s,versat_instance_template_str);

   fprintf(s,"reg [`CONFIG_W-1:0] configdata;\n");
   fprintf(s,"wire [%d:0] statedata;\n",stateBits-1);
   fprintf(s,"wire [31:0] unused");
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nInputs; ii++){
         fprintf(s,",output_%d_%d",i,ii);
      }
   }
   fprintf(s,";\n\n");

   fprintf(s,"wire [31:0] unused2");
   for(int i = 0; i < nUnitsMapped; i++){
      fprintf(s,",rdata_%d",i);
   }
   fprintf(s,";\n\n");
   
   // Memory mapping
   fprintf(s,"reg [%d:0] memoryMappedEnable,unitDone;\n",nUnitsMapped - 1);
   fprintf(s,"wire[%d:0] unitReady;\n",nUnitsMapped - 1);
   fprintf(s,"wire [31:0] unitRData[%d:0];\n\n",nUnitsMapped - 1);
   
   fprintf(s,"assign wor_ready = (|unitReady);\n\n");
   fprintf(s,"assign done = &unitDone;\n\n");

   fprintf(s,"assign unitRdataFinal = (unitRData[0]");

   for(int i = 1; i < nUnitsMapped; i++){
      fprintf(s," | unitRData[%d]",i);
   }
   fprintf(s,");\n\n");

   fprintf(s,"always @*\n");
   fprintf(s,"begin\n");
   fprintf(s,"%smemoryMappedEnable = {%d{1'b0}};\n",TAB,nUnitsMapped - 1);

   for(int i = 0; i < nUnitsMapped; i++){
      fprintf(s,"%sif(valid & addr[14] & addr[13:10] == 4'd%d)\n",TAB,i);
      fprintf(s,"%s%smemoryMappedEnable[%d] = 1'b1;\n",TAB,TAB,i);
   }
   fprintf(s,"end\n\n");

   // Config data decoding
   fprintf(s,"always @(posedge clk,posedge rst_int)\n");
   fprintf(s,"%sif(rst_int) begin\n",TAB);
   fprintf(s,"%s%sconfigdata <= {`CONFIG_W{1'b0}};\n",TAB,TAB);
   fprintf(s,"%send else if(valid & we & !addr[14]) begin\n",TAB);
   
   int index = 0;
   int bitCount = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nConfigs; ii++){
         int bitSize = decl->configBits[ii];;

         fprintf(s,"%s%sif(addr[13:0] == 16'd%d)\n",TAB,TAB,index++);
         fprintf(s,"%s%s%sconfigdata[%d+:%d] <= wdata[%d:0];\n",TAB,TAB,TAB,bitCount,bitSize,bitSize-1);
         bitCount += bitSize;
      }      
   }
   fprintf(s,"%send\n\n",TAB);

   // State data decoding
   index = 0;
   bitCount = 0;
   fprintf(s,"always @*\n");
   fprintf(s,"begin\n");
   fprintf(s,"%sstateRead = 32'h0;\n",TAB);
   fprintf(s,"%sif(valid & !addr[14]) begin\n",TAB);

   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      for(int ii = 0; ii < decl->nStates; ii++){
         int bitSize = decl->stateBits[ii];

         fprintf(s,"%s%sif(addr[13:0] == 16'd%d)\n",TAB,TAB,index++);
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
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      int configBits = 0;
      for(int ii = 0; ii < decl->nConfigs; ii++){
         configBits += decl->configBits[ii];
      }

      int stateBits = 0;
      for(int ii = 0; ii < decl->nStates; ii++){
         stateBits += decl->stateBits[ii];
      }

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
      if(decl->configBits){
         fprintf(s,"%s.configdata(configdata[%d:%d]),\n",TAB,configDataIndex + configBits - 1,configDataIndex);
      }
      
      // State
      if(decl->stateBits){
         fprintf(s,"%s.statedata(statedata[%d:%d]),\n",TAB,stateDataIndex + stateBits - 1,stateDataIndex);
      }

      // Memory mapped
      if(decl->memoryMapBytes){
         fprintf(s,"%s.done(unitDone[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.valid(memoryMappedEnable[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.we(we),\n",TAB);
         fprintf(s,"%s.addr(addr[%d:0]),\n",TAB,log2i(decl->memoryMapBytes / 4 - 1));
         fprintf(s,"%s.rdata(unitRData[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.ready(unitReady[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.wdata(wdata),\n\n",TAB);
         memoryMappedIndex += 1;
      }
      fprintf(s,"%s.run(run),\n",TAB);
      fprintf(s,"%s.clk(clk),\n",TAB);
      fprintf(s,"%s.rst(rst_int)\n",TAB);
      fprintf(s,"%s);\n\n",TAB);
   
      configDataIndex += configBits;
      stateDataIndex += stateBits;
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
