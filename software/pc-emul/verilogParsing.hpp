#ifndef INCLUDED_VERILOG_PARSER
#define INCLUDED_VERILOG_PARSER

#include <vector>
#include <unordered_map>

#include "versatPrivate.hpp"
#include "utils.hpp"
#include "parser.hpp"

typedef std::unordered_map<String,Value> ValueMap;
typedef std::unordered_map<String,String> MacroMap;

struct Arena;

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

struct Module{
   ValueMap parameters;
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

struct ModuleInfo{
   String name;
   Array<int> inputDelays;
   Array<int> outputLatencies;
   Array<Wire> configs;
   Array<Wire> states;
   Array<ExternalMemoryInterface> externalInterfaces;
   int nDelays;
   int nIO;
   int memoryMappedBits;
   bool doesIO;
   bool memoryMapped;
   bool hasDone;
   bool hasClk;
   bool hasReset;
   bool hasRun;
   bool hasRunning;
   bool isSource;
};

String PreprocessVerilogFile(Arena* output, String fileContent,std::vector<const char*>* includeFilepaths,Arena* tempArena);
std::vector<Module> ParseVerilogFile(String fileContent, std::vector<const char*>* includeFilepaths, Arena* tempArena); // Only handles preprocessed files
ModuleInfo ExtractModuleInfo(Module& module,Arena* permanent,Arena* tempArena);
void OutputModuleInfos(FILE* output,bool doExport,Array<ModuleInfo> infos,String nameSpace,CompiledTemplate* unitVerilogData,Arena* temp);

#endif // INCLUDED_VERILOG_PARSER
