#include "structParsing.hpp"

#include <cstdio>
#include <unordered_set>

#include "parser.hpp"

#define UNNAMED_ENUM_TEMPLATE "unnamed_enum_%d"

static Pool<MemberDef> defs = {};
static Pool<TypeDef> typeDefs = {};

void ParseHeaderFile(Tokenizer* tok,Arena* arena){
   while(!tok->Done()){
      Token type = tok->PeekToken();

      if(CompareString(type,"enum")){
         EnumDef def = ParseEnum(tok);

         TypeDef* newType = typeDefs.Alloc();
         newType->type = TypeDef::ENUM;
         newType->enumType = def;

      } else if(CompareString(type,"struct")){
         StructDef def = ParseStruct(tok,arena);
         if(def.members){
            TypeDef* typeDef = typeDefs.Alloc();
            typeDef->type = TypeDef::STRUCT;
            typeDef->structType = def;
         }
      } else if(CompareString(type,"template")){
         StructDef def = ParseTemplatedStructDefinition(tok,arena);

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
            StructDef def = ParseStruct(tok,arena);

            if(def.members){
               TypeDef* typeDef = typeDefs.Alloc();
               typeDef->type = TypeDef::STRUCT;
               typeDef->structType = def;
            }
         } else {
            ParseSimpleFullType(tok);
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

void OutputMembers(FILE* output,String structName,MemberDef* m,bool first,Arena* arena){
   BLOCK_REGION(arena);
   for(; m != nullptr; m = m->next){
      if(m->type.type == TypeDef::STRUCT){
         StructDef def = m->type.structType;
         if(def.name.size > 0){
            String name = PushString(arena,"%.*s::%.*s",UNPACK_SS(structName),UNPACK_SS(def.name));

            OutputMembers(output,name,m->type.structType.members,first,arena);
         } else {
            OutputMembers(output,structName,m->type.structType.members,first,arena); // anonymous struct
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

bool DependsOnTemplatedParam(TypeDef* def,String memberTypeName,Arena* arena){
   BLOCK_REGION(arena);

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
         bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded,arena);
         if(res){
            return res;
         }
         tok.AdvancePeek(search.peekFindNotIncluded);
         tok.AssertNextToken(",");
         continue;
      } else { // ">"
         bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded,arena);
         return res;
      }
   }

   UNREACHABLE;
}

void OutputRegisterTypesFunction(FILE* output,Arena* arena){
   std::unordered_set<String> seen;

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

   seen.insert(STRING("void"));

   fprintf(output,"#include \"utils.hpp\"\n");
   fprintf(output,"#include \"memory.hpp\"\n");
   fprintf(output,"#include \"type.hpp\"\n\n");

   fprintf(output,"void RegisterParsedTypes(){\n");

   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::TYPEDEF){
         fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TYPEDEF,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(def->typedefType.newType),UNPACK_SS(def->typedefType.newType),UNPACK_SS(def->typedefType.newType));
         seen.insert(def->typedefType.newType);
      }
   }
   fprintf(output,"\n");
   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::STRUCT && !def->structType.params){
         fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::STRUCT,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(def->structType.name),UNPACK_SS(def->structType.name),UNPACK_SS(def->structType.name));
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

            if(DependsOnTemplatedParam(def,name,arena)){
               continue;
            }

            String baseName = GetBaseType(name);
            auto iter = seen.find(baseName);

            if(iter == seen.end()){
               if(Contains(name,"<")){
                  fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TEMPLATED_INSTANCE,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(baseName),UNPACK_SS(baseName),UNPACK_SS(baseName));
               } else {
                  fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::UNKNOWN,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(baseName),UNPACK_SS(baseName),UNPACK_SS(baseName));
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
               fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::TEMPLATED_INSTANCE,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(name),UNPACK_SS(name),UNPACK_SS(name));
            } else {
               fprintf(output,"   RegisterOpaqueType(STRING(\"%.*s\"),Type::UNKNOWN,sizeof(%.*s),alignof(%.*s));\n",UNPACK_SS(name),UNPACK_SS(name),UNPACK_SS(name));
            }
            seen.insert(name);
         }
      }
   }
   fprintf(output,"\n");

   for(TypeDef* def : typeDefs){
      if(def->type == TypeDef::ENUM){
         Tokenizer tok(def->enumType.values,",{}",{});

         fprintf(output,"\tstatic Pair<String,int> %.*sData[] = {\n",UNPACK_SS(def->enumType.name));

         tok.AssertNextToken("{");
         bool first = true;
         while(!tok.Done()){
            Token name = tok.NextToken();

            if(first){
               first = false;
            } else {
               fprintf(output,",\n");
            }

            fprintf(output,"\t\t{STRING(\"%.*s\"),(int) %.*s::%.*s}",UNPACK_SS(name),UNPACK_SS(def->enumType.name),UNPACK_SS(name));

            String peek = tok.PeekFindUntil(",");

            if(peek.size == -1){
               break;
            }

            tok.AdvancePeek(peek);
            tok.AssertNextToken(",");
         }
         fprintf(output,"};\n\n");
         fprintf(output,"   RegisterEnum(STRING(\"%.*s\"),C_ARRAY_TO_ARRAY(%.*sData));\n\n",UNPACK_SS(def->enumType.name),UNPACK_SS(def->enumType.name));

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

         OutputMembers(output,inheritDef->structType.name,inheritDef->structType.members,first,arena);
      }

      OutputMembers(output,def->structType.name,def->structType.members,first,arena);
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

String OutputMember(MemberDef* def,Arena* arena){
   Byte* mark = MarkArena(arena);

   PushString(arena,"%.*s | ",UNPACK_SS(def->type.simpleType));
   PushString(arena,"%.*s",UNPACK_SS(def->name));
   if(def->arrays.size){
      PushString(arena,"%.*s",UNPACK_SS(def->arrays));
   }

   String res = PointArena(arena,mark);
   return res;
}

//#define STANDALONE

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Error, need at least 3 arguments: <program> <outputFile> <inputFile1> ...");

      return 0;
   }

   Arena tempArena = InitArena(Megabyte(256));
   Arena* arena = &tempArena;

   FILE* output = OpenFileAndCreateDirectories(argv[1],"w");

   if(!output){
      printf("Failed to open file: %s\n",argv[1]);
      printf("Exiting\n");
      return -1;
   }

   fprintf(output,"#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n\n");
   for(int i = 0; i < argc - 2; i++){
      String name = ExtractFilenameOnly(STRING(argv[2+i]));
      fprintf(output,"#include \"%.*s\"\n",UNPACK_SS(name));
   }

   for(int i = 0; i < argc - 2; i++){
      String content = PushFile(arena,argv[2+i]);

      if(content.size < 0){
         printf("Failed to open file: %s\n",argv[2+i]);
         return 0;
      }

      Tokenizer tok(content,"[]{}();*<>,=",{});
      ParseHeaderFile(&tok,arena);
   }

   {
   TypeDef* voidType = typeDefs.Alloc();
   voidType->type = TypeDef::SIMPLE;
   voidType->simpleType = STRING("void");
   }

   const char* test = "template<typename T> struct std::vector{T* mem; int size; int allocated;};";
   Tokenizer tok(STRING(test),"[]{}();*<>,=",{});
   ParseHeaderFile(&tok,arena);

   OutputRegisterTypesFunction(output,arena);

   return 0;
}
