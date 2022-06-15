#include "type.hpp"

#include "versat.hpp"

static Pool<TypeInfo> typeInfos;
static int basicTypes = 0;

static void RegisterSimpleType(const char* name,int size,ValueType value){
   TypeInfo* info = typeInfos.Alloc();

   info->type = TypeInfo::SIMPLES;
   info->size = size;
   info->name = name;
   info->id = (int) value;

   basicTypes = maxi(basicTypes,(int) value);
}

static TypeInfo* NameToType(const char* name){
   int nextValue = typeInfos.Size();

   for(TypeInfo* info : typeInfos){
      if(CompareString(info->name,name)){
         return info;
      }
   }

   TypeInfo* res = typeInfos.Alloc();

   res->type = TypeInfo::UNKNOWN;
   res->name = name;
   res->id = nextValue;

   return res;
}

static void RegisterStruct(const char* name, size_t size,std::initializer_list<Member> data){
   TypeInfo* info = NameToType(name);

   info->type = TypeInfo::STRUCT;
   info->size = size;

   int i = 0;
   size_t computedSize = 0;
   for(Member m : data){
      computedSize = ALIGN_4(computedSize);
      computedSize += m.size;

      info->members.push_back(m);

      TypeInfo* typeInfo = NameToType(m.baseType);
      if(typeInfo->type == TypeInfo::UNKNOWN){
         typeInfo->type = TypeInfo::STRUCT;
         typeInfo->size = m.baseTypeSize;
      }
   }
}

#define A(STRUCT) #STRUCT,sizeof(STRUCT)
#define B(STRUCT,FULLTYPE,TYPE,NAME,PTR,HAS_ARRAY,ARRAY_ELEMENTS)  ((Member){#TYPE,#NAME,sizeof(FULLTYPE),sizeof(TYPE),offsetof(STRUCT,NAME),PTR,ARRAY_ELEMENTS,HAS_ARRAY})

#include "verilogParser.hpp"
#include "typeInfo.inc"

void RegisterTypes(){
   static bool registered = false;
   if(registered){
      return;
   }
   registered = true;

   // Care, order is important
   RegisterSimpleType("int",sizeof(int),ValueType::NUMBER);
   RegisterSimpleType("int32_t",sizeof(int32_t),ValueType::NUMBER);
   RegisterSimpleType("bool",sizeof(char),ValueType::BOOLEAN);
   RegisterSimpleType("char",sizeof(char),ValueType::CHAR);
   RegisterSimpleType("Pool<FUInstance>",sizeof(Pool<FUInstance>*),ValueType::POOL);
   RegisterSimpleType("void",4,ValueType::NIL);

   RegisterComplexTypes();

   #if 0
   for(TypeInfo* info : typeInfos){
      printf("%s %s %d\n",info->name,(info->type == TypeInfo::UNKNOWN ? "UNKWNOWN" : "STRUCT"),(int) info->id);
   }
   #endif
}

Type GetType(const char* structName){
   Assert(typeInfos.Size());

   for(TypeInfo* info : typeInfos){
      if(CompareString(info->name,structName)){
         Type type = {};

         type.baseType = info;
         type.pointers = 0; // TODO: Parse struct name for pointers?

         return type;
      }
   }

   Assert(false);
   return (Type){};
}

static Value CollapseCustomIntoValue(void* ptr,Type type){
   Value val = {};
   // Should match the RegisterSimpleType order, maybe change if needed
   switch(type.baseType->id){
   case 0:{
      val.type = ValueType::NUMBER;
      val.number = *((int*) ptr);
   }break;
   case 1:{
      val.type = ValueType::BOOLEAN;
      val.boolean = *((bool*) ptr);
   }break;
   case 2:{
      val.type = ValueType::CHAR;
      val.ch = *((char*) ptr);
   }break;
   case 3:{
      val.type = ValueType::POOL;
      val.pool = (Pool<FUInstance>*) ptr;
   }break;
   case 4:{
      val.type = ValueType::NIL;
   }break;
   default:{
      val.type = ValueType::CUSTOM;
      val.custom = ptr;
      val.customType = type;
      //TODO: make this the one for all function to deal with object and type to value transformation, instead of only collapsing certain values
   }break;
   }

   return val;
}

static void* DeferencePointer(void* object,TypeInfo* info,int index){
   char** viewPtr = (char**) object;
   char* view = *viewPtr;

   char* objectPtr = view + info->size * index;

   return objectPtr;
}

Value AccessObjectIndex(Value object,int index){
   Value value = {};

   Assert(object.type == ValueType::POOL || object.type == ValueType::ARRAY || object.type == ValueType::CUSTOM);

   if(object.type == ValueType::ARRAY){
      Byte* array = (Byte*) object.array;

      Assert(index < object.size);
      void* objectPtr = (void*) (array + index * object.customType.baseType->size);

      value.type = ValueType::CUSTOM;
      value.custom = objectPtr;
      value.customType = object.customType;

      value = CollapseCustomIntoValue(objectPtr,value.customType);
   } else if(object.type == ValueType::POOL){
      Pool<FUInstance>* pool = object.pool;

      Assert(index < pool->Size());

      FUInstance* inst = pool->Get(index);

      value.type = ValueType::CUSTOM;
      value.custom = (void*) inst;
      value.customType = GetType("FUInstance");
   } else { // Ptr
      Assert(object.customType.pointers);

      void* objectPtr = DeferencePointer(object.custom,object.customType.baseType,index);

      value.type = ValueType::CUSTOM;
      value.custom = objectPtr;
      value.customType = object.customType;
      value.customType.pointers -= 1;

      Value v = CollapseCustomIntoValue(objectPtr,value.customType);

      if(v.type != ValueType::NIL){
         value = v;
      }
   }

   return value;
}

Value AccessObject(Value object,SizedString memberName){
   TypeInfo* info = object.customType.baseType;

   int offset = -1;
   Member member = {};
   for(Member m : info->members){
      if(CompareString(m.name,memberName)){
         offset = m.offset;
         member = m;
         break;
      }
   }

   Assert(offset >= 0);
   Assert(info->members.size()); // Type is incomplete

   char* deference = (char*) object.custom;
   for(int i = 0; i < object.customType.pointers; i++){
      deference = *((char**) deference);
   }

   char* view = deference;
   void* newObject = &view[offset];

   Type type = GetType(member.baseType);

   if(type.baseType->id < basicTypes){
      if(member.numberPtrs == 0){
         Value val = CollapseCustomIntoValue(newObject,type);

         if(val.type == ValueType::CHAR && member.isArray){
            val.type = ValueType::STRING;
            val.str = MakeSizedString((char*) newObject);
         }
         return val;
      } else if(member.numberPtrs == 1 && type.baseType->id == 2){
         Value val = {};

         char* str = *(char**) newObject;

         val.type = ValueType::STRING;
         val.str = MakeSizedString(str);
         return val;
      }
   }

   Value newValue = {};
   newValue.customType = type;
   newValue.customType.pointers = member.numberPtrs + (member.isArray ? 1 : 0); // TODO: See array
   newValue.custom = newObject;
   newValue.type = ValueType::CUSTOM;

   return newValue;
}

void Print(Value val){
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
}

void OutputObject(void* object,Type objectType){
   TypeInfo* info = objectType.baseType;

   Byte* ptr = (Byte*) object;
   for(Member m : info->members){
      Value val = {};

      val = CollapseCustomIntoValue(ptr + m.offset,GetType(m.baseType));
      val.customType.pointers = m.numberPtrs;

      printf("%s ",m.name);

      Print(val);

      printf("\n");
   }
}

Iterator Iterate(Value iterating){
   Iterator iter = {};
   iter.iterating = iterating;

   switch(iterating.type){
      case ValueType::NUMBER:{
         iter.currentNumber = 0;
      }break;
      case ValueType::POOL:{
         iter.poolIterator = iterating.pool->begin();
      }break;
      case ValueType::ARRAY:{
         iter.currentNumber = 0;
      }break;
      case ValueType::VECTOR:{
         iter.currentNumber = 0;
      }break;
      default:{
         Assert(false);
      }break;
   }

   return iter;
}

bool HasNext(Iterator iter){
   bool res;
   switch(iter.iterating.type){
      case ValueType::NUMBER:{
         res = (iter.currentNumber < iter.iterating.number);
      }break;
      case ValueType::POOL:{
         res = (iter.poolIterator != iter.iterating.pool->end());
      }break;
      case ValueType::ARRAY:{
         res = (iter.currentNumber < iter.iterating.size);
      }break;
      case ValueType::VECTOR:{
         res = (iter.currentNumber < iter.iterating.vec->size());
      }break;
      default:{
         Assert(false);
      }break;
   }

   return res;
}

void Advance(Iterator* iter){
   switch(iter->iterating.type){
      case ValueType::NUMBER:{
         iter->currentNumber += 1;
      }break;
      case ValueType::POOL:{
         ++iter->poolIterator;
      }break;
      case ValueType::ARRAY:{
         iter->currentNumber += 1;
      }break;
      case ValueType::VECTOR:{
         iter->currentNumber += 1;
      }break;
      default: {
         Assert(false);
      }break;
   }
}

Value GetValue(Iterator iter){
   Value val = {};
   switch(iter.iterating.type){
      case ValueType::NUMBER:{
         val = MakeValue(iter.currentNumber);
      }break;
      case ValueType::POOL:{
         val.customType = GetType("FUInstance");
         val.custom = *iter.poolIterator;
      }break;
      case ValueType::ARRAY:{
         val = AccessObjectIndex(iter.iterating,iter.currentNumber);
      }break;
      case ValueType::VECTOR:{
         val.customType = GetType("FUInstance");
         val.customType.pointers = 1;
         val.custom = &(*iter.iterating.vec)[iter.currentNumber];
      }break;
      default:{
         Assert(false);
      }break;
   }

   return val;
}

bool EqualValues(Value v1,Value v2){
   Assert(v1.type == v2.type);

   bool res = false;
   switch(v1.type){
   case ValueType::STRING:{
      res = (CompareString(v1.str,v2.str));
   }break;
   case ValueType::NUMBER:{
      res = (v1.number == v2.number);
   }break;
   default:{
      DebugSignal();
   }break;
   }

   return res;
}

Value ConvertValue(Value in,ValueType want){
   if(in.type == want){
      return in;
   }

   Value res = {};
   res.type = want;

   switch(want){
   case ValueType::BOOLEAN:{
      switch(in.type){
         case ValueType::NUMBER:{
            res.boolean = (in.number != 0);
         }break;
      default:{
         Assert(false);
      }break;
      }
   }break;
   default:{
      Assert(false);
   }break;
   }

   return res;
}

Value MakeValue(){
   Value val = {};
   val.type = ValueType::NIL;
   return val;
}

Value MakeValue(int integer){
   Value val = {};
   val.number = integer;
   val.type = ValueType::NUMBER;
   return val;
}

Value MakeValue(const char* str){
   Value res = MakeValue(MakeSizedString(str));
   return res;
}

Value MakeValue(SizedString str){
   Value val = {};
   val.str = str;
   val.type = ValueType::STRING;
   return val;
}

Value MakeValue(bool boolean){
   Value val = {};
   val.boolean = boolean;
   val.type = ValueType::BOOLEAN;
   return val;
}


