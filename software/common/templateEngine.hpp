#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "parser.hpp"

/*

What I should tackle first:

  Implement PrintAllTypesAndFieldsUsed(CompiledTemplate* compiled,Arena* temp);  

  Includes should be handled better, instead of being parsed and evaluated inside, they should be parsed once and kept around.
  Command could just be an expression.
  Maybe should check the seperation of expressions.
  Should also completely implement the parsing, instead of parsing some at eval. 
  Should also completely parse commands into an enum, instead of a simple string. Change if/else chains into switch

Rework ParseAndEvaluate. Parsing should be kept distinct from evaluation.

 */

struct Expression;

// TODO: A Command could just be an expression.
struct Command{
  String name;

  Array<Expression*> expressions;
};

struct Block{
  Token fullText;

  union{
    Token textBlock;
    Command* command;
  };
  Array<Block*> innerBlocks;
  enum {TEXT,COMMAND} type;
};

struct TemplateFunction{
  Expression** arguments;
  int numberArguments;
  Array<Block*> blocks;
};

struct CompiledTemplate{
  int totalMemoryUsed;
  String content;
  Array<Block*> blocks;
  String name;

  // Followed by content and the block/expression structure
};

struct Value;
typedef Value (*PipeFunction)(Value in,Arena* temp);
void RegisterPipeOperation(String name,PipeFunction func);

void InitializeTemplateEngine(Arena* perm);

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate,Arena* arena);
CompiledTemplate* CompileTemplate(String content,const char* name,Arena* out,Arena* temp);

void PrintTemplate(CompiledTemplate* compiled,Arena* arena);
void PrintAllTypesAndFieldsUsed(CompiledTemplate* compiled,Arena* temp); // Quick way of checking useless values 

void ClearTemplateEngine();

void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSet(const char* id,void* ptr);
void TemplateSetString(const char* id,const char* str);
void TemplateSetString(const char* id,String str);
void TemplateSetArray(const char* id,const char* baseType,void* array,int size);
void TemplateSetBool(const char* id,bool boolean);

#endif // TEMPLATE_ENGINE_INCLUDED
