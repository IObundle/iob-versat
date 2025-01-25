#include "type.hpp"

#include <cinttypes>
#include <cstddef>

#include "memory.hpp"
#include "parser.hpp"
#include "structParsing.hpp"
#include "utilsCore.hpp"

namespace ValueType{
  Type* NUMBER;
  Type* SIZE_T;
  Type* BOOLEAN;
  Type* CHAR;
  Type* STRING;
  Type* CONST_STRING;
  Type* NIL;
  Type* HASHMAP;
  Type* SIZED_STRING;
  Type* SIZED_STRING_BASE;  // TODO: This is ugly and comes from the fact that SizedString is a typedef of Array<const char>. There should be an equality function that checks 
  Type* TEMPLATE_FUNCTION;
  Type* SET;
  Type* POOL;
  Type* ARRAY;
  Type* STD_VECTOR;
};

static Arena permanentArena;
static Pool<Type> types;

TypeIterator IterateTypes(){
  TypeIterator iter = {};
  iter.iter = types.begin();
  iter.end = types.end();
  return iter;
}

bool HasNext(TypeIterator iter){
  bool res = (iter.iter != iter.end);
  return res;
}

Type* Next(TypeIterator& iter){
  Type* type = *iter.iter;
  ++iter.iter;
  return type;
}

static Type* CollapseTypeUntilBase(Type* type){
  Type* res = type;
  while(true){
	if(res->type == Subtype_POINTER){
	  res = res->pointerType;
	} else if(res->type == Subtype_TYPEDEF){
	  res = res->typedefType;
	} else {
	  break;
	}
  }
  return res;
}

String GetUniqueRepresentationOrFail(String name,Arena* out){
  STACK_ARENA(temp,Kilobyte(4));
  Opt<ParsedType> parsed = ParseType(name,&temp);
  Assert(parsed.has_value());
  
  String uniqueName = PushUniqueRepresentation(out,parsed.value());

  return uniqueName;
}

Type* RegisterSimpleType(String name,int size,int align){
  Type* type = types.Alloc();

  type->type = Subtype_BASE;
  type->size = size;
  type->name = GetUniqueRepresentationOrFail(name,&permanentArena);
  type->align = align;

  return type;
}

Type* RegisterOpaqueType(String name,Subtype subtype,int size,int align){
  STACK_ARENA(temp,Kilobyte(4));
  String tempName = GetUniqueRepresentationOrFail(name,&temp);

  for(Type* type : types){
    if(CompareString(type->name,tempName)){
      Assert(type->size == size);
      return type;
    }
  }

  Type* type = types.Alloc();

  type->name = PushString(&permanentArena,tempName);
  type->type = subtype;
  type->size = size;
  type->align = align;

  return type;
}

Type* RegisterEnum(String name,int size,int align,Array<Pair<String,int>> enumValues){
  STACK_ARENA(temp,Kilobyte(4));
  String tempName = GetUniqueRepresentationOrFail(name,&temp);

  for(Type* type : types){
    if(CompareString(type->name,tempName)){
      type->type = Subtype_ENUM;
      type->size = size;
      type->align = align;
      type->enumMembers = enumValues;
      return type;
    }
  }

  Type* type = types.Alloc();

  type->name = PushString(&permanentArena,tempName);
  type->type = Subtype_ENUM;
  type->size = size;
  type->align = align;
  type->enumMembers = enumValues;

  return type;
}

Type* RegisterTypedef(String oldName,String newName){
  Type* type = GetType(newName);
  Type* typedefType = GetType(oldName);

  Assert(type->type == Subtype_TYPEDEF);

  type->name = GetUniqueRepresentationOrFail(newName,&permanentArena);
  type->typedefType = typedefType;
  type->size = typedefType->size;
  type->align = typedefType->align;

  return type;
}

Type* RegisterTemplate(String baseName,Array<String> templateArgNames){
  Type* type = types.Alloc();

  type->name = GetUniqueRepresentationOrFail(baseName,&permanentArena);
  type->type = Subtype_TEMPLATED_STRUCT_DEF;
  type->templateArgs = templateArgNames;

  return type;
}

Type* RegisterStructMembers(String name,Array<Member> members){
  STACK_ARENA(temp,Kilobyte(4));
  String tempName = GetUniqueRepresentationOrFail(name,&temp);

  Type* type = GetType(tempName);

  Assert(type->type == Subtype_STRUCT);

  type->members = members;

  return type;
}

Type* RegisterTemplateMembers(String name,Array<TemplatedMember> members){
  STACK_ARENA(temp,Kilobyte(4));
  String tempName = GetUniqueRepresentationOrFail(name,&temp);

  Type* type = GetType(tempName);

  Assert(type->type == Subtype_TEMPLATED_STRUCT_DEF);

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

String GetBaseTypeName(String name){
  Tokenizer tok(name,"*[]",{});

  auto mark = tok.Mark();
  while(!tok.Done()){
    Token t = tok.PeekToken();

    if(CompareString(t,"*")){
      break;
    } else if(CompareString(t,"[")){
      break;
    } else if(CompareString(t,"]")){
      break;
    }

    tok.AdvancePeek();
  }

  String res = tok.Point(mark);
  return res;
}

Type* InstantiateTemplate(String name,Arena* arena){
  STACK_ARENA(temp,Kilobyte(4));
  Tokenizer tok(name,"<>,",{});
  
  // This check probably shouldn't be here.
  Type* alreadyExists = GetSpecificType(name);
  if(alreadyExists && alreadyExists->members.size > 0){
    return alreadyExists;
  }

  // Allocates first so that pointers to itself resolve correctly.
  if(arena){
    name = PushString(arena,"%.*s",UNPACK_SS(name));
  }

  Type* newTypeAlloc = types.Alloc();
  newTypeAlloc->name = name;
  
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

      String simpleType = ParseSimpleFullType(&tok);
      String templateArg = templateBase->templateArgs[index];

      templateArgTypes[index] = GetTypeOrFail(simpleType);

      templateToType->Insert(templateArg,simpleType);

      tok.IfNextToken(",");
      index += 1;
    }
  }

  int nMembers = templateBase->templateMembers.size;
  int numberPositions = 0;
  for(TemplatedMember& m : templateBase->templateMembers){
    numberPositions = std::max(numberPositions,m.memberOffset);
  }
  numberPositions += 1;

  bool mustFixRecursion = false;
  Array<Member> members = PushArray<Member>(&permanentArena,nMembers);
  Array<int> sizes = PushArray<int>(&temp,numberPositions);
  Array<int> align = PushArray<int>(&temp,numberPositions);
  Memset(sizes,0);
  Memset(align,0);

  for(int i = 0; i < templateBase->templateMembers.size; i++){
    TemplatedMember templateMember = templateBase->templateMembers[i];
    Tokenizer tok(templateMember.typeName,"*&[],<>",{});

    BLOCK_REGION(&temp);
    auto mark = StartString(&temp);

    while(!tok.Done()){
      Token token = tok.NextToken();

      String* found = templateToType->Get(token);
      if(found){
        PushString(&temp,"%.*s",UNPACK_SS(*found));
      } else {
        PushString(&temp,"%.*s",UNPACK_SS(token));
      }
    }

    String trueType = EndString(mark);
    String baseType = GetBaseTypeName(trueType);

    // TODO: This feels a bit like a hack. But it does work. Maybe it's fine with a bit of a cleanup and comments
    // Must be a pointer otherwise struct contains itself
    if(CompareString(baseType,name)){
      mustFixRecursion = true;

      Type* type = ValueType::NIL;
      members[i].type = ValueType::NIL;
      members[i].name = templateMember.name;
      members[i].offset = templateMember.memberOffset; // Temporarely store member position as the offset, for use later in this function
      sizes[templateMember.memberOffset] = std::max(sizes[templateMember.memberOffset],type->size);
      align[templateMember.memberOffset] = std::max(align[templateMember.memberOffset],type->align);

      continue;
    }
 
    Type* type = GetTypeOrFail(trueType); // MARKER - Fails when member contains same type as instantiated type (such as linked lists). Need to check for this situation and possibly change the code.

    members[i].type = type;
    members[i].name = templateMember.name;
    members[i].offset = templateMember.memberOffset; // Temporarely store member position as the offset, for use later in this function
    sizes[templateMember.memberOffset] = std::max(sizes[templateMember.memberOffset],type->size);
    align[templateMember.memberOffset] = std::max(align[templateMember.memberOffset],type->align);
  }

  Array<int> offsets = PushArray<int>(&temp,numberPositions + 1);
  offsets[0] = 0;
  for(int i = 1; i < offsets.size; i++){
    offsets[i] = Align(offsets[i-1],align[i-1]) + sizes[i - 1];
  }

  for(Member& member : members){
    member.offset = offsets[member.offset];
  }

  Type* newType = nullptr;
  if(alreadyExists){
    newType = alreadyExists;
  } else {
    newType = newTypeAlloc;
  }

  newType->type = Subtype_TEMPLATED_INSTANCE;
  newType->name = name;
  newType->members = members;
  newType->templateBase = templateBase;
  newType->templateArgTypes = templateArgTypes;

  int maxAlign = 0;
  for(int val : align){
    maxAlign = std::max(val,maxAlign);
  }
  newType->align = maxAlign;

  if(structDefined){
    newType->size = Align(offsets[sizes.size],maxAlign);
  } else {
    newType->size = templateBase->size;
  }

  if(mustFixRecursion){
    for(Member& member : newType->members){
      if(member.type == ValueType::NIL){
        member.type = newType;
      }
    }
  }

  return newType;
}

Type* GetType(String name){
  STACK_ARENA(temp,Kilobyte(4));

  Opt<ParsedType> parsed = ParseType(name,&temp);
  if(!parsed.has_value()){
    return nullptr;
  }
  
  String uniqueName = PushUniqueRepresentation(&temp,parsed.value());
  
  Type* typeExists = GetSpecificType(uniqueName);
  if(typeExists){
    return typeExists;
  }

  //DEBUG_BREAK_IF(CompareString(uniqueName,"Byte*"));
  // TODO: Replace this part with fetching data from ParsedType
  Type* res = nullptr;
  Tokenizer tok(uniqueName,"*&[]<>,",{});
  if(Contains(uniqueName,'<')){
    String baseName = tok.PeekToken();

    Type* templateTypeExists = GetSpecificType(baseName);

    ParsedType onlyTemplate = parsed.value();
    onlyTemplate.arrayExpressions.size = 0;
    onlyTemplate.amountOfPointers = 0;
    String onlyTemplateName = PushUniqueRepresentation(&temp,onlyTemplate);

    Type* onlyTemplateType = GetSpecificType(onlyTemplateName);

    String templatedName = ParseSimpleType(&tok);

    // TODO: This part is a mess because the GetSpecificType does not care about pointers, and we can reach a point where we either have the template type and not the specific part, meaning that we must instantiate it, we can have the instantiated part without the template type (which GetSpecificType does not return because we might also have pointers and whatnot). This portion would really like to be rewritten to use the ParsedType and for things to just happen in sequence, without this clubersome checking that is always being done for special cases. It should be Fetch main name (handle full template or not at this stage, and then progress to pointers and stuff)
    if(onlyTemplateType){
      res = onlyTemplateType;
    } else if(!templateTypeExists){
      return nullptr;
    } else if(CompareString(uniqueName,templatedName)){ // Name equal to templated name means that no pointers or arrays, only templated arguments are not found
      
      // Need to check if the templated name exists, otherwise we cannot go through
      Type* res = InstantiateTemplate(uniqueName,&permanentArena);
      return res;
    } else if(templatedName.size > 0){
      res = GetType(templatedName);
    }
  } else {
    String noPointers = ParseSimpleType(&tok);
    if(CompareString(noPointers,uniqueName)){
      return nullptr;
    }
    res = GetType(noPointers);
  }

  if(res == nullptr){
    return nullptr;
  }

  while(!tok.Done()){
    Token next = tok.NextToken();

    if(CompareString(next,"*")){
      bool found = false;
      for(Type* type : types){
        if(type->type == Subtype_POINTER && type->pointerType == res){
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
      Token arrayExpression = tok.PeekFindUntil("]").value();
      tok.AdvancePeekBad(arrayExpression);
      tok.AssertNextToken("]");

      if(!CheckFormat("%d",arrayExpression)){
        DEBUG_BREAK();
      }

      int arraySize = ParseInt(arrayExpression);
      bool found = false;
      for(Type* type : types){
        if(type->type == Subtype_ARRAY && type->arrayType == res && type->size == (arraySize * res->size)){
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

    NOT_POSSIBLE("Should have found a type by now");
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
    if(type->type == Subtype_POINTER && type->pointerType == baseType){
      return type;
    }
  }

  Type* ret = types.Alloc();
  ret->name = PushString(&permanentArena,"%.*s*",UNPACK_SS(baseType->name));
  ret->type = Subtype_POINTER;
  ret->pointerType = baseType;
  ret->size = sizeof(void*);
  ret->align = alignof(void*);

  return ret;
}

Type* GetArrayType(Type* baseType, int arrayLength){
  for(Type* type : types){
    if(type->type == Subtype_ARRAY && type->arrayType == baseType && ArrayLength(type) == arrayLength){
      return type;
    }
  }

  Type* ret = types.Alloc();
  ret->name = PushString(&permanentArena,"%.*s[%d]",UNPACK_SS(baseType->name),arrayLength);
  ret->type = Subtype_ARRAY;
  ret->pointerType = baseType;
  ret->size = arrayLength * baseType->size;
  ret->align = baseType->align;

  return ret;
}

void RegisterParsedTypes();

#define REGISTER(TYPE) RegisterSimpleType(STRING(#TYPE),sizeof(TYPE),alignof(TYPE))
#define REGISTER_TYPEDEF(OLD,NEW) \
  RegisterOpaqueType(STRING(#NEW),Subtype_TYPEDEF,sizeof(NEW),alignof(NEW)); \
  RegisterTypedef(STRING(#OLD),STRING(#NEW))

void RegisterTypes(){
  static bool registered = false;
  if(registered){
    return;
  }
  registered = true;

  permanentArena = InitArena(Kilobyte(64));

  ValueType::NUMBER = REGISTER(int);
  ValueType::BOOLEAN = REGISTER(bool);
  ValueType::CHAR = REGISTER(char);
  ValueType::STRING = GetPointerType(ValueType::CHAR);
  ValueType::NIL = RegisterSimpleType(STRING("void"),sizeof(char),alignof(char)); // TODO: Change size to void

  ValueType::CONST_STRING = ValueType::STRING;
  //ValueType::CONST_STRING = GetPointerType(REGISTER(const char));

  REGISTER(unsigned char);
  REGISTER(short);
  REGISTER(unsigned short);
  REGISTER(unsigned int);
 
  REGISTER(unsigned long);
  REGISTER(long);
  REGISTER(float);
  REGISTER(double);
  
  RegisterOpaqueType(STRING("size_t"),Subtype_TYPEDEF,sizeof(size_t),alignof(size_t));
  ValueType::SIZE_T = RegisterTypedef(STRING("unsigned long"),STRING("size_t"));
  
  REGISTER_TYPEDEF(unsigned char,uint8_t);
  REGISTER_TYPEDEF(char,int8_t);
  REGISTER_TYPEDEF(unsigned short,uint16_t);
  REGISTER_TYPEDEF(short,int16_t);
  REGISTER_TYPEDEF(unsigned int,uint32_t);
  REGISTER_TYPEDEF(int,int32_t);
  REGISTER_TYPEDEF(unsigned long,uint64_t);
  REGISTER_TYPEDEF(long,int64_t);

  // TODO: Problem here when changing 
  REGISTER_TYPEDEF(long,intptr_t);
  REGISTER_TYPEDEF(unsigned long,uintptr_t);
  
  RegisterParsedTypes();

#ifndef SIMPLE_TYPES
  ValueType::POOL = GetTypeOrFail(STRING("Pool"));
  ValueType::ARRAY = GetTypeOrFail(STRING("Array"));
  ValueType::STD_VECTOR = GetTypeOrFail(STRING("std::vector"));
  ValueType::HASHMAP = GetTypeOrFail(STRING("Hashmap"));
  ValueType::SIZED_STRING = GetTypeOrFail(STRING("String"));
  ValueType::SIZED_STRING_BASE = GetTypeOrFail(STRING("Array<const char>"));

  Type* normalTemplateFunction = GetTypeOrFail(STRING("TemplateFunction"));
  if(normalTemplateFunction){
    ValueType::TEMPLATE_FUNCTION = GetPointerType(normalTemplateFunction);
  }
#endif
}

#undef REGISTER

void FreeTypes(){
  Free(&permanentArena);
  types.Clear(true);
}
 
String GetDefaultValueRepresentation(Value in,Arena* arena){
  Value val = CollapseArrayIntoPtr(in);
  Type* type = val.type;
  String res = {};

  if(CompareString(type->name,"long int")){
    res = PushString(arena,"%ld",val.number);
    return res;
  } else if(type == ValueType::NUMBER){
    res = PushString(arena,"%" PRId64,val.number);
  } else if(type == ValueType::STRING || type == ValueType::CONST_STRING){
    if(val.literal){
      res = PushString(arena,"\"%.*s\"",UNPACK_SS(val.str));
    } else {
      res = PushString(arena,"%.*s",UNPACK_SS(val.str));
    }
  } else if(type == ValueType::CHAR){
    res = PushString(arena,"%c",val.ch);
  } else if(type == ValueType::SIZED_STRING || val.type == ValueType::SIZED_STRING_BASE){
    res = PushString(arena,"%.*s",UNPACK_SS(val.str));
  } else if(type == ValueType::BOOLEAN){
    res = PushString(arena,"%s",val.boolean ? "true" : "false");
  } else if(type == ValueType::SIZE_T){
    res = PushString(arena,"%d",*(int*)val.custom);
  } else if(type == ValueType::NIL){
    res = STRING("Nullptr");
  } else if(type->type == Subtype_POINTER){
    Assert(!val.isTemp);

    void* ptr = *(void**) val.custom;

    if(ptr == nullptr){
      res = PushString(arena,"0x0");
    } else {
      res = PushString(arena,"%p",ptr);
    }
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
    if(type->templateBase == ValueType::ARRAY){
      Opt<Value> size = AccessStruct(val,STRING("size"));
      if(!size){
        LogFatal(LogModule::TYPE,"Not an Array");
      }

      res = PushString(arena,"Array of Size:%" PRId64,size.value().number);
    } else {
      res = PushString(arena,"%.*s",UNPACK_SS(type->name)); // Return type name
    }
  } else if(type->type == Subtype_ENUM){
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
      DEBUG_BREAK();
      res = PushString(arena,"(enum %.*s:%d)",UNPACK_SS(type->name),enumValue);
    }
  } else {
    res = PushString(arena,"\"[GetDefaultValueRepresentation Type:%.*s]\"",UNPACK_SS(type->name)); // Return type name
  }

  return res;
}

Value CollapsePtrIntoStruct(Value object){
  Type* type = object.type;

  while(type->type == Subtype_TYPEDEF){
	type = type->typedefType;
  }

  if(type->type != Subtype_POINTER){
    object.type = type;
    return object;
  }

  // TODO: Temporary removed this and replaced with the if
  //Assert(!object.isTemp);
  if(object.isTemp){
    return object;
  }
  
  char* deference = (char*) object.custom;

  while(type->type == Subtype_POINTER || type->type == Subtype_TYPEDEF){
    if(type->type == Subtype_TYPEDEF){
	  type = type->typedefType;
	  continue;
	}

    if(type->pointerType == ValueType::NIL){
      DEBUG_BREAK();
      break;
    }

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
  } else if(CompareString(val.type->name,STRING("const char*"))){
    val.ch = *((char*) val.custom);
  } else if(val.type == ValueType::NIL){
    val.number = 0;
  } else if(val.type == ValueType::STRING || val.type == ValueType::CONST_STRING){
    char* str = *(char**) val.custom;

    if(str == nullptr){
      val.str = STRING("");
    } else {
      val.str = STRING(str);
    }
  } else if(val.type == ValueType::SIZED_STRING || val.type == ValueType::SIZED_STRING_BASE){
    val.str = *((String*) val.custom);
  } else if(val.type == ValueType::TEMPLATE_FUNCTION){
    val.templateFunction = *((TemplateFunction**) val.custom);
  } else if(val.type->type == Subtype_ENUM){
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

Opt<Value> AccessObjectIndex(Value object,int index){
  Value value = {};

  if(object.type->type == Subtype_ARRAY){
    Type* arrayType = object.type;
    Byte* array = (Byte*) object.custom;

    int arrayIndexSize = (arrayType->size / arrayType->arrayType->size);

    Assert(index < arrayIndexSize);

    void* objectPtr = (void*) (array + index * object.type->arrayType->size);

    value.type = object.type->arrayType;
    value.custom = objectPtr;
  } else if(object.type->type == Subtype_POINTER) { // Ptr
      Assert(!object.isTemp);

      void* objectPtr = DeferencePointer(object.custom,object.type->pointerType,index);

      value.type = object.type->pointerType;
      value.custom = objectPtr;
  } else if(object.type->type == Subtype_TEMPLATED_INSTANCE){
    if(object.type->templateBase == ValueType::ARRAY){
      Array<Byte>* byteArray = (Array<Byte>*) object.custom;

      int arrayLength = byteArray->size;
      if(index >= arrayLength){
        LogError(LogModule::TYPE,"Try to access member past array size");
        return (Opt<Value>{});
      }

      int size = object.type->templateArgTypes[0]->size;
      Byte* view = (Byte*) byteArray->data;

      value.custom = &view[index * size];
      value.type = object.type->templateArgTypes[0];
    } else if(object.type->templateBase == ValueType::POOL){
      Iterator iter = Iterate(object);
      for(int i = 0; HasNext(iter) && i < index; Advance(&iter),i += 1);
      value = GetValue(iter);
    } else if(object.type->templateBase == ValueType::HASHMAP){
      HashmapUnpackedIndex unpacked = UnpackHashmapIndex(index);

      Iterator iter = Iterate(object);
      for(int i = 0; HasNext(iter) && i < unpacked.index; Advance(&iter),i += 1);
      Value pair = GetValue(iter);

      Opt<Value> optVal;
      if(unpacked.data){
        optVal = AccessStruct(pair,STRING("data"));
      } else {
        optVal = AccessStruct(pair,STRING("key"));
      }

      if(!optVal){
        LogFatal(LogModule::TYPE,"Not a Hashmap");
      }
      value = optVal.value();
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  value.isTemp = false;
  value = CollapseValue(value);

  return value;
}

Opt<Value> AccessStruct(Value structure,Member* member){
  Assert(IsStruct(structure.type));
  //Assert(!structure.isTemp);
  
  char* view = (char*) structure.custom;

  if(structure.isTemp){
    view = (char*) &structure.str;
  }

  void* newObject = &view[member->offset];

  Value newValue = {};
  newValue.custom = newObject;
  newValue.type = member->type;
  newValue.isTemp = false;

  newValue = CollapseValue(newValue);

  return newValue;
}

Opt<Value> AccessStruct(Value val,int index){
  Assert(IsStruct(val.type));

  Member* member = &val.type->members[index];
  Opt<Value> res = AccessStruct(val,member);

  return res;
}

Opt<Value> AccessStruct(Value object,String memberName){
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
    LogError(LogModule::TYPE,"Failure to find member named: %.*s on structure: %.*s",UNPACK_SS(memberName),UNPACK_SS(structure.type->name));
    return Opt<Value>();
  }

  Opt<Value> res = AccessStruct(structure,member);

  return res;
}

static Type* GetBaseTypeOnce(Type* type){
  // For now only recurse on arrays and pointers
  switch(type->type){
  case Subtype_ARRAY:{
    Type* base = type->arrayType;
    return base;
  } break;
  case Subtype_POINTER:{
    return type->pointerType;
  } break;
  case Subtype_TYPEDEF:{
    return type->typedefType;
  } break;
  case Subtype_UNKNOWN:;
  case Subtype_BASE:;
  case Subtype_STRUCT:;
  case Subtype_TEMPLATED_STRUCT_DEF:;
  case Subtype_TEMPLATED_INSTANCE:;
  case Subtype_ENUM:;
  }
  
  return type;
}


static Type* GetBaseType(Type* type){
  // For now only recurse on arrays and pointers
  switch(type->type){
  case Subtype_ARRAY:{
    Type* base = GetBaseType(type->arrayType);
    return base;
  } break;
  case Subtype_POINTER:{
    return GetBaseType(type->pointerType);
  } break;
  case Subtype_TYPEDEF:{
    return GetBaseType(type->typedefType);
  } break;
  case Subtype_UNKNOWN:;
  case Subtype_BASE:;
  case Subtype_STRUCT:;
  case Subtype_TEMPLATED_STRUCT_DEF:;
  case Subtype_TEMPLATED_INSTANCE:;
  case Subtype_ENUM:;
  }
  
  return type;
}

Opt<Member*> GetMember(Type* structType,String memberName){
  Type* type = GetBaseType(structType);

  Assert(type->type == Subtype_STRUCT || type->type == Subtype_TEMPLATED_INSTANCE);
  
  for(Member& m : type->members){
    if(CompareString(m.name,memberName)){
      return &m;
    }
  }

  return {};
}

Array<String> GetStructMembersName(Type* structType,Arena* out){
  Assert(structType->type == Subtype_STRUCT || structType->type == Subtype_TEMPLATED_INSTANCE);
  
  Array<String> names = PushArray<String>(out,structType->members.size);
  int index = 0;
  for(Member& m : structType->members){
    names[index++] = m.name;
  }

  return names;
}

void PrintStructDefinition(Type* type){
  Type* collapsed = CollapseTypeUntilBase(type);
  Assert(IsStruct(collapsed));

  printf("Struct %.*s has the following members:\n",UNPACK_SS(type->name));
  for(Member& m : collapsed->members){
	printf("\t%.*s\n",UNPACK_SS(m.name));
  }
}

void* GetValueAddress(Value val){
  if(val.isTemp){
    return val.custom; // Only works for pointers, I think. The isTemp should be reworked
  } else {
    void* address = *(void**) val.custom;
    return address;
  }
}

int ArrayLength(Type* type){
  Assert(type->type == Subtype_ARRAY);

  int arraySize = type->size;
  int baseTypeSize = type->arrayType->size;

  int res = (arraySize / baseTypeSize);

  return res;
}

int IndexableSize(Value object){
  Type* type = object.type;

  Assert(IsIndexable(type));

  int size = 0;

  if(type->type == Subtype_ARRAY){
    size = ArrayLength(type);
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
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
      NOT_IMPLEMENTED("Implement as needed");
    }
  }

  return size;
}

bool IsIndexable(Type* type){
  bool res = false;

  if(type->type == Subtype_POINTER){
    res = true;
  }if(type->type == Subtype_ARRAY){
    res = true;
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
    if(IsStringLike(type)){
      res = false;
    } else if(type->templateBase == ValueType::POOL){
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
              type == ValueType::CONST_STRING ||
              type == ValueType::SIZED_STRING ||
              type == ValueType::SIZED_STRING_BASE);

  return res;
}

bool IsIndexableOfBasicType(Type* type){
  Assert(IsIndexable(type));

  bool res = false;
  if(type->type == Subtype_ARRAY){
    res = IsBasicType(type->arrayType);
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
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

      if(memberType->type == Subtype_POINTER && memberType->pointerType == type){
        res = true;
      }

      break;
    }
  }

  return res;
}

bool IsStruct(Type* type){
  bool res = type->type == Subtype_STRUCT || type->type == Subtype_TEMPLATED_INSTANCE;
  return res;
}

bool IsStringLike(Type* type){
  bool res = (type == ValueType::STRING ||
              type == ValueType::CONST_STRING ||
              type == ValueType::SIZED_STRING ||
              type == ValueType::SIZED_STRING_BASE);

  return res;
}

int HashmapIndex(int index,bool data){
  uint res = index;
  if(data){
    res = SET_BIT(res,31);
  }
  return res;
}

HashmapUnpackedIndex UnpackHashmapIndex(int index){
  HashmapUnpackedIndex res = {};

  res.data = GET_BIT(index,31);
  res.index = CLEAR_BIT(index,31);

  return res;
}

Type* GetBaseTypeOfIterating(Type* iterating){
  if(iterating->type == Subtype_BASE){ // TODO: Superfluous being a if based on the code below (not changed because can't test change currently);
    return iterating;
  }

  Assert(IsIndexable(iterating)); // should suffice as a check
  Type* type = GetBaseTypeOnce(iterating);
  
  switch(type->type){
  case Subtype_TEMPLATED_INSTANCE:{
    // TODO: Technically should check if the types are iterating and recurse
    //       on GetBaseTypeOfIterating instead of calling GetBaseType. Do now yet know if need to.
    if(type->templateBase == ValueType::ARRAY){
      return GetBaseType(type->templateArgTypes[0]);
    } else if(type->templateBase == ValueType::POOL){
      return GetBaseType(type = type->templateArgTypes[0]);
    } else if(type->templateBase == ValueType::HASHMAP){
      Type* keyType = type->templateArgTypes[0];
      Type* dataType = type->templateArgTypes[1];

      STACK_ARENA(temp,256);
      String pairName = PushString(&temp,"Pair<%.*s,%.*s>",UNPACK_SS(keyType->name),UNPACK_SS(dataType->name));

      Type* exists = GetSpecificType(pairName);
      if(!exists){
        String permanentName = PushString(&permanentArena,pairName);
        exists = GetType(permanentName);
      }
      return GetBaseType(exists);
    }
  } break;
  case Subtype_POINTER: break;
  case Subtype_UNKNOWN:;
  case Subtype_BASE:;
  case Subtype_STRUCT:;
  case Subtype_TEMPLATED_STRUCT_DEF:;
  case Subtype_ENUM:;
  case Subtype_ARRAY:;
  case Subtype_TYPEDEF:;
    NOT_IMPLEMENTED("Some of these could be but do not currently needing it");
  }
  return type;
}

bool IsIterable(Type* type){
  bool res = false;

  if(type->type == Subtype_POINTER){
    res = false;
  } else if(type->type == Subtype_ARRAY){
    res = true;
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
    if(IsStringLike(type)){
      res = false;
    } else if(type->templateBase == ValueType::POOL){
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

Iterator Iterate(Value iterating){
  iterating = RemoveTypedefIndirection(CollapsePtrIntoStruct(iterating));

  Type* type = iterating.type;

  Iterator iter = {};
  iter.iterating = iterating;

  if(type == ValueType::NUMBER){
    iter.currentNumber = 0;
  } else if(type->type == Subtype_ARRAY){
    iter.currentNumber = 0;
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
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
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(IsEmbeddedListKind(type)){
    // Info stored in iterating
  } else if(type == ValueType::NIL){
    // Do nothing. HasNext will fail
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return iter;
}

bool HasNext(Iterator iter){
  Type* type = iter.iterating.type;

  if(type == ValueType::NUMBER){
    int len = iter.iterating.number;

    bool res = (iter.currentNumber < len);
    return res;
  } else if(type->type == Subtype_ARRAY){
    int len = ArrayLength(type);

    bool res = (iter.currentNumber < len);
    return res;
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
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
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(IsEmbeddedListKind(type)){
    bool res = (iter.iterating.custom != nullptr);

    return res;
  } else if(type == ValueType::NIL){
    return false;
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return false;
}

void Advance(Iterator* iter){
  Type* type = iter->iterating.type;

  if(type == ValueType::NUMBER){
    iter->currentNumber += 1;
  } else if(type->type == Subtype_ARRAY){
    iter->currentNumber += 1;
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
    if(type->templateBase == ValueType::POOL){
      ++iter->poolIterator;
    } else if(type->templateBase == ValueType::STD_VECTOR){
      iter->currentNumber += 1;
    } else if(type->templateBase == ValueType::ARRAY){
      iter->currentNumber += 1;
    } else if(type->templateBase == ValueType::HASHMAP){
      iter->currentNumber += 1;
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(IsEmbeddedListKind(type)){
    Opt<Value> optVal = AccessStruct(iter->iterating,STRING("next"));
    if(!optVal){
      LogFatal(LogModule::TYPE,"No next member. Somehow returned a different type");
    }

    Value collapsed = CollapsePtrIntoStruct(optVal.value());
    iter->iterating = collapsed;
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }
}

Value GetValue(Iterator iter){
  Value val = {};

  Type* type = iter.iterating.type;

  if(type == ValueType::NUMBER){
    val = MakeValue(iter.currentNumber);
  } else if(type->type == Subtype_ARRAY){
    Opt<Value> optVal = AccessObjectIndex(iter.iterating,iter.currentNumber);
    if(!optVal){
      LogFatal(LogModule::TYPE,"Somehow went past the end? Should not be possible");
    }
    val = optVal.value();
  } else if(type->type == Subtype_TEMPLATED_INSTANCE){
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
      Opt<Value> optVal = AccessObjectIndex(iter.iterating,iter.currentNumber);
      if(!optVal){
        LogFatal(LogModule::TYPE,"Somehow went past the end? Should not be possible");
      }
      val = optVal.value();
    } else if(type->templateBase == ValueType::HASHMAP){
      Hashmap<Byte,Byte>* view = (Hashmap<Byte,Byte>*) iter.iterating.custom;

      Byte* data = (Byte*) view->data;

      val.custom = &data[iter.currentNumber * iter.hashmapType->size];
      val.type = iter.hashmapType;
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(IsEmbeddedListKind(type)){
    return iter.iterating;
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return RemoveTypedefIndirection(val);
}

static bool IsPointerLike(Type* type){
  bool res = type->type == Subtype_POINTER || type->type == Subtype_ARRAY;
  return res;
}

Value RemoveOnePointerIndirection(Value in){
  Assert(IsPointerLike(in.type));

  Value res = in;

  if(in.type->type == Subtype_ARRAY){
    res = CollapseArrayIntoPtr(in);
  }

  // Remove one level of indirection
  Assert(in.type->type == Subtype_POINTER);
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

Value RemoveTypedefIndirection(Value in){
  while(in.type->type == Subtype_TYPEDEF){
    in.type = in.type->typedefType;
  }
  return in;
}

Type* RemoveTypedefIndirection(Type* in){
  while(in->type == Subtype_TYPEDEF){
    in = in->typedefType;
  }
  return in;
}

static bool IsTypeBaseTypeOfPointer(Type* baseToCheck,Type* pointerLikeType){
  Type* checker = pointerLikeType;
  while(IsPointerLike(checker)){
    if(baseToCheck == checker){
      return true;
    } else if(checker->type == Subtype_ARRAY){
      checker = GetPointerType(checker->arrayType);
    } else if(checker->type == Subtype_POINTER){
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
    if(res.type->type == Subtype_ARRAY){
      res = CollapseArrayIntoPtr(res);
    } else if(res.type->type == Subtype_POINTER){
      res = RemoveOnePointerIndirection(res);
    } else if(res.type->type == Subtype_TYPEDEF){
	  res.type = res.type->typedefType;
	} else {
      NOT_POSSIBLE("No more possible type exists to collapse, I think");
    }
  }

  Assert(res.type == typeWanted);

  return res;
}

bool Equal(Value v1,Value v2){
  Value c1 = CollapseArrayIntoPtr(v1);
  Value c2 = CollapseArrayIntoPtr(v2);

  if((c1.type == ValueType::STRING || c1.type == ValueType::CONST_STRING) && c2.type == ValueType::SIZED_STRING){
    String str = c1.str;
    String ss = c2.str;
    Assert(c1.isTemp == true && c2.isTemp == true);

    return CompareString(ss,str);
  } else if(c1.type == ValueType::SIZED_STRING && (c2.type == ValueType::STRING || c2.type == ValueType::CONST_STRING)){
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
    if(IsBasicType(c2.type)){
      c1 = ConvertValue(c1,c2.type,nullptr);
    } else {
      c2 = ConvertValue(c2,c1.type,nullptr);
    }
  }

  bool res = false;

  if(c1.type == ValueType::NUMBER){
    res = (c1.number == c2.number);
  } else if(c1.type == ValueType::STRING || c1.type == ValueType::CONST_STRING){
    res = (CompareString(c1.str,c2.str));
  } else if(c1.type == ValueType::SIZED_STRING){
    res = CompareString(c1.str,c2.str);
  } else if(c1.type->type == Subtype_ENUM){
    if(c1.type != c2.type){
      res = false;
    } else {
      res = (c1.number == c2.number);
    }
  } else if(c1.type->type == Subtype_POINTER){
    Assert(!c1.isTemp && !c2.isTemp); // TODO: The concept of temp variables should be better handled. For now, just check that we are dealing with the must common case and proceed

    c1 = RemoveOnePointerIndirection(c1);
    c2 = RemoveOnePointerIndirection(c2);

    res = (c1.custom == c2.custom);
  } else if(IsStruct(c1.type)){
    Assert(!c1.isTemp);
    Assert(!c2.isTemp);

    res = (c1.custom == c2.custom);
  } else {
    NOT_IMPLEMENTED("Implement as needed");
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
  if(in.type->type != Subtype_ARRAY){
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
    if(CompareString(in.type->name,"Opt<int>")){
      Opt<int>* view = (Opt<int>*) in.custom;
      res.boolean = view->has_value();
    } else if(in.type == ValueType::NUMBER || in.type->type == Subtype_ENUM){
      res.boolean = (in.number != 0);
    } else if(in.type->type == Subtype_POINTER){
      int* pointerValue = (int*) DeferencePointer(in.custom,in.type->pointerType,0);

      res.boolean = (pointerValue != nullptr);
    } else if(in.type->type == Subtype_TEMPLATED_INSTANCE && in.type->templateBase == ValueType::ARRAY){
      int size = IndexableSize(in);

      res.boolean = (size != 0);
    } else if(IsStruct(in.type)){
      res.boolean = (in.custom != nullptr);
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(want == ValueType::NUMBER){
    if(in.type->type == Subtype_POINTER){
      void* pointerValue = (void*) DeferencePointer(in.custom,in.type->pointerType,0);

      res.number = (i64) pointerValue;
    } else if(CompareString(in.type->name,"iptr")) { // TODO: could be handled by the typedef and setting all values from cstdint as knows
         iptr* data = (iptr*) in.custom;
         res.number = (i64) *data;
    } else if(CompareString(in.type->name,"Opt<int>")){
      Opt<int>* view = (Opt<int>*) in.custom;
      res.number = view->value_or(0);
    } else if(in.type->type == Subtype_ENUM){
      res.number = in.number;
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(want == ValueType::SIZED_STRING){
    if(in.type == ValueType::STRING || in.type == ValueType::CONST_STRING){
      res = in;
      res.type = want;
    } else if(arena){
      res.str = GetDefaultValueRepresentation(in,arena);
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else if(want->type == Subtype_ENUM){
    if(in.type == ValueType::NUMBER){
      res = in;
      res.type = want;
    } else {
      NOT_IMPLEMENTED("Implement as needed");
    }
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return res;
}

Value MakeValue(){
  Value val = {};
  val.type = ValueType::NIL;
  val.isTemp = true;
  return val;
}

Value MakeValue(i64 integer){
  Value val = {};
  val.number = integer;
  val.type = ValueType::NUMBER;
  val.isTemp = true;
  return val;
}

Value MakeValue(unsigned int integer){
  Value val = {};
  val.number = (u64) integer; // Should probably maintain the unsigned information. TODO
  val.type = ValueType::NUMBER;
  val.isTemp = true;
  return val;
}

Value MakeValue(int integer){
  Value val = {};
  val.number = (i64) integer;
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

Value MakeValue(void* entity,Type* type){
  Value val = {};

  val.type = type;
  val.custom = entity;
  val.isTemp = false;

  return val;
}

Value MakeValue(void* entity,String typeName){
  Value val = {};

  val.type = GetType(typeName);
  val.custom = entity;
  val.isTemp = false;

  return val;
}

// This shouldn't be here, but cannot be on parser.cpp because otherwise struct parser would fail
Array<Value> ExtractValues(const char* format,String tok,Arena* arena){
  // TODO: This is pretty much a copy of CheckFormat but with small modifications
  // There should be a way to refactor into a single function, and probably less error prone
  if(!CheckFormat(format,tok)){
    return {};
  }

  DynamicArray<Value> arr = StartArray<Value>(arena);

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

        Value* val = arr.PushElem();
        *val = {};
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

        Value* val = arr.PushElem();
        *val = {};
        val->type = ValueType::STRING;
        val->str = str;
        val->isTemp = true;
      }break;
      case '\0':{
        NOT_POSSIBLE("Format char not finished"); // TODO: Probably should be a error that reports 
      }break;
      default:{
        NOT_IMPLEMENTED("Implement as needed");
      }break;
      }
    } else {
      Assert(formatChar == tok[tokenIndex]);
      formatIndex += 1;
      tokenIndex += 1;
    }
  }

  Array<Value> values = EndArray(arr);

  return values;
}

// Very dependent on compiler and enviromnent.
// Probably need to at least insert a few more asserts to make it sure that no weird bug escapes from this function in case of any environment change
String ExtractTypeNameFromPrettyFunction(String prettyFunctionFormat,Arena* out){
  Array<Value> values = ExtractValues("String GetTemplateTypeName(Arena*) [with T = %s; String = Array<const char>]",prettyFunctionFormat,out);

  Assert(values.size == 1);

  return values[0].str;
}

String PushUniqueRepresentation(Arena* out,ParsedType type){
  auto builder = StartString(out);

  PushString(out,type.baseName);
  if(type.templateMembers.size){
    PushString(out,"<");
    bool first = true;
    for(ParsedType t : type.templateMembers){
      if(first){
        first = false;
      } else {
        PushString(out,",");
      }
      PushUniqueRepresentation(out,t);
    }
    PushString(out,">");
  }

  for(int i = 0; i < type.amountOfPointers; i++){
    PushString(out,STRING("*"));
  }
  for(String expr : type.arrayExpressions){
    PushString(out,"[%.*s]",UNPACK_SS(expr));
  }
  
  return EndString(builder);
}

const char* specialCharacters = "*&[]<>,";

struct ModifierCount{
  int signedCount;
  int unsignedCount;
  int shortCount;
  int longCount;
};

String GetUniqueRepresentation(String type,ModifierCount modifiers){
  bool isUnsigned = modifiers.unsignedCount > 0;
  bool isSigned = modifiers.signedCount > 0 || modifiers.unsignedCount == 0;
  
  if(modifiers.shortCount > 0){
    Assert(modifiers.longCount == 0);
  }
  if(modifiers.longCount > 0){
    Assert(modifiers.shortCount == 0);
  }
  if(isSigned){
    Assert(!isUnsigned);
  }
  if(isUnsigned){
    Assert(!isSigned);
  }
  
  if(CompareString(type,"char")){
    Assert(modifiers.longCount == 0);
    Assert(modifiers.shortCount == 0);
    if(isUnsigned){
      return STRING("unsigned char");
    } else {
      return STRING("char");
    }
  }
  
  if(type.size == 0 || CompareString(type,"int")){
    if(modifiers.shortCount == 1){
      if(isSigned){
        return STRING("short int");
      } else {
        return STRING("unsigned short int");
      }
    }
    else if(modifiers.longCount == 1){
      if(isSigned){
        return STRING("long int");
      } else {
        return STRING("unsigned long int");
      }
    }
    else if(modifiers.longCount == 2){
      if(isSigned){
        return STRING("long long int");
      } else {
        return STRING("unsigned long  long int");
      }
    } else if(modifiers.shortCount == 0 && modifiers.longCount == 0){
      if(isSigned){
        return STRING("int");
      } else{
        return STRING("unsigned int");
      }
    } else {
      Assert(false); // Not possible
    }
  }

  Assert(false); // Must be char, blank or int, ignoring long double and booleans for now
  return {};
}

Opt<String> ParseBaseName(Tokenizer* tok,Arena* out){
  bool containsModifier = false;
  ModifierCount count = {};
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    // TODO: Should we keep const? GDB does differenciate between char and const char*.
    //       As well as volatile. It does not change the type semantics for our purposes, so IDK. Maybe handle it when we need to.
    if(CompareString(peek,"volatile") ||
       CompareString(peek,"static") ||
       CompareString(peek,"const")){
      // Do nothing
    } else if(CompareString(peek,"signed") ||
       CompareString(peek,"unsigned") ||
       CompareString(peek,"short") ||
       CompareString(peek,"long")){

      if(CompareString(peek,"signed")){
        count.signedCount += 1;
      }
      if(CompareString(peek,"unsigned")){
        count.unsignedCount += 1;
      }
      if(CompareString(peek,"short")){
        count.shortCount += 1;
      }
      if(CompareString(peek,"long")){
        count.longCount += 1;
      }
      
      containsModifier = true;
    } else if(peek.size == 1 && Contains(STRING(specialCharacters),peek[0])){
      if(containsModifier){
        return GetUniqueRepresentation({},count);
      } else {
        return {};
      }
    } else {
      // Assuming that it is a typeName. Would be better to make sure by having a function that checks that it contains only the characters that we expect.

      tok->AdvancePeek();

      if(containsModifier){
        return GetUniqueRepresentation(peek,count);
      } else {
        return PushString(out,peek);
      }
    }
    
    tok->AdvancePeek();
  }

  if(containsModifier){
    return GetUniqueRepresentation({},count);
  }
  
  return {};
}

Opt<ParsedType> ParseType(Tokenizer* tok,Arena* out);

Opt<NameAndTemplateArguments> ParseNameAndTemplateArguments(Tokenizer* tok,Arena* out){
  // TODO - Parse the arguments and templates. Use this function to then later take into account namespaces (add :: to the special characters and just replace the ParseType name and template with a loop that takes into account namespaces, if they exist.
  // NOTE - Like modifiers, do not let this info propagate out, just create the unique representation and save the string.
  STACK_ARENA(temp,Kilobyte(4));
  
  NameAndTemplateArguments result = {};
  
  Opt<String> baseName = ParseBaseName(tok,out);
  PROPAGATE(baseName);

  result.baseName = baseName.value();
  
  Token peek = tok->PeekToken();
  if(CompareString(peek,"<")){
    BLOCK_REGION(&temp);

    tok->AdvancePeek();

    ArenaList<ParsedType>* l = PushArenaList<ParsedType>(&temp);
    while(1){
      if(tok->Done()){
        return {};
      }
  
      Opt<ParsedType> templateType = ParseType(tok,out);
      PROPAGATE(templateType);

      *PushListElement(l) = templateType.value();
      
      if(tok->Done()){
        return {};
      }

      Token decider = tok->NextToken();

      if(CompareString(decider,">")){
        break;
      } else if(CompareString(decider,",")){
        continue;
        // Do nothing
      } else {
        DEBUG_BREAK(); // Should not be possible
      }
    }

    result.templateMembers = PushArrayFromList<ParsedType>(out,l);
  }

  return result;
}

String PushUniqueRepresentation(Arena* out,NameAndTemplateArguments named){
  auto builder = StartString(out);
  PushString(out,named.baseName);
  if(named.templateMembers.size){
    PushString(out,"<");
    bool first = true;
    for(ParsedType t : named.templateMembers){
      if(first){
        first = false;
      } else {
        PushString(out,",");
      }
      PushUniqueRepresentation(out,t);
    }
    PushString(out,">");
  }

  return EndString(builder);
}

Opt<ParsedType> ParseType(Tokenizer* tok,Arena* out){
  STACK_ARENA(temp,Kilobyte(4));
  
  ParsedType result = {};
  
  // Handles namespaces and template arguments. We do not propagate this information, we use it to build a unique representation string (that takes into account whitespaces and whatnot). 
  Opt<NameAndTemplateArguments> first = ParseNameAndTemplateArguments(tok,out);
  PROPAGATE(first);

  Token peek = tok->PeekToken();
  if(CompareString(peek,"::")){
    ArenaList<NameAndTemplateArguments>* list = PushArenaList<NameAndTemplateArguments>(&temp);
    while(CompareString(peek,"::")){
      tok->AdvancePeek();
      Opt<NameAndTemplateArguments> more = ParseNameAndTemplateArguments(tok,out);
      PROPAGATE(more);

      *list->PushElem() = more.value();
    
      peek = tok->PeekToken();
    }
    
    auto builder = StartString(out);
    PushUniqueRepresentation(out,first.value());

    for(auto p = list->head; p; p = p->next){
      PushString(out,STRING("::"));
      PushUniqueRepresentation(out,p->elem);
    }

    result.baseName = EndString(builder);
  } else {
    result.baseName = PushUniqueRepresentation(out,first.value());
  }
  Token possibleConst = tok->PeekToken();
  if(CompareString(possibleConst,"const")){
    tok->AdvancePeek();
  }
  
  // NOTE: This is not very good, mixing pointers and arrays in a weird way will probably cause this code to not work. Fix it when we get some examples of the ways in which this breaks
  int pointerCount = 0;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"*")){
      pointerCount += 1;
      tok->AdvancePeek();
      continue;
    }

    break;
  }
  result.amountOfPointers = pointerCount;

  auto builder = PushArenaList<String>(&temp);
  int arrayCount = 0;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"[")){
      arrayCount += 1;
      tok->AdvancePeek();
      Opt<Token> content = tok->NextFindUntil("]");
      if(content.has_value()){
        *builder->PushElem() = content.value();
      } else {
        *builder->PushElem() = STRING("");
      }
      
      tok->AssertNextToken("]");
      continue;
    }

    break;
  }
  result.arrayExpressions = PushArrayFromList(out,builder);
  
  return result;
}

Opt<ParsedType> ParseType(String typeStr,Arena* out){
  //DEBUG_BREAK_IF(CompareString(typeStr,"int"));
  //DEBUG_BREAK_IF(CompareString(typeStr,"char *"));
  Tokenizer tok(typeStr,specialCharacters,{"::"});
  auto res = ParseType(&tok,out);
  Assert(tok.Done());
  
  return res;
}









