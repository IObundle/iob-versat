#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "versat.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "type.hpp"

#define MAX_EXPRESSION_SIZE 8

struct Expression{
   char op;
   SizedString id;
   Expression* expressions[MAX_EXPRESSION_SIZE];
   int size;
   Value val;

   enum {OPERATION,IDENTIFIER,LITERAL,ARRAY_ACCESS,MEMBER_ACCESS,POINTER_ACCESS} type;
};

void ProcessTemplate(FILE* outputFile,const char* templateFilepath);

void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSetString(const char* id,const char* str);
void TemplateSetArray(const char* id,int* array,int size);
void TemplateSetInstancePool(const char* id,Pool<FUInstance>* pool); // Not very good, but no other choice
void TemplateSetInstanceVector(const char* id,std::vector<FUInstance*>* vec);

#endif // TEMPLATE_ENGINE_INCLUDED
