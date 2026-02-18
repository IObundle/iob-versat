#include "versatSpecificationParser.hpp"

#include "accelerator.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "embeddedData.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "symbolic.hpp"
#include "templateEngine.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "userConfigs.hpp"

#include "newParser.hpp"

// ======================================
// Constants

static SpecExpression SPEC_LITERAL_0 = {.val = 0,.type = SpecType_LITERAL};

// nocheckin
// TODO: Need to take an Env to properly check stuff.
SymbolicExpression* SymbolicFromSpecExpression(SpecExpression* spec,Arena* out){
  TEMP_REGION(temp,out);  

  auto Recurse = [temp](auto Recurse,SpecExpression* top) -> SymbolicExpression*{
    SymbolicExpression* res = nullptr;

    switch(top->type){
    case SpecType_OPERATION:{
      SymbolicExpression* left  = Recurse(Recurse,top->expressions[0]);
      SymbolicExpression* right = Recurse(Recurse,top->expressions[1]);

      if(top->op[0] == '+'){
        res = SymbolicAdd(left,right,temp);
      }
      if(top->op[0] == '-'){
        res = SymbolicSub(left,right,temp);
      }
      if(top->op[0] == '*'){
        res = SymbolicMult(left,right,temp);
      }
      if(top->op[0] == '/'){
        res = SymbolicDiv(left,right,temp);
      }

      Assert(res);
    } break;
    case SpecType_NAME:
    case SpecType_VAR:{
      res = PushVariable(temp,top->name);
    } break;
    case SpecType_LITERAL:{
      res = PushLiteral(temp,top->val);
    } break;
    case SpecType_SINGLE_ACCESS:
    case SpecType_ARRAY_ACCESS:
    case SpecType_FUNCTION_CALL: Assert(false);
    }

    return res;
  };

  SymbolicExpression* res = Recurse(Recurse,spec);

  return Normalize(res,out);
}

// TODO: Rework expression parsing to support error reporting similar to module diff.
//       A simple form of synchronization after detecting an error would vastly improve error reporting
//       Change iterative and merge to follow module def.
//       Need to detect multiple inputs to the same port and report error.
//       Error reporting is very generic. Implement more specific forms.
//       This parser is still not production ready. At the very least all the Asserts should be removed and replaced
//         by actual error reporting. Not a single Assert is programmer error detector, all the current ones are
//         user error that most be reported.


// TODO: This functions could show more lines of code before and after so we can actually see where the problem is.
void ReportError(String content,Token faultyToken,String error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("[Error]\n");
  printf("%.*s:\n",UN(error));
  printf("%.*s\n",UN(loc));
  printf("\n");
}

void ReportError2(String content,Token faultyToken,Token goodToken,String faultyError,String good){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);
  String loc2 = GetRichLocationError(content,goodToken,temp);
  
  printf("[Error]\n");
  printf("%.*s:\n",UN(faultyError));
  printf("%.*s\n",UN(loc));
  printf("%.*s:\n",UN(good));
  printf("%.*s\n",UN(loc2));
  printf("\n");
}

SpecExpression* ParseSpecExpression(Parser* parser,Arena* out,int bindingPower = 99);

String GetUniqueName(String name,Arena* out,InstanceTable* names){
  int counter = 0;
  String uniqueName = name;
  auto mark = MarkArena(out);
  while(names->Exists(uniqueName)){
    PopMark(mark);
    uniqueName = PushString(out,"%.*s_%d",UN(name),counter++);
  }

  names->Insert(uniqueName,nullptr);

  return uniqueName;
}

String GetActualArrayName(String baseName,int index,Arena* out){
  return PushString(out,"%.*s_%d",UN(baseName),index);
}

FUDeclaration* InstantiateMerge(MergeDef def){
  TEMP_REGION(temp,nullptr);
  
  auto declArr = StartArray<FUDeclaration*>(temp);
  for(TypeAndInstance tp : def.declarations){
    FUDeclaration* decl = GetTypeByNameOrFail(tp.typeName); // TODO: Rewrite stuff so that at this point we know that the type must exist
    *declArr.PushElem() = decl;
  }
  Array<FUDeclaration*> decl = EndArray(declArr);

  String name = PushString(globalPermanent,def.name);

  bool error = false;
  MergeModifier modifier = MergeModifier_NONE;

  for(Token t : def.mergeModifiers){
    Opt<MergeModifier> parsed = META_mergeModifiers_ReverseMap(t);

    if(!parsed.has_value()){
      printf("Error, merge does not support option: %.*s\n",UN(t));
      error = true;
    }

    modifier = (MergeModifier) (modifier | parsed.value());
  }

  if(error){
    return nullptr;
  }
  
  return Merge(decl,name,def.specifics,modifier);
}

int GetRangeCount(Env* env,Range<SpecExpression*> range){
  int start = env->CalculateConstantExpression(range.start);
  int end = env->CalculateConstantExpression(range.end);
  
  Assert(end >= start);
  return (end - start + 1);
}

// Connection type and number of connections
Pair<PortRangeType,int> GetConnectionInfo(Env* env,Var var){
  int indexCount = GetRangeCount(env,var.index);
  int portCount = GetRangeCount(env,var.extra.port);
  int delayCount = GetRangeCount(env,var.extra.delay);

  if(indexCount == 1 && portCount == 1 && delayCount == 1){
    return {PortRangeType_SINGLE,1};
  }

  // We cannot have more than one range at the same time because otherwise how do we decide how to connnect them?
  if((indexCount != 1 && portCount != 1) ||
     (indexCount != 1 && delayCount != 1) ||
     (portCount != 1 && delayCount != 1)){
    return {PortRangeType_ERROR,0};
  }
  
  if(var.isArrayAccess && indexCount != 1){
    return {PortRangeType_ARRAY_RANGE,indexCount};
  }

  if(portCount != 1){
    return {PortRangeType_PORT_RANGE,portCount};
  } else if(delayCount != 1){
    return {PortRangeType_DELAY_RANGE,delayCount};
  }

  NOT_POSSIBLE("Every condition should have been checked by now");
}

bool IsValidGroup(Env* env,VarGroup group){
  // TODO: Wether we can match the group or not.
  //       It depends on wether the ranges line up or not. 
  for(Var& var : group.vars){
    if(GetConnectionInfo(env,var).first == PortRangeType_ERROR){
      return false;
    }
  }

  return true;
}

int NumberOfConnections(Env* env,VarGroup group){
  Assert(IsValidGroup(env,group));

  int count = 0;

  for(Var& var : group.vars){
    count += GetConnectionInfo(env,var).second;
  }

  return count;
}

GroupIterator IterateGroup(VarGroup group){
  GroupIterator iter = {};
  iter.group = group;
  return iter;
}

bool HasNext(GroupIterator iter){
  if(iter.groupIndex >= iter.group.vars.size){
    return false;
  }

  return true;
}

struct ConnectionStartInfo{
  bool isPort;
  bool isDelay;
  bool isArray;

  Token name;

  int port;
  int delay;
  int arrayIndex;
};

ConnectionStartInfo Next(GroupIterator& iter){
  Assert(HasNext(iter));

  ConnectionStartInfo res = {};

  Var var = iter.group.vars[iter.groupIndex];
  Pair<PortRangeType,int> info = GetConnectionInfo(iter.env,var);
  
  PortRangeType type = info.first;
  int maxCount = info.second;

  Assert(type != PortRangeType_ERROR);

  res.name = var.name;

  res.port = iter.env->CalculateConstantExpression(var.extra.port.start);
  res.delay = iter.env->CalculateConstantExpression(var.extra.delay.start);
  res.arrayIndex = iter.env->CalculateConstantExpression(var.index.start);

  if(type == PortRangeType_SINGLE){
    iter.groupIndex += 1;
  } else {
    switch(type){
    case PortRangeType_SINGLE: break;
    case PortRangeType_ERROR: break;
    case PortRangeType_PORT_RANGE:{
      res.isPort = true;
      res.port += iter.varIndex;
    } break;
    case PortRangeType_DELAY_RANGE:{
      res.isDelay = true;
      res.delay += iter.varIndex;
    } break;
    case PortRangeType_ARRAY_RANGE:{
      res.isArray = true;
      res.arrayIndex += iter.varIndex;
    } break;
    }

    iter.varIndex += 1;
    if(iter.varIndex >= maxCount){
      iter.varIndex = 0;
      iter.groupIndex += 1;
    }
  }
  
  return res;
}

FUInstance* CreateFUInstanceWithParameters(Accelerator* accel,FUDeclaration* type,String name,InstanceDeclaration decl){
  FUInstance* inst = CreateFUInstance(accel,type,name);
  
  for(auto pair : decl.parameters){
    SymbolicExpression* expr = SymbolicFromSpecExpression(pair.second,globalPermanent);
    bool result = SetParameter(inst,pair.first,expr);

    if(!result){
      printf("Warning: Parameter %.*s for instance %.*s in module %.*s does not exist\n",UN(pair.first),UN(inst->name),UN(accel->name));
    }
  }

  return inst;
}

// TODO: Move this function to a better place
FUDeclaration* InstantiateModule(String content,ModuleDef def){
  Arena* perm = globalPermanent;
  Accelerator* circuit = CreateAccelerator(def.name,AcceleratorPurpose_MODULE);

  FREE_ARENA(envArena);
  FREE_ARENA(envArena2);
  Env* env = StartEnvironment(envArena,envArena2);

  env->circuit = circuit;

  for(VarDeclaration& decl : def.inputs){
    env->AddInput(decl);
  }

  int shareIndex = 0;
  for(InstanceDeclaration& decl : def.declarations){
    if(decl.modifier == InstanceDeclarationType_SHARE_CONFIG){
      decl.shareIndex = shareIndex++;
    }
  }

  for(InstanceDeclaration& decl : def.declarations){
    for(VarDeclaration& var : decl.declarations){
      env->AddInstance(decl,var);
    }
  }

  for(ConnectionDef& decl : def.connections){
    Assert(decl.type != ConnectionType_NONE);
    if(decl.type == ConnectionType_EQUALITY){
      env->AddEquality(decl);
    } else if(decl.type == ConnectionType_CONNECTION){
      env->AddConnection(decl);
    }
  }

  if(!Empty(env->errors)){
    for(String str : env->errors){
      printf("%.*s\n",UN(str));
    }
    
    exit(-1);
  }
  
  FUDeclaration* res = RegisterSubUnit(circuit,SubUnitOptions_BAREBONES);
  
  {
    TEMP_REGION(temp,nullptr);
    auto list = PushList<ConfigFunction*>(temp);
    for(auto funcDecl : def.configs){
      *list->PushElem() = InstantiateConfigFunction(env,&funcDecl,res,content,globalPermanent);
    };
    
    if(res->info.infos.size){
      res->info.infos[0].userFunctions = PushArray(perm,list);
    }
  }
  
  return res;
}

bool IsModuleLike(ConstructDef def){
  FULL_SWITCH(def.type){
  case ConstructType_MODULE:
  case ConstructType_MERGE:
  case ConstructType_ITERATIVE:
    return true;
    break;
    break;
} END_SWITCH();

  Assert(false);
  return false;
}

Array<Token> TypesUsed(ConstructDef def,Arena* out){
  TEMP_REGION(temp,out);

  FULL_SWITCH(def.type){
  case ConstructType_MERGE: {
    // TODO: How do we deal with same types being used?
    //       Do we just ignore it?
    Array<Token> result = Extract(def.merge.declarations,out,&TypeAndInstance::typeName);
    
    return result;
  } break;
  case ConstructType_MODULE: {
    Array<Token> result = Extract(def.module.declarations,temp,&InstanceDeclaration::typeName);

    return Unique(result,out);
  } break;
  case ConstructType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  };
} END_SWITCH();

  return {};
}

// TODO: Move this function to a better place, no reason to be inside the spec parser.
FUDeclaration* InstantiateSpecifications(String content,ConstructDef def){
  FULL_SWITCH(def.type){
  case ConstructType_MERGE: {
    return InstantiateMerge(def.merge);
  } break;
  case ConstructType_MODULE: {
    return InstantiateModule(content,def.module);
  } break;
  case ConstructType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  }; 
  default: Assert(false);
  } END_SWITCH();

  return nullptr;
}

// ======================================
// Hierarchical access

Env* StartEnvironment(Arena* freeUse,Arena* freeUse2){
  Env* env = PushStruct<Env>(freeUse);
  env->scopeArena = freeUse;
  env->miscArena = freeUse2;
  env->scopes = PushArray<EnvScope*>(freeUse,99);

  env->currentScope = -1;
  env->PushScope();

  env->errors = PushList<String>(freeUse2);
  env->table = PushTrieMap<String,FUInstance*>(freeUse2);

  return env;
}

void Env::ReportError(Token badToken,String msg){
  // TODO: More detailed error message
  String error = PushString(miscArena,"[%.*s] %.*s: '%.*s'",UN(circuit->name),UN(msg),UN(badToken));
  *this->errors->PushElem() = error;
}

void Env::PushScope(){
  this->currentScope += 1;

  ArenaMark mark = MarkArena(this->scopeArena);
  this->scopes[this->currentScope] = PushStruct<EnvScope>(this->scopeArena);
  this->scopes[this->currentScope]->mark = mark;
  this->scopes[this->currentScope]->variable = PushTrieMap<String,Entity>(this->scopeArena);
}

void Env::PopScope(){
  Assert(this->currentScope > 0);

  ArenaMark mark = this->scopes[this->currentScope]->mark;
  PopMark(mark);

  this->currentScope -= 1;
}

FUInstance* Env::CreateInstance(FUDeclaration* type,String name){
  FUInstance* inst = CreateFUInstance(circuit,type,name);

  Token tok = {};
  tok = *((Token*) &inst->name);
  Entity* ent = PushNewEntity(tok);
  ent->type = EntityType_FU;
  ent->instance = inst;

  return inst;
}

FUInstance* Env::GetFUInstance(Token name,int arrayIndexIfArray){
  TEMP_REGION(temp,nullptr);

  FUInstance* res = nullptr;
  if(name == "out"){
    res = GetOutputInstance();
  } else {
    Entity* ent = GetEntity(name);

    // TODO: Error reporting if entity does not exist.

    String asStr = name;
    if(ent->type == EntityType_FU_ARRAY){
      asStr = GetActualArrayName(asStr,arrayIndexIfArray,temp);
    }

    res = table->GetOrElse(asStr,nullptr);
  }
  return res;
}

FUInstance* Env::GetFUInstance(Var var){
  TEMP_REGION(temp,nullptr);
  
  FUInstance* res = nullptr;
  if(var.name == "out"){
    if(var.isArrayAccess){
      ReportError(var.name,"'out' special unit cannot have array subscriptions");
    }
    
    res = GetOutputInstance();
  } else {
    Entity* ent = GetEntity(var.name);
    
    String name = var.name;
    if(ent->type == EntityType_FU_ARRAY){
      int index = CalculateConstantExpression(var.index.low);
      name = GetActualArrayName(name,index,temp);
    }

    res = table->GetOrElse(name,nullptr);
  }
  return res;
}

FUInstance* Env::GetOutputInstance(){
  FUInstance* res = GetUnit(circuit,"out");

  if(!res){
    res = CreateFUInstance(circuit,BasicDeclaration::output,"out");
  }
  
  return res;
}

Entity* Env::PushNewEntity(Token name){
  if(name == "out"){
    ReportError(name,"Cannot have a variable with the reserved name out");
  }

  auto res = this->scopes[this->currentScope]->variable->GetOrAllocate(name);
  
  if(res.alreadyExisted){
    ReportError(name,"Entity already exists. Rename entity to resolve conflict");
  }

  return res.data;
}

Entity* Env::GetEntity(Token name){
  for(int i = this->currentScope; i >= 0; i--){
    Entity* ent = this->scopes[i]->variable->Get(name);

    if(ent){
      return ent;
    }
  }

  return nullptr;
}

Entity* Env::GetEntity(ConfigIdentifier* id,Arena* out){
  TEMP_REGION(temp,out);

  ConfigIdentifier* ptr = id;
  Assert(ptr->type == ConfigAccessType_BASE);

  Entity* ent = GetEntity(ptr->name);
  
  ptr = ptr->parent;
  for(; ent && ptr; ptr = ptr->parent){
    Entity* nextEnt = nullptr;

    switch(ptr->type){
      case ConfigAccessType_BASE:{
        Assert(false);
      } break;
      case ConfigAccessType_ARRAY:{
        if(ent->type != EntityType_FU_ARRAY){
          ReportError({},"Cannot access array since entity is not an array");
        }

        // NOTE: At this point there are two things to do.
        //       Either we check if the expression is a constant and at that point we try to return the actual FU variable.
        //       Or, if the expression is not a constant we just return that the entity is a FU_ARRAY.
        // TODO: Right now we are just assuming that it is a FU_ARRAY since we are just trying to implement the memory user config stuff

        nextEnt = ent;
      } break;
      case ConfigAccessType_ACCESS:{
        Token access = ptr->name;

        SWITCH(ent->type){
        case EntityType_FU:{
          Direction dir = Direction_NONE;
          int port = 0;
          if(access == "out0"){
            dir = Direction_OUTPUT;
          }
          if(access == "out1"){
            dir = Direction_OUTPUT;
            port = 1;
          }
          if(access == "in0"){
            dir = Direction_INPUT;
          }
          if(access == "in1"){
            dir = Direction_INPUT;
            port = 1;
          }
          
          if(dir != Direction_NONE){
            Entity* virtualWire = PushStruct<Entity>(out);
            virtualWire->type = EntityType_MEM_PORT;
            virtualWire->dir = dir;
            virtualWire->port = port;
            virtualWire->parent = ent;
            
            if(nextEnt) ReportError({},"Name collision");
            nextEnt = virtualWire;
          } 
          
          FUDeclaration* decl = ent->instance->declaration;
            
          // TODO: We need to check if we have the same name for stuff. If
          //       wire has the same name as a userConfig function then we have a problem and this
          //       code will not act correctly. Furthermore, this check needs to be done elsewhere.
          //       By the time we reach this point we are probably too far. Otherwise we need to 
          for(Wire& w : decl->configs){
            if(w.name == access){
              Entity* wire = PushStruct<Entity>(out);
              wire->wire = &w;
              wire->type = EntityType_CONFIG_WIRE;
              
              if(nextEnt) ReportError({},"Name collision");
              nextEnt = wire;
              break;
            }
          }

          for(Wire& w : decl->states){
            if(w.name == access){
              Entity* wire = PushStruct<Entity>(out);
              wire->wire = &w;
              wire->type = EntityType_STATE_WIRE;
              
              if(nextEnt) ReportError({},"Name collision");
              nextEnt = wire;
              break;
            }
          }
 
          for(MergePartition part : decl->info.infos){
            for(ConfigFunction* func : part.userFunctions){
              if(func->individualName == access){
                Entity* userFunc = PushStruct<Entity>(out);

                userFunc->func = func;
                userFunc->type = EntityType_CONFIG_FUNCTION;
          
                if(nextEnt) ReportError({},"Name collision");
                nextEnt = userFunc;
              }
            }
          }
        } break;
        case EntityType_NODE:{
          Assert(false); // We do not have nodes since Env is only used currently to store FUInstances and other parseed data stuff. Nothing concrete exists at this point in the code.
        } break;

        case EntityType_CONFIG_FUNCTION:
        case EntityType_FU_ARRAY:
        case EntityType_MEM_PORT:
        case EntityType_CONFIG_WIRE:
        case EntityType_STATE_WIRE:
        case EntityType_VARIABLE_INPUT:
        case EntityType_VARIABLE_SPECIAL:{
          ReportError(access,"Cannot have access expression for entity of this type");
        } break;
      }
      } break;
    }

    ent = nextEnt;
  }

  return ent;
}

int Env::CalculateConstantExpression(SpecExpression* top){
  TEMP_REGION(temp,nullptr);

  // TODO: SLOW AND NOT ROBUST TO ERRORS
  SymbolicExpression* expr = SymbolicFromSpecExpression(top,temp);

  int val = Evaluate(expr,nullptr);
  return val;
}

Entity* Env::GetEntity(SpecExpression* id,Arena* out){
  TEMP_REGION(temp,out);

  Entity* ent = nullptr;

  if(id->type == SpecType_NAME){
    ent = GetEntity(id->name);
  }
  if(id->type == SpecType_ARRAY_ACCESS){
    NOT_IMPLEMENTED();
  }
  if(id->type == SpecType_SINGLE_ACCESS){
    ent = GetEntity(id->name);
    
    SpecExpression* accessExpr = id->expressions[0];
    Assert(accessExpr->type == SpecType_NAME);
    
    Token access = accessExpr->name;

    Direction dir = Direction_NONE;
    int port = 0;
    if(access == "out0"){
      dir = Direction_OUTPUT;
    }
    if(access == "out1"){
      dir = Direction_OUTPUT;
      port = 1;
    }
    if(access == "in0"){
      dir = Direction_INPUT;
    }
    if(access == "in1"){
      dir = Direction_INPUT;
      port = 1;
    }

    Entity* nextEnt = nullptr;
    if(dir != Direction_NONE){
      Entity* virtualWire = PushStruct<Entity>(out);
      virtualWire->type = EntityType_MEM_PORT;
      virtualWire->dir = dir;
      virtualWire->port = port;
      virtualWire->parent = ent;
            
      nextEnt = virtualWire;
    } 
          
    FUDeclaration* decl = ent->instance->declaration;
            
    // TODO: We need to check if we have the same name for stuff. If
    //       wire has the same name as a userConfig function then we have a problem and this
    //       code will not act correctly. Furthermore, this check needs to be done elsewhere.
    //       By the time we reach this point we are probably too far. Otherwise we need to 
    for(Wire& w : decl->configs){
      if(w.name == access){
        Entity* wire = PushStruct<Entity>(out);
        wire->wire = &w;
        wire->type = EntityType_CONFIG_WIRE;
              
        if(nextEnt) ReportError({},"Name collision");
        nextEnt = wire;
        break;
      }
    }

    for(Wire& w : decl->states){
      if(w.name == access){
        Entity* wire = PushStruct<Entity>(out);
        wire->wire = &w;
        wire->type = EntityType_STATE_WIRE;
              
        if(nextEnt) ReportError({},"Name collision");
        nextEnt = wire;
        break;
      }
    }
 
    for(MergePartition part : decl->info.infos){
      for(ConfigFunction* func : part.userFunctions){
        if(func->individualName == access){
          Entity* userFunc = PushStruct<Entity>(out);

          userFunc->func = func;
          userFunc->type = EntityType_CONFIG_FUNCTION;
          
          if(nextEnt) ReportError({},"Name collision");
          nextEnt = userFunc;
        }
      }
    }
    
    if(nextEnt){
      nextEnt->parent = ent;
      ent = nextEnt;
    }
  }

  return ent;
}


void Env::AddInput(VarDeclaration var){
  TEMP_REGION(temp,nullptr);

  Entity* ent = PushNewEntity(var.name);

  if(var.isArray){
    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = var.name;
    ent->arraySize = var.arraySize;

    for(int i = 0; i < var.arraySize; i++){
      String actualName = GetActualArrayName(var.name,i,temp);
      FUInstance* input = CreateOrGetInput(circuit,actualName,insertedInputs++);
      table->Insert(input->name,input);
    }
  } else {
    FUInstance* input = CreateOrGetInput(circuit,var.name,insertedInputs++);
    table->Insert(input->name,input);
    
    ent->type = EntityType_FU;
    ent->instance = input;
  }

  ent->isInput = true;
}

void Env::AddInstance(InstanceDeclaration decl,VarDeclaration var){
  TEMP_REGION(temp,nullptr);

  FUDeclaration* type = GetTypeByName(decl.typeName);
  
  if(!type){
    ReportError(decl.typeName,"Typename does not exist");
  }

  Entity* ent = PushNewEntity(var.name);

  if(var.isArray){
    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = var.name;
    ent->arraySize = var.arraySize;

    for(int i = 0; i < var.arraySize; i++){
      String actualName = GetActualArrayName(var.name,i,temp);
      FUInstance* inst = CreateFUInstanceWithParameters(circuit,type,actualName,decl);
      table->Insert(inst->name,inst);
    }
  } else {
    FUInstance* inst = CreateFUInstanceWithParameters(circuit,type,var.name,decl);
    table->Insert(inst->name,inst);

    ent->type = EntityType_FU;
    ent->instance = inst;
  }

  for(auto iter = StartIteration(this,ent); iter.IsValid(); iter = iter.Next()){
    FUInstance* inst = iter.Current();
    
    switch(decl.modifier){
    case InstanceDeclarationType_NONE: break;
    case InstanceDeclarationType_SHARE_CONFIG:{
      ShareInstanceConfig(inst,decl.shareIndex);
      
      for(Token partialShareName : decl.shareNames){
        bool foundOne = false;
        for(int ii = 0; ii < inst->declaration->configs.size; ii++){
          if(inst->declaration->configs[ii].name == partialShareName){
            inst->isSpecificConfigShared[ii] = false;
            foundOne = true;
          }
        }

        if(!foundOne){
          TEMP_REGION(temp,nullptr);
          String errorMsg = PushString(temp,"Cannot share config wire since it does not exist for instance '%.*s' of type '%.*s'",UN(inst->name),UN(decl.typeName));
          ReportError(partialShareName,errorMsg);
        }
      }
    } break;
    case InstanceDeclarationType_STATIC:{
      SetStatic(inst);
    } break;
    }
  }
}

void Env::AddConnection(ConnectionDef decl){
  Assert(decl.type == ConnectionType_CONNECTION);

  int nOutConnections = NumberOfConnections(this,decl.output);
  int nInConnections = NumberOfConnections(this,decl.input);

  if(nOutConnections != nInConnections){
    ReportError({},"Connection missmatch");
  }

  GroupIterator out = IterateGroup(decl.output);
  GroupIterator in  = IterateGroup(decl.input);

  while(HasNext(out) && HasNext(in)){
    ConnectionStartInfo outVar = Next(out);
    ConnectionStartInfo inVar = Next(in);
      
    FUInstance* outInstance = GetFUInstance(outVar.name,outVar.arrayIndex);
    FUInstance* inInstance = GetFUInstance(inVar.name,inVar.arrayIndex);

    int outPort = outVar.port;
    int inPort  = inVar.port;
    ConnectUnits(outInstance,outPort,inInstance,inPort,outVar.delay);
  }

  Assert(HasNext(out) == HasNext(in));
}

void Env::AddEquality(ConnectionDef decl){
  TEMP_REGION(temp,nullptr);
  
  Assert(decl.type == ConnectionType_EQUALITY);

  // Only allow one for equality, for now
  Assert(decl.output.vars.size == 1);

  Var outVar = decl.output.vars[0];
  PortExpression portSpecExpression = InstantiateSpecExpression(decl.expression);

  // When dealing with equality, we can just increase array size by accessing higher and higher values.
  if(outVar.isArrayAccess){
    auto res = this->scopes[this->currentScope]->variable->GetOrAllocate(outVar.name);
    Entity* ent = res.data;

    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = outVar.name;
    ent->arraySize = MAX(ent->arraySize,CalculateConstantExpression(outVar.index.low));
  }

  String name = outVar.name;
  if(outVar.isArrayAccess){
    name = GetActualArrayName(name,CalculateConstantExpression(outVar.index.low),temp);
  }

  FUInstance* inst = portSpecExpression.inst;
  String uniqueName = GetUniqueName(name,globalPermanent,table);
  inst->name = PushString(globalPermanent,uniqueName);

  Token tok = {};
  tok = *((Token*) &inst->name);
  Entity* ent = PushNewEntity(tok);
  ent->type = EntityType_FU;
  ent->instance = inst;

  table->Insert(inst->name,inst);
}

void Env::AddVariable(Token name){
  Entity* ent = GetEntity(name);

  if(ent){
    ReportError(name,"This name is already being used");
  }

  Entity* entity = PushNewEntity(name);

  entity->type = EntityType_VARIABLE_INPUT;
  entity->varName = name;
}

FUInstanceIterator FUInstanceIterator::Next(){
  FUInstanceIterator next = *this;
  next.index += 1;
  return next;
}

bool FUInstanceIterator::IsValid(){
  bool res;
  if(max == 0){
    res = (index == 0);
  } else {
    res = (index < max);
  }
  
  return res;
}

FUInstance* FUInstanceIterator::Current(){
  Assert(IsValid());

  FUInstance* inst;
  if(this->max == 0){
    return ent->instance;
  } else {
    TEMP_REGION(temp,nullptr);
    String baseName = ent->arrayBaseName;
    String actualName = GetActualArrayName(baseName,index,temp);

    inst = GetUnit(this->env->circuit,actualName);
  }
  
  return inst;
}

FUInstanceIterator StartIteration(Env* env,Entity* ent){
  FUInstanceIterator iter = {};
  iter.env = env;
  iter.ent = ent;

  if(ent->type == EntityType_FU_ARRAY){
    iter.max = ent->arraySize;
  }

  return iter;
}

PortExpression Env::InstantiateSpecExpression(SpecExpression* root){
  Arena* perm = globalPermanent;
  PortExpression res = {};

  switch(root->type){
    // Just to remove warnings. TODO: Change expression so that multiple locations have their own expression struct, instead of reusing the same one.
  case SpecType_LITERAL:{
    int number = root->val;

    TEMP_REGION(temp,perm);
    String toSearch = PushString(temp,"N%d",number);

    FUInstance** found = table->Get(toSearch);

    if(!found){
      String uniqueName = GetUniqueName(toSearch,perm,table);

      FUInstance* digitInst = (FUInstance*) CreateInstance(GetTypeByName("Literal"),uniqueName);
      digitInst->literal = number;
      table->Insert(digitInst->name,digitInst);
      res.inst = digitInst;
    } else {
      res.inst = *found;
    }
  }break;
  case SpecType_VAR:{
    Var var = root->var;  
    String name = var.name;

    if(var.isArrayAccess){
      name = GetActualArrayName(var.name,CalculateConstantExpression(var.index.bottom),globalPermanent);
    }
    
    FUInstance* inst = table->GetOrFail(name);

    res.inst = inst;
    res.extra = var.extra;
  } break;
  case SpecType_OPERATION:{
    PortExpression expr0 = InstantiateSpecExpression(root->expressions[0]);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr0.extra.port.start == expr0.extra.port.end);
    Assert(expr0.extra.delay.start == expr0.extra.delay.end);

    if(root->expressions.size == 1){
      Assert(root->op[0] == '~' || root->op[0] == '-');

      String typeName = {};

      switch(root->op[0]){
      case '~':{
        typeName = "NOT";
      }break;
      case '-':{
        typeName = "NEG";
      }break;
      }

      String permName = GetUniqueName(typeName,perm,table);
      FUInstance* inst = CreateInstance(GetTypeByName(typeName),permName);
      table->Insert(inst->name,inst);

      int start = CalculateConstantExpression(expr0.extra.port.start);
      int delay = CalculateConstantExpression(expr0.extra.delay.start);
      ConnectUnits(expr0.inst,start,inst,0,delay);

      res.inst = inst;
      res.extra.port.end  = res.extra.port.start  = &SPEC_LITERAL_0;
      res.extra.delay.end = res.extra.delay.start = &SPEC_LITERAL_0;

      return res;
    } else {
      Assert(root->expressions.size == 2);
    }

    PortExpression expr1 = InstantiateSpecExpression(root->expressions[1]);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr1.extra.port.start == expr1.extra.port.end);
    Assert(expr1.extra.delay.start == expr1.extra.delay.end);

    String op = root->op;
    const char* typeName;
    if(CompareString(op,"&")){
      typeName = "AND";
    } else if(CompareString(op,"|")){
      typeName = "OR";
    } else if(CompareString(op,"^")){
      typeName = "XOR";
    } else if(CompareString(op,">><")){
      typeName = "RHR";
    } else if(CompareString(op,">>")){
      typeName = "SHR";
    } else if(CompareString(op,"><<")){
      typeName = "RHL";
    } else if(CompareString(op,"<<")){
      typeName = "SHL";
    } else if(CompareString(op,"+")){
      typeName = "ADD";
    } else if(CompareString(op,"-")){
      typeName = "SUB";
    } else {
      // TODO: Proper error reporting
      printf("%.*s\n",UN(op));
      Assert(false);
    }

    String typeStr = typeName;
    FUDeclaration* type = GetTypeByName(typeStr);
    String uniqueName = GetUniqueName(type->name,perm,table);

    FUInstance* inst = CreateInstance(type,uniqueName);
    table->Insert(inst->name,inst);

    int start0 = CalculateConstantExpression(expr0.extra.port.start);
    int delay0 = CalculateConstantExpression(expr0.extra.delay.start);

    int start1 = CalculateConstantExpression(expr1.extra.port.start);
    int delay1 = CalculateConstantExpression(expr1.extra.delay.start);

    ConnectUnits(expr0.inst,start0,inst,0,delay0);
    ConnectUnits(expr1.inst,start1,inst,1,delay1);

    res.inst = inst;
    res.extra.port.end  = res.extra.port.start  = &SPEC_LITERAL_0;
    res.extra.delay.end = res.extra.delay.start = &SPEC_LITERAL_0;
  } break;
  }

  Assert(res.inst);
  return res;
}

// nochecking
// TODO: Just to remove the syntax errors from the compiler
Token C(NewToken t){
  Token res = {};
  res.data = t.ptr;
  res.size = t.identifier.size;
  return res;
}

SpecExpression* ParseNumberOnly(Parser* parser,Arena* out){
  SpecExpression* res = PushStruct<SpecExpression>(out);

  res->val = parser->ExpectNext(NewTokenType_NUMBER).number;
  res->type = SpecType_LITERAL;

  return res;
}

Range<SpecExpression*> ParseRange(Parser* parser,Arena* out){
  Range<SpecExpression*> res = {};

  SpecExpression* n1 = ParseNumberOnly(parser,out);

  Assert(n1);

  res.start = n1;
  res.end = n1;

  if(parser->IfNextToken(NewTokenType_DOUBLE_DOT)){
    res.end = ParseNumberOnly(parser,out);
  }
  
  return res;
}

Var ParseVar(Parser* parser,Arena* out){
  Token name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

  bool isArrayAccess = false;
  SpecExpression* indexStart = &SPEC_LITERAL_0;
  SpecExpression* indexEnd = &SPEC_LITERAL_0;
  if(parser->IfNextToken('[')){
    Range<SpecExpression*> range = ParseRange(parser,out);
    indexStart = range.start;
    indexEnd = range.end;

    isArrayAccess = true;

    parser->ExpectNext(']');
  }
  
  SpecExpression* delayStart = &SPEC_LITERAL_0;
  SpecExpression* delayEnd = &SPEC_LITERAL_0;
  if(parser->IfNextToken('{')){
    Range<SpecExpression*> range = ParseRange(parser,out);
    delayStart = range.start;
    delayEnd = range.end;

    parser->ExpectNext('}');
  }

  SpecExpression* portStart = &SPEC_LITERAL_0;
  SpecExpression* portEnd = &SPEC_LITERAL_0;
  if(parser->IfNextToken(':')){
    Range<SpecExpression*> range = ParseRange(parser,out);

    portStart = range.start;
    portEnd = range.end;
  }

  Var var = {};
  var.name = name;
  var.isArrayAccess = isArrayAccess;
  var.index.start = indexStart;
  var.index.end = indexEnd;
  var.extra.delay.start = delayStart;
  var.extra.delay.end = delayEnd;
  var.extra.port.start = portStart;
  var.extra.port.end = portEnd;

  return var;
}

VarDeclaration ParseVarDeclaration(Parser* parser){
  VarDeclaration res = {};

  res.name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));
  
  // TODO: We should integrate the array parsing logic with this one
  if(parser->IfNextToken('[')){
    NewToken number = parser->ExpectNext(NewTokenType_NUMBER);
    int arraySize = number.number;

    parser->ExpectNext(']');

    res.arraySize = arraySize;
    res.isArray = true;
  }
  
  return res;
}

Array<VarDeclaration> ParseModuleInputDeclaration(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  auto vars = PushList<VarDeclaration>(temp);

  parser->ExpectNext('(');

  if(parser->IfNextToken(')')){
    return {};
  }
  
  while(!parser->Done()){
    VarDeclaration var = ParseVarDeclaration(parser);
    *vars->PushElem() = var;
    
    if(parser->IfNextToken(',')){
      continue;
    } else {
      break;
    }
  }

  parser->ExpectNext(')');

  Array<VarDeclaration> res = PushArray(out,vars);
  return res;
}

InstanceDeclaration ParseInstanceDeclaration(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  InstanceDeclaration res = {};

  InstanceDeclarationType modifier = InstanceDeclarationType_NONE;

  // TODO: Is it correct if we see a bunch of repeated modifiers? Think 'static static static'.
  while(1){
    NewToken potentialModifier = parser->PeekToken();

    InstanceDeclarationType parsedModifier = InstanceDeclarationType_NONE;
    if(potentialModifier.type == NewTokenType_KEYWORD_DEBUG){
      parser->NextToken();

      res.debug = true;
    } else if(potentialModifier.type == NewTokenType_KEYWORD_STATIC){
      parser->NextToken();
      parsedModifier = InstanceDeclarationType_STATIC;
    } else if(potentialModifier.type == NewTokenType_KEYWORD_SHARE){
      parser->NextToken();
      parser->ExpectNext('(');
      parser->ExpectNext(NewTokenType_KEYWORD_CONFIG);
      parser->ExpectNext(')');

      parsedModifier = InstanceDeclarationType_SHARE_CONFIG;
      
      res.typeName = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

      if(parser->IfNextToken('(')){
        // TODO: For now, we assume that every wire specified inside the spec file is a negative (remove share).
        auto toShare = StartArray<Token>(out);
        while(!parser->Done()){
          Token name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

          *toShare.PushElem() = name;

          if(parser->IfNextToken(',')){
            continue;
          } else {
            break;
          }
        }
      
        parser->ExpectNext(')');

        res.shareNames = EndArray(toShare);
      }

      parser->ExpectNext('{');

      auto array = StartArray<VarDeclaration>(out);
    
      while(!parser->Done()){
        NewToken peek = parser->PeekToken();

        if(peek.type == '}'){
          break;
        }

        *array.PushElem() = ParseVarDeclaration(parser);
      
        parser->ExpectNext(';');
      }
      res.declarations = EndArray(array);
    
      parser->ExpectNext('}');
      res.modifier = parsedModifier;
      // TODO: Weird logic. Already caused a bug, potentially need to rewrite this.
      return res;
    } else {
      break;
    }

    if(modifier == InstanceDeclarationType_NONE){
      modifier = parsedModifier;
    }
  }

  res.typeName = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

  NewToken possibleParameters = parser->PeekToken();
  auto list = PushList<Pair<String,SpecExpression*>>(temp);
  if(possibleParameters.type == '#'){
    parser->NextToken();
    parser->ExpectNext('(');

    while(!parser->Done()){
      parser->ExpectNext('.');
      Token parameterName = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

      parser->ExpectNext('(');

      SpecExpression* expr = ParseSpecExpression(parser,out);

      parser->ExpectNext(')');
      
      String savedParameter = PushString(out,parameterName);
      *list->PushElem() = {savedParameter,expr}; 

      if(parser->IfNextToken(',')){
        continue;
      }

      break;
    }
    parser->ExpectNext(')');

    res.parameters = PushArray(out,list);
  }

  VarDeclaration varDecl = ParseVarDeclaration(parser);

  res.declarations = PushArray<VarDeclaration>(out,1);
  res.declarations[0] = varDecl;
  res.modifier = modifier;

  parser->ExpectNext(';');

  return res;
}

// A group can also be a single var. It does not necessary mean that it must be of the form {...}
VarGroup ParseVarGroup(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  if(parser->IfNextToken('{')){
    auto vars = PushList<Var>(temp);

    while(!parser->Done()){
      Var var = ParseVar(parser,out);

      *vars->PushElem() = var;

      NewToken sepOrEnd = parser->NextToken();
      if(sepOrEnd.type == ','){
        continue;
      } else if(sepOrEnd.type == '}'){
        break;
      } else {
        parser->ReportUnexpectedToken(sepOrEnd,{TOK_TYPE(','),TOK_TYPE('}')});
      }
    }

    VarGroup res = {};
    res.vars = PushArray(out,vars);
    return res;
  } else {
    Var var = ParseVar(parser,out);

    VarGroup res = {};
    res.vars = PushArray<Var>(out,1);
    res.vars[0] = var;
    return res;
  }
}

// nocheckin 
// TODO: We do actually want to separate the parsing from a connection expression and the expressions used by the address gen stuff.
//       The problem that we had before is that we parsed symbolic directly instead of parsing into an ExpressionStruct and then we tried to check the semantic.
//       Basically, in the past, instead of doing Text -> Tokens -> Symbolic we where doing Text -> Symbolic which is the problem.
SpecExpression* ParseMathExpression(Parser* parser,Arena* out,int bindingPower = 99);

SpecExpression* ParseMathExpression(Parser* parser,Arena* out,int bindingPower){
  SpecExpression* topUnary = nullptr;
  SpecExpression* innerMostUnary = nullptr;

  SpecExpression* res = nullptr;
  
  // Parse unary
  while(!parser->Done()){
    SpecExpression* parsed = nullptr;
    if(!parsed &&  parser->IfNextToken('-')){
      parsed = PushStruct<SpecExpression>(out);
      parsed->op = "-";
    }

    if(parsed){
      parsed->type = SpecType_OPERATION;
    }

    if(parsed && !topUnary){
      topUnary = parsed;
      innerMostUnary = parsed;
      continue;
    }

    if(parsed){
      innerMostUnary->expressions = PushArray<SpecExpression*>(out,1);
      innerMostUnary->expressions[0] = parsed;
      innerMostUnary = parsed;
      continue;
    }

    break;
  }

  // Parse atom
  NewToken atom = parser->PeekToken();
  if(atom.type == '('){
    parser->ExpectNext('(');

    res = ParseMathExpression(parser,out);

    parser->ExpectNext(')');
  } else if(atom.type == NewTokenType_NUMBER){
    NewToken number = parser->ExpectNext(NewTokenType_NUMBER);
    res = PushStruct<SpecExpression>(out);

    res->type = SpecType_LITERAL;
    res->val = number.number;
  } else if(atom.type == NewTokenType_IDENTIFIER){
    TEMP_REGION(temp,out);

    Token name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));
    
    res = PushStruct<SpecExpression>(out);
    res->name = name;
    res->type = SpecType_NAME;

    if(parser->IfPeekToken('[')){
      auto accesses = PushList<SpecExpression*>(temp);       
      
      while(parser->IfNextToken('[')){
        SpecExpression* insideArray = ParseMathExpression(parser,out);

        *accesses->PushElem() = insideArray;

        parser->ExpectNext(']');
      }

      res->expressions = PushArray(out,accesses);
      res->type = SpecType_ARRAY_ACCESS;
    }

    // TODO: This is mostly for state right side.
    //       We might eventually just separate this into different parsing functions.
    if(parser->IfNextToken('.')){
      NewToken singleAccessName = parser->ExpectNext(NewTokenType_IDENTIFIER);
      
      SpecExpression* singleAccess = PushStruct<SpecExpression>(out);
      singleAccess->type = SpecType_NAME;
      singleAccess->name = C(singleAccessName);
      
      res->expressions = PushArray<SpecExpression*>(out,1);
      res->expressions[0] = singleAccess;
      res->type = SpecType_SINGLE_ACCESS;
    }

    if(parser->IfNextToken('(')){
      auto args = PushList<SpecExpression*>(temp);       
      
      while(!parser->Done()){
        SpecExpression* arg = ParseMathExpression(parser,out);

        *args->PushElem() = arg;
        
        if(parser->IfNextToken(',')){
          continue;
        }
        
        break;
      }

      parser->ExpectNext(')');
      
      res->expressions = PushArray(out,args);
      res->type = SpecType_FUNCTION_CALL;
    }
  } else {
    // TODO: Better error reporting
    parser->ReportUnexpectedToken(atom,{});
  }

  if(topUnary){
    innerMostUnary->expressions = PushArray<SpecExpression*>(out,1);
    innerMostUnary->expressions[0] = res;

    res = topUnary;
  }

  struct OpInfo{
    NewTokenType type;
    int bindingPower;
    const char* op;
  };

  // TODO: This should be outside the function itself.
  TEMP_REGION(temp,out);

  // TODO: Need to double check binding power
  auto infos = PushArray<OpInfo>(temp,4);
  infos[0] = {TOK_TYPE('/'),0,"/"};
  infos[1] = {TOK_TYPE('*'),0,"*"};

  infos[2] = {TOK_TYPE('+'),1,"+"};
  infos[3] = {TOK_TYPE('-'),1,"-"};
  
  // Parse binary ops.
  while(!parser->Done()){
    NewToken peek = parser->PeekToken();

    bool continueOuter = false;
    for(OpInfo info : infos){
      if(peek.type == info.type){
        if(info.bindingPower < bindingPower){
          parser->NextToken();

          SpecExpression* right = ParseMathExpression(parser,out,info.bindingPower);
      
          SpecExpression* op = PushStruct<SpecExpression>(out);

          op->op = info.op;
          op->expressions = PushArray<SpecExpression*>(out,2);
          op->expressions[0] = res;
          op->expressions[1] = right;

          res = op;
          continueOuter = true;
          break;
        }
      }
    }

    if(continueOuter){
      continue;
    }

    break;
  }
  
  return res;
}

SpecExpression* ParseSpecExpression(Parser* parser,Arena* out,int bindingPower){
  SpecExpression* topUnary = nullptr;
  SpecExpression* innerMostUnary = nullptr;

  SpecExpression* res = nullptr;
  
  // Parse unary
  while(!parser->Done()){
    SpecExpression* parsed = nullptr;
    if(!parsed && parser->IfNextToken('~')){
      parsed = PushStruct<SpecExpression>(out);
      parsed->op = "~";
    }
    if(!parsed &&  parser->IfNextToken('-')){
      parsed = PushStruct<SpecExpression>(out);
      parsed->op = "-";
    }

    if(parsed){
      parsed->type = SpecType_OPERATION;
    }

    if(parsed && !topUnary){
      topUnary = parsed;
      innerMostUnary = parsed;
      continue;
    }

    if(parsed){
      innerMostUnary->expressions = PushArray<SpecExpression*>(out,1);
      innerMostUnary->expressions[0] = parsed;
      innerMostUnary = parsed;
      continue;
    }

    break;
  }

  // Parse atom
  NewToken atom = parser->PeekToken();
  if(atom.type == '('){
    parser->ExpectNext('(');

    res = ParseSpecExpression(parser,out);

    parser->ExpectNext(')');
  } else if(atom.type == NewTokenType_NUMBER){
    NewToken number = parser->ExpectNext(NewTokenType_NUMBER);
    res = PushStruct<SpecExpression>(out);

    res->type = SpecType_LITERAL;
    res->val = number.number;
  } else if(atom.type == NewTokenType_IDENTIFIER){
    TEMP_REGION(temp,out);

    Var var = ParseVar(parser,out);
    
    res = PushStruct<SpecExpression>(out);
    res->var = var;
    res->type = SpecType_VAR;

    if(parser->IfPeekToken('[')){
      auto accesses = PushList<SpecExpression*>(temp);       
      
      while(parser->IfNextToken('[')){
        SpecExpression* insideArray = ParseSpecExpression(parser,out);

        *accesses->PushElem() = insideArray;

        parser->ExpectNext(']');
      }

      res->expressions = PushArray(out,accesses);
      res->type = SpecType_ARRAY_ACCESS;
    }

    // TODO: This is mostly for state right side.
    //       We might eventually just separate this into different parsing functions.
    if(parser->IfNextToken('.')){
      NewToken singleAccessName = parser->ExpectNext(NewTokenType_IDENTIFIER);
      
      SpecExpression* singleAccess = PushStruct<SpecExpression>(out);
      singleAccess->type = SpecType_NAME;
      singleAccess->name = C(singleAccessName);
      
      res->expressions = PushArray<SpecExpression*>(out,1);
      res->expressions[0] = singleAccess;
      res->type = SpecType_SINGLE_ACCESS;
    }

    if(parser->IfNextToken('(')){
      auto args = PushList<SpecExpression*>(temp);       
      
      while(!parser->Done()){
        SpecExpression* arg = ParseSpecExpression(parser,out);

        *args->PushElem() = arg;
        
        if(parser->IfNextToken(',')){
          continue;
        }
        
        break;
      }

      parser->ExpectNext(')');
      
      res->expressions = PushArray(out,args);
      res->type = SpecType_FUNCTION_CALL;
    }
  } else {
    // TODO: Better error reporting
    parser->ReportUnexpectedToken(atom,{});
  }

  if(topUnary){
    innerMostUnary->expressions = PushArray<SpecExpression*>(out,1);
    innerMostUnary->expressions[0] = res;

    res = topUnary;
  }

  struct OpInfo{
    NewTokenType type;
    int bindingPower;
    const char* op;
  };

  // TODO: This should be outside the function itself.
  TEMP_REGION(temp,out);
  auto infos = PushArray<OpInfo>(temp,11);

  // TODO: Need to double check binding power
  infos[0] = {TOK_TYPE('&'),0,"&"};
  infos[1] = {TOK_TYPE('|'),0,"|"};
  infos[2] = {TOK_TYPE('^'),0,"^"};

  infos[3] = {NewTokenType_ROTATE_LEFT,1,"><<"};
  infos[4] = {NewTokenType_ROTATE_RIGHT,1,">><"};
  infos[5] = {NewTokenType_SHIFT_LEFT,1,"<<"};
  infos[6] = {NewTokenType_SHIFT_RIGHT,1,">>"};

  infos[7]  = {TOK_TYPE('*'),2,"*"};
  infos[8] = {TOK_TYPE('/'),2,"/"};

  infos[9] = {TOK_TYPE('+'),3,"+"};
  infos[10] = {TOK_TYPE('-'),3,"-"};
  
  // Parse binary ops.
  while(!parser->Done()){
    NewToken peek = parser->PeekToken();

    bool continueOuter = false;
    for(OpInfo info : infos){
      if(peek.type == info.type){
        if(info.bindingPower < bindingPower){
          parser->NextToken();

          SpecExpression* right = ParseSpecExpression(parser,out,info.bindingPower);
      
          SpecExpression* op = PushStruct<SpecExpression>(out);

          op->op = info.op;
          op->expressions = PushArray<SpecExpression*>(out,2);
          op->expressions[0] = res;
          op->expressions[1] = right;

          res = op;
          continueOuter = true;
          break;
        }
      }
    }

    if(continueOuter){
      continue;
    }

    break;
  }
  
  return res;
}

ConnectionDef ParseConnection(Parser* parser,Arena* out){
  VarGroup outPortion = ParseVarGroup(parser,out);

  ConnectionType type = ConnectionType_NONE;
  
  if(parser->IfNextToken('=')){
    type = ConnectionType_EQUALITY;
  } else if(parser->IfNextToken(NewTokenType_XOR_EQUAL)){
    // TODO: We parse it but we do not use it. We probably wanna remove the testcase that uses this.
    type = ConnectionType_EQUALITY;
  } else if(parser->IfNextToken(NewTokenType_ARROW)){
    type = ConnectionType_CONNECTION;
  } else {
    parser->ReportUnexpectedToken(parser->NextToken(),{TOK_TYPE('='),NewTokenType_ARROW});
  }

  ConnectionDef def = {};
  SpecExpression* expr = nullptr;
  VarGroup inPortion = {};

  if(type == ConnectionType_EQUALITY){
    expr = ParseSpecExpression(parser,out);
  } else if(type == ConnectionType_CONNECTION){
    inPortion = ParseVarGroup(parser,out);
  }

  parser->ExpectNext(';');
  
  def.type = type;
  def.expression = expr;
  def.output = outPortion;
  def.input = inPortion;

  return def;
}

// TODO: nocheckin - remove forward decl
ConfigFunctionDef* ParseConfigFunction(Parser* parser,Arena* out);

// Any returned String points to tokenizer content.
// As long as tokenizer is valid, strings returned by this function are also valid.
ModuleDef ParseModuleDef(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  parser->ExpectNext(NewTokenType_KEYWORD_MODULE);

  Token name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));
  Array<VarDeclaration> vars = ParseModuleInputDeclaration(parser,out);

  //Token outputs = {};
  if(parser->IfNextToken(NewTokenType_ARROW)){
    // TODO: Right now we do not care about output info being given by the user.
    //       We probably should. We still parse it, we just ignore it for now.
    /* outputs = */ parser->ExpectNext(NewTokenType_NUMBER);
  }
  
  ArenaList<InstanceDeclaration>* decls = PushList<InstanceDeclaration>(temp);
  parser->ExpectNext('{');
  
  while(!parser->Done()){
    NewToken peek = parser->PeekToken();

    if(peek.type == ';'){
      parser->NextToken();
      continue;
    }

    if(peek.type == '#' || peek.type == NewTokenType_DOUBLE_HASHTAG || peek.type == '}'){
      break;
    }
    
    InstanceDeclaration decl = ParseInstanceDeclaration(parser,out);
    *decls->PushElem() = decl;
  }
  Array<InstanceDeclaration> declarations = PushArray(out,decls);

  ArenaList<ConnectionDef>* cons = PushList<ConnectionDef>(temp);
  if(parser->IfNextToken('#')){
    while(!parser->Done()){
      NewToken peek = parser->PeekToken();

      if(peek.type == ';'){
        parser->NextToken();
        continue;
      }

      if(peek.type == '}' || peek.type == NewTokenType_DOUBLE_HASHTAG){
        break;
      }

      ConnectionDef con = ParseConnection(parser,out);
 
      *cons->PushElem() = con;
    }
  }

  auto configFunctions = PushList<ConfigFunctionDef>(temp);

  // nocheckin
  if(parser->IfNextToken(NewTokenType_DOUBLE_HASHTAG)){
    while(!parser->Done()){
      // nocheckin TODO: Probably remove this and move the logic from the function to here
      if(IsNextTokenConfigFunctionStart(parser)){
        ConfigFunctionDef* func = ParseConfigFunction(parser,out);

        if(func){
          *configFunctions->PushElem() = *func;
        } else {
          printf("Error parsing user function\n");
        }
      } else {
        break;
      }
    }
  }
  
  parser->ExpectNext('}');

  ModuleDef def ={};

  def.name = name;
  def.inputs = vars;
  def.declarations = declarations;
  def.connections = PushArray(out,cons);
  def.configs = PushArray(out,configFunctions);
  
  return def;
}

TypeAndInstance ParseTypeAndInstance(Parser* parser){
  NewToken typeName = parser->ExpectNext(NewTokenType_IDENTIFIER);

  NewToken instanceName = {};
  if(parser->IfNextToken(':')){
    instanceName = parser->ExpectNext(NewTokenType_IDENTIFIER);
  }

  TypeAndInstance res = {};
  res.typeName = C(typeName);
  res.instanceName = C(instanceName);
  
  return res;
}

HierarchicalName ParseHierarchicalName(Parser* parser,Arena* out){
  NewToken topInstance = parser->ExpectNext(NewTokenType_IDENTIFIER);

  parser->ExpectNext('.');

  Var var = ParseVar(parser,out);

  HierarchicalName res = {};
  res.instanceName = C(topInstance);
  res.subInstance = var;

  return res;
}

MergeDef ParseMerge(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  parser->ExpectNext(NewTokenType_KEYWORD_MERGE);

  Array<Token> mergeModifiers = {};
  if(parser->IfNextToken('(')){
    auto tokenList = PushList<Token>(temp);
    
    while(!parser->Done()){
      NewToken peek = parser->PeekToken();
      
      if(peek.type == ')'){
        break;
      }

      *tokenList->PushElem() = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

      parser->IfNextToken(',');
    }

    mergeModifiers = PushArray(out,tokenList);
    
    parser->ExpectNext(')');
  }

  Token mergeName = C(parser->ExpectNext(NewTokenType_IDENTIFIER));
  
  parser->ExpectNext('=');

  ArenaList<TypeAndInstance>* declarationList = PushList<TypeAndInstance>(temp);
  while(!parser->Done()){
    TypeAndInstance typeInst = ParseTypeAndInstance(parser);

    *declarationList->PushElem() = typeInst;

    NewToken peek = parser->PeekToken();
    if(peek.type == '|'){
      parser->NextToken();
      continue;
    } else if(peek.type == '{'){
      break;
    } else if(peek.type == ';'){
      parser->NextToken();
      break;
    }
  }
  Array<TypeAndInstance> declarations = PushArray(out,declarationList);

  Array<SpecNode> specNodes = {};
  if(parser->IfNextToken('{')){
    ArenaList<SpecNode>* specList = PushList<SpecNode>(temp);
    while(!parser->Done()){
      NewToken peek = parser->PeekToken();
      if(peek.type == '}'){
        break;
      }

      HierarchicalName leftSide = ParseHierarchicalName(parser,out);
      parser->ExpectNext('-');
      HierarchicalName rightSide = ParseHierarchicalName(parser,out);
      parser->ExpectNext(';');

      *specList->PushElem() = {leftSide,rightSide};
    }
    specNodes = PushArray(out,specList);

    parser->ExpectNext('}');
  }
  
  auto specificsArr = StartArray<SpecificMergeNode>(out);
  for(SpecNode node : specNodes){
    // TODO: We do not do this in here. Parser only parses, semantic checks are performed later.
    int firstIndex = -1;
    int secondIndex = -1;
    for(int i = 0; i < declarations.size; i++){
      TypeAndInstance& decl = declarations[i];
      if(CompareString(node.first.instanceName,decl.instanceName)){
        firstIndex = i;
      } 
      if(CompareString(node.second.instanceName,decl.instanceName)){
        secondIndex = i;
      } 
    }

#if 0
    if(firstIndex == -1){
      Assert(false);
      // ReportError
    }
    if(secondIndex == -1){
      Assert(false);
      // ReportError
    }
#endif

    *specificsArr.PushElem() = {firstIndex,node.first.subInstance.name,secondIndex,node.second.subInstance.name};
  }
  Array<SpecificMergeNode> specifics = EndArray(specificsArr);

  MergeDef result = {};
  result.name = mergeName;
  result.declarations = declarations;
  result.specifics = specifics;
  result.mergeModifiers = mergeModifiers;
  
  return result;
}

Array<ConstructDef> ParseVersatSpecification(String content,Arena* out){
  TEMP_REGION(temp,out);

  auto TokenizeFunction = [](const char* start,const char* end) -> TokenizeResult{
    TokenizeResult res = ParseWhitespace(start,end);
    res |= ParseComments(start,end);

    res |= ParseMultiSymbol(start,end,">><",NewTokenType_ROTATE_RIGHT);
    res |= ParseMultiSymbol(start,end,"><<",NewTokenType_ROTATE_LEFT);

    res |= ParseMultiSymbol(start,end,"..",NewTokenType_DOUBLE_DOT);
    res |= ParseMultiSymbol(start,end,"##",NewTokenType_DOUBLE_HASHTAG);
    res |= ParseMultiSymbol(start,end,"->",NewTokenType_ARROW);
    res |= ParseMultiSymbol(start,end,">>",NewTokenType_SHIFT_RIGHT);
    res |= ParseMultiSymbol(start,end,"<<",NewTokenType_SHIFT_LEFT);
    res |= ParseMultiSymbol(start,end,"^=",NewTokenType_XOR_EQUAL);

    res |= ParseSymbols(start,end);
    res |= ParseNumber(start,end);

    res |= ParseIdentifier(start,end);

    if(res.token.type == NewTokenType_IDENTIFIER){
      String id = res.token.identifier;
      
      NewTokenType type = NewTokenType_INVALID;

      // TODO: We really need a fast way of checking this using size + character by character branching path.
      //       However this is something that we want to push to the meta function generation. We do not want to actually write this and potentially get it wrong.
      if(id == "module")     type = NewTokenType_KEYWORD_MODULE;
      if(id == "merge")      type = NewTokenType_KEYWORD_MERGE;
      if(id == "share")      type = NewTokenType_KEYWORD_SHARE;
      if(id == "static")     type = NewTokenType_KEYWORD_STATIC;
      if(id == "debug")      type = NewTokenType_KEYWORD_DEBUG;
      if(id == "config")     type = NewTokenType_KEYWORD_CONFIG;
      if(id == "state")      type = NewTokenType_KEYWORD_STATE;
      if(id == "mem")        type = NewTokenType_KEYWORD_MEM;
      if(id == "for")        type = NewTokenType_KEYWORD_FOR;

#if 0
      if(type == NewTokenType_INVALID && Parse_IsCKeyword(id)){
        res.token.type = NewTokenType_C_KEYWORD;
      }
      if(type == NewTokenType_INVALID && Parse_IsVerilogKeyword(id)){
        res.token.type = NewTokenType_VERILOG_KEYWORD;
      }
#endif

      if(type != NewTokenType_INVALID){
        res.token.type = type;
      }
    }

    return res;
  };

  FREE_ARENA(parseArena);
  Parser* parser = StartParsing(content,TokenizeFunction,parseArena,ParsingOptions_DEFAULT);

  ArenaList<ConstructDef>* typeList = PushList<ConstructDef>(temp);

  while(!parser->Done()){
    NewToken tok = parser->PeekToken();

    ConstructDef def = {};
    if(tok.type == NewTokenType_KEYWORD_MODULE){
      def.type = ConstructType_MODULE;
      def.module = ParseModuleDef(parser,out);
    } else if(tok.type == NewTokenType_KEYWORD_MERGE){
      def.type = ConstructType_MERGE;
      def.merge = ParseMerge(parser,out);
    } else {
      parser->ReportUnexpectedToken(tok,{NewTokenType_KEYWORD_MODULE,NewTokenType_KEYWORD_MERGE});
      parser->Synch({NewTokenType_KEYWORD_MODULE,NewTokenType_KEYWORD_MERGE});
    }

    *typeList->PushElem() = def;
  }

  if(parser->errors){
    for(String str : parser->errors){
      printf("%.*s\n",UN(str));
    }
    exit(-1);
  }

  Array<ConstructDef> defs = PushArray(out,typeList);

  return defs;
}

static ConfigIdentifier* ParseConfigIdentifier(Parser* parser,Arena* out){
  NewToken id = parser->ExpectNext(NewTokenType_IDENTIFIER);
  
  ConfigIdentifier* base = PushStruct<ConfigIdentifier>(out);
  base->type = ConfigAccessType_BASE;
  base->name = C(id);

  ConfigIdentifier* ptr = base;

  while(!parser->Done()){
    ConfigIdentifier* parsed = nullptr;

    if(!parsed && parser->IfNextToken('.')){
      Token access = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

      parsed = PushStruct<ConfigIdentifier>(out);
      parsed->type = ConfigAccessType_ACCESS;
      parsed->name = access;
    }

    if(!parsed && parser->IfNextToken('[')){
      SpecExpression* expr = ParseSpecExpression(parser,out);

      parser->ExpectNext(']');

      parsed = PushStruct<ConfigIdentifier>(out);
      parsed->type = ConfigAccessType_ARRAY;
      parsed->trueExpr = expr;
    }

    if(parsed){
      ptr->parent = parsed;
      ptr = parsed;
    }

    break;
  }

  return base;
}

static ConfigStatement* ParseConfigStatement(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  ConfigStatement* stmt = PushStruct<ConfigStatement>(out);

  if(parser->IfNextToken(NewTokenType_KEYWORD_FOR)){
    NewToken loopVariable = parser->ExpectNext(NewTokenType_IDENTIFIER);
 
    SpecExpression* start = ParseMathExpression(parser,out);
    parser->ExpectNext(NewTokenType_DOUBLE_DOT);
    SpecExpression* end = ParseMathExpression(parser,out);

    parser->ExpectNext('{');

    auto list = PushList<ConfigStatement*>(temp);
    while(!parser->Done()){
      ConfigStatement* child = ParseConfigStatement(parser,out);
      *list->PushElem() = child;

      if(parser->IfPeekToken('}')){
        break;
      }
    }

    parser->ExpectNext('}');
    
    stmt->def2.loopVariable = C(loopVariable);
    stmt->def2.startSym = start;
    stmt->def2.endSym = end;
    stmt->childs = PushArray(out,list);
    stmt->type = ConfigStatementType_FOR_LOOP;
  } else if(parser->IfPeekToken(NewTokenType_IDENTIFIER)) {
    stmt->lhs = ParseConfigIdentifier(parser,out);
    parser->ExpectNext('=');
    stmt->trueRhs = ParseMathExpression(parser,out);
    parser->ExpectNext(';');
    stmt->type = ConfigStatementType_STATEMENT;
  } else {
    parser->ReportUnexpectedToken(parser->NextToken(),{});
  }

  return stmt;
}

static ConfigVarDeclaration ParseConfigVarDeclaration(Parser* parser){
  ConfigVarDeclaration res = {};

  res.name = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

  if(parser->IfNextToken('[')){
    NewToken number = parser->ExpectNext(NewTokenType_NUMBER);
    int arraySize = number.number;

    parser->ExpectNext(']');

    res.arraySize = arraySize;
    res.isArray = true;
  }

  Token type = {};
  if(parser->IfNextToken(':')){
    type = C(parser->NextToken());
  }

  res.type = type;
  
  return res;
}

static Array<ConfigVarDeclaration> ParseConfigFunctionArguments(Parser* parser,Arena* out){
  auto array = StartArray<ConfigVarDeclaration>(out);

  parser->ExpectNext('(');
  
  while(!parser->Done()){
    if(parser->IfPeekToken(')')){
      break;
    }

    ConfigVarDeclaration var = ParseConfigVarDeclaration(parser);

    *array.PushElem() = var;
    
    if(parser->IfNextToken(',')){
      continue;
    } else {
      break;
    }
  }

  parser->ExpectNext(')');

  return EndArray(array);
}

ConfigFunctionDef* ParseConfigFunction(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  UserConfigType type = UserConfigType_NONE;
  if(parser->IfNextToken(NewTokenType_KEYWORD_CONFIG)){
    type = UserConfigType_CONFIG;
  } else if(parser->IfNextToken(NewTokenType_KEYWORD_MEM)){
    type = UserConfigType_MEM;
  } else if(parser->IfNextToken(NewTokenType_KEYWORD_STATE)){
    type = UserConfigType_STATE;
  }

  if(type == UserConfigType_NONE){
    return nullptr;
  }

  bool debug = parser->IfNextToken(NewTokenType_KEYWORD_DEBUG);
    
  Token configName = C(parser->ExpectNext(NewTokenType_IDENTIFIER));

  Array<ConfigVarDeclaration> functionVars = {};
  if(type == UserConfigType_MEM || type == UserConfigType_CONFIG){
    functionVars = ParseConfigFunctionArguments(parser,out);
  }

  parser->ExpectNext('{');

  auto stmts = PushList<ConfigStatement*>(temp);
  while(!parser->Done()){
    NewToken peek = parser->PeekToken();

    if(peek.type == '}'){
      break;
    }
        
    ConfigStatement* config = ParseConfigStatement(parser,out);
    *stmts->PushElem() = config;
  }

  parser->ExpectNext('}');

  ConfigFunctionDef* res = PushStruct<ConfigFunctionDef>(out);
  res->type = type;
  res->name = configName;
  res->variables = functionVars;
  res->statements = PushArray(out,stmts);
  res->debug = debug;

  return res;
}

Array<Token> AccumTokens(SpecExpression* top,Arena* out){
  TEMP_REGION(temp,out);

  auto AccumTokens = [](auto AccumTokens,SpecExpression* top,ArenaList<Token>* list) -> void {
    switch(top->type){
    case SpecType_LITERAL: break;
    case SpecType_NAME: {
      *list->PushElem() = top->name;
    } break;
    case SpecType_VAR: {
      *list->PushElem() = top->var.name;
    } break;
    case SpecType_OPERATION: {
    } break;
    case SpecType_SINGLE_ACCESS: {
      *list->PushElem() = top->name;
    } break;
    case SpecType_ARRAY_ACCESS: {
      *list->PushElem() = top->name;
    } break;
    case SpecType_FUNCTION_CALL: {
      *list->PushElem() = top->name;
    } break;
    }
    for(SpecExpression* child : top->expressions){
      AccumTokens(AccumTokens,child,list);
    }
  };

  auto list = PushList<Token>(temp);
  AccumTokens(AccumTokens,top,list);
  return PushArray(out,list);
}
