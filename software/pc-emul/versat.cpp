#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <array>
#include <functional>

#include <printf.h>

#include <set>
#include <utility>

#include "../versat.hpp"

#include "templateEngine.hpp"

#include "unitVerilogWrappers.hpp"

#define DELAY_BIT_SIZE 8

static int versat_base;

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2


// Implementations of units that versat needs to know about explicitly

#define MAX_DELAY 128

static int32_t* DefaultInitFunction(FUInstance* inst){
   inst->done = true;
   return nullptr;
}

static int32_t* DelayUpdateFunction(FUInstance* inst){
   static int32_t out;

   Assert(inst->delay[0] < MAX_DELAY);

   int32_t* fifo = (int32_t*) inst->extraData;

   if(inst->delay[0] == 0){
      out = GetInputValue(inst,0);
   } else {
      out = fifo[0];

      for(int i = 0; i < inst->delay[0]; i++){
         fifo[i] = fifo[i+1];
      }

      fifo[inst->delay[0] - 1] = GetInputValue(inst,0);
   }

   return &out;
}

static FUDeclaration* RegisterDelay(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"delay");
   decl.nInputs = 1;
   decl.nOutputs = 1;
   decl.latency = 1;
   decl.nDelays = 1;
   decl.delayType = DelayType::DELAY_TYPE_COMPUTE_DELAY;
   decl.extraDataSize = sizeof(int32_t) * MAX_DELAY;
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = DelayUpdateFunction;
   decl.type = FUDeclaration::SINGLE;

   return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterCircuitInput(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"circuitInput");
   decl.nOutputs = 99;
   decl.nInputs = 1;  // Used for templating circuit
   decl.initializeFunction = DefaultInitFunction;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterCircuitOutput(Versat* versat){
   FUDeclaration decl = {};

   strcpy(decl.name.str,"circuitOutput");
   decl.nInputs = 99;
   decl.nOutputs = 99; // Used for templating circuit
   decl.initializeFunction = DefaultInitFunction;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}

static int Visit(FUInstance*** ordering,FUInstance* inst){
   if(inst->tag == TAG_PERMANENT_MARK){
      return 0;
   }
   if(inst->tag == TAG_TEMPORARY_MARK){
      Assert(0);
   }

   if(inst->tempData->nodeType == GraphComputedData::TAG_SINK ||
     (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      return 0;
   }

   inst->tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
      for(int i = 0; i < inst->tempData->numberInputs; i++){
         count += Visit(ordering,inst->tempData->inputs[i].inst.inst);
      }
   }

   inst->tag = TAG_PERMANENT_MARK;

   if(inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
      *(*ordering) = inst;
      (*ordering) += 1;
      count += 1;
   }

   return count;
}

static void CalculateDAGOrdering(Accelerator* accel){
   Assert(!accel->locked);

   DAGOrder ordering = {};

   for(FUInstance* inst : accel->instances){
      inst->tag = 0;
   }

   ordering.instancePtrs = (FUInstance**) calloc(accel->instances.Size(),sizeof(FUInstance*));
   FUInstance** sourceUnits = ordering.instancePtrs;
   ordering.sources = sourceUnits;

   // Add source units, guaranteed to come first
   for(FUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE || (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
         *(sourceUnits++) = inst;
         ordering.numberSources += 1;
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   // Add compute units
   FUInstance** computeUnits = sourceUnits;
   ordering.computeUnits = computeUnits;
   for(FUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         *(computeUnits++) = inst;
         ordering.numberComputeUnits += 1;
         inst->tag = TAG_PERMANENT_MARK;
      } else if(inst->tag == 0 && inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
         ordering.numberComputeUnits += Visit(&computeUnits,inst);
      }
   }

   // Add sink units
   FUInstance** sinkUnits = computeUnits;
   ordering.sinks = sinkUnits;
   for(FUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_SINK || (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
         *(sinkUnits++) = inst;
         ordering.numberSinks += 1;
         Assert(inst->tag == 0);
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   for(FUInstance* inst : accel->instances){
      Assert(inst->tag == TAG_PERMANENT_MARK);
   }

   Assert(ordering.numberSources + ordering.numberComputeUnits + ordering.numberSinks == accel->instances.Size());

   for(int i = 0; i < accel->instances.Size(); i++){
      Assert(ordering.instancePtrs[i]);
   }

   accel->order = ordering;
}

void LockAccelerator(Accelerator* accel){
   if(accel->locked){
      return;
   }

   if(accel->graphDataMem){
      free(accel->graphDataMem);
      free(accel->order.instancePtrs);
   }

   CalculateGraphData(accel);
   CalculateDAGOrdering(accel);
   accel->locked = true;
}

static UnitInfo CalculateUnitInfo(FUInstance* inst){
   UnitInfo info = {};

   FUDeclaration decl = *inst->declaration;

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
}

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;

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

int32_t* UnaryNot(FUInstance* inst){
    static uint32_t out;
    out = ~GetInputValue(inst,0);
    inst->done = true;
    return (int32_t*) &out;
}

int32_t* BinaryXOR(FUInstance* inst){
    static uint32_t out;
    out = GetInputValue(inst,0) ^ GetInputValue(inst,1);
    inst->done = true;
    return (int32_t*) &out;
}

int32_t* BinaryADD(FUInstance* inst){
    static uint32_t out;
    out = GetInputValue(inst,0) + GetInputValue(inst,1);
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinaryAND(FUInstance* inst){
    static uint32_t out;
    out = GetInputValue(inst,0) & GetInputValue(inst,1);
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinaryOR(FUInstance* inst){
    static uint32_t out;
    out = GetInputValue(inst,0) | GetInputValue(inst,1);
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinaryRHR(FUInstance* inst){
    static uint32_t out;
    uint32_t value = GetInputValue(inst,0);
    uint32_t shift = GetInputValue(inst,1);
    out = (value >> shift) | (value << (32 - shift));
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinaryRHL(FUInstance* inst){
    static uint32_t out;
    uint32_t value = GetInputValue(inst,0);
    uint32_t shift = GetInputValue(inst,1);
    out = (value << shift) | (value >> (32 - shift));
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinarySHR(FUInstance* inst){
    static uint32_t out;
    uint32_t value = GetInputValue(inst,0);
    uint32_t shift = GetInputValue(inst,1);
    out = (value >> shift);
    inst->done = true;
    return (int32_t*) &out;
}
int32_t* BinarySHL(FUInstance* inst){
    static uint32_t out;
    uint32_t value = GetInputValue(inst,0);
    uint32_t shift = GetInputValue(inst,1);
    out = (value << shift);
    inst->done = true;
    return (int32_t*) &out;
}

void RegisterOperators(Versat* versat){
   const char* unary[] = {"NOT"};
   FUFunction unaryF[] = {UnaryNot};
   const char* binary[] = {"XOR","ADD","AND","OR","RHR","SHR","RHL","SHL"};
   FUFunction binaryF[] = {BinaryXOR,BinaryADD,BinaryAND,BinaryOR,BinaryRHR,BinarySHR,BinaryRHL,BinarySHL};

   FUDeclaration decl = {};
   decl.nOutputs = 1;
   decl.nInputs = 1;
   for(int i = 0; i < ARRAY_SIZE(unary); i++){
      FixedStringCpy(decl.name.str,MakeSizedString(unary[i]));
      decl.updateFunction = unaryF[i];
      RegisterFU(versat,decl);
   }

   decl.nInputs = 2;
   for(int i = 0; i < ARRAY_SIZE(binary); i++){
      FixedStringCpy(decl.name.str,MakeSizedString(binary[i]));
      decl.updateFunction = binaryF[i];
      RegisterFU(versat,decl);
   }
}

void InitVersat(Versat* versat,int base,int numberConfigurations){
   versat->numberConfigurations = numberConfigurations;
   versat->base = base;

   FUDeclaration nullDeclaration = {};
   RegisterFU(versat,nullDeclaration);

   versat->delay = RegisterDelay(versat);
   versat->input = RegisterCircuitInput(versat);
   versat->output = RegisterCircuitOutput(versat);
   versat->pipelineRegister = RegisterPipelineRegister(versat);

   // Versat specific units
   RegisterAdd(versat);
   RegisterReg(versat);
   RegisterVRead(versat);
   RegisterVWrite(versat);
   RegisterMem(versat,10);
   RegisterDebug(versat);
   RegisterConst(versat);

   RegisterOperators(versat);
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

FUDeclaration* GetTypeByName(Versat* versat,const char* str){
   FUDeclaration* res = GetTypeByName(versat,MakeSizedString(str));
   return res;
}

FUDeclaration* GetTypeByName(Versat* versat,SizedString name){
   for(FUDeclaration* decl : versat->declarations){
      if(CompareString(decl->name.str,name)){
         return decl;
      }
   }

   Assert(0);
   return nullptr;
}

FUInstance* GetInstance(Accelerator* circuit,std::initializer_list<char*> names){
   Accelerator* ptr = circuit;
   FUInstance* res = nullptr;

   for(char* str : names){
      int found = 0;

      for(FUInstance* inst : ptr->instances){
         if(CompareString(inst->name.str,str)){
            res = inst;
            found = 1;

            break;
         }
      }

      Assert(found);

      if(res->declaration->type == FUDeclaration::COMPOSITE){
         ptr = res->compositeAccel;
      }
   }

   return res;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){
   char buffer[256];

   va_list args;
   va_start(args,argc);

   Accelerator* ptr = circuit;
   FUInstance* res = nullptr;

   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);
      int found = 0;

      int arguments = parse_printf_format(str,0,nullptr);

      if(arguments){
         vsprintf(buffer,str,args);
         i += arguments;
         for(int ii = 0; ii < arguments; ii++){
            va_arg(args, int); // Need to consume something
         }
      } else {
         strcpy(buffer,str);
      }

      for(FUInstance* inst : ptr->instances){
         if(strcmp(inst->name.str,buffer) == 0){
            res = inst;
            found = 1;

            break;
         }
      }

      Assert(found);

      if(res->declaration->type == FUDeclaration::COMPOSITE){
         ptr = res->compositeAccel;
      }
   }

   va_end(args);

   return res;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   FUDeclaration* type = versat->declarations.Alloc();
   *type = decl;

   return type;
}

// Uses static allocated memory. Intended for use by OutputGraphDotFile
static char* FormatNameToOutput(FUInstance* inst){
   #define PRINT_ID 0
   #define HIERARCHICAL_NAME 0
   #define PRINT_DELAY 1

   static char buffer[1024];

   #if HIERARCHICAL_NAME == 1
      char* name = GetHierarchyNameRepr(inst->name);
   #else
      char* name = inst->name.str;
   #endif

   char* ptr = buffer;
   ptr += sprintf(ptr,"%s",name);

   #if PRINT_ID == 1
   ptr += sprintf(ptr,"_%d",inst->id);
   #endif

   #if PRINT_DELAY == 1
   ptr += sprintf(ptr,"_%d",inst->baseDelay);
   #endif

   #undef PRINT_ID
   #undef HIERARCHICAL_NAME
   #undef PRINT_DELAY

   return buffer;
}

void OutputGraphDotFile(Accelerator* accel,FILE* outputFile,int collapseSameEdges){
   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(FUInstance* inst : accel->instances){
      char* name = FormatNameToOutput(inst);

      fprintf(outputFile,"\t%s;\n",name);
   }

   std::set<std::pair<FUInstance*,FUInstance*>> sameEdgeCounter;

   for(Edge* edge : accel->edges){
      if(collapseSameEdges){
         std::pair<FUInstance*,FUInstance*> key{edge->units[0].inst,edge->units[1].inst};

         if(sameEdgeCounter.count(key) == 1){
            continue;
         }

         sameEdgeCounter.insert(key);
      }

      fprintf(outputFile,"\t%s -> ",FormatNameToOutput(edge->units[0].inst));
      fprintf(outputFile,"%s;\n",FormatNameToOutput(edge->units[1].inst));
   }

   fprintf(outputFile,"}\n");
}

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,FUInstance* instance,int layers){
   Accelerator* newAccel = CreateAccelerator(versat);

   std::set<FUInstance*> subgraphUnits;
   std::set<FUInstance*> tempSubgraphUnits;
   subgraphUnits.insert(instance);
   for(int i = 0; i < layers; i++){
      for(FUInstance* inst : subgraphUnits){
         for(Edge* edge : accel->edges){
            for(int i = 0; i < 2; i++){
               if(edge->units[i].inst == inst){
                  tempSubgraphUnits.insert(edge->units[1 - i].inst);
                  break;
               }
            }
         }
      }

      subgraphUnits.insert(tempSubgraphUnits.begin(),tempSubgraphUnits.end());
      tempSubgraphUnits.clear();
   }

   InstanceMap map;
   for(FUInstance* nonMapped : subgraphUnits){
      FUInstance* mapped = CreateShallowNamedFUInstance(newAccel,nonMapped->declaration,MakeSizedString(nonMapped->name.str),nullptr);
      map.insert({nonMapped,mapped});
   }

   for(Edge* edge : accel->edges){
      auto iter0 = map.find(edge->units[0].inst);
      auto iter1 = map.find(edge->units[1].inst);

      if(iter0 == map.end() || iter1 == map.end()){
         continue;
      }

      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = iter0->second;
      newEdge->units[1].inst = iter1->second;
   }

   SubgraphData data = {};

   data.accel = newAccel;
   data.instanceMapped = map.at(instance);

   return data;
}


Accelerator* CreateAccelerator(Versat* versat){
   Accelerator* accel = versat->accelerators.Alloc();
   accel->versat = versat;

   return accel;
}

static Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,HierarchyName* parent,InstanceMap* map,int32_t* config,int32_t* state,int32_t* memMapped);

static FUInstance* CreateSubFUInstance(Accelerator* accel,FUDeclaration* declaration,int32_t* config,int32_t* state,int32_t* memMapped){
   FUInstance* ptr = CreateShallowFUInstance(accel,declaration);

   ptr->config = config;
   ptr->state = state;
   ptr->memMapped = memMapped;

   if(declaration->nOutputs){
      ptr->outputs = (int32_t*) calloc(declaration->nOutputs,sizeof(int32_t));
      ptr->storedOutputs = (int32_t*) calloc(declaration->nOutputs,sizeof(int32_t));
   }

   if(declaration->extraDataSize){
      ptr->extraData = calloc(declaration->extraDataSize,sizeof(char));
   }

   ptr->delay = (int32_t*) calloc(1,sizeof(int32_t));

   if(declaration->type == FUDeclaration::COMPOSITE){
      InstanceMap map = {};
      ptr->compositeAccel = CopyAccelerator(accel->versat,declaration->circuit,&ptr->name,&map,config,state,memMapped);
   }

   return ptr;
}

static Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,HierarchyName* parent,InstanceMap* map,int32_t* config,int32_t* state,int32_t* memMapped){
   Accelerator* newAccel = CreateAccelerator(versat);

   // Flat copy of instances
   for(FUInstance* inst : accel->instances){
      FUInstance* newInst = CreateSubFUInstance(newAccel,inst->declaration,config,state,memMapped);

      config += inst->declaration->nConfigs;
      state += inst->declaration->nStates;
      memMapped = (int32_t*) (((char*) memMapped) + inst->declaration->memoryMapBytes); // Should be aligned, I think

      newInst->name.parent = parent;
      FixedStringCpy(newInst->name.str,MakeSizedString(inst->name.str));
      newInst->baseDelay = inst->baseDelay;

      if(newInst->declaration->initializeFunction){
         newInst->declaration->initializeFunction(newInst);
      }

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   for(Edge* edge : accel->edges){
      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = map->at(edge->units[0].inst);
      newEdge->units[1].inst = map->at(edge->units[1].inst);
   }

   // Flat copy of input instance pointers
   for(FUInstance** instPtr : accel->inputInstancePointers){
      FUInstance** newInstPtr = newAccel->inputInstancePointers.Alloc();

      *newInstPtr = map->at(*instPtr);
   }

   if(accel->outputInstance){
      newAccel->outputInstance = map->at(accel->outputInstance);
   }

   return newAccel;
}

FUInstance* CreateShallowFUInstance(Accelerator* accel,FUDeclaration* type){
   FUInstance* ptr = accel->instances.Alloc();

   ptr->id = accel->entityId++;
   ptr->accel = accel;
   ptr->declaration = type;

   ptr->delay = (int*) calloc(1,sizeof(int));

   sprintf(ptr->name.str,"%.15s",FixEntityName(type->name.str));

   return ptr;
}

static void CheckReallocation(AllocInfo* info,FUInstance* inst,Accelerator* accel,int* Accelerator::*accelArrayPtr,int* FUInstance::*arrayPtr,int FUDeclaration::*sizePtr){
   int size = inst->declaration->*sizePtr;

   if(size){
      if(info->allocated - info->size <= size){
         int newAllocated = maxi(info->size * 2,info->size + size);

         int32_t* newPtr = (int32_t*) realloc(accel->*accelArrayPtr,newAllocated * sizeof(int32_t));
         accel->*accelArrayPtr = newPtr;

         for(FUInstance* inst : accel->instances){
            inst->*arrayPtr = newPtr;
            newPtr += inst->declaration->*sizePtr;
         }

         info->allocated = newAllocated;
      }

      inst->*arrayPtr = &(accel->*accelArrayPtr)[info->size];
      info->size += size;
   }
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type){
   Assert(!accel->locked);

   FUInstance* ptr = CreateShallowFUInstance(accel,type);

   FUDeclaration decl = *type;

   CheckReallocation(&accel->configAlloc   ,ptr,accel,&Accelerator::config   ,&FUInstance::config   ,&FUDeclaration::nConfigs);
   CheckReallocation(&accel->stateAlloc    ,ptr,accel,&Accelerator::state    ,&FUInstance::state    ,&FUDeclaration::nStates);
   CheckReallocation(&accel->memMappedAlloc,ptr,accel,&Accelerator::memMapped,&FUInstance::memMapped,&FUDeclaration::memoryMapBytes);

   if(decl.nOutputs){
      ptr->outputs = (int32_t*) calloc(decl.nOutputs,sizeof(int32_t));
      ptr->storedOutputs = (int32_t*) calloc(decl.nOutputs,sizeof(int32_t));
   }

   if(decl.extraDataSize){
      ptr->extraData = calloc(decl.extraDataSize,sizeof(char));
   }

   ptr->delay = (int32_t*) calloc(1,sizeof(int32_t));

   if(decl.type == FUDeclaration::COMPOSITE){
      InstanceMap map = {};
      ptr->compositeAccel = CopyAccelerator(accel->versat,decl.circuit,&ptr->name,&map,ptr->config,ptr->state,ptr->memMapped);
   }

   if(decl.initializeFunction)
      decl.initializeFunction(ptr);

   return ptr;
}

FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName,HierarchyName* hierarchyParent){
   FUInstance* inst = CreateFUInstance(accel,type);

   FixedStringCpy(inst->name.str,entityName);
   inst->name.parent = hierarchyParent;

   return inst;
}

FUInstance* CreateShallowNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName,HierarchyName* hierarchyParent){
   FUInstance* inst = CreateShallowFUInstance(accel,type);

   FixedStringCpy(inst->name.str,entityName);
   inst->name.parent = hierarchyParent;

   return inst;
}

void RemoveFUInstance(Accelerator* accel,FUInstance* inst){
   Assert(!accel->locked);

   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == inst){
         accel->edges.Remove(edge);
      } else if(edge->units[1].inst == inst){
         accel->edges.Remove(edge);
      }
   }

   accel->instances.Remove(inst);
}

static bool IsGraphValid(Accelerator* accel){
   LockAccelerator(accel);

   InstanceMap map;

   for(FUInstance* inst : accel->instances){
      inst->tag = 0;

      map.insert({inst,inst});
   }

   for(Edge* edge : accel->edges){
      for(int i = 0; i < 2; i++){
         auto res = map.find(edge->units[i].inst);

         if(res == map.end()){
            return 0;
         }

         res->first->tag = 1;
      }
   }

   for(FUInstance* inst : accel->instances){
      if(inst->tag != 1){
         return 0;
      }
   }

   accel->locked = false;
   return 1;
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   InstanceMap map;
   Accelerator* newAccel = CopyAccelerator(versat,accel,nullptr,&map,nullptr,nullptr,nullptr);
   map.clear();

   PortInstance outputs[60];
   for(int i = 0; i < 60; i++){
      outputs[i] = (PortInstance){};
   }

   // TODO: figure out a way to have true hierarchircal naming schemes even when flattening
   // Use it to debug further. No point wasting time right now
   Pool<FUInstance*> compositeInstances = {};
   for(int i = 0; i < times; i++){
      for(FUInstance* inst : newAccel->instances){
         if(inst->declaration->type == FUDeclaration::COMPOSITE){
            FUInstance** ptr = compositeInstances.Alloc();
            *ptr = inst;
         }
      }

      int count = 0;
      for(FUInstance** instPtr : compositeInstances){
         FUInstance* inst = *instPtr;
         count += 1;
         Accelerator* circuit = inst->declaration->circuit;

         // Create new instance and map then
         #if 1
         for(FUInstance* circuitInst : circuit->instances){
            if(circuitInst->declaration->type == FUDeclaration::SPECIAL){
               continue;
            }

            FUInstance* newInst = CreateNamedFUInstance(newAccel,circuitInst->declaration,MakeSizedString(circuitInst->name.str),circuitInst->name.parent);
            newInst->baseDelay = circuitInst->baseDelay;

            map.insert({circuitInst,newInst});
         }
         #endif

         #if 1
         // Add accel edges to output instances
         for(Edge* edge : newAccel->edges){
            if(edge->units[0].inst == inst){
               for(Edge* circuitEdge: circuit->edges){
                  if(circuitEdge->units[1].inst == circuit->outputInstance && circuitEdge->units[1].port == edge->units[0].port){
                     Edge* newEdge = newAccel->edges.Alloc();

                     FUInstance* mappedInst = map.at(circuitEdge->units[0].inst);

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
         for(Edge* edge : newAccel->edges){
            if(edge->units[1].inst == inst){
               FUInstance* circuitInst = *circuit->inputInstancePointers.Get(edge->units[1].port);

               for(Edge* circuitEdge : circuit->edges){
                  if(circuitEdge->units[0].inst == circuitInst){
                     Edge* newEdge = newAccel->edges.Alloc();

                     FUInstance* mappedInst = map.at(circuitEdge->units[1].inst);

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
         for(Edge* circuitEdge : circuit->edges){
            auto input = map.find(circuitEdge->units[0].inst);
            auto output = map.find(circuitEdge->units[1].inst);

            if(input == map.end() || output == map.end()){
               continue;
            }

            Edge* newEdge = newAccel->edges.Alloc();

            *newEdge = *circuitEdge;
            newEdge->units[0].inst = input->second;
            newEdge->units[1].inst = output->second;
         }
         #endif

         #if 0
         SubgraphData subgraph = SubGraphAroundInstance(versat,newAccel,inst,6);
         {
            char buffer[128];
            sprintf(buffer,"debug/%d_%d_0.dot",i,count);
            FILE* dotFile = fopen(buffer,"w");
            OutputGraphDotFile(subgraph.accel,dotFile,1);
            fclose(dotFile);
         }
         #endif

         RemoveFUInstance(newAccel,inst);
         map.clear();

         #if 0
         RemoveFUInstance(subgraph.accel,subgraph.instanceMapped);
         {
            char buffer[128];
            sprintf(buffer,"debug/%d_%d_1.dot",i,count);
            FILE* dotFile = fopen(buffer,"w");
            OutputGraphDotFile(subgraph.accel,dotFile,1);
            fclose(dotFile);
         }
         #endif

         Assert(IsGraphValid(newAccel));
      }

      compositeInstances.Clear();
   }

   return newAccel;
}

// Debug output file
static FILE* accelOutputFile = nullptr;
static std::array<char,3> currentMapping = {'a','a','a'};
static void ResetMapping(){
   currentMapping[0] = 'a';
   currentMapping[1] = 'a';
   currentMapping[2] = 'a';
}

static void IncrementMapping(){
   for(int i = 2; i >= 0; i--){
      currentMapping[i] += 1;
      if(currentMapping[i] == 'z' + 1){
         currentMapping[i] = 'a';
      } else {
         return;
      }
   }
}

static void PrintVCDDefinitions_(Accelerator* accel){
   for(FUInstance* inst : accel->instances){
      fprintf(accelOutputFile,"$scope module %s_%d $end\n",inst->name.str,inst->id);

      for(int i = 0; i < inst->tempData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c %s_in%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],GetHierarchyNameRepr(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->tempData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c %s_out%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],GetHierarchyNameRepr(inst->name),i);
         IncrementMapping();
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         PrintVCDDefinitions_(inst->compositeAccel);
      }

      fprintf(accelOutputFile,"$upscope $end\n");
   }
}

static void PrintVCDDefinitions(Accelerator* accel){
   ResetMapping();

   fprintf(accelOutputFile,"$timescale   1ns $end\n");
   fprintf(accelOutputFile,"$scope module TOP $end\n");
   fprintf(accelOutputFile,"$var wire  1 a clk $end\n");
   PrintVCDDefinitions_(accel);
   fprintf(accelOutputFile,"$upscope $end\n");
   fprintf(accelOutputFile,"$enddefinitions $end\n");
}

static char* Bin(unsigned int val){
   static char buffer[33];
   buffer[32] = '\0';

   for(int i = 0; i < 32; i++){
      if(val - (1 << (31 - i)) < val){
         val = val - (1 << (31 - i));
         buffer[i] = '1';
      } else {
         buffer[i] = '0';
      }
   }
   return buffer;
}

static void PrintVCD_(Accelerator* accel){
   for(FUInstance* inst : accel->instances){
      for(int i = 0; i < inst->tempData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c\n",Bin(GetInputValue(inst,i)),currentMapping[0],currentMapping[1],currentMapping[2]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->tempData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c\n",Bin(inst->outputs[i]),currentMapping[0],currentMapping[1],currentMapping[2]);
         IncrementMapping();
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         PrintVCD_(inst->compositeAccel);
      }
   }
}

static void PrintVCD(Accelerator* accel,int time,int clock){ // Need to put some clock signal
   ResetMapping();

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock);
   PrintVCD_(accel);
}

// Declarations
typedef std::function<void(FUInstance*)> AcceleratorInstancesVisitor;
static void VisitAcceleratorInstances(Accelerator* accel,AcceleratorInstancesVisitor func);
static void AcceleratorRunStart(Accelerator* accel);

static void VisitAcceleratorInstances_(FUInstance* inst,AcceleratorInstancesVisitor func){
   func(inst);

   if(inst->declaration->type == FUDeclaration::COMPOSITE){
      VisitAcceleratorInstances(inst->compositeAccel,func);
   }
}

static void VisitAcceleratorInstances(Accelerator* accel,AcceleratorInstancesVisitor func){
   LockAccelerator(accel);

   for(FUInstance* inst : accel->instances){
      VisitAcceleratorInstances_(inst,func);
   }
}

static void AcceleratorRunStartVisitor(FUInstance* inst){
   FUDeclaration decl = *inst->declaration;
   FUFunction startFunction = decl.startFunction;

   if(startFunction){
      int32_t* startingOutputs = startFunction(inst);

      if(startingOutputs){
         memcpy(inst->outputs,startingOutputs,decl.nOutputs * sizeof(int32_t));
         memcpy(inst->storedOutputs,startingOutputs,decl.nOutputs * sizeof(int32_t));
      }
   }
}

static void AcceleratorRunStart(Accelerator* accel){
   LockAccelerator(accel);

   VisitAcceleratorInstances(accel,AcceleratorRunStartVisitor);
}

static bool AcceleratorDone(Accelerator* accel){
   for(FUInstance* inst : accel->instances){
      if(!inst->done){
         return false;
      }
   }

   return true;
}

static void AcceleratorRunIteration(Accelerator* accel){
   for(int i = 0; i < accel->instances.Size(); i++){
      FUInstance* inst = accel->order.instancePtrs[i];

      FUDeclaration decl = *inst->declaration;
      if(decl.type == FUDeclaration::SPECIAL){
         continue;
      } else if(decl.type == FUDeclaration::COMPOSITE){
         // Set accelerator input to instance input
         for(int ii = 0; ii < inst->tempData->numberInputs; ii++){
            FUInstance* input = *inst->compositeAccel->inputInstancePointers.Get(ii);

            for(int iii = 0; iii < input->tempData->numberOutputs; iii++){
               int32_t val = GetInputValue(inst,ii);
               input->outputs[iii] = val;
            }
         }

         AcceleratorRunIteration(inst->compositeAccel);

         // Calculate unit done
         inst->done = AcceleratorDone(inst->compositeAccel);

         // Set output instance value to accelerator output
         FUInstance* output = inst->compositeAccel->outputInstance;
         if(output){
            for(int ii = 0; ii < output->tempData->numberInputs; ii++){
               int32_t val = GetInputValue(output,ii);
               inst->outputs[ii] = val;
            }
         }
      } else {
         int32_t* newOutputs = decl.updateFunction(inst);

         if(inst->declaration->latency == 0 && inst->tempData->nodeType != GraphComputedData::TAG_SOURCE){
            memcpy(inst->outputs,newOutputs,decl.nOutputs * sizeof(int32_t));
            memcpy(inst->storedOutputs,newOutputs,decl.nOutputs * sizeof(int32_t));
         } else {
            memcpy(inst->storedOutputs,newOutputs,decl.nOutputs * sizeof(int32_t));
         }
      }
   }
}

void LoadConfiguration(Accelerator* accel,int configuration){
   // Implements the reverse of Save Configuration
}

void SaveConfiguration(Accelerator* accel,int configuration){
   //Assert(configuration < accel->versat->numberConfigurations);
}

void AcceleratorDoCycle(Accelerator* accel){
   VisitAcceleratorInstances(accel,[](FUInstance* inst){
      FUDeclaration decl = *inst->declaration;
      memcpy(inst->outputs,inst->storedOutputs,decl.nOutputs * sizeof(int32_t));
   });
}

// Need to generate VCD file or else I'll never debug this correctly
void AcceleratorRun(Accelerator* accel){
   static int numberRuns = 0;
   static int time = 0;

   // Only used to lock all acelerators
   VisitAcceleratorInstances(accel,[](FUInstance*){
   });

   {
      char buffer[128];
      sprintf(buffer,"debug/accelRun%d.vcd",numberRuns++);
      accelOutputFile = fopen(buffer,"w");
      Assert(accelOutputFile);
   }

   PrintVCDDefinitions(accel);

   AcceleratorRunStart(accel);
   AcceleratorRunIteration(accel);
   PrintVCD(accel,time++,0);
   AcceleratorDoCycle(accel);
   AcceleratorRunIteration(accel);
   PrintVCD(accel,time++,1);
   PrintVCD(accel,time++,0);

   for(int cycle = 0; 1; cycle++){
      AcceleratorDoCycle(accel);
      AcceleratorRunIteration(accel);
      PrintVCD(accel,time++,1);
      PrintVCD(accel,time++,0);

      if(AcceleratorDone(accel)){
         break;
      }
   }

   PrintVCD(accel,time++,1);
   PrintVCD(accel,time++,0);
   fclose(accelOutputFile);
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

static VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel){
   LockAccelerator(accel);

   VersatComputedValues res = {};

   for(FUInstance* inst : accel->instances){
      UnitInfo info = CalculateUnitInfo(inst);

      res.numberConnections += inst->tempData->numberOutputs;

      res.maxMemoryMapBytes = maxi(res.maxMemoryMapBytes,info.memoryMappedBytes);
      res.memoryMapped += info.memoryMappedBytes;

      if(info.memoryMappedBytes)
         res.unitsMapped += 1;

      res.nConfigs += info.nConfigsWithDelay;
      res.configurationBits += info.configBitSize;
      res.nStates += info.nStates;
      res.stateBits += info.stateBitSize;
      res.nUnitsIO += info.doesIO;
      res.nDelays += inst->declaration->nDelays;
   }

   // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
   res.nConfigs += 1;
   res.nStates += 1;

   res.memoryAddressBits = log2i(res.maxMemoryMapBytes);

   res.unitsMappedAddressBits = log2i(res.unitsMapped);
   res.memoryMappingAddressBits = res.unitsMappedAddressBits + res.memoryAddressBits;
   res.configurationAddressBits = log2i(res.nConfigs + res.nDelays);
   res.stateAddressBits = log2i(res.nStates);
   res.stateConfigurationAddressBits = maxi(res.configurationAddressBits,res.stateAddressBits);

   res.lowerAddressSize = maxi(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);
   res.addressSize = res.lowerAddressSize + 1; // One bit to select between memory or config/state

   res.memoryConfigDecisionBit = res.addressSize - 3;
   res.memoryMappedUnitAddressRange.high = res.addressSize - 4;
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

void OutputMemoryMap(Versat* versat,Accelerator* accel){
   VersatComputedValues val = ComputeVersatValues(versat,accel);

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
   printf("Configuration bits used: %d\n",val.configurationBits);
   printf("\n");
   printf("State registers: %d (including versat reg)\n",val.nStates);
   printf("State address bits: %d\n",val.stateAddressBits);
   printf("State bits used: %d\n",val.stateBits);
   printf("\n");
   printf("IO connections: %d\n",val.nUnitsIO);

   printf("\n");
   printf("Number units: %d\n",versat->accelerators.Get(0)->instances.Size());
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
   Accelerator* accel = instance->accel;

   for(Edge* edge : accel->edges){
      if(edge->units[1].inst == instance && edge->units[1].port == index){
         return edge->units[0].inst->outputs[edge->units[0].port];
      }
   }

   return 0;
}

// Connects out -> in
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex){
   FUDeclaration inDecl = *in->declaration;
   FUDeclaration outDecl = *out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl.nInputs);
   Assert(outIndex < outDecl.nOutputs);

   Accelerator* accel = out->accel;

   Assert(!accel->locked);

   Edge* edge = accel->edges.Alloc();

   edge->units[0].inst = out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = in;
   edge->units[1].port = inIndex;
}

void OutputCircuitSource(Versat* versat,FUDeclaration decl,Accelerator* accel,FILE* file){
   VersatComputedValues val = ComputeVersatValues(versat,accel);

   LockAccelerator(accel);

   TemplateSetCustom("accel",&decl,"FUDeclaration");

   // Output configuration file
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetInstancePool("instances",&accel->instances);
   TemplateSetNumber("numberUnits",accel->instances.Size());

   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryMappedUnitAddressRangeHigh",val.memoryMappedUnitAddressRange.high);
   TemplateSetNumber("memoryMappedUnitAddressRangeLow",val.memoryMappedUnitAddressRange.low);
   TemplateSetNumber("configurationBits",val.configurationBits);

   TemplateSetNumber("configAddressRangeHigh",val.configAddressRange.high);
   TemplateSetNumber("configAddressRangeLow",val.configAddressRange.low);
   TemplateSetNumber("stateAddressRangeHigh",val.stateAddressRange.high);
   TemplateSetNumber("stateAddressRangeLow",val.stateAddressRange.low);

   ProcessTemplate(file,"../../submodules/VERSAT/software/templates/versat_accelerator_template.tmp");
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath){
   RegisterTypes();

   LockAccelerator(accel);

   #if 0
   printf("\n\n\n\n");
   for(FUDeclaration* decl : versat->declarations){
      printf("%s %d\n",decl->name.str,decl->latency);
   }
   printf("\n\n\n\n");

   VisitAcceleratorInstances(accel,[](FUInstance* inst){
      printf("%s %d %d\n",inst->name.str,inst->baseDelay,inst->delay[0]);
   });

   #endif

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   // No need for templating, small file
   FILE* c = fopen(constantsFilepath,"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",constantsFilepath);
      return;
   }

   fprintf(c,"`define NUMBER_UNITS %d\n",accel->instances.Size());
   fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(c,"`define STATE_W %d\n",val.stateBits);
   fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(c,"`define nIO %d\n",val.nUnitsIO);
   fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);

   fclose(c);

   FILE* s = fopen(sourceFilepath,"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      return;
   }

   FILE* d = fopen(dataFilepath,"w");

   if(!d){
      fclose(s);
      printf("Error creating file, check if filepath is correct: %s\n",dataFilepath);
      return;
   }

   TemplateSetNumber("numberUnits",accel->instances.Size());
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");

   // Output configuration file
   TemplateSetInstancePool("instances",&accel->instances);

   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryMappedUnitAddressRangeHigh",val.memoryMappedUnitAddressRange.high);
   TemplateSetNumber("memoryMappedUnitAddressRangeLow",val.memoryMappedUnitAddressRange.low);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("configAddressRangeHigh",val.configAddressRange.high);
   TemplateSetNumber("configAddressRangeLow",val.configAddressRange.low);
   TemplateSetNumber("stateAddressRangeHigh",val.stateAddressRange.high);
   TemplateSetNumber("stateAddressRangeLow",val.stateAddressRange.low);

   ProcessTemplate(s,"../../submodules/VERSAT/software/templates/versat_top_instance_template.tmp");

   // TODO: Do not forget to remove this #if
   #if 0
   std::vector<FUInstance*> accum;
   VisitAcceleratorInstances(accel,[&](FUInstance* inst){
      if(inst->declaration->type == FUDeclaration::SINGLE){
         FUDeclaration decl = *inst->declaration;

         if(decl.nConfigs || decl.nStates || decl.memoryMapBytes){
            accum.push_back(inst);
         }
      }
   });
   #else
   // Since we do not need access to the lower FUs for the examples, for now, store less data by only storing top level information
   std::vector<FUInstance*> accum;
   for(FUInstance* inst : accel->instances){
      FUDeclaration decl = *inst->declaration;

      if(decl.nConfigs || decl.nStates || decl.memoryMapBytes){
         accum.push_back(inst);
      }
   }
   #endif

   TemplateSetNumber("config",(int)accel->config);
   TemplateSetNumber("state",(int)accel->state);
   TemplateSetNumber("memMapped",(int)accel->memMapped);

   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
   TemplateSetNumber("nConfigs",val.nConfigs);
   TemplateSetInstanceVector("instances",&accum);
   TemplateSetNumber("numberUnits",accum.size());
   ProcessTemplate(d,"../../submodules/VERSAT/software/templates/embedData.tmp");

   fclose(s);
   fclose(d);
}

#define MAX_CHARS 64

void CalculateGraphData(Accelerator* accel){
   Assert(!accel->locked);

   int memoryNeeded = sizeof(GraphComputedData) * accel->instances.Size() + 2 * accel->edges.Size() * sizeof(ConnectionInfo);

   char* mem = (char*) calloc(sizeof(char),memoryNeeded);
   GraphComputedData* computedData = (GraphComputedData*) mem;
   ConnectionInfo* inputBuffer = (ConnectionInfo*) &computedData[accel->instances.Size()];
   ConnectionInfo* outputBuffer = &inputBuffer[accel->edges.Size()];

   // Associate computed data to each instance
   {
      int i = 0;
      for(FUInstance* inst : accel->instances){
         inst->tempData = &computedData[i++];
      }
   }

   // Set inputs and outputs
   for(FUInstance* inst : accel->instances){
      inst->tempData->inputs = inputBuffer;
      inst->tempData->outputs = outputBuffer;

      for(Edge* edge : accel->edges){
         if(edge->units[0].inst == inst){
            outputBuffer->inst = edge->units[1];
            outputBuffer->port = edge->units[0].port;
            outputBuffer += 1;

            inst->tempData->outputPortsUsed = maxi(inst->tempData->outputPortsUsed,edge->units[0].port + 1);
            inst->tempData->numberOutputs += 1;
         }
         if(edge->units[1].inst == inst){
            inputBuffer->inst = edge->units[0];
            inputBuffer->port = edge->units[1].port;
            inputBuffer += 1;

            inst->tempData->inputPortsUsed = maxi(inst->tempData->inputPortsUsed,edge->units[1].port + 1);
            inst->tempData->numberInputs += 1;
         }
      }
   }

   for(FUInstance* inst : accel->instances){
      inst->tempData->nodeType = GraphComputedData::TAG_UNCONNECTED;

      bool hasInput = (inst->tempData->numberInputs > 0);
      bool hasOutput = (inst->tempData->numberOutputs > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(hasInput && hasOutput){
         if(CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY) || CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY)){
            inst->tempData->nodeType = GraphComputedData::TAG_SOURCE_AND_SINK;
         }  else {
            inst->tempData->nodeType = GraphComputedData::TAG_COMPUTE;
         }
      } else if(hasInput){
         inst->tempData->nodeType = GraphComputedData::TAG_SINK;
      } else if(hasOutput){
         inst->tempData->nodeType = GraphComputedData::TAG_SOURCE;
      } else {
         //Assert(0); // Unconnected
      }
   }

   accel->graphDataMem = mem;
}

static int CalculateLatency_(FUInstance* inst,int sourceAndSinkAsSource, std::unordered_map<FUInstance*,int>* memoization){
   if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && sourceAndSinkAsSource){
      return inst->declaration->latency;
   } else if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE){
      return inst->declaration->latency;
   } else if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
      return inst->declaration->latency;
   }

   auto iter = memoization->find(inst);

   if(iter != memoization->end()){
      return iter->second;
   }

   int latency = 0;
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      latency = maxi(latency,CalculateLatency_(inst->tempData->inputs[i].inst.inst,true,memoization));
   }

   int finalLatency = latency + inst->declaration->latency;

   memoization->insert({inst,finalLatency});

   return finalLatency;
}

int CalculateLatency(FUInstance* inst,int sourceAndSinkAsSource){
   std::unordered_map<FUInstance*,int> map;

   return CalculateLatency_(inst,sourceAndSinkAsSource,&map);
}

// Fixes edges such that unit before connected to after, is reconnected to new unit
void InsertUnit(Accelerator* accel, FUInstance* before, int beforePort, FUInstance* after, int afterPort, FUInstance* newUnit){
   Assert(!accel->locked);

   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == before && edge->units[0].port == beforePort && edge->units[1].inst == after && edge->units[1].port == afterPort){
         ConnectUnits(newUnit,0,after,afterPort);

         edge->units[1].inst = newUnit;
         edge->units[1].port = 0;

         return;
      }
   }
}

void SendLatencyUpwards(FUInstance* inst){
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
}

static void SetDelayRecursive(FUInstance* inst,int delay){
   inst->delay[0] = inst->baseDelay + delay;

   if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel != nullptr){
      for(FUInstance* child : inst->compositeAccel->instances){
         SetDelayRecursive(child,delay);
      }
   }
}

void CalculateDelay(Versat* versat,Accelerator* accel){
   LockAccelerator(accel);

   DAGOrder order = accel->order;

   // Calculate latency
   #if 1
   int maxLatency = 0;
   for(int i = 0; i < order.numberSinks; i++){
      FUInstance* inst = order.sinks[i];

      int latency = 0;
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         latency = CalculateLatency(inst,false);
      } else {
         latency = CalculateLatency(inst,true);
      }

      maxLatency = maxi(maxLatency,latency);

      inst->tempData->inputDelay = (latency - inst->declaration->latency);

      Assert(inst->tempData->inputDelay >= 0);

      SendLatencyUpwards(inst);
   }
   #endif

   // Send latency upwards on DAG
   for(int i = accel->instances.Size() - order.numberSinks - 1; i >= 0; i--){
      FUInstance* inst = order.instancePtrs[i];

      if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         continue;
      }

      int minimum = (1 << 25);

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         minimum = mini(minimum,inst->tempData->outputs[ii].delay);
      }

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         inst->tempData->outputs[ii].delay -= minimum;
      }

      inst->tempData->inputDelay = minimum - inst->declaration->latency;

      Assert(inst->tempData->inputDelay >= 0);

      SendLatencyUpwards(inst);
   }

   // If source units cannot meet delay requirements (cannot have source delay), send input delay back to outputs
   for(int i = 0; i < order.numberSources; i++){
      FUInstance* inst = order.sources[i];

      if(!CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY)){
         for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
            inst->tempData->outputs[ii].delay += inst->tempData->inputDelay;
         }
         inst->tempData->inputDelay = 0;
      } else {
         inst->baseDelay = inst->tempData->inputDelay;

         inst->tempData->inputDelay = 0;
      }
   }

   #if 0
   // Set absurd high delay to units not supposed to have delay active (sink but not sink delay, source but not source delay)
   for(FUInstance* inst : accel->instances){
      if(inst->tempData && inst->tempData->nodeType == GraphComputedData::TAG_SINK && !CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY)){
         inst->baseDelay = (1<<29);
      }
      if(inst->tempData && inst->tempData->nodeType == GraphComputedData::TAG_SOURCE && !CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY)){
         inst->baseDelay = (1<<29);
      }
   }
   #endif

   #if 0
   for(FUInstance* inst : accel->instances){
      printf("%s %d\n",inst->name.str,inst->tempData->inputDelay);
      #if 1
      for(int i = 0; i < inst->tempData->numberOutputs; i++){
         printf(">%d\n",inst->tempData->outputs[i].delay);
      }
      #endif
   }
   printf("\n");
   #endif

   #if 1
   for(int i = 0; i < order.numberSources; i++){
      FUInstance* inst = order.sources[i];

      if(CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY)){
         inst->delay[0] = inst->tempData->inputDelay;
      }
   }
   #endif

   #if 1
   accel->locked = false;
   // Insert delay units if needed
   int delaysInserted = 0;
   for(int i = accel->instances.Size() - 1; i >= 0; i--){
      FUInstance* inst = order.instancePtrs[i];

      //For now, I think it only makes sense for computing and source units to add any sort of delay
      if(inst->tempData->nodeType != GraphComputedData::TAG_COMPUTE && inst->declaration->type == FUDeclaration::SPECIAL && inst->tempData->nodeType != GraphComputedData::TAG_SOURCE){
         continue;
      }

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         ConnectionInfo* info = &inst->tempData->outputs[ii];

         if(info->delay != 0){
            char buffer[128];
            int size = sprintf(buffer,"delay%d",delaysInserted++);

            FUInstance* newInst = CreateShallowNamedFUInstance(accel,versat->delay,MakeSizedString(buffer,size),nullptr);

            InsertUnit(accel,inst,info->port,info->inst.inst,info->inst.port,newInst);

            newInst->baseDelay = info->delay - versat->delay->latency;
            Assert(newInst->baseDelay >= 0);
         }
      }
   }
   #endif

   for(FUInstance* inst : accel->instances){
      if(inst->declaration != versat->delay){
         SetDelayRecursive(inst,inst->tempData->inputDelay);
      }
   }

   accel->cyclesPerRun = maxLatency;
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

   LockAccelerator(accel1);
   LockAccelerator(accel2);

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

void AddMapping(Mapping* mappings, FUInstance* source,FUInstance* sink){
   for(int i = 0; i < 1024; i++){
      if(mappings[i].source == nullptr){
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

   return nullptr;
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

   return nullptr;
}

Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph){
   Mapping* graphToFinal = (Mapping*) calloc(sizeof(Mapping),1024);

   InstanceMap map = {};

   Accelerator* newGraph = CreateAccelerator(versat);

   LockAccelerator(accel1);
   LockAccelerator(accel2);

   // Create base instances (accel 1)
   for(FUInstance* inst : accel1->instances){
      FUInstance* newNode = CreateFUInstance(newGraph,inst->declaration);

      map.insert({inst,newNode});
   }

   #if 1
   for(FUInstance* inst : accel2->instances){
      FUInstance* mappedNode = NodeMapped(inst,graph); // Returns node in graph 1

      if(mappedNode){
         mappedNode = map.find(mappedNode)->second;
      } else {
         mappedNode = CreateFUInstance(newGraph,inst->declaration);
      }

      AddMapping(graphToFinal,inst,mappedNode);
   }
   #endif

   // Add edges from accel1
   for(FUInstance* inst : accel1->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         FUInstance* other = inst->tempData->outputs[ii].inst.inst;

         FUInstance* mappedInst = map.find(inst)->second;
         FUInstance* mappedOther = map.find(other)->second;

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }

   #if 1
   for(FUInstance* inst : accel2->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         FUInstance* other = inst->tempData->outputs[ii].inst.inst;

         FUInstance* mappedInst =  map.find(inst)->second;
         FUInstance* mappedOther = map.find(other)->second;

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }
   #endif

   return newGraph;
}

#undef NUMBER_OUTPUTS
#undef OUTPUT




























