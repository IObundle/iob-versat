#include "type.hpp"

#include <new>
#include <map>
#include <tuple>

#include "parser.hpp"
#include "versatPrivate.hpp"

namespace ValueType{
   Type* NUMBER;
   Type* BOOLEAN;
   Type* CHAR;
   Type* STRING;
   Type* NIL;
   Type* SIZED_STRING;
   Type* TEMPLATE_FUNCTION;
   Type* SET;
};

static Arena permanentArena;
static Pool<Type> types;

static Type* RegisterSimpleType(SizedString name,int size){
   Type* type = types.Alloc();

   type->type = Type::BASE;
   type->size = size;
   type->name = name;

   return type;
}

static Type* RegisterOpaqueType(SizedString name,Type::Subtype subtype,int size){
   Type* type = types.Alloc();

   type->name = name;
   type->type = subtype;
   type->size = size;

   return type;
}

static Type* RegisterOpaqueTemplateStruct(SizedString name){
   Type* type = types.Alloc();

   type->name = name;
   type->type = Type::TEMPLATED_STRUCT;

   return type;
}

static Type* RegisterEnum(SizedString name){
   Type* type = types.Alloc();

   type->name = name;
   type->type = Type::ENUM;
   type->size = sizeof(int);

   return type;
}

static Type* RegisterTypedef(SizedString name,SizedString typedefName){
   Type* type = GetType(name);
   Type* typedefType = GetType(typedefName);

   Assert(type->type == Type::TYPEDEF);

   type->name = name;
   type->typedefType = typedefType;
   type->size = typedefType->size;

   return type;
}

static Type* RegisterTemplateInstance(SizedString fullName,SizedString baseName,std::initializer_list<SizedString> data){
   Type* type = GetType(fullName);
   Assert(type->type == Type::TEMPLATED_INSTANCE);

   type->templateBase = GetType(baseName);

   for(SizedString name : data){
      TemplateArg* arg = PushStruct(&permanentArena,TemplateArg);

      arg->type = GetType(name);
      arg->next = type->templateArgs;

      type->templateArgs = arg;
   }

   return type;
}

static Type* RegisterStructMembers(SizedString name,std::initializer_list<Member*> data){
   Type* type = GetType(name);

   Assert(type->type == Type::STRUCT || type->type == Type::TEMPLATED_INSTANCE);

   Member* ptr = nullptr;
   for(Member* m : data){
      m->next = ptr;
      ptr = m;
   }

   type->members = ptr;

   return type;
}

Type* GetType(SizedString name){
   for(Type* type : types){
      if(CompareString(type->name,name)){
         return type;
      }
   }

   Tokenizer tok(name,"*[]<>",{});

   Token templatedName = tok.PeekFindIncluding(">");

   Type* res = nullptr;
   if(CompareString(name,templatedName)){ // Name equal to templated name means that no pointers or arrays, only templated arguments are not found
      Token baseName = tok.NextToken();
      tok.AssertNextToken("<");

      Type* base = GetType(baseName);

      Token argName = tok.PeekFindUntil(">");
      tok.AdvancePeek(argName);
      tok.AssertNextToken(">");

      Assert(!Contains(argName,",")); // Only handle 1 argument, for now

      Type* arg = GetType(argName);

      Type* newType = types.Alloc();

      newType->type = Type::TEMPLATED_INSTANCE;
      newType->name = PushString(&permanentArena,name);
      newType->templateBase = base;
      newType->templateArgs = PushStruct(&permanentArena,TemplateArg);

      newType->templateArgs->type = arg;

      res = newType;
   } else if(templatedName.size > 0){
      res = GetType(templatedName);
   } else {
      Token typeName = tok.NextToken();
      res = GetType(typeName);
   }

   while(!tok.Done()){
      Token next = tok.NextToken();

      if(CompareString(next,"*")){
         bool found = false;
         for(Type* type : types){
            if(type->type == Type::POINTER && type->pointerType == res){
               found = true;
               res = type;
               break;
            }
         }

         if(!found){
            res = GetPointerType(res);
         }

         continue;
      }

      if(CompareString(next,"[")){
         Token arrayExpression = tok.PeekFindUntil("]");
         tok.AdvancePeek(arrayExpression);
         tok.AssertNextToken("]");

         if(!CheckFormat("%d",arrayExpression)){
            DEBUG_BREAK;
         }

         int arraySize = ParseInt(arrayExpression);
         bool found = false;
         for(Type* type : types){
            if(type->type == Type::ARRAY && type->arrayType == res && type->size == (arraySize * res->size)){
               res = type;
               found = true;
               break;
            }
         }

         if(!found){
            res = GetArrayType(res,arraySize);
         }

         continue;
      }

      NOT_POSSIBLE;
   }
   Assert(res);

   return res;
}

Type* GetPointerType(Type* baseType){
   for(Type* type : types){
      if(type->type == Type::POINTER && type->pointerType == baseType){
         return type;
      }
   }

   Type* ret = types.Alloc();
   ret->name = PushString(&permanentArena,"%.*s*",UNPACK_SS(baseType->name));;
   ret->type = Type::POINTER;
   ret->pointerType = baseType;
   ret->size = sizeof(void*);

   return ret;
}

Type* GetArrayType(Type* baseType, int arrayLength){
   for(Type* type : types){
      if(type->type == Type::ARRAY && type->arrayType == baseType && ArrayLength(type) == arrayLength){
         return type;
      }
   }

   Type* ret = types.Alloc();
   ret->name = PushString(&permanentArena,"%.*s[%d]",UNPACK_SS(baseType->name),arrayLength);
   ret->type = Type::ARRAY;
   ret->pointerType = baseType;
   ret->size = arrayLength * baseType->size;

   return ret;
}

static Type* GetArrayType(SizedString name,int arrayLength){
   Type* type = GetType(name);
   Type* res = GetArrayType(type,arrayLength);

   return res;
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

   RegisterParsedTypes();

   ValueType::NUMBER = GetType(MakeSizedString("int"));
   ValueType::BOOLEAN =  GetType(MakeSizedString("bool"));
   ValueType::CHAR =  GetType(MakeSizedString("char"));
   ValueType::NIL = GetType(MakeSizedString("void"));
   ValueType::STRING = GetPointerType(ValueType::CHAR);
   ValueType::SIZED_STRING = GetType(MakeSizedString("SizedString"));
   ValueType::TEMPLATE_FUNCTION = GetPointerType(GetType(MakeSizedString("TemplateFunction")));

   #if 0
   for(Type* type : types){
      printf("%.*s: Size:%d Type:%d\n",UNPACK_SS(type->name),type->size,(int) type->type);

      if(type->type == Type::STRUCT ){
         for(Member* m = type->members; m != nullptr; m = m->next){
            printf("  [%.*s: Size:%d Type:%d] %.*s Offset: %d\n",UNPACK_SS(m->type->name),m->type->size,(int) m->type->type,UNPACK_SS(m->name),m->offset);
         }
      }
   }
   #endif
}

void FreeTypes(){
   Free(&permanentArena);
   types.Clear(true);
}

int ArrayLength(Type* type){
   Assert(type->type == Type::ARRAY);

   int arraySize = type->size;
   int baseTypeSize = type->arrayType->size;

   int res = (arraySize / baseTypeSize);

   return res;
}


Value CollapsePtrIntoStruct(Value object){
   Type* type = object.type;

   char* deference = (char*) object.custom;
   while(type->type == Type::POINTER){
      deference = *((char**) deference);
      type = type->pointerType;
   }

   Value newVal = {};
   newVal.type = type;
   newVal.custom = deference;

   return newVal;
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
   } else if(val.type == ValueType::NIL){
      val.number = 0;
   } else if(val.type == ValueType::STRING){
      val.str = MakeSizedString(*(char**) val.custom);
   } else if(val.type == ValueType::SIZED_STRING){
      val.str = *((SizedString*) val.custom);
   } else if(val.type == ValueType::TEMPLATE_FUNCTION){
      val.templateFunction = *((TemplateFunction**) val.custom);
   } else if(val.type->type == Type::ENUM){
      val.number = *(int*) val.custom;
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

   #if 0
   if(object.type == ValueType::POOL){
      Pool<FUInstance>* pool = object.pool;

      Assert(index < pool->Size());

      FUInstance* inst = pool->Get(index);

      value.type = GetType(MakeSizedString("FUInstance")); // TODO: place to replace when adding template support
      value.custom = (void*) inst;
   } else
   #endif
   if(object.type->type == Type::ARRAY){
      Type* arrayType = object.type;
      Byte* array = (Byte*) object.custom;

      int arrayIndexSize = (arrayType->size / arrayType->arrayType->size);

      Assert(index < arrayIndexSize);

      void* objectPtr = (void*) (array + index * object.type->arrayType->size);

      value.type = object.type->arrayType;
      value.custom = objectPtr;
   } else if(object.type->type == Type::POINTER) { // Ptr
      Assert(!object.isTemp);

      void* objectPtr = DeferencePointer(object.custom,object.type->pointerType,index);

      value.type = object.type->pointerType;
      value.custom = objectPtr;
   } else {
      NOT_IMPLEMENTED;
   }

   value.isTemp = false;
   value = CollapseValue(value);

   return value;
}

Value AccessObject(Value object,SizedString memberName){
   Assert(!object.isTemp); // Must be a struct and structs are never temp

   Value structure = CollapsePtrIntoStruct(object);
   Type* type = structure.type;
   char* deference = (char*) structure.custom;

   Assert(type->type == Type::STRUCT);
   Assert(type->members); // Type is incomplete

   int offset = -1;
   Member member = {};
   for(Member* m = type->members; type->members != nullptr; m = m->next){
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

   Value newValue = {};
   newValue.custom = newObject;
   newValue.type = member.type;;
   newValue.isTemp = false;

   newValue = CollapseValue(newValue);

   return newValue;
}

void Print(Value val){
   NOT_IMPLEMENTED;

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
         NOT_IMPLEMENTED;
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
   Type* type = iterating.type;

   Iterator iter = {};
   iter.iterating = iterating;

   if(type == ValueType::NUMBER){
      iter.currentNumber = 0;
   } else if(type->type == Type::ARRAY){
      iter.currentNumber = 0;
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      Assert(iterating.isTemp == false);

      if(type->templateBase == GetType(MakeSizedString("Pool"))){
         Assert(type->templateArgs);

         Pool<Byte>* pool = (Pool<Byte>*) iterating.custom; // Any type of pool is good enough

         Byte* page = pool->GetMemoryPtr();

         new (&iter.poolIterator) GenericPoolIterator(page,pool->Size(),type->templateArgs->type->size);
      } else if(type->templateBase == GetType(MakeSizedString("std::vector"))){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         Assert(sizeof(std::vector<Byte>) == sizeof(std::vector<FUInstance>)); // Assuming std::vector<T> is same for any T (otherwise, cast does not work)

         iter.currentNumber = 0;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return iter;
}

bool HasNext(Iterator iter){
   Type* type = iter.iterating.type;

   if(type == ValueType::NUMBER){
      int len = iter.iterating.number;

      bool res = (iter.currentNumber < len);
      return res;
   } else if(type->type == Type::ARRAY){
      int len = ArrayLength(type);

      bool res = (iter.currentNumber < len);
      return res;
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == GetType(MakeSizedString("Pool"))){
         bool res = iter.poolIterator.HasNext();

         return res;
      } else if(type->templateBase == GetType(MakeSizedString("std::vector"))){
         std::vector<Byte>* b = (std::vector<Byte>*) iter.iterating.custom;

         int byteSize = b->size();
         int len = byteSize / type->templateArgs->type->size;

         bool res = (iter.currentNumber < len);
         return res;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return false;
}

void Advance(Iterator* iter){
   Type* type = iter->iterating.type;

   if(type == ValueType::NUMBER){
      iter->currentNumber += 1;
   } else if(type->type == Type::ARRAY){
      iter->currentNumber += 1;
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == GetType(MakeSizedString("Pool"))){
         ++iter->poolIterator;
      } else if(type->templateBase == GetType(MakeSizedString("std::vector"))){
         iter->currentNumber += 1;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }
}

Value GetValue(Iterator iter){
   Value val = {};

   Type* type = iter.iterating.type;

   if(type == ValueType::NUMBER){
      val = MakeValue(iter.currentNumber);
   } else if(type->type == Type::ARRAY){
      val = AccessObjectIndex(iter.iterating,iter.currentNumber);
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == GetType(MakeSizedString("Pool"))){
         val.custom = *iter.poolIterator;
         val.type = type->templateArgs->type;
      } else if(type->templateBase == GetType(MakeSizedString("std::vector"))){
         std::vector<Byte>* b = (std::vector<Byte>*) iter.iterating.custom;

         int size = type->templateArgs->type->size;
         int index = iter.currentNumber;

         Byte* view = b->data();

         val.custom = &view[index * size];
         val.type = type->templateArgs->type;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return val;
}

bool Equal(Value v1,Value v2){
   Value c1 = CollapseArrayIntoPtr(v1);
   Value c2 = CollapseArrayIntoPtr(v2);

   if(c1.type == ValueType::STRING && c2.type == ValueType::SIZED_STRING){
      SizedString str = c1.str;
      SizedString ss = c2.str;
      Assert(c1.isTemp == true && c2.isTemp == true);

      return CompareString(ss,str);
   } else if(c1.type == ValueType::SIZED_STRING && c2.type == ValueType::STRING){
      SizedString str = c2.str;
      SizedString ss = c1.str;
      Assert(c2.isTemp == true && c1.isTemp == true);

      return CompareString(ss,str);
   }

   c2 = ConvertValue(c2,c1.type);

   bool res = false;

   if(c1.type == ValueType::NUMBER){
      res = (c1.number == c2.number);
   } else if(c1.type == ValueType::STRING){
      res = (CompareString(c1.str,c2.str));
   } else if(c1.type == ValueType::SIZED_STRING){
      res = CompareString(c1.str,c2.str);
   } else if(c1.type->type == Type::ENUM){
      if(c1.type != c2.type){
         res = false;
      } else {
         res = (c1.number == c2.number);
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return res;
}

bool Equal(Type* t1,Type* t2){
   if(t1->type != t2->type){
      return false;
   }

   if(t1->size != t2->size){
      return false;
   }

   if(t1->pointerType != t2->pointerType){
      return false;
   }

   return CompareString(t1->name,t2->name);
}

Value CollapseArrayIntoPtr(Value in){
   if(in.type->type != Type::ARRAY){
      return in;
   }

   Value newValue = {};
   newValue.type = GetPointerType(in.type->arrayType);
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
      if(in.type == ValueType::NUMBER || in.type->type == Type::ENUM){
         res.boolean = (in.number != 0);
      } else if(in.type->type == Type::POINTER){
         int* pointerValue = (int*) DeferencePointer(in.custom,in.type->pointerType,0);

         res.boolean = (pointerValue != nullptr);
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(want == ValueType::NUMBER){
      if(in.type->type == Type::POINTER){
         int* pointerValue = (int*) DeferencePointer(in.custom,in.type->pointerType,0);

         res.number = (int) pointerValue;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(want == ValueType::SIZED_STRING){
      if(in.type == GetType(MakeSizedString("HierarchyName"))){
         HierarchyName* name = (HierarchyName*) in.custom;

         res = MakeValue(MakeSizedString(name->str));
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(want->type == Type::ENUM){
      if(in.type == ValueType::NUMBER){
         res = in;
         res.type = want;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
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


