#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "parser.hpp"

struct Expression;

enum CommandType{
  CommandType_JOIN = 0,
  CommandType_FOR,
  CommandType_IF,
  CommandType_END,
  CommandType_SET,
  CommandType_LET,
  CommandType_INC,
  CommandType_ELSE,
  CommandType_DEBUG,
  CommandType_INCLUDE,
  CommandType_DEFINE,
  CommandType_CALL,
  CommandType_WHILE,
  CommandType_RETURN,
  CommandType_FORMAT,
  CommandType_DEBUG_MESSAGE,
  CommandType_DEBUG_BREAK
};

struct CommandDefinition{
  String name;
  int numberExpressions;
  CommandType type;
  bool isBlockType;
};

// TODO: A Command could just be an expression, I think
struct Command{
  CommandDefinition* definition;
  //String name;
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

enum TemplateRecordType{
  TemplateRecordType_FIELD,
  TemplateRecordType_IDENTIFER
};

struct TemplateRecord{
  TemplateRecordType type;

  union{
    struct {
      Type* identifierType;
      String identifierName;
    };
    struct {
      Type* structType;
      String fieldName;
    };
  };
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

String Repr(TemplateRecord record,Arena* arena);
void PrintTemplate(CompiledTemplate* compiled,Arena* arena);
Array<TemplateRecord> RecordTypesAndFieldsUsed(CompiledTemplate* compiled,Arena* out,Arena* temp); // Quick way of checking useless values 

Hashmap<String,Value>* GetAllTemplateValues();
void ClearTemplateEngine();

void TemplateSetCustom(const char* id,Value val);
//void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSet(const char* id,void* ptr);
void TemplateSetString(const char* id,const char* str);
void TemplateSetString(const char* id,String str);
void TemplateSetArray(const char* id,const char* baseType,int size,void* array);
void TemplateSetBool(const char* id,bool boolean);

#endif // TEMPLATE_ENGINE_INCLUDED
