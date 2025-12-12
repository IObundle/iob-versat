#pragma once

#include "parser.hpp"

Value MakeValue();
Value MakeValue(int number);
Value MakeValue(i64 number);
Value MakeValue(String str);
Value MakeValue(bool b);

// TODO: Add TemplateSetCAST, TemplateSetStringBuilder and stuff like that.
//       We can even provide faster implementations if we just pass in the type instead of doing conversions.

void TE_Init();

void TE_ProcessTemplate(StringBuilder* b,String tmpl);
String TE_ProcessTemplate(Arena* out,String tmpl);
void TE_ProcessTemplate(FILE* outputFile,String tmpl);

void TE_SimpleSubstitute(StringBuilder* b,String tmpl,Hashmap<String,String>* subs);
String TE_Substitute(String tmpl,String* valuesToReplace,Arena* out);

void TE_PushScope();
void TE_PopScope();

void TE_Clear();

void TE_SetNumber(String id,int number);
void TE_SetString(String id,String str);
void TE_SetHex(String id,int number);
void TE_SetBool(String id,bool boolean);
