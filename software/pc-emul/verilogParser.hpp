#ifndef INCLUDED_VERILOG_PARSER
#define INCLUDED_VERILOG_PARSER

#include <vector>
#include <unordered_map>

#include "utils.hpp"
#include "parser.hpp"

typedef std::unordered_map<String,Value> ValueMap;
typedef std::unordered_map<String,String> MacroMap;

struct Arena;

struct PortDeclaration{
   ValueMap attributes;
   Range range;
   String name;
   enum {INPUT,OUTPUT,INOUT} type;
};

struct Module{
   ValueMap parameters;
   std::vector<PortDeclaration> ports;
   String name;
   bool isSource;
};

struct ModuleInfo{
   String name;
   int nInputs;
   int nOutputs;
   int* inputDelays;
   int* outputLatencies;
   int nConfigs;
   Wire* configs;
   int nStates;
   Wire* states;
   int nDelays;
   bool doesIO;
   bool memoryMapped;
   int memoryMappedBits;
   bool hasDone;
   bool hasClk;
   bool hasReset;
   bool hasRun;
   bool hasRunning;
   bool isSource;
};

String PreprocessVerilogFile(Arena* output, String fileContent,std::vector<const char*>* includeFilepaths,Arena* tempArena);
std::vector<Module> ParseVerilogFile(String fileContent, std::vector<const char*>* includeFilepaths, Arena* tempArena);

#endif // INCLUDED_VERILOG_PARSER
