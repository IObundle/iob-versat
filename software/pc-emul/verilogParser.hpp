#ifndef INCLUDED_VERILOG_PARSER
#define INCLUDED_VERILOG_PARSER

#include <vector>
#include <unordered_map>

#include "utils.hpp"
#include "parser.hpp"

typedef std::unordered_map<SizedString,Value> ValueMap;
typedef std::unordered_map<SizedString,SizedString> MacroMap;

struct Arena;

struct PortDeclaration{
   ValueMap attributes;
   Range range;
   SizedString name;
   enum {INPUT,OUTPUT,INOUT} type;
};

struct Module{
   ValueMap parameters;
   std::vector<PortDeclaration> ports;
   SizedString name;
   bool isSource;
};

struct ModuleInfo{
   SizedString name;
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

SizedString PreprocessVerilogFile(Arena* output, SizedString fileContent,std::vector<const char*>* includeFilepaths,Arena* tempArena);
std::vector<Module> ParseVerilogFile(SizedString fileContent, std::vector<const char*>* includeFilepaths, Arena* tempArena);

#endif // INCLUDED_VERILOG_PARSER
