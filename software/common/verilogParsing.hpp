#pragma once

#include <vector>
#include <unordered_map>

#include "utils.hpp"
#include "parser.hpp"

typedef std::unordered_map<String,Value> ValueMap;
typedef std::unordered_map<String,String> MacroMap;

struct Arena;
struct CompiledTemplate;

typedef Range<Expression*> ExpressionRange;

enum VersatStage{
  VersatStage_COMPUTE, // The default is Compute
  VersatStage_READ,
  VersatStage_WRITE
};

struct PortDeclaration{
  Hashmap<String,Value>* attributes;
  ExpressionRange range;
  String name;
  enum {INPUT,OUTPUT,INOUT} type;
};

struct ParameterExpression{
  String name;
  Expression* expr;
};

struct Module{
  Array<ParameterExpression> parameters;
  Array<PortDeclaration> ports;
  String name;
  bool isSource;
};

// TODO: This is a copy of Wire from configurations.hpp, but we do not include because this file should not include from the compiler code.
template<typename T>
struct WireTemplate{
  String name;
  T bitSize;
  VersatStage stage;
  bool isStatic; // This is only used by the verilog parser (?) to store info. TODO: Use a different structure in the verilog parser which contains this and remove from Wire   
};

typedef WireTemplate<int> Wire;
typedef WireTemplate<ExpressionRange> WireExpression;

enum ExternalMemoryType{TWO_P = 0,DP}; // Two ports: one input and one output (think FIFO). Dual port: two input or output ports (think LUT)

// TODO: Because we changed memories to be byte space instead of symbol space, maybe it would be best to change how the address bit size is stored. These structures are supposed to be clean, and so the parser should identify any differences in address size and report an error. These structures should only have one address if we keep going with the byte space memories idea and the data size is used to calculate the bitSize for each respective port
template<typename T>
struct ExternalMemoryTwoPortsTemplate{ // tp
  T bitSizeIn;
  T bitSizeOut;
  T dataSizeIn;
  T dataSizeOut;
};

typedef ExternalMemoryTwoPortsTemplate<int> ExternalMemoryTwoPorts;
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

struct ExternalMemoryID{
  int interface;
  ExternalMemoryType type;
};

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

template<> struct std::hash<ExternalMemoryID>{
  std::size_t operator()(ExternalMemoryID const& s) const noexcept{
    std::size_t hash = s.interface * 2 + (int) s.type;
    return hash;
  }
};

inline bool operator==(const ExternalMemoryID& lhs,const ExternalMemoryID& rhs){
  bool res = (memcmp(&lhs,&rhs,sizeof(ExternalMemoryID)) == 0);
  return res;
}

struct ModuleInfo{
  String name;
  Array<ParameterExpression> defaultParameters;
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<WireExpression> configs;
  Array<WireExpression> states;
  Array<ExternalMemoryInterfaceExpression> externalInterfaces;
  int nDelays;
  int nIO;
  ExpressionRange memoryMappedBits;
  ExpressionRange databusAddrSize;
  bool doesIO;
  bool memoryMapped;
  bool hasDone;
  bool hasClk;
  bool hasReset;
  bool hasRun;
  bool hasRunning;
  bool isSource;
  bool signalLoop;
};

String PreprocessVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out,Arena* temp);
Array<Module> ParseVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out,Arena* temp); // Only handles preprocessed files
ModuleInfo ExtractModuleInfo(Module& module,Arena* permanent,Arena* tempArena);

Value Eval(Expression* expr,Array<ParameterExpression> parameters);
