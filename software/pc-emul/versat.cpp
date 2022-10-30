#include "versatPrivate.hpp"

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "type.hpp"
#include "codeGeneration.hpp"
#include "debug.hpp"
#include "parser.hpp"

#include <printf.h>
#include <unordered_map>

#include <utility>

#define IMPLEMENT_VERILOG_UNITS
#include "verilogWrapper.inc"

#define DELAY_BIT_SIZE 8

static int versat_base;

// Implementations of units that versat needs to know about explicitly

#define MAX_DELAY 128

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;

template<> class std::hash<PortInstance>{
   public:
   std::size_t operator()(PortInstance const& s) const noexcept{
      int res = SimpleHash(MakeSizedString((const char*) &s,sizeof(PortInstance)));

      return (std::size_t) res;
   }
};

bool operator<(const StaticInfo& left,const StaticInfo& right){
   bool res = std::tie(left.module,left.name,left.wires,left.ptr) < std::tie(right.module,right.name,right.wires,right.ptr);

   return res;
}

static int zeros[100] = {};

static int* DefaultInitFunction(ComplexFUInstance* inst){
   inst->done = true;
   return nullptr;
}

static FUDeclaration* RegisterCircuitInput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = MakeSizedString("CircuitInput");
   decl.nOutputs = 99;
   decl.nInputs = 1;  // Used for templating circuit
   decl.latencies = zeros;
   decl.inputDelays = zeros;
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SOURCE_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterCircuitOutput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = MakeSizedString("CircuitOutput");
   decl.nInputs = 99;
   decl.nOutputs = 99; // Used for templating circuit
   decl.latencies = zeros;
   decl.inputDelays = zeros;
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}

int* UnaryNot(ComplexFUInstance* inst){
    static uint out;
    out = ~GetInputValue(inst,0);
    inst->done = true;
    return (int*) &out;
}

int* BinaryXOR(ComplexFUInstance* inst){
    static uint out;
    out = GetInputValue(inst,0) ^ GetInputValue(inst,1);
    inst->done = true;
    return (int*) &out;
}

int* BinaryADD(ComplexFUInstance* inst){
    static uint out;
    out = GetInputValue(inst,0) + GetInputValue(inst,1);
    inst->done = true;
    return (int*) &out;
}
int* BinaryAND(ComplexFUInstance* inst){
    static uint out;
    out = GetInputValue(inst,0) & GetInputValue(inst,1);
    inst->done = true;
    return (int*) &out;
}
int* BinaryOR(ComplexFUInstance* inst){
    static uint out;
    out = GetInputValue(inst,0) | GetInputValue(inst,1);
    inst->done = true;
    return (int*) &out;
}
int* BinaryRHR(ComplexFUInstance* inst){
    static uint out;
    uint value = GetInputValue(inst,0);
    uint shift = GetInputValue(inst,1);
    out = (value >> shift) | (value << (32 - shift));
    inst->done = true;
    return (int*) &out;
}
int* BinaryRHL(ComplexFUInstance* inst){
    static uint out;
    uint value = GetInputValue(inst,0);
    uint shift = GetInputValue(inst,1);
    out = (value << shift) | (value >> (32 - shift));
    inst->done = true;
    return (int*) &out;
}
int* BinarySHR(ComplexFUInstance* inst){
    static uint out;
    uint value = GetInputValue(inst,0);
    uint shift = GetInputValue(inst,1);
    out = (value >> shift);
    inst->done = true;
    return (int*) &out;
}
int* BinarySHL(ComplexFUInstance* inst){
    static uint out;
    uint value = GetInputValue(inst,0);
    uint shift = GetInputValue(inst,1);
    out = (value << shift);
    inst->done = true;
    return (int*) &out;
}

void RegisterOperators(Versat* versat){
   const char* unary[] = {"NOT"};
   FUFunction unaryF[] = {UnaryNot};
   const char* binary[] = {"XOR","ADD","AND","OR","RHR","SHR","RHL","SHL"};
   FUFunction binaryF[] = {BinaryXOR,BinaryADD,BinaryAND,BinaryOR,BinaryRHR,BinarySHR,BinaryRHL,BinarySHL};
   const char* binaryOperation[] = {"^","+","&","|","<<",">>","<<",">>"};

   FUDeclaration decl = {};
   decl.nOutputs = 1;
   decl.nInputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.isOperation = true;
   for(unsigned int i = 0; i < ARRAY_SIZE(unary); i++){
      decl.name = MakeSizedString(unary[i]);
      decl.updateFunction = unaryF[i];
      decl.operation = "~";
      RegisterFU(versat,decl);
   }

   decl.nInputs = 2;
   for(unsigned int i = 0; i < ARRAY_SIZE(binary); i++){
      decl.name = MakeSizedString(binary[i]);
      decl.updateFunction = binaryF[i];
      decl.operation = binaryOperation[i];
      RegisterFU(versat,decl);
   }
}

Versat* InitVersat(int base,int numberConfigurations){
   static Versat versatInst = {};
   static bool doneOnce = false;

   Assert(!doneOnce); // For now, only allow one Versat instance
   doneOnce = true;

   Versat* versat = &versatInst;

   versat->debug.outputAccelerator = true;
   versat->debug.outputVersat = true;

   RegisterTypes();

   versat->numberConfigurations = numberConfigurations;
   versat->base = base;
   versat_base = base;

   InitArena(&versat->temp,Megabyte(256));
   InitArena(&versat->permanent,Megabyte(256));

   FUDeclaration nullDeclaration = {};
   nullDeclaration.latencies = zeros;
   nullDeclaration.inputDelays = zeros;
   RegisterFU(versat,nullDeclaration);

   RegisterAllVerilogUnits(versat);

   versat->buffer = GetTypeByName(versat,MakeSizedString("Buffer"));
   versat->fixedBuffer = GetTypeByName(versat,MakeSizedString("FixedBuffer"));
   versat->pipelineRegister = GetTypeByName(versat,MakeSizedString("PipelineRegister"));
   versat->multiplexer = GetTypeByName(versat,MakeSizedString("Mux2"));
   versat->input = RegisterCircuitInput(versat);
   versat->output = RegisterCircuitOutput(versat);

   RegisterOperators(versat);

   Log(LogModule::VERSAT,LogLevel::INFO,"Init versat");

   return versat;
}

void Free(Versat* versat){
   for(Accelerator* accel : versat->accelerators){
      if(accel->type == Accelerator::CIRCUIT){
         continue;
      }

      #if 1
      //if(accel->type != Accelerator::CIRCUIT){
         for(ComplexFUInstance* inst : accel->instances){
            if(inst->initialized && inst->declaration->destroyFunction){
               inst->declaration->destroyFunction(inst);
            }
         }
      //}
      #endif
   }

   for(Accelerator* accel : versat->accelerators){
      LockAccelerator(accel,Accelerator::FREE,true);

      Free(&accel->configAlloc);
      Free(&accel->stateAlloc);
      Free(&accel->delayAlloc);
      Free(&accel->staticAlloc);
      Free(&accel->outputAlloc);
      Free(&accel->storedOutputAlloc);
      Free(&accel->extraDataAlloc);

      accel->instances.Clear(true);
      accel->edges.Clear(true);
      accel->inputInstancePointers.Clear(true);
      accel->staticInfo.Clear(true);
   }

   for(FUDeclaration* decl : versat->declarations){
      decl->staticUnits.Clear(true);
   }

   versat->accelerators.Clear(true);
   versat->declarations.Clear(true);

   Free(&versat->temp);
   Free(&versat->permanent);

   FreeTypes();

   CheckMemoryStats();

}

void SetDefaultConfiguration(FUInstance* instance,int* config,int size){
   ComplexFUInstance* inst = (ComplexFUInstance*) instance;

   inst->config = config;
   inst->savedConfiguration = true;
}

void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex){
   ComplexFUInstance* inst = (ComplexFUInstance*) instance;

   inst->sharedIndex = shareBlockIndex;
   inst->sharedEnable = true;
}

void ParseCommandLineOptions(Versat* versat,int argc,const char** argv){
   #if 0
   for(int i = 0; i < argc; i++){
      printf("Arg %d: %s\n",i,argv[i]);
   }
   #endif

   for(int i = 1; i < argc; i++){
      versat->includeDirs.push_back(argv[i]);
   }
}

bool SetDebug(Versat* versat,VersatDebugFlags flags,bool flag){
   bool last;
   switch(flags){
   case VersatDebugFlags::OUTPUT_GRAPH_DOT:{
      last = versat->debug.outputGraphs;
      versat->debug.outputGraphs = flag;
   }break;
   case VersatDebugFlags::OUTPUT_ACCELERATORS_CODE:{
      last = versat->debug.outputAccelerator;
      versat->debug.outputAccelerator = flag;
   }break;
   case VersatDebugFlags::OUTPUT_VERSAT_CODE:{
      last = versat->debug.outputVersat;
      versat->debug.outputVersat = flag;
   }break;
   case VersatDebugFlags::OUTPUT_VCD:{
      last = versat->debug.outputVCD;
      versat->debug.outputVCD = flag;
   }break;
   case VersatDebugFlags::USE_FIXED_BUFFERS:{
      last = versat->debug.useFixedBuffers;
      versat->debug.useFixedBuffers = flag;
   }break;
   default:{
      NOT_POSSIBLE;
   }break;
   }

   return last;
}

static int AccessMemory(FUInstance* inst,int address, int value, int write){
   ComplexFUInstance* instance = (ComplexFUInstance*) inst;

   int res = instance->declaration->memAccessFunction(instance,address,value,write);

   return res;
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   if(instance->declaration->type == FUDeclaration::COMPOSITE){
      int offset = 0;

      for(FUInstance* inst : instance->compositeAccel->instances){
         if(!inst->declaration->isMemoryMapped){
            continue;
         }

         int mappedWords = 1 << inst->declaration->memoryMapBits;
         if(mappedWords){
            if(address >= offset && address <= offset + mappedWords){
               VersatUnitWrite(inst,address - offset,value);
            } else {
               offset += mappedWords;
            }
         }
      }
   } else {
      AccessMemory(instance,address,value,1);
   }
}

int VersatUnitRead(FUInstance* instance,int address){
   int res = AccessMemory(instance,address,0,0);
   return res;
}

FUDeclaration* GetTypeByName(Versat* versat,SizedString name){
   for(FUDeclaration* decl : versat->declarations){
      if(CompareString(decl->name,name)){
         return decl;
      }
   }

   Log(LogModule::VERSAT,LogLevel::FATAL,"[GetTypeByName] Didn't find the following type: %.*s",UNPACK_SS(name));

   return nullptr;
}

struct HierarchicalName{
   SizedString name;
   HierarchicalName* next;
};

static FUInstance* GetInstanceByHierarchicalName(Accelerator* accel,HierarchicalName* hier){
   Assert(hier != nullptr);

   HierarchicalName* savedHier = hier;
   FUInstance* res = nullptr;
   for(FUInstance* inst : accel->instances){
      Tokenizer tok(inst->name,"./",{});

      while(true){
         // Unpack individual name
         hier = savedHier;
         while(true){
            Token name = tok.NextToken();

            // Unpack hierarchical name
            Tokenizer hierTok(hier->name,":",{});
            Token hierName = hierTok.NextToken();

            if(!CompareString(name,hierName)){
               break;
            }

            Token possibleTypeQualifier = hierTok.PeekToken();

            if(CompareString(possibleTypeQualifier,":")){
               hierTok.AdvancePeek(possibleTypeQualifier);

               Token type = hierTok.NextToken();

               if(!CompareString(type,inst->declaration->name)){
                  break;
               }
            }

            Token possibleDot = tok.PeekToken();

            // If hierarchical name, need to advance through hierarchy
            if(CompareString(possibleDot,".") && hier->next){
               tok.AdvancePeek(possibleDot);
               Assert(hier); // Cannot be nullptr

               hier = hier->next;
               continue;
            } else if(inst->compositeAccel && hier->next){
               FUInstance* res = GetInstanceByHierarchicalName(inst->compositeAccel,hier->next);
               if(res){
                  return res;
               }
            } else if(!hier->next){ // Correct name and type (if specified) and no further hierarchical name to follow
               return inst;
            }
         }

         // Check if multiple names
         Token possibleDuplicateName = tok.PeekFindIncluding("/");
         if(possibleDuplicateName.size > 0){
            tok.AdvancePeek(possibleDuplicateName);
         } else {
            break;
         }
      }
   }

   return res;
}

static FUInstance* vGetInstanceByName_(Accelerator* circuit,int argc,va_list args){
   Arena* arena = &circuit->versat->temp;

   HierarchicalName fullName = {};
   HierarchicalName* namePtr = &fullName;
   HierarchicalName* lastPtr = nullptr;

   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);
      int arguments = parse_printf_format(str,0,nullptr);

      if(namePtr == nullptr){
         HierarchicalName* newBlock = PushStruct(arena,HierarchicalName);

         lastPtr->next = newBlock;
         namePtr = newBlock;
      }

      SizedString name = {};
      if(arguments){
         name = vPushString(arena,str,args);
         i += arguments;
         for(int ii = 0; ii < arguments; ii++){
            va_arg(args, int); // Need to consume something
         }
      } else {
         name = PushString(arena,"%s",str);
      }

      Tokenizer tok(name,".",{});
      Token hasDot = tok.PeekFindIncluding(".");

      while(!tok.Done()){
         if(namePtr == nullptr){
            HierarchicalName* newBlock = PushStruct(arena,HierarchicalName);

            lastPtr->next = newBlock;
            namePtr = newBlock;
         }

         Token name = tok.NextToken();

         namePtr->name = name;
         lastPtr = namePtr;
         namePtr = namePtr->next;

         Token peek = tok.PeekToken();
         if(CompareString(peek,".")){
            tok.AdvancePeek(peek);
            continue;
         }

         break;
      }
   }

   FUInstance* res = GetInstanceByHierarchicalName(circuit,&fullName);

   if(!res){
      GetInstanceByHierarchicalName(circuit,&fullName);
      Assert(false);
   }

   return res;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){
   va_list args;
   va_start(args,argc);

   FUInstance* res = vGetInstanceByName_(circuit,argc,args);

   va_end(args);

   return res;
}

FUInstance* GetInstanceByName_(FUInstance* instance,int argc, ...){
   FUInstance* inst = (FUInstance*) instance;

   Assert(inst->compositeAccel);

   va_list args;
   va_start(args,argc);

   FUInstance* res = vGetInstanceByName_(inst->compositeAccel,argc,args);

   va_end(args);

   return res;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   FUDeclaration* type = versat->declarations.Alloc();
   *type = decl;

   if(decl.nInputs){
      Assert(decl.inputDelays);
   }

   if(decl.nOutputs){
      Assert(decl.latencies);
   }

   if(type->type != FUDeclaration::COMPOSITE){
      type->nTotalOutputs = type->nOutputs;
   }

   return type;
}

// Uses static allocated memory. Intended for use by OutputGraphDotFile
static char* FormatNameToOutput(Versat* versat,FUInstance* inst){
   #define PRINT_ID 0
   #define PRINT_DELAY 1

   static char buffer[1024];

   char* ptr = buffer;
   ptr += sprintf(ptr,"%.*s",UNPACK_SS(inst->name));

   #if PRINT_ID == 1
   ptr += sprintf(ptr,"_%d",inst->id);
   #endif

   #if PRINT_DELAY == 1
   if(inst->declaration == versat->buffer && inst->config){
      ptr += sprintf(ptr,"_%d",inst->config[0]);
   } else if(inst->declaration == versat->fixedBuffer && inst->parameters.size > 0){
      Tokenizer tok(inst->parameters,".()",{"#("});

      while(!tok.Done()){
         Token amountParam = tok.NextToken();

         if(CompareString(amountParam,"AMOUNT")){
            tok.AssertNextToken("(");

            Token number = tok.NextToken();

            tok.AssertNextToken(")");

            ptr += sprintf(ptr,"_%.*s",UNPACK_SS(number));
            break;
         }
      }

   } else {
      ptr += sprintf(ptr,"_%d",inst->baseDelay);
   }
   #endif

   #undef PRINT_ID
   #undef PRINT_DELAY

   return buffer;
}

static void OutputGraphDotFile_(Versat* versat,Accelerator* accel,bool collapseSameEdges,FILE* outputFile){
   LockAccelerator(accel,Accelerator::Locked::GRAPH);

   fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(ComplexFUInstance* inst : accel->instances){
      char* name = FormatNameToOutput(versat,inst);

      fprintf(outputFile,"\t\"%s\";\n",name);
   }

   std::set<std::pair<ComplexFUInstance*,ComplexFUInstance*>> sameEdgeCounter;

   for(Edge* edge : accel->edges){
      if(collapseSameEdges){
         std::pair<ComplexFUInstance*,ComplexFUInstance*> key{edge->units[0].inst,edge->units[1].inst};

         if(sameEdgeCounter.count(key) == 1){
            continue;
         }

         sameEdgeCounter.insert(key);
      }

      fprintf(outputFile,"\t\"%s\" -> ",FormatNameToOutput(versat,edge->units[0].inst));
      fprintf(outputFile,"\"%s\"",FormatNameToOutput(versat,edge->units[1].inst));

      #if 1
      ComplexFUInstance* outputInst = edge->units[0].inst;
      int delay = 0;
      for(int i = 0; i < outputInst->tempData->numberOutputs; i++){
         if(edge->units[1].inst == outputInst->tempData->outputs[i].instConnectedTo.inst && edge->units[1].port == outputInst->tempData->outputs[i].instConnectedTo.port){
            delay = outputInst->tempData->outputs[i].delay;
            break;
         }
      }

      fprintf(outputFile,"[label=\"%d\"]",delay);
      //fprintf(outputFile,"[label=\"[%d:%d;%d:%d]\"]",outputInst->declaration->latencies[0],delay,edge->units[1].inst->declaration->inputDelays[edge->units[1].port],edge->delay);
      #endif

      fprintf(outputFile,";\n");
   }

   fprintf(outputFile,"}\n");
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...){
   char buffer[1024];

   va_list args;
   va_start(args,filenameFormat);

   vsprintf(buffer,filenameFormat,args);

   FILE* file = fopen(buffer,"w");
   OutputGraphDotFile_(versat,accel,collapseSameEdges,file);
   fclose(file);

   va_end(args);
}

Accelerator* CreateAccelerator(Versat* versat){
   Accelerator* accel = versat->accelerators.Alloc();
   accel->versat = versat;

   return accel;
}

struct FUInstanceInterfaces{
   PushPtr<int> config;
   PushPtr<int> state;
   PushPtr<int> delay;
   PushPtr<int> outputs;
   PushPtr<int> storedOutputs;
   PushPtr<int> statics;
   PushPtr<Byte> extraData;
};

// Forward declare
void PopulateAcceleratorRecursive(FUDeclaration* accelType,ComplexFUInstance* inst,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated);

struct SharingInfo{
   int* ptr;
   bool init;
};

void DoPopulate(Accelerator* accel,FUDeclaration* accelType,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated){
   std::vector<SharingInfo> sharingInfo;
   for(ComplexFUInstance* inst : accel->instances){
      SharingInfo* info = nullptr;
      PushPtr<int> savedConfig = in.config;

      if(inst->sharedEnable){
         int index = inst->sharedIndex;

         if(index >= sharingInfo.size()){
            sharingInfo.resize(index + 1);
         }

         info = &sharingInfo[index];

         if(info->init){ // Already exists, replace config with ptr
            in.config.Init(info->ptr,inst->declaration->nConfigs);
         } else {
            info->ptr = in.config.Push(0);
         }
      }

      PopulateAcceleratorRecursive(accelType,inst,in,staticsAllocated);

      if(inst->sharedEnable){
         if(info->init){
            in.config = savedConfig;
         }
         info->init = true;
      }
   }
}

void PopulateAcceleratorRecursive(FUDeclaration* accelType,ComplexFUInstance* inst,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated){
   FUDeclaration* type = inst->declaration;
   PushPtr<int> saved = in.config;

   int nConfigs  = inst->compositeAccel ? 0 : type->nConfigs;
   int nStates   = inst->compositeAccel ? 0 : type->nStates;
   int nDelays   = inst->compositeAccel ? 0 : type->nDelays;
   int nOutputs  = type->nOutputs;
   int extraData = inst->compositeAccel ? 0 : type->extraDataSize;

   bool foundStatic = false;
   if(inst->isStatic){
      Assert(accelType);

      for(StaticInfo* info : staticsAllocated){
         if(info->module == accelType && CompareString(info->name,inst->name)){
            in.config.Init(info->ptr,info->nConfigs);
            foundStatic = true;
            break;
         }
      }

      if(!foundStatic){
         StaticInfo* allocation = staticsAllocated.Alloc();
         allocation->module = accelType;
         allocation->name = inst->name;
         allocation->nConfigs = inst->declaration->nConfigs;
         allocation->ptr = in.statics.Push(0);
         allocation->wires = inst->declaration->configWires;

         in.config = in.statics;
      }
   }

   // Accelerator instance doesn't allocate memory, it shares memory with sub units
   if(inst->compositeAccel || nConfigs){
      inst->config = in.config.Push(nConfigs);
   }
   if(inst->compositeAccel || nStates){
      inst->state = in.state.Push(nStates);
   }
   if(inst->compositeAccel || nDelays){
      inst->delay = in.delay.Push(nDelays);
   }

   // Except for outputs, each unit has it's own output memory
   if(nOutputs){
      inst->outputs = in.outputs.Push(nOutputs);
      inst->storedOutputs = in.storedOutputs.Push(nOutputs);
   }
   if(extraData){
      inst->extraData = in.extraData.Push(extraData);
   }

   #if 1
   if(!inst->initialized && type->initializeFunction){
      type->initializeFunction(inst);
      inst->initialized = true;
   }
   #endif

   if(inst->compositeAccel){
      FUDeclaration* newAccelType = inst->declaration;

      DoPopulate(inst->compositeAccel,newAccelType,in,staticsAllocated);
   }

   if(inst->declarationInstance && ((ComplexFUInstance*) inst->declarationInstance)->savedConfiguration){
      memcpy(inst->config,inst->declarationInstance->config,inst->declaration->nConfigs * sizeof(int));
   }

   if(inst->isStatic){
      if(!foundStatic){
         in.statics = in.config;
      }
      in.config = saved;
   }

   //DisplayInstanceMemory(inst);
}

void InitializeFUInstances(Accelerator* accel,bool force){
   #if 1
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      FUDeclaration* type = inst->declaration;
      if((force && type->initializeFunction) || (!inst->initialized && type->initializeFunction)){
         type->initializeFunction(inst);
         inst->initialized = true;
      }
   }
   #endif
}

struct AcceleratorValues{
   int inputs;
   int outputs;

   int configs;
   int states;
   int delays;
   int ios;
   int extraData;
   int totalOutputs;
   int statics;

   int memoryMappedBits;
   bool isMemoryMapped;
};

struct StaticId{
   FUDeclaration* parent;
   SizedString name;
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      int res = SimpleHash(s.name);
      res += (int) s.parent;

      return (std::size_t) res;
   }
};

bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}

AcceleratorValues ComputeAcceleratorValues(Versat* versat,Accelerator* accel){
   AcceleratorValues val = {};

   std::vector<StaticId> staticSeen;
   std::vector<bool> seenShared;

   int memoryMapBits[32];
   memset(memoryMapBits,0,sizeof(int) * 32);

   // Handle non-static information
   for(ComplexFUInstance* inst : accel->instances){
      FUDeclaration* type = inst->declaration;

      // Check if shared
      if(inst->sharedEnable){
         if(inst->sharedIndex >= seenShared.size()){
            seenShared.resize(inst->sharedIndex + 1);
         }

         if(!seenShared[inst->sharedIndex]){
            val.configs += inst->declaration->nConfigs;
         }

         seenShared[inst->sharedIndex] = true;
      } else if(!inst->isStatic){ // Shared cannot be static
         val.configs += type->nConfigs;
      }

      if(type->isMemoryMapped){
         memoryMapBits[type->memoryMapBits] += 1;
      }

      if(type == versat->input){
         val.inputs += 1;
      }

      #if 0
      if(type->nConfigs){
         printf("here\n");
      }
      #endif

      val.states += type->nStates;
      val.delays += type->nDelays;
      val.ios += type->nIOs;
      val.extraData += type->extraDataSize;
      val.totalOutputs += type->nTotalOutputs;
   }

   if(accel->outputInstance){
      for(Edge* edge : accel->edges){
         if(edge->units[0].inst == accel->outputInstance){
            val.outputs = maxi(val.outputs - 1,edge->units[0].port) + 1;
         }
         if(edge->units[1].inst == accel->outputInstance){
            val.outputs = maxi(val.outputs - 1,edge->units[1].port) + 1;
         }
      }
   }
   val.totalOutputs += val.outputs;

   // Handle static information
   AcceleratorIterator iter = {};
   for(FUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      if(!inst->isStatic){
         continue;
      }

      StaticId id = {};

      id.name = inst->name;
      id.parent = iter.CurrentAcceleratorInstance()->declaration;

      StaticId* found = nullptr;
      for(StaticId& search : staticSeen){
         if(CompareString(id.name,search.name) && id.parent == search.parent){
            found = &search;
         }
      }

      if(!found){
         staticSeen.push_back(id);
         val.statics += inst->declaration->nConfigs;
      }
   }

   // Huffman encoding for memory mapping bits
   int last = -1;
   while(1){
      for(int i = 0; i < 32; i++){
         if(memoryMapBits[i]){
            memoryMapBits[i+1] += (memoryMapBits[i] / 2);
            memoryMapBits[i] = memoryMapBits[i] % 2;
            last = i;
         }
      }

      int first = -1;
      int second = -1;
      for(int i = 0; i < 32; i++){
         if(first == -1 && memoryMapBits[i] == 1){
            first = i;
         } else if(second == -1 && memoryMapBits[i] == 1){
            second = i;
            break;
         }
      }

      if(second == -1){
         break;
      }

      memoryMapBits[first] = 0;
      memoryMapBits[second] = 0;
      memoryMapBits[maxi(first,second) + 1] += 1;
   }

   if(last != -1){
      val.isMemoryMapped = true;
      val.memoryMappedBits = last;
   }

   return val;
}

void PopulateAccelerator(Versat* versat,Accelerator* accel){
   #if 1
   if(accel->type == Accelerator::CIRCUIT){
      return;
   }
   #endif

   // Accel should be top level
   AcceleratorValues val = ComputeAcceleratorValues(versat,accel);

   #if 1
   ZeroOutRealloc(&accel->configAlloc,val.configs);
   ZeroOutRealloc(&accel->stateAlloc,val.states);
   ZeroOutRealloc(&accel->delayAlloc,val.delays);
   ZeroOutRealloc(&accel->outputAlloc,val.totalOutputs);
   ZeroOutRealloc(&accel->storedOutputAlloc,val.totalOutputs);
   ZeroOutRealloc(&accel->extraDataAlloc,val.extraData);
   ZeroOutRealloc(&accel->staticAlloc,val.statics);
   #endif

   FUInstanceInterfaces inter = {};

   #if 1
   inter.config.Init(accel->configAlloc);
   inter.state.Init(accel->stateAlloc);
   inter.delay.Init(accel->delayAlloc);
   inter.outputs.Init(accel->outputAlloc);
   inter.storedOutputs.Init(accel->storedOutputAlloc);
   inter.extraData.Init(accel->extraDataAlloc);
   inter.statics.Init(accel->staticAlloc);
   #endif

   //printf("\n\n==\n\n");

   accel->staticInfo.Clear();
   // Assuming no static units on top, for now
   DoPopulate(accel,accel->subtype,inter,accel->staticInfo);

#if 1
   Assert(inter.config.Empty());
   Assert(inter.state.Empty());
   Assert(inter.delay.Empty());
   Assert(inter.outputs.Empty());
   Assert(inter.storedOutputs.Empty());
   Assert(inter.extraData.Empty());
   Assert(inter.statics.Empty());
#endif
}

void CheckMemory(Accelerator* topLevel,Accelerator* accel){
   //AcceleratorIterator iter = {};
   //for(FUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){

   for(ComplexFUInstance* inst : accel->instances){
      printf("%.*s:\n",UNPACK_SS(inst->name));
      if(inst->isStatic){
         printf("C:%d\n",inst->config ? inst->config - topLevel->staticAlloc.ptr : -1);
      } else {
         printf("C:%d\n",inst->config ? inst->config - topLevel->configAlloc.ptr : -1);
      }

      printf("S:%d\n",inst->state ? inst->state - topLevel->stateAlloc.ptr : -1);
      printf("D:%d\n",inst->delay ? inst->delay - topLevel->delayAlloc.ptr : -1);
      printf("O:%d\n",inst->outputs ? inst->outputs - topLevel->outputAlloc.ptr : -1);
      printf("o:%d\n",inst->storedOutputs ? inst->storedOutputs - topLevel->storedOutputAlloc.ptr : -1);
      printf("E:%d\n",inst->extraData ? inst->extraData - topLevel->extraDataAlloc.ptr : -1);
   }
}

static ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,SizedString newName,bool flat){
   ComplexFUInstance* newInst = (ComplexFUInstance*) CreateFUInstance(newAccel,oldInstance->declaration,newName,flat);

   newInst->baseDelay = oldInstance->baseDelay;
   newInst->isStatic = oldInstance->isStatic;
   newInst->sharedEnable = oldInstance->sharedEnable;
   newInst->sharedIndex = oldInstance->sharedIndex;

   return newInst;
}

#if 1
static Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map,bool flat){
   Accelerator* newAccel = CreateAccelerator(versat);
   InstanceMap nullCaseMap;

   if(map == nullptr){
      map = &nullCaseMap;
   }

   // Copy of instances
   for(ComplexFUInstance* inst : accel->instances){
      ComplexFUInstance* newInst = CopyInstance(newAccel,inst,inst->name,flat);

      newInst->declarationInstance = inst;

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   for(Edge* edge : accel->edges){
      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = (ComplexFUInstance*) map->at(edge->units[0].inst);
      newEdge->units[1].inst = map->at(edge->units[1].inst);
   }

   // Copy of input instance pointers
   for(ComplexFUInstance** instPtr : accel->inputInstancePointers){
      ComplexFUInstance** newInstPtr = newAccel->inputInstancePointers.Alloc();

      *newInstPtr = map->at(*instPtr);
   }

   if(accel->outputInstance){
      newAccel->outputInstance = map->at(accel->outputInstance);
   }

   return newAccel;
}
#endif

static bool CheckName(SizedString name){
   for(int i = 0; i < name.size; i++){
      char ch = name.str[i];

      bool allowed = (ch >= 'a' && ch <= 'z')
                 ||  (ch >= 'A' && ch <= 'Z')
                 ||  (ch >= '0' && ch <= '9' && i != 0)
                 ||  (ch == '_')
                 ||  (ch == '.')  // For now allow it, despite the fact that it should only be used internally by Versat
                 ||  (ch == '/'); // For now allow it, despite the fact that it should only be used internally by Versat

      if(!allowed){
         return false;
      }
   }

   return true;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString name,bool flat,bool isStatic){
   LockAccelerator(accel,Accelerator::Locked::FREE);

   Assert(CheckName(name));

   ComplexFUInstance* ptr = accel->instances.Alloc();

   ptr->name = name;
   ptr->id = accel->entityId++;
   ptr->accel = accel;
   ptr->declaration = type;
   ptr->namedAccess = true;
   ptr->isStatic = isStatic;

   if(type->type == FUDeclaration::COMPOSITE){
      //ptr->compositeAccel = InstantiateAccelerator(accel->versat,&type->baseCircuit);
      ptr->compositeAccel = CopyAccelerator(accel->versat,type->fixedDelayCircuit,nullptr,true);
   }

   #if 1
   if(!flat){
      PopulateAccelerator(accel->versat,accel);
   }
   #endif

   return ptr;
}

void RemoveFUInstance(Accelerator* accel,ComplexFUInstance* inst){
   #if 1
   LockAccelerator(accel,Accelerator::Locked::FREE);

   //TODO: Remove instance doesn't update the config / state / memMapped / delay pointers
   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == inst){
         accel->edges.Remove(edge);
      } else if(edge->units[1].inst == inst){
         accel->edges.Remove(edge);
      }
   }

   accel->instances.Remove(inst);
   #endif
}

static StaticInfo* SetLikeInsert(Pool<StaticInfo>& vec,StaticInfo& info){
   for(StaticInfo* iter : vec){
      if(info.module == iter->module && CompareString(info.name,iter->name)){
         return iter;
      }
   }

   StaticInfo* res = vec.Alloc();
   *res = info;

   return res;
}

FUDeclaration* RegisterSubUnit(Versat* versat,SizedString name,Accelerator* circuit){
   LockAccelerator(circuit,Accelerator::Locked::FREE,false);

   FUDeclaration decl = {};

   decl.type = FUDeclaration::COMPOSITE;
   //decl.circuit = circuit;
   decl.name = name;

   // HACK, for now
   circuit->subtype = &decl;

   for(ComplexFUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      if(d == versat->output){
         circuit->outputInstance = inst;
      }
   }

   decl.baseCircuit = CopyAccelerator(versat,circuit,nullptr,true);

   FixMultipleInputs(versat,circuit);

   decl.fixedMultiEdgeCircuit = CopyAccelerator(versat,circuit,nullptr,true);

   CalculateDelay(versat,circuit);

   decl.fixedDelayCircuit = circuit;

   AcceleratorValues val = ComputeAcceleratorValues(versat,decl.fixedDelayCircuit);

   decl.nInputs = val.inputs;
   decl.nOutputs = val.outputs;
   decl.nConfigs = val.configs;
   decl.nStates = val.states;
   decl.nDelays = val.delays;
   decl.nIOs = val.ios;
   decl.extraDataSize = val.extraData;
   decl.nTotalOutputs = val.totalOutputs;
   decl.nStaticConfigs = val.statics;
   decl.isMemoryMapped = val.isMemoryMapped;
   decl.memoryMapBits = val.memoryMappedBits;

   decl.inputDelays = PushArray(&versat->permanent,decl.nInputs,int);

   int i = 0;
   int minimum = (1 << 30);
   for(ComplexFUInstance** input : circuit->inputInstancePointers){
      decl.inputDelays[i++] = (*input)->baseDelay;
      minimum = mini(minimum,(*input)->baseDelay);
   }

   decl.latencies = PushArray(&versat->permanent,decl.nOutputs,int);

   if(circuit->outputInstance){
      for(int i = 0; i < decl.nOutputs; i++){
         decl.latencies[i] = circuit->outputInstance->tempData->inputDelay;
      }
   }

   decl.configWires = PushArray(&versat->permanent,decl.nConfigs,Wire);
   decl.stateWires = PushArray(&versat->permanent,decl.nStates,Wire);

   int configIndex = 0;
   int stateIndex = 0;
   for(ComplexFUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      if(!inst->isStatic){
         for(int i = 0; i < d->nConfigs; i++){
            decl.configWires[configIndex].name = PushString(&versat->permanent,"%.*s_%.2d",UNPACK_SS(d->configWires[i].name),configIndex);
            decl.configWires[configIndex++].bitsize = d->configWires[i].bitsize;
         }
      }

      for(int i = 0; i < d->nStates; i++){
         decl.stateWires[stateIndex].name = PushString(&versat->permanent,"%.*s_%.2d",UNPACK_SS(d->stateWires[i].name),stateIndex);
         decl.stateWires[stateIndex++].bitsize = d->stateWires[i].bitsize;
      }
   }

   // TODO: Change unit delay type inference. Only care about delay type to upper levels.
   // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
   #if 1
   bool hasSourceDelay = false;
   bool hasSinkDelay = false;
   #endif
   bool implementsDone = false;

   LockAccelerator(circuit,Accelerator::Locked::GRAPH);
   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      }

      if(inst->declaration->implementsDone){
         implementsDone = true;
      }
      #if 1
      if(inst->tempData->nodeType == GraphComputedData::TAG_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
      }
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE){
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      #endif
   }

   #if 1
   if(hasSourceDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SOURCE_DELAY);
   }
   if (hasSinkDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SINK_DELAY);
   }
   #endif

   decl.implementsDone = implementsDone;

   FUDeclaration* res = RegisterFU(versat,decl);
   res->baseCircuit->subtype = res;
   res->fixedMultiEdgeCircuit->subtype = res;
   res->fixedDelayCircuit->subtype = res;

   // TODO: Hackish, change Pool so that copying it around does nothing and then put this before the register unit
   #if 1
   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->isStatic){
         StaticInfo unit = {};
         unit.module = res;
         unit.name = inst->name;
         unit.nConfigs = inst->declaration->nConfigs;
         unit.wires = inst->declaration->configWires;

         Assert(unit.wires);

         SetLikeInsert(res->staticUnits,unit);
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         for(StaticInfo* unit : inst->declaration->staticUnits){
            SetLikeInsert(res->staticUnits,*unit);
         }
      }
   }

   for(StaticInfo* info : res->staticUnits){
      res->nStaticConfigs += info->nConfigs;
   }
   #endif

   #if 1
   {
   char buffer[256];
   sprintf(buffer,"src/%.*s.v",UNPACK_SS(decl.name));
   FILE* sourceCode = fopen(buffer,"w");
   OutputCircuitSource(versat,res,circuit,sourceCode);
   fclose(sourceCode);
   }
   #endif

   return res;
}

bool IsGraphValid(Accelerator* accel){
   InstanceMap map;

   for(ComplexFUInstance* inst : accel->instances){
      inst->tag = 0;

      map.insert({inst,inst});
   }

   for(Edge* edge : accel->edges){
      for(int i = 0; i < 2; i++){
         auto res = map.find(edge->units[i].inst);

         Assert(res != map.end());

         res->first->tag = 1;
      }

      Assert(edge->units[0].port < edge->units[0].inst->declaration->nOutputs && edge->units[0].port >= 0);
      Assert(edge->units[1].port < edge->units[1].inst->declaration->nInputs && edge->units[1].port >= 0);
   }

   if(accel->edges.Size()){
      for(ComplexFUInstance* inst : accel->instances){
         Assert(inst->tag == 1 || inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED);
      }
   }

   return 1;
}

void CompressAcceleratorMemory(Accelerator* accel){
   InstanceMap oldPosToNew;

   PoolIterator<ComplexFUInstance> iter = accel->instances.beginNonValid();

   for(ComplexFUInstance* inst : accel->instances){
      ComplexFUInstance* pos = *iter;

      iter.Advance();

      if(pos == inst){
         oldPosToNew.insert({inst,pos});
         continue;
      } else {
         ComplexFUInstance* newPos = accel->instances.Alloc();

         *newPos = *inst;
         oldPosToNew.insert({inst,newPos});

         accel->instances.Remove(inst);
      }
   }

   for(Edge* edge : accel->edges){
      auto unit1 = oldPosToNew.find(edge->units[0].inst);
      auto unit2 = oldPosToNew.find(edge->units[1].inst);

      if(unit1 != oldPosToNew.end()){
         edge->units[0].inst = unit1->second;
      }
      if(unit2 != oldPosToNew.end()){
         edge->units[1].inst = unit2->second;
      }
   }
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   #if 1
   InstanceMap map;
   Accelerator* newAccel = CopyAccelerator(versat,accel,&map,true);
   map.clear();

   Pool<ComplexFUInstance*> compositeInstances = {};
   Pool<ComplexFUInstance*> toRemove = {};
   std::unordered_map<StaticId,int> staticToIndex;

   for(int i = 0; i < times; i++){
      #if 1
      for(ComplexFUInstance* inst : newAccel->instances){
         if(inst->declaration->type == FUDeclaration::COMPOSITE){
            ComplexFUInstance** ptr = compositeInstances.Alloc();

            *ptr = inst;
         }
      }
      #endif

      if(compositeInstances.Size() == 0){
         break;
      }

      std::unordered_map<int,int> sharedToFirstChildIndex;

      int freeSharedIndex = 0;
      int count = 0;
      for(ComplexFUInstance** instPtr : compositeInstances){
         ComplexFUInstance* inst = *instPtr;

         Assert(inst->declaration->type == FUDeclaration::COMPOSITE);

         count += 1;
         Accelerator* circuit = inst->declaration->baseCircuit;

         int savedSharedIndex = freeSharedIndex;
         if(inst->sharedEnable){
            // Flattening a shared unit
            //printf("here\n");

            auto iter = sharedToFirstChildIndex.find(inst->sharedIndex);

            if(iter == sharedToFirstChildIndex.end()){
               sharedToFirstChildIndex.insert({inst->sharedIndex,freeSharedIndex});
            } else {
               freeSharedIndex = iter->second;
            }
         }

         std::unordered_map<int,int> sharedToShared;
         // Create new instance and map then
         #if 1
         for(ComplexFUInstance* circuitInst : circuit->instances){
            if(circuitInst->declaration->type == FUDeclaration::SPECIAL){
               continue;
            }

            SizedString newName = PushString(&versat->temp,"%.*s.%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
            ComplexFUInstance* newInst = CopyInstance(newAccel,circuitInst,newName,true);

            if(newInst->isStatic){
               bool found = false;
               int shareIndex = 0;
               for(auto iter : staticToIndex){
                  if(iter.first.parent == inst->declaration && CompareString(iter.first.name,circuitInst->name)){
                     found = true;
                     shareIndex = iter.second;
                     break;
                  }
               }

               if(!found){
                  shareIndex = freeSharedIndex++;

                  StaticId id = {};
                  id.name = circuitInst->name;
                  id.parent = inst->declaration;
                  staticToIndex.insert({id,shareIndex});
               }

               ShareInstanceConfig(newInst,shareIndex);
               newInst->isStatic = false;
            } else if(newInst->sharedEnable && inst->sharedEnable){
               auto ptr = sharedToShared.find(newInst->sharedIndex);

               if(ptr != sharedToShared.end()){
                  newInst->sharedIndex = ptr->second;
               } else {
                  int newIndex = freeSharedIndex++;

                  sharedToShared.insert({newInst->sharedIndex,newIndex});

                  newInst->sharedIndex = newIndex;
               }
            } else if(inst->sharedEnable){ // Currently flattening instance is shared
               ShareInstanceConfig(newInst,freeSharedIndex++);
            } else if(newInst->sharedEnable){
               auto ptr = sharedToShared.find(newInst->sharedIndex);

               if(ptr != sharedToShared.end()){
                  newInst->sharedIndex = ptr->second;
               } else {
                  int newIndex = freeSharedIndex++;

                  sharedToShared.insert({newInst->sharedIndex,newIndex});

                  newInst->sharedIndex = newIndex;
               }
            }

            map.insert({circuitInst,newInst});
         }
         #endif

         if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
            freeSharedIndex = savedSharedIndex;
         }

         #if 1
         // Add accel edges to output instances
         for(Edge* edge : newAccel->edges){
            if(edge->units[0].inst == inst){
               for(Edge* circuitEdge: circuit->edges){
                  if(circuitEdge->units[1].inst == circuit->outputInstance && circuitEdge->units[1].port == edge->units[0].port){
                     auto iter = map.find(circuitEdge->units[0].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     Edge* newEdge = newAccel->edges.Alloc();

                     ComplexFUInstance* mappedInst = iter->second;

                     newEdge->delay = edge->delay + circuitEdge->delay;
                     newEdge->units[0].inst = mappedInst;
                     newEdge->units[0].port = circuitEdge->units[0].port;
                     newEdge->units[1] = edge->units[1];
                  }
               }
            }
         }
         #endif

         #if 1
         // Add accel edges to input instances
         for(Edge* edge : newAccel->edges){
            if(edge->units[1].inst == inst){
               ComplexFUInstance* circuitInst = *circuit->inputInstancePointers.Get(edge->units[1].port);

               for(Edge* circuitEdge : circuit->edges){
                  if(circuitEdge->units[0].inst == circuitInst){
                     auto iter = map.find(circuitEdge->units[1].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     Edge* newEdge = newAccel->edges.Alloc();

                     ComplexFUInstance* mappedInst = iter->second;

                     newEdge->delay = edge->delay + circuitEdge->delay;
                     newEdge->units[0] = edge->units[0];
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

            newEdge->delay = circuitEdge->delay;
            newEdge->units[0].inst = input->second;
            newEdge->units[0].port = circuitEdge->units[0].port;
            newEdge->units[1].inst = output->second;
            newEdge->units[1].port = circuitEdge->units[1].port;
         }
         #endif

         #if 1
         // Add input to output specific edges
         for(Edge* edge1 : newAccel->edges){
            if(edge1->units[1].inst == inst){
               PortInstance input = edge1->units[0];
               ComplexFUInstance* circuitInput = *circuit->inputInstancePointers.Get(edge1->units[1].port);

               for(Edge* edge2 : newAccel->edges){
                  if(edge2->units[0].inst == inst){
                     PortInstance output = edge2->units[1];
                     int outputPort = edge2->units[0].port;

                     for(Edge* circuitEdge : circuit->edges){
                        if(circuitEdge->units[0].inst == circuitInput
                        && circuitEdge->units[1].inst == circuit->outputInstance
                        && circuitEdge->units[1].port == outputPort){

                           Edge* newEdge = newAccel->edges.Alloc();

                           newEdge->delay = edge1->delay + circuitEdge->delay + edge2->delay;
                           newEdge->units[0] = input;
                           newEdge->units[1] = output;
                        }
                     }
                  }
               }
            }
         }
         #endif

         *toRemove.Alloc() = inst;

         map.clear();
      }

      for(ComplexFUInstance** instPtr : toRemove){
         ComplexFUInstance* inst = *instPtr;

         RemoveFUInstance(newAccel,inst);
      }

      OutputGraphDotFile(versat,newAccel,true,"./debug/flatten.dot");

      IsGraphValid(newAccel);

      CompressAcceleratorMemory(newAccel);

      IsGraphValid(newAccel);

      toRemove.Clear();
      compositeInstances.Clear();
   }

   toRemove.Clear(true);
   compositeInstances.Clear(true);

   LockAccelerator(newAccel,Accelerator::FREE,true);

   //DebugTerminal(MakeValue(newAccel,"Accelerator"));

   OutputGraphDotFile(versat,accel,true,"./debug/original.dot");
   OutputGraphDotFile(versat,newAccel,true,"./debug/flatten.dot");

   PopulateAccelerator(versat,newAccel);
   InitializeFUInstances(newAccel,true);

   AcceleratorValues val1 = ComputeAcceleratorValues(versat,accel);
   AcceleratorValues val2 = ComputeAcceleratorValues(versat,newAccel);

   #if 0
   Assert((val1.configs + val1.statics) == val2.configs);
   Assert(val1.states == val2.states);
   Assert(val1.delays == val2.delays);
   #endif

   newAccel->staticInfo.Clear();

   return newAccel;
   #endif
}

// Debug output file

#define DUMP_VCD 1

static FILE* accelOutputFile = nullptr;
static std::array<char,4> currentMapping = {'a','a','a','a'};
static void ResetMapping(){
   currentMapping[0] = 'a';
   currentMapping[1] = 'a';
   currentMapping[2] = 'a';
   currentMapping[3] = 'a';
}

static void IncrementMapping(){
   for(int i = 3; i >= 0; i--){
      currentMapping[i] += 1;
      if(currentMapping[i] == 'z' + 1){
         currentMapping[i] = 'a';
      } else {
         return;
      }
   }
   Assert(false && "Increase mapping space");
}

static void PrintVCDDefinitions_(Accelerator* accel){
   #if 1
   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->nConfigs; i++){
         Wire* wire = &info->wires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s_%.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(info->name),UNPACK_SS(wire->name));
         IncrementMapping();
      }
   }
   #endif

   for(ComplexFUInstance* inst : accel->instances){
      fprintf(accelOutputFile,"$scope module %.*s_%d $end\n",UNPACK_SS(inst->name),inst->id);

      for(int i = 0; i < inst->tempData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_in%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->tempData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_out%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
         fprintf(accelOutputFile,"$var wire  32 %c%c%c%c %.*s_stored_out%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(inst->name),i);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nConfigs; i++){
         Wire* wire = &inst->declaration->configWires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(wire->name));
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nStates; i++){
         Wire* wire = &inst->declaration->stateWires[i];
         fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(wire->name));
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"$var wire 32 %c%c%c%c delay%d $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],i);
         IncrementMapping();
      }

      #if 0
      for(StaticInfo* info : accel->staticInfo){
         for(int i = 0; i < info->nConfigs; i++){
            Wire* wire = &info->wires[i];
            fprintf(accelOutputFile,"$var wire  %d %c%c%c%c %.*s_%.*s $end\n",wire->bitsize,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3],UNPACK_SS(info->name),UNPACK_SS(wire->name));
            IncrementMapping();
         }
      }
      #endif

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"$var wire  1 %c%c%c%c done $end\n",currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         PrintVCDDefinitions_(inst->compositeAccel);
      }

      fprintf(accelOutputFile,"$upscope $end\n");
   }
}

static void PrintVCDDefinitions(Accelerator* accel){
   #if DUMP_VCD == 0
      return;
   #endif

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

static void PrintVCD_(Accelerator* accel,int time){
   #if 1
   for(StaticInfo* info : accel->staticInfo){
      for(int i = 0; i < info->nConfigs; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(info->ptr[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         }
         IncrementMapping();
      }
   }
   #endif

   for(ComplexFUInstance* inst : accel->instances){
      for(int i = 0; i < inst->tempData->inputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(GetInputValue(inst,i)),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->tempData->outputPortsUsed; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->outputs[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->storedOutputs[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nConfigs; i++){
         if(time == 0){
            fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->config[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         }
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nStates; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->state[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      for(int i = 0; i < inst->declaration->nDelays; i++){
         fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(inst->delay[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      #if 0
      for(StaticInfo* info : accel->staticInfo){
         for(int i = 0; i < info->nConfigs; i++){
            if(time == 0){
               fprintf(accelOutputFile,"b%s %c%c%c%c\n",Bin(info->ptr[i]),currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
            }
            IncrementMapping();
         }
      }
      #endif

      if(inst->declaration->implementsDone){
         fprintf(accelOutputFile,"%d%c%c%c%c\n",inst->done ? 1 : 0,currentMapping[0],currentMapping[1],currentMapping[2],currentMapping[3]);
         IncrementMapping();
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         PrintVCD_(inst->compositeAccel,time);
      }
   }
}

static void PrintVCD(Accelerator* accel,int time,int clock){ // Need to put some clock signal
   #if DUMP_VCD == 0
      return;
   #endif

   ResetMapping();

   fprintf(accelOutputFile,"#%d\n",time * 10);
   fprintf(accelOutputFile,"%da\n",clock ? 1 : 0);
   PrintVCD_(accel,time);
}

static void AcceleratorRunStart(Accelerator* accel){
   LockAccelerator(accel,Accelerator::Locked::ORDERED);

   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      FUDeclaration* type = inst->declaration;

      if(type->startFunction){
         int* startingOutputs = type->startFunction(inst);

         if(startingOutputs){
            memcpy(inst->outputs,startingOutputs,inst->declaration->nOutputs * sizeof(int));
            memcpy(inst->storedOutputs,startingOutputs,inst->declaration->nOutputs * sizeof(int));
         }
      }
   }
}

static bool AcceleratorDone(Accelerator* accel){
   bool done = true;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
         bool subDone = AcceleratorDone(inst->compositeAccel);
         done &= subDone;
      } else if(inst->declaration->implementsDone && !inst->done){
         return false;
      }
   }

   return done;
}

static void AcceleratorRunIteration(Accelerator* accel){
   for(int i = 0; i < accel->instances.Size(); i++){
      ComplexFUInstance* inst = accel->order.instances.ptr[i];

      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         // Set accelerator input to instance input
         for(int ii = 0; ii < inst->tempData->numberInputs; ii++){
            ComplexFUInstance* input = *inst->compositeAccel->inputInstancePointers.Get(ii);

            int val = GetInputValue(inst,ii);
            for(int iii = 0; iii < input->tempData->numberOutputs; iii++){
               input->outputs[iii] = val;
               input->storedOutputs[iii] = val;
            }
         }

         AcceleratorRunIteration(inst->compositeAccel);

         // Note: Instead of propagating done upwards, for now, calculate everything inside the AcceleratorDone function (easier to debug and detect which unit isn't producing done when it's supposed to)
         #if 0
         if(inst->declaration->implementsDone){
            inst->done = AcceleratorDone(inst->compositeAccel);
         }
         #endif

         // Set output instance value to accelerator output
         ComplexFUInstance* output = inst->compositeAccel->outputInstance;
         if(output){
            for(int ii = 0; ii < output->tempData->numberInputs; ii++){
               int val = GetInputValue(output,ii);
               inst->outputs[ii] = val;
               inst->storedOutputs[ii] = val;
            }
         }
      } else {
         int* newOutputs = inst->declaration->updateFunction(inst);

         if(inst->declaration->nOutputs && inst->declaration->latencies[0] == 0 && inst->tempData->nodeType != GraphComputedData::TAG_SOURCE){
            memcpy(inst->outputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
         } else {
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
         }
      }
   }
}

void ClearConfigurations(Accelerator* accel){
   for(int i = 0; i < accel->configAlloc.size; i++){
      accel->configAlloc.ptr[i] = 0;
   }
}

void LoadConfiguration(Accelerator* accel,int configuration){
   // Implements the reverse of Save Configuration
}

void SaveConfiguration(Accelerator* accel,int configuration){
   //Assert(configuration < accel->versat->numberConfigurations);
}

void AcceleratorDoCycle(Accelerator* accel){
   Assert(accel->outputAlloc.size == accel->storedOutputAlloc.size);
   memcpy(accel->outputAlloc.ptr,accel->storedOutputAlloc.ptr,accel->outputAlloc.size * sizeof(int));
}

void FixAcceleratorDelay(Accelerator* accel){
   // TODO:
   FUDeclaration base = {};
   base.name = MakeSizedString("Top");
   accel->subtype = &base;

   CalculateDelay(accel->versat,accel);
   SetDelayRecursive(accel);
}

void AcceleratorRun(Accelerator* accel){
   static int numberRuns = 0;
   int time = 0;

   #if 1
   FUDeclaration base = {};
   base.name = MakeSizedString("Top");
   accel->subtype = &base;

   CalculateDelay(accel->versat,accel);
   SetDelayRecursive(accel);
   #endif

   // Lock all acelerators
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
         LockAccelerator(inst->compositeAccel,Accelerator::Locked::ORDERED);
      }
   }

   if(accel->versat->debug.outputVCD){
      char buffer[128];
      sprintf(buffer,"debug/accelRun%d.vcd",numberRuns++);
      accelOutputFile = fopen(buffer,"w");
      Assert(accelOutputFile);

      PrintVCDDefinitions(accel);
   }

   AcceleratorRunStart(accel);
   AcceleratorRunIteration(accel);

   if(accel->versat->debug.outputVCD){
      PrintVCD(accel,time++,0);
   }

   int cycle;
   for(cycle = 0; 1; cycle++){ // Max amount of iterations
      AcceleratorDoCycle(accel);
      AcceleratorRunIteration(accel);

      if(accel->versat->debug.outputVCD){
         PrintVCD(accel,time++,1);
         PrintVCD(accel,time++,0);
      }

      #if 1
      if(AcceleratorDone(accel)){
         break;
      }
      #endif
   }

   if(accel->versat->debug.outputVCD){
      PrintVCD(accel,time++,1);
      PrintVCD(accel,time++,0);
      fclose(accelOutputFile);
   }
}

void OutputMemoryMap(Versat* versat,Accelerator* accel){
   VersatComputedValues val = ComputeVersatValues(versat,accel);

   printf("\n");
   printf("Total bytes mapped: %d\n",val.memoryMappedBytes);
   printf("Maximum bytes mapped by a unit: %d\n",val.maxMemoryMapDWords * 4);
   printf("Memory address bits: %d\n",val.memoryAddressBits);
   printf("Units mapped: %d\n",val.unitsMapped);
   printf("Memory mapping address bits: %d\n",val.memoryMappingAddressBits);
   printf("\n");
   printf("Config registers: %d\n",val.nConfigs);
   printf("Config bits used: %d\n",val.configBits);
   printf("\n");
   printf("Static registers: %d\n",val.nStatics);
   printf("Static bits used: %d\n",val.staticBits);
   printf("\n");
   printf("Delay registers: %d\n",val.nDelays);
   printf("Delay bits used: %d\n",val.delayBits);
   printf("\n");
   printf("Configuration registers: %d (including versat reg, static and delays)\n",val.nConfigurations);
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
         printf("M ");
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
   printf("M - Memory mapped\n");
   printf("C - Used only by Config\n");
   printf("S - Used only by State\n");
   printf("B - Used by both Config and State\n");
   printf("\n");
   printf("Memory/Config bit: %d\n",val.memoryConfigDecisionBit);
   printf("Memory range: [%d:0]\n",val.memoryAddressBits - 1);
   printf("Config range: [%d:0]\n",val.configurationAddressBits - 1);
   printf("State range: [%d:0]\n",val.stateAddressBits - 1);
}

int GetInputValue(FUInstance* inst,int index){
   ComplexFUInstance* instance = (ComplexFUInstance*) inst;

   Assert(instance->tempData);

   for(int i = 0; i < instance->tempData->numberInputs; i++){
      ConnectionInfo connection = instance->tempData->inputs[i];

      if(connection.port == index){
         return connection.instConnectedTo.inst->outputs[connection.instConnectedTo.port];
      }
   }

   return 0;
}

// Connects out -> in
void ConnectUnits(PortInstance out,PortInstance in){
   ConnectUnits(out.inst,out.port,in.inst,in.port);
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex){
   ConnectUnitsWithDelay(out,outIndex,in,inIndex,0);
}

void ConnectUnitsWithDelay(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   FUDeclaration* inDecl = in->declaration;
   FUDeclaration* outDecl = out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl->nInputs);
   Assert(outIndex < outDecl->nOutputs);

   Accelerator* accel = out->accel;

   LockAccelerator(accel,Accelerator::Locked::FREE);

   Edge* edge = accel->edges.Alloc();

   edge->units[0].inst = (ComplexFUInstance*) out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = (ComplexFUInstance*) in;
   edge->units[1].port = inIndex;
   edge->delay = delay;
}

#define MAX_CHARS 64

#if 1
int CalculateLatency_(PortInstance portInst, std::unordered_map<PortInstance,int>* memoization,bool seenSourceAndSink){
   ComplexFUInstance* inst = portInst.inst;
   int port = portInst.port;

   if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && seenSourceAndSink){
      return inst->declaration->latencies[port];
   } else if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE){
      return inst->declaration->latencies[port];
   } else if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
      Assert(false);
   }

   auto iter = memoization->find(portInst);

   if(iter != memoization->end()){
      return iter->second;
   }

   int latency = 0;
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      ConnectionInfo* info = &inst->tempData->inputs[i];

      int lat = CalculateLatency_(info->instConnectedTo,memoization,true);

      //lat += abs(inst->tempData->inputs[i].delay);
      //lat += abs(inst->declaration->inputDelays[i]);

      Assert(inst->declaration->inputDelays[i] < 1000);

      latency = maxi(latency,lat);
   }

   int finalLatency = latency + inst->declaration->latencies[port];

   memoization->insert({portInst,finalLatency});

   return finalLatency;
}

int CalculateLatency(ComplexFUInstance* inst){
   std::unordered_map<PortInstance,int> map;

   Assert(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK || inst->tempData->nodeType == GraphComputedData::TAG_SINK);

   int maxLatency = 0;
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      ConnectionInfo* info = &inst->tempData->inputs[i];
      int latency = CalculateLatency_(info->instConnectedTo,&map,false);
      maxLatency = maxi(maxLatency,latency);
   }

   return maxLatency;
}
#endif

void SetDelayRecursive_(ComplexFUInstance* inst,int delay){
   if(inst->declaration == inst->accel->versat->buffer){
      inst->config[0] = inst->baseDelay;
      return;
   }

   int totalDelay = inst->baseDelay + delay;

   if(inst->declaration->type == FUDeclaration::COMPOSITE){
      for(ComplexFUInstance* child : inst->compositeAccel->instances){
         SetDelayRecursive_(child,totalDelay);
      }
   } else {
      // TODO: if bug, don't forget this piece of code was outside (there was no else)
      if(inst->declaration->nDelays){
         inst->delay[0] = totalDelay;
      }
   }
}

void SetDelayRecursive(Accelerator* accel){
   for(ComplexFUInstance* inst : accel->instances){
      SetDelayRecursive_(inst,0);
   }
}

#undef NUMBER_OUTPUTS
#undef OUTPUT

#include "verilogParser.hpp"

static void T(Accelerator* accel){
   for(ComplexFUInstance* inst : accel->instances){
      printf("%.*s %p\n",UNPACK_SS(inst->name),inst->memMapped);

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         T(inst->compositeAccel);
      }
   }
}

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst){

}


























