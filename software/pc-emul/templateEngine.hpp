#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "versatPrivate.hpp"
#include "parser.hpp"

struct Expression;

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
   Block* next;
   Block* nextInner;
   enum {TEXT,COMMAND} type;
};

struct TemplateFunction{
   Expression** arguments;
   int numberArguments;
   Block* block;
};

struct CompiledTemplate{
   int totalMemoryUsed;
   String content;
   Block* blocks;
   //const char* filepath;

   // Followed by content and then the block/expression structure
};

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate,Arena* arena);
CompiledTemplate* CompileTemplate(String content,Arena* arena);

// After embedding templates inside source code, these functions might just need to be removed
#if 0
void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* arena);
CompiledTemplate* CompileTemplate(const char* templateFilepath,Arena* arena);
#endif

void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSet(const char* id,void* ptr);
void TemplateSetString(const char* id,const char* str);
void TemplateSetArray(const char* id,const char* baseType,void* array,int size);
void TemplateSetBool(const char* id,bool boolean);

#endif // TEMPLATE_ENGINE_INCLUDED
