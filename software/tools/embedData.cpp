#include <cstdio>

#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

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

struct Defs{
  Array<EnumDef*> enums;
  Array<TableDef*> tables;
};

#define AssertToken(TOKENIZER,STR) (TOKENIZER)->AssertNextToken(STR)

EnumDef* ParseEnum(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  AssertToken(tok,"enum");

  String enumTypeName = tok->NextToken();

  AssertToken(tok,"{");

  auto memberList = PushArenaList<Pair<String,String>>(temp);
  while(!tok->Done()){
    String name = tok->NextToken();

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

  AssertToken(tok,"(");

  auto typeList = PushArenaList<Pair<TypeDef*,Token>>(temp);
  while(!tok->Done()){
    TypeDef* def = ParseTypeDef(tok,out);

    Token name = tok->NextToken();
    
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
  
void ReportError(Token tok,const char* error){
  printf("%s",error);
  printf("    Token: %.*s\n",UNPACK_SS(tok));
}

Defs* ParseContent(String content,Arena* out){
  TEMP_REGION(temp,out);
  Tokenizer tokInst(content,"!@#$%^&*()-+={[}]\\|~`;:'\",./?",{"//","/*","*/"});
  Tokenizer* tok = &tokInst;
  
  auto enumList = PushArenaList<EnumDef*>(temp);
  auto tableList = PushArenaList<TableDef*>(temp);

  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"enum")){
      EnumDef* def = ParseEnum(tok,out);
      *enumList->PushElem() = def;
    } else if(CompareString(peek,"table")){
      TableDef* def = ParseTable(tok,out);
      *tableList->PushElem() = def;
    } else {
      ReportError(peek,"Unexpected token at global scope");
      tok->AdvancePeek();
    }
  }

  Defs* defs = PushStruct<Defs>(out);
  defs->enums = PushArrayFromList(out,enumList);
  defs->tables = PushArrayFromList(out,tableList);

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
  
    fprintf(header,"#include \"utils.hpp\"\n");

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
  }
  
  {
    FILE* source = fopen(StaticFormat("%.*s",UNPACK_SS(sourceName)),"w");
    if(!source){
      printf("Error opening file for output: %.*s",UNPACK_SS(sourceName));
      return -1;
    }
    DEFER_CLOSE_FILE(source);

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
            fprintf(source,"static %.*s %.*s_aux%d[] = {\n",UNPACK_SS(typeDef->name),UNPACK_SS(name),i);

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
            fprintf(source,"%.*s_aux%d",UNPACK_SS(name),i);
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
  } // source
    
  return 0;
}
