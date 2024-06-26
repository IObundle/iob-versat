
#include "structParsing.hpp"

#include <cstdio>
#include <unordered_set>

#include "parser.hpp"
#include "utilsCore.hpp"

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
      StructDef def = ParseStruct(tok,arena).value; // TODO: Actually handle error
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

      Token line = tok->PeekFindUntil(";").value();

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

      auto oldMark = tok->Mark();

      Token peek = tok->PeekToken();
      if(CompareString(peek,"struct")){
        StructDef def = ParseStruct(tok,arena).value; // TODO: Actually handle error

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
	NOT_IMPLEMENTED("Implemented as needed"); // TypeDef::TEMPLATED 
	//name = type->templatedType.baseType;
  }break;
  case TypeDef::TYPEDEF:{
    name = type->typedefType.newType;
  }break;
  }

  return name;
}

TypeDef* GetDef(String name){
  // TODO: This is basically a hack to handled templated structures because
  //       we do not actually parse the structures. We just tokenize them.
  //       The best thing to do would be to divide the processing into two parts.
  //       The first part "tokenize" everything into strings.
  //       The second part parses type info in such a way that it would be simple to serialize.
  //       That way we could actually do some processing in here and let the actual program just load data directly.
  //          Only problem is the fact that code can create new types at runtime.
  Tokenizer tok(name,"<",{});
  String baseName = tok.NextToken();

  for(TypeDef* def : typeDefs){
    String typeName = GetTypeName(def);

    if(CompareString(typeName,baseName)){
      return def;
    }
  }

  return nullptr;
}

int OutputMembers(FILE* output,String structName,MemberDef* m,bool first,Arena* arena){
  BLOCK_REGION(arena);
  int count = 0;
  for(; m != nullptr; m = m->next){
    if(m->type.type == TypeDef::STRUCT){
      StructDef def = m->type.structType;
      if(def.name.size > 0){
        String name = PushString(arena,"%.*s::%.*s",UNPACK_SS(structName),UNPACK_SS(def.name));

        count += OutputMembers(output,name,m->type.structType.members,first,arena);
      } else {
        count += OutputMembers(output,structName,m->type.structType.members,first,arena); // anonymous struct
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

	  if(m->arrays.size){
		typeName = PushString(arena,"%.*s%.*s",UNPACK_SS(typeName),UNPACK_SS(m->arrays));
	  }

      fprintf(output,"%s(Member){",preamble);
      fprintf(output,"GetType(STRING(\"%.*s\"))",UNPACK_SS(typeName));
      fprintf(output,",STRING(\"%.*s\")",UNPACK_SS(m->name));
      fprintf(output,",offsetof(%.*s,%.*s)}\n",UNPACK_SS(structName),UNPACK_SS(m->name));

	  count += 1;
    }
    first = false;
  }

  return count;
}

int OutputTemplateMembers(FILE* output,MemberDef* m,int index,bool insideUnion, bool first,Arena* arena,int& memberArrayIndex){
  for(; m != nullptr; m = m->next){
    if(m->type.type == TypeDef::STRUCT){
      StructDef def = m->type.structType;
      int count = OutputTemplateMembers(output,m->type.structType.members,index,def.isUnion,first,arena,memberArrayIndex); // anonymous struct

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

	  String typeName = GetTypeName(&m->type);

	  if(m->arrays.size){
		typeName = PushString(arena,"%.*s%.*s",UNPACK_SS(typeName),UNPACK_SS(m->arrays));
	  }
	  
	  fprintf(output,"STRING(\"%.*s\")",UNPACK_SS(typeName));
      fprintf(output,",STRING(\"%.*s\")",UNPACK_SS(m->name));
      fprintf(output,",%d} // %d\n",index,memberArrayIndex++);

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

  auto mark = tok.Mark();
  Token base = tok.NextToken();

  SkipQualifiers(&tok);

  Token peek = tok.PeekToken();
  if(CompareString(peek,"<")){
    Token templateInst = tok.PeekIncludingDelimiterExpression({"<"},{">"},0).value();
    tok.AdvancePeek(templateInst);
    base = tok.Point(mark);
  }

  base = TrimWhitespaces(base);

  return base;
}

bool IsTemplatedParam(TypeDef* def,String memberTypeName){
  if(def->type != TypeDef::STRUCT){
    return false;
  }

  FOREACH_LIST(TemplateParamDef*,ptr,def->structType.params){
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
    Opt<FindFirstResult> res = tok.FindFirst({",",">"});
    Assert(res.has_value());

    FindFirstResult search = res.value();

    if(CompareString(search.foundFirst,",")){
      bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded,arena);
      if(res){
        return res;
      }
      tok.AdvancePeek(search.peekFindNotIncluded);
      tok.AssertNextToken(",");
      continue;
    } else {
      bool res = DependsOnTemplatedParam(def,search.peekFindNotIncluded,arena);
      return res;
    }
  }

  UNREACHABLE("while should never end without reaching a return, if reach here, maybe should be a handled error or maybe function was called in a context that does not validate the entirety of the string, check then");
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

        Opt<Token> peek = tok.PeekFindUntil(",");

        if(!peek.has_value()){
          break;
        }

        tok.AdvancePeek(peek.value());
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
  int memberArrayIndex = 0;
  for(TypeDef* def : typeDefs){
    if(!(def->type == TypeDef::STRUCT && def->structType.params)){
      continue;
    }

	int count = 0;
	if(def->structType.inherit.size){
	  // Make sure we are not inheriting directly from a templated typename (something like T for template<typename T>)
      bool inheritFromTypename = false;
      FOREACH_LIST(TemplateParamDef*,ptr,def->structType.params){
		if(CompareString(ptr->name,def->structType.inherit)){
		  inheritFromTypename = true;
		}
      }

	  if(!inheritFromTypename){
		TypeDef* inheritDef = GetDef(def->structType.inherit);

		if(inheritDef->structType.params){
		  count = OutputTemplateMembers(output,inheritDef->structType.members,0,inheritDef->structType.isUnion,first,arena,memberArrayIndex);
		} else {
		  count = OutputMembers(output,inheritDef->structType.name,inheritDef->structType.members,first,arena);
		}
	  }
	}

    OutputTemplateMembers(output,def->structType.members,count,def->structType.isUnion,first,arena,memberArrayIndex);
    first = false;
  }
  fprintf(output,"   };\n\n");

  index = 0;
  for(TypeDef* def : typeDefs){
    if(!(def->type == TypeDef::STRUCT && def->structType.params)){
      continue;
    }

    int size = CountMembers(def->structType.members);

    bool inheritFromTypename = false;
    FOREACH_LIST(TemplateParamDef*,ptr,def->structType.params){
	  if(CompareString(ptr->name,def->structType.inherit)){
		inheritFromTypename = true;
	  }
    }

	if(!inheritFromTypename && def->structType.inherit.size){
	  TypeDef* inheritDef = GetDef(def->structType.inherit);
	  size += CountMembers(inheritDef->structType.members);
	}
	
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
  auto mark = StartString(arena);

  PushString(arena,"%.*s | ",UNPACK_SS(def->type.simpleType));
  PushString(arena,"%.*s",UNPACK_SS(def->name));
  if(def->arrays.size){
    PushString(arena,"%.*s",UNPACK_SS(def->arrays));
  }

  String res = EndString(mark);
  return res;
}

struct RepresentationExpression{
  String text;
  enum {TEXT,VAR_NAME} type;
};

// Escapes input and possible creates new string so that the string could be pasted inside a printf and works
// Does not insert '"' at the beginning or the end, it's more for correct handling of '\' and '%' if they exist.
String EscapeStringForPrintf(String input,Arena* arena){
  // TODO: This function could be useful in the rest of the codebase, reallocate after beginning using it.

  auto mark = StartString(arena);
  for(int i = 0; i < input.size; i++){
	if(input[i] == '\\' && input[i] == '%'){
	  PushChar(arena,'\\');
	}
	PushChar(arena,input[i]);
  }
  String res = EndString(mark);
  return res;
}

bool IsNameValid(String name){
  for(char ch : name){
    switch(ch){
    case ':': return false;
    case '<': return false;
    case '>': return false;
    }
  }

  return true;
}

void OutputPrintAll(FILE* hppFile,FILE* cppFile,Arena* arena){
  // NOTE: For now only use InstanceInfo. It's basicaly the only struct that I really need right now.
  Set<String>* structsToSee = PushSet<String>(arena,99);
  structsToSee->Insert(STRING("InstanceInfo"));
  structsToSee->Insert(STRING("VersatComputedValues"));
  structsToSee->Insert(STRING("ComputedData"));
  structsToSee->Insert(STRING("MemoryAddressMask"));
  
  DynamicArray<TypeDef*> arr = StartArray<TypeDef*>(arena);
  for(TypeDef* def : typeDefs){
    if(def->type == TypeDef::STRUCT && structsToSee->Exists(def->structType.name)){
      *arr.PushElem() = def;
    }
  }
  Array<TypeDef*> structTypes = EndArray<TypeDef*>(arr);

  for(TypeDef* def : structTypes){
	fprintf(hppFile,"Array<String> GetFields(%.*s tag,Arena* out);\n",UNPACK_SS(def->structType.name));
	fprintf(hppFile,"Array<String> GetRepr(%.*s* info,Arena* out);\n",UNPACK_SS(def->structType.name));
	fprintf(hppFile,"String Repr(%.*s* info,Arena* out);\n",UNPACK_SS(def->structType.name));
  }
  
  for(TypeDef* def : structTypes){
    int memberSize = Size(def->structType.members);
    String name = def->structType.name;
    fprintf(cppFile,"#define %.*s_FIELDS %d\n",UNPACK_SS(name),memberSize);
    fprintf(cppFile,"Array<String> GetRepr(%.*s* s,Arena* out){\n",UNPACK_SS(name));
    fprintf(cppFile,"  Array<String> res = PushArray<String>(out,%.*s_FIELDS);\n",UNPACK_SS(name));
    fprintf(cppFile,"  int i = 0;\n");
    
    FOREACH_LIST(MemberDef*,iter,def->structType.members){
      fprintf(cppFile,"  res[i++] = Repr(&s->%.*s,out);\n",UNPACK_SS(iter->name));
    }
    
    fprintf(cppFile,"  return res;\n}\n\n");
  }

  for(TypeDef* def : structTypes){
#if 0
    if(def->structType.representationFormat.size > 0){
      NEWLINE();
      NEWLINE();
      PRINT_STRING(def->structType.name);
      NEWLINE();
      NEWLINE();
    }
#endif

    if(def->structType.representationFormat.size > 0){
      String name = def->structType.name;
      fprintf(cppFile,"String Repr(%.*s* s,Arena* out){\n",UNPACK_SS(name));
      Tokenizer tok(def->structType.representationFormat,"",{});

      fprintf(cppFile,"  auto mark = StartString(out);\n");
      while(!tok.Done()){
        Token white = tok.PeekWhitespace();
        tok.AdvancePeek(white);
        
        if(white.size > 0){
          fprintf(cppFile,"  PushString(out,\"%.*s\");\n",UNPACK_SS(white));
        }

        if(tok.Done()){
          break;
        }
        
        Token token = tok.NextToken();
        
        MemberDef* member = nullptr;
        FOREACH_LIST(MemberDef*,iter,def->structType.members){
          if(CompareString(iter->name,token)){
            member = iter;
            break;
          }
        }

        if(member){
          fprintf(cppFile,"  Repr(&s->%.*s,out);\n",UNPACK_SS(member->name));
        } else {
          fprintf(cppFile,"  PushString(out,\"%.*s\");\n",UNPACK_SS(token));
        }
      }
      fprintf(cppFile,"  return EndString(mark);\n}\n\n");
      
    } else {
      String name = def->structType.name;
      fprintf(cppFile,"String Repr(%.*s* s,Arena* out){\n",UNPACK_SS(name));
      fprintf(cppFile,"  STACK_ARENA(temp,Kilobyte(4));\n");
      fprintf(cppFile,"  Array<String> res = GetRepr(s,&temp);\n");
      fprintf(cppFile,"  auto mark = StartString(out);\n");
      fprintf(cppFile,"  for(String str : res){\n");
      fprintf(cppFile,"    PushString(out,str);\n");
      fprintf(cppFile,"    PushString(out,STRING(\" \"));\n");
      fprintf(cppFile,"  }\n");
    
      fprintf(cppFile,"  return EndString(mark);\n}\n\n");
    }
  }
  
  for(TypeDef* def : structTypes){
    int memberSize = Size(def->structType.members);
    String name = def->structType.name;
    fprintf(cppFile,"Array<String> GetFields(%.*s tag,Arena* out){\n",UNPACK_SS(name));

    fprintf(cppFile,"  Array<String> res = PushArray<String>(out,%.*s_FIELDS);\n",UNPACK_SS(name));
    fprintf(cppFile,"  int i = 0;\n");

    FOREACH_LIST(MemberDef*,iter,def->structType.members){
      fprintf(cppFile,"  res[i++] = STRING(\"%.*s\");\n",UNPACK_SS(iter->name));
    }

    fprintf(cppFile,"  return res;\n}\n\n");
  }
    
}

void OutputRepresentationFunction(FILE* hppFile,FILE* cppFile,Arena* arena){
  DynamicArray<TypeDef*> arr = StartArray<TypeDef*>(arena);
  for(TypeDef* def : typeDefs){
    if(def->type != TypeDef::STRUCT){
	  continue;
	}

	if(!def->structType.representationFormat.size){
	  continue;
	}

	*arr.PushElem() = def;
  }
  Array<TypeDef*> structs = EndArray(arr);

  fprintf(hppFile,"#include \"utils.hpp\"\n");
  fprintf(hppFile,"#include \"type.hpp\"\n");
  fprintf(hppFile,"#include \"textualRepresentation.hpp\"\n");
  fprintf(hppFile,"struct Arena;\n");
  
  fprintf(cppFile,"#include \"repr.hpp\"\n\n");
  fprintf(cppFile,"#include \"memory.hpp\"\n\n");

  // MARK - Left here. Added a simple hack to make this work for InstanceInfo for the time being
  //        Need to significantly improved the struct parser before progressing.
  //          Need to sort types based on dependency
  //          Would be useful to store the file from where the structure was parsed from.
  //          Should perform general codebase improvements, like converting member list to array and stuff like that.
#if 1
  OutputPrintAll(hppFile,cppFile,arena);
#endif
  
#if 0
  for(TypeDef* def : structs){
	String structName = def->structType.name;
    String reprFormat = def->structType.representationFormat;

	BLOCK_REGION(arena);

	Tokenizer tokInst(reprFormat,"{}",{});
	Tokenizer* tok = &tokInst;

	Byte* mark = MarkArena(arena);
	int count = 0;
	while(!tok->Done()){
	  Token string = tok->PeekFindUntil("{");

	  if(string.size > 0){
		RepresentationExpression* expr = PushStruct<RepresentationExpression>(arena);
		expr->text = string;
		expr->type = RepresentationExpression::TEXT;
		tok->AdvancePeek(string);
	  } else if(string.size < 0){
		Token rest = tok->Finish();
		if(rest.size > 0){
		  RepresentationExpression* expr = PushStruct<RepresentationExpression>(arena);
		  expr->text = rest;
		  expr->type = RepresentationExpression::TEXT;
		}
	  }

	  if(tok->Done()){
		break;
	  }

	  // Right on top of a variable
	  tok->AssertNextToken("{");
	  Token name = tok->NextToken();
	  tok->AssertNextToken("}");

	  RepresentationExpression* expr = PushStruct<RepresentationExpression>(arena);
	  expr->text = name;
	  expr->type = RepresentationExpression::VAR_NAME;
	}

	Array<RepresentationExpression> expressions = PointArray<RepresentationExpression>(arena,mark);

	// Make sure that every variable exists inside the struct and error out early
	for(RepresentationExpression expr : expressions){
	  if(expr.type == RepresentationExpression::TEXT){
		continue;
	  }

	  String memberName = expr.text;
	  bool found = false;
	  for(MemberDef* m = def->structType.members; m; m = m->next){
		if(CompareString(m->name,memberName)){
		  found = true;
		  break;
		}
	  }

	  if(!found){
		printf("Did not find name (%.*s) in representation (%.*s)\n",UNPACK_SS(memberName),UNPACK_SS(reprFormat));
		Assert(false);
	  }
	}

	fprintf(hppFile,"String Repr(%.*s* val,Arena* arena);\n",UNPACK_SS(structName));

	fprintf(cppFile,"%.*s;\n",UNPACK_SS(def->structType.fullExpression));
	fprintf(cppFile,"String Repr(%.*s* val,Arena* arena){\n",UNPACK_SS(structName));
	fprintf(cppFile,"\tauto mark = StartString(arena);\n");

	for(RepresentationExpression expr : expressions){
	  BLOCK_REGION(arena);
	  switch(expr.type){
	  case RepresentationExpression::TEXT:{
		String toOutput = EscapeStringForPrintf(expr.text,arena);
		fprintf(cppFile,"\tPushString(arena,\"%.*s\");\n",UNPACK_SS(toOutput));
	  }break;
	  case RepresentationExpression::VAR_NAME:{
		fprintf(cppFile,"\tRepr(&val->%.*s,arena);\n",UNPACK_SS(expr.text));
	  }break;
	  default:{
		NOT_IMPLEMENTED;
	  }break;
	  }
	}

	fprintf(cppFile,"\tString res = EndString(mark);\n");
	fprintf(cppFile,"\treturn res;\n");
	fprintf(cppFile,"}\n\n");
  }

  fprintf(hppFile,"String ValueRepresentation(Value val,Arena* arena);\n");

  fprintf(cppFile,"String ValueRepresentation(Value val,Arena* arena){\n");
  for(TypeDef* def : structs){
	String structName = def->structType.name;

	fprintf(cppFile,"\tstatic Type* %.*sType = GetType(STRING(\"%.*s\"));\n",UNPACK_SS(structName),UNPACK_SS(structName));
  }
  fprintf(cppFile,"\tString res = {};\n");
  fprintf(cppFile,"\tType* type = val.type;\n");
  for(TypeDef* def : structs){
	String structName = def->structType.name;

	fprintf(cppFile,"\tif(type ==  %.*sType){\n",UNPACK_SS(structName));
	fprintf(cppFile,"\t\tres = Repr((%.*s*) val.custom,arena);\n",UNPACK_SS(structName));
	fprintf(cppFile,"\t}\n");
  }
  fprintf(cppFile,"\treturn res;\n}\n");
#endif
}

int main(int argc,const char* argv[]){
  if(argc < 3){
    printf("Error, need at least 3 arguments: <program> <path to output typeInfo.cpp,repr.hpp,repr.cpp> <inputFile1> ...");

    return 0;
  }

  Arena tempArena = InitArena(Megabyte(256));
  Arena* arena = &tempArena;

  for(int i = 0; i < argc - 2; i++){
    String content = PushFile(arena,argv[2+i]);

    if(content.size < 0){
      printf("Failed to open file: %s\n",argv[2+i]);
      return 0;
    }

    Tokenizer tok(content,":[]{}();*<>,=",{"//","/*","*/","::"});
    ParseHeaderFile(&tok,arena);
  }

  {
	TypeDef* voidType = typeDefs.Alloc();
	voidType->type = TypeDef::SIMPLE;
	voidType->simpleType = STRING("void");
  }

#if 0
  const char* test = "template<typename T> struct std::vector{T* mem; int size; int allocated;};";
  Tokenizer tok(STRING(test),":[]{}();*<>,=",{"::"});
  ParseHeaderFile(&tok,arena);
#endif
  
#if 0
  region(arena){
	String typeInfoPath = PushString(arena,"%s/typeInfo.cpp",argv[1]);
	PushNullByte(arena);
	
	FILE* output = OpenFileAndCreateDirectories(typeInfoPath.data,"w");

	fprintf(output,"#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n\n");
	for(int i = 0; i < argc - 2; i++){
      String name = ExtractFilenameOnly(STRING(argv[2+i]));
      fprintf(output,"#include \"%.*s\"\n",UNPACK_SS(name));
	}

	if(!output){
      printf("Failed to open file: %s\n",typeInfoPath.data);
      printf("Exiting\n");
      return -1;
	}

	OutputRegisterTypesFunction(output,arena);

	fclose(output);
  }
#endif
  
#if 1
  region(arena){
	String cppPath = PushString(arena,"%s/repr.cpp",argv[1]);
	PushNullByte(arena);
	String hppPath = PushString(arena,"%s/repr.hpp",argv[1]);
	PushNullByte(arena);

	FILE* cppFile = OpenFileAndCreateDirectories(cppPath.data,"w");
	FILE* hppFile = OpenFileAndCreateDirectories(hppPath.data,"w");

	if(!cppFile){
      printf("Failed to open file: %s\n",cppPath.data);
      printf("Exiting\n");
      return -1;
	}

	if(!hppFile){
      printf("Failed to open file: %s\n",hppPath.data);
      printf("Exiting\n");
      return -1;
	}

	OutputRepresentationFunction(hppFile,cppFile,arena);

	fclose(cppFile);
	fclose(hppFile);
  }
#endif

  return 0;
}
