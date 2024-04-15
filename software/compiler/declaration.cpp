#include "declaration.hpp"

#include "versat.hpp"

static Pool<FUDeclaration> declarations;

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

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

static int ones[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static FUDeclaration* RegisterCircuitInput(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("CircuitInput");
  decl.inputDelays = Array<int>{zeros,0};
  decl.outputLatencies = Array<int>{zeros,1};
  decl.delayType = DelayType::DelayType_SOURCE_DELAY;
  decl.type = FUDeclaration::SPECIAL;

  return RegisterFU(versat,decl);
}
static FUDeclaration* RegisterCircuitOutput(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("CircuitOutput");
  decl.inputDelays = Array<int>{zeros,50};
  decl.outputLatencies = Array<int>{zeros,0};
  decl.delayType = DelayType::DelayType_SINK_DELAY;
  decl.type = FUDeclaration::SPECIAL;

  return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterData(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("Data");
  decl.inputDelays = Array<int>{zeros,50};
  decl.outputLatencies = Array<int>{ones,50};
  decl.delayType = DelayType::DelayType_SINK_DELAY;

  return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterLiteral(Versat* versat){
  FUDeclaration decl = {};

  decl.name = STRING("Literal");
  decl.outputLatencies = Array<int>{zeros,1};

  return RegisterFU(versat,decl);
}
static void RegisterOperators(Versat* versat){
  struct Operation{
    const char* name;
    const char* operation;
  };

  Operation unary[] =  {{"NOT","{0}_{1} = ~{2}"},
                        {"NEG","{0}_{1} = -{2}"}};
  Operation binary[] = {{"XOR","{0}_{1} = {2} ^ {3}"},
                         {"ADD","{0}_{1} = {2} + {3}"},
                         {"SUB","{0}_{1} = {2} - {3}"},
                         {"AND","{0}_{1} = {2} & {3}"},
                         {"OR"  ,"{0}_{1} = {2} | {3}"},
                         {"RHR","{0}_{1} = ({2} >> {3}) | ({2} << (32 - {3}))"}, // TODO: Only for 32 bits
                         {"SHR","{0}_{1} = {2} >> {3}"},
                         {"RHL","{0}_{1} = ({2} << {3}) | ({2} >> (32 - {3}))"}, // TODO: Only for 32 bits
                         {"SHL","{0}_{1} = {2} << {3}"}};

  FUDeclaration decl = {};
  decl.inputDelays = Array<int>{zeros,1};
  decl.outputLatencies = Array<int>{zeros,1};
  decl.isOperation = true;

  for(unsigned int i = 0; i < ARRAY_SIZE(unary); i++){
    decl.name = STRING(unary[i].name);
    decl.operation = unary[i].operation;
    RegisterFU(versat,decl);
  }

  decl.inputDelays = Array<int>{zeros,2};
  for(unsigned int i = 0; i < ARRAY_SIZE(binary); i++){
    decl.name = STRING(binary[i].name);
    decl.operation = binary[i].operation;
    RegisterFU(versat,decl);
  }
}

FUDeclaration* GetTypeByName(Versat* versat,String name){
  for(FUDeclaration* decl : versat->declarations){
    if(CompareString(decl->name,name)){
      return decl;
    }
  }
  
  LogFatal(LogModule::TOP_SYS,"Didn't find the following type: %.*s",UNPACK_SS(name));

  return nullptr;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
  FUDeclaration* type = versat->declarations.Alloc();
  *type = decl;

  return type;
}

void InitializeSimpleDeclarations(Versat* versat){
  RegisterOperators(versat);
  RegisterCircuitInput(versat);
  RegisterCircuitOutput(versat);
  RegisterData(versat);
  RegisterLiteral(versat);
}


