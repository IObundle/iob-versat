#include <cstdio>

#define STRUCT_PARSER

#include "parser.hpp"
#include "utils.hpp"

#include <set>

#define UNNAMED_ENUM_TEMPLATE "unnamed_enum_%d"

static Arena tempArenaInst;
static Arena* tempArena = &tempArenaInst;

struct MemberDef;

// Only support one inheritance for now
#if 0
struct InheritanceDef{
   String name;
   enum {PUBLIC,PRIVATE} type;
   InheritanceDef* next;
};
#endif

struct TemplateParamDef{
   String name;
   TemplateParamDef* next;
};

struct StructDef{
   String fullExpression;
   String name;
   String inherit;
   TemplateParamDef* params;
   MemberDef* members;
   bool isUnion;
};

struct TemplatedDef{
   String baseType;
   TemplateParamDef* next;
};

struct EnumDef{
   String name;
   String values;
};

struct TypedefDef{
   String oldType;
   String newType;
};

struct TypeDef{
   union{
      String simpleType;
      TypedefDef typedefType;
      EnumDef enumType;
      TemplatedDef templatedType;
      StructDef structType;
   };
   enum {SIMPLE,TYPEDEF,ENUM,TEMPLATED,STRUCT} type;
};

struct MemberDef{
   String fullExpression; // In theory, should be a combination of all the below
   TypeDef type;
   String pointersAndReferences;
   String name;
   String arrays;

   MemberDef* next;
};

static Pool<MemberDef> defs = {};
static Pool<TypeDef> typeDefs = {};

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

template<typename T>
T* ReverseList(T* head){
   if(head == nullptr){
      return head;
   }

   T* ptr = nullptr;
   T* next = head;

   while(next != nullptr){
      T* nextNext = next->next;
      next->next = ptr;
      ptr = next;
      next = nextNext;
   }

   return ptr;
}

static String ParseSimpleType(Tokenizer* tok){
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
   }

   peek = tok->PeekToken();
   while(CompareToken(peek,"*") || CompareToken(peek,"&")){
      tok->AdvancePeek(peek);

      peek = tok->PeekToken();
   }

   String res = tok->Point(mark);
   res = TrimWhitespaces(res);

   return res;
}

static StructDef ParseStruct(Tokenizer* tok);

// Add enum members and default init values so that we can generate code that iterates over enums or prints strings automatically

static EnumDef ParseEnum(Tokenizer* tok){
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

static MemberDef* ParseMember(Tokenizer* tok){
   Token token = tok->PeekToken();

   MemberDef def = {};

   if(CompareString(token,"struct") || CompareString(token,"union")){
      StructDef structDef = ParseStruct(tok);

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
      def.type.simpleType = ParseSimpleType(tok);
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
         Token advanceList = tok->PeekUntilDelimiterExpression({"("},{")"},0);
         tok->AdvancePeek(advanceList);
         tok->AssertNextToken(")");
         if(!tok->IfNextToken(";")){
            Token advanceFunctionBody = tok->PeekUntilDelimiterExpression({"{"},{"}"},0);
            tok->AdvancePeek(advanceFunctionBody);
            tok->AssertNextToken("}");
            tok->IfNextToken(";");
         }

         // We are inside a function declaration
         return nullptr;
      }

      Token finalToken = tok->NextToken();

      if(CompareString(finalToken,"(")){
         Token advanceList = tok->PeekUntilDelimiterExpression({"("},{")"},1);
         tok->AdvancePeek(advanceList);
         tok->AssertNextToken(")");
         if(!tok->IfNextToken(";")){
            Token advanceFunctionBody = tok->PeekUntilDelimiterExpression({"{"},{"}"},0);
            tok->AdvancePeek(advanceFunctionBody);
            tok->AssertNextToken("}");
            tok->IfNextToken(";");
         }

         // We are inside a function declaration
         return nullptr;
      } else {
         Assert(CompareString(finalToken,";"));
      }
   }

   MemberDef* mem = defs.Alloc();
   *mem = def;

   return mem;
}

static StructDef ParseStruct(Tokenizer* tok){
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

         def.inherit = ParseSimpleType(tok);
      }
   }

   def.name = name;

   token = tok->PeekToken();
   if(CompareToken(token,";")){
      def.fullExpression = tok->Point(mark);

      return def;
   }

   tok->AssertNextToken("{");

   Token peek = tok->PeekToken();
   if(CompareString(peek,"}")){
      tok->AdvancePeek(peek);
      def.fullExpression = tok->Point(mark);

      return def;
   }

   while(!tok->Done()){
      token = tok->PeekToken();

      if(CompareToken(token,"}")){
         tok->AdvancePeek(token);
         break;
      }

      MemberDef* member = ParseMember(tok);

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

StructDef ParseTemplatedStructDefinition(Tokenizer* tok){
   tok->AssertNextToken("template");
   tok->AssertNextToken("<");

   Token peek = tok->PeekToken();

   TemplateParamDef* ptr = nullptr;
   while(!tok->Done()){
      if(tok->IfPeekToken(">")){
         break;
      }

      tok->AssertNextToken("typename");

      Token parameter = tok->NextToken();

      TemplateParamDef* newParam = PushStruct<TemplateParamDef>(tempArena);
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

   StructDef def = ParseStruct(tok);

   def.params = ReverseList(ptr);

   return def;
}

void ParseHeaderFile(Tokenizer* tok){
   while(!tok->Done()){
      Token type = tok->PeekToken();

      if(CompareString(type,"enum")){
         EnumDef def = ParseEnum(tok);

         TypeDef* newType = typeDefs.Alloc();
         newType->type = TypeDef::ENUM;
         newType->enumType = def;

      } else if(CompareString(type,"struct")){
         StructDef def = ParseStruct(tok);
         if(def.members){
            TypeDef* typeDef = typeDefs.Alloc();
            typeDef->type = TypeDef::STRUCT;
            typeDef->structType = def;
         }
      } else if(CompareString(type,"template")){
         StructDef def = ParseTemplatedStructDefinition(tok);

         if(def.members){
            TypeDef* typeDef = typeDefs.Alloc();
            typeDef->type = TypeDef::STRUCT;
            typeDef->structType = def;
         }
      } else if(CompareString(type,"typedef")){
         tok->AdvancePeek(type);

         Token line = tok->PeekFindUntil(";");

         // Skip typedef of functions by checking if a '(' or a ')' appears in the line
         bool cont = false;
         for(int i = 0; i < line.size; i++){
            if(line[i] == '(' || line[i] == ')'){
               cont = true;
            }
         }
         if(cont){
            continue;
         }

         void* oldMark = tok->Mark();

         Token peek = tok->PeekToken();
         if(CompareString(peek,"struct")){
            StructDef def = ParseStruct(tok);

            if(def.members){
               TypeDef* typeDef = typeDefs.Alloc();
               typeDef->type = TypeDef::STRUCT;
               typeDef->structType = def;
            }
         } else {
            ParseSimpleType(tok);
         }

         String old = tok->Point(oldMark);
         old = TrimWhitespaces(old);

         Token name = tok->NextToken();
         tok->AssertNextToken(";");

         TypeDef* def = typeDefs.Alloc();
         def->type = TypeDef::TYPEDEF;
         def->typedefType.oldType = old;
         def->typedefType.newType = name;

         continue;
      } else {
         tok->AdvancePeek(type);
      }
   }
}

int CountMembers(MemberDef* m){
   int count = 0;
   for(; m; m = m->next){
      if(m->type.type == TypeDef::STRUCT){
         count += CountMembers(m->type.structType.members);
      } else {
         count += 1;
      }
   }

   return count;
}

String GetTypeName(TypeDef* type){
   String name = {};
   switch(type->type){
   case TypeDef::ENUM:{
      name = type->enumType.name;
   }break;
   case TypeDef::SIMPLE:{
      name = type->simpleType;
   }break;
   case TypeDef::STRUCT:{
      name = type->structType.name;
   }break;
   case TypeDef::TEMPLATED:{
      name = type->templatedType.baseType;
   }break;
   case TypeDef::TYPEDEF:{
      name = type->typedefType.oldType;
   }break;
   }

   return name;
}

TypeDef* GetDef(String name){
   for(TypeDef* def : typeDefs){
      String typeName = GetTypeName(def);

      if(CompareString(typeName,name)){
         return def;
      }
   }

   return nullptr;
}

void OutputMembers(FILE* output,String structName,MemberDef* m,bool first){
   for(; m != nullptr; m = m->next){
      if(m->type.type == TypeDef::STRUCT){
         StructDef def = m->type.structType;
         if(def.name.size > 0){
            String name = PushString(tempArena,"%.*s::%.*s",UNPACK_SS(structName),UNPACK_SS(def.name));

            OutputMembers(output,name,m->type.structType.members,first);
         } else {
            OutputMembers(output,structName,m->type.structType.members,first); // anonymous struct
         }
      } else {
         const char* preamble = "        ";
         if(!first){
            preamble = "        ,";
         }

         String typeName = GetTypeName(&m->type);

         if(typeName.size == 0){
            if(m->type.type == TypeDef::ENUM){
               typeName = STRING("int");
            }
         }

         fprintf(output,"%s(Member){",preamble);
         fprintf(output,"GetType(STRING(\"%.*s\"))",UNPACK_SS(typeName));
         fprintf(output,",STRING(\"%.*s\")",UNPACK_SS(m->name));
         fprintf(output,",offsetof(%.*s,%.*s)}\n",UNPACK_SS(structName),UNPACK_SS(m->name));
      }
      first = false;
   }
}

int OutputTemplateMembers(FILE* output,MemberDef* m,int index,bool insideUnion, bool first){
   for(; m != nullptr; m = m->next){
      if(m->type.type == TypeDef::STRUCT){
         StructDef def = m->type.structType;
         int count = OutputTemplateMembers(output,m->type.structType.members,index,def.isUnion,first); // anonymous struct

         if(def.isUnion){
            index += 1;
         } else {
            index += count;
         }
      } else {
         const char* preamble = "        ";
         if(!first){
            preamble = "        ,";
         }
         fprintf(output,"%s(TemplatedMember){",preamble);
         fprintf(output,"STRING(\"%.*s\")",UNPACK_SS(m->type.simpleType));
         fprintf(output,",STRING(\"%.*s\")",UNPACK_SS(m->name));
         fprintf(output,",%d}\n",index);

         if(!insideUnion){
            index += 1;
         }
      }
      first = false;
   }
   return index;
}

String GetBaseType(String name){
   Tokenizer tok(name,"<>*&",{});

   // TODO: This should call parsing simple type. Deal with unsigned and things like that
   SkipQualifiers(&tok);

   Token base = tok.NextToken();

   SkipQualifiers(&tok);

   Token peek = tok.PeekToken();
   if(CompareString(peek,"<")){
      Token templateInst = tok.PeekUntilDelimiterExpression({"<"},{">"},0);
      base = ExtendToken(base,templateInst);
      tok.AdvancePeek(templateInst);
      base = ExtendToken(base,tok.AssertNextToken(">"));
   }

   base = TrimWhitespaces(base);

   return base;
}

bool IsTemplatedParam(TypeDef* def,String memberTypeName){
   if(def->type != TypeDef::STRUCT){
      return false;
   }

   FOREACH_LIST(ptr,def->structType.params){
      if(CompareString(ptr->name,memberTypeName)){
         return true;
      }
   }
   return false;
}

bool DependsOnTemplatedParam(TypeDef* def,String memberTypeName){
   BLOCK_REGION(tempArena);

   String baseName = GetBaseType(memberTypeName);
   Tokenizer tok(baseName,"[]{}();*<>,=",{});

   String simpleType = tok.NextToken();
   bool res = IsTemplatedParam(def,simpleType);

   if(res || (simpleType.size == baseName.size)){
      return res;
   }

   tok.AssertNextToken("<");

   while(!tok.Done()){
      FindFirstResult search = tok.FindFirst({",",">"});
      Assert(search.foundFirst.size);

      if(CompareString(search.foundFirst,",")){
         bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded);
         if(res){
            return res;
         }
         tok.AdvancePeek(search.peekFindNotIncluded);
         tok.AssertNextToken(",");
         continue;
      } else { // ">"
         bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded);
         return res;
      }
   }

   UNREACHABLE;
}

void OutputRegisterTypesFunction(FILE* output){
   std::set<String> seen;

   for(TypeDef* def : typeDefs){
      switch(def->type){
      case TypeDef::SIMPLE:{
      }break;
      case TypeDef::TYPEDEF:{
      }break;
      case TypeDef::ENUM:{
      }break;
      case TypeDef::TEMPLATED:{
         if(CompareString(def->templatedType.baseType,STRING(""))){
            typeDefs.Remove(def);
         }
      }break;
      case TypeDef::STRUCT:{
         if(CompareString(def->structType.name,STRING(""))){
            typeDefs.Remove(def);
         }
      }break;
      default: Assert(false);
      }
   }

   // HACK, with more time, resolve these. Basically just need to make sure that we correctly identify which are template params or not
   #if 0
   seen.insert(STRING("void"));
   seen.insert(STRING("const"));
   seen.insert(STRING("T"));
   seen.insert(STRING("Array<Pair<Key,Data>>"));
   seen.insert(STRING("Pair<Key,Data>"));
   seen.insert(STRING("HashmapNode"));
   seen.insert(STRING("Array<HashmapNode<Key,Data>*>"));
   seen.insert(STRING("Array<HashmapNode<Key,Data>>"));
   seen.insert(STRING("HashmapHeader<Key,Data>"));
   seen.insert(STRING("Data"));
   #endif

   fprintf(output,"#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
   fprintf(output,"static void RegisterParsedTypes(){\n");

   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::TYPEDEF){
         fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TYPEDEF,sizeof(%.*s));\n",UNPACK_SS(def->typedefType.newType),UNPACK_SS(def->typedefType.newType));
         seen.insert(def->typedefType.newType);
      }
   }
   fprintf(output,"\n");
   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::STRUCT && !def->structType.params){
         fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::STRUCT,sizeof(%.*s));\n",UNPACK_SS(def->structType.name),UNPACK_SS(def->structType.name));
         seen.insert(def->structType.name);
      }
   }
   fprintf(output,"\n");

   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::STRUCT){
         for(MemberDef* m = def->structType.members; m; m = m->next){
            String name = GetTypeName(&m->type);

            if(name.size == 0){
               continue;
            }

            if(DependsOnTemplatedParam(def,name)){
               continue;
            }

            String baseName = GetBaseType(name);
            auto iter = seen.find(baseName);

            if(iter == seen.end()){
               if(Contains(name,"<")){
                  fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TEMPLATED_INSTANCE,sizeof(%.*s));\n",UNPACK_SS(baseName),UNPACK_SS(baseName));
               } else {
                  fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::UNKNOWN,sizeof(%.*s));\n",UNPACK_SS(baseName),UNPACK_SS(baseName));
               }

               seen.insert(name);
            }
         }
      }
      if(def->type == TypeDef::TYPEDEF){
         String name = def->typedefType.oldType;
         name = GetBaseType(name);
         auto iter = seen.find(name);

         if(iter == seen.end()){
            if(Contains(name,"<")){
               fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TEMPLATED_INSTANCE,sizeof(%.*s));\n",UNPACK_SS(name),UNPACK_SS(name));
            } else {
               fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::UNKNOWN,sizeof(%.*s));\n",UNPACK_SS(name),UNPACK_SS(name));
            }
            seen.insert(name);
         }
      }
   }
   fprintf(output,"\n");

   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::ENUM){
         fprintf(output,"   RegisterEnum(STRING(\"%.*s\"));\n",UNPACK_SS(def->enumType.name));
         seen.insert(def->enumType.name);
      }
   }
   fprintf(output,"\n");

   fprintf(output,"   static String templateArgs[] = {\n");
   bool first = true;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && def->structType.params)){
         continue;
      }

      for(TemplateParamDef* a = def->structType.params; a != nullptr; a = a->next){
         if(first){
            fprintf(output,"        ");
            first = false;
         } else {
            fprintf(output,"        ,");
         }
         fprintf(output,"STRING(\"%.*s\")\n",UNPACK_SS(a->name));
      }
   }
   fprintf(output,"   };\n\n");

   int index = 0;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && def->structType.params)){
         continue;
      }

      int count = 0;
      for(TemplateParamDef* a = def->structType.params; a != nullptr; a = a->next){
         count += 1;
         seen.insert(a->name);
      }

      fprintf(output,"   RegisterTemplate(STRING(\"%.*s\"),(Array<String>){&templateArgs[%d],%d});\n",UNPACK_SS(def->structType.name),index,count);
      index += count;
   }
   fprintf(output,"\n");

   for(TypeDef* def : typeDefs){
      if(def->type != TypeDef::TYPEDEF){
         continue;
      }

      fprintf(output,"   RegisterTypedef(STRING(\"%.*s\"),",UNPACK_SS(def->typedefType.oldType));
      fprintf(output,"STRING(\"%.*s\"));\n",UNPACK_SS(def->typedefType.newType));
   }
   fprintf(output,"\n");

   fprintf(output,"   static TemplatedMember templateMembers[] = {\n");
   first = true;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && def->structType.params)){
         continue;
      }

      OutputTemplateMembers(output,def->structType.members,0,def->structType.isUnion,first);
      first = false;
   }
   fprintf(output,"   };\n\n");

   index = 0;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && def->structType.params)){
         continue;
      }

      int size = CountMembers(def->structType.members);
      fprintf(output,"   RegisterTemplateMembers(STRING(\"%.*s\"),(Array<TemplatedMember>){&templateMembers[%d],%d});\n",UNPACK_SS(def->structType.name),index,size);
      index += size;
   }
   fprintf(output,"\n");

   fprintf(output,"   static Member members[] = {\n");
   index = 0;
   first = true;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && !def->structType.params)){
         continue;
      }

      if(def->structType.inherit.size){
         TypeDef* inheritDef = GetDef(def->structType.inherit);

         OutputMembers(output,inheritDef->structType.name,inheritDef->structType.members,first);
      }

      OutputMembers(output,def->structType.name,def->structType.members,first);
      first = false;
   }
   fprintf(output,"   };\n\n");

   index = 0;
   for(TypeDef* def : typeDefs){
      if(!(def->type == TypeDef::STRUCT && !def->structType.params)){
         continue;
      }

      int size = CountMembers(def->structType.members);

      if(def->structType.inherit.size){
         TypeDef* inheritDef = GetDef(def->structType.inherit);

         size += CountMembers(inheritDef->structType.members);
      }


      fprintf(output,"   RegisterStructMembers(STRING(\"%.*s\"),(Array<Member>){&members[%d],%d});\n",UNPACK_SS(def->structType.name),index,size);
      index += size;
   }
   fprintf(output,"\n");

   #if 1
   for(String type : seen){
      // HACK, with more time, resolve these. Basically just need to make sure that we correctly identify which are template params or not
      if(CompareString(type,"Array<Pair<Key,Data>>")){
         continue;
      }
      if(CompareString(type,"Pair<Key,Data>")){
         continue;
      }
      if(CompareString(type,"Array<HashmapNode<Key,Data>>")){
         continue;
      }
      if(CompareString(type,"Array<HashmapNode<Key,Data>*>")){
         continue;
      }
      if(CompareString(type,"HashmapHeader<Key,Data>")){
         continue;
      }

      String baseName = GetBaseType(type);

      if(Contains(type,"<")){
         fprintf(output,"   InstantiateTemplate(STRING(\"%.*s\"));\n",UNPACK_SS(baseName));
      }
   }
   #endif

   fprintf(output,"\n}\n");
}

String OutputMember(MemberDef* def){
   Byte* mark = MarkArena(tempArena);

   PushString(tempArena,"%.*s | ",UNPACK_SS(def->type.simpleType));
   PushString(tempArena,"%.*s",UNPACK_SS(def->name));
   if(def->arrays.size){
      PushString(tempArena,"%.*s",UNPACK_SS(def->arrays));
   }

   String res = PointArena(tempArena,mark);
   return res;
}

//#define STANDALONE

#ifdef STANDALONE
// Empty impl otherwise debug won't work on template engine
void StartDebugTerminal(){
}
void DebugTerminal(Value val){
}

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Error, need at least 3 arguments: <program> <outputFile> <inputFile1> ...");

      return 0;
   }

   tempArenaInst = InitArena(Megabyte(256));

   for(int i = 0; i < argc - 2; i++){
      String content = PushFile(tempArena,argv[2+i]);

      if(content.size < 0){
         printf("Failed to open file: %s\n",argv[2+i]);
         return 0;
      }

      Tokenizer tok(content,"[]{}();*<>,=",{});
      ParseHeaderFile(&tok);
   }

   {
   TypeDef* voidType = typeDefs.Alloc();
   voidType->type = TypeDef::SIMPLE;
   voidType->simpleType = STRING("void");
   }

   const char* test = "template<typename T> struct std::vector{T* mem; int size; int allocated;};";
   Tokenizer tok(STRING(test),"[]{}();*<>,=",{});
   ParseHeaderFile(&tok);

   FILE* output = OpenFileAndCreateDirectories(argv[1],"w");

   OutputRegisterTypesFunction(output);

   return 0;
}
#endif



