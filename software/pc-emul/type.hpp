#ifndef INCLUDED_TYPE
#define INCLUDED_TYPE

#include <vector>

#include "versat.hpp"
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

struct Type;

struct TypeInfo{
   Type* baseType;
   bool isPointer;
   bool isArray;
};

struct Type{
   SizedString name;
   union{
      TypeInfo info;

      struct{
         Member* members;
         int nMembers;
         int size; // Size of struct or base type
      };
   };

   enum {BASE,STRUCT,INDIRECT,UNKNOWN,COUNT} type;
};

namespace ValueType{
   extern Type* NUMBER;
   extern Type* BOOLEAN;
   extern Type* CHAR;
   extern Type* POOL;
   extern Type* STRING;
   extern Type* NIL;
   extern Type* SIZED_STRING;
   extern Type* TEMPLATE_FUNCTION;
   extern Type* SET;
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
      Pool<FUInstance>* pool; // TODO: Do not need to store Pool, only need to store a generic iterator to the start (and keep track of template type)
      struct {
         void* array;
         int size;
      };
      TemplateFunction* templateFunction;
      StaticInfo staticInfo;
      struct {
         void* custom;
      };
   };

   char smallBuffer[32];
   Type* type;
   bool isTemp;
};

struct Iterator{
   union{
      int currentNumber;
      PoolIterator<FUInstance> poolIterator;
      std::set<StaticInfo>::iterator setIterator;
   };

   Value iterating;
};

void RegisterTypes();

void Print(Value val);
void OutputObject(void* object,Type* objectType); // TODO: implement

Value CollapseArrayIntoPtr(Value in);
Value ConvertValue(Value in,Type* want);

Type* GetType(Type* baseType,bool isPointer = false,bool isArray = false);
Type* GetType(const char* typeName,bool isPointer = false,bool isArray = false);

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

template<typename T>
Value MakeValue(T val);



#endif // INCLUDED_TYPE
