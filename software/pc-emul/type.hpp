#ifndef INCLUDED_TYPE
#define INCLUDED_TYPE

#include <vector>

#include "versatPrivate.hpp"
#include "memory.hpp"
#include "utils.hpp"

struct Member;
struct Type;

struct EnumMember{
   String name;
   String value;

   EnumMember* next;
};

struct TemplateArg{
   String name;
   TemplateArg* next;
};

struct TemplatedMember{
   String typeName;
   String name;
   int memberOffset;
};

struct Type{
   String name;
   Type* baseType; // For struct inheritance
   union{
      Type* pointerType;
      Type* arrayType; // Array size can by calculated from size (size / size of arrayed type)
      Type* typedefType;
      EnumMember* enumMembers;

      struct{ // TEMPLATED_STRUCT_DEF
         Array<TemplatedMember> templateMembers;
         Array<String> templateArgs;
      };

      struct{ // TEMPLATED_INSTANCE
         Array<Member> members;
         Type* templateBase; // Points to the template base type that lead to this instance
         Array<Type*> templateArgTypes;
      };
   };

   int size; // Size of type (total size in case of arrays)
   enum Subtype {UNKNOWN = 0,BASE,STRUCT,POINTER,ARRAY,TEMPLATED_STRUCT_DEF,TEMPLATED_INSTANCE,ENUM,TYPEDEF} type;
};

struct Member{
   Type* type;
   String name;
   int offset;

   Member* next;

// STRUCT_PARSER
   Type* structType;
   String arrayExpression;
   int index;
};

namespace ValueType{
   extern Type* NUMBER;
   extern Type* SIZE_T;
   extern Type* BOOLEAN;
   extern Type* CHAR;
   extern Type* STRING;
   extern Type* NIL;
   extern Type* HASHMAP;
   extern Type* SIZED_STRING;
   extern Type* TEMPLATE_FUNCTION;
   extern Type* POOL;
   extern Type* ARRAY;
   extern Type* STD_VECTOR;
};

struct TemplateFunction;

struct Value{
   union{
      bool boolean;
      char ch;
      int64 number;
      struct{
         String str;
         bool literal;
      };
      TemplateFunction* templateFunction;
      struct {
         void* custom;
      };
   };

   Type* type;
   bool isTemp; // Temp is stored inside the Value, instead of storing a pointer to it in the custom variable
};

struct Iterator{
   union{
      int currentNumber;
      GenericPoolIterator poolIterator;
   };
   Type* hashmapType;

   Value iterating;
};

void RegisterTypes();
void FreeTypes();

String GetValueRepresentation(Value val,Arena* arena);

Value RemoveOnePointerIndirection(Value in);

Value CollapsePtrIntoStruct(Value in);
Value CollapseArrayIntoPtr(Value in);
Value CollapseValue(Value val);

Value ConvertValue(Value in,Type* want,Arena* arena);

Type* GetType(String typeName); // Parsable C like name (ex: "int*" for pointer to int) [Type name optionally followed by template argument then pointers then array]
Type* GetTypeOrFail(String typeName);
Type* GetPointerType(Type* baseType);
Type* GetArrayType(Type* baseType, int arrayLength);

Value AccessStruct(Value object,Member* member);
Value AccessStruct(Value val,int index);
Value AccessStruct(Value object,String memberName);
Value AccessObjectIndex(Value object,int index);

int ArrayLength(Type* type);
int IndexableSize(Value object); // If type is indexable, return the maximum size
bool IsIndexable(Type* type);

Iterator Iterate(Value iterating);
bool HasNext(Iterator iter);
void Advance(Iterator* iter);
Value GetValue(Iterator iter);

bool Equal(Value v1,Value v2);
bool Equal(Type* t1,Type* t2);

// Simple make value functions
Value MakeValue();
Value MakeValue(int64 integer);
Value MakeValue(unsigned int integer);
Value MakeValue(int integer);
Value MakeValue(String str);
Value MakeValue(bool boolean);

Value MakeValue(void* val,const char* typeName);

#endif // INCLUDED_TYPE
