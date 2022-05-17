#ifndef INCLUDED_TYPE
#define INCLUDED_TYPE

#include <vector>

#include "versat.hpp"
#include "utils.hpp"

struct Member{
   const char* baseType;
   const char* name;
   size_t size;
   size_t baseTypeSize;
   int offset;
   int numberPtrs;
   bool isArray;
   int arrayExpressionEval;
};

struct Type{
   int baseTypeId; // TODO: Should be a pointer
   int pointers;
};

// Care, order is important
enum ValueType{NUMBER = 0,BOOLEAN = 1,CHAR,POOL,STRING,STRING_LITERAL,ARRAY,NIL,VECTOR,CUSTOM};

struct TypeInfo {
   std::vector<Member> members;
   int size;
   enum {SIMPLES,STRUCT,UNION,UNKNOWN,COUNT} type;
   int id;
   const char* name;
};

struct Value{
   union{
      bool boolean;
      char ch;
      int number;
      SizedString str;
      Pool<FUInstance>* pool;
      std::vector<FUInstance*>* vec;
      struct {
         int* array;
         int size;
      };
      struct {
         Type customType;
         void* custom;
      };
   };

   ValueType type;
};

void RegisterTypes();

TypeInfo* GetTypeInfo(Type type);


Type GetType(const char* typeName);
Value AccessObject(Value object,SizedString memberName);
Value AccessObjectPointer(Value object,SizedString memberName);
Value AccessObjectIndex(Value object,int index);

bool EqualValues(Value v1,Value v2);

// Simple make value functions
Value MakeValue();
Value MakeValue(int integer);
Value MakeValue(const char* str);
Value MakeValue(SizedString str);
Value MakeValue(bool boolean);

#define MAX_CUSTOM_TYPES 256

#endif // INCLUDED_TYPE
