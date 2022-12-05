#include "versatPrivate.hpp"

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <printf.h>
#include <unordered_map>

#include "type.hpp"
#include "debug.hpp"
#include "parser.hpp"

#include "templateEngine.hpp"

#define IMPLEMENT_VERILOG_UNITS
#include "verilogWrapper.inc"

static int zeros[50] = {};
static int ones[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static int* DefaultInitFunction(ComplexFUInstance* inst){
   inst->done = true;
   return nullptr;
}
static FUDeclaration* RegisterCircuitInput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = MakeSizedString("CircuitInput");
   decl.nOutputs = 50;
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
   decl.nInputs = 50;
   decl.nOutputs = 50; // Used for templating circuit
   decl.latencies = zeros;
   decl.inputDelays = zeros;
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}
static int* DefaultUpdateFunction(ComplexFUInstance* inst){
   static int out[50];

   for(int i = 0; i < 50; i++){
      out[i] = GetInputValue(inst,i);
   }

   return out;
}
static FUDeclaration* RegisterData(Versat* versat){
   FUDeclaration decl = {};

   decl.name = MakeSizedString("Data");
   decl.nInputs = 50;
   decl.nOutputs = 50;
   decl.latencies = ones;
   decl.inputDelays = zeros;
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = DefaultUpdateFunction;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;

   return RegisterFU(versat,decl);
}
static int* UpdatePipelineRegister(ComplexFUInstance* inst){
   static int out;

   out = GetInputValue(inst,0);
   inst->done = true;

   return &out;
}
static FUDeclaration* RegisterPipelineRegister(Versat* versat){
   FUDeclaration decl = {};
   static int ones[] = {1};

   decl.name = MakeSizedString("PipelineRegister");
   decl.nInputs = 1;
   decl.nOutputs = 1;
   decl.latencies = ones;
   decl.inputDelays = zeros;
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = UpdatePipelineRegister;
   decl.isOperation = true;
   decl.operation = "{0}_{1} <= {2}";

   return RegisterFU(versat,decl);
}
static int* UnaryNot(ComplexFUInstance* inst){
   static uint out;
   out = ~GetInputValue(inst,0);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryXOR(ComplexFUInstance* inst){
   static uint out;
   out = GetInputValue(inst,0) ^ GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryADD(ComplexFUInstance* inst){
   static uint out;
   out = GetInputValue(inst,0) + GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinarySUB(ComplexFUInstance* inst){
   static uint out;
   out = GetInputValue(inst,0) - GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryAND(ComplexFUInstance* inst){
   static uint out;
   out = GetInputValue(inst,0) & GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryOR(ComplexFUInstance* inst){
   static uint out;
   out = GetInputValue(inst,0) | GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHR(ComplexFUInstance* inst){
   static uint out;
   uint value = GetInputValue(inst,0);
   uint shift = GetInputValue(inst,1);
   out = (value >> shift) | (value << (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHL(ComplexFUInstance* inst){
   static uint out;
   uint value = GetInputValue(inst,0);
   uint shift = GetInputValue(inst,1);
   out = (value << shift) | (value >> (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHR(ComplexFUInstance* inst){
   static uint out;
   uint value = GetInputValue(inst,0);
   uint shift = GetInputValue(inst,1);
   out = (value >> shift);
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHL(ComplexFUInstance* inst){
   static uint out;
   uint value = GetInputValue(inst,0);
   uint shift = GetInputValue(inst,1);
   out = (value << shift);
   inst->done = true;
   return (int*) &out;
}

static void RegisterOperators(Versat* versat){
   struct Operation{
      const char* name;
      FUFunction function;
      const char* operation;
   };

   Operation unary[] = {{"NOT",UnaryNot,"{0}_{1} = ~{2}"}};
   Operation binary[] = {{"XOR",BinaryXOR,"{0}_{1} = {2} ^ {3}"},
                         {"ADD",BinaryADD,"{0}_{1} = {2} + {3}"},
                         {"SUB",BinarySUB,"{0}_{1} = {2} - {3}"},
                         {"AND",BinaryAND,"{0}_{1} = {2} & {3}"},
                         {"OR" ,BinaryOR ,"{0}_{1} = {2} | {3}"},
                         {"RHR",BinaryRHR,"{0}_{1} = ({2} >> {3}) | ({2} << (32 - {3}))"},
                         {"SHR",BinarySHR,"{0}_{1} = {2} >> {3}"},
                         {"RHL",BinaryRHL,"{0}_{1} = ({2} << {3}) | ({2} >> (32 - {3}))"},
                         {"SHL",BinarySHL,"{0}_{1} = {2} << {3}"}};

   FUDeclaration decl = {};
   decl.nOutputs = 1;
   decl.nInputs = 1;
   decl.inputDelays = zeros;
   decl.latencies = zeros;
   decl.isOperation = true;

   for(unsigned int i = 0; i < ARRAY_SIZE(unary); i++){
      decl.name = MakeSizedString(unary[i].name);
      decl.updateFunction = unary[i].function;
      decl.operation = unary[i].operation;
      RegisterFU(versat,decl);
   }

   decl.nInputs = 2;
   for(unsigned int i = 0; i < ARRAY_SIZE(binary); i++){
      decl.name = MakeSizedString(binary[i].name);
      decl.updateFunction = binary[i].function;
      decl.operation = binary[i].operation;
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

   InitArena(&versat->temp,Megabyte(256));
   InitArena(&versat->permanent,Megabyte(256));

   FUDeclaration nullDeclaration = {};
   nullDeclaration.latencies = zeros;
   nullDeclaration.inputDelays = zeros;
   RegisterFU(versat,nullDeclaration);

   RegisterAllVerilogUnits(versat);

   versat->buffer = GetTypeByName(versat,MakeSizedString("Buffer"));
   versat->fixedBuffer = GetTypeByName(versat,MakeSizedString("FixedBuffer"));
   versat->pipelineRegister = RegisterPipelineRegister(versat);
   versat->multiplexer = GetTypeByName(versat,MakeSizedString("Mux2"));
   versat->combMultiplexer = GetTypeByName(versat,MakeSizedString("CombMux2"));
   versat->input = RegisterCircuitInput(versat);
   versat->output = RegisterCircuitOutput(versat);
   versat->data = RegisterData(versat);

   RegisterOperators(versat);

   Log(LogModule::VERSAT,LogLevel::INFO,"Init versat");

   return versat;
}

void Free(Versat* versat){
   for(Accelerator* accel : versat->accelerators){
      #if 0
      if(accel->type == Accelerator::CIRCUIT){
         continue;
      }
      #endif

      #if 1
      for(ComplexFUInstance* inst : accel->instances){
         if(inst->initialized && inst->declaration->destroyFunction){
            inst->declaration->destroyFunction(inst);
         }
      }
      #endif
   }

   for(Accelerator* accel : versat->accelerators){
      //LockAccelerator(accel,Accelerator::FREE,true);

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

static int CompositeMemoryAccess(ComplexFUInstance* inst,int address,int value,int write){
   int offset = 0;

   for(FUInstance* inst : inst->compositeAccel->instances){
      if(!inst->declaration->isMemoryMapped){
         continue;
      }

      int mappedWords = 1 << inst->declaration->memoryMapBits;
      if(mappedWords){
         if(address >= offset && address <= offset + mappedWords){
            if(write){
               VersatUnitWrite(inst,address - offset,value);
               return 0;
            } else {
               int result = VersatUnitRead(inst,address - offset);
               return result;
            }
         } else {
            offset += mappedWords;
         }
      }
   }

   NOT_POSSIBLE; // TODO: Figure out what to do in this situation
   return 0;
}

static int AccessMemory(FUInstance* inst,int address, int value, int write){
   ComplexFUInstance* instance = (ComplexFUInstance*) inst;

   int res = instance->declaration->memAccessFunction(instance,address,value,write);

   return res;
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   AccessMemory(instance,address,value,1);
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

      printf("Didn't find the following instance: ");

      bool first = true;
      for(HierarchicalName* ptr = &fullName; ptr != nullptr; ptr = ptr->next){
         if(first){
            first = false;
         } else {
            printf(".");
         }
         printf("%.*s",UNPACK_SS(ptr->name));
      }

      printf("\n");
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

   Edge* edge = accel->edges.Alloc();

   edge->units[0].inst = (ComplexFUInstance*) out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = (ComplexFUInstance*) in;
   edge->units[1].port = inIndex;
   edge->delay = delay;
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex){
   Accelerator* accel = out->accel;

   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == out && edge->units[0].port == outIndex
      && edge->units[1].inst == in  && edge->units[1].port == inIndex)

      return;
   }

   ConnectUnits(out,outIndex,in,inIndex);
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

   return type;
}

UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst){
   UnitValues res = {};

   FUDeclaration* type = inst->declaration;

   res.outputs = type->nOutputs; // Common all cases
   if(type->type == FUDeclaration::COMPOSITE){
      Assert(inst->compositeAccel);
   } else if(type->type == FUDeclaration::ITERATIVE){
      Assert(inst->compositeAccel);
      res.delays = 1;
      res.extraData = sizeof(int);
   } else {
      res.configs = type->nConfigs;
      res.states = type->nStates;
      res.delays = type->nDelays;
      res.extraData = type->extraDataSize;
   }

   return res;
}

UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel){
   UnitValues val = {};

   std::vector<StaticId> staticSeen;
   std::vector<bool> seenShared;

   int memoryMapBits[32];
   memset(memoryMapBits,0,sizeof(int) * 32);

   // Handle non-static information
   for(ComplexFUInstance* inst : accel->instances){
      FUDeclaration* type = inst->declaration;

      // Check if shared
      if(inst->sharedEnable){
         if(inst->sharedIndex >= (int) seenShared.size()){
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

      val.states += type->nStates;
      val.delays += type->nDelays;
      val.ios += type->nIOs;
      val.extraData += type->extraDataSize;
   }

   if(accel->outputInstance){
      for(Edge* edge : accel->edges){
         if(edge->units[0].inst == accel->outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[0].port) + 1;
         }
         if(edge->units[1].inst == accel->outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[1].port) + 1;
         }
      }
   }
   val.totalOutputs = CalculateTotalOutputs(accel);

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
      memoryMapBits[std::max(first,second) + 1] += 1;
   }

   if(last != -1){
      val.isMemoryMapped = true;
      val.memoryMappedBits = last;
   }

   return val;
}

bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst){
   int* config = inst->config;

   int delta = config - topLevel->configAlloc.ptr;

   if(delta >= 0 && delta < topLevel->configAlloc.size){
      return false;
   } else {
      return true;
   }
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
   FUDeclaration decl = {};

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   decl.type = FUDeclaration::COMPOSITE;
   //decl.circuit = circuit;
   decl.name = name;

   // HACK, for now
   circuit->subtype = &decl;

   // Keep track of input and output nodes
   for(ComplexFUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      if(d == versat->input){
         int index = inst->id;

         ComplexFUInstance** ptr = circuit->inputInstancePointers.Alloc(index);
         *ptr = (ComplexFUInstance*) inst;
      }

      if(d == versat->output){
         circuit->outputInstance = inst;
      }
   }

   decl.baseCircuit = CopyAccelerator(versat,circuit,nullptr,true);

   #if 0
   bool allOperations = true;
   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      }
      if(!inst->declaration->isOperation){
         allOperations = false;
         break;
      }
   }

   if(allOperations){
      circuit = Flatten(versat,circuit,99);
      circuit->subtype = &decl;
   }
   #endif

   FixMultipleInputs(versat,circuit);

   //OutputGraphDotFile(versat,circuit,true,"debug/FixMultipleInputs.dot");

   decl.fixedMultiEdgeCircuit = CopyAccelerator(versat,circuit,nullptr,true);

   {
   ArenaMarker marker(arena);
   AcceleratorView view = CreateAcceleratorView(circuit,arena);
   view.CalculateDelay(arena);
   FixDelays(versat,circuit,view);
   }

   decl.fixedDelayCircuit = circuit;

   UnitValues val = CalculateAcceleratorValues(versat,decl.fixedDelayCircuit);

   decl.nInputs = val.inputs;
   decl.nOutputs = val.outputs;
   decl.nConfigs = val.configs;
   decl.nStates = val.states;
   decl.nDelays = val.delays;
   decl.nIOs = val.ios;
   decl.extraDataSize = val.extraData;
   decl.nStaticConfigs = val.statics;
   decl.isMemoryMapped = val.isMemoryMapped;
   decl.memoryMapBits = val.memoryMappedBits;
   decl.memAccessFunction = CompositeMemoryAccess;

   decl.inputDelays = PushArray(&versat->permanent,decl.nInputs,int);

   int i = 0;
   int minimum = (1 << 30);
   for(ComplexFUInstance** input : circuit->inputInstancePointers){
      decl.inputDelays[i++] = (*input)->baseDelay;
      minimum = std::min(minimum,(*input)->baseDelay);
   }

   decl.latencies = PushArray(&versat->permanent,decl.nOutputs,int);

   if(circuit->outputInstance){
      for(int i = 0; i < decl.nOutputs; i++){
         decl.latencies[i] = circuit->outputInstance->graphData->inputDelay;
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

   AcceleratorView view = CreateAcceleratorView(circuit,&versat->temp);
   view.CalculateGraphData(&versat->temp);

   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      }

      if(inst->declaration->implementsDone){
         implementsDone = true;
      }
      #if 1
      if(inst->graphData->nodeType == GraphComputedData::TAG_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
      }
      if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE){
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
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
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE || inst->declaration->type == FUDeclaration::ITERATIVE){
         for(StaticInfo* unit : inst->declaration->staticUnits){
            SetLikeInsert(res->staticUnits,*unit);
         }
      }
   }

   for(StaticInfo* info : res->staticUnits){
      res->nStaticConfigs += info->nConfigs;
   }
   #endif

   ClearFUInstanceTempData(circuit);

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

static int* IterativeInitializeFunction(ComplexFUInstance* inst){
   return nullptr;
}

static int* IterativeStartFunction(ComplexFUInstance* inst){
   int* delay = (int*) inst->extraData;

   *delay = 0;

   return nullptr;
}

static void AcceleratorRunComposite(ComplexFUInstance*); // Fwd decl
static void AcceleratorRunIteration(Accelerator* accel);
static int* IterativeUpdateFunction(ComplexFUInstance* inst){
   static int out[99];

   FUDeclaration* type = inst->declaration;
   int* delay = (int*) inst->extraData;
   ComplexFUInstance* dataInst = (ComplexFUInstance*) GetInstanceByName(inst->compositeAccel,"data");

   //LockAccelerator(inst->compositeAccel,Accelerator::Locked::ORDERED);
   //Assert(inst->iterative);

   if(*delay == -1){ // For loop part
      AcceleratorRunComposite(inst);
   } else if(*delay == inst->delay[0]){
      for(int i = 0; i < type->nInputs; i++){
         int val = GetInputValue(inst,i);
         SetInputValue(inst->iterative,i,val);
      }

      AcceleratorRun(inst->iterative);

      for(int i = 0; i < type->nOutputs; i++){
         int val = GetOutputValue(inst->iterative,i);
         out[i] = val;
      }

      ComplexFUInstance* secondData = (ComplexFUInstance*) GetInstanceByName(inst->iterative,"data");

      for(int i = 0; i < secondData->declaration->nOutputs; i++){
         dataInst->outputs[i] = secondData->outputs[i];
         dataInst->storedOutputs[i] = secondData->outputs[i];
      }

      *delay = -1;
   } else {
      *delay += 1;
   }

   return out;
}

static int* IterativeDestroyFunction(ComplexFUInstance* inst){
   return nullptr;
}

FUDeclaration* RegisterIterativeUnit(Versat* versat,IterativeUnitDeclaration* decl){
   AcceleratorView initialView = CreateAcceleratorView(decl->initial,&versat->temp);
   AcceleratorView forLoopView = CreateAcceleratorView(decl->forLoop,&versat->temp);

   initialView.CalculateDAGOrdering(&versat->temp);
   forLoopView.CalculateDAGOrdering(&versat->temp);

   FUInstance* comb = nullptr;
   for(FUInstance* inst : decl->forLoop->instances){
      if(inst->declaration == decl->baseDeclaration){
         comb = inst;
      }
   }

   FUDeclaration* combType = comb->declaration;
   FUDeclaration declaration = {};

   // Default combinatorial values
   declaration = *combType; // By default, copy everything from combinatorial declaration
   declaration.type = FUDeclaration::ITERATIVE;
   declaration.staticUnits = combType->staticUnits.Copy();
   declaration.nDelays = 1; // At least one delay

   // Accelerator computed values
   UnitValues val = CalculateAcceleratorValues(versat,decl->forLoop);
   declaration.nOutputs = val.outputs;
   declaration.nInputs = val.inputs;
   declaration.extraDataSize = val.extraData + sizeof(int); // Save delay

   // Values from iterative declaration
   declaration.name = decl->name;
   declaration.unitName = decl->unitName;
   declaration.initial = decl->initial;
   declaration.forLoop = decl->forLoop;
   declaration.dataSize = decl->dataSize;

   declaration.initializeFunction = IterativeInitializeFunction;
   declaration.startFunction = IterativeStartFunction;
   declaration.updateFunction = IterativeUpdateFunction;
   declaration.destroyFunction = IterativeDestroyFunction;
   declaration.memAccessFunction = CompositeMemoryAccess;

   declaration.inputDelays = PushArray(&versat->permanent,declaration.nInputs,int);
   declaration.latencies = PushArray(&versat->permanent,declaration.nOutputs,int);
   Memset(declaration.inputDelays,0,declaration.nInputs);
   Memset(declaration.latencies,decl->latency,declaration.nOutputs);

   FUDeclaration* registeredType = RegisterFU(versat,declaration);

   char buffer[256];
   sprintf(buffer,"src/%.*s.v",UNPACK_SS(decl->name));
   FILE* sourceCode = fopen(buffer,"w");

   TemplateSetCustom("base",registeredType,"FUDeclaration");
   TemplateSetCustom("comb",comb,"ComplexFUInstance");

   FUInstance* firstPartComb = nullptr;
   FUInstance* firstData = nullptr;
   FUInstance* secondPartComb = nullptr;
   FUInstance* secondData = nullptr;

   for(FUInstance* inst : decl->initial->instances){
      if(inst->declaration == decl->baseDeclaration){
         firstPartComb = inst;
      }
      if(inst->declaration == versat->data){
         firstData = inst;
      }
   }
   for(FUInstance* inst : decl->forLoop->instances){
      if(inst->declaration == decl->baseDeclaration){
         secondPartComb = inst;
      }
      if(inst->declaration == versat->data){
         secondData = inst;
      }
   }

   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("firstComb",firstPartComb,"ComplexFUInstance");
   TemplateSetCustom("secondComb",secondPartComb,"ComplexFUInstance");
   TemplateSetCustom("firstOut",decl->initial->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("secondOut",decl->forLoop->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("firstData",firstData,"ComplexFUInstance");
   TemplateSetCustom("secondData",secondData,"ComplexFUInstance");

   ProcessTemplate(sourceCode,"../../submodules/VERSAT/software/templates/versat_iterative_template.tpl",&versat->temp);

   fclose(sourceCode);

   return registeredType;
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


static void AcceleratorRunStart(Accelerator* accel){
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

static void AcceleratorRunIteration(Accelerator* accel); // Fwd decl

static void AcceleratorRunComposite(ComplexFUInstance* compositeInst){
   // Set accelerator input to instance input
   for(int ii = 0; ii < compositeInst->graphData->numberInputs; ii++){
      ComplexFUInstance* input = *compositeInst->compositeAccel->inputInstancePointers.Get(ii);

      int val = GetInputValue(compositeInst,ii);
      for(int iii = 0; iii < input->graphData->numberOutputs; iii++){
         input->outputs[iii] = val;
         input->storedOutputs[iii] = val;
      }
   }

   AcceleratorRunIteration(compositeInst->compositeAccel);

   // Note: Instead of propagating done upwards, for now, calculate everything inside the AcceleratorDone function (easier to debug and detect which unit isn't producing done when it's supposed to)
   #if 0
   if(compositeInst->declaration->implementsDone){
      compositeInst->done = AcceleratorDone(compositeInst->compositeAccel);
   }
   #endif

   // Set output instance value to accelerator output
   ComplexFUInstance* output = compositeInst->compositeAccel->outputInstance;
   if(output){
      for(int ii = 0; ii < output->graphData->numberInputs; ii++){
         int val = GetInputValue(output,ii);
         compositeInst->outputs[ii] = val;
         compositeInst->storedOutputs[ii] = val;
      }
   }
}

static void AcceleratorRunIteration(Accelerator* accel){
   AcceleratorView view = CreateAcceleratorView(accel,&accel->versat->temp);
   DAGOrder order = view.CalculateDAGOrdering(&accel->versat->temp);

   for(int i = 0; i < accel->instances.Size(); i++){
      ComplexFUInstance* inst = order.instances[i];

      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         AcceleratorRunComposite(inst);
      } else {
         int* newOutputs = inst->declaration->updateFunction(inst);

         if(inst->declaration->nOutputs && inst->declaration->latencies[0] == 0 && inst->graphData->nodeType != GraphComputedData::TAG_SOURCE){
            memcpy(inst->outputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
         } else {
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->nOutputs * sizeof(int));
         }
      }
   }
}

void AcceleratorRun(Accelerator* accel){
   static int numberRuns = 0;
   int time = 0;

   Arena* arena = &accel->versat->temp;
   ArenaMarker marker(arena);

   #if 1
   FUDeclaration base = {};
   base.name = MakeSizedString("Top");
   accel->subtype = &base;

   AcceleratorView view = CreateAcceleratorView(accel,arena);
   view.CalculateDelay(arena);
   SetDelayRecursive(accel);
   #endif

   // Lock all acelerators
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
         AcceleratorView view = CreateAcceleratorView(inst->compositeAccel,arena);
         view.CalculateDAGOrdering(arena);
      }
   }

   FILE* accelOutputFile = nullptr;
   if(accel->versat->debug.outputVCD){
      char buffer[128];
      sprintf(buffer,"debug/accelRun%d.vcd",numberRuns++);
      accelOutputFile = fopen(buffer,"w");
      Assert(accelOutputFile);

      PrintVCDDefinitions(accelOutputFile,accel);
   }

   AcceleratorRunStart(accel);
   AcceleratorRunIteration(accel);

   if(accel->versat->debug.outputVCD){
      PrintVCD(accelOutputFile,accel,time++,0);
   }

   int cycle;
   for(cycle = 0; 1; cycle++){ // Max amount of iterations
      Assert(accel->outputAlloc.size == accel->storedOutputAlloc.size);
      memcpy(accel->outputAlloc.ptr,accel->storedOutputAlloc.ptr,accel->outputAlloc.size * sizeof(int));

      AcceleratorRunIteration(accel);

      if(accel->versat->debug.outputVCD){
         PrintVCD(accelOutputFile,accel,time++,1);
         PrintVCD(accelOutputFile,accel,time++,0);
      }

      if(AcceleratorDone(accel)){
         break;
      }
   }

   if(accel->versat->debug.outputVCD){
      PrintVCD(accelOutputFile,accel,time++,1);
      PrintVCD(accelOutputFile,accel,time++,0);
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

   Assert(instance->graphData);

   for(int i = 0; i < instance->graphData->numberInputs; i++){
      ConnectionInfo connection = instance->graphData->inputs[i];

      if(connection.port == index){
         return connection.instConnectedTo.inst->outputs[connection.instConnectedTo.port];
      }
   }

   return 0;
}

int GetNumberOfInputs(FUInstance* inst){
   return inst->declaration->nInputs;
}

int GetNumberOfOutputs(FUInstance* inst){
   return inst->declaration->nOutputs;
}

int GetNumberOfInputs(Accelerator* accel){
   return accel->inputInstancePointers.Size();
}

int GetNumberOfOutputs(Accelerator* accel){
   NOT_IMPLEMENTED;
   return 0;
}

void SetInputValue(Accelerator* accel,int portNumber,int number){
   Assert(accel->outputAlloc.ptr);

   ComplexFUInstance** instPtr = accel->inputInstancePointers.Get(portNumber);

   Assert(instPtr);

   ComplexFUInstance* inst = *instPtr;

   inst->outputs[0] = number;
   inst->storedOutputs[0] = number;
}

int GetOutputValue(Accelerator* accel,int portNumber){
   FUInstance* inst = accel->outputInstance;

   int value = GetInputValue(inst,portNumber);

   return value;
}

int GetInputPortNumber(Versat* versat,FUInstance* inputInstance){
   Assert(inputInstance->declaration == versat->input);

   return inputInstance->id;
}

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
   } else if(inst->declaration->nDelays){
      inst->delay[0] = totalDelay;
   }
}

void SetDelayRecursive(Accelerator* accel){
   for(ComplexFUInstance* inst : accel->instances){
      SetDelayRecursive_(inst,0);
   }
}

bool CheckValidName(SizedString name){
   for(int i = 0; i < name.size; i++){
      char ch = name.str[i];

      bool allowed = (ch >= 'a' && ch <= 'z')
                 ||  (ch >= 'A' && ch <= 'Z')
                 ||  (ch >= '0' && ch <= '9' && i != 0)
                 ||  (ch == '_')
                 ||  (ch == '.' && i != 0)  // For now allow it, despite the fact that it should only be used internally by Versat
                 ||  (ch == '/' && i != 0); // For now allow it, despite the fact that it should only be used internally by Versat

      if(!allowed){
         return false;
      }
   }

   return true;
}

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst){

}























