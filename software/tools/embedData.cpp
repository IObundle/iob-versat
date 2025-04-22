#include <cstdio>

#include "debug.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

// TODO: We are trying to be "clever" with the fact that we only parse stuff and then let outside code act on it, but I think that I just want to make stuff as simple as possible. If the parser sees a enum it immediatly registers it and the parsing code can immediatly check if a type already exists or not. I do not see the need to make this code respect lexer/parser/compiler boundaries because we immediatly fail and force the programmer to change if any parsing problem occurs. No need to synchronize or keep parsing or anything like that, just make sure that we fail fast.

// TODO: Support numbers inside tables. Support more levels of data hierarchies.
//       More importantly, implement stuff as they are needed. No point in trying to push for anything complex right now.
// TODO: Implement special code for bit flags (needed currently by the AddressGen code).
//       Do not know yet if I want them to be enum, need to test how easy C++ lets enum bit flags work.
//       If C++ is giving trouble, generate the code needed to support easy bit operations.

// TODO: Embed file could just become part of the embed data approach. No point in having two different approaches. Altought the embed data approach would need to become more generic
//       in order to do what the embed file approach does in a data oriented manner.

// TODO: We are currently doing everything manually in terms of instantiating C auxiliary arrays and that kinda sucks. I should be able to make it more automatic, I should be able to generalize it somewhat.

void ReportError(String content,Token faultyToken,const char* error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("\n");
  printf("%.*s\n",UNPACK_SS(faultyToken));
  printf("%s\n",error);
  printf("%.*s\n",UNPACK_SS(loc));
  printf("\n");

  DEBUG_BREAK();
}

void ReportError(Tokenizer* tok,Token faultyToken,const char* error){
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
    ReportError(tok,ID,StaticFormat("identifier '%.*s' is not a valid name",UNPACK_SS(ID))); \
    return {}; \
  }

bool _ExpectError(Tokenizer* tok,const char* str){
  TEMP_REGION(temp,nullptr);

  Token got = tok->NextToken();
  String expected = STRING(str);
  if(!CompareString(got,expected)){
    
    auto builder = StartString(temp);
    builder->PushString("Parser Error.\n Expected to find:  '");
    builder->PushString(PushEscapedString(temp,expected,' '));
    builder->PushString("'\n");
    builder->PushString("  Got:");
    builder->PushString(PushEscapedString(temp,got,' '));
    builder->PushString("\n");
    String text = EndString(temp,builder);
    ReportError(tok,got,StaticFormat("%*s",UNPACK_SS(text))); \
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

Pool<EnumDef> enums = {};
Pool<StructDef> structs = {};
Pool<TableDef> tables = {};
Pool<MapDef> maps = {};
Pool<FileDef> files = {};

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
  
  printf("Type '%.*s' does not exist\n",UNPACK_SS(name));
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

  auto memberList = PushArenaList<Pair<String,String>>(temp);
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
  res->valuesNamesWithValuesIfExist = PushArrayFromList(out,memberList);

  return res;
}

StructDef* ParseStruct(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"struct");

  Token structTypeName = tok->NextToken();
  CheckIdentifier(structTypeName);

  AssertToken(tok,"{");

  auto memberList = PushArenaList<Pair<String,String>>(temp);
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
  res->typeAndName = PushArrayFromList(out,memberList);

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

  auto typeList = PushArenaList<Token>(temp);
  while(!tok->Done()){
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *typeList->PushElem() = name;
    
    if(tok->IfPeekToken(")")){
      break;
    }

    bool seenComma = tok->IfNextToken(",");
  }
  AssertToken(tok,")");

  Array<Token> defs = PushArrayFromList(out,typeList);
  return defs;
}

Array<Parameter> ParseParameterList(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"(");

  auto typeList = PushArenaList<Parameter>(temp);
  while(!tok->Done()){
    TypeDef* def = ParseTypeDef(tok,out);
    
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *typeList->PushElem() = {def,name};
    
    if(tok->IfPeekToken(")")){
      break;
    }

    bool seenComma = tok->IfNextToken(",");
  }
  AssertToken(tok,")");

  Array<Parameter> defs = PushArrayFromList(out,typeList);
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
    DataValue* val = PushStruct<DataValue>(out);
    val->type = DataValueType_SINGLE;
    val->asStr = tok->NextToken();
    return val;
  }

  // Parsing an array here.
  auto valueList = PushArenaList<DataValue*>(temp);
  while(!tok->Done()){
    DataValue* val = ParseValue(tok,out);
    *valueList->PushElem() = val;
    
    bool seenComma = tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }
  AssertToken(tok,"}");

  DataValue* val = PushStruct<DataValue>(out);
  val->type = DataValueType_ARRAY;
  val->asArray = PushArrayFromList(out,valueList);
  return val;
}

Array<Array<DataValue*>> ParseDataTable(Tokenizer* tok,int expectedColumns,Arena* out){
  TEMP_REGION(temp,out);

  auto valueTableList = PushArenaList<Array<DataValue*>>(temp);
  while(!tok->Done()){
    auto valueList = PushArenaList<DataValue*>(temp);
    for(int i = 0; i < expectedColumns; i++){
      DataValue* val = ParseValue(tok,out);
      *valueList->PushElem() = val;

      if(i != expectedColumns - 1){
        AssertToken(tok,":");
      }
    }

    Array<DataValue*> array = PushArrayFromList(out,valueList);
    *valueTableList->PushElem() = array;    
    
    bool seenComma = tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }

  return PushArrayFromList(out,valueTableList);
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
    DEBUG_BREAK();
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

void ParseContent(String content,Arena* out){
  TEMP_REGION(temp,out);
  Tokenizer tokInst(content,"!@#$%^&*()-+={[}]\\|~`;:'\",./?",{"//","/*","*/"});
  Tokenizer* tok = &tokInst;
  
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"enum")){
      EnumDef* def = ParseEnum(tok,out);
    } else if(CompareString(peek,"struct")){
      StructDef* def = ParseStruct(tok,out);
    } else if(CompareString(peek,"table")){
      TableDef* def = ParseTable(tok,out);
    } else if(CompareString(peek,"file")){
      FileDef* def = ParseFile(tok,out);
    } else if(CompareString(peek,"map") || CompareString(peek,"define_map")){
      MapDef* def = ParseMap(tok,out);
    } else {
      ReportError(tok,peek,"Unexpected token at global scope");
      tok->AdvancePeek();
    }
  }
}

String DefaultRepr(DataValue* val,TypeDef* type,Arena* out){
  if(IsEnum(type->name)){
    return val->asStr;
  } else if(val->type == DataValueType_SINGLE){
    return PushString(out,"STRING(\"%.*s\")",UNPACK_SS(val->asStr));
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
    default: NOT_IMPLEMENTED("yet");
    }
  } else {
    printf("Did not find a function named %.*s\n",UNPACK_SS(func.name));
  }
}

// 0 - exe name
// 1 - definition file
// 2 - output name
int main(int argc,const char* argv[]){
  if(argc < 3){
    fprintf(stderr,"Need at least 3 arguments: <exe> <defFile> <outputFilename> \n");
    return -1;
  }

  InitDebug();
  
  const char* defFilePath = argv[1];
  const char* outputPath = argv[2];

  // Initialize context arenas
  Arena inst1 = InitArena(Megabyte(64));
  contextArenas[0] = &inst1;
  Arena inst2 = InitArena(Megabyte(64));
  contextArenas[1] = &inst2;

  TEMP_REGION(permanent,nullptr);
  TEMP_REGION(temp,permanent);

  String content = PushFile(temp,defFilePath);

  ParseContent(content,permanent);
  
  String headerName = PushString(temp,"%s.hpp",outputPath);
  String sourceName = PushString(temp,"%s.cpp",outputPath);

  {
    FILE* header = fopen(StaticFormat("%.*s",UNPACK_SS(headerName)),"w");
    if(!header){
      printf("Error opening file for output: %.*s",UNPACK_SS(headerName));
      return -1;
    }

    DEFER_CLOSE_FILE(header);

    fprintf(header,"// File auto generated by embedData tool\n");
    
    fprintf(header,"#pragma once\n\n");
  
    fprintf(header,"#include \"utilsCore.hpp\"\n");

    fprintf(header,"\n// Structs definition\n\n");

    for(StructDef* def : structs){
      fprintf(header,"struct %.*s {\n",UNPACK_SS(def->name));

      bool first = true;
      for(Pair<String,String> p : def->typeAndName){
        if(!first){
          fprintf(header,",\n");
        }

        fprintf(header,"  %.*s %.*s;\n",UNPACK_SS(p.first),UNPACK_SS(p.second));
      }
      fprintf(header,"};\n");

      fprintf(header,"extern Array<String> META_%.*s_Members;\n",UNPACK_SS(def->name));
    }
      
    fprintf(header,"\n// Enum definition\n\n");

    for(EnumDef* def : enums){
      fprintf(header,"enum %.*s {\n",UNPACK_SS(def->name));

      bool first = true;
      for(Pair<String,String> p : def->valuesNamesWithValuesIfExist){
        if(!first){
          fprintf(header,",\n");
        }

        fprintf(header,"  %.*s",UNPACK_SS(p.first));
        if(!Empty(p.second)){
          fprintf(header," = %.*s",UNPACK_SS(p.second));
        }

        first = false;
      }

      fprintf(header,"\n};\n");
    }

    fprintf(header,"\n// Table definition\n\n");

    for(TableDef* def : tables){
      fprintf(header,"struct %.*s_GenType {\n",UNPACK_SS(def->structTypename));
      for(Parameter p : def->parameterList){
        TypeDef* typeDef = p.type;
        String name = p.name;
      
        if(typeDef->isArray){
          fprintf(header,"  Array<%.*s> %.*s;\n",UNPACK_SS(typeDef->name),UNPACK_SS(name));
        } else {
          fprintf(header,"  %.*s %.*s;\n",UNPACK_SS(typeDef->name),UNPACK_SS(name));
        }
      }

      fprintf(header,"};\n");
    }
      
    fprintf(header,"\n// Tables\n\n");

    auto ToUpper = [](char ch) -> char{
      if(ch >= 'a' && ch <= 'z'){
        return ((ch - 'a') + 'A');
      }
      return ch;
    };
    auto ToLower = [](char ch) -> char{
      if(ch >= 'A' && ch <= 'Z'){
        return ((ch - 'A') + 'z');
      }
      return ch;
    };
    
    for(TableDef* def : tables){
      String name = def->structTypename;

      fprintf(header,"extern Array<%.*s_GenType> %.*s;\n",UNPACK_SS(name),UNPACK_SS(name));
    }
    
    fprintf(header,"\n// Define Maps (the arrays store the data, not the keys)\n\n");

    for(MapDef* def : maps){
      if(!def->isDefineMap){
        continue;
      }

      for(Array<DataValue*> p : def->dataTable){
        // TODO: Only implement for single type. Waht would an array define map look like?
        
        Assert(p[1]->type == DataValueType_SINGLE);
        fprintf(header,"#define %.*s STRING(\"%.*s\")\n",UNPACK_SS(p[0]->asStr),UNPACK_SS(p[1]->asStr));
      }

      fprintf(header,"\nextern Array<String> %.*s;\n\n",UNPACK_SS(def->name));
    }
    fprintf(header,"// Normal Maps\n\n");

    for(MapDef* def : maps){
      // TODO: All these exist because we are just trying to implement something quickly right now, but when the time comes implement this.
      if(def->isDefineMap){
        continue;
      }
      if(!def->isEnumMap){
        continue;
      }
      if(!def->isBijection){
        continue;
      }

      {
        int from = 0;
        int to = 1;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];
      
        fprintf(header,"Opt<%.*s> META_%.*s_Map(%.*s);\n",UNPACK_SS(pTo.type->name),UNPACK_SS(def->name),UNPACK_SS(pFrom.type->name));
      }
      {
        int from = 1;
        int to = 0;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];
        
        fprintf(header,"Opt<%.*s> META_%.*s_ReverseMap(%.*s);\n",UNPACK_SS(pTo.type->name),UNPACK_SS(def->name),UNPACK_SS(pFrom.type->name));
      }
    }

    fprintf(header,"\n// File content\n\n");
    for(FileDef* def : files){
      fprintf(header,"extern String META_%.*s_Content;\n",UNPACK_SS(def->name));
    }
  } // header
  
  {
    FILE* source = fopen(StaticFormat("%.*s",UNPACK_SS(sourceName)),"w");
    if(!source){
      printf("Error opening file for output: %.*s",UNPACK_SS(sourceName));
      return -1;
    }
    DEFER_CLOSE_FILE(source);

    fprintf(source,"// File auto generated by embedData tool\n");

    String headerFilenameOnly = ExtractFilenameOnly(headerName);
    fprintf(source,"#include \"%.*s\"\n",UNPACK_SS(headerFilenameOnly));

    fprintf(source,"\n// RaW c array auxiliary to Struct data\n\n");
    for(StructDef* def : structs){
      fprintf(source,"static String %.*s_MemberNames[] = {\n",UNPACK_SS(def->name));

      bool first = true;
      for(auto p : def->typeAndName){
        if(first){
          first = false;
        } else {
          fprintf(source,",\n");
        }
        fprintf(source,"  STRING(\"%.*s\")",UNPACK_SS(p.second));
      }
      fprintf(source,"\n};\n");
    }

    fprintf(source,"\n// Struct data\n\n");

    for(StructDef* def : structs){
      fprintf(source,"Array<String> META_%.*s_Members = {%.*s_MemberNames,ARRAY_SIZE(%.*s_MemberNames)};\n",UNPACK_SS(def->name),UNPACK_SS(def->name),UNPACK_SS(def->name));
    }
    
    fprintf(source,"\n// Raw C array auxiliary to tables\n\n");
  
    // TODO: We only handle 2 levels currently. Either data is simple or an array of simple.
    // TODO: Also only strings. We might want numbers, but strings are the main complication currently.
    for(TableDef* def : tables){
      for(int i = 0; i <  def->dataTable.size; i++){
        Array<DataValue*> row  =  def->dataTable[i];

        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def->parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(val->type == DataValueType_FUNCTION){
            DataValue* data = EvaluateFunction(val->asFunc,temp);
            val = data;
          }
          
          if(val->type == DataValueType_ARRAY){
            fprintf(source,"static %.*s %.*s_%.*s_aux%d[] = {\n",UNPACK_SS(typeDef->name),UNPACK_SS(def->structTypename),UNPACK_SS(name),i);

            bool first = true;
            for(DataValue* child : val->asArray){
              Assert(child->type == DataValueType_SINGLE); // TODO: Only 2 levels handled for now
              if(!first){
                fprintf(source,",\n");
              }
              fprintf(source,"  STRING(\"%.*s\")",UNPACK_SS(child->asStr));
              first = false;
            }
            fprintf(source,"\n};\n");
          }
        }
      }
    }

    fprintf(source,"\n// Raw C array Tables\n\n");
    
    for(TableDef* def : tables){
      fprintf(source,"static %.*s_GenType %.*s_Raw[] = {\n",UNPACK_SS(def->structTypename),UNPACK_SS(def->structTypename));
 
      bool firstOuter = true;
      for(int i = 0; i <  def->dataTable.size; i++){
        Array<DataValue*> row  =  def->dataTable[i];

        if(!firstOuter){
          fprintf(source,",\n");
        }

        fprintf(source,"  {");
        bool first = true;
        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def->parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

          if(!first){
            fprintf(source,",");
          }

          if(IsEnum(typeDef->name) || val->type == DataValueType_SINGLE){
            String repr = DefaultRepr(val,typeDef,temp);
            fprintf(source,"%.*s",UNPACK_SS(repr));
          } else {
            fprintf(source,"{%.*s_%.*s_aux%d,ARRAY_SIZE(%.*s_%.*s_aux%d)}",UNPACK_SS(def->structTypename),UNPACK_SS(name),i,UNPACK_SS(def->structTypename),UNPACK_SS(name),i);
          }

          first = false;
        }
        fprintf(source,"}");
        
        firstOuter = false;
      }

      fprintf(source,"\n};\n");
    }

    fprintf(source,"\n// Tables\n\n");

    for(TableDef* def : tables){
      String n = def->structTypename;
      fprintf(source,"Array<%.*s_GenType> %.*s = {%.*s_Raw,ARRAY_SIZE(%.*s_Raw)};\n",UNPACK_SS(n),UNPACK_SS(n),UNPACK_SS(n),UNPACK_SS(n));
    }
    
    fprintf(source,"\n// Define Maps arrays\n\n");

    for(MapDef* def : maps){
      if(!def->isDefineMap){
        continue;
      }

      fprintf(source,"static String %.*s_Raw[] = {\n",UNPACK_SS(def->name));
      bool first = true;
      for(Array<DataValue*> p : def->dataTable){
        // TODO: Only implement for single type. Waht would an array define map look like?
        Assert(p[1]->type == DataValueType_SINGLE);
        if(!first){
          fprintf(source,",\n");
        }
        fprintf(source,"  %.*s",UNPACK_SS(p[0]->asStr));
        first = false;
      }
      fprintf(source,"\n};\n");

      fprintf(source,"Array<String> %.*s = {%.*s_Raw,ARRAY_SIZE(%.*s_Raw)};\n",UNPACK_SS(def->name),UNPACK_SS(def->name),UNPACK_SS(def->name));
    }

    fprintf(source,"\n// Normal Maps\n\n");

    for(MapDef* def : maps){
      // TODO: All these exist because we are just trying to implement something quickly right now, but when the time comes implement this.
      if(def->isDefineMap){
        continue;
      }
      if(!def->isEnumMap){
        continue;
      }
      if(!def->isBijection){
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
      
        fprintf(source,"Opt<%.*s> META_%.*s_Map(%.*s val){\n",UNPACK_SS(pTo.type->name),UNPACK_SS(def->name),UNPACK_SS(pFrom.type->name));
      
        fprintf(source,"  switch(val){\n");

        for(Array<DataValue*> rows : def->dataTable){
          DataValue* fFrom = rows[from];
          DataValue* fTo = rows[to];
        
          String dFrom = DefaultRepr(fFrom,pFrom.type,temp);
          String dTo = DefaultRepr(fTo,pTo.type,temp);

          fprintf(source,"    case %.*s: return %.*s;\n",UNPACK_SS(dFrom),UNPACK_SS(dTo));
        }

        fprintf(source,"    default: return {};\n");
      
        fprintf(source,"  }\n");
        fprintf(source,"  return {};\n");
        fprintf(source,"}\n");
      }
      
      {
        int from = 1;
        int to = 0;
      
        Parameter pFrom = def->parameterList[from];
        Parameter pTo = def->parameterList[to];

        fprintf(source,"Opt<%.*s> META_%.*s_ReverseMap(%.*s val){\n",UNPACK_SS(pTo.type->name),UNPACK_SS(def->name),UNPACK_SS(pFrom.type->name));

        for(Array<DataValue*> rows : def->dataTable){
          DataValue* fFrom = rows[from];
          DataValue* fTo = rows[to];
        
          String dFrom = DefaultRepr(fFrom,pFrom.type,temp);
          String dTo = DefaultRepr(fTo,pTo.type,temp);

          fprintf(source,"  if(val == %.*s){return %.*s;}\n",UNPACK_SS(dFrom),UNPACK_SS(dTo));
        }
      
        fprintf(source,"  return {};\n");
        fprintf(source,"}\n");
      }
    }

    fprintf(source,"\n// Embedded Files\n\n");

    for(FileDef* def : files){
      String path = def->filepathFromRoot;
      String absolute = GetAbsolutePath(path,temp);
      String content = PushFile(temp,absolute);

      // TODO: This is probably a bit slower than needed.
      fprintf(source,"String META_%.*s_Content = STRING(\"",UNPACK_SS(def->name));
      
      for(char ch : content){
        switch(ch){
        case '\n': fprintf(source,"\\n"); break;
        case '\"': fprintf(source,"\\\""); break;
        case '\t': fprintf(source,"\\t"); break;
        case '\\': fprintf(source,"\\"); break;
        case '\r': fprintf(source,"\\r"); break;
        case '\'': fprintf(source,"\'"); break;
        default: fprintf(source,"%c",ch);
        }
      }

      fprintf(source,"\");");
    }
    
  } // source
    
  return 0;
}
