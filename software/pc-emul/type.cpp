#include "type.hpp"

#include <map>
#include <tuple>

#include "parser.hpp"
#include "versat.hpp"

namespace ValueType{
   Type* NUMBER;
   Type* BOOLEAN;
   Type* CHAR;
   Type* POOL;
   Type* STRING;
   Type* NIL;
   Type* SIZED_STRING;
   Type* TEMPLATE_FUNCTION;
   Type* SET;
};

struct CompareTypeInfo {
   bool operator()(const TypeInfo& left,const TypeInfo& right) const {
      bool res = std::tie(left.baseType,left.isPointer,left.isArray) < std::tie(right.baseType,right.isPointer,right.isArray);

      return res;
   }
};

static Arena permanentArena;
static Pool<Type> types;
static std::map<TypeInfo,Type*,CompareTypeInfo> infoToType;

static Type* RegisterSimpleType(const char* name,int size){
   Type* type = types.Alloc();

   type->type = Type::BASE;
   type->size = size;
   type->name = MakeSizedString(name);

   TypeInfo info = {};
   info.baseType = type;
   infoToType[info] = type;

   return type;
}

static Type* RegisterStruct(const char* name, size_t size,std::initializer_list<Member> data){
   Type* type = GetType(name);

   if(type->type != Type::UNKNOWN && type->nMembers > 0){
      return type;
   }

   type->type = Type::STRUCT;
   type->size = size;
   type->members = (Member*) PushArray(&permanentArena,data.size(),Member);

   for(Member m : data){
      type->members[type->nMembers++] = m;

      Type* typeInfo = GetType(m.baseType);
      if(typeInfo->type == Type::UNKNOWN){
         typeInfo->type = Type::STRUCT;
         typeInfo->size = m.baseTypeSize;
         typeInfo->nMembers = 0;
      }
   }

   TypeInfo info = {};
   info.baseType = type;
   infoToType[info] = type;

   return type;
}

#define A(STRUCT) #STRUCT,sizeof(STRUCT)
#define B(STRUCT,FULLTYPE,TYPE,NAME,PTR,HAS_ARRAY,ARRAY_ELEMENTS)  ((Member){#TYPE,#NAME,sizeof(FULLTYPE),sizeof(TYPE),offsetof(STRUCT,NAME),PTR,ARRAY_ELEMENTS,HAS_ARRAY})

#include "templateEngine.hpp"
#include "verilogParser.hpp"
#include "typeInfo.inc"

void RegisterTypes(){
   static bool registered = false;
   if(registered){
      return;
   }
   registered = true;

   InitArena(&permanentArena,Megabyte(16));
   permanentArena.align = true;

   // Care, order is important
   ValueType::NUMBER = RegisterSimpleType("int",sizeof(int));
   ValueType::BOOLEAN =  RegisterSimpleType("bool",sizeof(char));
   ValueType::CHAR =  RegisterSimpleType("char",sizeof(char));
   ValueType::POOL = RegisterSimpleType("Pool<FUInstance>",sizeof(Pool<FUInstance>*));
   ValueType::NIL = RegisterSimpleType("void",4);

   // Register a string
   ValueType::STRING = GetType("char",true,false);

   RegisterComplexTypes();

   ValueType::SIZED_STRING = GetType("SizedString",false,false);
   ValueType::TEMPLATE_FUNCTION = GetType("TemplateFunction",true,false);
   ValueType::SET = GetType("std::set<StaticInfo>",false,false);
}

Type* GetType(Type* baseType,bool isPointer,bool isArray){
   if(isPointer == false && isArray == false){
      return baseType;
   }

   Assert(!(isPointer && isArray)); // Only 1 can be active at once

   TypeInfo info = {};
   info.baseType = baseType;
   info.isArray = isArray;
   info.isPointer = isPointer;

   auto findDirect = infoToType.find(info);

   if(findDirect != infoToType.end()){
      return findDirect->second;
   }

   Type* type = types.Alloc();

   type->type = Type::INDIRECT;
   type->info = info;
   type->size = sizeof(void*);

   infoToType[type->info] = type;

   return type;
}

Type* GetType(const char* structName,bool isPointer,bool isArray){
   Assert(!(isPointer && isArray)); // Only 1 can be active at once

   for(Type* type : types){
      if(CompareString(structName,type->name)){
         Type* res = GetType(type,isPointer,isArray);
         return res;
      }
   }

   Type* type = types.Alloc();

   type->type = Type::UNKNOWN;
   type->name = PushString(&permanentArena,MakeSizedString(structName));

   return type;
}

static Value CollapseValue(Value val){
   Assert(!val.isTemp); // Only use this function for values you know are references when calling

   val.isTemp = true; // Assume that collapse happens

   if(val.type == ValueType::NUMBER){
      val.number = *((int*) val.custom);
   } else if(val.type == ValueType::BOOLEAN){
      val.boolean = *((bool*) val.custom);
   } else if(val.type == ValueType::CHAR){
      val.ch = *((char*) val.custom);
   } else if(val.type == ValueType::POOL){
      val.pool = (Pool<FUInstance>*) val.custom;
   } else if(val.type == ValueType::NIL){
      val.number = 0;
   } else if(val.type == ValueType::STRING){
      val.str = MakeSizedString(*(char**) val.custom);
   } else if(val.type == ValueType::SIZED_STRING){
      val.str = *((SizedString*) val.custom);
   } else if(val.type == ValueType::TEMPLATE_FUNCTION){
      val.templateFunction = *((TemplateFunction**) val.custom);
   } else {
      val.isTemp = false; // No collapse, same type
   }

   return val;
}

static void* DeferencePointer(void* object,Type* info,int index){
   char** viewPtr = (char**) object;
   char* view = *viewPtr;

   char* objectPtr = view + info->size * index;

   return objectPtr;
}

Value AccessObjectIndex(Value object,int index){
   Value value = {};

   if(object.type == ValueType::POOL){
      Pool<FUInstance>* pool = object.pool;

      Assert(index < pool->Size());

      FUInstance* inst = pool->Get(index);

      value.type = GetType("FUInstance"); // TODO: place to replace when adding template support
      value.custom = (void*) inst;
   } else if(object.type->info.isArray){
      Byte* array = (Byte*) object.array;

      Assert(index < object.size);

      void* objectPtr = (void*) (array + index * object.type->info.baseType->size);

      value.type = object.type->info.baseType;
      value.custom = objectPtr;
   } else if(object.type->info.isPointer) { // Ptr
      Assert(!object.isTemp);

      void* objectPtr = DeferencePointer(object.custom,object.type->info.baseType,index);

      value.type = object.type->info.baseType;
      value.custom = objectPtr;
   } else {
      Assert(false);
   }

   value.isTemp = false;
   value = CollapseValue(value);

   return value;
}

Value AccessObject(Value object,SizedString memberName){
   Type* type = object.type;

   char* deference = nullptr;
   if(object.isTemp){
      deference = (char*) &object;
   } else {
      deference = (char*) object.custom;
   }

   while(type->type == Type::INDIRECT){
      deference = *((char**) deference);
      type = type->info.baseType;
   }

   Assert(type->type == Type::STRUCT);
   Assert(type->nMembers); // Type is incomplete

   int offset = -1;
   Member member = {};
   for(int i = 0; i < type->nMembers; i++){
      Member* m = &type->members[i];
      if(CompareString(m->name,memberName)){
         offset = m->offset;
         member = *m;
         break;
      }
   }

   Assert(offset >= 0);

   #if 0
   for(int i = 0; i < object.customType.pointers; i++){
      deference = *((char**) deference);
   }
   #endif

   char* view = deference;
   void* newObject = &view[offset];

   Type* newType = GetType(member.baseType,member.numberPtrs,member.isArray); // Might fail for multiple pointers / arrays

   Value newValue = {};
   newValue.custom = newObject;
   newValue.type = newType;
   newValue.isTemp = false;

   newValue = CollapseValue(newValue);

   return newValue;
}

void Print(Value val){
   Assert(false);

   #if 0
   switch(val.type){
      case ValueType::NUMBER:{
         printf("%d",val.number);
      }break;
      case ValueType::CUSTOM:{
         printf("\n");
         OutputObject(val.custom,val.customType);
      }break;
      default:{
         Assert(false);
      }break;
   }
   #endif
}

void OutputObject(void* object,Type objectType){
   #if 0
   Type* info = objectType.baseType;

   Byte* ptr = (Byte*) object;
   for(Member m : info->members){
      Value val = {};

      val = CollapseCustomIntoValue(ptr + m.offset,GetType(m.baseType));
      val.customType.pointers = m.numberPtrs;

      printf("%s ",m.name);

      Print(val);

      printf("\n");
   }
   #endif
}

Iterator Iterate(Value iterating){
   Iterator iter = {};
   iter.iterating = iterating;

   if(iterating.type == ValueType::NUMBER){
      iter.currentNumber = 0;
   } else if(iterating.type == ValueType::POOL){
      iter.poolIterator = iterating.pool->begin();
   } else if(iterating.type->info.isArray || iterating.type->info.isPointer){
      iter.currentNumber = 0;
   } else if(iterating.type == ValueType::SET){
      std::set<StaticInfo>* set = (std::set<StaticInfo>*) iterating.custom;

      iter.setIterator = set->begin();
   } else if(iterating.type == GetType("std::vector<StaticInfo>",false,false)){
      iter.currentNumber = 0;
   } else {
      Assert(false);
   }

   return iter;
}

bool HasNext(Iterator iter){
   bool res;

   if(iter.iterating.type == ValueType::NUMBER){
      res = (iter.currentNumber < iter.iterating.number);
   } else if(iter.iterating.type == ValueType::POOL){
      res = (iter.poolIterator != iter.iterating.pool->end());
   } else if(iter.iterating.type->info.isArray || iter.iterating.type->info.isPointer){
      res = (iter.currentNumber < iter.iterating.size);
   } else if(iter.iterating.type == ValueType::SET){
      auto set = (std::set<StaticInfo>*) iter.iterating.custom;

      res = (iter.setIterator != set->end());
   } else if(iter.iterating.type == GetType("std::vector<StaticInfo>",false,false)){
      auto vec = (std::vector<StaticInfo>*) iter.iterating.custom;

      res = (iter.currentNumber < vec->size());
   } else {
      Assert(false);
   }

   return res;
}

void Advance(Iterator* iter){
   if(iter->iterating.type == ValueType::NUMBER){
      iter->currentNumber += 1;
   } else if(iter->iterating.type == ValueType::POOL){
      ++iter->poolIterator;
   } else if(iter->iterating.type->info.isArray || iter->iterating.type->info.isPointer){
      iter->currentNumber += 1;
   } else if(iter->iterating.type == ValueType::SET){
      ++iter->setIterator;
   } else if(iter->iterating.type == GetType("std::vector<StaticInfo>",false,false)){
      iter->currentNumber += 1;
   } else {
      Assert(false);
   }
}

Value GetValue(Iterator iter){
   Value val = {};

   if(iter.iterating.type == ValueType::NUMBER){
      val = MakeValue(iter.currentNumber);
   } else if(iter.iterating.type == ValueType::POOL){
      val.type = GetType("FUInstance");
      val.custom = *iter.poolIterator;
   } else if(iter.iterating.type->info.isArray || iter.iterating.type->info.isPointer){
      val = AccessObjectIndex(iter.iterating,iter.currentNumber);
   } else if(iter.iterating.type == ValueType::SET){
      val.type = GetType("StaticInfo",false,false);
      val.staticInfo = *iter.setIterator;
      val.isTemp = true;
   } else if(iter.iterating.type == GetType("std::vector<StaticInfo>",false,false)){
      auto& vec = *(std::vector<StaticInfo>*) iter.iterating.custom;

      val.type = GetType("StaticInfo",false,false);
      val.staticInfo = vec[iter.currentNumber];
      val.isTemp = true;
   } else {
      Assert(false);
   }

   return val;
}

bool EqualValues(Value v1,Value v2){
   Value c1 = CollapseArrayIntoPtr(v1);
   Value c2 = CollapseArrayIntoPtr(v2);

   c2 = ConvertValue(c2,c1.type);

   bool res = false;

   if(c1.type == ValueType::NUMBER){
      res = (c1.number == c2.number);
   } else if(c1.type == ValueType::STRING){
      res = (CompareString(c1.str,c2.str));
   } else if(c1.type == ValueType::SIZED_STRING){
      res = CompareString(c1.str,c2.str);
   } else {
      Assert(false);
   }

   return res;
}

Value CollapseArrayIntoPtr(Value in){
   if(!in.type->info.isArray){
      return in;
   }

   Value newValue = {};
   newValue.type = GetType(in.type->info.baseType,true,false);
   newValue.custom = &in.custom;
   newValue.isTemp = false;

   newValue = CollapseValue(newValue);

   return newValue;
}

Value ConvertValue(Value in,Type* want){
   if(in.type == want){
      return in;
   }

   Value res = {};
   res.type = want;

   if(want == ValueType::BOOLEAN){
      if(in.type == ValueType::NUMBER){
         res.boolean = (in.number != 0);
      } else if(in.type->type == Type::INDIRECT){
         int* pointerValue = (int*) DeferencePointer(in.custom,in.type->info.baseType,0);

         res.boolean = (pointerValue != nullptr);
      } else {
         Assert(false);
      }
   } else if(want == ValueType::NUMBER){
      if(in.type->type == Type::INDIRECT){
         int* pointerValue = (int*) DeferencePointer(in.custom,in.type->info.baseType,0);

         res.number = (int) pointerValue;
      } else {
         Assert(false);
      }
   } else if(want == ValueType::SIZED_STRING){
      if(in.type == GetType("HierarchyName",false,false)){
         HierarchyName* name = (HierarchyName*) in.custom;

         res = MakeValue(MakeSizedString(name->str));
      } else {
         Assert(false);
      }
   } else {
      Assert(false);
   }

   return res;
}

Value MakeValue(){
   Value val = {};
   val.type = ValueType::NIL;
   val.isTemp = true;
   return val;
}

Value MakeValue(unsigned int integer){
   Value val = {};
   val.number = (int) integer;
   val.type = ValueType::NUMBER;
   val.isTemp = true;
   return val;
}

Value MakeValue(int integer){
   Value val = {};
   val.number = integer;
   val.type = ValueType::NUMBER;
   val.isTemp = true;
   return val;
}

Value MakeValue(const char* str){
   Value res = MakeValue(MakeSizedString(str));
   res.isTemp = true;
   return res;
}

Value MakeValue(SizedString str){
   Value val = {};
   val.str = str;
   val.type = ValueType::STRING;
   val.isTemp = true;
   return val;
}

Value MakeValue(bool boolean){
   Value val = {};
   val.boolean = boolean;
   val.type = ValueType::BOOLEAN;
   val.isTemp = true;
   return val;
}


