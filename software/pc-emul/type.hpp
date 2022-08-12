#ifndef INCLUDED_TYPE
#define INCLUDED_TYPE

#include <vector>

#include "versatPrivate.hpp"
#include "memory.hpp"
#include "utils.hpp"

struct Member;
struct Type;

struct EnumMember{
   SizedString name;
   SizedString value;

   EnumMember* next;
};

struct TemplateArg{
   Type* type;

   TemplateArg* next;
};

struct Type{
   SizedString name;
   union{
      Type* pointerType;
      Type* arrayType; // Array size can by calculated from size (size / size of arrayed type)
      Type* typedefType;
      EnumMember* enumMembers;

      struct{ // TEMPLATED_STRUCT
         Member* members;
         TemplateArg* templateArgs;
      };

      struct{ // TEMPLATED_INSTANCE
         Type* templateBase;
         TemplateArg* templateArgs_;
      };
   };

   int size; // Size of type (total size in case of arrays)
   enum Subtype {UNKNOWN = 0,BASE,STRUCT,POINTER,ARRAY,TEMPLATED_STRUCT,TEMPLATED_INSTANCE,TEMPLATE_PARAMETER,ENUM,TYPEDEF} type;
};

struct Member{
   Type* type;
   SizedString name;
   int offset;

   Member* next;

#ifdef STRUCT_PARSER
   Type* structType;
   SizedString arrayExpression;
#endif
};

namespace ValueType{
   extern Type* NUMBER;
   extern Type* BOOLEAN;
   extern Type* CHAR;
   extern Type* STRING;
   extern Type* NIL;
   extern Type* SIZED_STRING;
   extern Type* TEMPLATE_FUNCTION;
};

struct TemplateFunction;

struct Value{
   union{
      bool boolean;
      char ch;
      int number;
      struct{
         SizedString str;
         bool literal;
      };
      TemplateFunction* templateFunction;
      StaticInfo staticInfo;
      struct {
         void* custom;
      };
   };

   Type* type;
   bool isTemp;
};

struct Iterator{
   union{
      int currentNumber;
      GenericPoolIterator poolIterator;
   };

   Value iterating;
};

void RegisterTypes();
void FreeTypes();

void Print(Value val);
void OutputObject(void* object,Type* objectType); // TODO: implement

Value CollapsePtrIntoStruct(Value in);
Value CollapseArrayIntoPtr(Value in);
Value ConvertValue(Value in,Type* want);

int ArrayLength(Type* type);

Type* GetType(SizedString typeName); // Parsable C like name (ex: "int*" for pointer to int) [Type name optionally followed by template argument then pointers then array]
Type* GetPointerType(Type* baseType);
Type* GetArrayType(Type* baseType, int arrayLength);

Value AccessObject(Value object,SizedString memberName);
Value AccessObjectIndex(Value object,int index);

Iterator Iterate(Value iterating);
bool HasNext(Iterator iter);
void Advance(Iterator* iter);
Value GetValue(Iterator iter);

bool Equal(Value v1,Value v2);
bool Equal(Type* t1,Type* t2);

// Simple make value functions
Value MakeValue();
Value MakeValue(unsigned int integer);
Value MakeValue(int integer);
Value MakeValue(const char* str);
Value MakeValue(SizedString str);
Value MakeValue(bool boolean);

template<typename T>
Value MakeValue(T val);



#endif // INCLUDED_TYPE
