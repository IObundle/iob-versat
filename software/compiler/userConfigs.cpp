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

  SymbolicExpression* expr = nullptr;
  if(tok->IfNextToken("[")){
    expr = ParseSymbolicExpression(tok,out);
    PROPAGATE(expr);
    EXPECT(tok,"]");
  }

  ConfigIdentifier res = {};
  res.name = id;
  res.wire = access;
  res.expr = expr;
  
  return res;
}

static Opt<ConfigStatement*> ParseConfigStatement(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  Token peek = tok->PeekToken();

  ConfigStatement* stmt = PushStruct<ConfigStatement>(out);

  if(CompareString(peek,"for")){
    tok->AdvancePeek();
      
    Token loopVariable = tok->NextToken();
    CHECK_IDENTIFIER(loopVariable);
      
    Array<Token> startSym = TokenizeSymbolicExpression(tok,out);
    if(Empty(startSym)){
      return {};
    }
      
    EXPECT(tok,"..");

    Array<Token> endSym = TokenizeSymbolicExpression(tok,out);
    if(Empty(endSym)){
      return {};
    }

    EXPECT(tok,"{");
      
    stmt->def = (AddressGenForDef){.loopVariable = loopVariable,.startSym = startSym,.endSym = endSym};
    
    auto list = PushArenaList<ConfigStatement*>(temp);
    while(!tok->Done()){
      Opt<ConfigStatement*> child = ParseConfigStatement(tok,out);
      PROPAGATE(child);

      *list->PushElem() = child.value();

      if(tok->IfPeekToken("}")){
        break;
      }
    }

    stmt->childs = PushArrayFromList(out,list);
    stmt->type = ConfigStatementType_FOR_LOOP;

    EXPECT(tok,"}");
  } else {
    // Assignment
    Opt<ConfigIdentifier> lhs = ParseConfigIdentifier(tok,out);
    PROPAGATE(lhs);
    EXPECT(tok,"=");

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
      stmt->rhsType = ConfigRHSType_SYMBOLIC_EXPR;
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
        stmt->rhsType = ConfigRHSType_FUNCTION_CALL;
        
        // TODO
      } else {
        Opt<ConfigIdentifier> rhs = ParseConfigIdentifier(tok,out);
        PROPAGATE(rhs);
        
        stmt->rhsId = rhs.value();
        stmt->rhsType = ConfigRHSType_IDENTIFIER;
      }
    }

    EXPECT(tok,";");
    
    stmt->type = ConfigStatementType_STATEMENT;
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

  ConfigVarType type = ConfigVarType_SIMPLE;
  if(tok->IfNextToken(":")){
    Token possibleType = tok->NextToken();

    if(CompareString(possibleType,"Address")){
      type = ConfigVarType_ADDRESS;
    } else if(CompareString(possibleType,"Fixed")){
      type = ConfigVarType_FIXED;
    } else if(CompareString(possibleType,"Dyn")){
      type = ConfigVarType_DYN;
    } else {
      printf("Error, unknown variable type\n");
      return {};
    }
  }

  res.type = type;
  
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

  if(tok->IfNextToken("mem")){
    Token memName = tok->NextToken();
    CHECK_IDENTIFIER(memName);

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
    res->type = UserConfigurationType_MEM;
    res->name = memName;
    res->variables = vars.value();
    res->statements = PushArrayFromList(out,configs);

    return res;
  }

  NOT_POSSIBLE();
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

ConfigFunction* InstantiateConfigFunction(ConfigFunctionDef* def,FUDeclaration* declaration,String content,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushArenaList<ConfigStuff>(temp);
  ConfigFunctionType type = {};

  // TODO: Where is the error checking being perform to check if a given composite module contains the actual function?
  String structToReturnName = "void";
  Array<ConfigVarDeclaration> variables = def->variables;
  Array<Token> variableNames = Extract(variables,temp,&ConfigVarDeclaration::name);

  Array<String> variableStr = CopyArray<String,Token>(variableNames,temp);
  
  FREE_ARENA(temp3);
  ARENA_NO_POP(temp3);
  Env* env = StartEnvironment(temp3);

  for(ConfigVarDeclaration var : variables){
    if(var.type == ConfigVarType_ADDRESS){
      AddOrSetVariable(env,var.name,VariableType_VOID_PTR);
    } else {
      AddOrSetVariable(env,var.name,VariableType_INTEGER);
    }
  }

  // TODO: This flow is not good. With a bit more work we probably can join state and config into the same flow or at least avoid duplicating work. For now we are mostly prototyping so gonna keep pushing what we have.

  // Break apart every for loop + statement into individual statements.
  // NOTE: Kinda slow but should not be a problem anytime soon.
  TrieMap<ConfigStatement*,ConfigStatement*>* nodeToParent = PushTrieMap<ConfigStatement*,ConfigStatement*>(temp);
  auto simpleStmtList = PushArenaList<ConfigStatement*>(temp);

  auto Recurse = [nodeToParent,simpleStmtList](auto Recurse,ConfigStatement* top) -> void{
    if(top->type == ConfigStatementType_FOR_LOOP){
      for(ConfigStatement* child : top->childs){
        nodeToParent->Insert(child,top);
        Recurse(Recurse,child);
      }
    }
    if(top->type == ConfigStatementType_STATEMENT){
      *simpleStmtList->PushElem() = top;
    }
  };

  for(ConfigStatement* top : def->statements){
    Recurse(Recurse,top);
  }

  auto stmtList = PushArenaList<Array<ConfigStatement*>>(temp);
  for(ConfigStatement* stmt : simpleStmtList){
    auto list = PushArenaList<ConfigStatement*>(temp);
    
    ConfigStatement* ptr = stmt;
    while(ptr){
      *list->PushElem() = ptr;

      ConfigStatement** possibleParent = nodeToParent->Get(ptr);
      ConfigStatement* parent = possibleParent ? *possibleParent : nullptr;

      ptr = parent;
    }

    Array<ConfigStatement*> asArray = PushArrayFromList(temp,list);
    ReverseInPlace(asArray);
    *stmtList->PushElem() = asArray;
  }

  // From this point on use this. Every N sized array is composed of N-1 FOR_LOOP types and 1 STATEMENT type.
  // TODO: Remember, after pushing every statement into an individual loop, we need to do error checking and check if the variable still exists. We cannot do variable checking globally since some statements might not be inside one of the loops.
  Array<Array<ConfigStatement*>> individualStatements = PushArrayFromList(temp,stmtList);

  if(def->type == UserConfigurationType_CONFIG){
    type = ConfigFunctionType_CONFIG;

    for(Array<ConfigStatement*> stmts : individualStatements){
      ConfigStatement* stmt = stmts[0];
      ConfigStatement* simple = stmts[stmts.size - 1];
      // TODO: Call entity function to make sure that the entity exists and it is a config wire

      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;

      FULL_SWITCH(stmt->type){
      case ConfigStatementType_FOR_LOOP:{
        auto forLoops = PushArenaList<AddressGenForDef>(temp);

        for(int i = 0; i < stmts.size - 1; i++){
          *forLoops->PushElem() = stmts[i]->def;
        }

        Array<AddressGenForDef> loops = PushArrayFromList(temp,forLoops);
        
        Assert(simple->type == ConfigStatementType_STATEMENT);
        Assert(simple->rhsType == ConfigRHSType_IDENTIFIER);
        Assert(simple->rhsId.expr);

        Array<String> accessExpr = PushArray<String>(temp,1);
        accessExpr[0] = simple->lhs.name;

        Opt<Entity> entityOpt = GetEntityFromHierAccess(&declaration->info,accessExpr);
        if(!entityOpt){
          printf("Did not find entity 123\n");
          exit(-1);
        }

        Entity ent = entityOpt.value();
        Assert(ent.type == EntityType_NODE);

        name = simple->lhs.name;
        wireName = simple->lhs.wire;

        // Pointers are stronger than integers since there might exist cases where we want to store a pointer inside the config of an unit.
        //AddOrSetVariable(env,simple->rhsId.name,VariableType_VOID_PTR);

        AddressAccess* access = CompileAddressGen({},variableNames,loops,simple->rhsId.expr,content);
        AddressGenInst supported = ent.inst->decl->supportedAddressGen;

        ConfigStuff* newAssign = list->PushElem();
        newAssign->info = ent.inst;
        newAssign->type = ConfigStuffType_ADDRESS_GEN;
        newAssign->access.access = access;
        newAssign->access.inst = supported;
        newAssign->accessVariableName = PushString(out,simple->rhsId.name);
        newAssign->lhs = PushString(out,name);
      } break;
      case ConfigStatementType_STATEMENT:{
        FULL_SWITCH(stmt->rhsType){
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
            ConfigVariable arg = function->variables[i];

            SymbolicExpression* var = PushVariable(temp,functionInvoc->arguments[i]);
            argToVar->Insert(arg.name,var);
          }
      
          for(ConfigStuff assign : function->stuff){
            // TODO: We are building the struct access expression in here but I got a feeling that we probably want to preserve data as much as possible in order to tackle merge later on.
            String lhs = PushString(out,"%.*s.%.*s",UN(name),UN(assign.assign.lhs));
        
            SymbolicExpression* rhs = assign.assign.rhs;
            SymbolicExpression* newExpr = ReplaceVariables(rhs,argToVar,out);
        
            ConfigStuff* newAssign = list->PushElem();
            newAssign->type = ConfigStuffType_ASSIGNMENT;
            newAssign->assign.lhs = lhs;
            newAssign->assign.rhs = newExpr;
          }
        } break;
        case ConfigRHSType_SYMBOLIC_EXPR:{
          ConfigStuff* assign = list->PushElem();
          assign->type = ConfigStuffType_ASSIGNMENT;
          assign->assign.lhs = PushString(out,"%.*s.%.*s",UN(name),UN(wireName));
          assign->assign.rhs = SymbolicDeepCopy(stmt->expr,out);
        } break;
        case ConfigRHSType_IDENTIFIER: {
          // TODO: Proper error reporting
          Assert(false);
        } break;
      }
      } break;
      }
    }
  }

  auto newStructs = PushArenaList<String>(temp);

  if(def->type == UserConfigurationType_STATE){
    type = ConfigFunctionType_STATE;

    CEmitter* c = StartCCode(temp);
    
    String structName = PushString(out,"%.*s_%.*s_Struct",UN(declaration->name),UN(def->name));
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

    *newStructs->PushElem() = PushASTRepr(c,out,false);

    for(ConfigStatement* stmt : def->statements){
      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;

      FULL_SWITCH(stmt->rhsType){
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

          for(ConfigStuff stmt : func->stuff){
            ConfigStuff* assign = list->PushElem();
            assign->type = ConfigStuffType_ASSIGNMENT;
            assign->assign.lhs = name;
            assign->assign.rhsId = PushString(out,"%.*s.%.*s",UN(info->name),UN(stmt.assign.rhsId));
          }
        } else if(entity.type == EntityType_STATE_WIRE){
          ConfigStuff* assign = list->PushElem();
          
          assign->type = ConfigStuffType_ASSIGNMENT;
          assign->assign.lhs = name;
          assign->assign.rhsId = PushString(out,"%.*s.%.*s",UN(stmt->rhsId.name),UN(stmt->rhsId.wire));
        } else {
          // TODO: Proper error reporting.
          printf("The entity is not a function or a state wire\n");
          exit(-1);
        }
      } break;
      }
    }
  }
  
  if(def->type == UserConfigurationType_MEM){
    type = ConfigFunctionType_MEM;

    for(Array<ConfigStatement*> stmts : individualStatements){
      ConfigStatement* stmt = stmts[0];
      ConfigStatement* simple = stmts[stmts.size - 1];

      ConfigIdentifier id = stmt->lhs;
      Token name = id.name;
      Token wireName = id.wire;

      FULL_SWITCH(stmt->type){
      case ConfigStatementType_STATEMENT:{
        FULL_SWITCH(stmt->rhsType){
        case ConfigRHSType_FUNCTION_CALL:{
          // TODO: Proper error reporting
          Assert(false);
        } break;
        case ConfigRHSType_SYMBOLIC_EXPR:{
          // TODO: Proper error reporting
          Assert(false);
        } break;
        case ConfigRHSType_IDENTIFIER:{
          // Only tackling simple forms, for now.
          Assert(stmt->lhs.expr);
          Assert(stmt->rhsId.expr);
        
          // Check that left entity supports memory access
          Array<String> accessExpr = PushArray<String>(temp,1);
          accessExpr[0] = stmt->lhs.name;

          // MARK
          Opt<Entity> entityOpt = GetEntityFromHierAccessWithEnvironment(&declaration->info,env,accessExpr);

          if(!entityOpt){
            // TODO: Proper error reporting.
            printf("Error, entity does not exist\n");
            exit(-1);
          }
          Entity entity = entityOpt.value();
          Assert(entity.type == EntityType_NODE);
          Assert(entity.inst->decl->memoryMapBits.has_value());
          
          //AddOrSetVariable(env,simple->rhsId.name,VariableType_VOID_PTR);

          ConfigStuff* assign = list->PushElem();
          assign->type = ConfigStuffType_MEMORY_TRANSFER;
          assign->transfer.dir = TransferDirection_READ;
          assign->transfer.identity = "TOP_m_addr";
          assign->transfer.sizeExpr = "1";
          assign->transfer.variable = PushString(out,simple->rhsId.name);
        } break;
      }
      } break;
      case ConfigStatementType_FOR_LOOP:{
        ConfigStatement* ptr = stmt;
        auto forLoops = PushArenaList<AddressGenForDef>(temp);

        for(int i = 0; i < stmts.size - 1; i++){
          *forLoops->PushElem() = stmts[i]->def;
        }
        
        Assert(simple->type == ConfigStatementType_STATEMENT);
        Assert(simple->rhsType == ConfigRHSType_IDENTIFIER);
        
        // Need to build an address gen in here or something that we can then use to extract the info that we need.
      } break;
    }
    }
  }

  auto varStuff = PushArenaList<ConfigVariable>(out);
  
  for(ConfigVarDeclaration decl : variables){
    ConfigVariable* var = varStuff->PushElem();
    
    var->type = decl.type;
    var->name = PushString(out,decl.name);
  }

  ConfigFunction func = {};
  func.type = type;
  func.decl = declaration;
  func.stuff = PushArrayFromList(out,list);
  func.variables = PushArrayFromList(out,varStuff);
  func.individualName = PushString(out,def->name);
  func.fullName = GlobalConfigFunctionName(func.individualName,declaration,out);
  func.newStructs = PushArrayFromList(out,newStructs);
  func.structToReturnName = structToReturnName;

  ConfigFunction* res = nameToFunction->Insert(func.fullName,func);
  
  return res;  
}
