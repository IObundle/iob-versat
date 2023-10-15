#include "structParsing.hpp"

#include <cstdio>

//#include "versatPrivate.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <set>

#define UNNAMED_ENUM_TEMPLATE "unnamed_enum_%d"

void SkipQualifiers(Tokenizer* tok){
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
      if(CompareToken(peek,"public:")){
         tok->AdvancePeek(peek);
         continue;
      }
      if(CompareToken(peek,"private:")){
         tok->AdvancePeek(peek);
         continue;
      }

      break;
   }
}

String ParseSimpleType(Tokenizer* tok){
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
      peek = tok->PeekToken();
   }

   #if 0
   while(CompareToken(peek,"*") || CompareToken(peek,"&")){
      tok->AdvancePeek(peek);

      peek = tok->PeekToken();
   }
   #endif

   String res = tok->Point(mark);
   res = TrimWhitespaces(res);

   return res;
}

String ParseSimpleFullType(Tokenizer* tok){
   void* mark = tok->Mark();

   ParseSimpleType(tok);

   Token peek = tok->PeekToken();
   while(CompareToken(peek,"*") || CompareToken(peek,"&")){
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
   if(!CompareToken(peek,"{")){ // Unnamed enum
      enumName = tok->NextToken();
   }

   peek = tok->PeekToken();
   void* valuesMark = tok->Mark();
   String values = {};
   if(CompareToken(peek,"{")){
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

MemberDef* ParseMember(Tokenizer* tok,Arena* out){
   Token token = tok->PeekToken();

   MemberDef def = {};

   if(CompareString(token,"struct") || CompareString(token,"union")){
      StructDef structDef = ParseStruct(tok,out);

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

      Token peek = tok->PeekToken();
      void* arraysMark = tok->Mark();
      if(CompareToken(peek,"[")){
         tok->AdvancePeek(peek);
         /*Token arrayExpression =*/ tok->NextFindUntil("]");
         tok->AssertNextToken("]");
 
         def.arrays = tok->Point(arraysMark);
      } else if(CompareToken(peek,"(")){
         Token advanceList = tok->PeekIncludingDelimiterExpression({"("},{")"},0);
         tok->AdvancePeek(advanceList);
         if(!tok->IfNextToken(";")){
            Token advanceFunctionBody = tok->PeekIncludingDelimiterExpression({"{"},{"}"},0);
            tok->AdvancePeek(advanceFunctionBody);
            tok->IfNextToken(";"); // Consume any extra ';'
         }

         // We are inside a function declaration
         return nullptr;
      }

      Token finalToken = tok->NextToken();

      if(CompareString(finalToken,"(")){
         Token advanceList = tok->PeekIncludingDelimiterExpression({"("},{")"},1);
         tok->AdvancePeek(advanceList);
         if(!tok->IfNextToken(";")){
            Token advanceFunctionBody = tok->PeekIncludingDelimiterExpression({"{"},{"}"},0);
            tok->AdvancePeek(advanceFunctionBody);
            tok->IfNextToken(";"); // Consume any extra ';'
         }

         // We are inside a function declaration
         return nullptr;
      } else {
         Assert(CompareString(finalToken,";"));
      }
   }

   MemberDef* mem = PushStruct<MemberDef>(out);
   *mem = def;

   return mem;
}

StructDef ParseStruct(Tokenizer* tok,Arena* arena){
   void* mark = tok->Mark();

   Token token = tok->NextToken();
   Assert(CompareToken(token,"struct") || CompareToken(token,"union"));
   String name = {};

   StructDef def = {};
   if(CompareToken(token,"union")){
      def.isUnion = true;
   }

   token = tok->PeekToken();
   if(CompareToken(token,"{")){ // Unnamed
   } else { // Named struct
      name = tok->NextToken();

      token = tok->PeekToken();
      if(CompareToken(token,":")){ // inheritance
         tok->AssertNextToken(":");

         tok->AssertNextToken("public"); // Forced to be a public inheritance

         def.inherit = ParseSimpleFullType(tok);
      }
   }

   def.name = name;

   token = tok->PeekToken();
   if(CompareToken(token,";")){
      def.fullExpression = tok->Point(mark);

      return def;
   }

   tok->AssertNextToken("{");

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

	  //UNHANDLED_ERROR; // LEFT: The program was not finding the Repr that it was supposed to find.
	                     //       Or maybe the data was not being caried over.
	  //printf("%.*s %.*s\n",UNPACK_SS(name),UNPACK_SS(token));
      if(CompareToken(token,"}")){
         tok->AdvancePeek(token);
		 break;
      } else if(CompareString(token,STRING("//"))){ // The // symbol and the /* */ must be part of the special chars of the tokenizer
		tok->AdvancePeek(token);

		Token specialType = tok->NextToken();

		if(CompareString(specialType,"Repr")){
		  tok->AssertNextToken(":");

		  Token formatString = tok->NextFindUntil("\n");

		  if(def.representationFormat.size){
			LogWarn(LogModule::PARSER,"Struct already has a representation format: %.*s",UNPACK_SS(name));
		  }
		  def.representationFormat = TrimWhitespaces(formatString);
		  continue;
		} else {
		  // We are in a normal comment
		  tok->NextFindUntil("\n");
		  continue;
		}
	  } else if(CompareString(token,STRING("/*"))){
		tok->NextFindUntil("*/");
		continue;
	  }
	  tok->keepComments = false;

      MemberDef* member = ParseMember(tok,arena);

      if(member == nullptr){
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

   StructDef def = ParseStruct(tok,arena);

   def.params = ReverseList(ptr);

   return def;
}
