#include <cstdio>

#include "debug.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "CEmitter.hpp"

#include <dirent.h>

// TODO: We might wnat to be able to generate different source files in order to take advantage of the compilation of objects. No point recompiling stuff everytime we only want to change a single enum or something like that. This is only important if the time taken to compile the sources starts to become problematic.

// TODO: We are trying to be "clever" with the fact that we only parse stuff and then let outside code act on it, but I think that I just want to make stuff as simple as possible. If the parser sees a enum it immediatly registers it and the parsing code can immediatly check if a type already exists or not. I do not see the need to make this code respect lexer/parser/compiler boundaries because we immediatly fail and force the programmer to change if any parsing problem occurs. No need to synchronize or keep parsing or anything like that, just make sure that we fail fast.

// TODO: Support numbers inside tables. Support more levels of data hierarchies.
//       More importantly, implement stuff as they are needed. No point in trying to push for anything complex right now.

// TODO: Implement special code for bit flags (needed currently by the AddressGen code).
//       Do not know yet if I want them to be enum, need to test how easy C++ lets enum bit flags work.
//       If C++ is giving trouble, generate the code needed to support easy bit operations.

// TODO: Embed file could just become part of the embed data approach. No point in having two different approaches. Altought the embed data approach would need to become more generic
//       in order to do what the embed file approach does in a data oriented manner.

// TODO: We are currently doing everything manually in terms of instantiating C auxiliary arrays and that kinda sucks. I should be able to make it more automatic, I should be able to generalize it somewhat.

void ReportError(String content,Token faultyToken,String error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("\n");
  printf("%.*s\n",UN(faultyToken));
  printf("%.*s\n",UN(error));
  printf("%.*s\n",UN(loc));
  printf("\n");

  DEBUG_BREAK_OR_EXIT();
}

void ReportError(Tokenizer* tok,Token faultyToken,String error){
  String content = tok->GetContent();
  ReportError(content,faultyToken,error);
}

static bool IsValidIdentifier(String str){
  auto CheckSingle = [](char ch){
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_');
  };

  if(str.size <= 0){
    return false;
  }

  if(!CheckSingle(str.data[0])){
    return false;
  }

  for(int i = 1; i < str.size; i++){
    char ch = str.data[i];
    if(!CheckSingle(ch) && !(ch >= '0' && ch <= '9')){
      return false;
    }
  }

  return true;
}

#define CheckIdentifier(ID) \
  if(!IsValidIdentifier(ID)){ \
    ReportError(tok,ID,StaticFormat("identifier '%.*s' is not a valid name",UN(ID))); \
    return {}; \
  }

bool _ExpectError(Tokenizer* tok,String expected){
  TEMP_REGION(temp,nullptr);

  Token got = tok->NextToken();
  if(!CompareString(got,expected)){
    
    auto builder = StartString(temp);
    builder->PushString("Parser Error.\n Expected to find:  '");
    builder->PushString(PushEscapedString(temp,expected,' '));
    builder->PushString("'\n  Got:");
    builder->PushString(PushEscapedString(temp,got,' '));
    builder->PushString("\n");
    String text = EndString(temp,builder);
    ReportError(tok,got,StaticFormat("%*s",UN(text))); \
    return true;
  }
  return false;
}

#define AssertToken(TOKENIZER,STR) \
  do{ \
    if(_ExpectError(TOKENIZER,STR)){ \
      return {}; \
    } \
  } while(0)

struct EnumDef{
  Token name;
  Array<Pair<String,String>> valuesNamesWithValuesIfExist;
};

struct StructDef{
  Token name;
  Array<Pair<String,String>> typeAndName;
};

struct TypeDef{
  Token name;
  bool isArray;
};

enum DataValueType{
  DataValueType_SINGLE,
  DataValueType_ARRAY,
  DataValueType_FUNCTION
};

struct FunctionValue{
  Token name;
  Array<Token> parameters;
};

struct DataValue{
  DataValueType type;
  union{
    String asStr;
    Array<DataValue*> asArray;
    FunctionValue asFunc;
  };
};

struct ArrayDef{
  Token name;
  Token type;
  DataValue* val;
};

struct Parameter{
  TypeDef* type;
  Token name;
};

struct TableDef{
  String structTypename;
  Array<Parameter> parameterList;
  Array<Array<DataValue*>> dataTable;
};

struct MapDef{
  String name;
  Array<Parameter> parameterList;
  Array<Array<DataValue*>> dataTable;
  bool isDefineMap;
  bool isEnumMap; // One parameter is an enum, the other is not.
  bool isBijection;
};

struct FileDef{
  String name;
  String filepathFromRoot;
};

struct FileGroupDef{
  String name;
  String commonFolder;
  Array<String> foldersFromRoot;
};

struct Defs{
  Array<EnumDef*> enums;
  Array<StructDef*> structs;
  Array<TableDef*> tables;
  Array<MapDef*> maps;
  Array<FileDef*> files;
};

enum DefType{
  DefType_ENUM,
  DefType_STRUCT,
  DefType_TABLE,
  DefType_MAP,
  DefType_FILE
};

struct GenericDef{
  DefType type;

  union{
    void* asGenericPointer;
    EnumDef* asEnum;
    StructDef* asStruct;
    TableDef* asTable;
    MapDef* asMap;
  };
};
    
struct FileGroupInfo{
  String originalRelativePath;
  String filename;
  String commonFolder;
  String contentRawArrayName;
};

Pool<EnumDef> enums = {};
Pool<StructDef> structs = {};
Pool<TableDef> tables = {};
Pool<MapDef> maps = {};
Pool<FileDef> files = {};
Pool<FileGroupDef> fileGroups = {};
Pool<ArrayDef> arrays = {};

// TODO: Weird way of doing things but works for now.
GenericDef GetDefinition(Token name){
  GenericDef res = {};
  for(EnumDef* def : enums){
    if(CompareString(def->name,name)){
      res.type = DefType_ENUM;
      res.asGenericPointer = def;
      return res;
    }
  }
  for(StructDef* def : structs){
    if(CompareString(def->name,name)){
      res.type = DefType_STRUCT;
      res.asGenericPointer = def;
      return res;
    }
  }
  for(TableDef* def : tables){
    if(CompareString(def->structTypename,name)){
      res.type = DefType_TABLE;
      res.asGenericPointer = def;
      return res;
    }
  }
  for(MapDef* def : maps){
    if(CompareString(def->name,name)){
      res.type = DefType_MAP;
      res.asGenericPointer = def;
      return res;
    }
  }
  
  printf("Type '%.*s' does not exist\n",UN(name));
  Assert(false);
  return {};
}

static bool IsEnum(String name){
  for(EnumDef* def : enums){
    if(def->name == name){
      return true;
    }
  }
  return false;
}

EnumDef* ParseEnum(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"enum");

  Token enumTypeName = tok->NextToken();
  CheckIdentifier(enumTypeName);

  AssertToken(tok,"{");

  auto memberList = PushList<Pair<String,String>>(temp);
  while(!tok->Done()){
    Token name = tok->NextToken();
    CheckIdentifier(name);

    String fullValue = {};
    if(tok->IfNextToken("=")){
      auto mark = tok->Mark();

      // TODO: Proper enum parsing otherwise we are going to run into errors constantly.
      //       But need to see what kinda of expressions we are expected to support as well.
      while(!tok->Done() && !tok->IfPeekToken(",") && !tok->IfPeekToken("}")){
        tok->AdvancePeek();
      }

      fullValue = tok->Point(mark);
      fullValue = TrimWhitespaces(fullValue);
    }

    *memberList->PushElem() = {name,fullValue};

    tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }

  AssertToken(tok,"}");
  AssertToken(tok,";");

  EnumDef* res = enums.Alloc();
  res->name = enumTypeName;
  res->valuesNamesWithValuesIfExist = PushArray(out,memberList);

  return res;
}

StructDef* ParseStruct(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"struct");

  Token structTypeName = tok->NextToken();
  CheckIdentifier(structTypeName);

  AssertToken(tok,"{");

  auto memberList = PushList<Pair<String,String>>(temp);
  while(!tok->Done()){
    Token type = tok->NextToken();
    CheckIdentifier(type);
    
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *memberList->PushElem() = {type,name};

    tok->IfNextToken(";");

    if(tok->IfPeekToken("}")){
      break;
    }
  }

  AssertToken(tok,"}");
  AssertToken(tok,";");

  StructDef* res = structs.Alloc();
  res->name = structTypeName;
  res->typeAndName = PushArray(out,memberList);

  return res;
}

TypeDef* ParseTypeDef(Tokenizer* tok,Arena* out){
  Token name = tok->NextToken();
  CheckIdentifier(name);

  bool isArray = false;
  if(tok->IfNextToken("[")){
    AssertToken(tok,"]");
    isArray = true;
  }

  TypeDef* def = PushStruct<TypeDef>(out);
  def->name = name;
  def->isArray = isArray;

  return def;
}

Array<Token> ParseList(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"(");

  auto typeList = PushList<Token>(temp);
  while(!tok->Done()){
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *typeList->PushElem() = name;
    
    if(tok->IfPeekToken(")")){
      break;
    }

    tok->IfNextToken(",");
  }
  AssertToken(tok,")");

  Array<Token> defs = PushArray(out,typeList);
  return defs;
}

Array<Parameter> ParseParameterList(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"(");

  auto typeList = PushList<Parameter>(temp);
  while(!tok->Done()){
    TypeDef* def = ParseTypeDef(tok,out);
    
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *typeList->PushElem() = {def,name};
    
    if(tok->IfPeekToken(")")){
      break;
    }

    tok->IfNextToken(",");
  }
  AssertToken(tok,")");

  Array<Parameter> defs = PushArray(out,typeList);
  return defs;
}

DataValue* ParseValue(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

  if(tok->IfNextToken("@")){
    Token functionName = tok->NextToken();

    Array<Token> parameters = ParseList(tok,out);

    DataValue* val = PushStruct<DataValue>(out);
    val->type = DataValueType_FUNCTION;
    val->asFunc.name = functionName;
    val->asFunc.parameters = parameters;
    return val;
  }
  
  if(!tok->IfNextToken("{")){
    // Parsing a simple value here
    DataValue* val = PushStruct<DataValue>(out);
    val->type = DataValueType_SINGLE;

    Token peek = tok->PeekToken();
    if(peek == "\""){
      tok->AdvancePeek();
      TokenizerMark mark = tok->Mark();

      while(true){
        Token peek = tok->PeekToken();
        if(peek == "\""){
          break;
        }
        tok->AdvancePeek();
      }

      Token fullValue = tok->Point(mark);
      tok->AdvancePeek();

      val->asStr = fullValue;
      return val;
    } else {
      val->asStr = tok->NextToken();
      return val;
    }
  }

  // Parsing an array here.
  auto valueList = PushList<DataValue*>(temp);
  while(!tok->Done()){
    DataValue* val = ParseValue(tok,out);
    *valueList->PushElem() = val;
    
    tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }
  AssertToken(tok,"}");

  DataValue* val = PushStruct<DataValue>(out);
  val->type = DataValueType_ARRAY;
  val->asArray = PushArray(out,valueList);
  return val;
}

Array<Array<DataValue*>> ParseDataTable(Tokenizer* tok,int expectedColumns,Arena* out){
  TEMP_REGION(temp,out);

  auto valueTableList = PushList<Array<DataValue*>>(temp);
  while(!tok->Done()){
    auto valueList = PushList<DataValue*>(temp);
    for(int i = 0; i < expectedColumns; i++){
      DataValue* val = ParseValue(tok,out);
      *valueList->PushElem() = val;

      if(i != expectedColumns - 1){
        AssertToken(tok,":");
      }
    }

    Array<DataValue*> array = PushArray(out,valueList);
    *valueTableList->PushElem() = array;    
    
    tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }

  return PushArray(out,valueTableList);
}

TableDef* ParseTable(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"table");

  Token tableStructName = tok->NextToken();
  CheckIdentifier(tableStructName);

  Array<Parameter> defs = ParseParameterList(tok,out);
  Assert(defs.size > 0);
  
  AssertToken(tok,"{");

  Array<Array<DataValue*>> table = ParseDataTable(tok,defs.size,out);

  AssertToken(tok,"}");
  AssertToken(tok,";");
  
  TableDef* def = tables.Alloc();
  def->structTypename = tableStructName;
  def->parameterList = defs;
  def->dataTable = table;

  return def;
}

FileDef* ParseFile(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"file");

  String fileName = tok->NextToken();

  AssertToken(tok,"=");

  auto mark = tok->Mark();

  while(!tok->Done()){
    if(tok->IfPeekToken(";")){
      break;
    }
    tok->NextToken();
  }

  String filepath = tok->Point(mark);
  AssertToken(tok,";");

  FileDef* def = files.Alloc();
  def->name = fileName;
  def->filepathFromRoot = TrimWhitespaces(filepath);

  return def;
}

FileGroupDef* ParseFileGroup(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"fileGroup");

  TokenizerTemplate* tmpl = CreateTokenizerTemplate(temp,"|=;()",{});
  TOKENIZER_REGION(tok,tmpl);
  
  String groupName = tok->NextToken();

  String commonFolder = {};
  if(tok->IfNextToken("(")){
    commonFolder = tok->NextToken();
    
    AssertToken(tok,")");
  }
  
  AssertToken(tok,"=");

  auto list = PushList<String>(temp);
  while(!tok->Done()){
    if(tok->IfPeekToken(";")){
      break;
    }
    
    Token folderFilepath = tok->NextToken();

    *list->PushElem() = TrimWhitespaces(folderFilepath);
    tok->IfNextToken("|");
  }

  AssertToken(tok,";");

  FileGroupDef* def = fileGroups.Alloc();
  def->name = groupName;
  def->commonFolder = commonFolder;
  def->foldersFromRoot = PushArray(out,list);

  return def;
}

MapDef* ParseMap(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

  Token type = tok->NextToken();
  bool isDefineMap = false;

  if(CompareString(type,"define_map")){
    isDefineMap = true;
  } else if(CompareString(type,"map")){
    isDefineMap = false;
  } else {
    printf("Should not be possible to reach this point\n");
    DEBUG_BREAK_OR_EXIT();
  }

  Token mapName = tok->NextToken();
  CheckIdentifier(mapName);

  if(!isDefineMap){
    Array<Parameter> parameters = ParseParameterList(tok,out);
    Assert(parameters.size > 0);
    
    AssertToken(tok,"{");

    Array<Array<DataValue*>> table = ParseDataTable(tok,parameters.size,out);
    
    AssertToken(tok,"}");
    AssertToken(tok,";");

    MapDef* res = maps.Alloc();
    res->parameterList = parameters;
    res->name = mapName;
    res->dataTable = table;
    res->isDefineMap = isDefineMap;

    if(parameters.size == 2){
      // TODO: This can be more complicated. We can only have true bijection if the normal map and the reverse are both well defined functions. For now assuming that any table of 2 elements is a bijection and be done with it.
      res->isBijection = true;

      if((IsEnum(parameters[0].type->name) && !IsEnum(parameters[1].type->name)) ||
         (IsEnum(parameters[1].type->name) && !IsEnum(parameters[0].type->name))){
        res->isEnumMap = true;
      }
    }
    
    return res;
  } else if(isDefineMap){
    AssertToken(tok,"{");

    Array<Array<DataValue*>> table = ParseDataTable(tok,2,out);

    AssertToken(tok,"}");
    AssertToken(tok,";");

    MapDef* res = maps.Alloc();
    res->name = mapName;
    res->dataTable = table;
    res->isDefineMap = isDefineMap;

    return res;
  }

  NOT_POSSIBLE();
  return nullptr;
}

ArrayDef* ParseArray(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"array");

  Token arrayType = tok->NextToken();
  CheckIdentifier(arrayType);

  Token arrayName = tok->NextToken();
  CheckIdentifier(arrayName);
  
  AssertToken(tok,"=");

  DataValue* val = ParseValue(tok,out);
  Assert2(val->type == DataValueType_ARRAY,"Array cannot handle single values. Pass an array of values using {}");
  
  AssertToken(tok,";");

  ArrayDef* res = arrays.Alloc();
  res->name = arrayName;
  res->type = arrayType;
  res->val = val;

  return res;
}

void ParseContent(String content,Arena* out){
  TEMP_REGION(temp,out);
  Tokenizer tokInst(content,"!@#$%^&*()-+={[}]\\|~`;:'\",./?",{"//","/*","*/"});
  Tokenizer* tok = &tokInst;
  
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"enum")){
      ParseEnum(tok,out);
    } else if(CompareString(peek,"struct")){
      ParseStruct(tok,out);
    } else if(CompareString(peek,"array")){
      ParseArray(tok,out);
    } else if(CompareString(peek,"table")){
      ParseTable(tok,out);
    } else if(CompareString(peek,"file")){
      ParseFile(tok,out);
    } else if(CompareString(peek,"map") || CompareString(peek,"define_map")){
      ParseMap(tok,out);
    } else if(CompareString(peek,"fileGroup")){
      ParseFileGroup(tok,out);
    } else {
      ReportError(tok,peek,"Unexpected token at global scope");
      tok->AdvancePeek();
    }
  }
}

String DefaultRepr(DataValue* val,TypeDef* type,Arena* out){
  if(type && IsEnum(type->name)){
    return val->asStr;
  } else if(val->type == DataValueType_SINGLE){
    return PushString(out,"String(\"%.*s\")",UN(val->asStr));
  }

  NOT_IMPLEMENTED("yet");
  return {};
}

DataValue* EncodeStringAsData(String str,Arena* out){
  DataValue* result = PushStruct<DataValue>(out);

  result->type = DataValueType_SINGLE;
  result->asStr = PushString(out,str);
  return result;
}

DataValue* EncodeArrayAsData(Array<String> strings,Arena* out){
  int size = strings.size;

  Array<DataValue*> individual = PushArray<DataValue*>(out,size);
  for(int i = 0; i < size; i++){
    individual[i] = EncodeStringAsData(strings[i],out);
  }

  DataValue* result = PushStruct<DataValue>(out);
  result->type = DataValueType_ARRAY;
  result->asArray = individual;
  
  return result;
}

static DataValue* EvaluateFunction(FunctionValue func,Arena* out){
  TEMP_REGION(temp,out);
  
  if(CompareString(func.name,"Members")){
    Token type = func.parameters[0];
    
    GenericDef def = GetDefinition(type);

    switch(def.type){
    case DefType_STRUCT: {
      Array<String> names = Extract(def.asStruct->typeAndName,temp,&Pair<String,String>::second);
      
      return EncodeArrayAsData(names,out);
    } break;
    case DefType_MAP:{
      NOT_IMPLEMENTED("yet");
    } break;
    case DefType_ENUM:{
      NOT_IMPLEMENTED("yet");
    } break;
    case DefType_FILE:{
      NOT_IMPLEMENTED("yet");
    } break;
    case DefType_TABLE:{
      NOT_IMPLEMENTED("yet");
    } break;
    }
  } else {
    printf("Did not find a function named %.*s\n",UN(func.name));
  }

  return nullptr;
}

String EscapeCString(String content,Arena* out){
  TEMP_REGION(temp,out);
  StringBuilder* b = StartString(temp);
  for(char ch : content){
    switch(ch){
    case '\n': b->PushString("\\n"); break;
    case '\"': b->PushString("\\\""); break;
    case '\t': b->PushString("\\t"); break;
    case '\\': b->PushString("\\\\"); break;
    case '\r': b->PushString("\\r"); break;
    case '\'': b->PushString("\'"); break;
    default:{
      Assert(ch >= 32 && ch < 127);

      b->PushString("%c",ch);
    } break;
    }
  }

  return EndString(out,b);
}

// 0 - exe name
// 1 - definition file
// 2 - output name
int main(int argc,const char* argv[]){
  if(argc < 3){
    fprintf(stderr,"Need at least 3 arguments: <exe> <defFile> <outputFilename> \n");
    return -1;
  }
 
  // Initialize context arenas
  Arena inst0 = InitArena(Megabyte(64));
  contextArenas[0] = &inst0;
  Arena inst1 = InitArena(Megabyte(64));
  contextArenas[1] = &inst1;

  InitDebug(argv[0]);
  
  const char* defFilePath = argv[1];
  const char* outputPath = argv[2];

  Arena arena3 = InitArena(Megabyte(64));
  
  TEMP_REGION(permanent,nullptr);
  TEMP_REGION(temp,permanent);

  for(int i = 0; i < 8; i++){
    singleUseCasesArenas[i] = InitArena(Megabyte(1));
  }

  String content = PushFile(temp,defFilePath);

  ParseContent(content,permanent);
  
  String headerName = PushString(temp,"%s.hpp",outputPath);
  String sourceName = PushString(temp,"%s.cpp",outputPath);

  // TODO: Because we are implementing more logic, we probably want to have two steps, specially because we have things like FileGroups and so on.
  if(argc == 4){
    if(CompareString(argv[3],"-d")){
      // Generate the .d file.

      String dFileName = PushString(temp,"%s.d",outputPath);

      FILE* dFile = fopen(CS(dFileName),"w");
      
      headerName = OS_NormalizePath(headerName,temp);
      sourceName = OS_NormalizePath(sourceName,temp);

      auto list = PushList<String>(temp);

      for(FileDef* def : files){
        String path = def->filepathFromRoot;
        String absolute = GetAbsolutePath(path,temp);
        *list->PushElem() = absolute;
      }
      
      for(FileGroupDef* def : fileGroups){
        for(String folderPath : def->foldersFromRoot){
          DIR* directory = opendir(SF("%.*s",UN(folderPath)));
          Assert(directory);
          
          dirent* entry = nullptr;
          while ((entry = readdir(directory)) != NULL){
            if (entry->d_type == DT_DIR) {
              continue;
            }
            
            String fileName = PushString(temp,"%s",entry->d_name);

            String fullPath = PushString(temp,"%.*s/%.*s",UN(folderPath),UN(fileName));
            *list->PushElem() = GetAbsolutePath(fullPath,temp);
          }
        
          closedir(directory);
        }
      }

      fprintf(dFile,"%.*s %.*s:",UN(headerName),UN(sourceName));

      for(String str : list){
        fprintf(dFile," \\\n %.*s",UN(str));
      }
      
      fclose(dFile);
    }
    
    return 0;
  }

  {
    FILE* header = fopen(CS(headerName),"w");
    if(!header){
      printf("Error opening file for output: %.*s",UN(headerName));
      return -1;
    }

    DEFER_CLOSE_FILE(header);

    FREE_ARENA(emitter);
    CEmitter* h = StartCCode(emitter);
  
    h->Comment("File auto generated by embedData tool");
    h->Once();
    h->Include("utilsCore.hpp");

    h->Line();
    h->Comment("Structs definitions");
    
    for(StructDef* def : structs){
      h->Struct(def->name);
      for(Pair<String,String> p : def->typeAndName){
        h->Member(p.first,p.second);
      }
      h->EndBlock();

      h->Extern("Array<String>",SF("META_%.*s_Members",UN(def->name)));
    }

    h->Comment("Enum definition");
    for(EnumDef* def : enums){
      h->Enum(def->name);
      for(Pair<String,String> p : def->valuesNamesWithValuesIfExist){
        h->EnumMember(p.first,p.second);
      }
      h->EndEnum();

      h->FunctionDeclOnlyBlock("String","META_Repr");
      h->Argument(def->name,"val");
      h->EndBlock();
      
      h->ArrayDeclareBlock(SF("const %.*s",UN(def->name)),SF("%.*ss",UN(def->name)),true);
      for(Pair<String,String> p : def->valuesNamesWithValuesIfExist){
        h->Elem(p.first);
      }
      h->EndBlock();
    }

    h->Comment("Table definition");

    for(TableDef* def : tables){
      h->Struct(PushString(temp,"%.*s_GenType",UN(def->structTypename)));

      for(Parameter p : def->parameterList){
        TypeDef* typeDef = p.type;
        String name = p.name;

        if(typeDef->isArray){
          h->Member(PushString(temp,"Array<%.*s>",UN(typeDef->name)),name);
        } else {
          h->Member(typeDef->name,name);
        }

      }
      h->EndStruct();
    }

    h->Comment("Arrays");
    for(ArrayDef* def : arrays){
      h->Extern(SF("Array<%.*s>",UN(def->type)),def->name);
    }
    
    h->Comment("Tables");
    
    for(TableDef* def : tables){
      h->Extern(SF("Array<%.*s_GenType>",UN(def->structTypename)),def->structTypename);
    }
    
    h->Comment("Define Maps (the arrays store the data, not the keys)");
    
    for(MapDef* def : maps){
      if(!def->isDefineMap){
        continue;
      }

      for(Array<DataValue*> p : def->dataTable){
        // TODO: Only implement for single type. What would an array define map look like?
        
        Assert(p[1]->type == DataValueType_SINGLE);
        h->RawLine(PushString(temp,"#define %.*s \"%.*s\"",UN(p[0]->asStr),UN(p[1]->asStr)));
      }

      h->Extern("Array<String>",def->name);
    }

    h->Comment("Normal Maps");
    for(MapDef* def : maps){
      if(def->isDefineMap || !def->isEnumMap || !def->isBijection){
        continue;
      }

      {
        int from = 0;
        int to = 1;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];

        h->FunctionDeclOnlyBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_Map",UN(def->name)));
        h->Argument(pFrom.type->name,"val");
        h->EndBlock(); // Function
      }
      {
        int from = 1;
        int to = 0;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];

        h->FunctionDeclOnlyBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_ReverseMap",UN(def->name)));
        h->Argument(pFrom.type->name,"val");
        h->EndBlock();
      }
    }
    
    h->Comment("File content");
    
    for(FileDef* def : files){
      h->Extern("String",SF("META_%.*s_Content",UN(def->name)));
    }

    h->Struct("FileContent");
    h->Member("String","fileName");
    h->Comment("Without filename");
    h->Member("String","originalRelativePath");
    h->Member("String","commonFolder"); // TODO: We could push this outwards, since right now this is common to all the files.
    h->Member("String","content");
    h->EndStruct();

    for(FileGroupDef* def : fileGroups){
      h->Extern("Array<FileContent>",def->name);
    }
    
    CAST* end = EndCCode(h);
    StringBuilder* b = StartString(temp);
    Repr(end,b,true,0);
    String content = EndString(temp,b);
    fprintf(header,"%.*s\n",UN(content));
  } // header
  
  {
    FILE* source = fopen(CS(sourceName),"w");
    if(!source){
      printf("Error opening file for output: %.*s",UN(sourceName));
      return -1;
    }
    DEFER_CLOSE_FILE(source);

    FREE_ARENA(emitter);
    CEmitter* c = StartCCode(emitter);

    // Trying to bundle header and source appears fine but kinda limited? 
    // TODO: We could just have 2 emitters and make an interface on top of them, if needed.
    
    String headerFilenameOnly = ExtractFilenameOnly(headerName);

    c->Include(headerFilenameOnly);

    // Enum stuff
    c->Comment("Enum representation");
    for(EnumDef* def : enums){
      c->FunctionBlock("String","META_Repr");
      c->Argument(def->name,"val");

      c->SwitchBlock("val");
      for(Pair<String,String> p : def->valuesNamesWithValuesIfExist){
        String name = p.first;
        String val = PushString(temp,"String(\"%.*s\");",UN(name));

        c->CaseBlock(name);
        c->Return(val);
        c->EndBlock();
      }
      c->EndBlock();

      c->Statement("Assert(false)");
      c->Return("{}");
      c->EndBlock();
    }
      
    // Declare auxiliary data arrays
    c->Comment("Raw c array auxiliary to Struct data");
    for(StructDef* def : structs){
      c->ArrayDeclareBlock("String",SF("%.*s_MemberNames",UN(def->name)),true);
      for(auto p : def->typeAndName){
        c->StringElem(p.second);
      }
      c->EndBlock();
    }

    c->Comment("Struct data");
    for(StructDef* def : structs){
      c->VarDeclareBlock("Array<String>",SF("META_%.*s_Members",UN(def->name)));
      c->Elem(PushString(temp,"%.*s_MemberNames",UN(def->name)));
      c->Elem(PushString(temp,"ARRAY_SIZE(%.*s_MemberNames)",UN(def->name)));
      c->EndBlock();
    }    
    
    c->Comment("Raw C array auxiliary to tables");
    for(TableDef* def : tables){
      for(int i = 0; i <  def->dataTable.size; i++){
        Array<DataValue*> row  =  def->dataTable[i];

        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def->parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(val->type == DataValueType_FUNCTION){
            DataValue* data = EvaluateFunction(val->asFunc,&arena3);
            val = data;
          }
          
          if(val->type == DataValueType_ARRAY){
            c->ArrayDeclareBlock(typeDef->name,SF("%.*s_%.*s_aux%d",UN(def->structTypename),UN(name),i));
            
            for(DataValue* child : val->asArray){
              c->StringElem(child->asStr);
            }
            c->EndBlock();
          }
        }
      }
    }

    c->Comment("Raw C array Arrays");

    auto EmitRawArray = [](CEmitter* m,String arrayName){
      TEMP_REGION(temp,nullptr);
      m->Elem(PushString(temp,"%.*s_Raw",UN(arrayName)));
      m->Elem(PushString(temp,"ARRAY_SIZE(%.*s_Raw)",UN(arrayName)));
    };

    for(ArrayDef* def : arrays){
      Assert(def->val->type == DataValueType_ARRAY);

      c->ArrayDeclareBlock(def->type,SF("%.*s_Raw",UN(def->name)),true);

      for(int i = 0; i < def->val->asArray.size; i++){
        DataValue* val  = def->val->asArray[i];
        Assert(val->type == DataValueType_SINGLE);
        
        c->Elem(DefaultRepr(val,nullptr,temp));
      }

      c->EndBlock();

      c->VarDeclareBlock(SF("Array<%.*s>",UN(def->type)),def->name);
      EmitRawArray(c,def->name);
      c->EndBlock();
    }
      
    c->Comment("Raw C array Tables");

    for(TableDef* def : tables){
      c->ArrayDeclareBlock(SF("%.*s_GenType",UN(def->structTypename)),SF("%.*s_Raw",UN(def->structTypename)),true);

      for(int i = 0; i <  def->dataTable.size; i++){
        Array<DataValue*> row = def->dataTable[i];

        c->VarBlock();
        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def->parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(IsEnum(typeDef->name) || val->type == DataValueType_SINGLE){
            c->Elem(DefaultRepr(val,typeDef,temp));
          } else {
            c->VarBlock();

            String arrayName = PushString(temp,"%.*s_%.*s_aux%d",UN(def->structTypename),UN(name),i);

            // TODO: We can just create an Emit array function where we also provide the end to append to the string name.
            c->Elem(arrayName);
            c->Elem(PushString(temp,"ARRAY_SIZE(%.*s)",UN(arrayName)));

            c->EndBlock();
          }
        }

        c->EndBlock();
      }
        
      c->EndBlock(); // Array block
    }
    
    c->Comment("Tables");
    
    for(TableDef* def : tables){
      String n = def->structTypename;
      c->VarDeclareBlock(SF("Array<%.*s_GenType>",UN(n)),n);
      EmitRawArray(c,n);
      c->EndBlock();
    }

    c->Comment("Define Maps arrays");

    for(MapDef* def : maps){
      if(!def->isDefineMap){
        continue;
      }

      c->ArrayDeclareBlock("String",SF("%.*s_Raw",UN(def->name)),true);

      for(Array<DataValue*> p : def->dataTable){
        Assert(p[1]->type == DataValueType_SINGLE);
        c->Elem(p[0]->asStr);
      }
      
      c->EndBlock();
      
      c->VarDeclareBlock("Array<String>",def->name);
      EmitRawArray(c,def->name);
      c->EndBlock();
    }

    c->Comment("Normal Maps");
    
    for(MapDef* def : maps){
      // TODO: All these exist because we are just trying to implement something quickly right now, but when the time comes implement this.
      if(def->isDefineMap || !def->isEnumMap || !def->isBijection){
        continue;
      }
      
      {
        int from = 0;
        int to = 1;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];

        // TODO: Currently assuming that the first member is the enum.
        //       This is important because we are generating a switch statement based on the enum type
        Assert(IsEnum(pFrom.type->name));

        // TODO: Check if the table members are actually values of the enum or not.

        c->FunctionBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_Map",UN(def->name)));
        c->Argument(pFrom.type->name,"val");
        
        c->SwitchBlock("val");
        for(Array<DataValue*> rows : def->dataTable){
          DataValue* fFrom = rows[from];
          DataValue* fTo = rows[to];
        
          String dFrom = DefaultRepr(fFrom,pFrom.type,temp);
          String dTo = DefaultRepr(fTo,pTo.type,temp);

          c->CaseBlock(dFrom);
          c->Return(dTo);
          c->EndBlock();
        }
        c->EndBlock();
        c->Return("{}");

        c->EndBlock(); // Function
      }
      {
        int from = 1;
        int to = 0;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];

        c->FunctionBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_ReverseMap",UN(def->name)));
        c->Argument(pFrom.type->name,"val");
        
        //c->SwitchBlock(S8("val"));
        for(Array<DataValue*> rows : def->dataTable){
          DataValue* fFrom = rows[from];
          DataValue* fTo = rows[to];
        
          String dFrom = DefaultRepr(fFrom,pFrom.type,temp);
          String dTo = DefaultRepr(fTo,pTo.type,temp);

          c->If(PushString(temp,"val == %.*s",UN(dFrom)));
          c->Return(dTo);
          c->EndIf();
        }

        c->Return("{}");
        c->EndBlock(); // Function
      }
    }

    c->Comment("Embedded Files");

    for(FileDef* def : files){
      String path = def->filepathFromRoot;
      String absolute = GetAbsolutePath(path,temp);
      String content = PushFile(temp,absolute);
      String escapedString = EscapeCString(content,temp);
      
      c->VarDeclareBlock("String",SF("META_%.*s_Content",UN(def->name)));
      c->StringElem(escapedString);
      c->EndBlock();
    }

    c->Comment("Embed File Groups");

    for(FileGroupDef* def : fileGroups){
      auto list = PushList<FileGroupInfo>(temp);

      for(String folderPath : def->foldersFromRoot){
        DIR* directory = opendir(SF("%.*s",UN(folderPath)));
        Assert(directory);
        
        dirent* entry = nullptr;
        while ((entry = readdir(directory)) != NULL){
          if (entry->d_type == DT_DIR) {
            continue;
          }

          String fileName = PushString(temp,"%s",entry->d_name);

          FileGroupInfo* info = list->PushElem();
          info->originalRelativePath = folderPath;
          info->filename = fileName;
          info->commonFolder = def->commonFolder;
        }
        
        closedir(directory);
      }

      Array<FileGroupInfo> allFilePaths = PushArray(temp,list);
     
      static int index = 0;
      // Emit temp var to store escaped string content
      for(FileGroupInfo& info : allFilePaths){
        String dirPath = info.originalRelativePath;
        String fileName = info.filename;

        String fullPath = PushString(temp,"%.*s/%.*s",UN(dirPath),UN(fileName));
        
        String content = PushFile(temp,fullPath);
        String escapedString = EscapeCString(content,temp);

        const char* name = SF("temp_%d_CONTENT",index++);
        info.contentRawArrayName = PushString(temp,"%s",name);
        
        c->VarDeclareBlock("String",name,false);
        c->StringElem(escapedString);
        c->EndBlock();
      }

      // Emit raw array
      c->ArrayDeclareBlock("FileContent",SF("%.*s_Raw",UN(def->name)));
      for(FileGroupInfo& info : allFilePaths){
        String dirPath = info.originalRelativePath;
        String fileName = info.filename;
        String commonFolder = info.commonFolder;
        String rawArray = info.contentRawArrayName;

        c->VarBlock();
        c->StringElem(fileName);
        c->StringElem(dirPath);
        c->StringElem(commonFolder);
        c->Elem(rawArray);
        c->EndBlock();
      }
      c->EndBlock();

      // Emit array
      c->VarDeclareBlock("Array<FileContent>",def->name);
      EmitRawArray(c,def->name);
      c->EndBlock();
    }
    
    CAST* end = EndCCode(c);
    StringBuilder* b = StartString(temp);
    Repr(end,b,true,0);
    String content = EndString(temp,b);
    fprintf(source,"%.*s",UN(content));
  } // source
    
  return 0;
}
