#ifndef INCLUDED_VERILOG_PARSER
#define INCLUDED_VERILOG_PARSER

#include <vector>
#include <unordered_map>

//#include "versatPrivate.hpp"
#include "utils.hpp"
#include "parser.hpp"
//#include "configurations.hpp"

typedef std::unordered_map<String,Value> ValueMap;
typedef std::unordered_map<String,String> MacroMap;

struct Arena;
struct CompiledTemplate;

struct RangeAndExpr{
   Range range;
   Expression* top;
   Expression* bottom;
};

struct PortDeclaration{
   ValueMap attributes;
   Range range; // Cannot be a range, otherwise we cannot deal with parameters
   Expression* top;
   Expression* bottom;
   String name;
   enum {INPUT,OUTPUT,INOUT} type;
};

struct ParameterExpression{
   String name;
   Expression* expr;
};

struct Module{
   Array<ParameterExpression> parameters;
   //ValueMap parameters;
   std::vector<PortDeclaration> ports;
   String name;
   bool isSource;
};

struct ExternalMemoryDPDef{
   PortDeclaration* addr[2];
   PortDeclaration* out[2];
   PortDeclaration* in[2];
   PortDeclaration* write[2];
   PortDeclaration* enable[2];
};

// TODO: This is a copy of Wire from configurations.hpp, but we do not include because this file should not include from the compiler code.
struct Wire{
   String name;
   int bitsize;
   bool isStatic; // This is only used by the verilog parser (?) to store info. TODO: Use a different structure in the verilog parser which contains this and remove from Wire   
};

struct WireExpression{
   String name;
   Expression* top;
   Expression* bottom;
   //int bitsize;
   bool isStatic; // This is only used by the verilog parser (?) to store info. TODO: Use a different structure in the verilog parser which contains this and remove from Wire
};

enum ExternalMemoryType{TWO_P = 0,DP};

struct ExternalMemoryInterfaceExpression{
   int interface;
   Expression* bitSize;
   Expression* dataSize;
   //int bitsize;
   //int datasize;
   ExternalMemoryType type;
};

struct ExternalMemoryInterface{
   int interface;
   int bitsize;
   int datasizeIn[2];
   int datasizeOut[2];
   ExternalMemoryType type;
};

struct ExternalMemoryID{
   int interface;
   ExternalMemoryType type;
};

template<> class std::hash<ExternalMemoryID>{
public:
   std::size_t operator()(ExternalMemoryID const& s) const noexcept{
      std::size_t hash = s.interface * 2 + (int) s.type;
      return hash;
   }
};

struct ExternalPortInfo{
   int addrSize;
   int dataInSize;
   int dataOutSize;
   bool enable;
   bool write;
};

// Contain info parsed directly by verilog.
// This probably should be a union of all the memory types
// The code in the verilog parser almost demands it
struct ExternalMemoryInfo{
   int numberPorts;

   // Maximum of 2 ports
   ExternalPortInfo ports[2];
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
   Array<ExternalMemoryInterface> externalInterfaces; // TODO: Implement expressions later
   int nDelays;
   int nIO;
   int memoryMappedBits;
   Expression* databusAddrBottom;
   Expression* databusAddrTop;
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

struct ModuleInfoInstance{
   String name;
   Array<ParameterExpression> parameters;
   Array<int> inputDelays;
   Array<int> outputLatencies;
   Array<Wire> configs;
   Array<Wire> states;
   Array<ExternalMemoryInterface> externalInterfaces;
   int nDelays;
   int nIO;
   int memoryMappedBits;
   int databusAddrSize;
   //Expression* databusAddrBottom;
   //Expression* databusAddrTop;
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

String PreprocessVerilogFile(Arena* output, String fileContent,std::vector<String>* includeFilepaths,Arena* tempArena);
std::vector<Module> ParseVerilogFile(String fileContent, std::vector<String>* includeFilepaths, Arena* tempArena); // Only handles preprocessed files
ModuleInfo ExtractModuleInfo(Module& module,Arena* permanent,Arena* tempArena);
void OutputModuleInfos(FILE* output,ModuleInfoInstance info,String nameSpace,CompiledTemplate* unitVerilogData,Arena* temp,Array<Wire> configsHeaderSide,Array<String> statesHeaderSide); // TODO: This portion should be remade, parameters are not the ones we want (VerilogParsing do not need to know about Accelerator)

Array<String> GetAllIdentifiers(Expression* expr,Arena* arena);
Value Eval(Expression* expr,Array<ParameterExpression> parameters);

#endif // INCLUDED_VERILOG_PARSER

