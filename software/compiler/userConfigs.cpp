#include "userConfigs.hpp"

#include "accelerator.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "symbolic.hpp"
#include "declaration.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include "CEmitter.hpp"
#include "versatSpecificationParser.hpp"

// ============================================================================
// Instantiation and manipulation

static String GlobalConfigFunctionName(String functionName,FUDeclaration* decl,Arena* out){
  String name = PushString(out,"%.*s_%.*s",UN(decl->name),UN(functionName));
  return name;
}

struct ParseResult{
  bool isArray;
  bool isFunctionInvoc;
  bool containsAccess;
  bool isExpr;
  
  SYM_Expr expr;
  Array<SpecExpression*> args;
  Token entityName;
  Token wireName; 
  Token functionName;
};

struct DecompConfigStatement{
  // Single type
  bool isFunctionInvoc;

  // LHS type
  bool isVirtualWire;
  bool isSingleWire;
  bool isEntityOnly;
  
  // RHS type
  bool isExprOnly;
  bool isArrayExpr;
  bool isHierAccess;

  bool isArray;
  bool isExpr;

  Entity* parentEntity;
  Entity* subEntity;

  SYM_Expr expr;
  Array<SpecExpression*> args;
  Token entityName;
  Token wireName; 

  ConfigFunction* func;
};

DecompConfigStatement DecomposeConfigStatement(Env* env,ConfigStatement* stmt,Arena* out){
  DecompConfigStatement res = {};
  
  Assert(stmt->type != ConfigStatementType_FOR_LOOP);
  if(stmt->type == ConfigStatementType_FUNCTION_CALL){
    res.isFunctionInvoc = true;
    
    Entity* ent = env->GetEntity(stmt->lhs,out);
    
    res.func = ent->func;
    res.args = stmt->lhs->parent->arguments;
  }
  if(stmt->type == ConfigStatementType_EQUALITY){
    // Left hand side
    Entity* lhs = env->GetEntity(stmt->lhs,out);

    res.parentEntity = lhs;
    res.subEntity = nullptr;

    if(IsEntitySubType(lhs->type)){
      Assert(lhs->parent);

      res.parentEntity = lhs->parent;
      res.subEntity = lhs;
    }

    if(res.subEntity && res.subEntity->type == EntityType_MEM_PORT){
      // Statement of the form mem.in0 = val
      // This can only be an expression on the other side.
      res.isVirtualWire = true;
    } else if(res.subEntity){
      // Statement of the form ent.wire = val
      res.isSingleWire = true;
    } else {
      res.isEntityOnly = true;
    }

    // Right hand side
    SpecExpression* rhs = stmt->rhs;
    if(rhs->type == SpecType_ARRAY_ACCESS){
      // Statement of the form x = addr[expr]
      // This almost always implies a VUnit access 
      res.isArray = true;
      res.expr = env->SymbolicFromSpecExpression(rhs->expressions[0]);
      res.entityName = rhs->name;
    } else if(rhs->type == SpecType_SINGLE_ACCESS){
      // Statement of the form x = ent.wire
      // This is mostly state statements
      res.isHierAccess = true;
      res.entityName = rhs->name;
      res.wireName = rhs->expressions[0]->name;
    } else {    
      // Only expressions remain as valid statements
      res.isExpr = true;
      res.expr = env->SymbolicFromSpecExpression(rhs);
    }
  }
  
  return res;
}

// TODO: We could do better. We could have a decompose ConfigStatement
ParseResult ParseRHS(Env* env,SpecExpression* top,Arena* out){
/*
  RHS can be:

  wire           'name.wire'
  addrExpr       'addr[expr]'
  expression     'expr'
*/

  // TODO: We still need to access env to check if this makes sense or not.
  //       nocheckin

  ParseResult res = {};
  if(top->type == SpecType_ARRAY_ACCESS){
    res.isArray = true;
    res.expr = env->SymbolicFromSpecExpression(top->expressions[0]);
    res.entityName = top->name;
  } else if(top->type == SpecType_SINGLE_ACCESS){
    res.containsAccess = true;
    res.entityName = top->name;
    res.wireName = top->expressions[0]->name;
  } else {    
    res.isExpr = true;
    res.expr = env->SymbolicFromSpecExpression(top);
  }

  return res;
}

ConfigFunction* InstantiateConfigFunction(Env* env,ConfigFunctionDef* def,FUDeclaration* declaration,String content,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<ConfigStuff>(temp);
  ConfigFunctionType type = {};

  // TODO: Where is the error checking being perform to check if a given composite module contains the actual function?
  String structToReturnName = "void";
  Array<ConfigVarDeclaration> variables = def->variables;
  Array<Token> variableNames = Extract(variables,temp,&ConfigVarDeclaration::name);

  env->PushScope();

  for(Token name : variableNames){
    env->AddVariable(name);
  }
  
  // TODO: This flow is not good. With a bit more work we probably can join state and config into the same flow or at least avoid duplicating work. For now we are mostly prototyping so gonna keep pushing what we have.

  // Break apart every for loop + statement into individual statements.
  // NOTE: Kinda slow but should not be a problem anytime soon.
  TrieMap<ConfigStatement*,ConfigStatement*>* nodeToParent = PushTrieMap<ConfigStatement*,ConfigStatement*>(temp);
  auto simpleStmtList = PushList<ConfigStatement*>(temp);

  auto Recurse = [nodeToParent,simpleStmtList](auto Recurse,ConfigStatement* top) -> void{
    if(top->type == ConfigStatementType_FOR_LOOP){
      for(ConfigStatement* child : top->childs){
        nodeToParent->Insert(child,top);
        Recurse(Recurse,child);
      }
    }
    if(IsLeaf(top->type)){
      *simpleStmtList->PushElem() = top;
    }
  };

  for(ConfigStatement* top : def->statements){
    Recurse(Recurse,top);
  }

  auto stmtList = PushList<Array<ConfigStatement*>>(temp);
  for(ConfigStatement* stmt : simpleStmtList){
    auto list = PushList<ConfigStatement*>(temp);
    
    ConfigStatement* ptr = stmt;
    while(ptr){
      *list->PushElem() = ptr;

      ConfigStatement** possibleParent = nodeToParent->Get(ptr);
      ConfigStatement* parent = possibleParent ? *possibleParent : nullptr;

      ptr = parent;
    }

    Array<ConfigStatement*> asArray = PushArray(temp,list);
    ReverseInPlace(asArray);
    *stmtList->PushElem() = asArray;
  }

  // From this point on use this. Every N sized array is composed of N-1 FOR_LOOP types and 1 STATEMENT type.
  // TODO: Remember, after pushing every statement into an individual loop, we need to do error checking and check if the variable still exists. We cannot do variable checking globally since some statements might not be inside one of the loops.
  Array<Array<ConfigStatement*>> individualStatements = PushArray(temp,stmtList);

  // TODO: Kinda stupid calculating things this way but the rest of the code needs to collapse into a simpler form for the more robust approach first.
  auto variablesUsedOnLoopExpressions = PushTrieSet<String>(temp);

  for(Array<ConfigStatement*> arr : individualStatements){
    for(ConfigStatement* conf : arr){
      if(conf->type != ConfigStatementType_FOR_LOOP){
        continue;
      }

      AddressGenForDef def = conf->def;

      {
        auto tokens = AccumTokens(def.startSym,temp);

        for(Token tok : tokens){
          variablesUsedOnLoopExpressions->Insert(tok.identifier); 
        }
      }

      {
        auto tokens = AccumTokens(def.endSym,temp);

        for(Token tok : tokens){
          variablesUsedOnLoopExpressions->Insert(tok.identifier); 
        }
      }
    }
  }

  Array<ConfigVariable> varInfo = PushArray<ConfigVariable>(out,def->variables.size);
  Array<String> varNames = PushArray<String>(out,def->variables.size); 
  for(int i = 0; i < variables.size; i++){
    ConfigVarDeclaration decl = variables[i];
    Token typeTok = decl.type;

    ConfigVarType type = ConfigVarType_SIMPLE;

    if(typeTok.identifier == "Address"){
      type = ConfigVarType_ADDRESS;
    } else if(typeTok.identifier == "Fixed"){
      type = ConfigVarType_FIXED;
    } else if(typeTok.identifier == "Dyn"){
      type = ConfigVarType_DYN;
    } else if(!Empty(typeTok.identifier)){
      // TODO: Proper error report, can only be one of three
      env->ReportError(typeTok,"Not a valid variable type: Must be one of: 'Address', 'Fixed' or 'Dyn'");
    }
    
    varInfo[i].type = type;
    varInfo[i].name = PushString(out,decl.name.identifier);

    varNames[i] = varInfo[i].name;
  }

  bool supportsSizeCalc = true;
  for(ConfigVariable var : varInfo){
    if(variablesUsedOnLoopExpressions->Exists(var.name)){
      if(var.type != ConfigVarType_FIXED && var.type != ConfigVarType_DYN){
        supportsSizeCalc = false;

        // TODO: Better error calculation.
        printf("[WARNING] UserConfig function \"%.*s\" does not support runtime size calculations because var \"%.*s\" which is part of loop logic is not defined as Fixed or Dyn\n",UN(def->name.identifier),UN(var.name));
      }
    }
  }

  if(def->type == UserConfigType_CONFIG){
    type = ConfigFunctionType_CONFIG;

    for(Array<ConfigStatement*> stmts : individualStatements){
      ConfigStatement* simple = stmts[stmts.size - 1];
      // TODO: Call entity function to make sure that the entity exists and it is a config wire

      String lhsName = GetBase(simple->lhs)->name.identifier;
      
      auto forLoops = PushList<AddressGenForDef>(temp);

      for(int i = 0; i < stmts.size - 1; i++){
        *forLoops->PushElem() = stmts[i]->def;
      }

      Array<AddressGenForDef> loops = PushArray(temp,forLoops);

      DecompConfigStatement decomp = DecomposeConfigStatement(env,simple,temp);

      // TODO: We can clear this code even further.
      Entity* ent = decomp.parentEntity; //env->GetEntity(simple->lhs,temp);
      Entity* wireEnt = decomp.subEntity;
      Entity* portEnt = decomp.subEntity;

      // Function invocation is basically argument instantiation and replacing the 
      // statements with the new version.
      if(decomp.isFunctionInvoc){
        Array<SpecExpression*> args = decomp.args;
        ConfigFunction* function = decomp.func;

        // TODO: Not an assert, should be a proper error
        Assert(function->type == ConfigFunctionType_CONFIG);

        if(!function){
          // TODO: Error
          printf("Error 2, function does not exist, make sure the name is correct\n");
          exit(-1);
          return nullptr;
        }

        if(args.size != function->variables.size){
          printf("Error 2.1, number of arguments does not match\n");
          exit(-1);
          return nullptr;
        }
      
        // Invocation var to function argument
        TrieMap<String,SYM_Expr>* argToVar = PushTrieMap<String,SYM_Expr>(temp);
        for(int i = 0; i <  args.size; i++){
          // Arg is in the function space
          ConfigVariable arg = function->variables[i];

          SYM_Expr var = env->SymbolicFromSpecExpression(args[i]);

          // TODO: Kinda stupid.
          Array<String> vars = SYM_GetAllVariables(var,temp);
          if(arg.usedOnLoopExpressions){
            for(String s : vars){
              variablesUsedOnLoopExpressions->Insert(s);
            }
          }

          argToVar->Insert(arg.name,var);
        }
      
        for(ConfigStuff stuff : function->stuff){
          // TODO: We are building the struct access expression in here but I got a feeling that we probably want to preserve data as much as possible in order to tackle merge later on.
          FULL_SWITCH(stuff.type){
          case ConfigStuffType_ASSIGNMENT:{
            String lhs = PushString(out,"%.*s.%.*s",UN(lhsName),UN(stuff.assign.lhs));
        
            SYM_Expr rhs = stuff.assign.rhs;
            SYM_Expr newExpr = SYM_Replace(rhs,argToVar);
        
            ConfigStuff* newAssign = list->PushElem();
            newAssign->type = ConfigStuffType_ASSIGNMENT;
            newAssign->assign.lhs = lhs;
            newAssign->assign.rhs = newExpr;
          } break;
          case ConfigStuffType_ADDRESS_GEN:{
            AccessAndType access = stuff.access;

            ConfigStuff* newAccess = list->PushElem();
            newAccess->type = ConfigStuffType_ADDRESS_GEN;

            // TODO: By doing stuff this way we do not allow expressions inside functions.
            //       We cannot have ent.func(expr + expr) for example since we assume that the expression
            //       inside is just simple substitution.
            newAccess->access = access;
            newAccess->access.access = ReplaceVariables(access.access,argToVar,varNames,out);
            newAccess->lhs = lhsName + stuff.lhs;
          } break;
          case ConfigStuffType_MEMORY_TRANSFER:{
            // We should never have memory transfers at config functions, right?
            NOT_IMPLEMENTED();
          } break;
        }
        }
      } else if(decomp.isSingleWire){
        // We are setting a value to a constant wire.
        ConfigIdentifier* before = GetBeforeBase(simple->lhs);
        String wireName = before->name.identifier;

        ConfigStuff* assign = list->PushElem();
        assign->type = ConfigStuffType_ASSIGNMENT;
        assign->assign.lhs = PushString(out,"%.*s.%.*s",UN(lhsName),UN(wireName));
        assign->assign.rhs = decomp.expr;
      } else if(decomp.isVirtualWire || decomp.isExpr || decomp.isArray){
        // Is Address gen expression. Including array accesses for VUnits
        AddressAccess* access = CompileAddressGen(env,variableNames,loops,decomp.expr,content);
        AddressGenInst supported = ent->instance->declaration->supportedAddressGen;

        // NOTE: Memories and Generator do not follow the addr[expr]. They just have <instance> = <expr>.
        ConfigStuff* newAssign = list->PushElem();

        newAssign->type = ConfigStuffType_ADDRESS_GEN;
        newAssign->access.access = access;
        newAssign->access.inst = supported;

        if(portEnt){
          newAssign->access.dir = portEnt->dir;
          newAssign->access.port = portEnt->port;
        }

        newAssign->accessVariableName = PushString(out,decomp.entityName.identifier);
        newAssign->lhs = lhsName;
      } else {
        Assert(false);
      }
    }
  }

  String stateStructContent = {};

  if(def->type == UserConfigType_STATE){
    type = ConfigFunctionType_STATE;

    CEmitter* c = StartCCode(temp);
    
    String structName = PushString(out,"%.*s_%.*s_Struct",UN(declaration->name),UN(def->name.identifier));
    structToReturnName = structName;

    c->Struct(structName);
    for(ConfigStatement* stmt : def->statements){
      String name = GetBase(stmt->lhs)->name.identifier;
          
      String wireName = {};
      
      ConfigIdentifier* before = GetBeforeBase(stmt->lhs);
      if(before){
        wireName = before->name.identifier;
      }      

      // TODO: Proper error reporting. 
      Assert(Empty(wireName));

      c->Member("int",name);
    }
    c->EndStruct();

    stateStructContent = PushASTRepr(c,out,false);

    for(ConfigStatement* stmt : def->statements){
      String name = GetBase(stmt->lhs)->name.identifier;

      String wireName = {};
      
      ConfigIdentifier* before = GetBeforeBase(stmt->lhs);
      if(before){
        wireName = before->name.identifier;
      }

      Entity* ent = env->GetEntity(stmt->rhs,temp);

      ParseResult parsedRhs = ParseRHS(env,stmt->rhs,temp);

      if(ent->type == EntityType_CONFIG_FUNCTION){
        String varName = parsedRhs.entityName.identifier;
        
        for(ConfigStuff stmt : ent->func->stuff){
          ConfigStuff* assign = list->PushElem();
          assign->type = ConfigStuffType_ASSIGNMENT;
          assign->assign.lhs = name;
          assign->assign.rhsId = PushString(out,"%.*s.%.*s",UN(varName),UN(stmt.assign.rhsId));
        }      
      } else if(parsedRhs.containsAccess){
        ConfigStuff* assign = list->PushElem();
          
        String wireName = parsedRhs.wireName.identifier;
        String varName = parsedRhs.entityName.identifier;

        assign->type = ConfigStuffType_ASSIGNMENT;
        assign->assign.lhs = name;
        assign->assign.rhsId = PushString(out,"%.*s.%.*s",UN(varName),UN(wireName));
      }
    }
  }
  
  if(def->type == UserConfigType_MEM){
    type = ConfigFunctionType_MEM;

    for(Array<ConfigStatement*> stmts : individualStatements){
      ConfigStatement* stmt = stmts[0];
      ConfigStatement* simple = stmts[stmts.size - 1];

      bool singleStatement = (stmts.size == 1);
      
      // We currently only support a single statement or a single loop.
      // We technically can do multi loops just fine, just need to augment the logic to support it.
      Assert(singleStatement || (stmt->type == ConfigStatementType_FOR_LOOP 
                              && IsLeaf(simple->type)));

      DecompConfigStatement decomp = DecomposeConfigStatement(env,simple,temp);
      
      if(decomp.isFunctionInvoc){
        ConfigFunction* func = decomp.func;

        // TODO: Not an assert, should be a proper error
        Assert(func->type == ConfigFunctionType_MEM);

        for(ConfigStuff stuff : func->stuff){
          ConfigStuff* assign = list->PushElem();
          assign->type = ConfigStuffType_MEMORY_TRANSFER;

          //nocheckin: This probably only currently works because variable have the same names
          // TODO: Need to create more complex tests to force the issue
          assign->transfer = stuff.transfer;
          assign->transfer.name = simple->lhs->name.identifier + assign->transfer.name;
        }
      } else {
        ParseResult parsedRhs = ParseRHS(env,simple->rhs,temp);

        Assert(parsedRhs.isArray);

        Entity* left = env->GetEntity(simple->lhs,temp);
        Entity* right = env->GetEntity(parsedRhs.entityName);

        Entity* unit = nullptr;
        Entity* addrVar = nullptr;

        TransferDirection dir = TransferDirection_NONE;
        if(left->type == EntityType_FU){
          Assert(right->type == EntityType_VARIABLE_INPUT);
          dir = TransferDirection_READ;

          unit = left;
          addrVar = right;
        } else {
          Assert(left->type == EntityType_VARIABLE_INPUT);
          dir = TransferDirection_WRITE;

          unit = right;
          addrVar = left;
        }

        SYM_Expr size = SYM_One;
        if(!singleStatement){
          SYM_Expr start = env->SymbolicFromSpecExpression(stmt->def.startSym);
          SYM_Expr end = env->SymbolicFromSpecExpression(stmt->def.endSym);

          size = end - start;
        }

        ConfigStuff* assign = list->PushElem();
        assign->type = ConfigStuffType_MEMORY_TRANSFER;
        assign->transfer.dir = dir;
        assign->transfer.size = size;
        assign->transfer.name = assign->transfer.name + unit->instance->name;

        assign->transfer.variable = PushString(out,addrVar->varName);
      }
    }
  }

  for(int i = 0; i < variables.size; i++){
    varInfo[i].usedOnLoopExpressions = variablesUsedOnLoopExpressions->Exists(varInfo[i].name);
  }

  ConfigFunction* func = PushStruct<ConfigFunction>(out);
  func->type = type;
  func->decl = declaration;
  func->stuff = PushArray(out,list);
  func->variables = varInfo;
  func->individualName = PushString(out,def->name.identifier);
  func->fullName = GlobalConfigFunctionName(func->individualName,declaration,out);
  func->structToReturnName = structToReturnName;
  func->stateStructContent = stateStructContent;
  func->debug = def->debug;
  func->supportsSizeCalc = supportsSizeCalc;

  env->PopScope();
  
  return func;  
}
