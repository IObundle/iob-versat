#include "structParsing.hpp"

#include <cstdio>

#include "debug.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <set>

#define UNNAMED_ENUM_TEMPLATE "unnamed_enum_%d"

void SkipQualifiers(Tokenizer* tok){
  while(1){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"const")){
      tok->AdvancePeek(peek);
      continue;
    }
    if(CompareString(peek,"volatile")){
      tok->AdvancePeek(peek);
      continue;
    }
    if(CompareString(peek,"static")){
      tok->AdvancePeek(peek);
      continue;
    }
    if(CompareString(peek,"inline")){
      tok->AdvancePeek(peek);
      continue;
    }
    if(CompareString(peek,"public:")){
      tok->AdvancePeek(peek);
      continue;
    }
    if(CompareString(peek,"private:")){
      tok->AdvancePeek(peek);
      continue;
    }

    break;
  }
}

String ParseFundamentalType(Tokenizer* tok){
  String accepted[] = {STRING("signed"),
                       STRING("unsigned"),
                       STRING("char"),
                       STRING("short"),
                       STRING("int"),
                       STRING("long")};
  
  auto mark = tok->Mark();

  while(1){
    Token name = tok->PeekToken();

    bool doContinue = false;
    for(String& str : accepted){
      if(CompareString(name,str)){
        tok->AdvancePeek(name);
        doContinue = true;
        break;
      }
    }

    if(doContinue) continue;

    break;
  }

  String res = tok->Point(mark);
  return res;
}

String ParseSimpleType(Tokenizer* tok){
  SkipQualifiers(tok);

  auto mark = tok->Mark();
  String name = ParseFundamentalType(tok);
  
  if(name.size <= 0){
    tok->NextToken();
  }
  
  Token peek = tok->PeekToken();
  if(CompareString(peek,"<")){ // Template
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
    peek = tok->PeekToken();
  }

  String res = tok->Point(mark);
  res = TrimWhitespaces(res);

  return res;
}

String ParseSimpleFullType(Tokenizer* tok){
  auto mark = tok->Mark();

  ParseSimpleType(tok);

  Token peek = tok->PeekToken();
  while(CompareString(peek,"*") || CompareString(peek,"&")){
    tok->AdvancePeek(peek);

    peek = tok->PeekToken();
  }

  String res = tok->Point(mark);
  res = TrimWhitespaces(res);

  return res;
}

// Add enum members and default init values so that we can generate code that iterates over enums or prints strings automatically
EnumDef ParseEnum(Tokenizer* tok){
  tok->AssertNextToken("enum");

  Token peek = tok->PeekToken();
  Token enumName = {};
  if(!CompareString(peek,"{")){ // Unnamed enum
      enumName = tok->NextToken();
  }

  peek = tok->PeekToken();
  auto valuesMark = tok->Mark();
  String values = {};
  if(CompareString(peek,"{")){
    tok->AdvancePeek(peek);

    while(1){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"}")){
        tok->AdvancePeek(peek);

        values = tok->Point(valuesMark);
        break;
      }
      if(CompareString(peek,",")){
        tok->AdvancePeek(peek);
      }

      /*Token enumMemberName = */ tok->NextToken();

      Token value = {};
      peek = tok->PeekToken();
      if(CompareString(peek,"=")){
        tok->AdvancePeek(peek);

        value = tok->NextToken();
      }
    }
  }

  EnumDef def = {};
  def.name = enumName;
  def.values = values;

  return def;
}

void ConsumeUntilLineTerminator(Tokenizer* tok){
  int insideBracket = 0;
  while(!tok->Done()){
    Token token = tok->NextToken();

    if(CompareString(token,"{")){
      insideBracket += 1;
    }

    if(CompareString(token,"}")){
      insideBracket -= 1;
    }
    
    if(CompareString(token,";") && insideBracket == 0){
      break;
    }
  }
}

bool IsValidIdentifier(String str){
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

MemberDef* ParseMember(Tokenizer* tok,Arena* out){
  Token token = tok->PeekToken();

  MemberDef def = {};

  if(CompareString(token,"struct") || CompareString(token,"union")){
    StructDef structDef = ParseStruct(tok,out).value; // TODO: Actually handle error

    def.type.structType = structDef;
    def.type.type = TypeDef::STRUCT;

    Token next = tok->NextToken();
    if(CompareString(next,";")){ // Unnamed struct
    } else {
      // No array for named structure declarations, for now
      tok->AssertNextToken(";");
    }
  } else if(CompareString(token,"enum")){
    EnumDef enumDef = ParseEnum(tok);

    def.type.enumType = enumDef;
    def.type.type = TypeDef::ENUM;

    def.name = tok->NextToken();
    tok->AssertNextToken(";");
  } else {
    def.type.simpleType = ParseSimpleFullType(tok);
    def.type.type = TypeDef::SIMPLE;

    Token name = tok->NextToken();
    def.name = name;

    if(CompareString(name,"operator")){
      return nullptr;
    }
    
    // Probably inside a function
    if(!IsValidIdentifier(name)){
      ConsumeUntilLineTerminator(tok);
      return nullptr;
    }
    
    Token peek = tok->PeekToken();

    auto arraysMark = tok->Mark();
    if(CompareString(peek,"[")){
      tok->AdvancePeek(peek);
      /*Token arrayExpression =*/ tok->NextFindUntil("]");
      tok->AssertNextToken("]");
 
      def.arrays = tok->Point(arraysMark);
    } else if(CompareString(peek,"(")){
      Token advanceList = tok->PeekIncludingDelimiterExpression({"("},{")"},0).value();
      tok->AdvancePeek(advanceList);
      if(!tok->IfNextToken(";")){
        Token advanceFunctionBody = tok->PeekIncludingDelimiterExpression({"{"},{"}"},0).value();
        tok->AdvancePeek(advanceFunctionBody);
        tok->IfNextToken(";"); // Consume any extra ';'
      }

      // We are inside a function declaration
      return nullptr;
    }

    Token finalToken = tok->NextToken();

    if(CompareString(finalToken,"(")){
      // We are inside a function declaration
      ConsumeUntilLineTerminator(tok);
      return nullptr;
    } else {
#if 0
      while(!tok->Done()){

        Token token = tok->NextToken();
        
        PRINT_STRING(token);
        if(CompareString(token,";")){
          printf("Left\n");
          return nullptr;
        } else if(CompareString(token,"}")){
          printf("Left\n");
          return nullptr;
        }
      }
      return nullptr;
#endif
      
      Assert(CompareString(finalToken,";"));
    }
  }

  MemberDef* mem = PushStruct<MemberDef>(out);
  *mem = def;

  return mem;
}

Result<StructDef,String> ParseStruct(Tokenizer* tok,Arena* arena){
  auto mark = tok->Mark();

  Token token = tok->NextToken();
  Assert(CompareString(token,"struct") || CompareString(token,"union"));
  String name = {};

  StructDef def = {};
  if(CompareString(token,"union")){
    def.isUnion = true;
  }

  token = tok->PeekToken();
  if(CompareString(token,"{")){ // Unnamed
  } else { // Named struct
    name = tok->NextToken();

    token = tok->PeekToken();
    if(CompareString(token,":")){ // inheritance
      tok->AssertNextToken(":");

      tok->AssertNextToken("public"); // Forced to be a public inheritance

      def.inherit = ParseSimpleFullType(tok);
    }
  }

  def.name = name;

  token = tok->PeekToken();
  if(CompareString(token,";")){
    def.fullExpression = tok->Point(mark);

    return def;
  }

  if(!tok->IfNextToken("{")){
    return STRING("Expected {");
  }
  //tok->AssertNextToken("{");

  defer{tok->keepComments = false;}; // TODO: A lit bit of a hack, part of the reason is that the tokenizer should be able to freely change types of special characters and such midway.

  tok->keepComments = true; // TODO: A little bit hacky to get the Repr to work. Need to do this before hand because the peek goes over if it's the first comment.
  Token peek = tok->PeekToken();
  if(CompareString(peek,"}")){
    tok->AdvancePeek(peek);
    def.fullExpression = tok->Point(mark);

    return def;
  }

  while(!tok->Done()){
	tok->keepComments = true;
	token = tok->PeekToken();

    //printf("Attemp to parse a member: %.*s\n",UNPACK_SS(token));
    if(CompareString(token,";")){
      tok->AdvancePeek(token);
      continue;
    }
    
	//UNHANDLED_ERROR; // LEFT: The program was not finding the Repr that it was supposed to find.
	//       Or maybe the data was not being caried over.
	//printf("%.*s %.*s\n",UNPACK_SS(name),UNPACK_SS(token));
    if(CompareString(token,"}")){
      tok->AdvancePeek(token);
	  break;
    } else if(CompareString(token,STRING("//"))){ // The // symbol and the /* */ must be part of the special chars of the tokenizer
		tok->AdvancePeek(token);

		Token specialType = tok->NextToken();

		if(CompareString(specialType,"Repr")){
		  tok->AssertNextToken(":");

		  Token formatString = tok->NextFindUntil("\n").value();

		  if(def.representationFormat.size){
			LogWarn(LogModule::PARSER,"Struct already has a representation format: %.*s",UNPACK_SS(name));
		  }
		  def.representationFormat = TrimWhitespaces(formatString);
		  continue;
		} else {
		  // We are in a normal comment
          tok->AdvancePeek(tok->PeekRemainingLine());
		  tok->NextFindUntil("\n");
		  continue;
		}
	} else if(CompareString(token,STRING("/*"))){
	  tok->NextFindUntil("*/");
	  continue;
	}
	tok->keepComments = false;

    auto mark = tok->Mark();
    MemberDef* member = ParseMember(tok,arena);

    if(member == nullptr){
#if 1
      tok->Rollback(mark);
      int depth = 0;
      while(!tok->Done()){
        Token token = tok->NextToken();
        
        //PRINT_STRING(token);
        if(CompareString(token,"{")){
          depth += 1;
        }
        if(CompareString(token,"}")){
          depth -= 1;
          if(depth <= 0){
            //printf("Left\n");
            break;
          }
        }
        if(depth == 0 && CompareString(token,";")){
          //printf("Left\n");
          break;
        } 
      }
#endif

      continue;
    }

    if(tok->Done()){
      break;
    }

    member->next = def.members;
    def.members = member;
  }

  def.fullExpression = tok->Point(mark);
  def.members = ReverseList(def.members);

  return def;
}

StructDef ParseTemplatedStructDefinition(Tokenizer* tok,Arena* arena){
  tok->AssertNextToken("template");
  tok->AssertNextToken("<");

  Token peek = tok->PeekToken();

  TemplateParamDef* ptr = nullptr;
  while(!tok->Done()){
    if(tok->IfPeekToken(">")){
      break;
    }

    //tok->AssertNextToken("typename");
    Token type = tok->NextToken();

    Token parameter = tok->NextToken();

    TemplateParamDef* newParam = PushStruct<TemplateParamDef>(arena);
    newParam->name = parameter;
    newParam->next = ptr;
    ptr = newParam;

    if(!tok->IfNextToken(",")){
      break;
    }
  }

  tok->AssertNextToken(">");

  peek = tok->PeekToken();
  if(!CompareString(peek,"struct")){
    StructDef def = {};
    return def; // Only parse structs
  }

  Result<StructDef,String> optDef = ParseStruct(tok,arena);

  if(optDef.isError){
    return {};
  }

  StructDef def = optDef.value;
  def.params = ReverseList(ptr);

  return def;
}
