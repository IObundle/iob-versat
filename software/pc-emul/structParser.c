#if MARK

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "utils.h"
#include "parser.h"

static char buffer[1024 * 1024];
static int charBufferSize;
static TypeInfo typeInfoBuffer[1024];
static int typeInfoIndex;
static Member memberBuffer[4*1024];
static int memberIndex;

static int StringCmp(char* str1,char* str2){
   if(str1 == NULL || str2 == NULL){
      if(str1 == NULL && str2 == NULL){
         return 1;
      } else {
         return 0;
      }
   }

   while(*str1 != '\0' && *str2 != '\0'){
      if(*str1 != *str2){
         return 0;
      }

      str1++;
      str2++;
   }

   return (*str1 == *str2);
}

static char* PushString(char* str,int size){
   char* res = &buffer[charBufferSize];

   for(int i = 0; i < size & *str != '\0'; i++){
      buffer[charBufferSize++] = *(str++);
   }

   buffer[charBufferSize++] = '\0';

   return res;
}

static int IsWhitespace(char ch){
   if(ch == '\n' || ch == ' ' || ch == '\t'){
      return 1;
   }

   return 0;
}

static void ConsumeWhitespace(Tokenizer* tok){
   while(1){
      if(tok->index >= tok->size)
         return;

      char ch = tok->content[tok->index];

      if(ch == '\n'){
         tok->index += 1;
         tok->lines += 1;
         continue;
      }

      if(IsWhitespace(ch)){
         tok->index += 1;
         continue;
      }

      return;
   }
   return;
}

static int SingleChar(char ch){
   char options[] = "[](){}+;,*";

   for(int i = 0; options[i] != '\0'; i++){
      if(options[i] == ch){
         return 1;
      }
   }

   return 0;
}

static int CompareStringNoEnd(char* strNoEnd,char* strWithEnd){
   for(int i = 0; strWithEnd[i] != '\0'; i++){
      if(strNoEnd[i] != strWithEnd[i]){
         return 0;
      }
   }

   return 1;
}

static int SpecialChars(Tokenizer* tok,Token* out){
   const char* special[] = {"/*","*/"};

   for(int i = 0; i < 2; i++){
      if(CompareStringNoEnd(&tok->content[tok->index],special[i])){
         int size = strlen(special[i]);

         memcpy(out->str,special[i],size);
         out->size = size;

         return 1;
      }
   }

   return 0;
}

static Token PeekToken(Tokenizer* tok){
   Token token = {};

   while(1){
      ConsumeWhitespace(tok);

      if(tok->index + 1 >= tok->size){
         break;
      }

      if(tok->content[tok->index] == '/' && tok->content[tok->index + 1] == '/'){
         while(tok->index + 1 < tok->size && tok->content[tok->index] != '\n') tok->index += 1;
         tok->index += 1;
         tok->lines += 1;
         continue;
      }

      if(tok->content[tok->index] == '/' && tok->content[tok->index + 1] == '*'){
         while(tok->index + 1 < tok->size &&
               !(tok->content[tok->index] == '*' && tok->content[tok->index + 1] == '/')) {
            tok->index += 1;
            if(tok->content[tok->index] == '\n'){
               tok->lines += 1;
            }
         }
         tok->index += 2;
         continue;
      }

      break;
   }

   #if 0
   if(SpecialChars(tok,&token)){
      return token;
   }
   #endif

   char ch = tok->content[tok->index];

   token.str[0] = ch;
   token.size = 1;

   if(SingleChar(ch)){
      return token;
   }

   if(ch == '\"'){ // Special case for strings
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         token.str[i] = ch;
         token.size = (i + 1);

         if(ch == '\"'){
            token.str[token.size] = '\0';
            return token;
         }
      }
   } else {
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         if(SingleChar(ch) || IsWhitespace(ch)){
            token.str[token.size] = '\0';
            return token;
         }

         token.str[i] = ch;
         token.size = (i + 1);
      }
   }

   token.str[token.size] = '\0';

   return token;
}

static Token NextToken(Tokenizer* tok){
   Token res = PeekToken(tok);

   tok->index += res.size;

   #if 0
   printf("%s\n",res.str);
   #endif

   return res;
};

static void StoreToken(Token token,char* buffer){
   memcpy(buffer,token.str,token.size);
   buffer[token.size] = '\0';
}

static void SkipQualifiers(Tokenizer* tokenizer){
   while(1){
      Token peek = PeekToken(tokenizer);

      if(CompareToken(peek,"const")){
         NextToken(tokenizer);
         continue;
      }
      if(CompareToken(peek,"volatile")){
         NextToken(tokenizer);
         continue;
      }

      break;
   }
}

static int ParseEvalExpression(Tokenizer* tokenizer){
   Token token = NextToken(tokenizer); // Only simple numbers for now

   int val = 0;
   for(int i = 0; i < token.size; i++){
      val *= 10;

      Assert(token.str[i] >= '0' && token.str[i] <= '9');
      val += token.str[i] - '0';
   }

   return val;
}

static TypeInfo* NameToType(char* name){
   for(int i = 0; i < typeInfoIndex; i++){
      if(StringCmp(typeInfoBuffer[i].name,name)){
         return &typeInfoBuffer[i];
      }
   }

   TypeInfo type = {};

   strcpy(type.name,name);

   TypeInfo* res = &typeInfoBuffer[typeInfoIndex];
   typeInfoBuffer[typeInfoIndex++] = type;

   return res;
}

static TypeInfo* AddType(TypeInfo info){
   TypeInfo* type = NameToType(info.name);

   if(info.type == TYPE_INFO_STRUCT_DECL){
      if(type->type == TYPE_INFO_UNKNOWN){
         *type = info;
      }
   } else {
      *type = info;
   }

   return type;
}

static void RegisterSimpleType(char* name,int size){
   TypeInfo info = {};

   info.type = TYPE_INFO_SIMPLES;
   info.size = size;
   strcpy(info.name,name);

   AddType(info);
}

static Type ParseType(Tokenizer* tokenizer);

static Member ParseDeclaration(Tokenizer* tokenizer,int insideStruct){
   Member member = {};

   member.type = ParseType(tokenizer);

   Token token = NextToken(tokenizer);
   StoreToken(token,member.name);

   token = PeekToken(tokenizer);
   if(CompareToken(token,"[")){
      NextToken(tokenizer);

      int size = ParseEvalExpression(tokenizer);

      Assert(size > 0);

      member.extraArrayElements = (size - 1);

      Assert(CompareToken(NextToken(tokenizer),"]"));
   }

   if(!insideStruct) {
      Assert(CompareToken(NextToken(tokenizer),";"));
   } else {
      if(CompareToken(PeekToken(tokenizer),";")){
         NextToken(tokenizer);
      }
   }

   return member;
}

static TypeInfo* ParseStruct(Tokenizer* tokenizer,int insideStruct){
   static int unnamedStructIndex = 0;
   TypeInfo structInfo = {};
   structInfo.type = TYPE_INFO_STRUCT_DECL;

   Token token = NextToken(tokenizer);
   Assert(CompareToken(token,"struct") || CompareToken(token,"union"));

   char* name = "struct ";
   int instanceType = TYPE_INFO_STRUCT;
   if(CompareToken(token,"union")){
      name = "union ";
      instanceType = TYPE_INFO_UNION;
      structInfo.type = TYPE_INFO_UNION_DECL;
   }

   token = PeekToken(tokenizer);

   if(CompareToken(token,"{")){ // Unnamed
      sprintf(structInfo.name,"%sunnamed_type_%d",name,unnamedStructIndex++);
   } else { // Named struct
      token = NextToken(tokenizer);

      strcpy(structInfo.name,name);
      StoreToken(token,&structInfo.name[7]);
   }

   token = PeekToken(tokenizer);

   if(CompareToken(token,";") || !CompareToken(token,"{")){
      return AddType(structInfo);
   } else {
      token = NextToken(tokenizer);
   }

   Assert(CompareToken(token,"{"));

   structInfo.type = instanceType;
   structInfo.members = &memberBuffer[memberIndex];
   while(1){
      token = PeekToken(tokenizer);

      if(CompareToken(token,"}")){
         NextToken(tokenizer);
         break;
      }

      Member info = ParseDeclaration(tokenizer,insideStruct);

      memberBuffer[memberIndex++] = info;
      structInfo.nMembers += 1;
   }

   return AddType(structInfo);
}

static Type ParseType(Tokenizer* tokenizer){
   Type type = {};

   SkipQualifiers(tokenizer);

   Token token = PeekToken(tokenizer);

   char typeStr[512];
   if(CompareToken(token,"struct")){
      TypeInfo* info = ParseStruct(tokenizer,1);
      type.info = info;
   } else if(CompareToken(token,"union")){
      TypeInfo* info = ParseStruct(tokenizer,1);
      type.info = info;
   } else {
      token = NextToken(tokenizer);
      StoreToken(token,typeStr);
      type.info = NameToType(typeStr);
   }

   token = PeekToken(tokenizer);
   while(CompareToken(token,"*")){
      NextToken(tokenizer);
      type.ptr += 1;
      token = PeekToken(tokenizer);
   }

   return type;
}

static TypeInfo* ParseTypedef(Tokenizer* tokenizer){
   Assert(CompareToken(NextToken(tokenizer),"typedef"));

   Type type = ParseType(tokenizer);

   Token nameToken = NextToken(tokenizer); // Typedef name

   Assert(CompareToken(NextToken(tokenizer),";"));

   TypeInfo info = {};

   info.type = TYPE_INFO_ALIAS;
   info.alias = type;
   StoreToken(nameToken,info.name);

   return AddType(info);
}

// Quick and dirty way to ignore function typedefs
static int CheckParenthesisOnLine(Tokenizer* tokenizer){
   int i = tokenizer->index;
   for(; i < tokenizer->size && tokenizer->content[i] != '\n'; i++){
      if(tokenizer->content[i] == '/' && tokenizer->content[i+1] == '/'){
         if(tokenizer->content[i+1] == '/'){
            return 0;
         }
         if(tokenizer->content[i+1] == '*'){
            return 0;
         }
      }

      if(tokenizer->content[i] == '(' || tokenizer->content[i] == ')'){
         return 1;
      }
   }

   return 0;
}

static int GetSize(Type type){
   TypeInfo* info = type.info;

   if(type.ptr){
      return sizeof(void*);
   } else {
      switch(info->type){
         case TYPE_INFO_SIMPLES: {
            return info->size;
         } break;
         case TYPE_INFO_ALIAS: {
            return GetSize(info->alias);
         } break;
         case TYPE_INFO_STRUCT: {
            int sizeAccum = 0;
            for(int i = 0; i < info->nMembers; i++){
               Member member = info->members[i];

               sizeAccum += GetSize(member.type) * (member.extraArrayElements + 1);
            }
            return sizeAccum;
         } break;
         case TYPE_INFO_STRUCT_DECL: {
            Assert(0);
         } break;
         case TYPE_INFO_UNKNOWN: {
            return 4;
         } break;
      }
   }
   Assert(0);
   return 0;
}

static int GetOffset(Type type,int align,char* member){
   int offset = 0;

   TypeInfo* info = type.info;
   Assert(info->type == TYPE_INFO_STRUCT);
   for(int i = 0; i < info->nMembers; i++){
      if(strcmp(info->members[i].name,member) == 0){
         break;
      } else {
         int size = GetSize(info->members[i].type) * (info->members[i].extraArrayElements + 1);

         if(align){
            size = ((size + 3) & ~0x3);
         }

         offset += size;
      }
   }

   return offset;
}

TypeInfoReturn ParseMain(FILE* file){
   char* buffer = (char*) malloc(256*1024*1024*sizeof(char));
   int fileSize = fread(buffer,sizeof(char),256*1024*1024,file);

   Tokenizer tokenizerInst = {};
   Tokenizer* tokenizer = &tokenizerInst;

   tokenizer->content = buffer;
   tokenizer->size = fileSize;

   RegisterSimpleType("char",sizeof(char));
   RegisterSimpleType("int",sizeof(int));
   RegisterSimpleType("int32_t",sizeof(int32_t));
   RegisterSimpleType("bool",sizeof(char));
   RegisterSimpleType("void",sizeof(void));

   int state = 0;
   while(tokenizer->index < tokenizer->size){
      Token token = PeekToken(tokenizer);

      if(CompareToken(token,"struct") && !CheckParenthesisOnLine(tokenizer)){
         ParseStruct(tokenizer,0);
      } else if(CompareToken(token,"typedef") && !CheckParenthesisOnLine(tokenizer)){
         ParseTypedef(tokenizer);
      } else {
         NextToken(tokenizer);
      }
   }

   #if 1
   for(int i = 0; i < typeInfoIndex; i++){
      TypeInfo info = typeInfoBuffer[i];

      Type type = {};
      type.info = &typeInfoBuffer[i];

      printf("%s\n",info.name);

      switch(info.type){
         case TYPE_INFO_SIMPLES:{
            printf("\tSIMPLE: %d\n",GetSize(type));
         }break;
         case TYPE_INFO_ALIAS:{
            printf("\tALIAS: %d %s\n",GetSize(type),info.alias.info->name);
         }break;
         case TYPE_INFO_STRUCT:{
            printf("\tSTRUCT: %d\n",GetSize(type));
            for(int ii = 0; ii < info.nMembers; ii++){
               printf("\t\t%d %s\n",GetSize(info.members[ii].type) * (info.members[ii].extraArrayElements + 1),info.members[ii].name);
            }
         }break;
         case TYPE_INFO_UNKNOWN:{
            printf("\tUNKNOWN\n");
         }break;
      };
   }
   #endif

   TypeInfoReturn ret = {};

   ret.info = typeInfoBuffer;
   ret.nInfo = typeInfoIndex;

   return ret;
}

#endif



































