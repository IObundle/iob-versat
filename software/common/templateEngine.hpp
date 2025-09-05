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

// TODO: Add TemplateSetCAST, TemplateSetStringBuilder and stuff like that.
//       We can even provide faster implementations if we just pass in the top level type instead of doing the conversions above.

void TemplateSetNumber(String id,int number);
void TemplateSetString(String id,String str);
void TemplateSetHex(String id,int number);
void TemplateSetBool(String id,bool boolean);
