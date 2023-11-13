#include "versat.hpp"

#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <unordered_map>

#include "printf.h"

#include "thread.hpp"
#include "type.hpp"
#include "debug.hpp"
#include "parser.hpp"
#include "configurations.hpp"
#include "debugGUI.hpp"
#include "graph.hpp"
#include "acceleratorSimulation.hpp"
#include "unitVerilation.hpp"
#include "verilogParsing.hpp"
#include "acceleratorStats.hpp"

#include "templateEngine.hpp"
#include "templateData.hpp"

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

//#define IMPLEMENT_VERILOG_UNITS
//#include "wrapper.inc"
//#include "templateData.inc"

static int ones[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static int* DefaultInitFunction(FUInstance* inst){
  inst->done = true;
  return nullptr;
}

static int* DefaultUpdateFunction(FUInstance* inst,Array<int> inputs){
  static int out[99];

  for(int i = 0; i < inputs.size; i++){
    out[i] = inputs[i];
  }

  return out;
}

static FUDeclaration* RegisterCircuitInput(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("CircuitInput");
  decl.inputDelays = Array<int>{zeros,0};
  decl.outputLatencies = Array<int>{zeros,1};
  decl.initializeFunction = DefaultInitFunction;
  decl.delayType = DelayType::DELAY_TYPE_SOURCE_DELAY;
  decl.type = FUDeclaration::SPECIAL;

  return RegisterFU(versat,decl);
}
static FUDeclaration* RegisterCircuitOutput(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("CircuitOutput");
  decl.inputDelays = Array<int>{zeros,50};
  decl.outputLatencies = Array<int>{zeros,0};
  decl.initializeFunction = DefaultInitFunction;
  decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
  decl.type = FUDeclaration::SPECIAL;

  return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterData(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("Data");
  decl.inputDelays = Array<int>{zeros,50};
  decl.outputLatencies = Array<int>{ones,50};
  decl.initializeFunction = DefaultInitFunction;
  decl.updateFunction = DefaultUpdateFunction;
  decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;

  return RegisterFU(versat,decl);
}
static int* UpdatePipelineRegister(FUInstance* inst,Array<int> inputs){
  static int out;

  out = inputs[0];
  inst->done = true;

  return &out;
}
static FUDeclaration* RegisterPipelineRegister(Versat* versat){
  FUDeclaration decl = {};
  static int ones[] = {1};

  decl.name = STRING("PipelineRegister");
  decl.inputDelays = Array<int>{zeros,1};
  decl.outputLatencies = Array<int>{ones,1};
  decl.initializeFunction = DefaultInitFunction;
  decl.updateFunction = UpdatePipelineRegister;
  decl.isOperation = true;
  decl.operation = "{0}_{1} <= {2}";

  return RegisterFU(versat,decl);
}

static int* LiteralStartFunction(FUInstance* inst){
  static int out;
  out = inst->literal;
  inst->done = true;
  return &out;
}
static int* LiteraltUpdateFunction(FUInstance* inst,Array<int> inputs){
  static int out;
  out = inst->literal;
  return &out;
}

static FUDeclaration* RegisterLiteral(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("Literal");
  decl.outputLatencies = Array<int>{zeros,1};
  decl.startFunction = LiteralStartFunction;
  decl.updateFunction = LiteraltUpdateFunction;

  return RegisterFU(versat,decl);
}

static int* UnaryNot(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = ~inputs[0];
  inst->done = true;
  return (int*) &out;
}
static int* UnaryNegate(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = -inputs[0];
  inst->done = true;
  return (int*) &out;
}
static int* BinaryXOR(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = inputs[0] ^ inputs[1];
  inst->done = true;
  return (int*) &out;
}
static int* BinaryADD(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = inputs[0] + inputs[1];
  inst->done = true;
  return (int*) &out;
}
static int* BinarySUB(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = inputs[0] - inputs[1];
  inst->done = true;
  return (int*) &out;
}
static int* BinaryAND(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = inputs[0] & inputs[1];
  inst->done = true;
  return (int*) &out;
}
static int* BinaryOR(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  out = inputs[0] | inputs[1];
  inst->done = true;
  return (int*) &out;
}
static int* BinaryRHR(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  unsigned int value = inputs[0];
  unsigned int shift = inputs[1];
  out = (value >> shift) | (value << (32 - shift));
  inst->done = true;
  return (int*) &out;
}
static int* BinaryRHL(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  unsigned int value = inputs[0];
  unsigned int shift = inputs[1];
  out = (value << shift) | (value >> (32 - shift));
  inst->done = true;
  return (int*) &out;
}
static int* BinarySHR(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  unsigned int value = inputs[0];
  unsigned int shift = inputs[1];
  out = (value >> shift);
  inst->done = true;
  return (int*) &out;
}
static int* BinarySHL(FUInstance* inst,Array<int> inputs){
  static unsigned int out;
  unsigned int value = inputs[0];
  unsigned int shift = inputs[1];
  out = (value << shift);
  inst->done = true;
  return (int*) &out;
}

static void RegisterOperators(Versat* versat){
  struct Operation{
    const char* name;
    FUUpdateFunction function;
    const char* operation;
  };

  Operation unary[] =  {{"NOT",UnaryNot,"{0}_{1} = ~{2}"},
                        {"NEG",UnaryNegate,"{0}_{1} = -{2}"}};
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
    decl.name = STRING(unary[i].name);
    decl.updateFunction = unary[i].function;
    decl.operation = unary[i].operation;
    RegisterFU(versat,decl);
  }

  decl.inputDelays = Array<int>{zeros,2};
  for(unsigned int i = 0; i < ARRAY_SIZE(binary); i++){
    decl.name = STRING(binary[i].name);
    decl.updateFunction = binary[i].function;
    decl.operation = binary[i].operation;
    RegisterFU(versat,decl);
  }
}

static Pool<FUDeclaration> basicDeclarations;
namespace BasicDeclaration{
  FUDeclaration* buffer;
  FUDeclaration* fixedBuffer;
  FUDeclaration* input;
  FUDeclaration* output;
  FUDeclaration* multiplexer;
  FUDeclaration* combMultiplexer;
  FUDeclaration* stridedMerge;
  FUDeclaration* timedMultiplexer;
  FUDeclaration* pipelineRegister;
  FUDeclaration* data;
}

namespace BasicTemplates{
  CompiledTemplate* acceleratorTemplate;
  CompiledTemplate* topAcceleratorTemplate;
  CompiledTemplate* dataTemplate;
  CompiledTemplate* wrapperTemplate;
  CompiledTemplate* acceleratorHeaderTemplate;
  CompiledTemplate* externalPortmapTemplate;
  CompiledTemplate* externalPortTemplate;
  CompiledTemplate* externalInstTemplate;
  CompiledTemplate* iterativeTemplate;
}

static Value HexValue(Value in,Arena* out){
  static char buffer[128];

  int number = ConvertValue(in,ValueType::NUMBER,nullptr).number;

  int size = sprintf(buffer,"0x%x",number);

  Value res = {};
  res.type = ValueType::STRING;
  res.str = PushString(out,String{buffer,size});

  return res;
}

static Value EscapeString(Value val,Arena* out){
  Assert(val.type == ValueType::SIZED_STRING || val.type == ValueType::STRING);
  Assert(val.isTemp);

  String str = val.str;

  String escaped = PushString(out,str);
  char* view = (char*) escaped.data;

  for(int i = 0; i < escaped.size; i++){
    char ch = view[i];

    if(   (ch >= 'a' && ch <= 'z')
          || (ch >= 'A' && ch <= 'Z')
          || (ch >= '0' && ch <= '9')){
      continue;
    } else {
      view[i] = '_'; // Replace any foreign symbol with a underscore
    }
  }

  Value res = val;
  res.str = escaped;

  return res;
}

int CountNonOperationChilds(Accelerator* accel){
  if(accel == nullptr){
    return 0;
  }

  int count = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(IsTypeHierarchical(ptr->inst->declaration)){
      count += CountNonOperationChilds(ptr->inst->declaration->fixedDelayCircuit);
    }

    if(!ptr->inst->declaration->isOperation && ptr->inst->declaration->type != FUDeclaration::SPECIAL){
      count += 1;
    }
  }

  return count;
}

extern "C" void DebugAcceleratorC(Accelerator* accel){
#ifdef VERSAT_DEBUG
  DebugAccelerator(accel);
#endif
}

extern "C" Versat* InitVersatC(int base,int numberConfigurations,bool initUnits){
  return InitVersat(base,numberConfigurations,initUnits);
}

Versat* InitVersat(int base,int numberConfigurations,bool initUnits){
  static Versat versatInst = {};
  static bool doneOnce = false;

  Assert(!doneOnce); // For now, only allow one Versat instance
  doneOnce = true;

  InitDebug();
  RegisterTypes();

  //InitThreadPool(16);
  Versat* versat = &versatInst;

  versat->debug.outputAccelerator = true;
  versat->debug.outputVersat = true;

  versat->debug.dotFormat = GRAPH_DOT_FORMAT_NAME;

  versat->numberConfigurations = numberConfigurations;
  versat->base = base;

#ifdef x86
  versat->temp = InitArena(Megabyte(256));
#else
  versat->temp = InitArena(Gigabyte(8));
#endif // x86
  versat->permanent = InitArena(Megabyte(256));

  InitializeTemplateEngine(&versat->permanent);

  // Technically, if more than 1 versat in the future, could move this outside
  BasicTemplates::acceleratorTemplate = CompileTemplate(versat_accelerator_template,"accel",&versat->permanent);
  BasicTemplates::topAcceleratorTemplate = CompileTemplate(versat_top_instance_template,"top",&versat->permanent);
  BasicTemplates::wrapperTemplate = CompileTemplate(versat_wrapper_template,"wrapper",&versat->permanent);
  BasicTemplates::acceleratorHeaderTemplate = CompileTemplate(versat_header_template,"header",&versat->permanent);
  BasicTemplates::externalPortmapTemplate = CompileTemplate(external_memory_portmap_template,"ext_portmap",&versat->permanent);
  BasicTemplates::externalPortTemplate = CompileTemplate(external_memory_port_template,"ext_port",&versat->permanent);
  BasicTemplates::externalInstTemplate = CompileTemplate(external_memory_inst_template,"ext_inst",&versat->permanent);
  BasicTemplates::iterativeTemplate = CompileTemplate(versat_iterative_template,"iter",&versat->permanent);

  FUDeclaration nullDeclaration = {};
  nullDeclaration.inputDelays = Array<int>{zeros,1};
  nullDeclaration.outputLatencies = Array<int>{zeros,1};

  RegisterPipeOperation(STRING("Hex"),HexValue);
  RegisterPipeOperation(STRING("Identify"),EscapeString);
  RegisterPipeOperation(STRING("CountNonOperationChilds"),[](Value val,Arena* out){
    Accelerator* accel = *((Accelerator**) val.custom);

    val = MakeValue(CountNonOperationChilds(accel));
    return val;
  });

  // Kinda of a hack, for now. We register on versat and then move the basic declarations out
  RegisterFU(versat,nullDeclaration);
  RegisterOperators(versat);
  RegisterPipelineRegister(versat);
  RegisterCircuitInput(versat);
  RegisterCircuitOutput(versat);
  RegisterData(versat);
  RegisterLiteral(versat);

  for(FUDeclaration* decl : versat->declarations){
    FUDeclaration* newSpace = basicDeclarations.Alloc();
    *newSpace = *decl;
  }
  versat->declarations.Clear(false);

  // This ones are specific for the instance
  //RegisterAllVerilogUnitsVerilog(versat);

  // TODO: Not a good approach. Fix it when finishing versat compiler
  if(initUnits){
    BasicDeclaration::buffer = GetTypeByName(versat,STRING("Buffer"));
    BasicDeclaration::fixedBuffer = GetTypeByName(versat,STRING("FixedBuffer"));
    BasicDeclaration::pipelineRegister = GetTypeByName(versat,STRING("PipelineRegister"));
    BasicDeclaration::multiplexer = GetTypeByName(versat,STRING("Mux2"));
    BasicDeclaration::combMultiplexer = GetTypeByName(versat,STRING("CombMux2"));
    BasicDeclaration::stridedMerge = GetTypeByName(versat,STRING("StridedMerge"));
    BasicDeclaration::timedMultiplexer = GetTypeByName(versat,STRING("TimedMux"));
    BasicDeclaration::input = GetTypeByName(versat,STRING("CircuitInput"));
    BasicDeclaration::output = GetTypeByName(versat,STRING("CircuitOutput"));
    BasicDeclaration::data = GetTypeByName(versat,STRING("Data"));
  }

  //Log(LogModule::TOP_SYS,LogLevel::INFO,"Init versat");

  return versat;
}

void Free(Versat* versat){
#if 0
  Arena* arena = &versat->temp;
  for(Accelerator* accel : versat->accelerators){
#if 0
    if(accel->type == Accelerator::CIRCUIT){
      continue;
    }
#endif

    // Cannot free this way because most of accelerators are bound to a FUDeclaration but are incomplete.
    // Therefore
    AcceleratorIterator iter = {};
    for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      if(inst->declaration->destroyFunction){
        inst->declaration->destroyFunction(inst);
      }
    }
  }
#endif

  for(Accelerator* accel : versat->accelerators){
    Free(&accel->configAlloc);
    Free(&accel->stateAlloc);
    //Free(&accel->delayAlloc);
    //Free(&accel->staticAlloc);
    Free(&accel->extraDataAlloc);
    Free(&accel->externalMemoryAlloc);
    Free(&accel->outputAlloc);
    Free(&accel->storedOutputAlloc);
    //Free(&accel->debugDataAlloc);

    accel->staticUnits.clear();
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

static int AccessMemory(FUInstance* inst,int address, int value, int write){
  FUInstance* instance = (FUInstance*) inst->declarationInstance;
  instance->memMapped = inst->memMapped;
  instance->config = inst->config;
  instance->state = inst->state;
  instance->delay = inst->delay;
  instance->externalMemory = inst->externalMemory;
  instance->outputs = inst->outputs;
  instance->storedOutputs = inst->storedOutputs;
  instance->extraData = inst->extraData;

  int res = instance->declaration->memAccessFunction(instance,address,value,write);

  return res;
}

static int CompositeMemoryAccess(FUInstance* instance,int address,int value,int write){
  int offset = 0;

  // TODO: Probably should use Accelerator Iterator ?
  FOREACH_LIST(InstanceNode*,ptr,instance->declaration->fixedDelayCircuit->allocated){
    FUInstance* inst = ptr->inst;

    if(!inst->declaration->isMemoryMapped){
      continue;
    }

    int mappedWords = 1 << inst->declaration->memoryMapBits;
    if(mappedWords){
      if(address >= offset && address <= offset + mappedWords){
        if(write){
          AccessMemory(inst,address - offset,value,1);
          return 0;
        } else {
          int result = AccessMemory(inst,address - offset,0,0);
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

extern "C" void UnitWrite(Versat* versat,Accelerator* accel,int addr,int val){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Skip()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    iptr memAddress = (iptr) inst->memMapped;
    iptr delta = (1 << (decl->memoryMapBits + 2));
    if(addr >= memAddress && addr < (memAddress + delta)){
      AccessMemory(inst,addr - memAddress,val,1);
      return;
    }
  }

  assert("Failed to write to unit.");
}

extern "C" int UnitRead(Versat* versat,Accelerator* accel,int addr){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Skip()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    iptr memAddress = (iptr) inst->memMapped;
    iptr delta = (1 << (decl->memoryMapBits + 2));
    if(addr >= memAddress && addr < (memAddress + delta)){
      return AccessMemory(inst,addr - memAddress,0,0); // Convert to symbol space
    }
  }
  assert("Failed to read unit");
  return 0; // Unreachable, prevent compiler complaining
}

void VersatMemoryCopy(FUInstance* instance,int* dest,int* data,int size){
  FUInstance* inst = (FUInstance*) instance;
  FUDeclaration* decl = inst->declaration;

  // Memory mapped must pass through VersatUnitWrite functions
  int memMappedDelta = dest - inst->memMapped;
  if(memMappedDelta >= 0 && memMappedDelta < (1 << decl->memoryMapBits)){
    Assert(memMappedDelta + size <= (1 << decl->memoryMapBits));

    for(int i = 0; i < size; i++){
      AccessMemory(instance,i + memMappedDelta,data[i],1);
    }
  } else {
    Memcpy(dest,data,size);
  }
}

void VersatMemoryCopy(FUInstance* instance,int* dest,Array<int> data){
  VersatMemoryCopy(instance,dest,data.data,data.size);
}

FUDeclaration* GetTypeByName(Versat* versat,String name){
  for(FUDeclaration* decl : versat->declarations){
    if(CompareString(decl->name,name)){
      return decl;
    }
  }

  for(FUDeclaration* decl : basicDeclarations){
    if(CompareString(decl->name,name)){
      return decl;
    }
  }

  LogFatal(LogModule::TOP_SYS,"Didn't find the following type: %.*s",UNPACK_SS(name));

  return nullptr;
}

// TODO: Does anything use this function or is it only the Get...Name2 that is used?
#if 0
static FUInstance* GetInstanceByHierarchicalName(Accelerator* accel,HierarchicalName* hier){
  Assert(hier != nullptr);

  HierarchicalName* savedHier = hier;
  FUInstance* res = nullptr;
  FOREACH_LIST(ptr,accel->allocated){
  FUInstance* inst = ptr->inst;
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

      // TODO: The type qualifier ideia does not appear to be very good, even if it technically solves the problem
      // TODO: With 2 stage, we can generate all the different structs to different types and let the programmer select the one it wants to use. Can safely remove this portion of code
      Token possibleTypeQualifier = hierTok.PeekToken();

      //       If the merge problem can be solved by behaving diferently based on currently activated merged accelerator, remove this portion of code
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
#endif

// Function used by pc-emul/versat.cpp
extern "C" Accelerator* CreateSimulableAccelerator(Versat* versat,const char* acceleratorTypeName){
  Accelerator* accel = CreateAccelerator(versat);
  FUDeclaration* type = GetTypeByName(versat,STRING(acceleratorTypeName));
  CreateFUInstance(accel,type,STRING("TOP"));

  InitializeAccelerator(versat,accel,&versat->temp);
  return accel;
}

extern "C" void SignalLoopC(Accelerator* accel){
  STACK_ARENA(temp,Kilobyte(4));
  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,&temp,true); node; node = iter.Skip()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    if(decl->signalFunction){
      decl->signalFunction(inst);
    }
  }
}

extern "C" void* GetStartOfConfig(Accelerator* accel){
  return accel->configAlloc.ptr;
}

extern "C" void* GetStartOfState(Accelerator* accel){
  return accel->stateAlloc.ptr;
}

static String GetFullHierarchicalName(HierarchicalName* head,Arena* arena){
  Byte* mark = MarkArena(arena);

  bool first = true;
  FOREACH_LIST(HierarchicalName*,ptr,head){
    if(!first){
      PushString(arena,STRING("."));
    }
    PushString(arena,"%.*s",UNPACK_SS(ptr->name));
    first = false;
  }

  String res = PointArena(arena,mark);
  return res;
}

static FUInstance* GetInstanceByHierarchicalName2(AcceleratorIterator iter,HierarchicalName* hier,Arena* arena){
  HierarchicalName* savedHier = hier;
  FUInstance* res = nullptr;
  for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
    FUInstance* inst = node->inst;
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
          AcceleratorIterator it = iter.LevelBelowIterator();
          FUInstance* res = GetInstanceByHierarchicalName2(it,hier->next,arena);
          if(res){
            return res;
          }
        } else if(!hier->next){ // Correct name and type (if specified) and no further hierarchical name to follow
               Accelerator* accel = iter.topLevel;

               //#if 0
               //return node->inst;
               //#else
               if(iter.GetFullLevel() == 0){
                 return node->inst;
               } else {
                 FUInstance* instBuffer = accel->subInstances.Alloc();
                 *instBuffer = *((FUInstance*)inst);
                 instBuffer->declarationInstance = inst;

                 Accelerator* accel = iter.topLevel;
                 Arena arena = SubArena(accel->accelMemory,256);
                 instBuffer->name = GetFullHierarchicalName(savedHier,&arena);

                 return instBuffer;
               }
               //#endif
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

static FUInstance* vGetInstanceByName_(AcceleratorIterator iter,Arena* arena,int argc,va_list args){
  HierarchicalName fullName = {};
  HierarchicalName* namePtr = &fullName;
  HierarchicalName* lastPtr = nullptr;

  for (int i = 0; i < argc; i++){
    char* str = va_arg(args, char*);
    int arguments = parse_printf_format(str,0,nullptr);

    if(namePtr == nullptr){
      HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);
      *newBlock = {};

      lastPtr->next = newBlock;
      namePtr = newBlock;
    }

    String name = {};
    if(arguments){
      name = vPushString(arena,str,args);
      i += arguments;
#ifdef x86 // For some reason this is needed for 32 bits but not for 64 bits. Hack for now, but need to do a full debug and see what is happening
      for(int ii = 0; ii < arguments; ii++){
        /* int unused = */ va_arg(args, int); // Need to consume something
      }
#endif
    } else {
      name = PushString(arena,"%s",str);
    }

    Tokenizer tok(name,".",{});
    while(!tok.Done()){
      if(namePtr == nullptr){
        HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);
        *newBlock = {};

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

  FUInstance* res = GetInstanceByHierarchicalName2(iter,&fullName,arena);

  if(!res){
    String name = GetFullHierarchicalName(&fullName,arena);

    printf("Couldn't find: %.*s\n",UNPACK_SS(name));

    Assert(res);
  }

  return res;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){
  Arena* temp = &circuit->versat->temp;
  BLOCK_REGION(temp);

  va_list args;
  va_start(args,argc);

  AcceleratorIterator iter = {};
  iter.Start(circuit,temp,true);

  FUInstance* res = vGetInstanceByName_(iter,temp,argc,args);

  va_end(args);

  return res;
}

FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* instance,int argc, ...){
  Arena* temp = &topLevel->versat->temp;
  BLOCK_REGION(temp);

  va_list args;
  va_start(args,argc);

  FUInstance* inst = (FUInstance*) instance;
  Assert(inst->declaration->fixedDelayCircuit);

  AcceleratorIterator iter = {};
  iter.Start(topLevel,inst,temp,true);

  FUInstance* res = vGetInstanceByName_(iter,temp,argc,args);

  va_end(args);

  return res;
}

// TODO: Bunch of functions that could go to graph.hpp
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;

  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->inputDelays.size);
  Assert(outIndex < outDecl->outputLatencies.size);

  Accelerator* accel = out->accel;

  FOREACH_LIST(Edge*,edge,accel->edges){
    if(edge->units[0].inst == (FUInstance*) out &&
       edge->units[0].port == outIndex &&
       edge->units[1].inst == (FUInstance*) in &&
       edge->units[1].port == inIndex &&
       edge->delay == delay) {
      return edge;
    }
  }

  return nullptr;
}

Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  Edge* edge = ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,nullptr);

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

  FOREACH_LIST(Edge*,edge,accel->edges){
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

Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previousEdge){
  Edge* edge = ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,previousEdge);

  return edge;
}

int EvalRange(ExpressionRange range,Array<ParameterExpression> expressions){
  // TODO: Right now nullptr indicates that the wire does not exist. Do not know if it's worth to make it more explicit or not. Appears to be fine for now.
  if(range.top == nullptr || range.bottom == nullptr){
	Assert(range.top == range.bottom); // They must both be nullptr if one is
	return 0;
  }

  int top = Eval(range.top,expressions).number;
  int bottom = Eval(range.bottom,expressions).number;
  int size = (top - bottom) + 1;
  Assert(size > 0);

  return size;
}

// TODO: Need to remake this function and probably ModuleInfo has the versat compiler change is made
FUDeclaration* RegisterModuleInfo(Versat* versat,ModuleInfo* info){
  Arena* arena = &versat->permanent;

  FUDeclaration decl = {};
  
  // Check same name
  for(FUDeclaration* decl : versat->declarations){
    if(CompareString(decl->name,info->name)){
      LogFatal(LogModule::TOP_SYS,"Found a module with a same name (%.*s). Cannot proceed",UNPACK_SS(info->name));
    }
  }

  Array<Wire> configs = PushArray<Wire>(arena,info->configs.size);
  Array<Wire> states = PushArray<Wire>(arena,info->states.size);
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(arena,info->externalInterfaces.size);
  int memoryMapBits = 0;
  //int databusAddrSize = 0;

  region(arena){
    Array<ParameterExpression> instantiated = PushArray<ParameterExpression>(arena,info->defaultParameters.size);

    for(int i = 0; i < instantiated.size; i++){
      ParameterExpression def = info->defaultParameters[i];

      // TODO: Make this more generic. Probably gonna need to take a look at a FUDeclaration as a instance of a more generic FUType construct. FUDeclaration has constant values, the FUType stores the expressions to compute them.
      // Override databus size for current architecture
      if(CompareString(def.name,STRING("AXI_ADDR_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(versat->opts.addrSize);

        def.expr = expr;
      }

      if(CompareString(def.name,STRING("AXI_DATA_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(versat->opts.dataSize);

        def.expr = expr;
      }

      // Override length. Use 20 for now
      if(CompareString(def.name,STRING("LEN_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(16);

        def.expr = expr;
      }

      instantiated[i] = def;
    }

    for(int i = 0; i < info->configs.size; i++){
      WireExpression& wire = info->configs[i];

      int size = EvalRange(wire.bitSize,instantiated);

      configs[i].name = info->configs[i].name;
      configs[i].bitSize = size;
      configs[i].isStatic = false;
    }

    for(int i = 0; i < info->states.size; i++){
      WireExpression& wire = info->states[i];

      int size = EvalRange(wire.bitSize,instantiated);

      states[i].name = info->states[i].name;
      states[i].bitSize = size;
      states[i].isStatic = false;
    }

    for(int i = 0; i < info->externalInterfaces.size; i++){
      ExternalMemoryInterfaceExpression& expr = info->externalInterfaces[i];

      external[i].interface = expr.interface;
      external[i].type = expr.type;

      switch(expr.type){
      case ExternalMemoryType::TWO_P:{
        external[i].tp.bitSizeIn = EvalRange(expr.tp.bitSizeIn,instantiated);
        external[i].tp.bitSizeOut = EvalRange(expr.tp.bitSizeOut,instantiated);
        external[i].tp.dataSizeIn = EvalRange(expr.tp.dataSizeIn,instantiated);
        external[i].tp.dataSizeOut = EvalRange(expr.tp.dataSizeOut,instantiated);
      }break;
      case ExternalMemoryType::DP:{
        for(int ii = 0; ii < 2; ii++){
          external[i].dp[ii].bitSize = EvalRange(expr.dp[ii].bitSize,instantiated);
          external[i].dp[ii].dataSizeIn = EvalRange(expr.dp[ii].dataSizeIn,instantiated);
          external[i].dp[ii].dataSizeOut = EvalRange(expr.dp[ii].dataSizeOut,instantiated);
        }
      }break;
      default: NOT_IMPLEMENTED;
      }
    }

    memoryMapBits = EvalRange(info->memoryMappedBits,instantiated);
    //databusAddrSize = EvalRange(info->databusAddrSize,instantiated);
  }

  decl.name = info->name;
  decl.inputDelays = info->inputDelays;
  decl.outputLatencies = info->outputLatencies;
  decl.configInfo.configs = configs;
  decl.configInfo.states = states;
  decl.externalMemory = external;
  decl.configInfo.delayOffsets.max = info->nDelays;
  decl.nIOs = info->nIO;
  decl.memoryMapBits = memoryMapBits;
  //decl.databusAddrSize = databusAddrSize; // TODO: How to handle different units with different databus address sizes??? For now do nothing. Do not even know if it's worth to bother with this, I think that all units should be force to use AXI_ADDR_W for databus addresses.
  decl.isMemoryMapped = info->memoryMapped;
  decl.implementsDone = info->hasDone;
  decl.signalLoop = info->signalLoop;

  if(info->isSource){
    decl.delayType = decl.delayType | DelayType::DELAY_TYPE_SINK_DELAY;
  }

  decl.configInfo.configOffsets.max = info->configs.size;
  decl.configInfo.stateOffsets.max = info->states.size;

  FUDeclaration* res = RegisterFU(versat,decl);
  return res;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
  if(!IsTypeHierarchical(&decl)){
    decl.configInfo.outputOffsets.max = decl.outputLatencies.size;
  }

  FUDeclaration* type = versat->declarations.Alloc();
  *type = decl;

  return type;
}

// Bunch of functions that could go to accelerator stats
// Returns the values when looking at a unit individually
UnitValues CalculateIndividualUnitValues(FUInstance* inst){
  UnitValues res = {};

  FUDeclaration* type = inst->declaration;

  res.outputs = type->outputLatencies.size; // Common all cases
  if(IsTypeHierarchical(type)){
    Assert(inst->declaration->fixedDelayCircuit);
	//} else if(type->type == FUDeclaration::ITERATIVE){
	//   Assert(inst->declaration->fixedDelayCircuit);
	//   res.delays = 1;
	//   res.extraData = sizeof(int);
  } else {
    res.configs = type->configInfo.configs.size;
    res.states = type->configInfo.states.size;
    res.delays = type->configInfo.delayOffsets.max;
    res.extraData = type->configInfo.extraDataOffsets.max;
  }

  return res;
}

// Returns the values when looking at subunits as well. For simple units, same as individual unit values
// For composite units, same as calculate accelerator values + calculate individual unit values
UnitValues CalculateAcceleratorUnitValues(Versat* versat,FUInstance* inst){
  UnitValues res = {};

  FUDeclaration* type = inst->declaration;

  if(IsTypeHierarchical(type)){
    res.totalOutputs = type->configInfo.outputOffsets.max;
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

  Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(&accel->versat->temp,1000);

  int memoryMappedDWords = 0;

  // Handle non-static information
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(inst->sharedIndex >= (int) seenShared.size()){
        seenShared.resize(inst->sharedIndex + 1);
      }

      if(!seenShared[inst->sharedIndex]){
        val.configs += type->configInfo.configs.size;
        val.sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         val.configs += type->configInfo.configs.size;
    }

    if(type->isMemoryMapped){
      val.isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits);
      memoryMappedDWords += 1 << type->memoryMapBits;
    }

    if(type == BasicDeclaration::input){
      val.inputs += 1;
    }

    val.states += type->configInfo.states.size;
    val.delays += type->configInfo.delayOffsets.max;
    val.ios += type->nIOs;
    val.extraData += type->configInfo.extraDataOffsets.max;

    if(type->externalMemory.size){
      val.externalMemoryInterfaces += type->externalMemory.size;
	  val.externalMemoryByteSize = ExternalMemoryByteSize(type->externalMemory);
    }

    val.signalLoop |= type->signalLoop;
  }

  val.memoryMappedBits = log2i(memoryMappedDWords);

  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    FOREACH_LIST(Edge*,edge,accel->edges){
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
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      val.statics += inst->declaration->configInfo.configs.size;
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          val.statics += pair.second.configs.size;
        }
      }
    }
  }

  return val;
}

void FillDeclarationWithAcceleratorValues(Versat* versat,FUDeclaration* decl,Accelerator* accel){
  Arena* permanent = &versat->permanent;
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  UnitValues val = CalculateAcceleratorValues(versat,accel);

  bool anySavedConfiguration = false;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    anySavedConfiguration |= inst->savedConfiguration;
  }

  decl->configInfo.delayOffsets.max = val.delays;
  decl->nIOs = val.ios;
  decl->configInfo.extraDataOffsets.max = val.extraData;
  decl->nStaticConfigs = val.statics;
  decl->isMemoryMapped = val.isMemoryMapped;
  decl->memoryMapBits = val.memoryMappedBits;
  decl->memAccessFunction = CompositeMemoryAccess;

  decl->inputDelays = PushArray<int>(permanent,val.inputs);

  decl->externalMemory = PushArray<ExternalMemoryInterface>(permanent,val.externalMemoryInterfaces);
  int externalIndex = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    Array<ExternalMemoryInterface> arr = ptr->inst->declaration->externalMemory;
    for(int i = 0; i < arr.size; i++){
      decl->externalMemory[externalIndex] = arr[i];
      decl->externalMemory[externalIndex].interface = externalIndex;
      externalIndex += 1;
    }
  }

  for(int i = 0; i < val.inputs; i++){
    FUInstance* input = GetInputInstance(accel->allocated,i);

    if(!input){
      continue;
    }

    decl->inputDelays[i] = input->baseDelay;
  }

  decl->outputLatencies = PushArray<int>(permanent,val.outputs);

  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    for(int i = 0; i < decl->outputLatencies.size; i++){
      decl->outputLatencies[i] = outputInstance->baseDelay;
    }
  }

  decl->configInfo.configs = PushArray<Wire>(permanent,val.configs);
  decl->configInfo.states = PushArray<Wire>(permanent,val.states);

  Hashmap<int,int>* staticsSeen = PushHashmap<int,int>(temp,val.sharedUnits);

  int configIndex = 0;
  int stateIndex = 0;

  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* d = inst->declaration;

    if(!inst->isStatic){
      if(inst->sharedEnable){
        if(staticsSeen->InsertIfNotExist(inst->sharedIndex,0)){
          for(Wire& wire : d->configInfo.configs){
            decl->configInfo.configs[configIndex].name = PushString(permanent,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
            decl->configInfo.configs[configIndex++].bitSize = wire.bitSize;
          }
        }
      } else {
        for(Wire& wire : d->configInfo.configs){
          decl->configInfo.configs[configIndex].name = PushString(permanent,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
          decl->configInfo.configs[configIndex++].bitSize = wire.bitSize;
        }
      }
    }

    for(Wire& wire : d->configInfo.states){
      decl->configInfo.states[stateIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
      decl->configInfo.states[stateIndex++].bitSize = wire.bitSize;
    }
  }

#if 0
  if(anySavedConfiguration){
    decl->defaultConfig = PushArray<iptr>(permanent,val.configs);
    decl->defaultStatic = PushArray<iptr>(permanent,val.statics);

    int configIndex = 0;
    //int staticIndex = 0;
    bool didOnce = false;
    FOREACH_LIST(ptr,accel->allocated){
    FUInstance* inst = ptr->inst;

    if(inst->savedConfiguration){
      if(inst->isStatic){
        Assert(!didOnce); // In order to properly do default config for statics, we need to first map their location and then copy
        didOnce = true;
        for(int i = 0; i < inst->declaration->configs.size; i++){
          decl->defaultStatic[configIndex++] = inst->config[i];
        }
      } else {
        for(int i = 0; i < inst->declaration->configs.size; i++){
          decl->defaultConfig[configIndex++] = inst->config[i];
        }
      }
    }
  }
  }
#endif

  decl->signalLoop = val.signalLoop;

  decl->configInfo.configOffsets = CalculateConfigurationOffset(accel,MemType::CONFIG,permanent);
  decl->configInfo.stateOffsets = CalculateConfigurationOffset(accel,MemType::STATE,permanent);
  decl->configInfo.delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,permanent);

  Assert(decl->configInfo.configOffsets.max == val.configs);
  Assert(decl->configInfo.stateOffsets.max == val.states);
  Assert(decl->configInfo.delayOffsets.max == val.delays);

  decl->configInfo.outputOffsets = CalculateOutputsOffset(accel,val.outputs,permanent);
  Assert(decl->configInfo.outputOffsets.max == val.totalOutputs + val.outputs);
  decl->configInfo.outputOffsets.max = val.totalOutputs + val.outputs; // Includes the output portion of the composite instance

  decl->configInfo.extraDataOffsets = CalculateConfigurationOffset(accel,MemType::EXTRA,permanent);
  Assert(decl->configInfo.extraDataOffsets.max == val.extraData);
}

FUDeclaration* RegisterSubUnit(Versat* versat,String name,Accelerator* circuit){
  FUDeclaration decl = {};

  Arena* permanent = &versat->permanent;
  Arena* temp = &versat->temp;
  ArenaMarker marker(temp);

#if 0
  if(IsCombinatorial(circuit)){
    circuit = Flatten(versat,circuit,99);
  }
#endif

  decl.type = FUDeclaration::COMPOSITE;
  decl.name = name;

  // HACK, for now
  circuit->subtype = &decl;

  OutputGraphDotFile(versat,circuit,false,"debug/%.*s/base.dot",UNPACK_SS(name));
  decl.baseCircuit = CopyAccelerator(versat,circuit,nullptr);
  OutputGraphDotFile(versat,decl.baseCircuit,false,"debug/%.*s/base2.dot",UNPACK_SS(name));

  ReorganizeAccelerator(circuit,temp);

  region(temp){
    auto delays = CalculateDelay(versat,circuit,temp);

    OutputGraphDotFile(versat,circuit,false,"debug/%.*s/beforeFixDelay.dot",UNPACK_SS(name));

    FixDelays(versat,circuit,delays);

    OutputGraphDotFile(versat,circuit,false,"debug/%.*s/fixDelay.dot",UNPACK_SS(name));
  }

  ReorganizeAccelerator(circuit,temp);
  decl.fixedDelayCircuit = circuit;

  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    if(ptr->multipleSamePortInputs){
      printf("Multiple inputs: %.*s\n",UNPACK_SS(name));
      break;
    }
  }

  // Need to calculate versat data here.
  FillDeclarationWithAcceleratorValues(versat,&decl,circuit);

  // TODO: Change unit delay type inference. Only care about delay type to upper levels.
  // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
  bool hasSourceDelay = false;
  bool hasSinkDelay = false;
  bool implementsDone = false;

  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration->type == FUDeclaration::SPECIAL){
      continue;
    }

    if(inst->declaration->implementsDone){
      implementsDone = true;
    }
    if(ptr->type == InstanceNode::TAG_SINK){
      hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
    }
    if(ptr->type == InstanceNode::TAG_SOURCE){
      hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
    }
    if(ptr->type == InstanceNode::TAG_SOURCE_AND_SINK){
      hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
      hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
    }
  }

  if(hasSourceDelay){
    decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SOURCE_DELAY);
  }
  if (hasSinkDelay){
    decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SINK_DELAY);
  }

  decl.implementsDone = implementsDone;

  decl.staticUnits = PushHashmap<StaticId,StaticData>(permanent,1000); // TODO: Set correct number of elements
  int staticOffset = 0;
  // Start by collecting all the existing static allocated units in subinstances
  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(IsTypeHierarchical(inst->declaration)){

      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = pair.second;
        newData.offset = staticOffset;

        if(decl.staticUnits->InsertIfNotExist(pair.first,newData)){
          staticOffset += newData.configs.size;
        }
      }
    }
  }

  FUDeclaration* res = RegisterFU(versat,decl);
  res->baseCircuit->subtype = res;
  res->fixedDelayCircuit->subtype = res;

  // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = res;

      StaticData data = {};
      data.configs = inst->declaration->configInfo.configs;
      data.offset = staticOffset;

      if(res->staticUnits->InsertIfNotExist(id,data)){
        staticOffset += inst->declaration->configInfo.configs.size;
      }
    }
  }

  // TODO: This should be outside.
#if 1
  if(versat->debug.outputAccelerator){
    char buffer[256];
    sprintf(buffer,"%.*s/modules/%.*s.v",UNPACK_SS(versat->outputLocation),UNPACK_SS(decl.name));
    FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
    OutputCircuitSource(versat,res,circuit,sourceCode);
    fclose(sourceCode);
  }
#endif

  return res;
}

#if 0
static int* IterativeInitializeFunction(FUInstance* inst){
  return nullptr;
}

static int* IterativeStartFunction(FUInstance* inst){
  int* delay = (int*) inst->extraData;

  *delay = 0;

  return nullptr;
}

static int* IterativeUpdateFunction(FUInstance* inst,Array<int> inputs){
  static int out[99];

#if 0
  FUDeclaration* type = inst->declaration;
  int* delay = (int*) inst->extraData;
  FUInstance* dataInst = (FUInstance*) GetInstanceByName(inst->iterative,"data");

  //LockAccelerator(inst->compositeAccel,Accelerator::Locked::ORDERED);
  //Assert(inst->iterative);

  if(*delay == -1){ // For loop part
      AcceleratorIterator iter = {};
      Arena* arena = &inst->accel->versat->temp;
      BLOCK_REGION(arena);
      iter.Start(inst->accel,arena,true);
      AcceleratorRunComposite(iter);
  } else if(*delay == inst->delay[0]){
    for(int i = 0; i < type->inputDelays.size; i++){
      int val = inputs[i]; //GetInputValue(inst,i);
      SetInputValue(inst->iterative,i,val);
    }

    AcceleratorRun(inst->iterative);

    for(int i = 0; i < type->outputLatencies.size; i++){
      int val = GetOutputValue(inst->iterative,i);
      out[i] = val;
    }

    FUInstance* secondData = (FUInstance*) GetInstanceByName(inst->iterative,"data");

    for(int i = 0; i < secondData->declaration->outputLatencies.size; i++){
      dataInst->outputs[i] = secondData->outputs[i];
      dataInst->storedOutputs[i] = secondData->outputs[i];
    }

    *delay = -1;
  } else {
    *delay += 1;
  }
#endif

  return out;
}

static int* IterativeDestroyFunction(FUInstance* inst){
  return nullptr;
}
#endif

struct Connection{
  InstanceNode* input;
  InstanceNode* mux;
  PortNode unit;
};

FUDeclaration* RegisterIterativeUnit(Versat* versat,Accelerator* accel,FUInstance* inst,int latency,String name){
  InstanceNode* node = GetInstanceNode(accel,(FUInstance*) inst);

  Assert(node);

  Arena* arena = &versat->temp;
  BLOCK_REGION(arena);

  Array<Array<PortNode>> inputs = GetAllInputs(node,arena);

  for(Array<PortNode>& arr : inputs){
    Assert(arr.size <= 2); // Do not know what to do if size bigger than 2.
  }

  static String MuxNames[] = {STRING("Mux1"),STRING("Mux2"),STRING("Mux3"),STRING("Mux4"),STRING("Mux5"),STRING("Mux6"),STRING("Mux7"),
                               STRING("Mux8"),STRING("Mux9"),STRING("Mux10"),STRING("Mux11"),STRING("Mux12"),STRING("Mux13"),STRING("Mux14"),STRING("Mux15"),
                               STRING("Mux16"),STRING("Mux17"),STRING("Mux18"),STRING("Mux19"),STRING("Mux20"),STRING("Mux21"),STRING("Mux22"),STRING("Mux23"),
                               STRING("Mux24"),STRING("Mux25"),STRING("Mux26"),STRING("Mux27"),STRING("Mux28"),STRING("Mux29"),STRING("Mux30"),STRING("Mux31"),STRING("Mux32"),STRING("Mux33"),STRING("Mux34"),
                               STRING("Mux35"),STRING("Mux36"),STRING("Mux37"),STRING("Mux38"),STRING("Mux39"),STRING("Mux40"),STRING("Mux41"),STRING("Mux42"),STRING("Mux43"),STRING("Mux44"),STRING("Mux45"),STRING("Mux46"),STRING("Mux47"),STRING("Mux48"),STRING("Mux49"),STRING("Mux50"),STRING("Mux51"),STRING("Mux52"),STRING("Mux53"),STRING("Mux54"),STRING("Mux55")};

  FUDeclaration* type = BasicDeclaration::timedMultiplexer;

  OutputGraphDotFile(versat,accel,false,"debug/iterativeBefore.dot");

  Array<Connection> conn = PushArray<Connection>(arena,inputs.size);
  Memset(conn,{});

  int muxInserted = 0;
  for(int i = 0; i < inputs.size; i++){
    Array<PortNode>& arr = inputs[i];
    if(arr.size != 2){
      continue;
    }

    PortNode first = arr[0];

    if(first.node->inst->declaration != BasicDeclaration::input){
      SWAP(arr[0],arr[1]);
    }

    Assert(arr[0].node->inst->declaration == BasicDeclaration::input);

    Assert(i < ARRAY_SIZE(MuxNames));
    FUInstance* mux = CreateFUInstance(accel,type,MuxNames[i]);
    InstanceNode* muxNode = GetInstanceNode(accel,(FUInstance*) mux);

    RemoveConnection(accel,arr[0].node,arr[0].port,node,i);
    RemoveConnection(accel,arr[1].node,arr[1].port,node,i);

    ConnectUnitsGetEdge(arr[0],(PortNode){muxNode,0},0);
    ConnectUnitsGetEdge(arr[1],(PortNode){muxNode,1},0);

    ConnectUnitsGetEdge((PortNode){muxNode,0},(PortNode){node,i},0);

    conn[i].input = arr[0].node;
    conn[i].unit = PortNode{arr[1].node,arr[1].port};
    conn[i].mux = muxNode;

    muxInserted += 1;
  }

  FUDeclaration* bufferType = BasicDeclaration::buffer;
  int buffersAdded = 0;
  for(int i = 0; i < inputs.size; i++){
    if(!conn[i].mux){
      continue;
    }

    InstanceNode* unit = conn[i].unit.node;
    Assert(i < unit->inst->declaration->inputDelays.size);

    if(unit->inst->declaration->inputDelays[i] == 0){
      continue;
    }

    FUInstance* buffer = (FUInstance*) CreateFUInstance(accel,bufferType,PushString(&versat->permanent,"Buffer%d",buffersAdded++));

    buffer->bufferAmount = unit->inst->declaration->inputDelays[i] - BasicDeclaration::buffer->outputLatencies[0];
    Assert(buffer->bufferAmount >= 0);
    SetStatic(accel,buffer);

    if(buffer->config){
      buffer->config[0] = buffer->bufferAmount;
    }

    InstanceNode* bufferNode = GetInstanceNode(accel,buffer);

    InsertUnit(accel,(PortNode){conn[i].mux,0},conn[i].unit,(PortNode){bufferNode,0});
  }

  OutputGraphDotFile(versat,accel,true,"debug/iterativeAfter.dot");

  FUDeclaration declaration = {};

  declaration = *inst->declaration; // By default, copy everything from unit
  declaration.name = PushString(&versat->permanent,name);
  declaration.type = FUDeclaration::ITERATIVE;

  FillDeclarationWithAcceleratorValues(versat,&declaration,accel);

  // Kinda of a hack, for now
#if 0
  int min = std::min(node->inst->declaration->inputDelays.size,declaration.inputDelays.size);
  for(int i = 0; i < min; i++){
    declaration.inputDelays[i] = node->inst->declaration->inputDelays[i];
  }
#endif

  // TODO: We are not checking connections here, we are just assuming that unit is directly connected to out.
  //       Probably a bad ideia but still do not have an example which would make it not ideal
  for(int i = 0; i < declaration.outputLatencies.size; i++){
    declaration.outputLatencies[i] = latency * (node->inst->declaration->outputLatencies[i] + 1);
  }

  // Values from iterative declaration
  ReorganizeIterative(accel,arena);
  //ReorganizeAccelerator(accel,arena);
  declaration.baseCircuit = accel;
  declaration.fixedDelayCircuit = accel;

#if 0
  Accelerator* f = CopyAccelerator(versat,accel,nullptr);
  ReorganizeIterative(f,arena);

  // TODO: HACK, for now
  FUDeclaration temp = {};
  temp.name = name;
  f->subtype = &temp;

  auto delays = CalculateDelay(versat,f,arena);
  FixDelays(versat,f,delays);

  OutputGraphDotFile(versat,f,true,"debug/iterative2.dot");

  f->ordered = nullptr;
  ReorganizeIterative(f,arena);

  declaration.fixedDelayCircuit = f;
#endif

#if 0
  declaration.initializeFunction = IterativeInitializeFunction;
  declaration.startFunction = IterativeStartFunction;
  declaration.updateFunction = IterativeUpdateFunction;
  declaration.destroyFunction = IterativeDestroyFunction;
#endif
  declaration.memAccessFunction = CompositeMemoryAccess;

  declaration.staticUnits = PushHashmap<StaticId,StaticData>(&versat->permanent,1000); // TODO: Set correct number of elements
  int staticOffset = 0;
  // Start by collecting all the existing static allocated units in subinstances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(IsTypeHierarchical(inst->declaration)){

      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = pair.second;
        newData.offset = staticOffset;

        if(declaration.staticUnits->InsertIfNotExist(pair.first,newData)){
          staticOffset += newData.configs.size;
        }
      }
    }
  }

  FUDeclaration* registeredType = RegisterFU(versat,declaration);

  // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = registeredType;

      StaticData data = {};
      data.configs = inst->declaration->configInfo.configs;
      data.offset = staticOffset;

      if(registeredType->staticUnits->InsertIfNotExist(id,data)){
        staticOffset += inst->declaration->configInfo.configs.size;
      }
    }
  }

  {
    VersatComputedValues val = ComputeVersatValues(versat,accel);
	ComputedData computedData = CalculateVersatComputedData(accel->allocated,val,arena);

	TemplateSetCustom("staticUnits",registeredType->staticUnits,"Hashmap<StaticId,StaticData>");
	TemplateSetCustom("accel",registeredType,"FUDeclaration");
	TemplateSetCustom("versatData",&computedData.data,"Array<VersatComputedData>");
	TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
	TemplateSetCustom("instances",accel->allocated,"InstanceNode");
	TemplateSetNumber("unitsMapped",val.unitsMapped);
	TemplateSetCustom("inputDecl",BasicDeclaration::input,"FUDeclaration");
	TemplateSetCustom("outputDecl",BasicDeclaration::output,"FUDeclaration");
  }

  char buffer[256];
  sprintf(buffer,"src/%.*s.v",UNPACK_SS(name));
  FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
  ProcessTemplate(sourceCode,BasicTemplates::iterativeTemplate,&versat->temp);

  return registeredType;
}

void ClearConfigurations(Accelerator* accel){
  for(int i = 0; i < accel->configAlloc.size; i++){
    accel->configAlloc.ptr[i] = 0;
  }
}

// TODO: Move to debug because it is debug code. Probably not needed to be gui because it's helpful to be able to copy everything in it as text
#if 0
void OutputMemoryMap(Versat* versat,Accelerator* accel){
  UNHANDLED_ERROR; // Might need to create a view so that ComputeVersatValues works
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
  printf("Number units: %d\n",Size(versat->accelerators.Get(0)->allocated));
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
#endif

//TODO: functions that should go to accelerator
int GetInputValue(InstanceNode* node,int index){
  if(!node->inputs.size){
    return 0;
  }

  PortNode* other = &node->inputs[index];

  if(!other->node){
    return 0;
  }

  return other->node->inst->outputs[other->port];
}

int GetInputValue(FUInstance* inst,int index){
  InstanceNode* node = GetInstanceNode(inst->accel,(FUInstance*) inst);
  if(!node){
    return 0;
  }

  PortNode* other = &node->inputs[index];

#if 01
  if(!other->node){
    return 0;
  }

  return other->node->inst->outputs[other->port];
#endif
}

int GetNumberOfInputs(FUInstance* inst){
  return inst->declaration->inputDelays.size;
}

int GetNumberOfOutputs(FUInstance* inst){
  return inst->declaration->outputLatencies.size;
}

FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == portNumber){
      return inst;
    }
  }

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::input,name);
  inst->portIndex = portNumber;

  return inst;
}

FUInstance* CreateOrGetOutput(Accelerator* accel){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::output){
      return inst;
    }
  }

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::output,STRING("out"));
  return inst;
}

int GetNumberOfInputs(Accelerator* accel){
  int count = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::input){
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

  FUInstance* inst = GetInputInstance(accel->allocated,portNumber);
  Assert(inst);

  inst->outputs[0] = number;
  inst->storedOutputs[0] = number;
}

int GetOutputValue(Accelerator* accel,int portNumber){
  FUInstance* inst = GetOutputInstance(accel->allocated);

  int value = GetInputValue(inst,portNumber);

  return value;
}

int GetInputPortNumber(FUInstance* inputInstance){
  Assert(inputInstance->declaration == BasicDeclaration::input);

  return ((FUInstance*) inputInstance)->portIndex;
}

void SetDelayRecursive_(AcceleratorIterator iter,int delay){
  FUInstance* inst = iter.Current()->inst;

  if(inst->declaration == BasicDeclaration::buffer){
    inst->config[0] = inst->bufferAmount;
    return;
  }

  int totalDelay = inst->baseDelay + delay;

  if(IsTypeHierarchical(inst->declaration)){
    AcceleratorIterator it = iter.LevelBelowIterator();

    for(InstanceNode* child = it.Current(); child; child = it.Skip()){
      SetDelayRecursive_(it,totalDelay);
    }
  } else if(inst->declaration->configInfo.delayOffsets.max){
    inst->delay[0] = totalDelay;
  }
}

int CountNonSpecialChilds(InstanceNode* nodes){
  int count = 0;
  FOREACH_LIST(InstanceNode*,ptr,nodes){
    FUInstance* inst = ptr->inst;
    if(inst->declaration->type == FUDeclaration::SPECIAL){
    } else if(IsTypeHierarchical(inst->declaration)){
      count += CountNonSpecialChilds(inst->declaration->baseCircuit->allocated);
    } else {
      count += 1;
    }
  }

  return count;
}

void SetDelayRecursive(Accelerator* accel,Arena* arena){
  BLOCK_REGION(arena);

  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Skip()){
    SetDelayRecursive_(iter,0);
  }
}

bool CheckValidName(String name){
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
#if 0
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
#endif

  return totalSize;
}


#include "debugGUI.hpp"

void VersatSimDebug(Versat* versat){
}

void Hook(Versat* versat,FUDeclaration* decl,Accelerator* accel,FUInstance* inst){

}
