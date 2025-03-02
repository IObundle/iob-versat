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
  String name;
  Array<Pair<String,String>> valuesNamesWithValuesIfExist;
};

struct TypeDef{
  Token name;
  bool isArray;
};

enum DataValueType{
  DataValueType_SINGLE,
  DataValueType_ARRAY
};

struct DataValue{
  DataValueType type;
  union{
    String asStr;
    Array<DataValue*> asArray;
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

struct Defs{
  Array<EnumDef*> enums;
  Array<TableDef*> tables;
  Array<MapDef*> maps;
};

Pool<EnumDef> globalEnums = {};

static bool IsEnum(String name){
  for(EnumDef* def : globalEnums){
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

  EnumDef* res = globalEnums.Alloc();
  //EnumDef* res = PushStruct<EnumDef>(out);
  res->name = enumTypeName;
  res->valuesNamesWithValuesIfExist = PushArrayFromList(out,memberList);

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

DataValue* ParseValue(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

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
  
  TableDef* def = PushStruct<TableDef>(out);
  def->structTypename = tableStructName;
  def->parameterList = defs;
  def->dataTable = table;

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

    MapDef* res = PushStruct<MapDef>(out);
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

    MapDef* res = PushStruct<MapDef>(out);
    res->name = mapName;
    res->dataTable = table;
    res->isDefineMap = isDefineMap;

    return res;
  }

  NOT_POSSIBLE();
  return nullptr;
}

Defs* ParseContent(String content,Arena* out){
  TEMP_REGION(temp,out);
  Tokenizer tokInst(content,"!@#$%^&*()-+={[}]\\|~`;:'\",./?",{"//","/*","*/"});
  Tokenizer* tok = &tokInst;
  
  auto enumList = PushArenaList<EnumDef*>(temp);
  auto tableList = PushArenaList<TableDef*>(temp);
  auto mapList = PushArenaList<MapDef*>(temp);

  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"enum")){
      EnumDef* def = ParseEnum(tok,out);
      *enumList->PushElem() = def;
    } else if(CompareString(peek,"table")){
      TableDef* def = ParseTable(tok,out);
      *tableList->PushElem() = def;
    } else if(CompareString(peek,"map") || CompareString(peek,"define_map")){
      MapDef* def = ParseMap(tok,out);
      *mapList->PushElem() = def;
    } else {
      ReportError(tok,peek,"Unexpected token at global scope");
      tok->AdvancePeek();
    }
  }

  Defs* defs = PushStruct<Defs>(out);
  defs->enums = PushArrayFromList(out,enumList);
  defs->tables = PushArrayFromList(out,tableList);
  defs->maps = PushArrayFromList(out,mapList);
  
  return defs;
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
  
  TEMP_REGION(temp,nullptr);

  String content = PushFile(temp,defFilePath);

  Defs* defs = ParseContent(content,temp);

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

    fprintf(header,"\n// Enum definition\n\n");

    for(EnumDef* def : defs->enums){
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

    fprintf(header,"\n// Struct definition\n\n");

    for(TableDef* def : defs->tables){
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
    
    for(TableDef* def : defs->tables){
      String name = def->structTypename;

      fprintf(header,"extern Array<%.*s_GenType> %.*s;\n",UNPACK_SS(name),UNPACK_SS(name));
    }

    fprintf(header,"\n// Define Maps (the arrays store the data, not the keys)\n\n");

    for(MapDef* def : defs->maps){
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

    for(MapDef* def : defs->maps){
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

    fprintf(source,"\n// Raw C array auxiliary to tables\n\n");
  
    // TODO: We only handle 2 levels currently. Either data is simple or an array of simple.
    // TODO: Also only strings. We might want numbers, but strings are the main complication currently.
    for(TableDef* def : defs->tables){
      for(int i = 0; i <  def->dataTable.size; i++){
        Array<DataValue*> row  =  def->dataTable[i];

        for(int ii = 0; ii <  row.size; ii++){
          DataValue* val  =  row[ii];
          Parameter p =  def->parameterList[ii];
          TypeDef* typeDef = p.type;
          String name = p.name;

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
    
    for(TableDef* def : defs->tables){
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
            fprintf(source,"%.*s_%.*s_aux%d",UNPACK_SS(def->structTypename),UNPACK_SS(name),i);
          }

          first = false;
        }
        fprintf(source,"}");
        
        firstOuter = false;
      }

      fprintf(source,"\n};\n");
    }

    fprintf(source,"\n// Tables\n\n");
    
    for(TableDef* def : defs->tables){
      String n = def->structTypename;
      fprintf(source,"Array<%.*s_GenType> %.*s = {%.*s_Raw,ARRAY_SIZE(%.*s_Raw)};\n",UNPACK_SS(n),UNPACK_SS(n),UNPACK_SS(n),UNPACK_SS(n));
    }

    fprintf(source,"\n// Define Maps arrays\n\n");

    for(MapDef* def : defs->maps){
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

    for(MapDef* def : defs->maps){
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
  } // source
    
  return 0;
}
