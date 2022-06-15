#ifndef INCLUDED_TYPE
#define INCLUDED_TYPE

#include <vector>

#include "memory.hpp"
#include "utils.hpp"

struct FUInstance;

struct Member{
   const char* baseType;
   const char* name;
   size_t size;
   size_t baseTypeSize;
   int offset;
   int numberPtrs;
   int arrayExpressionEval;
   bool isArray;
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

struct Type{
   TypeInfo* baseType;
   int pointers;
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
         Type customType_;
         void* array;
         int size;
      };
      struct {
         Type customType;
         void* custom;
      };
   };

   char smallBuffer[32];
   ValueType type;
};

struct Iterator{
   union{
      int currentNumber;
      PoolIterator<FUInstance> poolIterator;
   };

   Value iterating;
   ValueType type;
};

void RegisterTypes();

void Print(Value val);
void OutputObject(void* object,Type objectType); // TODO: implement

Value ConvertValue(Value in,ValueType want);

Type GetType(const char* typeName);
Value AccessObject(Value object,SizedString memberName);
Value AccessObjectIndex(Value object,int index);

Iterator Iterate(Value iterating);
bool HasNext(Iterator iter);
void Advance(Iterator* iter);
Value GetValue(Iterator iter);

bool EqualValues(Value v1,Value v2);

// Simple make value functions
Value MakeValue();
Value MakeValue(int integer);
Value MakeValue(const char* str);
Value MakeValue(SizedString str);
Value MakeValue(bool boolean);

#define MAX_CUSTOM_TYPES 256

#endif // INCLUDED_TYPE
