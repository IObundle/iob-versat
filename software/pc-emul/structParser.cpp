#include <cstdio>

#include "parser.hpp"
#include "utils.hpp"

struct SimpleMemberInfo;

struct SimpleTypeInfo{
   SizedString name;
   SimpleMemberInfo* members;
   SimpleTypeInfo* templateType;
   int pointers;
   bool isStruct; // or union
   bool isTemplated;

   SimpleTypeInfo* next;
};

struct SimpleMemberInfo{
   SizedString name;
   SimpleTypeInfo* type;
   SizedString arrayExpression;
   bool hasArray;

   SimpleMemberInfo* next;
};

static Arena tempArenaInst;
static Arena* tempArena = &tempArenaInst;
//static SimpleTypeInfo* types;
static SimpleTypeInfo* structures;

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

      break;
   }
}

#if 0
static SimpleTypeInfo* NameToType(SizedString name){
   for(SimpleTypeInfo* ptr = types; ptr != nullptr; ptr = ptr->next){
      if(CompareString(ptr->name,name)){
         return ptr;
      }
   }

   SimpleTypeInfo* type = PushStruct(tempArena,SimpleTypeInfo);
   type->name = PushString(tempArena,name);

   type->next = types;
   types = type;

   return type;
}
#endif

// No struct or union parsing
static SimpleTypeInfo* ParseSimpleType(Tokenizer* tok){
   SimpleTypeInfo* res = PushStruct(tempArena,SimpleTypeInfo);

   SkipQualifiers(tok);

   Token type = tok->NextToken();

   Token peek = tok->PeekToken();
   if(CompareToken(peek,"<")){ // Template
      tok->AdvancePeek(peek);

      SimpleTypeInfo* templateType = ParseSimpleType(tok);

      templateType->next = res->templateType;
      res->templateType = templateType;
      res->isTemplated = true;

      Token last = tok->AssertNextToken(">");

      type = ExtendToken(type,last);
   }

   res->name = type;

   peek = tok->PeekToken();
   while(CompareToken(peek,"*")){
      tok->AdvancePeek(peek);
      res->pointers += 1;
      peek = tok->PeekToken();
   }

   return res;
}

static SimpleTypeInfo* ParseStruct(Tokenizer* tok,SizedString outsideStructName,bool insideStruct);

static SimpleMemberInfo* ParseMember(Tokenizer* tok,SizedString outsideStructName){
   SimpleMemberInfo* member = PushStruct(tempArena,SimpleMemberInfo);
   Token token = tok->PeekToken();

   if(CompareString(token,"struct") || CompareString(token,"union")){
      member->type = ParseStruct(tok,outsideStructName,true);

      Token next = tok->NextToken();
      if(CompareString(next,";")){ // Unnamed struct
         member->name.size = 0;
      } else {
         member->name = next;

         // No array, for now
         tok->AssertNextToken(";");
      }
   } else if(CompareString(token,"enum")){
      tok->AdvancePeek(token);

      Token peek = tok->PeekToken();
      Token enumName = {};
      if(!CompareToken(peek,"{")){
         enumName = tok->NextToken();

         if(outsideStructName.size > 0){
            Byte* ptr = PushBytes(tempArena,1024);

            sprintf(ptr,"%.*s::%.*s",UNPACK_SS(outsideStructName),UNPACK_SS(enumName));

            enumName = MakeSizedString(ptr);
         }
      } else {
         enumName = MAKE_SIZED_STRING("int");
      }

      SimpleTypeInfo* type = PushStruct(tempArena,SimpleTypeInfo);

      type->name = enumName;
      member->type = type;

      peek = tok->PeekToken();
      if(CompareToken(peek,"{")){
         tok->NextFindUntil("}");
         tok->AssertNextToken("}");
      }

      member->name = tok->NextToken();
      tok->AssertNextToken(";");
   } else {
      SimpleTypeInfo* simple = ParseSimpleType(tok);

      simple->next = member->type;
      member->type = simple;

      Token name = tok->NextToken();
      member->name = name;

      Token peek = tok->PeekToken();
      if(CompareToken(peek,"[")){
         tok->AdvancePeek(peek);

         Token peek = tok->NextFindUntil("]");

         member->arrayExpression = peek;
         member->hasArray = true;

         tok->AssertNextToken("]");
      }

      tok->AssertNextToken(";");
   }

   return member;
}

static SimpleTypeInfo* ParseStruct(Tokenizer* tok,SizedString outsideStructName, bool insideStruct){
   SimpleTypeInfo* structInfo = PushStruct(tempArena,SimpleTypeInfo);

   Token token = tok->NextToken();
   Assert(CompareToken(token,"struct") || CompareToken(token,"union"));

   structInfo->isStruct = true;

   token = tok->PeekToken();
   if(CompareToken(token,"{")){ // Unnamed
      structInfo->name.size = 0;
   } else { // Named struct
      Token name = tok->NextToken();

      if(insideStruct){
         Byte* ptr = PushBytes(tempArena,1024);

         sprintf(ptr,"%.*s::%.*s",UNPACK_SS(outsideStructName),UNPACK_SS(name));

         structInfo->name = MakeSizedString(ptr);
      } else {
         structInfo->name = name;
      }
   }

   token = tok->PeekToken();
   if(CompareToken(token,";")){
      return structInfo;
   }
   tok->AssertNextToken("{");

   while(1){
      token = tok->PeekToken();

      if(CompareToken(token,"}")){
         tok->AdvancePeek(token);
         break;
      }

      SimpleMemberInfo* info = ParseMember(tok,structInfo->name);

      if(tok->Done()){
         break;
      }

      info->next = structInfo->members;
      structInfo->members = info;
   }

   return structInfo;
}

void ParseHeaderFile(Tokenizer* tok){
   while(!tok->Done()){
      Token skip = tok->PeekFindUntil("struct");

      if(skip.size == -1){
         tok->Finish();
         break;
      }
      tok->AdvancePeek(skip);

      SimpleTypeInfo* info = ParseStruct(tok,MakeSizedString(""),false);

      info->next = structures;
      structures = info;
   }
}

char* GenerateTypeExpression(SimpleMemberInfo* m){
   static char buffer[1024];

   char* ptr = buffer;
   if(m->type->name.size == 0){
      buffer[0] = '\0';
   } else {
      ptr += sprintf(ptr,"%.*s",UNPACK_SS(m->type->name));
      for(int x = 0; x < m->type->pointers; x++){
         ptr += sprintf(ptr,"*");
      }
      if(m->hasArray){
         ptr += sprintf(ptr,"[%.*s]",UNPACK_SS(m->arrayExpression));
      }
   }

   return buffer;
}

/*
TODO: just refactor the majority of this code, if you can't read it in plain english, it's probably too complicated.
      Remove the unnecessary calculations, use the sizeof and offsetof for the arrays. Just assume that we only have one dimensional arrays or one pointer of indirection and no more
*/

struct IterateSubmembers{
   SimpleMemberInfo* currentMember[16];
   int stackIndex;
};

SimpleMemberInfo* Next(IterateSubmembers* iter){
   while(iter->stackIndex >= 0){
      SimpleMemberInfo*& ptr = iter->currentMember[iter->stackIndex];
      ptr = ptr->next;

      if(!ptr){
         iter->stackIndex -= 1;
         continue;
      }

      if(ptr->type->name.size == 0){
         iter->stackIndex += 1;
         iter->currentMember[iter->stackIndex] = ptr->type->members;
      }

      break;
   }

   if(iter->stackIndex < 0){
      return nullptr;
   }

   return iter->currentMember[iter->stackIndex];
}

char* GetFullInstanceName(IterateSubmembers* iter){
   static char buffer[1024];

   char* ptr = buffer;
   bool first = true;
   for(int i = 0; i <= iter->stackIndex; i++){
      if(iter->currentMember[i]->name.size){
         if(!first){
            ptr += sprintf(ptr,".");
         }
         ptr += sprintf(ptr,"%.*s",UNPACK_SS(iter->currentMember[i]->name));
         first = false;
      }
   }

   return buffer;
}

void OutputRegisterTypesFunction(FILE* output){
   fprintf(output,"static void RegisterComplexTypes(){\n");

   for(SimpleTypeInfo* info = structures; info != nullptr; info = info->next){
      fprintf(output,"  RegisterStruct(A(%.*s),{\n",UNPACK_SS(info->name));

      bool first = true;
      IterateSubmembers iter = {};
      iter.currentMember[0] = info->members;

      for(SimpleMemberInfo* m = info->members; m != nullptr; m = Next(&iter)){
         SizedString arrayElements = MAKE_SIZED_STRING("0");
         SizedString hasArray = MAKE_SIZED_STRING("false");
         if(m->hasArray){
            arrayElements = m->arrayExpression;
            hasArray = MAKE_SIZED_STRING("true");
         }

         const char* preamble = "               ";
         if(!first){
            preamble = "              ,";
         } else {
            first = false;
         }

         fprintf(output,"%s((Member){",preamble);

         fprintf(output,"\"%.*s\"",UNPACK_SS(m->type->name));
         fprintf(output,",\"%s\"",GetFullInstanceName(&iter));
         fprintf(output,",sizeof(%s)",GenerateTypeExpression(m));

         if(CompareString(m->type->name,"void")){
            fprintf(output,",4");
         } else {
            fprintf(output,",sizeof(%.*s)",UNPACK_SS(m->type->name));
         }

         fprintf(output,",offsetof(%.*s,%s)",UNPACK_SS(info->name),GetFullInstanceName(&iter));
         fprintf(output,",%d",m->type->pointers); // Ptr
         fprintf(output,",%.*s",UNPACK_SS(arrayElements)); // array elements
         fprintf(output,",%.*s",UNPACK_SS(hasArray)); // has array

         fprintf(output,"})\n");

      }
      fprintf(output,"             });\n");
   }
   fprintf(output,"}\n");
}

//#define STANDALONE

#ifdef STANDALONE
struct Teste{
   struct T1{
      int info[3];
   } lol;
   struct{
      int what[4];
   };
};

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Error, need at least 3 arguments: <program> <outputFile> <inputFile1> ...");

      return 0;
   }

   printf("%d\n",offsetof(Teste,lol.info));

   InitArena(tempArena,Megabyte(64));

   for(int i = 0; i < argc - 2; i++){
      SizedString content = PushFile(tempArena,argv[2+i]);

      if(content.size < 0){
         printf("Failed to open file: %s\n",argv[2+i]);
         return 0;
      }

      Tokenizer tok(content,"[]{};*<>",{});
      ParseHeaderFile(&tok);
   }

   FILE* output = fopen(argv[1],"w");
   OutputRegisterTypesFunction(output);

   return 0;
}
#endif





