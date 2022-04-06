#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "parser.h"

typedef struct Value_t{
   union{
      int number;
      char* str;
      struct {
         int* array;
         int size;
      };
      void* ptr;
   };

   int type;
} Value;

#define CUSTOM_TYPE_STRUCT 0
#define CUSTOM_TYPE_PTR    1

#define MAX_EXPRESSION_SIZE 8

typedef struct Expression_t{
   union{
      struct{
         char* id;
         struct Expression_t* expressions[MAX_EXPRESSION_SIZE];
         int size;
      };
      Value val;
   };

   int type;
} Expression;

#define TYPE_EXPRESSION 0
#define TYPE_IDENTIFIER 1
#define TYPE_VAL        2
#define TYPE_ARRAY      3

#define VAL_NUMBER 0
#define VAL_STRING 1
#define VAL_ARRAY  2
#define VAL_NULL   3

#define VAL_CUSTOM 256

typedef struct SymbolEntry_t{
   Value val;
   char* symbol;
} SymbolEntry;

void ProcessTemplate(char* templateFilepath,char* outputFilepath);

void TemplateSetNumber(char* id,int number);
void TemplateSetString(char* id,char* str);
void TemplateSetArray(char* id,int* array,int size);

#endif // TEMPLATE_ENGINE_INCLUDED
