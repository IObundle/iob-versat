#include <cstdio>

#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

// TODO: Support numbers inside tables. Support more levels of data hierarchies.
//       More importantly, implement stuff as they are needed. No point in trying to push for anything complex right now.
// TODO: Implement special code for bit flags (needed currently by the AddressGen code).
//       Do not know yet if I want them to be enum, need to test how easy C++ lets enum bit flags work.
// TODO: Embed file could just become part of the embed data approach. No point in having two different approaches. Altought the embed data approach would need to become more generic
//       in order to do what the embed file approach does in a data oriented manner.

void ReportError(String content,Token faultyToken,const char* error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("\n");
  printf("%s\n",error);
  printf("%.*s\n",UNPACK_SS(loc));
  printf("\n");
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

struct TableDef{
  String structTypename;
  Array<Pair<TypeDef*,Token>> typeList;
  Array<Array<DataValue*>> dataTable;
};

struct MapDef{
  String name;
  Array<Pair<String,DataValue*>> maps;
  bool isDefineMap;
};

struct Defs{
  Array<EnumDef*> enums;
  Array<TableDef*> tables;
  Array<MapDef*> maps;
};

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

      while(!tok->Done() && !tok->IfPeekToken(",")){
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

  EnumDef* res = PushStruct<EnumDef>(out);
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

TableDef* ParseTable(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"table");

  Token tableStructName = tok->NextToken();
  CheckIdentifier(tableStructName);

  AssertToken(tok,"(");

  auto typeList = PushArenaList<Pair<TypeDef*,Token>>(temp);
  while(!tok->Done()){
    TypeDef* def = ParseTypeDef(tok,out);
    
    Token name = tok->NextToken();
    CheckIdentifier(name);

    *typeList->PushElem() = {def,name};
    
    bool seenComma = tok->IfNextToken(",");
    
    if(tok->IfPeekToken(")")){
      break;
    }
  }
  AssertToken(tok,")");

  Array<Pair<TypeDef*,Token>> defs = PushArrayFromList(out,typeList);
  Assert(defs.size > 0);

  AssertToken(tok,"{");
  
  auto valueTableList = PushArenaList<Array<DataValue*>>(temp);
  while(!tok->Done()){
    auto valueList = PushArenaList<DataValue*>(temp);
    for(int i = 0; i < defs.size; i++){
      DataValue* val = ParseValue(tok,out);
      *valueList->PushElem() = val;

      if(i != defs.size - 1){
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

  AssertToken(tok,"}");
  AssertToken(tok,";");
  
  TableDef* def = PushStruct<TableDef>(out);
  def->structTypename = tableStructName;
  def->typeList = defs;
  def->dataTable = PushArrayFromList(out,valueTableList);

  return def;
}

MapDef* ParseMap(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

  Token type = tok->NextToken();
  bool isDefineMap = false;

  if(CompareString(type,"define_map")){
    isDefineMap = true;
  } else if(CompareString(type,"map")){
    ReportError(tok,type,"Not implemented yet");
  } else {
    printf("Should not be possible to reach this point\n");
    DEBUG_BREAK();
  }
  
  //AssertToken(tok,"map");

  Token mapName = tok->NextToken();
  CheckIdentifier(mapName);

  AssertToken(tok,"{");

  auto memberList = PushArenaList<Pair<String,DataValue*>>(temp);
  while(!tok->Done()){
    Token name = tok->NextToken();
    CheckIdentifier(name);

    AssertToken(tok,":");

    DataValue* data = ParseValue(tok,out);
    
    *memberList->PushElem() = {name,data};

    tok->IfNextToken(",");

    if(tok->IfPeekToken("}")){
      break;
    }
  }

  AssertToken(tok,"}");
  AssertToken(tok,";");

  MapDef* res = PushStruct<MapDef>(out);
  res->name = mapName;
  res->maps = PushArrayFromList(out,memberList);
  res->isDefineMap = isDefineMap;
  
  return res;
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

// 0 - exe name
// 1 - definition file
// 2 - output name
int main(int argc,const char* argv[]){
  if(argc < 3){
    fprintf(stderr,"Need at least 3 arguments: <exe> <defFile> <outputFilename> \n");
    return -1;
  }
  
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
      for(Pair<TypeDef*,Token> p : def->typeList){
        TypeDef* typeDef = p.first;
        String name = p.second;
      
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

      for(Pair<String,DataValue*> p : def->maps){
        // TODO: Only implement for single type. Waht would an array define map look like?
        Assert(p.second->type == DataValueType_SINGLE);
        fprintf(header,"#define %.*s STRING(\"%.*s\")\n",UNPACK_SS(p.first),UNPACK_SS(p.second->asStr));
      }

      fprintf(header,"\nextern Array<String> %.*s;\n\n",UNPACK_SS(def->name));
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
          Pair<TypeDef*,Token> p =  def->typeList[ii];
          TypeDef* typeDef = p.first;
          String name = p.second;

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

    auto IsEnum = [defs](String name) -> bool{
      for(EnumDef* def : defs->enums){
        if(def->name == name){
          return true;
        }
      }
      return false;
    };
    
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
          Pair<TypeDef*,Token> p =  def->typeList[ii];
          TypeDef* typeDef = p.first;
          String name = p.second;

          if(!first){
            fprintf(source,",");
          }

          if(IsEnum(typeDef->name)){
            fprintf(source,"%.*s",UNPACK_SS(val->asStr));
          } else if(val->type == DataValueType_SINGLE){
            fprintf(source,"STRING(\"%.*s\")",UNPACK_SS(val->asStr));
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
      for(Pair<String,DataValue*> p : def->maps){
        // TODO: Only implement for single type. Waht would an array define map look like?
        Assert(p.second->type == DataValueType_SINGLE);
        if(!first){
          fprintf(source,",\n");
        }
        fprintf(source,"  %.*s",UNPACK_SS(p.first));
        first = false;
      }
      fprintf(source,"\n};\n");

      fprintf(source,"Array<String> %.*s = {%.*s_Raw,ARRAY_SIZE(%.*s_Raw)};\n",UNPACK_SS(def->name),UNPACK_SS(def->name),UNPACK_SS(def->name));
    }
  } // source
    
  return 0;
}
