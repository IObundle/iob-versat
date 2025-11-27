#include "userConfigs.hpp"

#include "accelerator.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "declaration.hpp"
#include "utilsCore.hpp"

#include "CEmitter.hpp"

// TODO: Copied from spec parser, we probably want to move this somewhere and reconciliate everything.
// We probably want a common parsing file (different from parser.hpp) that contains all this logic instead of repeating or separating over multiple files.
static void ReportError3(String content,Token faultyToken,String error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("[Error]\n");
  printf("%.*s:\n",UN(error));
  printf("%.*s\n",UN(loc));
  printf("\n");
}

static void ReportError(Tokenizer* tok,Token faultyToken,String error){
  String content = tok->GetContent();
  ReportError3(content,faultyToken,error);
}

static bool _ExpectError(Tokenizer* tok,String expected){
  TEMP_REGION(temp,nullptr);

  Token got = tok->NextToken();
  if(!CompareString(got,expected)){
    
    auto builder = StartString(temp);
    builder->PushString("Parser Error.\n Expected to find:  '");
    builder->PushString(PushEscapedString(temp,expected,' '));
    builder->PushString("'\n");
    builder->PushString("  Got:");
    builder->PushString(PushEscapedString(temp,got,' '));
    builder->PushString("\n");
    String text = EndString(temp,builder);
    ReportError(tok,got,StaticFormat("%*s",UN(text))); \
    return true;
  }
  return false;
}

// Macro because we want to return as well
#define EXPECT(TOKENIZER,STR) \
  do{ \
    if(_ExpectError(TOKENIZER,STR)){ \
      return {}; \
    } \
  } while(0)

#define CHECK_IDENTIFIER(ID) \
  if(!IsIdentifier(ID)){ \
    ReportError(tok,ID,StaticFormat("type name '%.*s' is not a valid name",UN(ID))); \
    return {}; \
  }

// TODO: We probably want to change this to carry an array of tokens instead of just having 2 (name and wire) and furthermore we could create proper functions that given an accelerator or something similar would "bind" the access array into a proper entity. Make this more generic. Have a HierAccess that we can then query to see if we are accessing an node, or a wire or anything in between.
static Opt<ConfigIdentifier> ParseConfigIdentifier(Tokenizer* tok,Arena* out){
  Token id = tok->PeekToken();
  CHECK_IDENTIFIER(id);
  tok->AdvancePeek();
  
  Token access = {};
  if(tok->IfNextToken(".")){
    access = tok->NextToken();

    CHECK_IDENTIFIER(access);
  }

  ConfigIdentifier res = {};
  res.name = id;
  res.wire = access;
  
  return res;
}

static Opt<ConfigStatement*> ParseConfigStatement(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  Token peek = tok->PeekToken();

  ConfigStatement* stmt = nullptr;

  if(CompareString(peek,"for")){
    // For loop
    NOT_IMPLEMENTED();
  } else {
    // Assignment
    Opt<ConfigIdentifier> lhs = ParseConfigIdentifier(tok,out);
    PROPAGATE(lhs);
    EXPECT(tok,"=");

    stmt = PushStruct<ConfigStatement>(out);

    stmt->lhs = lhs.value();

/*
    if lhs is an identifier with a wire name, then we must parse a symbolic on the other side.
    Otherwise, we either have a function or an identifier.

    // Symbolic
    c.constant = x;

    // Function invocation
    c = Function(x);

    // Identifier.
    res = x.currentValue
*/
    
    if(!Empty(lhs.value().wire)){
      SymbolicExpression* expr = ParseSymbolicExpression(tok,out);
      
      stmt->expr = expr;
      stmt->type = ConfigRHSType_SYMBOLIC_EXPR;
      // TODO
    } else {
      if(tok->PeekToken(1) == "("){
        String functionName = tok->NextToken();
        tok->AdvancePeek();

        auto list = PushArenaList<Token>(temp);
        while(!tok->Done()){
          tok->IfNextToken(",");

          if(tok->IfNextToken(")")){
            break;
          }

          Token var = tok->NextToken();
          if(IsIdentifier(var)){
            *list->PushElem() = var;
          }
        }
        
        FunctionInvocation* func = PushStruct<FunctionInvocation>(out);
        func->functionName = functionName;
        func->arguments = PushArrayFromList(out,list);

        stmt->func = func;
        stmt->type = ConfigRHSType_FUNCTION_CALL;
        
        // TODO
      } else {
        Opt<ConfigIdentifier> rhs = ParseConfigIdentifier(tok,out);
        PROPAGATE(rhs);
        
        stmt->rhsId = rhs.value();
        stmt->type = ConfigRHSType_IDENTIFIER;
      }
    }

    EXPECT(tok,";");
  }

  return stmt;
}

// TODO: Copy from versatSpec. This function at the very least needs to be moved into a more common place.
//       Furthermore we can make this more generic if we take an enum flag that indicates the type of numbers accepted, if needed.
static Opt<int> ParseNumber(Tokenizer* tok){
  // TODO: We only handle integers, for now.
  Token number = tok->NextToken();

  bool negate = false;
  if(CompareString(number,"-")){
    negate = true;
    number = tok->NextToken();
  }

  for(int i = 0; i < number.size; i++){
    if(!(number[i] >= '0' && number[i] <= '9')){
      ReportError(tok,number,StaticFormat("%.*s is not a valid number",UN(number)));
      return {};
    }
  }

  int res = ParseInt(number);
  if(negate){
    res = -res;
  }
  
  return res;
}

static Opt<ConfigVarDeclaration> ParseConfigVarDeclaration(Tokenizer* tok){
  ConfigVarDeclaration res = {};

  res.name = tok->NextToken();
  CHECK_IDENTIFIER(res.name);

  Token peek = tok->PeekToken();
  if(CompareString(peek,"[")){
    tok->AdvancePeek();

    Opt<int> number = ParseNumber(tok);
    PROPAGATE(number);
    int arraySize = number.value();

    EXPECT(tok,"]");

    res.arraySize = arraySize;
    res.isArray = true;
  }
  
  return res;
}

static Opt<Array<ConfigVarDeclaration>> ParseConfigFunctionArguments(Tokenizer* tok,Arena* out){
  auto array = StartArray<ConfigVarDeclaration>(out);

  EXPECT(tok,"(");

  if(tok->IfNextToken(")")){
    return EndArray(array);
  }
  
  while(!tok->Done()){
    Opt<ConfigVarDeclaration> var = ParseConfigVarDeclaration(tok);
    PROPAGATE(var);

    *array.PushElem() = var.value();
    
    if(tok->IfNextToken(",")){
      continue;
    } else {
      break;
    }
  }

  EXPECT(tok,")");

  return EndArray(array);
}

ConfigFunctionDef* ParseConfigFunction(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  ConfigFunctionDef* res = nullptr;

  if(tok->IfNextToken("config")){
    Token configName = tok->NextToken();
    CHECK_IDENTIFIER(configName);

    Opt<Array<ConfigVarDeclaration>> vars = ParseConfigFunctionArguments(tok,out);
    PROPAGATE(vars);

    EXPECT(tok,"{");

    auto configs = PushArenaList<ConfigStatement*>(temp);
    while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareString(peek,";")){
        tok->AdvancePeek();
        continue;
      }
      if(CompareString(peek,"}")){
        break;
      }
        
      Opt<ConfigStatement*> config = ParseConfigStatement(tok,out);
      PROPAGATE(config);

      *configs->PushElem() = config.value();
    }

    EXPECT(tok,"}");

    res = PushStruct<ConfigFunctionDef>(out);
    res->type = UserConfigurationType_CONFIG;
    res->name = configName;
    res->variables = vars.value();
    res->statements = PushArrayFromList(out,configs);

    return res;
  }
  
  // TODO: Merge logic with config because there is overlap.
  if(tok->IfNextToken("state")){
    Token stateName = tok->NextToken();
    CHECK_IDENTIFIER(stateName);
    
    EXPECT(tok,"{");

    auto states = PushArenaList<ConfigStatement*>(temp);

    while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareString(peek,";")){
        tok->AdvancePeek();
        continue;
      }
      if(CompareString(peek,"}")){
        break;
      }
        
      Opt<ConfigStatement*> config = ParseConfigStatement(tok,out);
      PROPAGATE(config);

      *states->PushElem() = config.value();
    }

    EXPECT(tok,"}");

    res = PushStruct<ConfigFunctionDef>(out);
    res->type = UserConfigurationType_STATE;
    res->name = stateName;
    res->variables = {};
    res->statements = PushArrayFromList(out,states);

    return res;
  }

  DEBUG_BREAK_OR_EXIT();
  return nullptr;
}

bool IsNextTokenConfigFunctionStart(Tokenizer* tok){
  bool res = false;

  res |= tok->IfPeekToken("config");
  res |= tok->IfPeekToken("state");
  res |= tok->IfPeekToken("mem");
  
  return res;
}

// ======================================
// Globals

static TrieMap<String,ConfigFunction>* nameToFunction;
static Arena userConfigsArenaInst;
static Arena* userConfigsArena;

void InitializeUserConfigs(){
  userConfigsArenaInst = InitArena(Megabyte(1));
  userConfigsArena = &userConfigsArenaInst;

  nameToFunction = PushTrieMap<String,ConfigFunction>(userConfigsArena);
}

// ============================================================================
// Instantiation and manipulation

static FunctionInvocation* ParseFunctionInvocation(Array<Token> tokens,Arena* out){
  TEMP_REGION(temp,out);
  
  String functionName = {};

  #define CHECK_OR_ERROR(VAL) \
    if(CompareString(tokens[index],VAL)){ \
      index++; \
    } else { \
    } \
  
  int index = 0;

  if(IsIdentifier(tokens[index])){
    functionName = tokens[index++];
  } else {
    // TODO: Error
  }

  CHECK_OR_ERROR("(");
  
  auto list = PushArenaList<Token>(temp);
  while(index < tokens.size){
    if(CompareString(tokens[index],",")){
      index++;
    }

    if(CompareString(tokens[index],")")){
      index++;
      break;
    }

    if(IsIdentifier(tokens[index])){
      *list->PushElem() = tokens[index];
      index++;
    }
  }

  if(index != tokens.size){
    // TODO: Error
  }

  FunctionInvocation* res = PushStruct<FunctionInvocation>(out);
  res->functionName = functionName;
  res->arguments = PushArrayFromList(out,list);
  return res;
}

static String GlobalConfigFunctionName(String functionName,FUDeclaration* decl,Arena* out){
  String name = PushString(out,"%.*s_%.*s",UN(decl->name),UN(functionName));
  return name;
}

ConfigFunction* InstantiateConfigFunction(ConfigFunctionDef* def,FUDeclaration* declaration){
  Arena* perm = globalPermanent;

  TEMP_REGION(temp,perm);

  auto list = PushArenaList<ConfigAssignment>(temp);

  // TODO: Where is the error checking being perform to check if a given composite module contains the actual function?

  String structToReturnName = {};
  Array<ConfigVarDeclaration> variables = def->variables;
  Array<Token> variableNames = Extract(variables,temp,&ConfigVarDeclaration::name);
  Array<String> variableStrings = CopyArray<String,Token>(variableNames,perm);

  // TODO: This flow is not good. With a bit more work we probably can join state and config into the same flow or at least avoid duplicating work. For now we are mostly prototyping so gonna keep pushing what we have.
  if(def->type == UserConfigurationType_CONFIG){
    for(ConfigStatement* stmt : def->statements){
      // TODO: Call entity function to make sure that the entity exists and it is a config wire

      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;

      FULL_SWITCH(stmt->type){
      case ConfigRHSType_FUNCTION_CALL:{
        FunctionInvocation* functionInvoc = stmt->func;

        ConfigFunction* function = nameToFunction->Get(functionInvoc->functionName);
        if(!function){
          // TODO: Error
          printf("Error 2, function does not exist, make sure the name is correct\n");
          exit(-1);
          return nullptr;
        }

        if(functionInvoc->arguments.size != function->variables.size){
          printf("Error 2.1, number of arguments does not match\n");
          exit(-1);
          return nullptr;
        }
      
        // Invocation var to function argument
        Hashmap<String,SymbolicExpression*>* argToVar = PushHashmap<String,SymbolicExpression*>(temp,functionInvoc->arguments.size);
        for(int i = 0; i <  functionInvoc->arguments.size; i++){
          String arg = function->variables[i];

          SymbolicExpression* var = PushVariable(temp,functionInvoc->arguments[i]);
          argToVar->Insert(arg,var);
        }
      
        for(ConfigAssignment assign : function->assignments){
          // TODO: We are building the struct access expression in here but I got a feeling that we probably want to preserve data as much as possible in order to tackle merge later on.
          String lhs = PushString(perm,"%.*s.%.*s",UN(name),UN(assign.lhs));
        
          SymbolicExpression* rhs = assign.rhs;
          SymbolicExpression* newExpr = ReplaceVariables(rhs,argToVar,perm);
        
          ConfigAssignment* newAssign = list->PushElem();
          newAssign->lhs = lhs;
          newAssign->rhs = newExpr;
        }
      } break;
      case ConfigRHSType_SYMBOLIC_EXPR:{
        ConfigAssignment* assign = list->PushElem();
        assign->lhs = PushString(perm,"%.*s.%.*s",UN(name),UN(wireName));
        assign->rhs = SymbolicDeepCopy(stmt->expr,perm);
      } break;
      case ConfigRHSType_IDENTIFIER: {
        // TODO: Proper error reporting
        Assert(false);
      } break;
    }
    }
  }

  auto newStructs = PushArenaList<String>(temp);

  if(def->type == UserConfigurationType_STATE){
    CEmitter* c = StartCCode(temp);
    
    String structName = PushString(perm,"%.*s_%.*s_Struct",UN(declaration->name),UN(def->name));
    structToReturnName = structName;

    c->Struct(structName);
    for(ConfigStatement* stmt : def->statements){
      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;
      
      // TODO: Proper error reporting. 
      Assert(Empty(wireName));

      c->Member("int",name);
    }
    c->EndStruct();

    *newStructs->PushElem() = PushASTRepr(c,perm,false);

    for(ConfigStatement* stmt : def->statements){
      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;

      FULL_SWITCH(stmt->type){
      case ConfigRHSType_FUNCTION_CALL:{
        // TODO: Proper error reporting
        Assert(false);
      } break;
      case ConfigRHSType_SYMBOLIC_EXPR:{
        // TODO: Proper error reporting
        Assert(false);
      } break;
      case ConfigRHSType_IDENTIFIER:{

        Array<String> accessExpr = PushArray<String>(temp,2);
        accessExpr[0] = stmt->rhsId.name;
        accessExpr[1] = stmt->rhsId.wire;

        Opt<Entity> entityOpt = GetEntityFromHierAccess(&declaration->info,accessExpr);

        if(!entityOpt){
          // TODO: Proper error reporting.
          printf("Error, entity does not exist\n");
          exit(-1);
        }
        Entity entity = entityOpt.value();
        
        if(entity.type == EntityType_CONFIG_FUNCTION){
          InstanceInfo* info = entity.inst;
          ConfigFunction* func = entity.func;

          for(ConfigAssignment stmt : func->assignments){
            ConfigAssignment* assign = list->PushElem();
            assign->lhs = name;
            assign->rhsId = PushString(perm,"%.*s.%.*s",UN(info->name),UN(stmt.rhsId));
          }
        } else if(entity.type == EntityType_STATE_WIRE){
          ConfigAssignment* assign = list->PushElem();
          
          assign->lhs = name;
          assign->rhsId = PushString(perm,"%.*s.%.*s",UN(stmt->rhsId.name),UN(stmt->rhsId.wire));
        } else {
          // TODO: Proper error reporting.
          printf("The entity is not a function or a state wire\n");
          exit(-1);
        }
      } break;
      }
    }
  }

  ConfigFunction func = {};
  func.decl = declaration;
  func.assignments = PushArrayFromList(perm,list);
  func.variables = variableStrings;
  func.individualName = PushString(perm,def->name);
  func.fullName = GlobalConfigFunctionName(func.individualName,declaration,perm);
  func.newStructs = PushArrayFromList(perm,newStructs);
  func.structToReturnName = structToReturnName;

  ConfigFunction* res = nameToFunction->Insert(func.fullName,func);
  
  return res;  
}
