#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "versatPrivate.hpp"
#include "parser.hpp"

struct Expression;

struct Command{
   SizedString name;

   Expression** expressions;
   int nExpressions;
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

void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* tempArena);

// TODO: Probably should all have the same name and overload
void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSetString(const char* id,const char* str);
void TemplateSetArray(const char* id,const char* baseType,void* array,int size);
void TemplateSetBool(const char* id,bool boolean);

#endif // TEMPLATE_ENGINE_INCLUDED
