#ifndef INCLUDED_STRUCT_PARSER
#define INCLUDED_STRUCT_PARSER

#include "utils.hpp"

struct Tokenizer;

// Only support one inheritance for now
#if 0
struct InheritanceDef{
   String name;
   enum {PUBLIC,PRIVATE} type;
   InheritanceDef* next;
};
#endif

struct TemplateParamDef{
   String name;
   TemplateParamDef* next;
};

struct MemberDef;

struct StructDef{
   String fullExpression;
   String name;
   String inherit;
   String representationFormat;
   TemplateParamDef* params;
   MemberDef* members;
   bool isUnion;
};

struct TemplatedDef{
   String baseType;
   TemplateParamDef* params;
};

struct EnumDef{
   String name;
   String values;
};

struct TypedefDef{
   String oldType;
   String newType;
};

struct TypeDef{
   union{
      String simpleType;
      TypedefDef typedefType;
      EnumDef enumType;
      TemplatedDef templatedType;
      StructDef structType;
   };
   enum {SIMPLE,TYPEDEF,ENUM,TEMPLATED,STRUCT} type;
};

struct MemberDef{
   String fullExpression; // In theory, should be a combination of all the below
   TypeDef type;
   String pointersAndReferences;
   String name;
   String arrays;

   MemberDef* next;
};

void SkipQualifiers(Tokenizer* tok);
String ParseSimpleType(Tokenizer* tok);
String ParseSimpleFullType(Tokenizer* tok);
EnumDef ParseEnum(Tokenizer* tok);
MemberDef* ParseMember(Tokenizer* tok,Arena* out);
StructDef ParseStruct(Tokenizer* tok,Arena* out);
StructDef ParseTemplatedStructDefinition(Tokenizer* tok,Arena* out);

#endif // INCLUDED_STRUCT_PARSER
