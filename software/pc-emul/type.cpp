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
   Type* POOL;
   Type* ARRAY;
   Type* STD_VECTOR;
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

   RegisterParsedTypes();

   ValueType::NUMBER = GetType(MakeSizedString("int"));
   ValueType::BOOLEAN =  GetType(MakeSizedString("bool"));
   ValueType::CHAR =  GetType(MakeSizedString("char"));
   ValueType::NIL = GetType(MakeSizedString("void"));
   ValueType::STRING = GetPointerType(ValueType::CHAR);
   ValueType::SIZED_STRING = GetType(MakeSizedString("SizedString"));
   ValueType::TEMPLATE_FUNCTION = GetPointerType(GetType(MakeSizedString("TemplateFunction")));
   ValueType::POOL = GetType(MakeSizedString("Pool"));
   ValueType::ARRAY = GetType(MakeSizedString("Array"));
   ValueType::STD_VECTOR = GetType(MakeSizedString("std::vector"));

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

SizedString GetValueRepresentation(Value in,Arena* arena){
   Value val = CollapseArrayIntoPtr(in);
   SizedString res = {};

   if(val.type == ValueType::NUMBER){
      res = PushString(arena,"%d",val.number);
   } else if(val.type == ValueType::STRING){
      if(val.literal){
         res = PushString(arena,"\"%.*s\"",UNPACK_SS(val.str));
      } else {
         res = PushString(arena,"%.*s",UNPACK_SS(val.str));
      }
   } else if(val.type == ValueType::CHAR){
      res = PushString(arena,"%c",val.ch);
   } else if(val.type == ValueType::SIZED_STRING){
      res = PushString(arena,"%.*s",UNPACK_SS(val.str));
   } else if(val.type == ValueType::BOOLEAN){
      res = PushString(arena,"%s",val.boolean ? "true" : "false");
   } else if(val.type->type == Type::POINTER){
      Assert(!val.isTemp);

      void* ptr = *(void**) val.custom;

      if(ptr == nullptr){
         res = PushString(arena,"0x0");
      } else {
         res = PushString(arena,"%p",ptr);
      }
   } else {
      // Return empty
   }

   return res;
}

Value CollapsePtrIntoStruct(Value object){
   Type* type = object.type;

   if(type->type != Type::POINTER){
      return object;
   }

   Assert(!object.isTemp);

   char* deference = (char*) object.custom;

   while(type->type == Type::POINTER){
      if(deference != nullptr){
         deference = *((char**) deference);
         type = type->pointerType;
      } else {
         return MakeValue();
      }
   }

   if(deference == nullptr){
      return MakeValue();
   }

   Value newVal = {};
   newVal.type = type;
   newVal.custom = deference;

   return newVal;
}

Value CollapseValue(Value val){
   Assert(!val.isTemp); // Only use this function for values you know are references when calling

   val.isTemp = true; // Assume that collapse happens

   if(val.type == ValueType::NUMBER){
      val.number = *((int*) val.custom);
   } else if(val.type == ValueType::BOOLEAN){
      bool boolean = *((bool*) val.custom);
      val.number = 0;
      val.boolean = boolean;
   } else if(val.type == ValueType::CHAR){
      val.ch = *((char*) val.custom);
   } else if(val.type == ValueType::NIL){
      val.number = 0;
   } else if(val.type == ValueType::STRING){
      char* str = *(char**) val.custom;

      if(str == nullptr){
         val.str = MakeSizedString("");
      } else {
         val.str = MakeSizedString(str);
      }
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
      Pool<ComplexFUInstance>* pool = object.pool;

      Assert(index < pool->Size());

      ComplexFUInstance* inst = pool->Get(index);

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
   } else if(object.type->type == Type::TEMPLATED_INSTANCE){
      if(object.type->templateBase == ValueType::ARRAY){
         Array<Byte>* byteArray = (Array<Byte>*) object.custom;

         int size = object.type->templateArgs->type->size;
         Byte* view = (Byte*) byteArray->data;

         value.custom = &view[index * size];
         value.type = object.type->templateArgs->type;
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }

   value.isTemp = false;
   value = CollapseValue(value);

   return value;
}

Value AccessStruct(Value structure,Member* member){
   Assert(structure.type->type == Type::STRUCT);
   Assert(!structure.isTemp);

   char* view = (char*) structure.custom;
   void* newObject = &view[member->offset];

   Value newValue = {};
   newValue.custom = newObject;
   newValue.type = member->type;
   newValue.isTemp = false;

   newValue = CollapseValue(newValue);

   return newValue;
}

Value AccessStruct(Value val,int index){
   Assert(val.type->type == Type::STRUCT);

   Member* member = val.type->members;

   for(int i = 0; i < index && member != nullptr; i++){
      member = member->next;
   }

   Value res = AccessStruct(val,member);

   return res;
}

Value AccessStruct(Value object,SizedString memberName){
   Value structure = CollapsePtrIntoStruct(object);
   Type* type = structure.type;

   Assert(type->members); // Type is incomplete

   int offset = -1;
   Member* member = nullptr;
   for(Member* m = type->members; m != nullptr; m = m->next){
      if(CompareString(m->name,memberName)){
         offset = m->offset;
         member = m;
         break;
      }
   }

   if(offset == -1){
      Log(LogModule::PARSER,LogLevel::FATAL,"Failure to find member named: %.*s on structure: %.*s",UNPACK_SS(memberName),UNPACK_SS(structure.type->name));
   }

   Value res = AccessStruct(structure,member);

   return res;
}

int ArrayLength(Type* type){
   Assert(type->type == Type::ARRAY);

   int arraySize = type->size;
   int baseTypeSize = type->arrayType->size;

   int res = (arraySize / baseTypeSize);

   return res;
}

int IndexableSize(Value object){
   Type* type = object.type;

   Assert(IsIndexable(type));

   int size = 0;

   if(type->type == Type::ARRAY){
      size = ArrayLength(type);
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == ValueType::POOL){
         for(Iterator iter = Iterate(object); HasNext(iter); Advance(&iter)){
            size += 1;
         }
      } else if(type->templateBase == ValueType::STD_VECTOR){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         std::vector<Byte>* b = (std::vector<Byte>*) object.custom;

         int byteSize = b->size();
         size = byteSize / type->templateArgs->type->size;
      } else if(type->templateBase == ValueType::ARRAY){
         Array<Byte>* view = (Array<Byte>*) object.custom;
         size = view->size;
      }
   }

   return size;
}

bool IsIndexable(Type* type){
   bool res = false;

   if(type->type == Type::ARRAY){
      res = true;
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == ValueType::POOL){
         res = true;
      } else if(type->templateBase == ValueType::STD_VECTOR){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         res = true;
      } else if(type->templateBase == ValueType::ARRAY){
         res = true;
      }
   }

   return res;
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

      if(type->templateBase == ValueType::POOL){
         Assert(type->templateArgs);

         Pool<Byte>* pool = (Pool<Byte>*) iterating.custom; // Any type of pool is good enough

         Byte* page = pool->GetMemoryPtr();

         new (&iter.poolIterator) GenericPoolIterator(page,pool->Size(),type->templateArgs->type->size);
      } else if(type->templateBase == ValueType::STD_VECTOR){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         Assert(sizeof(std::vector<Byte>) == sizeof(std::vector<ComplexFUInstance>)); // Assuming std::vector<T> is same for any T (otherwise, cast does not work)

         iter.currentNumber = 0;
      } else if(type->templateBase == ValueType::ARRAY){
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
      if(type->templateBase == ValueType::POOL){
         bool res = iter.poolIterator.HasNext();

         return res;
      } else if(type->templateBase == ValueType::STD_VECTOR){
         int len = IndexableSize(iter.iterating);

         bool res = (iter.currentNumber < len);
         return res;
      } else if(type->templateBase == ValueType::ARRAY){
         int len = IndexableSize(iter.iterating);

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
      if(type->templateBase == ValueType::POOL){
         ++iter->poolIterator;
      } else if(type->templateBase == ValueType::STD_VECTOR){
         iter->currentNumber += 1;
      } else if(type->templateBase == ValueType::ARRAY){
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
      if(type->templateBase == ValueType::POOL){
         val.custom = *iter.poolIterator;
         val.type = type->templateArgs->type;
      } else if(type->templateBase == ValueType::STD_VECTOR){
         std::vector<Byte>* b = (std::vector<Byte>*) iter.iterating.custom;

         int size = type->templateArgs->type->size;
         int index = iter.currentNumber;

         Byte* view = b->data();

         val.custom = &view[index * size];
         val.type = type->templateArgs->type;
      } else if(type->templateBase == ValueType::ARRAY){
         val = AccessObjectIndex(iter.iterating,iter.currentNumber);
      } else {
         NOT_IMPLEMENTED;
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return val;
}

static bool IsPointerLike(Type* type){
   bool res = type->type == Type::POINTER || type->type == Type::ARRAY;
   return res;
}

Value RemoveOnePointerIndirection(Value in){
   Assert(IsPointerLike(in.type));

   Value res = in;

   if(in.type->type == Type::ARRAY){
      res = CollapseArrayIntoPtr(in);
   }

   // Remove one level of indirection
   Assert(in.type->type == Type::POINTER);
   Type* type = res.type;

   char* deference = (char*) res.custom;

   if(deference != nullptr){
      res.custom = *((char**) deference);
      res.type = type->pointerType;
   } else {
      res = MakeValue();
   }

   return res;
}

static bool IsTypeBaseTypeOfPointer(Type* baseToCheck,Type* pointerLikeType){
   Type* checker = pointerLikeType;
   while(IsPointerLike(checker)){
      if(baseToCheck == checker){
         return true;
      } else if(checker->type == Type::ARRAY){
         checker = GetPointerType(checker->arrayType);
      } else if(checker->type == Type::POINTER){
         checker = checker->pointerType;
      } else {
         break;
      }
   }

   bool res = (baseToCheck == checker);
   return res;
}

static Value CollapsePtrUntilType(Value in, Type* typeWanted){
   Value res = in;

   while(res.type != typeWanted){
      if(res.type->type == Type::ARRAY){
         res = CollapseArrayIntoPtr(res);
      } else if(in.type->type == Type::POINTER){
         res = RemoveOnePointerIndirection(res);
      } else {
         NOT_POSSIBLE;
      }
   }

   Assert(res.type == typeWanted);

   return res;
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

   if(IsTypeBaseTypeOfPointer(c2.type,c1.type)){
      c1 = CollapsePtrUntilType(c1,c2.type);
   } else if(IsTypeBaseTypeOfPointer(c1.type,c2.type)){
      c2 = CollapsePtrUntilType(c2,c1.type);
   }

   if(c2.type != c1.type){
      c2 = ConvertValue(c2,c1.type,nullptr);
   }

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
   } else if(c1.type->type == Type::POINTER){
      Assert(!c1.isTemp && !c2.isTemp); // TODO: The concept of temp variables should be better handled. For now, just check that we are dealing with the must common case and proceed

      c1 = RemoveOnePointerIndirection(c1);
      c2 = RemoveOnePointerIndirection(c2);

      res = (c1.custom == c2.custom);
   } else if(c1.type->type == Type::STRUCT){
      Assert(!c1.isTemp);
      Assert(!c2.isTemp);

      res = (c1.custom == c2.custom);
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

Value ConvertValue(Value in,Type* want,Arena* arena){
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
      } else if(in.type->type == Type::STRUCT){
         res.boolean = (in.custom != nullptr);
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
      if(in.type == ValueType::STRING){
         res = in;
         res.type = want;
      } else if(arena){
         res.str = GetValueRepresentation(in,arena);
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

Value MakeValue(uint integer){
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

Value MakeValue(void* entity,const char* typeName){
   Value val = {};

   val.type = GetType(MakeSizedString(typeName));
   val.custom = entity;
   val.isTemp = false;

   return val;
}

