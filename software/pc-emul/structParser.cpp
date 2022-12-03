#include <cstdio>

#define STRUCT_PARSER

#include "parser.hpp"
#include "utils.hpp"

#define UNNAMED_TEMPLATE "unnamed_%d"
#define UNNAMED_ENUM_TEMPLATE "unnamed_enum_%d"

static Arena tempArenaInst;
static Arena* tempArena = &tempArenaInst;

static Pool<Type> types = {};
static Pool<Member> members = {};

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

static Type* ParseSimpleType(Tokenizer* tok);

// Checks if type already exists
static Type* FindOrCreateType(SizedString name){
   for(Type* ptr : types){
      if(CompareString(ptr->name,name)){
         return ptr;
      }
   }

   Type* type = types.Alloc();

   type->name = name;
   type->type = Type::UNKNOWN;

   return type;
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

static Type* ParseSimpleType(Tokenizer* tok){
   SkipQualifiers(tok);

   Token name = tok->NextToken();
   Token peek = tok->PeekToken();
   Type* templateBase = nullptr;
   TemplateArg* templateArgs = nullptr;
   if(CompareToken(peek,"<")){ // Template
      templateBase = FindOrCreateType(name);

      templateBase->type = Type::TEMPLATED_STRUCT;

      Token nameRemaining = tok->PeekFindIncluding(">");
      name = ExtendToken(name,nameRemaining);
      tok->AdvancePeek(peek);

      while(1){
         TemplateArg* arg = PushStruct(tempArena,TemplateArg);

         arg->type = ParseSimpleType(tok);
         arg->next = templateArgs;
         templateArgs = arg;

         Token peek = tok->PeekToken();

         if(CompareString(peek,",")){
            tok->AdvancePeek(peek);
         } else if(CompareString(peek,">")){
            tok->AdvancePeek(peek);
            break;
         }
      }
   }

   Type* type = FindOrCreateType(name);

   if(templateBase){
      type->type = Type::TEMPLATED_INSTANCE;
      type->templateBase = templateBase;
      type->templateArgs = templateArgs;
   }

   peek = tok->PeekToken();
   while(CompareToken(peek,"*")){
      tok->AdvancePeek(peek);

      Byte* nameBuffer = PushBytes(tempArena,type->name.size + 2); // extra * and null terminator
      sprintf(nameBuffer,"%.*s*",UNPACK_SS(type->name));

      Type* pointer = FindOrCreateType(MakeSizedString(nameBuffer,type->name.size + 1));

      pointer->type = Type::POINTER;
      pointer->pointerType = type;

      type = pointer;

      peek = tok->PeekToken();
   }

   return type;
}

static Type* ParseStruct(Tokenizer* tok,SizedString outsideStructName);

// Add enum members and default init values so that we can generate code that iterates over enums or prints strings automatically

static Type* ParseEnum(Tokenizer* tok,SizedString outsideStructName){
   tok->AssertNextToken("enum");

   Token peek = tok->PeekToken();
   Token enumName = {};
   if(CompareToken(peek,"{")){ // Unnamed enum
      static int nUnnamedEnum = 1;

      if(outsideStructName.size > 0){
         enumName = PushString(tempArena,"%.*s::" UNNAMED_ENUM_TEMPLATE,UNPACK_SS(outsideStructName),nUnnamedEnum++);
      } else {
         enumName = PushString(tempArena,UNNAMED_ENUM_TEMPLATE,nUnnamedEnum++);
      }
   } else {
      enumName = tok->NextToken();

      if(outsideStructName.size > 0){
         enumName = PushString(tempArena,"%.*s::%.*s",UNPACK_SS(outsideStructName),UNPACK_SS(enumName));
      }
   }

   Type* type = FindOrCreateType(enumName);

   type->type = Type::ENUM;
   type->name = enumName;
   peek = tok->PeekToken();
   if(CompareToken(peek,"{")){
      tok->AdvancePeek(peek);

      Assert(type->enumMembers == nullptr);

      while(1){
         Token peek = tok->PeekToken();

         if(CompareString(peek,"}")){
            tok->AdvancePeek(peek);
            break;
         }
         if(CompareString(peek,",")){
            tok->AdvancePeek(peek);
         }

         Token enumMemberName = tok->NextToken();

         Token value = {};
         peek = tok->PeekToken();
         if(CompareString(peek,"=")){
            tok->AdvancePeek(peek);

            value = tok->NextToken();
         }

         if(value.size == 0){
            if(type->enumMembers == nullptr){
               value = PushString(tempArena,"0");
            } else {
               value = PushString(tempArena,"((int) %.*s) + 1",UNPACK_SS(type->enumMembers->name));
            }
         }

         EnumMember* newMember = PushStruct(tempArena,EnumMember);
         newMember->name = enumMemberName;
         newMember->value = value;
         newMember->next = type->enumMembers;

         type->enumMembers = newMember;
      }
   }

   return type;
}

static Member* ParseMember(Tokenizer* tok,SizedString outsideStructName){
   Member* member = members.Alloc();
   member->index = -1;
   Token token = tok->PeekToken();

   if(CompareString(token,"struct") || CompareString(token,"union")){
      member->type = ParseStruct(tok,outsideStructName);

      Token next = tok->NextToken();
      if(CompareString(next,";")){ // Unnamed struct
         member->name.size = 0;
      } else {
         member->name = next;

         // No array for named structure declarations, for now
         tok->AssertNextToken(";");
      }
   } else if(CompareString(token,"enum")){
      Type* enumType = ParseEnum(tok,outsideStructName);

      member->type = enumType;
      member->name = tok->NextToken();
      tok->AssertNextToken(";");
   } else {
      Type* type = ParseSimpleType(tok);

      Token name = tok->NextToken();
      member->name = name;

      Token peek = tok->PeekToken();
      if(CompareToken(peek,"[")){
         tok->AdvancePeek(peek);

         Token arrayExpression = tok->NextFindUntil("]");

         member->arrayExpression = arrayExpression;

         Byte* nameBuffer = PushBytes(tempArena,type->name.size + arrayExpression.size + 3); // extra [] and null terminator
         sprintf(nameBuffer,"%.*s[%.*s]",UNPACK_SS(type->name),UNPACK_SS(arrayExpression));

         Type* arrayType = FindOrCreateType(MakeSizedString(nameBuffer,type->name.size + arrayExpression.size + 2));

         arrayType->type = Type::ARRAY;
         arrayType->arrayType = type;

         type = arrayType;

         tok->AssertNextToken("]");
      }

      member->type = type;

      tok->AssertNextToken(";");
   }

   return member;
}

static Type* ParseStruct(Tokenizer* tok,SizedString outsideStructName){
   Token token = tok->NextToken();
   Assert(CompareToken(token,"struct") || CompareToken(token,"union"));

   SizedString name = {};

   Type* baseType = nullptr;

   token = tok->PeekToken();
   if(CompareToken(token,"{")){ // Unnamed
      static int nUnnamed = 1;

      if(outsideStructName.size > 0){
         name = PushString(tempArena,"%.*s::" UNNAMED_TEMPLATE,UNPACK_SS(outsideStructName),nUnnamed);
      } else {
         name = PushString(tempArena,UNNAMED_TEMPLATE,nUnnamed++);
      }
   } else { // Named struct
      name = tok->NextToken();

      if(outsideStructName.size > 0){
         name = PushString(tempArena,"%.*s::%.*s",UNPACK_SS(outsideStructName),UNPACK_SS(name));
      }

      token = tok->PeekToken();
      if(CompareToken(token,":")){ // inheritance
         tok->AssertNextToken(":");
         tok->AssertNextToken("public"); // Forced to be a public inheritance

         baseType = ParseSimpleType(tok);
      }
   }

   Type* structInfo = FindOrCreateType(name);

   structInfo->type = Type::STRUCT;
   structInfo->baseType = baseType;

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

      Member* info = ParseMember(tok,structInfo->name);

      if(tok->Done()){
         break;
      }

      info->structType = structInfo;
      info->next = structInfo->members;
      structInfo->members = info;
   }

   return structInfo;
}

Type* ParseTemplatedStructDefinition(Tokenizer* tok){
   tok->AssertNextToken("template");
   tok->AssertNextToken("<");

   Token peek = tok->PeekToken();

   TemplateArg* arguments = nullptr;

   while(!tok->Done()){
      if(tok->IfPeekToken(">")){
         break;
      }

      tok->AssertNextToken("typename");

      Token parameter = tok->NextToken();

      Type* type = FindOrCreateType(parameter);
      Assert(type->type == Type::UNKNOWN || type->type == Type::TEMPLATE_PARAMETER);

      type->type = Type::TEMPLATE_PARAMETER;

      TemplateArg* newArgument = PushStruct(tempArena,TemplateArg);
      newArgument->type = type;
      newArgument->next = arguments;

      arguments = newArgument;

      if(!tok->IfNextToken(",")){
         break;
      }
   }

   tok->AssertNextToken(">");

   peek = tok->PeekToken();
   if(!CompareString(peek,"struct")){
      return nullptr; // Only parse structs, for now
   }

   Type* type = ParseStruct(tok,MakeSizedString(""));

   if(arguments){
      type->type = Type::TEMPLATED_STRUCT;
      type->templateArgs = ReverseList(arguments);
   }

   return type;
}

void ParseHeaderFile(Tokenizer* tok){
   while(!tok->Done()){
      Token type = tok->PeekToken();

      if(CompareString(type,"enum")){
         ParseEnum(tok,MakeSizedString(""));
      } else if(CompareString(type,"struct")){
         ParseStruct(tok,MakeSizedString(""));
      } else if(CompareString(type,"template")){
         ParseTemplatedStructDefinition(tok);
      } else if(CompareString(type,"typedef")){
         tok->AdvancePeek(type);

         Token line = tok->PeekFindUntil(";");

         // Skip typedef of functions by checking if a '(' or a ')' appears in the line
         bool cont = false;
         for(int i = 0; i < line.size; i++){
            if(line.str[i] == '(' || line.str[i] == ')'){
               cont = true;
            }
         }
         if(cont){
            continue;
         }

         Token peek = tok->PeekToken();
         Type* type = nullptr;
         if(CompareString(peek,"struct")){
            type = ParseStruct(tok,MakeSizedString(""));
         } else {
            type = ParseSimpleType(tok);
         }

         Token name = tok->NextToken();
         tok->AssertNextToken(";");

         Type* typedefType = FindOrCreateType(name);

         typedefType->type = Type::TYPEDEF;
         typedefType->typedefType = type;
         typedefType->size = type->size;

         continue;
      } else {
         tok->AdvancePeek(type);
      }
   }
}

struct IterateSubmembers{
   Member* currentMember[16];
   int stackIndex;
};

Member* Next(IterateSubmembers* iter){
   while(iter->stackIndex >= 0){
      Member*& ptr = iter->currentMember[iter->stackIndex];
      ptr = ptr->next;

      if(!ptr){
         iter->stackIndex -= 1;
         continue;
      }

      if(ptr->name.size == 0){
         iter->stackIndex += 1;
         iter->currentMember[iter->stackIndex] = ptr->type->members;
         Assert(ptr->type->type == Type::STRUCT);
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

         if(i == iter->stackIndex){
            ptr += sprintf(ptr,"%.*s",UNPACK_SS(iter->currentMember[i]->name));
         } else {
            ptr += sprintf(ptr,"%.*s",UNPACK_SS(iter->currentMember[i]->structType->name));
         }

         first = false;
      }
   }

   return buffer;
}

/*

Left to do:

   Add, either to the struct parser or to the type, info for the std classes used ex: std::vector and stuff like that
   Finish the process of registering templated structures
   Finish rewriting the type.cpp and test it

*/

Type* GetBaseType(Type* type){
   switch (type->type){
      case Type::BASE: return type;
      case Type::STRUCT: return type;
      case Type::POINTER: return GetBaseType(type->pointerType);
      case Type::ARRAY: return GetBaseType(type->arrayType);
      case Type::TEMPLATED_STRUCT: return type;
      case Type::TEMPLATED_INSTANCE: return type;
      case Type::TEMPLATE_PARAMETER: return type;
      case Type::TYPEDEF: return GetBaseType(type->typedefType);
      case Type::ENUM: return type;
      case Type::UNKNOWN: return type;
      default:
         NOT_IMPLEMENTED;
   };

   NOT_POSSIBLE;
   return nullptr;
}

const char* TypeEnumName(Type* type){
   switch (type->type){
      case Type::BASE: return "BASE";
      case Type::STRUCT: return "STRUCT";
      case Type::POINTER: return "POINTER";
      case Type::ARRAY: return "ARRAY";
      case Type::TEMPLATED_STRUCT: return "TEMPLATED_STRUCT";
      case Type::TEMPLATED_INSTANCE: return "TEMPLATED_INSTANCE";
      case Type::TEMPLATE_PARAMETER: return "TEMPLATE_PARAMETER";
      case Type::TYPEDEF: return "TYPEDEF";
      case Type::ENUM: return "ENUM";
      case Type::UNKNOWN: return "UNKNOWN";
      default:
         NOT_IMPLEMENTED;
   };

   NOT_POSSIBLE;
   return nullptr;
}

bool IsUnnamedStruct(Type* type){
   Tokenizer tok(type->name,"",{"::"});

   while(!tok.Done()){
      Token token = tok.NextToken();

      if(CheckFormat(UNNAMED_TEMPLATE,token)){
         return true;
      }
   }

   return false;
}

bool IsUnnamedEnum(Type* type){
   Tokenizer tok(type->name,"",{"::"});

   while(!tok.Done()){
      Token token = tok.NextToken();

      if(CheckFormat(UNNAMED_ENUM_TEMPLATE,token)){
         return true;
      }
   }

   return false;
}

void OutputRegisterTypesFunction(FILE* output){
   #if 0
   for(Type* type : types){
      if(type->type == Type::TEMPLATED_INSTANCE){
         Tokenizer tok(type->name,"<>",{});

         Token baseName = tok.NextToken();

         Type* base = FindOrCreateType(baseName);
         Assert(base->type == Type::TEMPLATED_STRUCT);

         tok.AssertNextToken("<");

         while(1){
            Token argName = tok.NextToken();
         }
      }
   }
   #endif

   #if 0
   for(Type* type : types){
      #if 1
      printf("%.*s %s\n",UNPACK_SS(type->name),TypeEnumName(type));
      #endif

      switch(type->type){
         case Type::BASE:;
         case Type::POINTER:;
         case Type::ARRAY:;
         case Type::UNKNOWN:;
         case Type::TEMPLATE_PARAMETER:;
            break;

         #if 0
         case Type::TYPEDEF:{
            char* fullName = TypeFullName(type->typedefType);
            printf("  %s\n",fullName);
         } break;
         #endif

         #if 0
         case Type::STRUCT:{
            for(Member* m = type->members; m != nullptr; m = m->next){
               char* fullName = TypeFullName(m->type);
               printf("  %s %.*s\n",fullName,UNPACK_SS(m->name));
            }
         } break;
         #endif

         #if 0
         case Type::ENUM:{
            for(EnumMember* m = type->enumMembers; m != nullptr; m = m->next){
               printf("  %.*s = %.*s\n",UNPACK_SS(m->name),UNPACK_SS(m->value));
            }
         } break;
         #endif

         #if 0
         case Type::TEMPLATED_STRUCT:{
            for(Member* m = type->members; m != nullptr; m = m->next){
               char* fullName = TypeFullName(m->type);
               printf("  %s %.*s\n",fullName,UNPACK_SS(m->name));
            }
            printf("  Args: ");
            for(TemplateArg* arg = type->templateArgs; arg != nullptr; arg = arg->next){
               printf(" %.*s ",UNPACK_SS(arg->name));
            }
            printf("\n");
         } break;
         #endif

         #if 1
         case Type::TEMPLATED_INSTANCE:{
            printf("  Base: %.*s\n",UNPACK_SS(type->templateBase->name));

            for(TemplateArg* a = type->templateArgs; a != nullptr; a = a->next){
               printf("    %.*s\n",UNPACK_SS(a->type->name));
            }

            #if 0
            for(Member* m = type->members; m != nullptr; m = m->next){
               printf("  %.*s %.*s\n",UNPACK_SS(m->type->name),UNPACK_SS(m->name));
            }
            #endif
         } break;
         #endif

      };
   }
   #endif

   fprintf(output,"#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
   fprintf(output,"static void RegisterParsedTypes(){\n");

   for(Type* info : types){
      if(info->type != Type::BASE){
         continue;
      }

      if(CompareString(info->name,"void")){
         fprintf(output,"    RegisterSimpleType(MakeSizedString(\"%.*s\"),sizeof(char));\n",UNPACK_SS(info->name));
      } else {
         fprintf(output,"    RegisterSimpleType(MakeSizedString(\"%.*s\"),sizeof(%.*s));\n",UNPACK_SS(info->name),UNPACK_SS(info->name));
      }
   }
   fprintf(output,"\n");

   for(Type* info : types){
      if(!(info->type == Type::STRUCT || info->type == Type::TYPEDEF || info->type == Type::TEMPLATED_INSTANCE || info->type == Type::UNKNOWN)){
         continue;
      }

      if(info->type == Type::STRUCT && IsUnnamedStruct(info)){
         continue;
      }

      fprintf(output,"    RegisterOpaqueType(MakeSizedString(\"%.*s\"),Type::%s,sizeof(%.*s));\n",UNPACK_SS(info->name),TypeEnumName(info),UNPACK_SS(info->name));
   }
   fprintf(output,"\n");

   for(Type* info : types){
      if(info->type != Type::TEMPLATED_STRUCT){
         continue;
      }

      fprintf(output,"    RegisterOpaqueTemplateStruct(MakeSizedString(\"%.*s\"));\n",UNPACK_SS(info->name));
   }
   fprintf(output,"\n");

   for(Type* info : types){
      if(info->type != Type::ENUM){
         continue;
      }

      fprintf(output,"    RegisterEnum(MakeSizedString(\"%.*s\"));\n",UNPACK_SS(info->name));
   }
   fprintf(output,"\n");

   for(Type* info : types){
      if(info->type != Type::TEMPLATED_INSTANCE){
         continue;
      }

      fprintf(output,"    RegisterTemplateInstance(MakeSizedString(\"%.*s\"),MakeSizedString(\"%.*s\"),{",UNPACK_SS(info->name),UNPACK_SS(info->templateBase->name));

      bool first = true;
      for(TemplateArg* a = info->templateArgs; a != nullptr; a = a->next){
         if(first){
            first = false;
         } else {
            fprintf(output,",");
         }

         fprintf(output,"MakeSizedString(\"%.*s\")",UNPACK_SS(a->type->name));
      }
      fprintf(output,"});\n");
   }
   fprintf(output,"\n");

   for(Type* info : types){
      if(info->type != Type::TYPEDEF){
         continue;
      }

      fprintf(output,"    RegisterTypedef(MakeSizedString(\"%.*s\"),",UNPACK_SS(info->name));
      fprintf(output,"MakeSizedString(\"%.*s\"));\n",UNPACK_SS(info->typedefType->name));
   }
   fprintf(output,"\n");

   fprintf(output,"    static Member members[] = {\n");
   int index = 0;
   bool first = true;
   for(Type* info : types){
      if(info->type != Type::STRUCT || info->name.size == 0 || IsUnnamedStruct(info)){
         continue;
      }

      IterateSubmembers iter = {};
      iter.currentMember[0] = info->members;

      for(Member* m = info->members; m != nullptr; m = Next(&iter)){
         const char* preamble = "        ";
         if(!first){
            preamble = "        ,";
         } else {
            first = false;
         }

         if(m->index == -1){
            m->index = index++;
         }

         fprintf(output,"%s(Member){",preamble);

         if(m->type->type == Type::ARRAY){
            fprintf(output,"GetArrayType(MakeSizedString(\"%.*s\"),%.*s)",UNPACK_SS(m->type->arrayType->name),UNPACK_SS(m->arrayExpression));
         } else {
            fprintf(output,"GetType(MakeSizedString(\"%.*s\"))",UNPACK_SS(m->type->name));
         }

         fprintf(output,",MakeSizedString(\"%.*s\")",UNPACK_SS(m->name));
         fprintf(output,",offsetof(%.*s,%.*s)",UNPACK_SS(info->name),UNPACK_SS(m->name));
         fprintf(output,",nullptr");

         fprintf(output,"}\n");
      }
   }
   fprintf(output,"    };\n\n");

   //int index = 0;
   for(Type* info : types){
      if(info->type != Type::STRUCT || info->name.size == 0 || IsUnnamedStruct(info)){
         continue;
      }

      fprintf(output,"    RegisterStructMembers(MakeSizedString(\"%.*s\"),{",UNPACK_SS(info->name));

      bool first = true;

      if(info->baseType){
         IterateSubmembers iter = {};
         iter.currentMember[0] = info->baseType->members;

         for(Member* m = info->baseType->members; m != nullptr; m = Next(&iter)){
            if(first){
               fprintf(output,"&members[%d]",m->index);
               first = false;
            } else {
               fprintf(output,",&members[%d]",m->index);
            }
         }
      }

      IterateSubmembers iter = {};
      iter.currentMember[0] = info->members;

      for(Member* m = info->members; m != nullptr; m = Next(&iter)){
         if(first){
            fprintf(output,"&members[%d]",m->index);
            first = false;
         } else {
            fprintf(output,",&members[%d]",m->index);
         }
      }
      fprintf(output,"});\n");
   }

   fprintf(output,"}\n");
}

static Type* RegisterSimpleType(SizedString name,int size){
   Type* type = types.Alloc();

   type->type = Type::BASE;
   type->size = size;
   type->name = name;

   return type;
}

void RegisterSimpleTypes(){
   #define R(TYPE) RegisterSimpleType(MakeSizedString(#TYPE),sizeof(TYPE))

   R(int);
   R(unsigned int);
   R(char);
   R(bool);
   R(unsigned char);
   R(float);
   R(double);
   R(size_t);

   RegisterSimpleType(MakeSizedString("void"),1);
}

//#define STANDALONE

#ifdef STANDALONE

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Error, need at least 3 arguments: <program> <outputFile> <inputFile1> ...");

      return 0;
   }

   InitArena(tempArena,Megabyte(256));

   RegisterSimpleTypes();

   for(int i = 0; i < argc - 2; i++){
      SizedString content = PushFile(tempArena,argv[2+i]);

      if(content.size < 0){
         printf("Failed to open file: %s\n",argv[2+i]);
         return 0;
      }

      Tokenizer tok(content,"[]{};*<>,=",{});
      ParseHeaderFile(&tok);
   }

   FILE* output = fopen(argv[1],"w");
   OutputRegisterTypesFunction(output);

   return 0;
}
#endif

/*

Currently the parser is incapable of handling multiple arrays and it cannot handle pointers to arrays using the default C expression with no reliance on typedefs (can only handle arrays of pointers).
   Do not care enough to fix this, for now

Currently the parse can only parse one template parameter, nothing more.

*/




