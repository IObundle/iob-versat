#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "parser.hpp"

/*

What I should tackle first:

  Implement PrintAllTypesAndFieldsUsed(CompiledTemplate* compiled,Arena* temp);  
    The original 

  Only common.tpl including is working. For now there is no need for full includes
  Command could just be an expression.
  Maybe should check the seperation of expressions.
  Should also completely parse commands into an enum, instead of a simple string. Change if/else chains into switch. Speeds up evaluation.
  #{} should never print. @{} should always print. The logic should be unified and enforced, right now #{} gets parsed as commands and @{} gets parsed as expressions but realistically we could figure out if its a command or an expression by finding if the first id is a command id or not (but if so we cannot have identifiers with the same name as a command).

 */

struct Expression;

// TODO: A Command could just be an expression, I think
struct Command{
  String name;
  Array<Expression*> expressions;
  String fullText;
};

enum BlockType {
  BlockType_TEXT,
  BlockType_COMMAND,
  BlockType_EXPRESSION
};

struct Block{
  Token fullText;

  union{
    String textBlock;
    Command* command;
    Expression* expression;
  };
  Array<Block*> innerBlocks;
  BlockType type;
  int line;
};

struct TemplateFunction{
  Array<Expression*> arguments;
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
