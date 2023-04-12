#include "type.hpp"

#include "parser.hpp"
#include "versatPrivate.hpp"

namespace ValueType{
   Type* NUMBER;
   Type* SIZE_T;
   Type* BOOLEAN;
   Type* CHAR;
   Type* STRING;
   Type* NIL;
   Type* HASHMAP;
   Type* SIZED_STRING;
   Type* TEMPLATE_FUNCTION;
   Type* SET;
   Type* POOL;
   Type* ARRAY;
   Type* STD_VECTOR;
};

static Arena permanentArena;
static Pool<Type> types;

static void SkipQualifiers(Tokenizer* tok){
   while(1){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"const")){
         tok->AdvancePeek(peek);
         continue;
      }
      if(CompareToken(peek,"volatile")){
         tok->AdvancePeek(peek);
         continue;
      }
      if(CompareToken(peek,"static")){
         tok->AdvancePeek(peek);
         continue;
      }
      if(CompareToken(peek,"inline")){
         tok->AdvancePeek(peek);
         continue;
      }

      break;
   }
}

static String ParseSimpleType(Tokenizer* tok){
   Assert(tok->IsSingleChar("<>,"));

   SkipQualifiers(tok);

   void* mark = tok->Mark();
   Token name = tok->NextToken();

   if(CompareString(name,"unsigned")){
      Token next = tok->NextToken();
      name = ExtendToken(name,next);
   }

   Token peek = tok->PeekToken();
   if(CompareToken(peek,"<")){ // Template
      Token nameRemaining = tok->PeekFindIncluding(">");
      name = ExtendToken(name,nameRemaining);
      tok->AdvancePeek(peek);

      while(1){
         ParseSimpleType(tok);

         Token peek = tok->PeekToken();

         if(CompareString(peek,",")){
            tok->AdvancePeek(peek);
         } else if(CompareString(peek,">")){
            tok->AdvancePeek(peek);
            break;
         }
      }
   }

   String res = tok->Point(mark);
   res = TrimWhitespaces(res);

   return res;
}

static Type* RegisterSimpleType(String name,int size){
   Type* type = types.Alloc();

   type->type = Type::BASE;
   type->size = size;
   type->name = name;

   return type;
}

static Type* RegisterOpaqueType(String name,Type::Subtype subtype,int size){
   for(Type* type : types){
      if(CompareString(type->name,name)){
         Assert(type->size == size);
         return type;
      }
   }

   Type* type = types.Alloc();

   type->name = name;
   type->type = subtype;
   type->size = size;

   return type;
}

static Type* RegisterEnum(String name,Array<Pair<String,int>> enumValues){
   for(Type* type : types){
      if(CompareString(type->name,name)){
         type->type = Type::ENUM;
         type->enumMembers = enumValues;
         return type;
      }
   }

   Type* type = types.Alloc();

   type->name = name;
   type->type = Type::ENUM;
   type->size = sizeof(int);

   return type;
}

static Type* RegisterTypedef(String oldName,String newName){
   Type* type = GetType(newName);
   Type* typedefType = GetType(oldName);

   Assert(type->type == Type::TYPEDEF);

   type->name = newName;
   type->typedefType = typedefType;
   type->size = typedefType->size;

   return type;
}

static Type* RegisterTemplate(String baseName,Array<String> templateArgNames){
   Type* type = types.Alloc();

   type->name = baseName;
   type->type = Type::TEMPLATED_STRUCT_DEF;
   type->templateArgs = templateArgNames;

   return type;
}

static Type* RegisterStructMembers(String name,Array<Member> members){
   Type* type = GetType(name);

   Assert(type->type == Type::STRUCT);

   type->members = members;

   return type;
}

static Type* RegisterTemplateMembers(String name,Array<TemplatedMember> members){
   Type* type = GetType(name);

   Assert(type->type == Type::TEMPLATED_STRUCT_DEF);

   type->templateMembers = members;

   return type;
}

Type* GetSpecificType(String name){
  for(Type* type : types){
      if(CompareString(type->name,name)){
         return type;
      }
   }

   return nullptr;
}

Type* InstantiateTemplate(String name){
   STACK_ARENA(temp,Kilobyte(4));
   Tokenizer tok(name,"<>,",{});

   // This check probably shouldn't be here.
   Type* alreadyExists = GetSpecificType(name);
   if(alreadyExists && alreadyExists->members.size > 0){
      return alreadyExists;
   }

   Hashmap<String,String>* templateToType = PushHashmap<String,String>(&temp,16);

   Token baseName = tok.NextToken();
   Type* templateBase = GetSpecificType(baseName);

   if(!templateBase){
      return nullptr;
   }

   tok.AssertNextToken("<");
   int index = 0;

   bool structDefined = (templateBase->templateArgs.size > 0);

   Array<Type*> templateArgTypes = {};
   if(structDefined){
      templateArgTypes = PushArray<Type*>(&permanentArena,templateBase->templateArgs.size);
      while(!tok.Done()){
         Token peek = tok.PeekToken();
         if(CompareString(peek,">")){
            break;
         }

         String simpleType = ParseSimpleType(&tok);
         String templateArg = templateBase->templateArgs[index];

         templateArgTypes[index] = GetTypeOrFail(simpleType);

         templateToType->Insert(templateArg,simpleType);

         tok.IfNextToken(",");
         index += 1;
      }
   }

   int nMembers = templateBase->templateMembers.size;
   Array<Member> members = PushArray<Member>(&permanentArena,nMembers);
   Array<int> sizes = PushArray<int>(&temp,nMembers);
   Memset(sizes,0);
   for(int i = 0; i < nMembers; i++){
      TemplatedMember templateMember = templateBase->templateMembers[i];
      Tokenizer tok(templateMember.typeName,"*&[],<>",{});

      Byte* mark = MarkArena(&temp);

      while(!tok.Done()){
         Token token = tok.NextToken();

         String* found = templateToType->Get(token);
         if(found){
            PushString(&temp,"%.*s",UNPACK_SS(*found));
         } else {
            PushString(&temp,"%.*s",UNPACK_SS(token));
         }
      }

      String trueType = PointArena(&temp,mark);
      Type* type = GetTypeOrFail(trueType);

      members[i].type = type;
      members[i].name = templateMember.name;
      members[i].offset = templateMember.memberOffset; // Temporarely store member position as the offset, for use later in this function
      sizes[templateMember.memberOffset] = std::max(sizes[templateMember.memberOffset],type->size);

      PopMark(&temp,mark);
   }

   Array<int> offsets = PushArray<int>(&temp,nMembers + 1);
   offsets[0] = 0;
   for(int i = 1; i < offsets.size; i++){
      offsets[i] = offsets[i-1] + sizes[i - 1];
   }

   for(Member& member : members){
      member.offset = offsets[member.offset];
   }

   Type* newType = nullptr;
   if(alreadyExists){
      newType = alreadyExists;
   } else {
      newType = types.Alloc();
   }

   newType->type = Type::TEMPLATED_INSTANCE;
   newType->name = name;
   newType->members = members;
   newType->templateBase = templateBase;
   newType->templateArgTypes = templateArgTypes;

   if(structDefined){
      newType->size = offsets[sizes.size];
   } else {
      newType-> size = templateBase->size;
   }

   return newType;
}

Type* GetType(String name){
   Type* typeExists = GetSpecificType(name);
   if(typeExists){
      return typeExists;
   }

   Tokenizer tok(name,"*[]<>,",{});

   Token templatedName = ParseSimpleType(&tok);

   Type* res = nullptr;
   if(CompareString(name,templatedName)){ // Name equal to templated name means that no pointers or arrays, only templated arguments are not found
      Type* res = InstantiateTemplate(name);
      return res;
   } else if(templatedName.size > 0){
      res = GetType(templatedName);
   } else {
      Token typeName = ParseSimpleType(&tok);
      if(CompareString(typeName,name)){
         return nullptr;
      }
      res = GetType(typeName);
   }

   if(res == nullptr){
      return nullptr;
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
            DEBUG_BREAK();
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

   return res;
}

Type* GetTypeOrFail(String name){
   Type* type = GetType(name);

   if(!type){
      printf("Failed to find type: %.*s\n",UNPACK_SS(name));
      Assert(false);
   }

   return type;
}

Type* GetPointerType(Type* baseType){
   for(Type* type : types){
      if(type->type == Type::POINTER && type->pointerType == baseType){
         return type;
      }
   }

   Type* ret = types.Alloc();
   ret->name = PushString(&permanentArena,"%.*s*",UNPACK_SS(baseType->name));
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

#define A(STRUCT) #STRUCT,sizeof(STRUCT)
#define B(STRUCT,FULLTYPE,TYPE,NAME,PTR,HAS_ARRAY,ARRAY_ELEMENTS)  ((Member){#TYPE,#NAME,sizeof(FULLTYPE),sizeof(TYPE),offsetof(STRUCT,NAME),PTR,ARRAY_ELEMENTS,HAS_ARRAY})

#include "templateEngine.hpp"
#include "verilogParser.hpp"
#include "scratchSpace.hpp"
#include "utils.hpp"
#include "typeInfo.inc"

void RegisterTypes(){
   static bool registered = false;
   if(registered){
      return;
   }
   registered = true;

   permanentArena = InitArena(Megabyte(16));

   ValueType::NUMBER = RegisterSimpleType(STRING("int"),sizeof(int));
   ValueType::SIZE_T = RegisterSimpleType(STRING("size_t"),sizeof(size_t));
   ValueType::BOOLEAN = RegisterSimpleType(STRING("bool"),sizeof(bool));
   ValueType::CHAR = RegisterSimpleType(STRING("char"),sizeof(char));
   ValueType::NIL = RegisterSimpleType(STRING("void"),sizeof(char));
   RegisterSimpleType(STRING("unsigned int"),sizeof(unsigned int));
   RegisterSimpleType(STRING("unsigned char"),sizeof(unsigned char));
   RegisterSimpleType(STRING("float"),sizeof(float));
   RegisterSimpleType(STRING("double"),sizeof(double));

   RegisterParsedTypes();

   ValueType::HASHMAP = GetTypeOrFail(STRING("Hashmap"));
   ValueType::STRING = GetPointerType(ValueType::CHAR);
   ValueType::SIZED_STRING = GetTypeOrFail(STRING("String"));
   ValueType::TEMPLATE_FUNCTION = GetPointerType(GetTypeOrFail(STRING("TemplateFunction")));
   ValueType::POOL = GetTypeOrFail(STRING("Pool"));
   ValueType::ARRAY = GetTypeOrFail(STRING("Array"));
   ValueType::STD_VECTOR = GetTypeOrFail(STRING("std::vector"));
}

void FreeTypes(){
   Free(&permanentArena);
   types.Clear(true);
}

String GetDefaultValueRepresentation(Value in,Arena* arena){
   Value val = CollapseArrayIntoPtr(in);
   Type* type = val.type;
   String res = {};

   if(val.type == ValueType::NUMBER){
      res = PushString(arena,"%lld",val.number);
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
   } else if(val.type == ValueType::SIZE_T){
      res = PushString(arena,"%d",*(int*)val.custom);
   } else if(type == ValueType::NIL){
      res = STRING("Nullptr");
   } else if(val.type->type == Type::POINTER){
      Assert(!val.isTemp);

      void* ptr = *(void**) val.custom;

      if(ptr == nullptr){
         res = PushString(arena,"0x0");
      } else {
         res = PushString(arena,"%p",ptr);
      }
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == ValueType::ARRAY){
         Value size = AccessStruct(val,STRING("size"));
         res = PushString(arena,"Size:%lld",size.number);
      } else {
         res = PushString(arena,"%.*s",UNPACK_SS(type->name));// Return type name
      }
   } else if(type->type == Type::ENUM){
      int enumValue = in.number;
      Pair<String,int>* pairFound = nullptr;
      for(auto pair : type->enumMembers){
         if(pair.second == enumValue){
            pairFound = &pair;
            break;
         }
      }
      if(pairFound){
         res = PushString(arena,"%.*s::%.*s",UNPACK_SS(type->name),UNPACK_SS(pairFound->first));
      } else {
         res = PushString(arena,"(%.*s) %d",UNPACK_SS(type->name),enumValue);
      }
   } else {
      res = PushString(arena,"\"[GetValueRepresentation Type: %.*s]\"",UNPACK_SS(type->name));// Return type name
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
         val.str = STRING("");
      } else {
         val.str = STRING(str);
      }
   } else if(val.type == ValueType::SIZED_STRING){
      val.str = *((String*) val.custom);
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

         int size = object.type->templateArgTypes[0]->size;
         Byte* view = (Byte*) byteArray->data;

         value.custom = &view[index * size];
         value.type = object.type->templateArgTypes[0];
      } else if(object.type->templateBase == ValueType::POOL){
         Iterator iter = Iterate(object);
         for(int i = 0; HasNext(iter) && i < index; Advance(&iter),i += 1);
         value = GetValue(iter);
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
   Assert(IsStruct(structure.type));
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
   Assert(IsStruct(val.type));

   Member* member = &val.type->members[index];
   Value res = AccessStruct(val,member);

   return res;
}

Value AccessStruct(Value object,String memberName){
   Value structure = CollapsePtrIntoStruct(object);
   Type* type = structure.type;

   Assert(type->members.size); // Type is incomplete

   Member* member = nullptr;
   for(Member& m : type->members){
      if(CompareString(m.name,memberName)){
         member = &m;
         break;
      }
   }

   if(member == nullptr){
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
         size = byteSize / type->templateArgTypes[0]->size;
      } else if(type->templateBase == ValueType::ARRAY){
         Array<Byte>* view = (Array<Byte>*) object.custom;
         size = view->size;
      } else if(type->templateBase == ValueType::HASHMAP){
         Hashmap<Byte,Byte>* view = (Hashmap<Byte,Byte>*) object.custom;
         size = view->nodesUsed;
      } else {
         NOT_IMPLEMENTED;
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
      } else if(type->templateBase == ValueType::HASHMAP){
         res = true;
      }
   }

   return res;
}

bool IsBasicType(Type* type){
   bool res = (type == ValueType::NUMBER ||
               type == ValueType::SIZE_T ||
               type == ValueType::BOOLEAN ||
               type == ValueType::CHAR ||
               type == ValueType::STRING ||
               type == ValueType::SIZED_STRING);

   return res;
}

bool IsIndexableOfBasicType(Type* type){
   Assert(IsIndexable(type));

   bool res = false;
   if(type->type == Type::ARRAY){
      res = IsBasicType(type->arrayType);
   } else if(type->type == Type::TEMPLATED_INSTANCE){
      if(type->templateBase == ValueType::POOL){
         res = IsBasicType(type->templateArgTypes[0]);
      } else if(type->templateBase == ValueType::STD_VECTOR){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         res = IsBasicType(type->templateArgTypes[0]);
      } else if(type->templateBase == ValueType::ARRAY){
         res = IsBasicType(type->templateArgTypes[0]);
      } else if(type->templateBase == ValueType::HASHMAP){
         res = false; //res = IsBasicType(type->templateArgTypes[1]);
      }
   }

   return res;
}

bool IsEmbeddedListKind(Type* type){
   if(!IsStruct(type)){
      return false;
   }

   bool res = false;
   for(Member& member : type->members){
      if(CompareString(member.name,"next")){
         Type* memberType = member.type;

         if(memberType->type == Type::POINTER && memberType->pointerType == type){
            res = true;
         }

         break;
      }
   }

   return res;
}

bool IsStruct(Type* type){
   bool res = type->type == Type::STRUCT || type->type == Type::TEMPLATED_INSTANCE;
   return res;
}

Iterator Iterate(Value iterating){
   iterating = CollapsePtrIntoStruct(iterating);

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
         Pool<Byte>* pool = (Pool<Byte>*) iterating.custom; // Any type of pool is good enough

         Byte* page = pool->GetMemoryPtr();

         iter.poolIterator.Init(page,type->templateArgTypes[0]->size);
      } else if(type->templateBase == ValueType::STD_VECTOR){ // TODO: A lot of assumptions are being made for std::vector so this works. Probably not safe (change later)
         iter.currentNumber = 0;
      } else if(type->templateBase == ValueType::ARRAY){
         iter.currentNumber = 0;
      } else if(type->templateBase == ValueType::HASHMAP){
         iter.currentNumber = 0;

         Type* keyType = type->templateArgTypes[0];
         Type* dataType = type->templateArgTypes[1];

         STACK_ARENA(temp,256);
         String pairName = PushString(&temp,"Pair<%.*s,%.*s>",UNPACK_SS(keyType->name),UNPACK_SS(dataType->name));

         Type* exists = GetSpecificType(pairName);
         if(!exists){
            String permanentName = PushString(&permanentArena,pairName);
            exists = GetType(permanentName);
         }
         iter.hashmapType = exists;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(IsEmbeddedListKind(type)){
      // Info stored in iterating
   } else if(type == ValueType::NIL){
      // Do nothing. HasNext will fail
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
      } else if(type->templateBase == ValueType::HASHMAP){
         int len = IndexableSize(iter.iterating);

         bool res = (iter.currentNumber < len);
         return res;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(IsEmbeddedListKind(type)){
      bool res = (iter.iterating.custom != nullptr);

      return res;
   } else if(type == ValueType::NIL){
      return false;
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
      } else if(type->templateBase == ValueType::HASHMAP){
         iter->currentNumber += 1;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(IsEmbeddedListKind(type)){
      Value val = AccessStruct(iter->iterating,STRING("next"));
      Value collapsed = CollapsePtrIntoStruct(val);
      iter->iterating = collapsed;
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
         val.type = type->templateArgTypes[0];
      } else if(type->templateBase == ValueType::STD_VECTOR){
         std::vector<Byte>* b = (std::vector<Byte>*) iter.iterating.custom;

         int size = type->templateArgTypes[0]->size;
         int index = iter.currentNumber;

         Byte* view = b->data();

         val.custom = &view[index * size];
         val.type = type->templateArgTypes[0];
      } else if(type->templateBase == ValueType::ARRAY){
         val = AccessObjectIndex(iter.iterating,iter.currentNumber);
      } else if(type->templateBase == ValueType::HASHMAP){
         Hashmap<Byte,Byte>* view = (Hashmap<Byte,Byte>*) iter.iterating.custom;

         Byte* data = (Byte*) view->data;

         val.custom = &data[iter.currentNumber * iter.hashmapType->size];
         val.type = iter.hashmapType;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(IsEmbeddedListKind(type)){
      return iter.iterating;
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
      String str = c1.str;
      String ss = c2.str;
      Assert(c1.isTemp == true && c2.isTemp == true);

      return CompareString(ss,str);
   } else if(c1.type == ValueType::SIZED_STRING && c2.type == ValueType::STRING){
      String str = c2.str;
      String ss = c1.str;
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
   } else if(IsStruct(c1.type)){
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
      } else if(IsStruct(in.type)){
         res.boolean = (in.custom != nullptr);
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(want == ValueType::NUMBER){
      if(in.type->type == Type::POINTER){
         void* pointerValue = (void*) DeferencePointer(in.custom,in.type->pointerType,0);

         res.number = (int64) pointerValue;
      } else {
         NOT_IMPLEMENTED;
      }
   } else if(want == ValueType::SIZED_STRING){
      if(in.type == ValueType::STRING){
         res = in;
         res.type = want;
      } else if(arena){
         res.str = GetDefaultValueRepresentation(in,arena);
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

Value MakeValue(int64 integer){
   Value val = {};
   val.number = integer;
   val.type = ValueType::NUMBER;
   val.isTemp = true;
   return val;
}

Value MakeValue(unsigned int integer){
   Value val = {};
   val.number = (uint64) integer; // Should probably maintain the unsigned information. TODO
   val.type = ValueType::NUMBER;
   val.isTemp = true;
   return val;
}

Value MakeValue(int integer){
   Value val = {};
   val.number = (int64) integer;
   val.type = ValueType::NUMBER;
   val.isTemp = true;
   return val;
}

Value MakeValue(unsigned int integer);

Value MakeValue(const char* str){
   Value res = MakeValue(STRING(str));
   res.isTemp = true;
   return res;
}

Value MakeValue(String str){
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

   val.type = GetType(STRING(typeName));
   val.custom = entity;
   val.isTemp = false;

   return val;
}

// This shouldn't be here, but cannot be on parser.cpp because otherwise struct parser would fail
Array<Value> ExtractValues(const char* format,Token tok,Arena* arena){
   // TODO: This is pretty much a copy of CheckFormat but with small modifications
   // It probably should be a way to refactor into a single function, and probably less error prone
   if(!CheckFormat(format,tok)){
      return {};
   }

   Byte* mark = MarkArena(arena);

   int tokenIndex = 0;
   for(int formatIndex = 0; 1;){
      char formatChar = format[formatIndex];

      if(formatChar == '\0'){
         break;
      }

      Assert(tokenIndex < tok.size);

      if(formatChar == '%'){
         char type = format[formatIndex + 1];
         formatIndex += 2;

         switch(type){
         case 'd':{
            String numberStr = {};
            numberStr.data = &tok[tokenIndex];

            for(tokenIndex += 1; tokenIndex < tok.size; tokenIndex++){
               if(!IsNum(tok[tokenIndex])){
                  break;
               }
            }

            numberStr.size = &tok.data[tokenIndex] - numberStr.data;
            int number = ParseInt(numberStr);

            Value* val = PushStruct<Value>(arena);
            val->type = ValueType::NUMBER;
            val->number = number;
            val->isTemp = true;
         }break;
         case 's':{
            char terminator = format[formatIndex];
            String str = {};
            str.data = &tok[tokenIndex];

            for(;tokenIndex < tok.size; tokenIndex++){
               if(tok[tokenIndex] == terminator){
                  break;
               }
            }

            str.size = &tok.data[tokenIndex] - str.data;

            Value* val = PushStruct<Value>(arena);
            val->type = ValueType::STRING;
            val->str = str;
            val->isTemp = true;
         }break;
         case '\0':{
            NOT_POSSIBLE;
         }break;
         default:{
            NOT_IMPLEMENTED;
         }break;
         }
      } else {
         Assert(formatChar == tok[tokenIndex]);
         formatIndex += 1;
         tokenIndex += 1;
      }
   }

   Array<Value> values = PointArray<Value>(arena,mark);

   return values;
}

