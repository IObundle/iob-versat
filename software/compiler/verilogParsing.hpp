#pragma once

#include "embeddedData.hpp"

#include "utils.hpp"
#include "parser.hpp"
#include "VerilogEmitter.hpp"

struct Arena;
struct SymbolicExpression;

typedef Range<Expression*> ExpressionRange;

struct MacroDefinition{
  String content;
  Array<String> functionArguments;
};

enum VersatStage{
  VersatStage_COMPUTE, // The default is Compute
  VersatStage_READ,
  VersatStage_WRITE
};

struct PortDeclaration{
  Hashmap<String,Value>* attributes;
  ExpressionRange range;
  String name;
  WireDir type;
};

struct ParameterExpression{
  String name;
  Expression* expr;
  ParamFlags flags;
};

struct Module{
  Array<ParameterExpression> parameters;
  Array<PortDeclaration> ports;
  String name;
  bool isSource;
};

struct Wire{
  String name;
  int bitSize; // TODO: This needs to be removed. We need to base all the logic on sizeExpr in order to properly handle parameters. Furthermore, if we eventually need to collapse parameters into concrete values, SymbolicExpressions can also do that. 
  VersatStage stage;

  SymbolicExpression* sizeExpr;
};

inline u64 Hash(Wire w){
  u64 res = Hash(w.name);
  return res;
}
inline bool operator==(Wire lhs,Wire rhs){
  bool res = (lhs.name == rhs.name);
  return res;
}

struct WireExpression{
  String name;
  ExpressionRange bitSize;
  VersatStage stage;
  bool isStatic;
};

enum ExternalMemoryType {
  ExternalMemoryType_2P,  // Two ports: one input and one output each with individual address (think FIFO)
  ExternalMemoryType_DP   // Dual port: two input or output ports (think LUT)
}; 

// TODO: Because we changed memories to be byte space instead of symbol space, maybe it would be best to change how the address bit size is stored. These structures are supposed to be clean, and so the parser should identify any differences in address size and report an error. These structures should only have one address if we keep going with the byte space memories idea and the data size is used to calculate the bitSize for each respective port
// NOTE: Do error checking after parsing everything. Parse first, check errors later.
template<typename T>
struct ExternalMemoryTwoPortsTemplate{ // tp
  T bitSizeIn;
  T bitSizeOut;
  T dataSizeIn;
  T dataSizeOut;
};

typedef ExternalMemoryTwoPortsTemplate<ExpressionRange> ExternalMemoryTwoPortsExpression;

template<typename T>
struct ExternalMemoryDualPortTemplate{ // dp
  T bitSize;
  T dataSizeIn;
  T dataSizeOut;
};

typedef ExternalMemoryDualPortTemplate<int> ExternalMemoryDualPort;
typedef ExternalMemoryDualPortTemplate<ExpressionRange> ExternalMemoryDualPortExpression;

template<typename T>
struct ExternalMemoryTemplate{
  union{
	ExternalMemoryTwoPortsTemplate<T> tp;
	ExternalMemoryDualPortTemplate<T> dp[2];
  };
};

typedef ExternalMemoryTemplate<int> ExternalMemory;
typedef ExternalMemoryTemplate<ExpressionRange> ExternalMemoryExpression;

// TODO: Do not know if it was better if this was a union. There are some differences that we currently ignore because we can always fill the interface with the information that we need. It's just that the code must take those into account, while the union approach would "simplify" somewhat the type system.
template<typename T>
struct ExternalMemoryInterfaceTemplate : public ExternalMemoryTemplate<T>{
  // union{
    // tp;
    // dp[2];
  // };
  ExternalMemoryType type;
  int interface;
};

typedef ExternalMemoryInterfaceTemplate<int> ExternalMemoryInterface;
typedef ExternalMemoryInterfaceTemplate<ExpressionRange> ExternalMemoryInterfaceExpression;
typedef ExternalMemoryInterfaceTemplate<SymbolicExpression*> ExternalMemorySymbolic;

struct ExternalMemoryID{
  int interface;
  ExternalMemoryType type;
};

inline u64 Hash(ExternalMemoryID id){
  u64 res = id.interface * 2 + id.type;
  return res;
}

inline bool operator==(ExternalMemoryID lhs,ExternalMemoryID rhs){
  bool res = (memcmp(&lhs,&rhs,sizeof(ExternalMemoryID)) == 0);
  return res;
}

struct ExternalInfoTwoPorts : public ExternalMemoryTwoPortsExpression{
  bool write;
  bool read;
};

struct ExternalInfoDualPort : public ExternalMemoryDualPortExpression{
  bool enable;
  bool write;
};

struct ExternalMemoryInfo{
  union{
	ExternalInfoTwoPorts tp;
	ExternalInfoDualPort dp[2];
  };
  ExternalMemoryType type;
};

enum SingleInterfaces{
  SingleInterfaces_DONE        = (1<<1),
  SingleInterfaces_CLK         = (1<<2),
  SingleInterfaces_RESET       = (1<<3),
  SingleInterfaces_RUN         = (1<<4),
  SingleInterfaces_RUNNING     = (1<<5),
  SingleInterfaces_SIGNAL_LOOP = (1<<6)
};

static inline SingleInterfaces& operator|=(SingleInterfaces& left,SingleInterfaces in){
  left = (SingleInterfaces) (((int) left) | ((int) in));
  return left;
}

struct PortInfo{
  int delay;
  ExpressionRange range;
};

enum ModuleSource{
  ModuleSource_DEFAULT_UNIT,
  ModuleSource_USER_UNIT
};

struct ModuleInfo{
  String name;
  Array<ParameterExpression> defaultParameters;

  Array<PortInfo> inputs;
  Array<PortInfo> outputs;
  
  Array<WireExpression> configs;
  Array<WireExpression> states;
  Array<ExternalMemoryInterfaceExpression> externalInterfaces;
  int nDelays;
  int nIO;
  ExpressionRange memoryMappedBits;
  ExpressionRange databusAddrSize;
  SingleInterfaces singleInterfaces;
  bool doesIO;
  bool memoryMapped;
  bool isSource;
  ModuleSource moduleSource;
};

SymbolicExpression* SymbolicExpressionFromVerilog(Expression* topExpr,Arena* out);
SymbolicExpression* SymbolicExpressionFromVerilog(ExpressionRange range,Arena* out);

String PreprocessVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out);
Array<Module> ParseVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out); // Only handles preprocessed files
ModuleInfo ExtractModuleInfo(Module& module,Arena* out);

Value Eval(Expression* expr,Array<ParameterExpression> parameters);
