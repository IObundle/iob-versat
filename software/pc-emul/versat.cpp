#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../versat.hpp"

extern "C"{
#include "template_engine.h"
//#include "versat_instance_template.h"
}

#define DELAY_BIT_SIZE 8

static int versat_base;

typedef struct Range_t{
   int high,low;
} Range;

bool IsAlpha(char ch){
   if(ch >= 'a' && ch < 'z')
      return true;

   if(ch >= 'A' && ch <= 'Z')
      return true;

   if(ch >= '0' && ch <= '9')
      return true;

   return false;
}

// Some names might not work as identifiers (special the way we implement parameters)
// This creates a id able name, care that memory is allocated statically
static char* FixEntityName(const char* name){
   static char buffer[128];

   int index;
   for(index = 0; IsAlpha(name[index]); index++){
      buffer[index] = name[index];
   }
   buffer[index] = '\0';

   return buffer;
}

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat,int base,int numberConfigurations){
   versat->declarations = new FUDeclaration[100];
   versat->numberConfigurations = numberConfigurations;
   versat_base = base;

   versat->declarations[0] = {};

   FUDeclaration nullDeclaration = {};
   RegisterFU(versat,nullDeclaration);
}

static int32_t AccessMemory(FUInstance* instance,int address, int value, int write){
   FUDeclaration decl = *instance->declaration;
   int32_t res = decl.memAccessFunction(instance,address,value,write);

   return res;
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   AccessMemory(instance,address,value,1);
}

int32_t VersatUnitRead(FUInstance* instance,int address){
   int32_t res = AccessMemory(instance,address,0,0);
   return res;
}

FUDeclaration* GetTypeByName(Versat* versat,const char* name){
   for(int i = 0; i < versat->nDeclarations; i++){
      if(strcmp(versat->declarations[i].name.str,name) == 0){
         FUDeclaration* type = &versat->declarations[i];
         return type;
      }
   }

   Assert(0);
   return NULL;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){


#if MARK
   va_list args;
   va_start(args,argc);

   Accelerator* ptr = circuit;
   FUInstance* res = NULL;

   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);
      int found = 0;

      ForEach(FUInstance,inst,ptr->instances){
         if(strcmp(inst->name.str,str) == 0){
            res = inst;
            found = 1;

            break;
         }
      }

      Assert(found);

      if(res->declaration->type == COMPOSITE){
         ptr = res->compositeAccel;
      }
   }

   va_end(args);
#endif
   return nullptr;
   //return res;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   printf("%d\n",sizeof(Versat));

   FUDeclaration* type = &versat->declarations[versat->nDeclarations];
   versat->declarations[versat->nDeclarations++] = decl;

   return type;
}

void OutputGraphDotFile(Accelerator* accel,FILE* outputFile,int collapseSameEdges){
   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
#if MARK
   ForEach(FUInstance,inst,accel->instances){
      fprintf(outputFile,"\t%s_%d;\n",inst->name.str,inst->id);
   }

   HashMap map = {};
   map.dataSize = sizeof(4);
   map.keySize = 2 * sizeof(FUInstance*);

   ForEach(Edge,edge,accel->edges){
      if(collapseSameEdges){
         struct {FUInstance* insts[2];} key;

         key.insts[0] = edge->units[0].inst;
         key.insts[1] = edge->units[1].inst;

         void* item = HashMapGetItem(map,&key,void*);

         if(item != NULL){
            continue;
         }

         HashMapAddItem(&map,&key,&key);
      }

      fprintf(outputFile,"\t%s_%d -> ",edge->units[0].inst->name.str,edge->units[0].inst->id);
      fprintf(outputFile,"%s_%d;\n",edge->units[1].inst->name.str,edge->units[1].inst->id);
   }
   HashMapFree(&map);
#endif

   fprintf(outputFile,"}\n");
}

Accelerator* CreateAccelerator(Versat* versat){
#if MARK
   Accelerator* accel = Alloc(versat->accelerators,Accelerator);

   accel->versat = versat;

   return accel;
#endif
   return nullptr;
}

static Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,std::unordered_map<FUInstance*,FUInstance*>* map){
   Accelerator* newAccel = CreateAccelerator(versat);
#if MARK
   map->keySize = sizeof(FUInstance*);
   map->dataSize = sizeof(FUInstance*);

   // Flat copy of instances
   ForEach(FUInstance,inst,accel->instances){
      FUInstance* newInst = CreateFUInstance(newAccel,inst->declaration);
      strcpy(newInst->name.str,inst->name.str);

      HashMapAddItem(map,&inst,&newInst);
   }

   // Flat copy of edges
   ForEach(Edge,edge,accel->edges){
      Edge* newEdge = Alloc(newAccel->edges,Edge);

      *newEdge = *edge;
      newEdge->units[0].inst = *HashMapGetItem(*map,&edge->units[0].inst,FUInstance*);
      newEdge->units[1].inst = *HashMapGetItem(*map,&edge->units[1].inst,FUInstance*);
   }
#endif

   return newAccel;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type){
#if MARK
   static int entityIndex = 0;

   FUInstance instance = {};
   FUDeclaration decl = *type;

   instance.id = entityIndex++;
   instance.accel = accel;
   instance.declaration = type;

   sprintf(instance.name.str,"%.15s",FixEntityName(decl.name.str));

   if(decl.nOutputs){
      instance.outputs = (int32_t*) calloc(decl.nOutputs,sizeof(int32_t));
      instance.storedOutputs = (int32_t*) calloc(decl.nOutputs,sizeof(int32_t));
   }
   if(decl.nStates)
      instance.state = (volatile int*) calloc(decl.nStates,sizeof(int));
   if(decl.nConfigs)
      instance.config = (volatile int*) calloc(decl.nConfigs,sizeof(int));
   if(decl.extraDataSize)
      instance.extraData = calloc(decl.extraDataSize,sizeof(char));

   FUInstance* ptr = Alloc(accel->instances,FUInstance);
   *ptr = instance;

   if(decl.type == COMPOSITE){
      HashMap copyMap = {};
      instance.compositeAccel = CopyAccelerator(accel->versat,decl.circuit,&copyMap);
      HashMapFree(&copyMap);
   }

   if(decl.initializeFunction)
      decl.initializeFunction(ptr);

   return ptr;
#endif
   return nullptr;
}

FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,const char* entityName,HierarchyName* hierarchyParent){
#if MARK
   FUInstance* inst = CreateFUInstance(accel,type);

   FixedStringCpy(inst->name.str,entityName,MAX_NAME_SIZE);
   inst->name.parent = hierarchyParent;

   return inst;
#endif
   return nullptr;
}

void RemoveFUInstance(Accelerator* accel,FUInstance* inst){
#if MARK
   PoolRemoveElement(&accel->instances,inst);

   ForEach(Edge,edge,accel->edges){
      if(edge->units[0].inst == inst){
         PoolRemoveElement(&accel->edges,edge);
      } else if(edge->units[1].inst == inst){
         PoolRemoveElement(&accel->edges,edge);
      }
   }
#endif
}

static bool IsGraphValid(Accelerator* accel){
#if MARK
   static HashMap map = {};
   map.keySize = sizeof(FUInstance*);
   map.dataSize = sizeof(void*);

   HashMapClear(&map);

   ForEach(FUInstance,inst,instances){
      inst->tag = 0;
      HashMapAddItem(&map,&inst,&inst);
   }

   ForEach(Edge,edge,edges){
      for(int i = 0; i < 2; i++){
         void* ptr = HashMapGetItem(map,&edge->units[i].inst,void*);
         edge->units[i].inst->tag = 1;

         if(ptr == NULL){
            return 0;
         }
      }
   }

   ForEach(FUInstance,inst,instances){
      if(inst->tag != 1){
         return 0;
      }
   }
#endif
   return 1;
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
#if MARK
   HashMap copyMap = {};
   Accelerator* newAccel = CopyAccelerator(versat,accel,&copyMap);
   HashMapFree(&copyMap);

   PortInstance outputs[60];
   for(int i = 0; i < 60; i++){
      outputs[i] = (PortInstance){};
   }

   int count = 0;
   HashMap map = {};
   Pool compositeInstances = {};
   for(int i = 0; i < times; i++){
      ForEach(FUInstance,inst,newAccel->instances){
         if(inst->declaration->type == COMPOSITE){
            FUInstance** ptr = Alloc(compositeInstances,FUInstance*);
            *ptr = inst;
         }
      }

      ForEach(FUInstance*,instPtr,compositeInstances){
         FUInstance* inst = *instPtr;

         count += 1;

         Assert(inst->declaration->type == COMPOSITE);

         map.keySize = sizeof(FUInstance*);
         map.dataSize = sizeof(FUInstance*);

         Accelerator* circuit = inst->declaration->circuit;

         // Create new instance and map then
         #if 1
         ForEach(FUInstance,circuitInst,circuit->instances){
            if(circuitInst->declaration->type == SPECIAL){
               continue;
            }

            FUInstance* newInst = CreateNamedFUInstance(newAccel,circuitInst->declaration,circuitInst->name.str,circuitInst->name.parent);

            HashMapAddItem(&map,&circuitInst,&newInst);

            void* ptr = *HashMapGetItem(map,&circuitInst,void*);
            Assert(ptr == newInst);
         }
         #endif

         #if 1
         // Add accel edges to output instances
         ForEach(Edge,edge,newAccel->edges){
            if(edge->units[0].inst == inst){
               ForEach(Edge,circuitEdge,circuit->edges){
                  if(circuitEdge->units[1].inst == circuit->outputInstance && circuitEdge->units[1].port == edge->units[0].port){
                     Edge* newEdge = Alloc(newAccel->edges,Edge);

                     FUInstance* mappedInst = *HashMapGetItem(map,&circuitEdge->units[0].inst,FUInstance*);

                     *newEdge = *edge;
                     newEdge->units[0].inst = mappedInst;
                     newEdge->units[0].port = edge->units[0].port;
                  }
               }
            }
         }
         #endif

         #if 1
         // Add accel edges to input instances
         ForEach(Edge,edge,newAccel->edges){
            if(edge->units[1].inst == inst){
               FUInstance* circuitInst = *PoolGet(circuit->inputInstancePointers,edge->units[1].port,FUInstance*);

               ForEach(Edge,circuitEdge,circuit->edges){
                  if(circuitEdge->units[0].inst == circuitInst){
                     Edge* newEdge = Alloc(newAccel->edges,Edge);

                     FUInstance* mappedInst = *HashMapGetItem(map,&circuitEdge->units[1].inst,FUInstance*);

                     *newEdge = *edge;
                     newEdge->units[1].inst = mappedInst;
                     newEdge->units[1].port = circuitEdge->units[1].port;
                  }
               }
            }
         }
         #endif

         #if 1
         // Add circuit specific edges
         ForEach(Edge,circuitEdge,circuit->edges){
            FUInstance** input = HashMapGetItem(map,&circuitEdge->units[0].inst,FUInstance*);
            FUInstance** output = HashMapGetItem(map,&circuitEdge->units[1].inst,FUInstance*);

            if(!input || !output){
               continue;
            }

            Edge* newEdge = Alloc(newAccel->edges,Edge);

            *newEdge = *circuitEdge;
            newEdge->units[0].inst = *input;
            newEdge->units[1].inst = *output;
         }
         #endif

         RemoveFUInstance(newAccel,inst);
         HashMapClear(&map);

         if(count == 2){
            printf("here\n");
            //return newAccel;
         }

         #if 0
         Assert(IsGraphValid(newAccel));
         #else
         if(!IsGraphValid(newAccel)){
            return newAccel;
         }
         #endif
      }

      PoolClear(&compositeInstances);
   }

   PoolFree(&compositeInstances);
   HashMapFree(&map);

   return newAccel;
#endif

   return nullptr;
}

static void AcceleratorRunStart(Accelerator* accel){
#if MARK
   ForEach(FUInstance,inst,accel->instances){
      FUDeclaration decl = *inst->declaration;
      FUFunction startFunction = decl.startFunction;

      if(startFunction){
         int32_t* startingOutputs = startFunction(inst);

         if(startingOutputs)
            memcpy(inst->outputs,startingOutputs,decl.nOutputs * sizeof(int32_t));
      }
   }
#endif
}

static void AcceleratorRunIteration(DAGOrder order,int numberInstances){
   for(int i = 0; i < numberInstances; i++){
      FUInstance* inst = order.instancePtrs[i];

      FUDeclaration decl = *inst->declaration;
      int32_t* newOutputs = decl.updateFunction(inst);
      memcpy(inst->storedOutputs,newOutputs,decl.nOutputs * sizeof(int32_t));
   }

   for(int i = 0; i < numberInstances; i++){
      FUInstance* inst = order.instancePtrs[i];

      FUDeclaration decl = *inst->declaration;
      memcpy(inst->outputs,inst->storedOutputs,decl.nOutputs * sizeof(int32_t));
   }
}

void LoadConfiguration(Accelerator* accel,int configuration){
   // Implements the reverse of Save Configuration
}

void SaveConfiguration(Accelerator* accel,int configuration){
   //Assert(configuration < accel->versat->numberConfigurations);
}

void AcceleratorRun(Accelerator* accel){
#if MARK
   AcceleratorRunStart(accel);

   DAGOrder order = CalculateDAGOrdering(accel);

   int cycle = 0;
   while(true){
      AcceleratorRunIteration(order,accel->instances.allocated);

      bool done = true;
      ForEach(FUInstance,inst,accel->instances){
         if(inst->declaration->delayType & DELAY_TYPE_IMPLEMENTS_DONE){
            if(!inst->done){
               done = false;
               break;
            }
         }
      }

      cycle += 1;

      if(done){
         break;
      }
   }
#endif
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
#if MARK
   // Only dealing with one accelerator for now
   Accelerator* accel = PoolGetValid(versat->accelerators,0,Accelerator);

   ForEach(FUInstance,inst,accel->instances){
      UnitInfo info = CalculateUnitInfo(inst);

      res.maxMemoryMapBytes = maxi(res.maxMemoryMapBytes,info.memoryMappedBytes);
      res.memoryMapped += info.memoryMappedBytes;

      if(info.memoryMappedBytes)
         res.unitsMapped += 1;

      res.nConfigs += info.nConfigsWithDelay;
      res.configurationBits += info.configBitSize;
      res.nStates += info.nStates;
      res.stateBits += info.stateBitSize;
      res.nUnitsIO += info.doesIO;
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
#endif
   return res;
}

void OutputMemoryMap(Versat* versat){
#if MARK
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
   printf("Number units: %d\n",PoolGetValid(versat->accelerators,0,Accelerator)->instances.allocated);
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
#endif
}

int32_t GetInputValue(FUInstance* instance,int index){
#if MARK
   Accelerator* accel = instance->accel;

   ForEach(Edge,edge,accel->edges){
      if(edge->units[1].inst == instance && edge->units[1].port == index){
         return edge->units[0].inst->outputs[edge->units[0].port];
      }
   }
#endif
   return 0;
}

// Connects out -> in
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex){
#if MARK
   FUDeclaration inDecl = *in->declaration;
   FUDeclaration outDecl = *out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl.nInputs);
   Assert(outIndex < outDecl.nOutputs);

   Accelerator* accel = out->accel;

   Edge* edge = Alloc(accel->edges,Edge);

   edge->units[0].inst = out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = in;
   edge->units[1].port = inIndex;
#endif
}

#define TAB "   "

static void OutputConfigurationFile(Versat* versat,FILE* f){

}

void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath,const char* configurationFilepath){
#if MARK
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

   FILE* c = fopen(configurationFilepath,"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",configurationFilepath);
      fclose(d);
      fclose(s);
      return;
   }
   // Only dealing with 1 accelerator, for now
   Accelerator* accel = PoolGetValid(versat->accelerators,0,Accelerator);

   VersatComputedValues val = ComputeVersatValues(versat);

   // Output configuration file
   TemplateSetNumber("nInstances",accel->instances.allocated);

   #if 0
   /*
   int* nOutputs = (int*) malloc(sizeof(int) * accel->instances.allocated);
   ForEach(FUInstance,inst,accel->instances){
      nOutputs[i] = inst->declaration->nOutputs;
   }
   */

   int* bitArray = (int*) malloc(sizeof(int) * val.configurationBits);
   int index = 0;
   ForEach(FUInstance,inst,accel->instances){
      FUDeclaration decl = *inst->declaration;

      int delays = UnitDelays(inst);
      if(delays){
         for(int ii = 0; ii < delays; ii++){
            bitArray[index++] = DELAY_BIT_SIZE;
         }
      }

      for(int ii = 0; ii < decl.nConfigs; ii++){
         bitArray[index++] = decl.configWires[ii].bitsize;
      }
   }

   TemplateSetArray("configurationBitArray",bitArray,index);
   TemplateSetNumber("configurationBitSize",index);

   int* stateBits = (int*) malloc(sizeof(int) * val.nStates);
   index = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];
      FUDeclaration decl = *instance->declaration;

      for(int ii = 0; ii < decl.nStates; ii++){
         stateBits[index++] = decl.stateWires[ii].bitsize;
      }
   }

   TemplateSetArray("stateBitArray",stateBits,index);
   TemplateSetNumber("stateBitSize",index);

   TemplateSetArray("nOutputs",nOutputs,accel->nInstances);

   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryMappedUnitAddressRangeHigh",val.memoryMappedUnitAddressRange.high);
   TemplateSetNumber("memoryMappedUnitAddressRangeLow",val.memoryMappedUnitAddressRange.low);
   TemplateSetNumber("configurationBits",val.configurationBits);

   TemplateSetNumber("configAddressRangeHigh",val.configAddressRange.high);
   TemplateSetNumber("configAddressRangeLow",val.configAddressRange.low);
   TemplateSetNumber("stateAddressRangeHigh",val.stateAddressRange.high);
   TemplateSetNumber("stateAddressRangeLow",val.stateAddressRange.low);

   ProcessTemplate("versat_instance_template_copy.v","templating.v");

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

      FUDeclaration decl = *instance->declaration;

      for(int ii = 0; ii < decl.nOutputs; ii++){
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

   index = 1; // 0 is reserved for versat registers
   int bitCount = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FUDeclaration decl = *instance->declaration;

      int delays = UnitDelays(instance);
      if(delays){
         for(int ii = 0; ii < delays; ii++){
            int bitSize = DELAY_BIT_SIZE;

            fprintf(s,"%s%sif(addr[%d:%d] == %d'd%d)\n",TAB,TAB,val.configAddressRange.high,val.configAddressRange.low,val.configAddressRange.high - val.configAddressRange.low + 1,index++);
            fprintf(s,"%s%s%sconfigdata[%d+:%d] <= wdata[%d:0]; // %s_%d delay%d\n",TAB,TAB,TAB,bitCount,bitSize,bitSize-1,decl.name,i,ii);
            bitCount += bitSize;
         }
      }

      for(int ii = 0; ii < decl.nConfigs; ii++){
         int bitSize = decl.configWires[ii].bitsize;

         fprintf(s,"%s%sif(addr[%d:%d] == %d'd%d)\n",TAB,TAB,val.configAddressRange.high,val.configAddressRange.low,val.configAddressRange.high - val.configAddressRange.low + 1,index++);
         fprintf(s,"%s%s%sconfigdata[%d+:%d] <= wdata[%d:0]; // %s_%d %s\n",TAB,TAB,TAB,bitCount,bitSize,bitSize-1,decl.name,i,decl.configWires[ii].name);
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

      FUDeclaration decl = *instance->declaration;

      for(int ii = 0; ii < decl.nStates; ii++){
         int bitSize = decl.stateWires[ii].bitsize;

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

      FUDeclaration decl = *instance->declaration;

      // Small temporary fix to remove any unwanted character
      char* buffer = FixEntityName(decl.name);

      fprintf(s,"%s%s %s_%d(\n",TAB,decl.name,buffer,i);

      for(int ii = 0; ii < decl.nOutputs; ii++){
         fprintf(s,"%s.out%d(output_%d_%d),\n",TAB,ii,i,ii);
      }

      for(int ii = 0; ii < decl.nInputs; ii++){
         if(instance->inputs[ii].instance){
            fprintf(s,"%s.in%d(output_%d_%d),\n",TAB,ii,(int) (instance->inputs[ii].instance - accel->instances),instance->inputs[ii].index);
         } else {
            fprintf(s,"%s.in%d(0),\n",TAB,ii);
         }
      }

      // Delays act as config data but handled entirely by versat
      // They are defined as the first bytes of the unit config
      int delays = UnitDelays(instance);
      if(delays){
         for(int ii = 0; ii < delays; ii++){
            fprintf(s,"%s.delay%d(configdata[%d:%d]),\n",TAB,ii,configDataIndex + DELAY_BIT_SIZE - 1,configDataIndex);
            configDataIndex += DELAY_BIT_SIZE;
         }
      }

      for(int ii = 0; ii < decl.nConfigs; ii++){
         Wire wire = decl.configWires[ii];
         fprintf(s,"%s.%s(configdata[%d:%d]),\n",TAB,wire.name,configDataIndex + wire.bitsize - 1,configDataIndex);
         configDataIndex += wire.bitsize;
      }

      for(int ii = 0; ii < decl.nStates; ii++){
         Wire wire = decl.stateWires[ii];
         fprintf(s,"%s.%s(statedata[%d:%d]),\n",TAB,wire.name,stateDataIndex + wire.bitsize - 1,stateDataIndex);
         stateDataIndex += wire.bitsize;
      }

      if(decl.doesIO){
         fprintf(s,"%s.databus_ready(m_databus_ready[%d]),\n",TAB,ioIndex);
         fprintf(s,"%s.databus_valid(m_databus_valid[%d]),\n",TAB,ioIndex);
         fprintf(s,"%s.databus_addr(m_databus_addr[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_rdata(m_databus_rdata[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_wdata(m_databus_wdata[%d+:32]),\n",TAB,ioIndex * 32);
         fprintf(s,"%s.databus_wstrb(m_databus_wstrb[%d+:4]),\n",TAB,ioIndex * 4);
         ioIndex += 1;
      }

      // Memory mapped
      if(decl.memoryMapBytes){
         fprintf(s,"%s.valid(memoryMappedEnable[%d]),\n",TAB,memoryMappedIndex);
         fprintf(s,"%s.wstrb(wstrb),\n",TAB);
         fprintf(s,"%s.addr(addr[%d:0]),\n",TAB,log2i(decl.memoryMapBytes / 4) - 1);
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

   #endif

   fprintf(s,"endmodule");

   fclose(c);
   fclose(s);
   fclose(d);
#endif
}

#define MAX_CHARS 64

#define CHECK_TYPE(inst,T) ((inst->declaration->delayType & T) == T)

#define TAG_UNCONNECTED 0
#define TAG_SOURCE      1
#define TAG_SINK        2
#define TAG_COMPUTE     3
#define TAG_SOURCE_AND_SINK 4

void* CalculateGraphData(Accelerator* accel){
#if MARK
   int memoryNeeded = sizeof(GraphComputedData) * accel->instances.allocated + 2 * accel->edges.allocated * sizeof(ConnectionInfo);

   char* mem = (char*) calloc(sizeof(char),memoryNeeded);
   GraphComputedData* computedData = (GraphComputedData*) mem;
   ConnectionInfo* inputBuffer = (ConnectionInfo*) &computedData[accel->instances.allocated];
   ConnectionInfo* outputBuffer = &inputBuffer[accel->edges.allocated];

   // Associate computed data to each instance
   {
      int i = 0;
      ForEach(FUInstance,inst,accel->instances){
         inst->tempData = &computedData[i++];
      }
   }

   // Set inputs and outputs
   ForEach(FUInstance,inst,accel->instances){
      inst->tempData->inputs = inputBuffer;
      inst->tempData->outputs = outputBuffer;

      ForEach(Edge,edge,accel->edges){
         if(edge->units[0].inst == inst){
            outputBuffer->inst = edge->units[1];
            outputBuffer->port = edge->units[0].port;
            outputBuffer += 1;

            inst->tempData->numberOutputs += 1;
         }
         if(edge->units[1].inst == inst){
            inputBuffer->inst = edge->units[0];
            inputBuffer->port = edge->units[1].port;
            inputBuffer += 1;

            inst->tempData->numberInputs += 1;
         }
      }
   }

   ForEach(FUInstance,inst,accel->instances){
      inst->tempData->nodeType = TAG_UNCONNECTED;

      bool hasInput = (inst->tempData->numberInputs > 0);
      bool hasOutput = (inst->tempData->numberOutputs > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(CHECK_TYPE(inst,DELAY_TYPE_SINK) && CHECK_TYPE(inst,DELAY_TYPE_SOURCE)){
         if(hasInput && hasOutput){
            inst->tempData->nodeType = TAG_SOURCE_AND_SINK;
         } else if(hasInput){
            inst->tempData->nodeType = TAG_SINK;
         } else if(hasOutput){
            inst->tempData->nodeType = TAG_SOURCE;
         }
      }
      else if(CHECK_TYPE(inst,DELAY_TYPE_SINK) && hasInput){
         inst->tempData->nodeType = TAG_SINK;
      }
      else if(CHECK_TYPE(inst,DELAY_TYPE_SOURCE) && hasOutput){
         inst->tempData->nodeType = TAG_SOURCE;
      }
      else if(hasOutput && hasInput){
         inst->tempData->nodeType = TAG_COMPUTE;
      }
   }

   return mem;
#endif

   return nullptr;
}

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

static int Visit(FUInstance*** ordering,FUInstance* inst){
#if MARK
   if(inst->tag == TAG_PERMANENT_MARK){
      return 0;
   }
   if(inst->tag == TAG_TEMPORARY_MARK){
      Assert(0);
   }

   inst->tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(inst->tempData->nodeType == TAG_SINK || inst->tempData->nodeType == TAG_COMPUTE){
      for(int i = 0; i < inst->tempData->numberInputs; i++){
         count += Visit(ordering,inst->tempData->inputs[i].inst.inst);
      }
   }

   inst->tag = TAG_PERMANENT_MARK;

   if(inst->tempData->nodeType == TAG_COMPUTE){
      *(*ordering) = inst;
      (*ordering) += 1;
      count += 1;
   }

   return count;
#endif

   return 0;
}

DAGOrder CalculateDAGOrdering(Accelerator* accel){
   DAGOrder ordering = {};

#if MARK
   void* mem = CalculateGraphData(accel);

   ForEach(FUInstance,inst,accel->instances){
      inst->tag = 0;
   }

   ordering.instancePtrs = calloc(accel->instances.allocated,sizeof(FUInstance*));
   FUInstance** sourceUnits = ordering.instancePtrs;
   ordering.sources = sourceUnits;
   ForEach(FUInstance,inst,accel->instances){
      if(inst->tempData->nodeType == TAG_SOURCE){
         *(sourceUnits++) = inst;
         ordering.numberSources += 1;
      }
   }

   FUInstance** computeUnits = sourceUnits;
   ordering.computeUnits = computeUnits;
   ForEach(FUInstance,inst,accel->instances){
      if(inst->tag == 0 && inst->tempData->nodeType == TAG_COMPUTE){
         ordering.numberComputeUnits += Visit(&computeUnits,inst);
      }
   }

   FUInstance** sinkUnits = computeUnits;
   ordering.sinks = sinkUnits;
   ForEach(FUInstance,inst,accel->instances){
      if(inst->tempData->nodeType == TAG_SINK || inst->tempData->nodeType == TAG_SOURCE_AND_SINK){
         *(sinkUnits++) = inst;
         ordering.numberSinks += 1;
         Assert(inst->tag == 0);
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   free(mem);

   ForEach(FUInstance,inst,accel->instances){
      Assert(inst->tag == TAG_PERMANENT_MARK);
   }

   Assert(ordering.numberSources + ordering.numberComputeUnits + ordering.numberSinks == accel->instances.allocated);

   for(int i = 0; i < accel->instances.allocated; i++){
      Assert(ordering.instancePtrs[i]);
   }
#endif

   return ordering;
}

UnitInfo CalculateUnitInfo(FUInstance* inst){
   UnitInfo info = {};
#if MARK
   FUDeclaration decl = *inst->declaration;

   info.numberDelays = UnitDelays(inst);
   info.implementsDelay = (info.numberDelays > 0 ? true : false);
   info.nConfigsWithDelay = decl.nConfigs + info.numberDelays;

   if(decl.nConfigs){
      for(int ii = 0; ii < decl.nConfigs; ii++){
         info.configBitSize += decl.configWires[ii].bitsize;
      }
   }
   info.configBitSize += DELAY_BIT_SIZE * info.numberDelays;

   info.nStates = decl.nStates;
   if(decl.nStates){
      for(int ii = 0; ii < decl.nStates; ii++){
         info.stateBitSize += decl.stateWires[ii].bitsize;
      }
   }

   info.memoryMappedBytes = decl.memoryMapBytes;
   info.implementsDone = decl.doesIO;
   info.doesIO = decl.doesIO;

   return info;
#endif

   return info;
}

int CalculateLatency(FUInstance* inst,int sourceAndSinkAsSource){
   if(inst->tempData->nodeType == TAG_SOURCE_AND_SINK && sourceAndSinkAsSource){
      return inst->declaration->latency;
   } else if(inst->tempData->nodeType == TAG_SOURCE){
      return inst->declaration->latency;
   } else if(inst->tempData->nodeType == TAG_UNCONNECTED){
      return inst->declaration->latency;
   }

   int latency = 0;
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      latency = maxi(latency,CalculateLatency(inst->tempData->inputs[i].inst.inst,true));
   }

   return latency + inst->declaration->latency;
}

// Fixes edges such that unit before connected to after, is reconnected to new unit
void InsertUnit(Accelerator* accel, FUInstance* before, int beforePort, FUInstance* after, int afterPort, FUInstance* newUnit){
#if MARK
   ForEach(Edge,edge,accel->edges){
      if(edge->units[0].inst == before && edge->units[0].port == beforePort && edge->units[1].inst == after && edge->units[1].port == afterPort){
         ConnectUnits(newUnit,0,after,afterPort);

         edge->units[1].inst = newUnit;
         edge->units[1].port = 0;

         return;
      }
   }
#endif
}

void SendLatencyUpwards(FUInstance* inst){
#if MARK
   int val = inst->tempData->inputDelay;
   for(int ii = 0; ii < inst->tempData->numberInputs; ii++){
      ConnectionInfo* info = &inst->tempData->inputs[ii];

      FUInstance* other = inst->tempData->inputs[ii].inst.inst;
      for(int iii = 0; iii < other->tempData->numberOutputs; iii++){
         ConnectionInfo* otherInfo = &other->tempData->outputs[iii];

         if(info->inst.inst == other && info->inst.port == otherInfo->port &&
            otherInfo->inst.inst == inst && otherInfo->inst.port == info->port){
            otherInfo->delay = inst->tempData->inputDelay;
         }
      }
   }
#endif
}

void CalculateDelay(Versat* versat,Accelerator* accel){
#if MARK
   DAGOrder order = CalculateDAGOrdering(accel);

   // Calculate output of nodes
   void* outputMem = CalculateGraphData(accel);

   // Calculate latency
   #if 1
   for(int i = 0; i < order.numberSinks; i++){
      FUInstance* inst = order.sinks[i];

      int latency = 0;
      if(inst->tempData->nodeType == TAG_SOURCE_AND_SINK){
         latency = CalculateLatency(inst,false);
      } else {
         latency = CalculateLatency(inst,true);
      }

      inst->tempData->inputDelay = (latency - inst->declaration->latency);

      SendLatencyUpwards(inst);
   }
   #endif

   // Send latency upwards on DAG
   for(int i = accel->instances.allocated - order.numberSinks - 1; i >= 0; i--){
      FUInstance* inst = order.instancePtrs[i];

      int minimum = 1000000; // Should be int max

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         minimum = mini(minimum,inst->tempData->outputs[ii].delay);
      }

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         inst->tempData->outputs[ii].delay -= minimum;
      }

      inst->tempData->inputDelay = minimum - inst->declaration->latency;

      SendLatencyUpwards(inst);
   }

   #if 1
   ForEach(FUInstance,inst,accel->instances){
      printf("%s %d\n",inst->name.str,inst->tempData->inputDelay);
      #if 0
      for(int i = 0; i < inst->tempData->numberOutputs; i++){
         printf(">%d\n",inst->tempData->outputs[i].delay);
      }
      #endif
   }
   #endif

   FUDeclaration* delay = GetTypeByName(versat,"delay");

   #if 1
   // Insert delay units if needed
   for(int i = accel->instances.allocated - 1; i >= 0; i--){
      FUInstance* inst = order.instancePtrs[i];

      //For now, I think it only makes sense for computing and source units to add any sort of delay
      if(inst->tempData->nodeType != TAG_COMPUTE && inst->tempData->nodeType != TAG_SOURCE){
         continue;
      }

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         ConnectionInfo* info = &inst->tempData->outputs[ii];

         if(info->delay != 0){
            FUInstance* newInst = CreateFUInstance(accel,delay);

            InsertUnit(accel,inst,info->port,info->inst.inst,info->inst.port,newInst);
         }
      }
   }
   #endif

   ForEach(FUInstance,inst,accel->instances){
      if(inst->declaration != delay){
         inst->delay = inst->tempData->inputDelay;
      }
   }

   free(outputMem);
   free(order.instancePtrs);

#endif
}

static int NodeMappingConflict(Edge edge1,Edge edge2){
   FUInstance* list[4] = {edge1.units[0].inst,edge1.units[1].inst,edge2.units[0].inst,edge2.units[1].inst};
   FUInstance* instances[4] = {0,0,0,0};

   if(!(edge1.units[0].port == edge2.units[0].port && edge1.units[1].port == edge2.units[1].port)){
      return 0;
   }

   for(int i = 0; i < 4; i++){
      for(int ii = 0; ii < 4; ii++){
         if(instances[ii] == list[i]){
            return 1;
         } else if(instances[i] == 0){
            instances[ii] = list[i];
            break;
         }
      }
   }

   return 0;
}

static int MappingConflict(MappingNode map1,MappingNode map2){
   int res = (NodeMappingConflict(map1.edges[0],map1.edges[1]) ||
              NodeMappingConflict(map1.edges[0],map2.edges[0]) ||
              NodeMappingConflict(map1.edges[0],map2.edges[1]) ||
              NodeMappingConflict(map1.edges[1],map2.edges[0]) ||
              NodeMappingConflict(map1.edges[1],map2.edges[1]) ||
              NodeMappingConflict(map2.edges[0],map2.edges[1]));

   return res;
}

ConsolidationGraph GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2){
   ConsolidationGraph graph = {};

   graph.nodes = (MappingNode*) malloc(sizeof(MappingNode) * 1024);
   graph.edges = (MappingEdge*) malloc(sizeof(MappingEdge) * 1024);

   void* mem1 = CalculateGraphData(accel1);
   void* mem2 = CalculateGraphData(accel2);

   // Check node mapping
   #if 0
   for(int i = 0; i < accel1->nInstances; i++){
      for(int ii = 0; ii < accel2->nInstances; ii++){
         FUInstance* inst1 = &accel1->instances[i];
         FUInstance* inst2 = &accel2->instances[ii];

         if(inst1->declaration == inst2->declaration){
            MappingNode node = {};

            node.edges[0].inst[0] = inst1;
            node.edges[0].inst[1] = inst1;
            node.edges[1].inst[0] = inst2;
            node.edges[1].inst[1] = inst2;

            graph.nodes[graph.numberNodes++] = node;
         }
      }
   }
   #endif

   #if 0
   // Check possible edges
   for(int i = 0; i < accel1->nInstances; i++){
      for(int ii = 0; ii < accel2->nInstances; ii++){
         FUInstance* inst1 = &accel1->instances[i];
         FUInstance* inst2 = &accel2->instances[ii];

         for(int iii = 0; iii < inst1->tempData->numberOutputs; iii++){
            for(int iv = 0; iv < inst2->tempData->numberOutputs; iv++){
               FUInstance* other1 = inst1->tempData->outputs[iii].inst;
               FUInstance* other2 = inst2->tempData->outputs[iv].inst;

               // WRONG, INSTANCES CAN HAVE MULTIPLE PORTS CONNECTED
               // THIS WAY ONLY HAD 1 POSSIBLE

               PortEdge port1 = GetPort(inst1,other1);
               PortEdge port2 = GetPort(inst2,other2);

               if(inst1->declaration == inst2->declaration &&
                  other1->declaration == other2->declaration &&
                  port1.inPort == port2.inPort &&
                  port1.outPort == port2.outPort){

                  MappingNode node = {};

                  node.edges[0].inst[0] = inst1;
                  node.edges[0].inst[1] = other1;
                  node.edges[0].inPort = port1.inPort;
                  node.edges[0].outPort = port1.outPort;
                  node.edges[1].inst[0] = inst2;
                  node.edges[1].inst[1] = other2;
                  node.edges[1].inPort = port1.inPort;
                  node.edges[1].outPort = port1.outPort;

                  graph.nodes[graph.numberNodes++] = node;
               }
            }
         }
      }
   }

   // Check edge conflicts
   for(int i = 0; i < graph.numberNodes; i++){
      for(int ii = 0; ii < graph.numberNodes; ii++){
         if(i == ii){
            continue;
         }

         MappingNode node1 = graph.nodes[i];
         MappingNode node2 = graph.nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         MappingEdge edge = {};

         edge.nodes[0] = node1;
         edge.nodes[1] = node2;

         graph.edges[graph.numberEdges++] = edge;
      }
   }
   #endif

   free(mem1);
   free(mem2);

   return graph;
}

int EdgeEqual(Edge edge1,Edge edge2){
   int res = (memcmp(&edge1,&edge2,sizeof(Edge)) == 0);

   return res;
}

int MappingNodeEqual(MappingNode node1,MappingNode node2){
   int res = (EdgeEqual(node1.edges[0],node2.edges[0]) &&
              EdgeEqual(node1.edges[1],node2.edges[1]));

   return res;
}

void OnlyNeighbors(ConsolidationGraph* graph,int validNode){
   int neighbors[1024];

   MappingNode* node = &graph->nodes[validNode];

   for(int i = 0; i < 1024; i++){
      neighbors[i] = 0;
   }

   for(int i = 0; i < graph->numberEdges; i++){
      MappingEdge* edge = &graph->edges[i];

      if(MappingNodeEqual(edge->nodes[0],*node)){
         for(int j = 0; j < graph->numberNodes; j++){
            if(MappingNodeEqual(edge->nodes[1],graph->nodes[j])){
               neighbors[j] = 1;
            }
         }
      }
      if(MappingNodeEqual(edge->nodes[1],*node)){
         for(int j = 0; j < graph->numberNodes; j++){
            if(MappingNodeEqual(edge->nodes[0],graph->nodes[j])){
               neighbors[j] = 1;
            }
         }
      }
   }

   for(int i = 0; i < graph->numberNodes; i++){
      graph->validNodes[i] &= neighbors[i];
   }
}

ConsolidationGraph Copy(ConsolidationGraph graph){
   ConsolidationGraph res = {};

   res.nodes = (MappingNode*) calloc(graph.numberNodes,sizeof(MappingNode));
   memcpy(res.nodes,graph.nodes,sizeof(MappingNode) * graph.numberNodes);
   res.numberNodes = graph.numberNodes;

   res.edges = (MappingEdge*) calloc(graph.numberEdges,sizeof(MappingEdge));
   memcpy(res.edges,graph.edges,sizeof(MappingEdge) * graph.numberEdges);
   res.numberEdges = graph.numberEdges;

   res.validNodes = (int*) calloc(graph.numberNodes,sizeof(int));
   memcpy(res.validNodes,graph.validNodes,sizeof(int) * graph.numberNodes);

   return res;
}

int NumberNodes(ConsolidationGraph graph){
   int num = 0;
   for(int i = 0; i < graph.numberNodes; i++){
      if(graph.validNodes[i]){
         num += 1;
      }
   }

   return num;
}

static int max = 0;
static int found = 0;
static int table[1024];
static ConsolidationGraph clique;

void Clique(ConsolidationGraph graphArg,int size){
   if(NumberNodes(graphArg) == 0){
      if(size > max){
         max = size;
         clique = graphArg;
         found = true;
      }
      return;
   }

   ConsolidationGraph graph = Copy(graphArg);

   int num = 0;
   while((num = NumberNodes(graph)) != 0){
      if(size + num <= max){
         return;
      }

      int i;
      for(i = 0; i < graph.numberNodes; i++){
         if(graph.validNodes[i]){
            break;
         }
      }

      if(size + table[i] <= max){
         return;
      }

      OnlyNeighbors(&graph,i);
      Clique(graph,size + 1);
      if(found == true){
         return;
      }
   }
}

ConsolidationGraph MaxClique(ConsolidationGraph graph){
   ConsolidationGraph res = {};

   max = 0;
   found = 0;
   for(int i = 0; i < 1024; i++){
      table[i] = 0;
   }

   graph.validNodes = (int*) calloc(sizeof(int),graph.numberNodes);
   for(int i = graph.numberNodes - 1; i >= 0; i--){
      for(int j = 0; j < graph.numberNodes; j++){
         graph.validNodes[j] = (i <= j ? 1 : 0);
         printf("%d ",graph.validNodes[j]);
      }
      printf("\n");

      OnlyNeighbors(&graph,i);
      graph.validNodes[i] = 1;

      for(int j = 0; j < graph.numberNodes; j++){
         printf("%d ",graph.validNodes[j]);
      }
      printf("\n");
      printf("\n");

      Clique(graph,1);

      table[i] = max;
   }

   return graph;
}

typedef struct Mapping_t{
   FUInstance* source;
   FUInstance* sink;
} Mapping;

void AddMapping(Mapping* mappings, FUInstance* source,FUInstance* sink){
   for(int i = 0; i < 1024; i++){
      if(mappings[i].source == NULL){
         mappings[i].source = source;
         mappings[i].sink = sink;
         return;
      }
   }
}

Mapping* GetMapping(Mapping* mappings, FUInstance* source){
   for(int i = 0; i < 1024; i++){
      if(mappings[i].source == source){
         return &mappings[i];
      }
   }

   return NULL;
}

FUInstance* NodeMapped(FUInstance* inst, ConsolidationGraph graph){
   for(int i = 0; i < graph.numberNodes; i++){
      MappingNode node = graph.nodes[i];

      if(!graph.validNodes[i]){
         continue;
      }

      if(node.edges[0].units[0].inst == node.edges[0].units[1].inst){ // Node mapping
         if(node.edges[0].units[0].inst == inst){
            return node.edges[1].units[0].inst;
         }
         if(node.edges[1].units[0].inst == inst){
            return node.edges[0].units[0].inst;
         }
      } else { // Edge mapping
         if(node.edges[0].units[0].inst == inst){
            return node.edges[1].units[0].inst;
         }
         if(node.edges[0].units[1].inst == inst){
            return node.edges[1].units[1].inst;
         }
         if(node.edges[1].units[0].inst == inst){
            return node.edges[0].units[0].inst;
         }
         if(node.edges[1].units[1].inst == inst){
            return node.edges[0].units[1].inst;
         }
      }
   }

   return NULL;
}

Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph){
#if MARK
   Mapping* graphToFinal = (Mapping*) calloc(sizeof(Mapping),1024);
   HashMap map = {};
   map.dataSize = sizeof(FUInstance*);
   map.keySize = sizeof(FUInstance*);

   Accelerator* newGraph = CreateAccelerator(versat);

   void* mem1 = CalculateGraphData(accel1);
   void* mem2 = CalculateGraphData(accel2);

   // Create base instances (accel 1)
   ForEach(FUInstance,inst,accel1->instances){
      FUInstance* newNode = CreateFUInstance(newGraph,inst->declaration);

      HashMapAddItem(&map,&inst,&newNode);
   }

   #if 1
   ForEach(FUInstance,inst,accel2->instances){
      FUInstance* mappedNode = NodeMapped(inst,graph); // Returns node in graph 1

      if(mappedNode){
         mappedNode = *HashMapGetItem(map,&mappedNode,FUInstance*);
      } else {
         mappedNode = CreateFUInstance(newGraph,inst->declaration);
      }

      AddMapping(graphToFinal,inst,mappedNode);
   }
   #endif

   // Add edges from accel1
   ForEach(FUInstance,inst,accel1->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         FUInstance* other = inst->tempData->outputs[ii].inst.inst;

         FUInstance* mappedInst = *HashMapGetItem(map,&inst,FUInstance*);
         FUInstance* mappedOther = *HashMapGetItem(map,&other,FUInstance*);

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }

   #if 1
   ForEach(FUInstance,inst,accel2->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         FUInstance* other = inst->tempData->outputs[ii].inst.inst;

         FUInstance* mappedInst = *HashMapGetItem(map,&inst,FUInstance*);
         FUInstance* mappedOther = *HashMapGetItem(map,&other,FUInstance*);

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }
   #endif

   free(mem1);
   free(mem2);
   HashMapFree(&map);
   return newGraph;
#endif
   return nullptr;
}

#undef NUMBER_OUTPUTS
#undef OUTPUT




























