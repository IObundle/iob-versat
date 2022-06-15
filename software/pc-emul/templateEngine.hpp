#ifndef TEMPLATE_ENGINE_INCLUDED
#define TEMPLATE_ENGINE_INCLUDED

#include "versat.hpp"
#include "parser.hpp"

void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* tempArena);

// TODO: Probably should all have the same name and overload
void TemplateSetCustom(const char* id,void* entity,const char* typeName);
void TemplateSetNumber(const char* id,int number);
void TemplateSetString(const char* id,const char* str);
void TemplateSetArray(const char* id,const char* baseType,void* array,int size);
void TemplateSetBool(const char* id,bool boolean);
void TemplateSetInstancePool(const char* id,Pool<FUInstance>* pool); // Not very good, but no other choice
void TemplateSetInstanceVector(const char* id,std::vector<FUInstance*>* vec);

#endif // TEMPLATE_ENGINE_INCLUDED
