#include <cstdio>

#include "debug.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "CEmitter.hpp"
#include "parser.hpp"
#include "utilsCore.hpp"

#include <dirent.h>

// TODO: Support numbers inside tables. Support more levels of data hierarchies.
//       More importantly, implement stuff as they are needed. No point in trying to push for anything complex right now.

// TODO: Implement special code for bit flags (needed currently by the AddressGen code).
//       Do not know yet if I want them to be enum, need to test how easy C++ lets enum bit flags work.
//       If C++ is giving trouble, generate the code needed to support easy bit operations.

// TODO: Embed file could just become part of the embed data approach. No point in having two different approaches. Altought the embed data approach would need to become more generic
//       in order to do what the embed file approach does in a data oriented manner.

// TODO: We are currently doing everything manually in terms of instantiating C auxiliary arrays and that kinda sucks. I should be able to make it more automatic, I should be able to generalize it somewhat.

struct EnumDef{
  String name;
  Array<Pair<String,String>> valuesNamesWithValuesIfExist;
};

struct StructDef{
  String name;
  Array<Pair<String,String>> typeAndName;
};

struct TypeDef{
  String name;
  bool isArray;
};

enum DataValueType{
  DataValueType_SINGLE,
  DataValueType_ARRAY,
  DataValueType_FUNCTION
};

struct FunctionValue{
  String name;
  Array<String> parameters;
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
  String name;
  String type;
  DataValue* val;
};

struct Parameter{
  TypeDef* type;
  String name;
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

ArenaList<EnumDef>* enums = nullptr;
ArenaList<StructDef>* structs = nullptr;
ArenaList<TableDef>* tables = nullptr;
ArenaList<MapDef>* maps = nullptr;
ArenaList<FileDef>* files = nullptr;
ArenaList<FileGroupDef>* fileGroups = nullptr;
ArenaList<ArrayDef>* arrays = nullptr;

// TODO: Weird way of doing things but works for now.
GenericDef GetDefinition(String name){
  GenericDef res = {};
  for(EnumDef& def : enums){
    if(CompareString(def.name,name)){
      res.type = DefType_ENUM;
      res.asGenericPointer = &def;
      return res;
    }
  }
  for(StructDef& def : structs){
    if(CompareString(def.name,name)){
      res.type = DefType_STRUCT;
      res.asGenericPointer = &def;
      return res;
    }
  }
  for(TableDef& def : tables){
    if(CompareString(def.structTypename,name)){
      res.type = DefType_TABLE;
      res.asGenericPointer = &def;
      return res;
    }
  }
  for(MapDef& def : maps){
    if(CompareString(def.name,name)){
      res.type = DefType_MAP;
      res.asGenericPointer = &def;
      return res;
    }
  }
  
  printf("Type '%.*s' does not exist\n",UN(name));
  Assert(false);
  return {};
}

static bool IsEnum(String name){
  for(EnumDef def : enums){
    if(def.name == name){
      return true;
    }
  }
  return false;
}

EnumDef* ParseEnum(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  tok->ExpectIdentifier("enum");

  Token enumTypeName = tok->ExpectNext(TokenType_IDENTIFIER);

  tok->ExpectNext('{');
  
  auto memberList = PushList<Pair<String,String>>(temp);
  while(!tok->Done()){
    Token name = tok->ExpectNext(TokenType_IDENTIFIER);

    String fullValue = {};
    if(tok->IfNextToken('=')){
      auto b = StartString(temp);
      while(!tok->Done() && !tok->IfPeekToken(',') && !tok->IfPeekToken('}')){
        Token t = tok->NextToken();
        b->PushString(t.originalData);
      }

      fullValue = EndString(out,b);
    }
    
    *memberList->PushElem() = {name.identifier,fullValue};
    
    tok->IfNextToken(',');
    
    if(tok->IfPeekToken('}')){
      break;
    }
  }

  tok->ExpectNext('}');
  tok->ExpectNext(';');

  EnumDef* res = enums->PushElem();
  res->name = enumTypeName.identifier;
  res->valuesNamesWithValuesIfExist = PushArray(out,memberList);

  return res;
}

StructDef* ParseStruct(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);

  tok->ExpectIdentifier("struct");

  Token structTypeName = tok->ExpectNext(TokenType_IDENTIFIER);

  tok->ExpectNext('{');

  auto memberList = PushList<Pair<String,String>>(temp);
  while(!tok->Done()){
    Token type = tok->ExpectNext(TokenType_IDENTIFIER);
    Token name = tok->ExpectNext(TokenType_IDENTIFIER);

    *memberList->PushElem() = {type.identifier,name.identifier};

    tok->IfNextToken(';');

    if(tok->IfPeekToken('}')){
      break;
    }
  }

  tok->ExpectNext('}');
  tok->ExpectNext(';');

  StructDef* res = structs->PushElem();
  res->name = structTypeName.identifier;
  res->typeAndName = PushArray(out,memberList);

  return res;
}

TypeDef* ParseTypeDef(Parser* tok,Arena* out){
  Token name = tok->ExpectNext(TokenType_IDENTIFIER);

  bool isArray = false;
  if(tok->IfNextToken('[')){
    tok->ExpectNext(']');
    isArray = true;
  }

  TypeDef* def = PushStruct<TypeDef>(out);
  def->name = name.identifier;
  def->isArray = isArray;

  return def;
}

Array<String> ParseList(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  tok->ExpectNext('(');

  auto typeList = PushList<String>(temp);
  while(!tok->Done()){
    Token name = tok->ExpectNext(TokenType_IDENTIFIER);

    *typeList->PushElem() = name.identifier;
    
    if(tok->IfPeekToken(')')){
      break;
    }

    tok->IfNextToken(',');
  }
  tok->ExpectNext(')');

  Array<String> defs = PushArray(out,typeList);
  return defs;
}

Array<Parameter> ParseParameterList(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  tok->ExpectNext('(');

  auto typeList = PushList<Parameter>(temp);
  while(!tok->Done()){
    TypeDef* def = ParseTypeDef(tok,out);
    
    Token name = tok->ExpectNext(TokenType_IDENTIFIER);

    *typeList->PushElem() = {def,name.identifier};
    
    if(tok->IfPeekToken(')')){
      break;
    }

    tok->IfNextToken(',');
  }
  tok->ExpectNext(')');

  Array<Parameter> defs = PushArray(out,typeList);
  return defs;
}

DataValue* ParseValue(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);

  if(tok->IfNextToken('@')){
    Token functionName = tok->NextToken();

    Array<String> parameters = ParseList(tok,out);

    DataValue* val = PushStruct<DataValue>(out);
    val->type = DataValueType_FUNCTION;
    val->asFunc.name = functionName.identifier;
    val->asFunc.parameters = parameters;
    return val;
  }
  
  if(!tok->IfNextToken('{')){
    // Parsing a simple value here
    DataValue* val = PushStruct<DataValue>(out);
    val->type = DataValueType_SINGLE;

    Token value = tok->NextToken();

    val->asStr = value.cString;
    return val;
  }

  // Parsing an array here.
  auto valueList = PushList<DataValue*>(temp);
  while(!tok->Done()){
    DataValue* val = ParseValue(tok,out);
    *valueList->PushElem() = val;
    
    tok->IfNextToken(',');

    if(tok->IfPeekToken('}')){
      break;
    }
  }
  tok->ExpectNext('}');

  DataValue* val = PushStruct<DataValue>(out);
  val->type = DataValueType_ARRAY;
  val->asArray = PushArray(out,valueList);
  return val;
}

Array<Array<DataValue*>> ParseDataTable(Parser* tok,int expectedColumns,Arena* out){
  TEMP_REGION(temp,out);

  auto valueTableList = PushList<Array<DataValue*>>(temp);
  while(!tok->Done()){
    auto valueList = PushList<DataValue*>(temp);
    for(int i = 0; i < expectedColumns; i++){
      DataValue* val = ParseValue(tok,out);
      *valueList->PushElem() = val;

      if(i != expectedColumns - 1){
        tok->ExpectNext(':');
      }
    }

    Array<DataValue*> array = PushArray(out,valueList);
    *valueTableList->PushElem() = array;    
    
    tok->IfNextToken(',');

    if(tok->IfPeekToken('}')){
      break;
    }
  }

  return PushArray(out,valueTableList);
}

TableDef* ParseTable(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);

  tok->ExpectIdentifier("table");
  Token tableStructName = tok->ExpectNext(TokenType_IDENTIFIER);

  Array<Parameter> defs = ParseParameterList(tok,out);
  Assert(defs.size > 0);
  
  tok->ExpectNext('{');

  Array<Array<DataValue*>> table = ParseDataTable(tok,defs.size,out);

  tok->ExpectNext('}');
  tok->ExpectNext(';');
  
  TableDef* def = tables->PushElem();
  def->structTypename = tableStructName.identifier;
  def->parameterList = defs;
  def->dataTable = table;

  return def;
}

FileDef* ParseFile(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  tok->ExpectIdentifier("file");

  String fileName = tok->NextToken().identifier;

  tok->ExpectNext('=');
  Token filepath = tok->ExpectNext(TokenType_FILEPATH);
  tok->ExpectNext(';');

  FileDef* def = files->PushElem();
  def->name = fileName;
  def->filepathFromRoot = TrimWhitespaces(filepath.filepath);

  return def;
}

FileGroupDef* ParseFileGroup(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  tok->ExpectIdentifier("fileGroup");
  
  Token groupName = tok->NextToken();

  Token commonFolder = {};
  if(tok->IfNextToken('(')){
    commonFolder = tok->NextToken();
    
    tok->ExpectNext(')');
  }
  
  tok->ExpectNext('=');

  auto list = PushList<String>(temp);
  while(!tok->Done()){
    if(tok->IfPeekToken(';')){
      break;
    }
    
    Token folderFilepath = tok->ExpectNext(TokenType_FILEPATH);

    *list->PushElem() = TrimWhitespaces(folderFilepath.filepath);
    tok->IfNextToken('|');
  }

  tok->ExpectNext(';');

  FileGroupDef* def = fileGroups->PushElem();
  def->name = groupName.identifier;
  def->commonFolder = commonFolder.identifier;
  def->foldersFromRoot = PushArray(out,list);

  return def;
}

MapDef* ParseMap(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);

  Token type = tok->ExpectNext(TokenType_IDENTIFIER);
  bool isDefineMap = false;

  if(type.identifier == "define_map"){
    isDefineMap = true;
  } else if(type.identifier == "map"){
    isDefineMap = false;
  } else {
    tok->ReportError("Did not find either a define_map or a map");
  }

  Token mapName = tok->ExpectNext(TokenType_IDENTIFIER);

  if(!isDefineMap){
    Array<Parameter> parameters = ParseParameterList(tok,out);
    Assert(parameters.size > 0);
    
    tok->ExpectNext('{');

    Array<Array<DataValue*>> table = ParseDataTable(tok,parameters.size,out);
    
    tok->ExpectNext('}');
    tok->ExpectNext(';');

    MapDef* res = maps->PushElem();
    res->parameterList = parameters;
    res->name = mapName.identifier;
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
    tok->ExpectNext('{');

    Array<Array<DataValue*>> table = ParseDataTable(tok,2,out);

    tok->ExpectNext('}');
    tok->ExpectNext(';');

    MapDef* res = maps->PushElem();
    res->name = mapName.identifier;
    res->dataTable = table;
    res->isDefineMap = isDefineMap;

    return res;
  }

  NOT_POSSIBLE();
  return nullptr;
}

ArrayDef* ParseArray(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);
  tok->ExpectIdentifier("array");

  Token arrayType = tok->ExpectNext(TokenType_IDENTIFIER);
  Token arrayName = tok->ExpectNext(TokenType_IDENTIFIER);
  
  tok->ExpectNext('=');

  DataValue* val = ParseValue(tok,out);
  Assert2(val->type == DataValueType_ARRAY,"Array cannot handle single values. Pass an array of values using {}");
  
  tok->ExpectNext(';');

  ArrayDef* res = arrays->PushElem();
  res->name = arrayName.identifier;
  res->type = arrayType.identifier;
  res->val = val;

  return res;
}

void ParseContent(String content,Arena* out){
  TEMP_REGION(temp,out);

  auto TokenizeFunction = [](void* tokenizerState) -> Token{
    DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
    
    const char* start = state->ptr;
    const char* end = state->end;

    if(start >= end){
      Token token = {};
      token.type = TokenType_EOF;
      return token;
    }

    TokenizeResult res = {};
    if(res.token.type == TokenType_INVALID) res |= ParseWhitespace(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseComments(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseCString(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseFilepath(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseSymbols(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseNumber(start,end);
    if(res.token.type == TokenType_INVALID) res |= ParseIdentifier(start,end);

    int size = res.bytesParsed;
    if(size <= 0 && state->ptr != state->end){
      size = 1;
    }

    state->ptr += size;

    // NOTE: Something very bad must happen to the point where the file is 1 byte after the end.
    //       We expect it to only reach file->end, not file->end + 1
    Assert(state->ptr < state->end + 1);

    return res.token;
  };

  FREE_ARENA(parseArena);
  Parser* parser = StartParsing(TokenizeFunction,content,parseArena);
  
  while(!parser->Done()){
    Token peek = parser->PeekToken();

    if(peek.type != TokenType_IDENTIFIER){
      parser->ReportError("Unexpected token at global scope");
      parser->NextToken();
      continue;
    }

    String val = peek.identifier;

    if(val == "enum"){
      ParseEnum(parser,out);
    } else if(val == "struct"){
      ParseStruct(parser,out);
    } else if(val == "array"){
      ParseArray(parser,out);
    } else if(val == "table"){
      ParseTable(parser,out);
    } else if(val == "file"){
      ParseFile(parser,out);
    } else if(val ==  "map" || val == "define_map"){
      ParseMap(parser,out);
    } else if(val == "fileGroup"){
      ParseFileGroup(parser,out);
    } else {
      parser->ReportError("Unexpected token at global scope");
      parser->NextToken();
    }
  }

  for(String err : parser->errors){
    printf("%.*s\n",UN(err));
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
    String type = func.parameters[0];
    
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


static String PushFile(Arena* arena,String path){
  // Care since this is stored on a static buffer
  const char* asCStr = StaticFormat("%.*s",UN(path));
  FILE* file = fopen(asCStr,"r");

  long int size = GetFileSize(file);

  AlignArena(arena,alignof(void*));

  Byte* mem = PushBytes(arena,size);
  int amountRead = fread(mem,sizeof(Byte),size,file);

  if(amountRead != size){
    fprintf(stderr,"Memory PushFile failed to read entire file\n");
    exit(-1);
  }

  String res = {};
  res.size = size;
  res.data = (const char*) mem;

  return res;
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

  FREE_ARENA(allData);

  enums = PushList<EnumDef>(allData);
  structs = PushList<StructDef>(allData);
  tables = PushList<TableDef>(allData);
  maps = PushList<MapDef>(allData);
  files = PushList<FileDef>(allData);
  fileGroups = PushList<FileGroupDef>(allData);
  arrays = PushList<ArrayDef>(allData);

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

      for(FileDef& def : files){
        String path = def.filepathFromRoot;
        String absolute = GetAbsolutePath(path,temp);
        *list->PushElem() = absolute;
      }
      
      for(FileGroupDef& def : fileGroups){
        for(String folderPath : def.foldersFromRoot){
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
    
    for(StructDef& def : structs){
      h->Struct(def.name);
      for(Pair<String,String> p : def.typeAndName){
        h->Member(p.first,p.second);
      }
      h->EndBlock();

      h->Extern("Array<String>",SF("META_%.*s_Members",UN(def.name)));
    }

    h->Comment("Enum definition");
    for(EnumDef& def : enums){

      h->Enum(def.name);
      for(Pair<String,String> p : def.valuesNamesWithValuesIfExist){
        h->EnumMember(p.first,p.second);
      }
      h->EndEnum();

      h->FunctionDeclOnlyBlock("String","META_Repr");
      h->Argument(def.name,"val");
      h->EndBlock();
      
      h->ArrayDeclareBlock(SF("const %.*s",UN(def.name)),SF("%.*ss",UN(def.name)),true);
      for(Pair<String,String> p : def.valuesNamesWithValuesIfExist){
        h->Elem(p.first);
      }
      h->EndBlock();
    }

    h->Comment("Table definition");

    for(TableDef& def : tables){
      h->Struct(PushString(temp,"%.*s_GenType",UN(def.structTypename)));

      for(Parameter p : def.parameterList){
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
    for(ArrayDef& def : arrays){
      h->Extern(SF("Array<%.*s>",UN(def.type)),def.name);
    }
    
    h->Comment("Tables");
    
    for(TableDef& def : tables){
      h->Extern(SF("Array<%.*s_GenType>",UN(def.structTypename)),def.structTypename);
    }
    
    h->Comment("Define Maps (the arrays store the data, not the keys)");
    
    for(MapDef& def : maps){
      if(!def.isDefineMap){
        continue;
      }

      for(Array<DataValue*> p : def.dataTable){
        // TODO: Only implement for single type. What would an array define map look like?
        
        Assert(p[1]->type == DataValueType_SINGLE);
        h->RawLine(PushString(temp,"#define %.*s \"%.*s\"",UN(p[0]->asStr),UN(p[1]->asStr)));
      }

      h->Extern("Array<String>",def.name);
    }

    h->Comment("Normal Maps");
    for(MapDef& def : maps){
      if(def.isDefineMap || !def.isEnumMap || !def.isBijection){
        continue;
      }

      {
        int from = 0;
        int to = 1;
      
        Parameter pFrom = def.parameterList[from];
        Parameter pTo = def.parameterList[to];

        h->FunctionDeclOnlyBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_Map",UN(def.name)));
        h->Argument(pFrom.type->name,"val");
        h->EndBlock(); // Function
      }
      {
        int from = 1;
        int to = 0;
      
        Parameter pFrom = def.parameterList[from];
        Parameter pTo = def.parameterList[to];

        h->FunctionDeclOnlyBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_ReverseMap",UN(def.name)));
        h->Argument(pFrom.type->name,"val");
        h->EndBlock();
      }
    }
    
    h->Comment("File content");
    
    for(FileDef& def : files){
      h->Extern("String",SF("META_%.*s_Content",UN(def.name)));
    }

    for(FileGroupDef& def : fileGroups){
      h->RawLine(SF("\n#define DEFINED_%.*s 1",UN(def.name)));
      h->Extern("Array<FileContent>",def.name);
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
    for(EnumDef& def : enums){
      c->FunctionBlock("String","META_Repr");
      c->Argument(def.name,"val");

      c->SwitchBlock("val");
      for(Pair<String,String> p : def.valuesNamesWithValuesIfExist){
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
    for(StructDef& def : structs){
      c->ArrayDeclareBlock("String",SF("%.*s_MemberNames",UN(def.name)),true);
      for(auto p : def.typeAndName){
        c->StringElem(p.second);
      }
      c->EndBlock();
    }

    c->Comment("Struct data");
    for(StructDef& def : structs){
      c->VarDeclareBlock("Array<String>",SF("META_%.*s_Members",UN(def.name)));
      c->Elem(PushString(temp,"%.*s_MemberNames",UN(def.name)));
      c->Elem(PushString(temp,"ARRAY_SIZE(%.*s_MemberNames)",UN(def.name)));
      c->EndBlock();
    }    
    
    c->Comment("Raw C array auxiliary to tables");
    for(TableDef& def : tables){
      for(int i = 0; i <  def.dataTable.size; i++){
        Array<DataValue*> row  =  def.dataTable[i];

        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def.parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(val->type == DataValueType_FUNCTION){
            DataValue* data = EvaluateFunction(val->asFunc,&arena3);
            val = data;
          }
          
          if(val->type == DataValueType_ARRAY){
            c->ArrayDeclareBlock(typeDef->name,SF("%.*s_%.*s_aux%d",UN(def.structTypename),UN(name),i));
            
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

    for(ArrayDef& def : arrays){
      Assert(def.val->type == DataValueType_ARRAY);

      c->ArrayDeclareBlock(def.type,SF("%.*s_Raw",UN(def.name)),true);

      for(int i = 0; i < def.val->asArray.size; i++){
        DataValue* val  = def.val->asArray[i];
        Assert(val->type == DataValueType_SINGLE);
        
        c->Elem(DefaultRepr(val,nullptr,temp));
      }

      c->EndBlock();

      c->VarDeclareBlock(SF("Array<%.*s>",UN(def.type)),def.name);
      EmitRawArray(c,def.name);
      c->EndBlock();
    }
      
    c->Comment("Raw C array Tables");

    for(TableDef& def : tables){
      c->ArrayDeclareBlock(SF("%.*s_GenType",UN(def.structTypename)),SF("%.*s_Raw",UN(def.structTypename)),true);

      for(int i = 0; i <  def.dataTable.size; i++){
        Array<DataValue*> row = def.dataTable[i];

        c->VarBlock();
        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def.parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(IsEnum(typeDef->name) || val->type == DataValueType_SINGLE){
            c->Elem(DefaultRepr(val,typeDef,temp));
          } else {
            c->VarBlock();

            String arrayName = PushString(temp,"%.*s_%.*s_aux%d",UN(def.structTypename),UN(name),i);

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
    
    for(TableDef& def : tables){
      String n = def.structTypename;
      c->VarDeclareBlock(SF("Array<%.*s_GenType>",UN(n)),n);
      EmitRawArray(c,n);
      c->EndBlock();
    }

    c->Comment("Define Maps arrays");

    for(MapDef& def : maps){
      if(!def.isDefineMap){
        continue;
      }

      c->ArrayDeclareBlock("String",SF("%.*s_Raw",UN(def.name)),true);

      for(Array<DataValue*> p : def.dataTable){
        Assert(p[1]->type == DataValueType_SINGLE);
        c->Elem(p[0]->asStr);
      }
      
      c->EndBlock();
      
      c->VarDeclareBlock("Array<String>",def.name);
      EmitRawArray(c,def.name);
      c->EndBlock();
    }

    c->Comment("Normal Maps");
    
    for(MapDef& def : maps){
      // TODO: All these exist because we are just trying to implement something quickly right now, but when the time comes implement this.
      if(def.isDefineMap || !def.isEnumMap || !def.isBijection){
        continue;
      }
      
      {
        int from = 0;
        int to = 1;
      
        Parameter pFrom = def.parameterList[from];
        Parameter pTo = def.parameterList[to];

        // TODO: Currently assuming that the first member is the enum.
        //       This is important because we are generating a switch statement based on the enum type
        Assert(IsEnum(pFrom.type->name));

        // TODO: Check if the table members are actually values of the enum or not.

        c->FunctionBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_Map",UN(def.name)));
        c->Argument(pFrom.type->name,"val");
        
        c->SwitchBlock("val");
        for(Array<DataValue*> rows : def.dataTable){
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
      
        Parameter pFrom = def.parameterList[from];
        Parameter pTo = def.parameterList[to];

        c->FunctionBlock(PushString(temp,"Opt<%.*s>",UN(pTo.type->name)),PushString(temp,"META_%.*s_ReverseMap",UN(def.name)));
        c->Argument(pFrom.type->name,"val");
        
        //c->SwitchBlock(S8("val"));
        for(Array<DataValue*> rows : def.dataTable){
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

    for(FileDef& def : files){
      String path = def.filepathFromRoot;
      String absolute = GetAbsolutePath(path,temp);
      String content = PushFile(temp,absolute);
      String escapedString = EscapeCString(content,temp);
      
      c->VarDeclareBlock("String",SF("META_%.*s_Content",UN(def.name)));
      c->StringElem(escapedString);
      c->EndBlock();
    }

    c->Comment("Embed File Groups");

    for(FileGroupDef& def : fileGroups){
      auto list = PushList<FileGroupInfo>(temp);

      for(String folderPath : def.foldersFromRoot){
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
          info->commonFolder = def.commonFolder;
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
      c->ArrayDeclareBlock("FileContent",SF("%.*s_Raw",UN(def.name)));
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
      c->VarDeclareBlock("Array<FileContent>",def.name);
      EmitRawArray(c,def.name);
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
