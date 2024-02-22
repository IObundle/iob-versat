#pragma once

#include <vector>

#include "utils.hpp"
#include "utilsCore.hpp"

struct Arena;
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
    Array<Pair<String,int>> enumMembers;

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
  int align;
  enum Subtype {UNKNOWN = 0,BASE,STRUCT,POINTER,ARRAY,TEMPLATED_STRUCT_DEF,TEMPLATED_INSTANCE,ENUM,TYPEDEF} type;
};

struct Member{
  Type* type;
  String name;
  int offset;

  //Member* next; // TODO: Appears to not be used. If no compile error, remove later

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
  extern Type* SIZED_STRING_BASE;
  extern Type* TEMPLATE_FUNCTION;
  extern Type* POOL;
  extern Type* ARRAY; // Array templated structure, not to confuse with a normal C style array
  extern Type* STD_VECTOR;
};

struct TemplateFunction;

struct Value{
  union{
    bool boolean;
    char ch;
    i64 number;
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

struct HashmapUnpackedIndex{
  int index;
  bool data;
};

Type* RegisterSimpleType(String name,int size,int align);
Type* RegisterOpaqueType(String name,Type::Subtype subtype,int size,int align);
Type* RegisterEnum(String name,Array<Pair<String,int>> enumValues);
Type* RegisterTypedef(String oldName,String newName);
Type* RegisterTemplate(String baseName,Array<String> templateArgNames);
Type* RegisterStructMembers(String name,Array<Member> members);
Type* RegisterTemplateMembers(String name,Array<TemplatedMember> members);
Type* InstantiateTemplate(String name,Arena* arena = nullptr);

void RegisterTypes();
void FreeTypes();

String GetDefaultValueRepresentation(Value val,Arena* arena);

Value RemoveOnePointerIndirection(Value in);

Value CollapsePtrIntoStruct(Value in);
Value CollapseArrayIntoPtr(Value in);
Value CollapseValue(Value val);

Value ConvertValue(Value in,Type* want,Arena* arena);

Type* GetType(String typeName); // Parsable C like name (ex: "int*" for pointer to int) [Type name optionally followed by template argument then pointers then array]
Type* GetTypeOrFail(String typeName);
Type* GetPointerType(Type* baseType);
Type* GetArrayType(Type* baseType, int arrayLength);

Optional<Value> AccessObjectIndex(Value object,int index);
Optional<Value> AccessStruct(Value object,Member* member);
Optional<Value> AccessStruct(Value val,int index);
Optional<Value> AccessStruct(Value object,String memberName);
Optional<Member*> GetMember(Type* structType,String memberName);

void PrintStructDefinition(Type* type); // Mainly for debug purposes

int ArrayLength(Type* type);
int IndexableSize(Value object); // If type is indexable, return the maximum size
bool IsIndexable(Type* type);
bool IsBasicType(Type* type);
bool IsIndexableOfBasicType(Type* type);
bool IsEmbeddedListKind(Type* type); // Any structure with a next pointer to itself is considered an embedded list
bool IsStruct(Type* type);
Type* GetBaseTypeOfIterating(Type* iterating);

int HashmapIndex(int fullIndex,bool data); 
HashmapUnpackedIndex UnpackHashmapIndex(int index);

Iterator Iterate(Value iterating);
bool HasNext(Iterator iter);
void Advance(Iterator* iter);
Value GetValue(Iterator iter);

bool Equal(Value v1,Value v2);
bool Equal(Type* t1,Type* t2);

// Simple make value functions
Value MakeValue();
Value MakeValue(i64 integer);
Value MakeValue(unsigned int integer);
Value MakeValue(int integer);
Value MakeValue(String str);
Value MakeValue(bool boolean);

Value MakeValue(void* val,const char* typeName);
Value MakeValue(void* val,String typeName);
Value MakeValue(void* val,Type* type);

String ExtractTypeNameFromPrettyFunction(String prettyFunctionFormat,Arena* out);

template<typename T>
String GetTemplateTypeName(Arena* out){
  String func = STRING(__PRETTY_FUNCTION__);
  String typeName = ExtractTypeNameFromPrettyFunction(func,out);

  return typeName;
}

template<typename T>
Value MakeValue(T* t){
  STACK_ARENA(temp,Kilobyte(1));

  String str = GetTemplateTypeName<T>(&temp);
  Value val = MakeValue(t,str);

  return val;
}
