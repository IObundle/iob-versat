#include "versatSpecificationParser.hpp"

#include "accelerator.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "embeddedData.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "templateEngine.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "userConfigs.hpp"

#include "parser.hpp"

// ======================================
// Constants

static SpecExpression SPEC_LITERAL_0 = {.val = 0,.type = SpecType_LITERAL};

// NOTE: Spec expression is more associated to the equality operator when defining the graph
//       Math expression is everything else
//       One of the biggest differences is that a SpecExpression can contain delay statements "ex: x{0}", while a math expression cannot.

// TODO: We could join these into a single one if we can abstract the differences (which are not a lot).
SpecExpression* ParseSpecExpression(Parser* parser,Arena* out,int bindingPower = 99);
SpecExpression* ParseMathExpression(Parser* parser,Arena* out,int bindingPower = 99);

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

String GetActualArrayName(String baseName,Array<int> index,Arena* out){
  TEMP_REGION(temp,out);
  
  Array<String> asString = PushArray<String>(temp,index.size);
  for(int i = 0; i < index.size; i++){
    asString[i] = PushString(temp,"%d",index[i]);
  }
  String idPart = JoinStrings(asString,"_",temp);
  
  return PushString(out,"%.*s_%.*s",UN(baseName),UN(idPart));
}

FUDeclaration* InstantiateMerge(MergeDef def){
  TEMP_REGION(temp,nullptr);
  
  int size = def.declarations.size;
  
  Array<FUDeclaration*> decl = PushArray<FUDeclaration*>(temp,size);
  for(int i = 0; i <  size; i++){
    TypeAndInstance tp = def.declarations[i];
    FUDeclaration* d = GetTypeByNameOrFail(tp.typeName.identifier);
    decl[i] = d;
  }

  String name = PushString(globalPermanent,def.name.identifier);

  bool error = false;
  MergeModifier modifier = MergeModifier_NONE;

  for(Token t : def.mergeModifiers){
    Opt<MergeModifier> parsed = META_mergeModifiers_ReverseMap(t.identifier);

    if(!parsed.has_value()){
      printf("Error, merge does not support option: %.*s\n",UN(t.identifier));
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
  Entity* ent = env->GetEntity(var.name);

  int expectedRanges = 1;
  if(ent->type == EntityType_FU_ARRAY){
    expectedRanges += 1; // arraySize
  }

  if(var.index.size != expectedRanges){
    env->ReportError(var.name,"Too many array subscriptions for this entity");
  }

  Range<SpecExpression*> range = var.index[var.index.size - 1];
  
  int indexCount = GetRangeCount(env,range);
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

GroupIterator IterateGroup(Env* env,VarGroup group){
  GroupIterator iter = {};
  iter.group = group;
  iter.env = env;
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

  Range<SpecExpression*> range = var.index[var.index.size - 1];
  res.arrayIndex = iter.env->CalculateConstantExpression(range.start);

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

// TODO: Merge this function with the RegisterSubUnit function. There is not purpose to having this be separated.
FUDeclaration* InstantiateModule(String content,ModuleDef def){
  Arena* perm = globalPermanent;
  TEMP_REGION(temp,perm);

  Accelerator* circuit = CreateAccelerator(def.name.identifier,AcceleratorPurpose_MODULE);

  FREE_ARENA(envArena);
  FREE_ARENA(envArena2);
  Env* env = StartEnvironment(envArena,envArena2);

  auto paramList = PushList<ParameterDef>(temp);
  for(ParameterDeclaration param : def.params){
    env->AddParam(param.name);

    ParameterDef* def = paramList->PushElem();
    
    def->name = param.name.identifier;
    def->defaultValue = env->SymbolicFromSpecExpression(param.defaultValue);
  }
  auto params = PushArray(temp,paramList);

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
  
  FUDeclaration* res = RegisterSubUnit(circuit,params,SubUnitOptions_BAREBONES);
  
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

  for(String error : env->errors){
    printf("%.*s\n",UN(error));
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
}

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

    // nocheckin: TODO: Check repetition
    return result;
    ///return Unique(result,out);
  } break;
  case ConstructType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  };
}

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
  }

  return nullptr;
}

// ======================================
// Hierarchical access

bool IsEntitySubType(EntityType type){
  bool res = false;

  res |= (type == EntityType_MEM_PORT);
  res |= (type == EntityType_CONFIG_WIRE);
  
  return res;
}

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
  String error = PushString(miscArena,"[%.*s] %.*s: '%.*s'",UN(circuit->name),UN(msg),UN(badToken.identifier));
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
  tok.identifier = inst->name;
  tok.type = TokenType_IDENTIFIER;
  Entity* ent = PushNewEntity(tok);
  ent->type = EntityType_FU;
  ent->instance = inst;

  return inst;
}


FUInstance* Env::CreateFUInstanceWithDeclaration(FUDeclaration* type,String name,InstanceDeclaration decl){
  FUInstance* inst = CreateFUInstance(circuit,type,name);
  
  for(auto pair : decl.parameters){
    SYM_Expr expr = SymbolicFromSpecExpression(pair.second);
    bool result = SetParameter(inst,pair.first,expr);

    if(!result){
      printf("Warning: Parameter %.*s for instance %.*s in module %.*s does not exist\n",UN(pair.first),UN(inst->name),UN(circuit->name));
    }
  }

  return inst;
}

FUInstance* Env::GetFUInstance(Token name,int arrayIndexIfArray){
  TEMP_REGION(temp,nullptr);

  FUInstance* res = nullptr;
  if(name.identifier == "out"){
    res = GetOutputInstance();
  } else {
    Entity* ent = GetEntity(name);

    // TODO: Error reporting if entity does not exist.

    String asStr = name.identifier;
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
  if(var.name.identifier == "out"){
    if(var.isArrayAccess){
      ReportError(var.name,"'out' special unit cannot have array subscriptions");
    }
    
    res = GetOutputInstance();
  } else {
    Entity* ent = GetEntity(var.name);
    
    String name = var.name.identifier;
    if(ent->type == EntityType_FU_ARRAY){
      Array<int> index = ConvertRangeToIndex(var.index,temp);
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
  if(name.identifier == "out"){
    ReportError(name,"Cannot have a variable with the reserved name out");
  }

  auto res = this->scopes[this->currentScope]->variable->GetOrAllocate(name.identifier);
  
  if(res.alreadyExisted){
    ReportError(name,"Entity already exists. Rename entity to resolve conflict");
  }

  return res.data;
}

void Env::CheckIfEntityExists(Token name){
  Entity* ent = GetEntity(name);
  
  // nocheckin: TODO: Currently not enabled since code is not properly
  //                  setting values needed to make this properly work
#if 0
  if(!ent){
    ReportError(name,"Entity does not exist");
    DEBUG_BREAK();
  }
#endif
}

Entity* Env::GetEntity(Token name){
  for(int i = this->currentScope; i >= 0; i--){
    Entity* ent = this->scopes[i]->variable->Get(name.identifier);

    if(ent){
      return ent;
    }
  }

  return nullptr;
}

Entity* Env::GetEntity(String name){
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
        if(ent->type == EntityType_FU_ARRAY){
          SYM_Expr expr = SymbolicFromSpecExpression(id->arrayExpr);
          
          SYM_EvaluateResult eval = SYM_ConstantEvaluate(expr);

          if(eval.Error()){
            ReportError({},"Not a constant expression");
          }
          
          int arrayIndex = eval.result;

          String asStr = ent->arrayBaseName;
          if(ent->type == EntityType_FU_ARRAY){
            asStr = GetActualArrayName(asStr,arrayIndex,temp);
          }

          FUInstance* res = table->GetOrElse(asStr,nullptr);
          
          // NOTE: We only support 1D arrays doing things this way.
          nextEnt = GetEntity(res->name);
        }

        nextEnt = ent;
      } break;
      case ConfigAccessType_FUNC_CALL:{
        Token funcName = ptr->functionName;
        FUDeclaration* decl = ent->instance->declaration;

        for(MergePartition part : decl->info.infos){
          for(ConfigFunction* func : part.userFunctions){
            if(func->individualName == funcName.identifier){
              Entity* userFunc = PushStruct<Entity>(out);

              userFunc->func = func;
              userFunc->type = EntityType_CONFIG_FUNCTION;

              if(nextEnt) ReportError({},"Name collision");
              nextEnt = userFunc;
            }
          }
        }
      } break;
      case ConfigAccessType_ACCESS:{
        String access = ptr->name.identifier;

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
        } break;
        case EntityType_NODE:{
          Assert(false); // We do not have nodes since Env is only used currently to store FUInstances and other parseed data stuff. Nothing concrete exists at this point in the code.
        } break;

        case EntityType_CONFIG_FUNCTION:
        case EntityType_FU_ARRAY:
        case EntityType_MEM_PORT:
        case EntityType_PARAM:
        case EntityType_CONFIG_WIRE:
        case EntityType_STATE_WIRE:
        case EntityType_VARIABLE_INPUT:
        case EntityType_VARIABLE_SPECIAL:{
          ReportError(ptr->name,"Cannot have access expression for entity of this type");
        } break;
      }
      } break;
    }

    if(nextEnt){
      nextEnt->parent = ent;
    }

    ent = nextEnt;
  }

  return ent;
}

Array<int> Env::CalculateArraySize(Array<SpecExpression*> exprs){
  if(exprs.size <= 0){
    Assert(false); // Not an error. Programmer cannot call this if empty (not an array)
  }

  Array<int> res = PushArray<int>(scopeArena,exprs.size);

  int arraySize = 1;
  for(int i = 0; i <  exprs.size; i++){
    SpecExpression* expr = exprs[i];
    res[i] = CalculateConstantExpression(expr);
  }

  return res;
}

int Env::CalculateConstantExpression(SpecExpression* top){
  TEMP_REGION(temp,nullptr);

  // TODO: Need to report error if we find a non constant value in here.
  //       And then return 0

  SYM_Expr expr = SymbolicFromSpecExpression(top);
  SYM_EvaluateResult eval = SYM_ConstantEvaluate(expr);

  //Assert(!eval.Error());

  return eval.result;
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
    
    String access = accessExpr->name.identifier;

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

Array<int> Env::ConvertRangeToIndex(Array<Range<SpecExpression*>> range,Arena* out){
  Array<int> res = PushArray<int>(out,range.size);

  for(int i = 0; i <  range.size; i++){
    Range<SpecExpression*> r = range[i];
    
    if(r.start != r.end){
      ReportError({},"Did not expect a range at this point");
    }

    res[i] = CalculateConstantExpression(r.start);
  }

  return res;
}

void Env::AddInput(VarDeclaration var){
  TEMP_REGION(temp,nullptr);

  Entity* ent = PushNewEntity(var.name);

  if(var.arrayDims.size){
    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = var.name.identifier;
    ent->arrayDims = CalculateArraySize(var.arrayDims);

    for(auto iter = StartIteration(ent->arrayDims,temp); iter->IsValid(); iter->Advance()){
      Array<int> index = iter->Current();

      String actualName = GetActualArrayName(var.name.identifier,index,temp);
      FUInstance* input = CreateOrGetInput(circuit,actualName,insertedInputs++);
      table->Insert(input->name,input);
    }
  } else {
    FUInstance* input = CreateOrGetInput(circuit,var.name.identifier,insertedInputs++);
    table->Insert(input->name,input);
    
    ent->type = EntityType_FU;
    ent->instance = input;
  }

  ent->isInput = true;
}

void Env::AddInstance(InstanceDeclaration decl,VarDeclaration var){
  TEMP_REGION(temp,nullptr);

  FUDeclaration* type = GetTypeByName(decl.typeName.identifier);
  
  if(!type){
    ReportError(decl.typeName,"Typename does not exist");
  }

  Entity* ent = PushNewEntity(var.name);

  if(var.arrayDims.size){
    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = var.name.identifier;
    ent->arrayDims = CalculateArraySize(var.arrayDims);

    for(auto iter = StartIteration(ent->arrayDims,temp); iter->IsValid(); iter->Advance()){
      Array<int> index = iter->Current();

      String actualName = GetActualArrayName(var.name.identifier,index,temp);
      FUInstance* input = CreateOrGetInput(circuit,actualName,insertedInputs++);
      table->Insert(input->name,input);
    }
  } else {
    FUInstance* inst = CreateFUInstanceWithDeclaration(type,var.name.identifier,decl);
    table->Insert(inst->name,inst);

    ent->type = EntityType_FU;
    ent->instance = inst;
  }

  for(auto iter = StartIteration(this,ent,temp); iter.IsValid(); iter = iter.Next()){
    FUInstance* inst = iter.Current();
    
    switch(decl.modifier){
    case InstanceDeclarationType_NONE: break;
    case InstanceDeclarationType_SHARE_CONFIG:{
      ShareInstanceConfig(inst,decl.shareIndex);
      
      for(Token partialShareName : decl.shareNames){
        bool foundOne = false;
        for(int ii = 0; ii < inst->declaration->configs.size; ii++){
          if(inst->declaration->configs[ii].name == partialShareName.identifier){
            inst->isSpecificConfigShared[ii] = false;
            foundOne = true;
          }
        }

        if(!foundOne){
          TEMP_REGION(temp,nullptr);
          String errorMsg = PushString(temp,"Cannot share config wire since it does not exist for instance '%.*s' of type '%.*s'",UN(inst->name),UN(decl.typeName.identifier));
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

  GroupIterator out = IterateGroup(this,decl.output);
  GroupIterator in  = IterateGroup(this,decl.input);

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
    auto res = this->scopes[this->currentScope]->variable->GetOrAllocate(outVar.name.identifier);
    Entity* ent = res.data;

    ent->type = EntityType_FU_ARRAY;
    ent->arrayBaseName = outVar.name.identifier;

    ent->arrayDims[0] = MAX(ent->arrayDims[0],CalculateConstantExpression(outVar.index[0].high));
  }

  String name = outVar.name.identifier;
  if(outVar.isArrayAccess){
    Array<int> index = ConvertRangeToIndex(outVar.index,temp);
    name = GetActualArrayName(name,index,temp);
  }

  FUInstance* inst = portSpecExpression.inst;
  String uniqueName = GetUniqueName(name,globalPermanent,table);
  inst->name = PushString(globalPermanent,uniqueName);

  Token tok = {};
  tok.type = TokenType_IDENTIFIER;
  tok.identifier = inst->name;
  Entity* ent = PushNewEntity(tok);
  ent->type = EntityType_FU;
  ent->instance = inst;

  table->Insert(inst->name,inst);
}

void Env::AddParam(Token name){
  Entity* ent = GetEntity(name);

  if(ent){
    ReportError(name,"This name is already being used");
  }

  Entity* entity = PushNewEntity(name);

  entity->type = EntityType_PARAM;
  entity->paramName = name.identifier;
}

void Env::AddVariable(Token name){
  Entity* ent = GetEntity(name);

  if(ent){
    ReportError(name,"This name is already being used");
  }

  Entity* entity = PushNewEntity(name);

  entity->type = EntityType_VARIABLE_INPUT;
  entity->varName = name.identifier;
}

FUInstanceIterator FUInstanceIterator::Next(){
  FUInstanceIterator next = *this;
  
  iter->Advance();
  return next;
}

bool FUInstanceIterator::IsValid(){
  return iter->IsValid();
}

FUInstance* FUInstanceIterator::Current(){
  Assert(IsValid());

  FUInstance* inst;
  if(!iter){
    return ent->instance;
  } else {
    TEMP_REGION(temp,nullptr);
    String baseName = ent->arrayBaseName;
    String actualName = GetActualArrayName(baseName,iter->Current(),temp);

    inst = GetUnit(this->env->circuit,actualName);
  }
  
  return inst;
}

FUInstanceIterator StartIteration(Env* env,Entity* ent,Arena* out){
  FUInstanceIterator iter = {};
  iter.env = env;
  iter.ent = ent;

  if(ent->type == EntityType_FU_ARRAY){
    iter.iter = StartIteration(ent->arrayDims,out);
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
    NOT_POSSIBLE();
#if 0
    Var var = root->var;  
    String name = var.name.identifier;

    if(var.isArrayAccess){
      name = GetActualArrayName(var.name.identifier,CalculateConstantExpression(var.index.bottom),globalPermanent);
    }
    
    FUInstance* inst = table->GetOrFail(name);

    res.inst = inst;
    res.extra = var.extra;
#endif
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

SYM_Expr Env::SymbolicFromSpecExpression(SpecExpression* spec){
  auto Recurse = [this](auto Recurse,SpecExpression* top) -> SYM_Expr{
    SYM_Expr res = SYM_Zero;

    switch(top->type){
    case SpecType_OPERATION:{
      if(top->expressions.size == 1){
        if(top->op[0] == '-' || top->op[0] == '~'){
          SYM_Expr left  = Recurse(Recurse,top->expressions[0]);
          res = -left;
        } else {
          NOT_IMPLEMENTED();
        }
      } else {
        SYM_Expr left  = Recurse(Recurse,top->expressions[0]);
        SYM_Expr right = Recurse(Recurse,top->expressions[1]);

        if(top->op[0] == '+'){
          res = left + right;
        }
        if(top->op[0] == '-'){
          res = left - right;
        }
        if(top->op[0] == '*'){
          res = left * right;
        }
        if(top->op[0] == '/'){
          res = left / right;
        }
      }
    } break;
    case SpecType_NAME:
    case SpecType_VAR:{
      CheckIfEntityExists(top->name);
      res = SYM_Var(top->name.identifier);
    } break;
    case SpecType_LITERAL:{
      res = SYM_Lit(top->val);
    } break;
    case SpecType_SINGLE_ACCESS:
    case SpecType_ARRAY_ACCESS:
    case SpecType_FUNCTION_CALL: Assert(false);
    }

    return res;
  };

  SYM_Expr res = Recurse(Recurse,spec);
  return res;
}

SpecExpression* ParseNumberOnly(Parser* parser,Arena* out){
  SpecExpression* res = PushStruct<SpecExpression>(out);

  res->val = parser->ExpectNext(TokenType_NUMBER).number;
  res->type = SpecType_LITERAL;

  return res;
}

Range<SpecExpression*> ParseRange(Parser* parser,Arena* out){
  Range<SpecExpression*> res = {};

  SpecExpression* n1 = ParseNumberOnly(parser,out);

  Assert(n1);

  res.start = n1;
  res.end = n1;

  if(parser->IfNextToken(TokenType_DOUBLE_DOT)){
    res.end = ParseNumberOnly(parser,out);
  }
  
  return res;
}

Var ParseVar(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  Token name = parser->ExpectNext(TokenType_IDENTIFIER);

  bool isArrayAccess = false;
  auto list = PushList<Range<SpecExpression*>>(temp); 
  while(parser->IfNextToken('[')){
    Range<SpecExpression*> range = ParseRange(parser,out);
    *list->PushElem() = range;

    //isArrayAccess = true;

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
  var.index = PushArray(out,list);
  var.extra.delay = {delayStart,delayEnd};
  var.extra.port = {portStart,portEnd};

  return var;
}

VarDeclaration ParseVarDeclaration(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  VarDeclaration res = {};

  res.name = parser->ExpectNext(TokenType_IDENTIFIER);
  
  // TODO: We should integrate the array parsing logic with this one
  auto list = PushList<SpecExpression*>(temp);

  while(parser->IfNextToken('[')){
    SpecExpression* arraySize = ParseMathExpression(parser,out);

    parser->ExpectNext(']');

    *list->PushElem() = arraySize;
  }

  res.arrayDims = PushArray(out,list);
  
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
    VarDeclaration var = ParseVarDeclaration(parser,out);
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
    Token potentialModifier = parser->PeekToken();

    InstanceDeclarationType parsedModifier = InstanceDeclarationType_NONE;
    if(potentialModifier.type == TokenType_KEYWORD_DEBUG){
      parser->NextToken();

      res.debug = true;
    } else if(potentialModifier.type == TokenType_KEYWORD_STATIC){
      parser->NextToken();
      parsedModifier = InstanceDeclarationType_STATIC;
    } else if(potentialModifier.type == TokenType_KEYWORD_SHARE){
      parser->NextToken();
      parser->ExpectNext('(');
      parser->ExpectNext(TokenType_KEYWORD_CONFIG);
      parser->ExpectNext(')');

      parsedModifier = InstanceDeclarationType_SHARE_CONFIG;
      
      res.typeName = parser->ExpectNext(TokenType_IDENTIFIER);

      if(parser->IfNextToken('(')){
        // TODO: For now, we assume that every wire specified inside the spec file is a negative (remove share).
        auto toShare = PushList<Token>(temp);
        while(!parser->Done()){
          Token name = parser->ExpectNext(TokenType_IDENTIFIER);

          *toShare->PushElem() = name;

          if(parser->IfNextToken(',')){
            continue;
          } else {
            break;
          }
        }
      
        parser->ExpectNext(')');

        res.shareNames = PushArray(out,toShare);
      }

      parser->ExpectNext('{');

      auto varList = PushList<VarDeclaration>(temp);
    
      while(!parser->Done()){
        Token peek = parser->PeekToken();

        if(peek.type == '}'){
          break;
        }

        *varList->PushElem() = ParseVarDeclaration(parser,out);
      
        parser->ExpectNext(';');
      }
      res.declarations = PushArray(out,varList);
    
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

  res.typeName = parser->ExpectNext(TokenType_IDENTIFIER);

  Token possibleParameters = parser->PeekToken();
  auto list = PushList<Pair<String,SpecExpression*>>(temp);
  if(possibleParameters.type == '#'){
    parser->NextToken();
    parser->ExpectNext('(');

    while(!parser->Done()){
      parser->ExpectNext('.');
      Token parameterName = parser->ExpectNext(TokenType_IDENTIFIER);

      parser->ExpectNext('(');

      SpecExpression* expr = ParseSpecExpression(parser,out);

      parser->ExpectNext(')');
      
      String savedParameter = PushString(out,parameterName.identifier);
      *list->PushElem() = {savedParameter,expr}; 

      if(parser->IfNextToken(',')){
        continue;
      }

      break;
    }
    parser->ExpectNext(')');

    res.parameters = PushArray(out,list);
  }

  VarDeclaration varDecl = ParseVarDeclaration(parser,out);

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

      Token sepOrEnd = parser->NextToken();
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
  Token atom = parser->PeekToken();
  if(atom.type == '('){
    parser->ExpectNext('(');

    res = ParseMathExpression(parser,out);

    parser->ExpectNext(')');
  } else if(atom.type == TokenType_NUMBER){
    Token number = parser->ExpectNext(TokenType_NUMBER);
    res = PushStruct<SpecExpression>(out);

    res->type = SpecType_LITERAL;
    res->val = number.number;
  } else if(atom.type == TokenType_IDENTIFIER){
    TEMP_REGION(temp,out);

    Token name = parser->ExpectNext(TokenType_IDENTIFIER);

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
      Token singleAccessName = parser->ExpectNext(TokenType_IDENTIFIER);
      
      SpecExpression* singleAccess = PushStruct<SpecExpression>(out);
      singleAccess->type = SpecType_NAME;
      singleAccess->name = singleAccessName;
      
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
    TokenType type;
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
    Token peek = parser->PeekToken();

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
  Token atom = parser->PeekToken();
  if(atom.type == '('){
    parser->ExpectNext('(');

    res = ParseSpecExpression(parser,out);

    parser->ExpectNext(')');
  } else if(atom.type == TokenType_NUMBER){
    Token number = parser->ExpectNext(TokenType_NUMBER);
    res = PushStruct<SpecExpression>(out);

    res->type = SpecType_LITERAL;
    res->val = number.number;
  } else if(atom.type == TokenType_IDENTIFIER){
    TEMP_REGION(temp,out);

    Var var = ParseVar(parser,out);
    
    res = PushStruct<SpecExpression>(out);
    res->var = var;
    res->type = SpecType_VAR;
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
    TokenType type;
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

  infos[3] = {TokenType_ROTATE_LEFT,1,"><<"};
  infos[4] = {TokenType_ROTATE_RIGHT,1,">><"};
  infos[5] = {TokenType_SHIFT_LEFT,1,"<<"};
  infos[6] = {TokenType_SHIFT_RIGHT,1,">>"};

  infos[7]  = {TOK_TYPE('*'),2,"*"};
  infos[8] = {TOK_TYPE('/'),2,"/"};

  infos[9] = {TOK_TYPE('+'),3,"+"};
  infos[10] = {TOK_TYPE('-'),3,"-"};
  
  // Parse binary ops.
  while(!parser->Done()){
    Token peek = parser->PeekToken();

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
  } else if(parser->IfNextToken(TokenType_XOR_EQUAL)){
    // TODO: We parse it but we do not use it. We probably wanna remove the testcase that uses this.
    type = ConnectionType_EQUALITY;
  } else if(parser->IfNextToken(TokenType_ARROW)){
    type = ConnectionType_CONNECTION;
  } else {
    parser->ReportUnexpectedToken(parser->NextToken(),{TOK_TYPE('='),TokenType_ARROW});
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

ParameterDeclaration ParseParameterDeclaration(Parser* parser,Arena* out){
  ParameterDeclaration res = {};

  res.name = parser->ExpectNext(TokenType_IDENTIFIER);
  
  if(parser->IfNextToken('=')){
    res.defaultValue = ParseMathExpression(parser,out);
  }

  return res;
}

// Any returned String points to tokenizer content.
// As long as tokenizer is valid, strings returned by this function are also valid.
ModuleDef ParseModuleDef(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  parser->ExpectNext(TokenType_KEYWORD_MODULE);

  Token name = parser->ExpectNext(TokenType_IDENTIFIER);

  Array<ParameterDeclaration> params = {};
  if(parser->IfNextToken('#')){
    parser->ExpectNext('(');

    auto paramList = PushList<ParameterDeclaration>(temp);
    
    while(!parser->Done()){
      ParameterDeclaration param = ParseParameterDeclaration(parser,out);
      *paramList->PushElem() = param;
    
      if(parser->IfNextToken(',')){
        continue;
      } else {
        break;
      }
    }

    parser->ExpectNext(')');

    params = PushArray(out,paramList);
  }

  Array<VarDeclaration> vars = ParseModuleInputDeclaration(parser,out);

  //Token outputs = {};
  if(parser->IfNextToken(TokenType_ARROW)){
    // TODO: Right now we do not care about output info being given by the user.
    //       We probably should. We still parse it, we just ignore it for now.
    /* outputs = */ parser->ExpectNext(TokenType_NUMBER);
  }
  
  ArenaList<InstanceDeclaration>* decls = PushList<InstanceDeclaration>(temp);
  parser->ExpectNext('{');
  
  while(!parser->Done()){
    Token peek = parser->PeekToken();

    if(peek.type == ';'){
      parser->NextToken();
      continue;
    }

    if(peek.type == '#' || peek.type == TokenType_DOUBLE_HASHTAG || peek.type == '}'){
      break;
    }
    
    InstanceDeclaration decl = ParseInstanceDeclaration(parser,out);
    *decls->PushElem() = decl;
  }
  Array<InstanceDeclaration> declarations = PushArray(out,decls);

  ArenaList<ConnectionDef>* cons = PushList<ConnectionDef>(temp);
  if(parser->IfNextToken('#')){
    while(!parser->Done()){
      Token peek = parser->PeekToken();

      if(peek.type == ';'){
        parser->NextToken();
        continue;
      }

      if(peek.type == '}' || peek.type == TokenType_DOUBLE_HASHTAG){
        break;
      }

      ConnectionDef con = ParseConnection(parser,out);
 
      *cons->PushElem() = con;
    }
  }

  auto configFunctions = PushList<ConfigFunctionDef>(temp);

  // nocheckin
  if(parser->IfNextToken(TokenType_DOUBLE_HASHTAG)){
    while(!parser->Done()){
      // nocheckin TODO: Probably remove this and move the logic from the function to here
      
      bool isConfigFunctionStart = false;
      
      isConfigFunctionStart |= parser->IfPeekToken(TokenType_KEYWORD_CONFIG);
      isConfigFunctionStart |= parser->IfPeekToken(TokenType_KEYWORD_STATE);
      isConfigFunctionStart |= parser->IfPeekToken(TokenType_KEYWORD_MEM);

      if(isConfigFunctionStart){
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
  def.params = params;
  def.inputs = vars;
  def.declarations = declarations;
  def.connections = PushArray(out,cons);
  def.configs = PushArray(out,configFunctions);
  
  return def;
}

TypeAndInstance ParseTypeAndInstance(Parser* parser){
  Token typeName = parser->ExpectNext(TokenType_IDENTIFIER);

  Token instanceName = {};
  if(parser->IfNextToken(':')){
    instanceName = parser->ExpectNext(TokenType_IDENTIFIER);
  }

  TypeAndInstance res = {};
  res.typeName = typeName;
  res.instanceName = instanceName;
  
  return res;
}

HierarchicalName ParseHierarchicalName(Parser* parser,Arena* out){
  Token topInstance = parser->ExpectNext(TokenType_IDENTIFIER);

  parser->ExpectNext('.');

  Var var = ParseVar(parser,out);

  HierarchicalName res = {};
  res.instanceName = topInstance;
  res.subInstance = var;

  return res;
}

MergeDef ParseMerge(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  parser->ExpectNext(TokenType_KEYWORD_MERGE);

  Array<Token> mergeModifiers = {};
  if(parser->IfNextToken('(')){
    auto tokenList = PushList<Token>(temp);
    
    while(!parser->Done()){
      Token peek = parser->PeekToken();
      
      if(peek.type == ')'){
        break;
      }

      *tokenList->PushElem() = parser->ExpectNext(TokenType_IDENTIFIER);

      parser->IfNextToken(',');
    }

    mergeModifiers = PushArray(out,tokenList);
    
    parser->ExpectNext(')');
  }

  Token mergeName = parser->ExpectNext(TokenType_IDENTIFIER);
  
  parser->ExpectNext('=');

  ArenaList<TypeAndInstance>* declarationList = PushList<TypeAndInstance>(temp);
  while(!parser->Done()){
    TypeAndInstance typeInst = ParseTypeAndInstance(parser);

    *declarationList->PushElem() = typeInst;

    Token peek = parser->PeekToken();
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
      Token peek = parser->PeekToken();
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
  
  auto specificsNodes = PushList<SpecificMergeNode>(temp);
  for(SpecNode node : specNodes){
    // TODO: We do not do this in here. Parser only parses, semantic checks are performed later.
    int firstIndex = -1;
    int secondIndex = -1;
    for(int i = 0; i < declarations.size; i++){
      TypeAndInstance& decl = declarations[i];
      if(CompareString(node.first.instanceName.identifier,decl.instanceName.identifier)){
        firstIndex = i;
      } 
      if(CompareString(node.second.instanceName.identifier,decl.instanceName.identifier)){
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

    *specificsNodes->PushElem() = {firstIndex,node.first.subInstance.name.identifier,secondIndex,node.second.subInstance.name.identifier};
  }
  Array<SpecificMergeNode> specifics = PushArray(out,specificsNodes);

  MergeDef result = {};
  result.name = mergeName;
  result.declarations = declarations;
  result.specifics = specifics;
  result.mergeModifiers = mergeModifiers;
  
  return result;
}

#include "filesystem.hpp"

Array<ConstructDef> ParseVersatSpecification(String content,Arena* out){
  TEMP_REGION(temp,out);

  auto TokenizeFunction = [](void* tokenizerState) -> Token{
    DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
    
    const char* start = state->ptr;
    const char* end = state->end;

    if(start >= end){
      Token token = {};
      token.type = TokenType_EOF;
      return token;
    }

    TokenizeResult res = ParseWhitespace(start,end);
    res |= ParseComments(start,end);

    res |= ParseMultiSymbol(start,end,">><",TokenType_ROTATE_RIGHT);
    res |= ParseMultiSymbol(start,end,"><<",TokenType_ROTATE_LEFT);

    res |= ParseMultiSymbol(start,end,"..",TokenType_DOUBLE_DOT);
    res |= ParseMultiSymbol(start,end,"##",TokenType_DOUBLE_HASHTAG);
    res |= ParseMultiSymbol(start,end,"->",TokenType_ARROW);
    res |= ParseMultiSymbol(start,end,">>",TokenType_SHIFT_RIGHT);
    res |= ParseMultiSymbol(start,end,"<<",TokenType_SHIFT_LEFT);
    res |= ParseMultiSymbol(start,end,"^=",TokenType_XOR_EQUAL);

    res |= ParseSymbols(start,end);
    res |= ParseNumber(start,end);

    res |= ParseIdentifier(start,end);

    if(res.token.type == TokenType_IDENTIFIER){
      String id = res.token.identifier;
      
      TokenType type = TokenType_INVALID;

      // TODO: We really need a fast way of checking this using size + character by character branching path.
      //       However this is something that we want to push to the meta function generation. We do not want to actually write this and potentially get it wrong.
      if(id == "module")     type = TokenType_KEYWORD_MODULE;
      if(id == "merge")      type = TokenType_KEYWORD_MERGE;
      if(id == "share")      type = TokenType_KEYWORD_SHARE;
      if(id == "static")     type = TokenType_KEYWORD_STATIC;
      if(id == "debug")      type = TokenType_KEYWORD_DEBUG;
      if(id == "config")     type = TokenType_KEYWORD_CONFIG;
      if(id == "state")      type = TokenType_KEYWORD_STATE;
      if(id == "mem")        type = TokenType_KEYWORD_MEM;
      if(id == "for")        type = TokenType_KEYWORD_FOR;

      if(type != TokenType_INVALID){
        res.token.type = type;
      }
    }

    int size = res.bytesParsed;
    if(size <= 0 && state->ptr != state->end){
      size = 1;
    }

    state->ptr += size;

    // NOTE: Something very bad must happen to the point where the file is 1 byte after the end.
    //       We expect it to only reach file->end, not file->end + 1
    Assert(state->ptr < state->end + 1);

    Token ret = res.token;
    ret.originalFile = state->content;

    return ret;
  };

  FREE_ARENA(parseArena);
  Parser* parser = StartParsing(TokenizeFunction,content,parseArena,ParsingOptions_DEFAULT);

  // TODO:
  // nocheckin: Kinda hacky way of doing this.
  //            We cannot put filesystem stuff on the parser since parser is common.
  DefaultTokenizerState* state = (DefaultTokenizerState*) parser->tokenizerState;
  FileContent contentAsFile = FILE_GetFileContentFromString(content);
  state->content = contentAsFile;

  ArenaList<ConstructDef>* typeList = PushList<ConstructDef>(temp);

  while(!parser->Done()){
    Token tok = parser->PeekToken();

    ConstructDef def = {};
    if(tok.type == TokenType_KEYWORD_MODULE){
      def.type = ConstructType_MODULE;
      def.module = ParseModuleDef(parser,out);
    } else if(tok.type == TokenType_KEYWORD_MERGE){
      def.type = ConstructType_MERGE;
      def.merge = ParseMerge(parser,out);
    } else {
      parser->ReportUnexpectedToken(tok,{TokenType_KEYWORD_MODULE,TokenType_KEYWORD_MERGE});
      parser->Synch({TokenType_KEYWORD_MODULE,TokenType_KEYWORD_MERGE});
    }

    *typeList->PushElem() = def;
  }

  if(!Empty(parser->errors)){
    for(String str : parser->errors){
      printf("%.*s\n",UN(str));
    }
    exit(-1);
  }

  Array<ConstructDef> defs = PushArray(out,typeList);

  return defs;
}

static ConfigIdentifier* ParseConfigIdentifier(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);
  
  Token id = parser->ExpectNext(TokenType_IDENTIFIER);
  
  ConfigIdentifier* base = PushStruct<ConfigIdentifier>(out);
  base->type = ConfigAccessType_BASE;
  base->name = id;

  ConfigIdentifier* ptr = base;

  while(!parser->Done()){
    ConfigIdentifier* parsed = nullptr;

    if(!parsed && parser->IfNextToken('.')){
      Token access = parser->ExpectNext(TokenType_IDENTIFIER);

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

        parsed = PushStruct<ConfigIdentifier>(out);
        parsed->type = ConfigAccessType_FUNC_CALL;
        parsed->functionName = access;
        parsed->arguments = PushArray(out,args);
      } else {
        parsed = PushStruct<ConfigIdentifier>(out);
        parsed->type = ConfigAccessType_ACCESS;
        parsed->name = access;
      }
    }

    if(!parsed && parser->IfNextToken('[')){
      SpecExpression* expr = ParseMathExpression(parser,out);

      parser->ExpectNext(']');

      parsed = PushStruct<ConfigIdentifier>(out);
      parsed->type = ConfigAccessType_ARRAY;
      parsed->arrayExpr = expr;
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

  if(parser->IfNextToken(TokenType_KEYWORD_FOR)){
    Token loopVariable = parser->ExpectNext(TokenType_IDENTIFIER);
 
    SpecExpression* start = ParseMathExpression(parser,out);
    parser->ExpectNext(TokenType_DOUBLE_DOT);
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
    
    stmt->def.loopVariable = loopVariable.identifier;
    stmt->def.startSym = start;
    stmt->def.endSym = end;
    stmt->childs = PushArray(out,list);
    stmt->type = ConfigStatementType_FOR_LOOP;
  } else if(parser->IfPeekToken(TokenType_IDENTIFIER)) {
    stmt->lhs = ParseConfigIdentifier(parser,out);

    if(parser->IfNextToken('=')){
      stmt->rhs = ParseMathExpression(parser,out);
      parser->ExpectNext(';');
      stmt->type = ConfigStatementType_EQUALITY;
    } else {
      parser->ExpectNext(';');
      stmt->type = ConfigStatementType_FUNCTION_CALL;
    }
  } else {
    parser->ReportUnexpectedToken(parser->NextToken(),{});
    // TODO: stmt = EmptyStmt (Return something so that code does not have to worry about null statements).
  }

  return stmt;
}

static ConfigVarDeclaration ParseConfigVarDeclaration(Parser* parser){
  ConfigVarDeclaration res = {};

  res.name = parser->ExpectNext(TokenType_IDENTIFIER);

  if(parser->IfNextToken('[')){
    Token number = parser->ExpectNext(TokenType_NUMBER);
    int arraySize = number.number;

    parser->ExpectNext(']');

    res.arraySize = arraySize;
    res.isArray = true;
  }

  Token type = {};
  if(parser->IfNextToken(':')){
    type = parser->NextToken();
  }

  res.type = type;
  
  return res;
}

static Array<ConfigVarDeclaration> ParseConfigFunctionArguments(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<ConfigVarDeclaration>(temp);

  parser->ExpectNext('(');
  
  while(!parser->Done()){
    if(parser->IfPeekToken(')')){
      break;
    }

    ConfigVarDeclaration var = ParseConfigVarDeclaration(parser);

    *list->PushElem() = var;
    
    if(parser->IfNextToken(',')){
      continue;
    } else {
      break;
    }
  }

  parser->ExpectNext(')');

  return PushArray(out,list);
}

ConfigFunctionDef* ParseConfigFunction(Parser* parser,Arena* out){
  TEMP_REGION(temp,out);

  UserConfigType type = UserConfigType_NONE;
  if(parser->IfNextToken(TokenType_KEYWORD_CONFIG)){
    type = UserConfigType_CONFIG;
  } else if(parser->IfNextToken(TokenType_KEYWORD_MEM)){
    type = UserConfigType_MEM;
  } else if(parser->IfNextToken(TokenType_KEYWORD_STATE)){
    type = UserConfigType_STATE;
  }

  if(type == UserConfigType_NONE){
    return nullptr;
  }

  bool debug = parser->IfNextToken(TokenType_KEYWORD_DEBUG);
    
  Token configName = parser->ExpectNext(TokenType_IDENTIFIER);

  Array<ConfigVarDeclaration> functionVars = {};
  if(type == UserConfigType_MEM || type == UserConfigType_CONFIG){
    functionVars = ParseConfigFunctionArguments(parser,out);
  }

  parser->ExpectNext('{');

  auto stmts = PushList<ConfigStatement*>(temp);
  while(!parser->Done()){
    Token peek = parser->PeekToken();

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

DimIterator* StartIteration(Array<int> dimensions,Arena* out){
  DimIterator* res = PushStruct<DimIterator>(out);

  res->dim = CopyArray(dimensions,out);
  res->current = PushArray<int>(out,dimensions.size);

  return res;
}

// MARK: Move to a better place
void DimIterator::Advance(){
  for(int i = current.size - 1; i >= 0; i--){
    current[i] += 1;

    if(i != 0 && current[i] >= dim[i]){
      current[i] = 0;
      continue;
    }

    break;
  }
}

bool DimIterator::IsValid(){
  if(current[0] >= dim[0]){
    return false;
  }
 
  return true;
}

Array<int> DimIterator::Current(){
  return current;
}
