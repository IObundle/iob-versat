#pragma once

#include "parser.hpp"

void InitializeTemplateEngine(Arena* perm);

void ProcessTemplateSimple(FILE* outputFile,String tmpl);
void TemplateSimpleSubstitute(StringBuilder* b,String tmpl,Hashmap<String,String>* subs);
String TemplateSubstitute(String tmpl,String* valuesToReplace,Arena* out);

void ClearTemplateEngine();

Value MakeValue();
Value MakeValue(int number);
Value MakeValue(i64 number);
Value MakeValue(String str);
Value MakeValue(bool b);

void TemplateSetNumber(const char* id,int number);
void TemplateSetString(const char* id,const char* str);
void TemplateSetString(const char* id,String str);
void TemplateSetHex(const char* id,int number);
void TemplateSetBool(const char* id,bool boolean);
