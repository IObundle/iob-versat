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
   decl.inputDelays = Array<int>{zeros,0};
   decl.outputLatencies = Array<int>{zeros,50};
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SOURCE_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}
static FUDeclaration* RegisterCircuitOutput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = MakeSizedString("CircuitOutput");
   decl.inputDelays = Array<int>{zeros,50};
   decl.outputLatencies = Array<int>{zeros,0};
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
   decl.inputDelays = Array<int>{zeros,50};
   decl.outputLatencies = Array<int>{ones,50};
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
   decl.inputDelays = Array<int>{zeros,1};
   decl.outputLatencies = Array<int>{ones,1};
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = UpdatePipelineRegister;
   decl.isOperation = true;
   decl.operation = "{0}_{1} <= {2}";

   return RegisterFU(versat,decl);
}
static int* UnaryNot(ComplexFUInstance* inst){
   static unsigned int out;
   out = ~GetInputValue(inst,0);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryXOR(ComplexFUInstance* inst){
   static unsigned int out;
   out = GetInputValue(inst,0) ^ GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryADD(ComplexFUInstance* inst){
   static unsigned int out;
   out = GetInputValue(inst,0) + GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinarySUB(ComplexFUInstance* inst){
   static unsigned int out;
   out = GetInputValue(inst,0) - GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryAND(ComplexFUInstance* inst){
   static unsigned int out;
   out = GetInputValue(inst,0) & GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryOR(ComplexFUInstance* inst){
   static unsigned int out;
   out = GetInputValue(inst,0) | GetInputValue(inst,1);
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHR(ComplexFUInstance* inst){
   static unsigned int out;
   unsigned int value = GetInputValue(inst,0);
   unsigned int shift = GetInputValue(inst,1);
   out = (value >> shift) | (value << (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHL(ComplexFUInstance* inst){
   static unsigned int out;
   unsigned int value = GetInputValue(inst,0);
   unsigned int shift = GetInputValue(inst,1);
   out = (value << shift) | (value >> (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHR(ComplexFUInstance* inst){
   static unsigned int out;
   unsigned int value = GetInputValue(inst,0);
   unsigned int shift = GetInputValue(inst,1);
   out = (value >> shift);
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHL(ComplexFUInstance* inst){
   static unsigned int out;
   unsigned int value = GetInputValue(inst,0);
   unsigned int shift = GetInputValue(inst,1);
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

   Operation unary[] =  {{"NOT",UnaryNot,"{0}_{1} = ~{2}"}};
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
   decl.inputDelays = Array<int>{zeros,1};
   decl.outputLatencies = Array<int>{zeros,1};
   decl.isOperation = true;

   for(unsigned int i = 0; i < ARRAY_SIZE(unary); i++){
      decl.name = MakeSizedString(unary[i].name);
      decl.updateFunction = unary[i].function;
      decl.operation = unary[i].operation;
      RegisterFU(versat,decl);
   }

   decl.inputDelays = Array<int>{zeros,2};
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
   versat->debug.dotFormat = GRAPH_DOT_FORMAT_NAME |
                             //GRAPH_DOT_FORMAT_ID |
                             GRAPH_DOT_FORMAT_DELAY |
                             //GRAPH_DOT_FORMAT_TYPE |
                             GRAPH_DOT_FORMAT_EXPLICIT |
                             GRAPH_DOT_FORMAT_LATENCY;

   RegisterTypes();

   versat->numberConfigurations = numberConfigurations;
   versat->base = base;

   InitArena(&versat->temp,Megabyte(256));
   InitArena(&versat->permanent,Megabyte(256));

   FUDeclaration nullDeclaration = {};
   nullDeclaration.inputDelays = Array<int>{zeros,1};
   nullDeclaration.outputLatencies = Array<int>{zeros,1};
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

   Log(LogModule::TOP_SYS,LogLevel::INFO,"Init versat");

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
      accel->staticInfo.Clear(true);
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

uint SetDebug(Versat* versat,VersatDebugFlags flags,uint flag){
   uint last;
   switch(flags){
   case VersatDebugFlags::OUTPUT_GRAPH_DOT:{
      last = versat->debug.outputGraphs;
      versat->debug.outputGraphs = flag;
   }break;
   case VersatDebugFlags::GRAPH_DOT_FORMAT:{
      last = versat->debug.dotFormat;
      versat->debug.dotFormat = flag;
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

   for(FUInstance* inst : inst->declaration->fixedDelayCircuit->instances){
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

   Log(LogModule::TOP_SYS,LogLevel::FATAL,"[GetTypeByName] Didn't find the following type: %.*s",UNPACK_SS(name));

   return nullptr;
}

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
            } else if(inst->declaration->fixedDelayCircuit && hier->next){
               FUInstance* res = GetInstanceByHierarchicalName(inst->declaration->fixedDelayCircuit,hier->next);
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

static FUInstance* GetInstanceByHierarchicalName2(AcceleratorIterator iter,HierarchicalName* hier,Arena* arena){
   HierarchicalName* savedHier = hier;
   FUInstance* res = nullptr;
   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Skip()){
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
            } else if(inst->declaration->fixedDelayCircuit && hier->next){
               AcceleratorIterator it = iter.LevelBelowIterator(arena);
               FUInstance* res = GetInstanceByHierarchicalName2(it,hier->next,arena);
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

static FUInstance* vGetInstanceByName_(Accelerator* circuit,Arena* arena,int argc,va_list args){
   HierarchicalName fullName = {};
   HierarchicalName* namePtr = &fullName;
   HierarchicalName* lastPtr = nullptr;

   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);
      int arguments = parse_printf_format(str,0,nullptr);

      if(namePtr == nullptr){
         HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);

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
            HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);

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

   AcceleratorIterator iter = {};
   iter.Start(circuit,arena,true);
   FUInstance* res = GetInstanceByHierarchicalName2(iter,&fullName,arena);

   #if 0
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
   #endif

   return res;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){
   va_list args;
   va_start(args,argc);

   STACK_ARENA(temp,Kilobyte(16));
   FUInstance* res = vGetInstanceByName_(circuit,&temp,argc,args);

   va_end(args);

   return res;
}

FUInstance* GetInstanceByName_(FUDeclaration* decl,int argc, ...){
   va_list args;
   va_start(args,argc);

   STACK_ARENA(temp,Kilobyte(16));
   FUInstance* res = vGetInstanceByName_(decl->baseCircuit,&temp,argc,args);

   va_end(args);

   return res;
}

FUInstance* GetInstanceByName_(FUInstance* instance,int argc, ...){
   va_list args;
   va_start(args,argc);

   FUInstance* inst = (FUInstance*) instance;
   Assert(inst->declaration->fixedDelayCircuit);

   STACK_ARENA(temp,Kilobyte(16));
   FUInstance* res = vGetInstanceByName_(inst->declaration->fixedDelayCircuit,&temp,argc,args);

   va_end(args);

   return res;
}

Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   FUDeclaration* inDecl = in->declaration;
   FUDeclaration* outDecl = out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl->inputDelays.size);
   Assert(outIndex < outDecl->outputLatencies.size);

   Accelerator* accel = out->accel;

   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == (ComplexFUInstance*) out &&
         edge->units[0].port == outIndex &&
         edge->units[1].inst == (ComplexFUInstance*) in &&
         edge->units[1].port == inIndex &&
         edge->delay == delay) {
         return edge;
      }
   }

   return nullptr;
}

// Connects out -> in
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   FUDeclaration* inDecl = in->declaration;
   FUDeclaration* outDecl = out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl->inputDelays.size);
   Assert(outIndex < outDecl->outputLatencies.size);

   Accelerator* accel = out->accel;

   Edge* edge = accel->edges.Alloc();

   edge->units[0].inst = (ComplexFUInstance*) out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = (ComplexFUInstance*) in;
   edge->units[1].port = inIndex;
   edge->delay = delay;

   return edge;
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   ConnectUnitsIfNotConnectedGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   Accelerator* accel = out->accel;

   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == out && edge->units[0].port == outIndex &&
         edge->units[1].inst == in  && edge->units[1].port == inIndex &&
         delay == edge->delay){
         return edge;
      }
   }

   return ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnits(PortInstance out,PortInstance in,int delay){
   return ConnectUnitsGetEdge(out.inst,out.port,in.inst,in.port,delay);
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   if(decl.type == FUDeclaration::SINGLE || decl.type == FUDeclaration::SPECIAL){
      decl.totalOutputs = decl.outputLatencies.size;
   }

   FUDeclaration* type = versat->declarations.Alloc();
   *type = decl;

   return type;
}

// Returns the values when looking at a unit individually
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst){
   UnitValues res = {};

   FUDeclaration* type = inst->declaration;

   res.outputs = type->outputLatencies.size; // Common all cases
   if(type->type == FUDeclaration::COMPOSITE){
      Assert(inst->declaration->fixedDelayCircuit);
   } else if(type->type == FUDeclaration::ITERATIVE){
      Assert(inst->declaration->fixedDelayCircuit);
      res.delays = 1;
      res.extraData = sizeof(int);
   } else {
      res.configs = type->configs.size;
      res.states = type->states.size;
      res.delays = type->nDelays;
      res.extraData = type->extraDataSize;
   }

   return res;
}

// Returns the values when looking at subunits as well. For simple units, same as individual unit values
// For composite units, same as calculate accelerator values + calculate individual unit values
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst){
   UnitValues res = {};

   FUDeclaration* type = inst->declaration;

   if(type->type == FUDeclaration::COMPOSITE){
      res.totalOutputs = type->totalOutputs;
   } else {
      res = CalculateIndividualUnitValues(inst);
      res.totalOutputs = res.outputs;
   }

   return res;
}

UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel){
   ArenaMarker marker(&accel->versat->temp);

   UnitValues val = {};

   std::vector<bool> seenShared;

   Hashmap<StaticId,int> staticSeen = {};
   staticSeen.Init(&accel->versat->temp,50);

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
            val.configs += type->configs.size;
            val.sharedUnits += 1;
         }

         seenShared[inst->sharedIndex] = true;
      } else if(!inst->isStatic){ // Shared cannot be static
         val.configs += type->configs.size;
      }

      if(type->isMemoryMapped){
         memoryMapBits[type->memoryMapBits] += 1;
      }

      if(type == versat->input){
         val.inputs += 1;
      }

      val.states += type->states.size;
      val.delays += type->nDelays;
      val.ios += type->nIOs;
      val.extraData += type->extraDataSize;
   }

   ComplexFUInstance* outputInstance = GetOutputInstance(accel);
   if(outputInstance){
      for(Edge* edge : accel->edges){
         if(edge->units[0].inst == outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[0].port) + 1;
         }
         if(edge->units[1].inst == outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[1].port) + 1;
         }
      }
   }
   val.totalOutputs = CalculateTotalOutputs(accel);

   // Handle static information
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->isStatic){
         StaticId id = {};

         id.name = inst->name;

         staticSeen.Insert(id,1);
         val.statics += inst->declaration->configs.size;
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
            int* possibleFind = staticSeen.Get(pair.first);

            if(!possibleFind){
               staticSeen.Insert(pair.first,1);
               val.statics += pair.second.configs.size;
            }
         }
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

   if(config == nullptr){
      return false;
   }

   int delta = config - topLevel->configAlloc.ptr;

   if(delta >= 0 && delta < topLevel->configAlloc.size){
      return false;
   } else {
      return true;
   }
}

FUDeclaration* RegisterSubUnit(Versat* versat,SizedString name,Accelerator* circuit){
   FUDeclaration decl = {};

   Arena* permanent = &versat->permanent;
   Arena* temp = &versat->temp;
   ArenaMarker marker(temp);

   decl.type = FUDeclaration::COMPOSITE;
   decl.name = name;

   // HACK, for now
   circuit->subtype = &decl;

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

   decl.fixedMultiEdgeCircuit = CopyAccelerator(versat,circuit,nullptr,true);

   {
   ArenaMarker marker(temp);
   AcceleratorView view = CreateAcceleratorView(circuit,temp);
   view.CalculateDelay(temp);
   FixDelays(versat,circuit,view);
   }

   decl.fixedDelayCircuit = circuit;

   AcceleratorView view = CreateAcceleratorView(circuit,temp);
   view.CalculateGraphData(temp);
   view.CalculateDAGOrdering(temp);
   view.CalculateDelay(temp);

   #if 1
   // Reorganize nodes based on DAG order
   DAGOrder order = view.order;
   Hashmap<ComplexFUInstance*,int> instanceToNewIndex = {};
   instanceToNewIndex.Init(temp,circuit->instances.Size());
   for(int i = 0; i < view.nodes.size; i++){
      ComplexFUInstance* inst = order.instances[i];

      instanceToNewIndex.Insert(inst,i);
   }

   Hashmap<int,int> newIndexToOld = {};
   newIndexToOld.Init(temp,circuit->instances.Size());
   int i = 0;
   for(ComplexFUInstance* inst : circuit->instances){
      int newIndex = instanceToNewIndex.GetOrFail(inst);

      newIndexToOld.Insert(newIndex,i);
      i++;
   }

   Hashmap<ComplexFUInstance*,ComplexFUInstance*> oldInstanceToNewInstance = {};
   oldInstanceToNewInstance.Init(temp,circuit->instances.Size());
   Pool<ComplexFUInstance> newInstances = {};
   for(int i = 0; i < view.nodes.size; i++){
      ComplexFUInstance* old = order.instances[i];

      ComplexFUInstance* newInst = newInstances.Alloc();

      oldInstanceToNewInstance.Insert(old,newInst);
      *newInst = *old;
   }

   circuit->instances.Clear();
   circuit->instances = newInstances;
   for(Edge* edge : circuit->edges){
      edge->units[0].inst = oldInstanceToNewInstance.GetOrFail(edge->units[0].inst);
      edge->units[1].inst = oldInstanceToNewInstance.GetOrFail(edge->units[1].inst);
   }
   #endif

   view = CreateAcceleratorView(circuit,permanent);
   view.CalculateGraphData(permanent);
   view.CalculateDAGOrdering(permanent);
   view.CalculateDelay(permanent);
   decl.temporaryOrder = view.order;

   OutputGraphDotFile(versat,view,true,"debug/test.dot");

   UnitValues val = CalculateAcceleratorValues(versat,decl.fixedDelayCircuit);

   decl.nDelays = val.delays;
   decl.nIOs = val.ios;
   decl.extraDataSize = val.extraData;
   decl.nStaticConfigs = val.statics;
   decl.isMemoryMapped = val.isMemoryMapped;
   decl.memoryMapBits = val.memoryMappedBits;
   decl.memAccessFunction = CompositeMemoryAccess;

   decl.inputDelays = PushArray<int>(permanent,val.inputs);

   for(int i = 0; i < val.inputs; i++){
      ComplexFUInstance* input = GetInputInstance(circuit,i);

      if(!input){
         continue;
      }

      decl.inputDelays[i++] = input->baseDelay;
   }

   decl.outputLatencies = PushArray<int>(permanent,val.outputs);

   ComplexFUInstance* outputInstance = GetOutputInstance(circuit);
   if(outputInstance){
      for(int i = 0; i < decl.outputLatencies.size; i++){
         decl.outputLatencies[i] = outputInstance->baseDelay;
      }
   }

   decl.configs = PushArray<Wire>(permanent,val.configs);
   decl.states = PushArray<Wire>(permanent,val.states);

   Hashmap<int,int> staticsSeen = {};
   staticsSeen.Init(temp,val.sharedUnits);

   int configIndex = 0;
   int stateIndex = 0;
   for(ComplexFUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      if(!inst->isStatic){
         if(inst->sharedEnable){
            if(staticsSeen.InsertIfNotExist(inst->sharedIndex,0)){
               for(Wire& wire : d->configs){
                  decl.configs[configIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),configIndex);
                  decl.configs[configIndex++].bitsize = wire.bitsize;
               }
            }
         } else {
            for(Wire& wire : d->configs){
               decl.configs[configIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),configIndex);
               decl.configs[configIndex++].bitsize = wire.bitsize;
            }
         }
      }

      for(Wire& wire : d->states){
         decl.states[stateIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
         decl.states[stateIndex++].bitsize = wire.bitsize;
      }
   }

   decl.configOffsets = PushArray<int>(permanent,circuit->instances.Size());
   decl.stateOffsets = PushArray<int>(permanent,circuit->instances.Size());
   decl.delayOffsets = PushArray<int>(permanent,circuit->instances.Size());
   decl.outputOffsets = PushArray<int>(permanent,circuit->instances.Size());
   decl.extraDataOffsets = PushArray<int>(permanent,circuit->instances.Size());
   Hashmap<int,int> sharedToOffset = {};
   sharedToOffset.Init(temp,val.configs);

   configIndex = 0;
   stateIndex = 0;
   int delayIndex = 0;
   int outputIndex = 0;
   int extraDataIndex = 0;
   int unitIndex = 0;
   //for(ComplexFUInstance* inst : circuit->instances){
   for(int i = 0 ; i < circuit->instances.Size(); i++){
      int oldPos = newIndexToOld.GetOrFail(i);
      ComplexFUInstance* inst = view.nodes[oldPos];

      UnitValues val = CalculateAcceleratorUnitValues(versat,inst);

      int savedIndex = -1;

      if(inst->sharedEnable){
         int* possibleOffset = sharedToOffset.Get(inst->sharedIndex);

         if(possibleOffset){
            savedIndex = configIndex;
            configIndex = *possibleOffset;
         } else {
            sharedToOffset.Insert(inst->sharedIndex,configIndex);
         }
      }

      FUDeclaration* d = inst->declaration;

      decl.configOffsets[oldPos] = configIndex;
      configIndex += d->configs.size;
      decl.stateOffsets[oldPos] = stateIndex;
      stateIndex += d->states.size;
      decl.delayOffsets[oldPos] = delayIndex;
      delayIndex += d->nDelays;
      decl.extraDataOffsets[oldPos] = extraDataIndex;
      extraDataIndex += d->extraDataSize;

      // Outputs follow the DAG order
      decl.outputOffsets[unitIndex] = outputIndex;
      outputIndex += val.totalOutputs;

      if(savedIndex != -1){
         configIndex = savedIndex;
      }

      unitIndex += 1;
   }
   decl.totalOutputs = outputIndex;
   decl.totalOutputs += decl.outputLatencies.size;

   // TODO: Change unit delay type inference. Only care about delay type to upper levels.
   // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
   bool hasSourceDelay = false;
   bool hasSinkDelay = false;
   bool implementsDone = false;

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
   //Hashmap<StaticId,int> staticMap(temp,1000);
   res->staticUnits.Init(permanent,1000); // TODO: Set correct number of elements
   int staticOffset = 0;
   // Start by collecting all the existing static allocated units in subinstances
   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->declaration->type == FUDeclaration::COMPOSITE ||
         inst->declaration->type == FUDeclaration::ITERATIVE){

         for(auto pair : inst->declaration->staticUnits){
            res->staticUnits.InsertIfNotExist(pair.first,pair.second);
            staticOffset = std::max(staticOffset,pair.second.offset + pair.second.configs.size);
         }
      }
   }

   // Add this units static instances
   for(ComplexFUInstance* inst : circuit->instances){
      if(inst->isStatic){
         StaticId id = {};
         id.name = inst->name;
         id.parent = res;

         StaticData data = {};
         data.configs = inst->declaration->configs;
         data.offset = staticOffset;

         res->staticUnits.InsertIfNotExist(id,data);
         staticOffset += inst->declaration->configs.size;
      }
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
   return out;
   #if 0

   FUDeclaration* type = inst->declaration;
   int* delay = (int*) inst->extraData;
   ComplexFUInstance* dataInst = (ComplexFUInstance*) GetInstanceByName(inst->compositeAccel,"data");

   //LockAccelerator(inst->compositeAccel,Accelerator::Locked::ORDERED);
   //Assert(inst->iterative);

   if(*delay == -1){ // For loop part
      AcceleratorRunComposite(inst);
   } else if(*delay == inst->delay[0]){
      for(int i = 0; i < type->inputDelays.size; i++){
         int val = GetInputValue(inst,i);
         SetInputValue(inst->iterative,i,val);
      }

      AcceleratorRun(inst->iterative);

      for(int i = 0; i < type->outputLatencies.size; i++){
         int val = GetOutputValue(inst->iterative,i);
         out[i] = val;
      }

      ComplexFUInstance* secondData = (ComplexFUInstance*) GetInstanceByName(inst->iterative,"data");

      for(int i = 0; i < secondData->declaration->outputLatencies.size; i++){
         dataInst->outputs[i] = secondData->outputs[i];
         dataInst->storedOutputs[i] = secondData->outputs[i];
      }

      *delay = -1;
   } else {
      *delay += 1;
   }

   return out;
   #endif
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
   declaration.staticUnits = combType->staticUnits;
   declaration.nDelays = 1; // At least one delay

   // Accelerator computed values
   UnitValues val = CalculateAcceleratorValues(versat,decl->forLoop);
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

   declaration.inputDelays = PushArray<int>(&versat->permanent,val.inputs);
   declaration.outputLatencies = PushArray<int>(&versat->permanent,val.outputs);
   Memset(declaration.inputDelays,0);
   Memset(declaration.outputLatencies,decl->latency);

   FUDeclaration* registeredType = RegisterFU(versat,declaration);

   if(!versat->debug.outputAccelerator){
      return registeredType;
   }

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

   #if 0
   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("firstComb",firstPartComb,"ComplexFUInstance");
   TemplateSetCustom("secondComb",secondPartComb,"ComplexFUInstance");
   TemplateSetCustom("firstOut",decl->initial->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("secondOut",decl->forLoop->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("firstData",firstData,"ComplexFUInstance");
   TemplateSetCustom("secondData",secondData,"ComplexFUInstance");
   #endif

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

void PopulateAccelerator2(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>& staticMap);

static void AcceleratorRunStart(Accelerator* accel,Hashmap<StaticId,StaticData>& staticMap){
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel,&accel->versat->temp,true); inst; inst = iter.Next()){
      FUDeclaration* type = inst->declaration;

      if(type->startFunction){
         int* startingOutputs = type->startFunction(inst);

         if(startingOutputs){
            memcpy(inst->outputs,startingOutputs,inst->declaration->outputLatencies.size * sizeof(int));
            memcpy(inst->storedOutputs,startingOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         }
      }
   }
}

static bool AcceleratorDone(Accelerator* accel){
   bool done = true;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->declaration->fixedDelayCircuit){
         bool subDone = AcceleratorDone(inst->declaration->fixedDelayCircuit);
         done &= subDone;
      } else if(inst->declaration->implementsDone && !inst->done){
         return false;
      }
   }

   return done;
}

void PopulateAccelerator(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>& staticMap){
   int configIndex = 0;
   int stateIndex = 0;
   int delayIndex = 0;
   int outputIndex = 0;
   int extraDataIndex = 0;

   for(ComplexFUInstance* inst : accel->instances){
      FUDeclaration* decl = inst->declaration;
      UnitValues val = CalculateAcceleratorUnitValues(accel->versat,inst);

      if(inst->isStatic){
         StaticId id = {};
         id.parent = topDeclaration;
         id.name = inst->name;

         StaticData* staticInfo = staticMap.Get(id);
         Assert(staticInfo);
         inst->config = &inter.statics.ptr[staticInfo->offset];
      } else{
         inst->config = &inter.config.ptr[topDeclaration->configOffsets[configIndex++]];
      }
      inst->state = &inter.state.ptr[topDeclaration->stateOffsets[stateIndex++]];
      inst->delay = &inter.delay.ptr[topDeclaration->delayOffsets[delayIndex++]];
      inst->outputs = &inter.outputs.ptr[topDeclaration->outputOffsets[outputIndex]];
      inst->storedOutputs = &inter.storedOutputs.ptr[topDeclaration->outputOffsets[outputIndex++]];
      inst->extraData = &inter.extraData.ptr[topDeclaration->extraDataOffsets[extraDataIndex++]];
   }
}

void PopulateAccelerator2(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>& staticMap){
   STACK_ARENA(temp,Kilobyte(16));

   Hashmap<int,int*> sharedToConfigPtr = {};
   sharedToConfigPtr.Init(&temp,accel->instances.Size());

   for(ComplexFUInstance* inst : accel->instances){
      FUDeclaration* decl = inst->declaration;
      UnitValues val = CalculateAcceleratorUnitValues(accel->versat,inst);

      inst->config = nullptr;
      inst->state = nullptr;
      inst->delay = nullptr;
      inst->outputs = nullptr;
      inst->storedOutputs = nullptr;
      inst->extraData = nullptr;

      if(inst->isStatic){
         StaticId id = {};
         id.parent = topDeclaration;
         id.name = inst->name;

         StaticData* staticInfo = staticMap.Get(id);
         Assert(staticInfo);
         inst->config = &inter.statics.ptr[staticInfo->offset];
      } else if(inst->sharedEnable){
         int** ptr = sharedToConfigPtr.Get(inst->sharedIndex);

         if(ptr){
            inst->config = *ptr;
         } else {
            inst->config = inter.config.Push(decl->configs.size);
            sharedToConfigPtr.Insert(inst->sharedIndex,inst->config);
         }
      } else if(decl->configs.size){
         inst->config = inter.config.Push(decl->configs.size);
      }
      if(decl->states.size){
         inst->state = inter.state.Push(decl->states.size);
      }
      if(decl->nDelays){
         inst->delay = inter.delay.Push(decl->nDelays);
      }
      #if 1
      if(val.totalOutputs){
         inst->outputs = inter.outputs.Push(val.totalOutputs);
         inst->storedOutputs = inter.storedOutputs.Push(val.totalOutputs);
      }
      #endif
      if(decl->extraDataSize){
         inst->extraData = inter.extraData.Push(decl->extraDataSize);
      }
   }
}

static void AcceleratorRunIter(AcceleratorIterator iter,Hashmap<StaticId,StaticData>& staticMap);
static void AcceleratorRunComposite(AcceleratorIterator iter,Hashmap<StaticId,StaticData>& staticMap){
   // Set accelerator input to instance input
   ComplexFUInstance* compositeInst = iter.Current();
   Assert(compositeInst->declaration->type == FUDeclaration::COMPOSITE);
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   // Order is very important. Need to populate before setting input values
   AcceleratorIterator it = iter.LevelBelowIterator();

   // Cannot access input here because not populated
   for(int i = 0; i < compositeInst->graphData->singleInputs.size; i++){
      if(compositeInst->graphData->singleInputs[i].inst == nullptr){
         continue;
      }

      ComplexFUInstance* input = GetInputInstance(accel,i);
      Assert(input);
      int val = GetInputValue(compositeInst,i);
      for(int ii = 0; ii < input->graphData->outputs; ii++){
         input->outputs[ii] = val;
         input->storedOutputs[ii] = val;
      }
   }

   AcceleratorRunIter(it,staticMap);

   // Set output instance value to accelerator output
   ComplexFUInstance* output = GetOutputInstance(accel);
   if(output){
      for(int ii = 0; ii < output->graphData->singleInputs.size; ii++){
         if(output->graphData->singleInputs[ii].inst == nullptr){
            continue;
         }

         int val = GetInputValue(output,ii);
         compositeInst->outputs[ii] = val;
         compositeInst->storedOutputs[ii] = val;
      }
   }
}

static void AcceleratorRunIter(AcceleratorIterator iter,Hashmap<StaticId,StaticData>& staticMap){
   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Skip()){
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         AcceleratorRunComposite(iter,staticMap);
      } else {
         int* newOutputs = inst->declaration->updateFunction(inst);

         if(inst->declaration->outputLatencies.size && inst->declaration->outputLatencies[0] == 0 && inst->graphData->nodeType != GraphComputedData::TAG_SOURCE){
            memcpy(inst->outputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         } else {
            memcpy(inst->storedOutputs,newOutputs,inst->declaration->outputLatencies.size * sizeof(int));
         }
      }
   }
}

void AcceleratorRun(Accelerator* accel){
   static int numberRuns = 0;
   int time = 0;

   Arena* arena = &accel->versat->temp;
   ArenaMarker marker(arena);

   // TODO: Eventually retire this portion. Make the Accelerator structure a map, maybe a std::unordered_map
   Hashmap<StaticId,StaticData> staticUnits = {};
   staticUnits.Init(arena,accel->staticInfo.Size());
   for(StaticInfo* info : accel->staticInfo){
      staticUnits.Insert(info->id,info->data);
   }

   #if 1
   FUDeclaration base = {};
   base.name = MakeSizedString("Top");
   accel->subtype = &base;

   AcceleratorView view = CreateAcceleratorView(accel,arena);
   view.CalculateDelay(arena);
   view.CalculateDAGOrdering(arena);
   SetDelayRecursive(accel);
   #endif

   FILE* accelOutputFile = nullptr;
   if(accel->versat->debug.outputVCD){
      char buffer[128];
      sprintf(buffer,"debug/accelRun%d.vcd",numberRuns++);
      accelOutputFile = fopen(buffer,"w");
      Assert(accelOutputFile);

      PrintVCDDefinitions(accelOutputFile,accel);
   }

   #if 1
   AcceleratorIterator iter = {};
   iter.Start(accel,arena,true);

   //CheckMemory(iter);
   #endif

   AcceleratorRunStart(accel,staticUnits);
   iter.Start(accel,arena,true);
   AcceleratorRunIter(iter,staticUnits);
   //AcceleratorRunTopLevel(view,staticUnits);

   if(accel->versat->debug.outputVCD){
      PrintVCD(accelOutputFile,accel,time++,0);
   }

   int cycle;
   for(cycle = 0; 1; cycle++){ // Max amount of iterations
      Assert(accel->outputAlloc.size == accel->storedOutputAlloc.size);
      memcpy(accel->outputAlloc.ptr,accel->storedOutputAlloc.ptr,accel->outputAlloc.size * sizeof(int));

      iter.Start(accel,arena,true);
      AcceleratorRunIter(iter,staticUnits);
      //AcceleratorRunTopLevel(view,staticUnits);

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

   PortInstance& other = instance->graphData->singleInputs[index];

   #if 1
   if(!other.inst){
      return 0;
   }
   #endif

   return other.inst->outputs[other.port];
}

int GetNumberOfInputs(FUInstance* inst){
   return inst->declaration->inputDelays.size;
}

int GetNumberOfOutputs(FUInstance* inst){
   return inst->declaration->outputLatencies.size;
}

FUInstance* CreateOrGetInput(Accelerator* accel,SizedString name,int portNumber){
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration == accel->versat->input && inst->id == portNumber){
         return inst;
      }
   }

   FUInstance* inst = CreateFUInstance(accel,accel->versat->input,name,false,false);
   inst->id = portNumber;

   return inst;
}

int GetNumberOfInputs(Accelerator* accel){
   int count = 0;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration == accel->versat->input){
         count += 1;
      }
   }

   return count;
}

int GetNumberOfOutputs(Accelerator* accel){
   NOT_IMPLEMENTED;
   return 0;
}

void SetInputValue(Accelerator* accel,int portNumber,int number){
   Assert(accel->outputAlloc.ptr);

   ComplexFUInstance* inst = GetInputInstance(accel,portNumber);
   Assert(inst);

   inst->outputs[0] = number;
   inst->storedOutputs[0] = number;
}

int GetOutputValue(Accelerator* accel,int portNumber){
   ComplexFUInstance* inst = GetOutputInstance(accel);

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
      for(ComplexFUInstance* child : inst->declaration->fixedDelayCircuit->instances){
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
      char ch = name[i];

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

int CalculateMemoryUsage(Versat* versat){
   int totalSize = 0;
   for(Accelerator* accel : versat->accelerators){
      int accelSize = accel->instances.MemoryUsage() + accel->edges.MemoryUsage();
      accelSize += MemoryUsage(accel->configAlloc);
      accelSize += MemoryUsage(accel->stateAlloc);
      accelSize += MemoryUsage(accel->delayAlloc);
      accelSize += MemoryUsage(accel->staticAlloc);
      accelSize += MemoryUsage(accel->outputAlloc);
      accelSize += MemoryUsage(accel->storedOutputAlloc);
      accelSize += MemoryUsage(accel->extraDataAlloc);

      totalSize += accelSize;
   }

   totalSize += versat->permanent.used; // Not total allocated, because it would distort the actual data we care about

   return totalSize;
}

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst){

}























