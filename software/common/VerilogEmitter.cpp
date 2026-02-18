#include "VerilogEmitter.hpp"

// ============================================================================
// Verilog module interface manipulation

void VerilogModuleBuilder::StartGroup(String name){
  Assert(currentPorts == nullptr);

  if(!Empty(name)){
    currentPortGroupName = PushString(arena,name);
  }
  currentPorts = PushList<VerilogPortSpec>(arena);
}

void VerilogModuleBuilder::EndGroup(bool preserveEmptyGroup){
  if(preserveEmptyGroup || Size(this->currentPorts) > 0){
    VerilogPortGroup* group = this->groups->PushElem();

    group->name = this->currentPortGroupName;
    group->ports = PushArray(arena,this->currentPorts);
  }
    
  this->currentPortGroupName = {};
  this->currentPorts = nullptr;
}

void VerilogModuleBuilder::AddPort(String name,SymbolicExpression* expr,WireDir dir,SpecialPortProperties props){
  bool endGroup = false;
  if(!this->currentPorts){
    StartGroup(nullptr);
    endGroup = true;
  }
  
  VerilogPortSpec* spec = this->currentPorts->PushElem();

  spec->name = PushString(arena,name);
  spec->size = expr;
  spec->dir = dir;
  spec->props = props;

  if(endGroup){
    EndGroup();
  }
}

void VerilogModuleBuilder::AddPortIndexed(const char* name,int index,SymbolicExpression* expr,WireDir dir,SpecialPortProperties props){
  bool endGroup = false;
  if(!this->currentPorts){
    StartGroup(nullptr);
    endGroup = true;
  }

  VerilogPortSpec* spec = this->currentPorts->PushElem();

  spec->name = PushString(arena,name,index);
  spec->size = expr;
  spec->dir = dir;
  spec->props = props;

  if(endGroup){
    EndGroup();
  }
}

void VerilogModuleBuilder::AddInterface(Array<VerilogPortSpec> interface,String prefix){
  for(VerilogPortSpec spec : interface){
    char* name = CS(spec.name);
    if(!Empty(prefix)){
      name = SF("%.*s_%.*s",UN(prefix),UN(spec.name));
    }
    
    AddPort(name,spec.size,spec.dir,spec.props);
  }
}

void VerilogModuleBuilder::AddInterfaceIndexed(Array<VerilogPortSpec> interfaceFormat,int index,String prefix){
  for(VerilogPortSpec spec : interfaceFormat){
    char* name = CS(spec.name);
    if(!Empty(prefix)){
      name = SF("%.*s_%.*s",UN(prefix),UN(spec.name));
    }

    AddPortIndexed(name,index,spec.size,spec.dir,spec.props);
  }
}

VerilogModuleBuilder* StartVerilogModuleInterface(Arena* arena){
  VerilogModuleBuilder* builder = PushStruct<VerilogModuleBuilder>(arena);
  builder->arena = arena;
  builder->groups = PushList<VerilogPortGroup>(arena);
  
  return builder;
}

VerilogPortSpec Copy(VerilogPortSpec in,Arena* out){
  VerilogPortSpec res = in;
  res.name = PushString(out,in.name);
  res.size = SymbolicDeepCopy(in.size,out);
  return res;
}

VerilogPortGroup Copy(VerilogPortGroup in,Arena* out){
  VerilogPortGroup res = {};
  res.name = PushString(out,in.name);
  res.ports = PushArray<VerilogPortSpec>(out,in.ports.size);
  for(int i = 0; i < in.ports.size; i++){
    res.ports[i] = Copy(in.ports[i],out);
  }
  return res;
}

VerilogModuleInterface* Copy(VerilogModuleInterface* interface,Arena* out){
  VerilogModuleInterface* res = PushStruct<VerilogModuleInterface>(out);
  
  res->portGroups = PushArray<VerilogPortGroup>(out,interface->portGroups.size);
  for(int i = 0; i < interface->portGroups.size; i++){
    res->portGroups[i] = Copy(interface->portGroups[i],out);
  }

  return res;
}

VerilogModuleInterface* End(VerilogModuleBuilder* builder,Arena* out){
  VerilogModuleInterface* res = PushStruct<VerilogModuleInterface>(out);

  res->portGroups = PushArray<VerilogPortGroup>(out,Size(builder->groups));
  int index = 0;
  for(VerilogPortGroup group : builder->groups){
    res->portGroups[index++] = Copy(group,out);
  }

  return res;
}

// ======================================
// Verilog interface manipulation

Array<VerilogPortSpec>  AppendSuffix(Array<VerilogPortSpec> in,String suffix,Arena* out){
  Array<VerilogPortSpec> res = PushArray<VerilogPortSpec>(out,in.size);
  for(int i = 0; i < in.size; i++){
    res[i] = in[i];

    res[i].size = SymbolicDeepCopy(in[i].size,out);
    res[i].name = PushString(out,"%.*s%.*s",UN(in[i].name),UN(suffix));
  }

  return res;
}

Array<VerilogPortSpec>  ReverseInterfaceDirection(Array<VerilogPortSpec> in,Arena* out){
  Array<VerilogPortSpec> res = PushArray<VerilogPortSpec>(out,in.size);
  for(int i = 0; i < in.size; i++){
    res[i] = Copy(in[i],out);

    res[i].dir = ReverseDir(in[i].dir);
  }

  return res;
}

Array<VerilogPortSpec> AddDirectionToName(Array<VerilogPortSpec> in,Arena* out){
  Array<VerilogPortSpec> res = PushArray<VerilogPortSpec>(out,in.size);
  for(int i = 0; i < in.size; i++){
    res[i] = in[i];

    String directionAsName = "i";
    if(in[i].dir == WireDir_OUTPUT){
      directionAsName = "o";
    }
    
    res[i].size = SymbolicDeepCopy(in[i].size,out);
    res[i].name = PushString(out,"%.*s_%.*s",UN(in[i].name),UN(directionAsName));
  }

  return res;
}

Array<VerilogPortSpec> ExtractAllPorts(VerilogModuleInterface* interface,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<VerilogPortSpec>(temp);
  for(VerilogPortGroup group : interface->portGroups){
    for(VerilogPortSpec spec : group.ports){
      *list->PushElem() = spec;
    }
  }

  Array<VerilogPortSpec> res = PushArray(out,list);
  return res;
}

Array<VerilogPortSpec> ObtainGroupByName(VerilogModuleInterface* interface,String name){
  for(VerilogPortGroup group : interface->portGroups){
    if(CompareString(group.name,name)){
      return group.ports;
    }
  }

  return {};
}

bool RemoveGroupInPlace(VerilogModuleInterface* interface,String name){
  for(int i = 0; i < interface->portGroups.size; i++){
    if(CompareString(interface->portGroups[i].name,name)){
      interface->portGroups = RemoveElement(interface->portGroups,i);
      return true;
    }
  }

  return false;
}

Opt<VerilogPortSpec> GetPortSpecByName(Array<VerilogPortSpec> array,String name){
  for(VerilogPortSpec spec : array){
    if(CompareString(spec.name,name)){
      return spec;
    }
  }
  return {};
}

bool ContainsGroup(VerilogModuleInterface* interface,String name){
  for(VerilogPortGroup group : interface->portGroups){
    if(CompareString(name,group.name)){
      return true;
    }
  }

  return false;
}

// ============================================================================
// Emitter

String Repr(VASTType type){
  return {};
}

void VEmitter::PushLevel(VAST* level){
  buffer[top++] = level;
}

void VEmitter::PopLevel(){
  top -= 1;
  Assert(top >= 0);
}

void VEmitter::InsertDeclaration(VAST* declarationAST){
  if(top == 0){
    *topLevel->top.declarations->PushElem() = declarationAST;
    return;
  }

  for(int i = top - 1; i >= 0; i--){
    VAST* cast = buffer[i];
    VASTType type = cast->type;

    FULL_SWITCH(type){
    case VASTType_VAR_DECL:
    case VASTType_INTERFACE_DECL:
    case VASTType_INTEGER_DECL:
    case VASTType_LOCAL_PARAM:
    case VASTType_SIMPLE_OP:
    case VASTType_SET:
    case VASTType_FORCED_SET:
    case VASTType_EXPR:
    case VASTType_PORT_CONNECT:
    case VASTType_ASSIGN_DECL:
    case VASTType_INSTANCE:;
    case VASTType_BLANK:
    case VASTType_PORT_GROUP:
    case VASTType_COMMENT:
    case VASTType_RAW_STATEMENT:
    case VASTType_PORT_DECL: continue;

    case VASTType_WIRE_ASSIGN_BLOCK:{
      *cast->wireAssignBlock.expressions->PushElem() = declarationAST;
      return;
    } break;

    case VASTType_IF:{
      if(cast->ifExpr.elseStatements){
        *cast->ifExpr.elseStatements->PushElem() = declarationAST;
      } else {
        *cast->ifExpr.ifExpressions->tail->elem.statements->PushElem() = declarationAST;
      }

      //*cast->ifExpr.declarations->PushElem() = declarationAST;
      return;
    } break;

    case VASTType_LOOP:{
      *cast->loopExpr.statements->PushElem() = declarationAST;
      return;
    } break;
      
    case VASTType_ALWAYS_BLOCK: {
      *cast->alwaysBlock.declarations->PushElem() = declarationAST;
      return;
    } break;

    case VASTType_INITIAL_BLOCK: {
      *cast->initialBlock.declarations->PushElem() = declarationAST;
      return; 
    } break;

    case VASTType_TASK_DECL:{
      *cast->task.declarations->PushElem() = declarationAST;
      return; 
    } break;
      
    case VASTType_MODULE_DECL: {
      *cast->module.declarations->PushElem() = declarationAST;
      return; 
    } break;
    case VASTType_TOP_LEVEL: {
      Assert(false);
    } break;
    } END_SWITCH();
  }
}

void VEmitter::InsertPortDeclaration(VAST* portAST){
  if(top == 0){
    *topLevel->top.declarations->PushElem() = portAST;
    return;
  }

  for(int i = top - 1; i >= 0; i--){
    VAST* cast = buffer[i];
    VASTType type = cast->type;

    FULL_SWITCH(type){
    case VASTType_VAR_DECL:
    case VASTType_INTEGER_DECL:
    case VASTType_SIMPLE_OP:
    case VASTType_SET:
    case VASTType_FORCED_SET:
    case VASTType_LOCAL_PARAM:
    case VASTType_INTERFACE_DECL:
    case VASTType_EXPR:
    case VASTType_ASSIGN_DECL:
    case VASTType_INSTANCE:;
    case VASTType_PORT_CONNECT:
    case VASTType_BLANK:
    case VASTType_ALWAYS_BLOCK:
    case VASTType_INITIAL_BLOCK:
    case VASTType_IF:
    case VASTType_LOOP:
    case VASTType_COMMENT:
    case VASTType_RAW_STATEMENT:
    case VASTType_WIRE_ASSIGN_BLOCK:
    case VASTType_PORT_DECL: continue;

    case VASTType_PORT_GROUP: {
      *cast->portGroup.portDeclarations->PushElem() = portAST;
      return;
    } break;

    case VASTType_TASK_DECL:{
      *cast->task.portDeclarations->PushElem() = portAST;
      return; 
    } break;
      
    case VASTType_MODULE_DECL: {
      *cast->module.portDeclarations->PushElem() = portAST;
      return; 
    } break;
    case VASTType_TOP_LEVEL: {
      Assert(false);
    } break;
    } END_SWITCH();
  }
};

void VEmitter::InsertPortConnect(VAST* portAST){
  if(top == 0){
    *topLevel->top.portConnections->PushElem() = portAST;
    return;
  }

  for(int i = top - 1; i >= 0; i--){
    VAST* cast = buffer[i];
    VASTType type = cast->type;

    FULL_SWITCH(type){
    case VASTType_VAR_DECL:
    case VASTType_INTEGER_DECL:
    case VASTType_INTERFACE_DECL:
    case VASTType_SIMPLE_OP:
    case VASTType_LOCAL_PARAM:
    case VASTType_SET:
    case VASTType_FORCED_SET:
    case VASTType_EXPR:
    case VASTType_ASSIGN_DECL:
    case VASTType_BLANK:
    case VASTType_ALWAYS_BLOCK:
    case VASTType_PORT_CONNECT:
    case VASTType_IF:
    case VASTType_INITIAL_BLOCK:
    case VASTType_LOOP:
    case VASTType_PORT_GROUP:
    case VASTType_RAW_STATEMENT:
    case VASTType_WIRE_ASSIGN_BLOCK:
    case VASTType_COMMENT:
    case VASTType_PORT_DECL: continue;

    case VASTType_INSTANCE:{
      *cast->instance.portConnections->PushElem() = portAST;
      return;
    } break;

    case VASTType_TASK_DECL:{
      Assert(false);
      return; 
    } break;
      
    case VASTType_MODULE_DECL: {
      Assert(false);
      return; 
    } break;
    case VASTType_TOP_LEVEL: {
      Assert(false);
    } break;
    } END_SWITCH();
  }
};

VAST* VEmitter::FindFirstVASTType(VASTType type,bool errorIfNotFound){
  if(type == VASTType_TOP_LEVEL){
    return topLevel;
  }
    
  for(i8 i = top - 1; i >= 0; i--){
    if(buffer[i]->type == type){
      return buffer[i];
    }
  }

  if(errorIfNotFound){
    printf("Did not find VAST of type: %.*s\n",UN(Repr(type)));
    printf("Emitter stack currently has %d levels and is, from zero:\n",top);
    for(int i = 0; i < top; i++){
      printf("%.*s\n",UN(Repr(buffer[i]->type)));
    }
    DEBUG_BREAK_OR_EXIT();
  }
  
  return nullptr;
}

void VEmitter::Timescale(String timeUnit,String timePrecision){
  topLevel->top.timescaleExpression = PushString(arena,"%.*s / %.*s",UN(timeUnit),UN(timePrecision));
}

void VEmitter::Include(String name){
  *topLevel->top.includes->PushElem() = PushString(arena,"%.*s",UN(name));
}

void VEmitter::Module(String name){
  VAST* decl = PushVAST(VASTType_MODULE_DECL,arena);

  decl->module.name = PushString(arena,name);
  decl->module.parameters = PushList<Pair<String,String>>(arena);
  decl->module.portDeclarations = PushList<VAST*>(arena);
  decl->module.declarations = PushList<VAST*>(arena);
  
  InsertDeclaration(decl);
  PushLevel(decl);
}

void VEmitter::ModuleParam(String name,int value){
  VAST* decl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  Pair<String,String>* p = decl->module.parameters->PushElem();
  p->first = PushString(arena,"%.*s",UN(name));
  p->second = PushString(arena,"%d",value);
}

void VEmitter::ModuleParam(String name,String value){
  VAST* decl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  Pair<String,String>* p = decl->module.parameters->PushElem();
  p->first = PushString(arena,"%.*s",UN(name));
  p->second = PushString(arena,value);
}

void VEmitter::EndModule(){
  while(buffer[top-1]->type != VASTType_MODULE_DECL){
    PopLevel();
  }
  PopLevel();
}

void VEmitter::Task(String name){
  VAST* decl = PushVAST(VASTType_TASK_DECL,arena);

  decl->task.name = PushString(arena,name);
  decl->task.portDeclarations = PushList<VAST*>(arena);
  decl->task.declarations = PushList<VAST*>(arena);
  
  InsertDeclaration(decl);
  PushLevel(decl);
}

void VEmitter::EndTask(){
  while(buffer[top-1]->type != VASTType_TASK_DECL){
    PopLevel();
  }
  PopLevel();
}

void VEmitter::StartPortGroup(){
  VAST* decl = PushVAST(VASTType_PORT_GROUP,arena);
  
  decl->portGroup.portDeclarations = PushList<VAST*>(arena);
  
  InsertDeclaration(decl);
  PushLevel(decl);
}

void VEmitter::EndPortGroup(){
  PopLevel();
}

void VEmitter::Input(String name,int bitsize){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  decl->portDecl.isInput = true;

  InsertPortDeclaration(decl);
}

void VEmitter::Input(String name,String expr){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%.*s-1:0]",UN(expr));
  decl->portDecl.isInput = true;

  InsertPortDeclaration(decl);
}

void VEmitter::Input(String name,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);

  String repr = PushRepr(temp,expr);
  Input(name,repr);
}

void VEmitter::InputIndexed(const char* format,int index,int bitsize){
  Input(StaticFormat(format,index),bitsize);
}

void VEmitter::InputIndexed(const char* format,int index,String expression){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,format,index);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%.*s-1:0]",UN(expression));
  decl->portDecl.isInput = true;

  InsertPortDeclaration(decl);
}

void VEmitter::InputIndexed(const char* format,int index,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  InputIndexed(format,index,repr);
}

void VEmitter::Output(String name,int bitsize){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  decl->portDecl.isInput = false;

  InsertPortDeclaration(decl);
}

void VEmitter::Output(String name,String expr){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%.*s-1:0]",UN(expr));
  decl->portDecl.isInput = false;

  InsertPortDeclaration(decl);
}

void VEmitter::Output(String name,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  Output(name,repr);
}
  
void VEmitter::OutputIndexed(const char* format,int index,int bitsize){
  Output(StaticFormat(format,index),bitsize);
}

void VEmitter::OutputIndexed(const char* format,int index,String expression){
  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,format,index);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%.*s-1:0]",UN(expression));
  decl->portDecl.isInput = false;

  InsertPortDeclaration(decl);
}

void VEmitter::OutputIndexed(const char* format,int index,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  OutputIndexed(format,index,repr);
}

void VEmitter::Wire(String name,int bitsize){
  VAST* wireDecl = PushVAST(VASTType_VAR_DECL,arena);

  wireDecl->varDecl.name = PushString(arena,name);
  wireDecl->varDecl.arrayDim = {};
  wireDecl->varDecl.bitSize = PushString(arena,"%d",bitsize);
  wireDecl->varDecl.isWire = true;
  
  InsertDeclaration(wireDecl);
}

void VEmitter::Wire(String name,String bitsizeExpr){
  VAST* wireDecl = PushVAST(VASTType_VAR_DECL,arena);

  wireDecl->varDecl.name = PushString(arena,name);
  wireDecl->varDecl.arrayDim = {};
  wireDecl->varDecl.bitSize = PushString(arena,"%.*s",UN(bitsizeExpr));
  wireDecl->varDecl.isWire = true;
  
  InsertDeclaration(wireDecl);
}

void VEmitter::Wire(String name,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  Wire(name,repr);
}
  
void VEmitter::WireArray(String name,int count,int bitsize){
  VAST* wireArray = PushVAST(VASTType_VAR_DECL,arena);

  wireArray->varDecl.name = PushString(arena,name);
  wireArray->varDecl.arrayDim = PushString(arena,"%d",count);
  wireArray->varDecl.bitSize = PushString(arena,"%d",bitsize);
  wireArray->varDecl.isWire = true;
  
  InsertDeclaration(wireArray);
}

void VEmitter::WireArray(String name,int count,String bitsizeExpr){
  VAST* wireArray = PushVAST(VASTType_VAR_DECL,arena);

  wireArray->varDecl.name = PushString(arena,name);
  wireArray->varDecl.arrayDim = PushString(arena,"%d",count);
  wireArray->varDecl.bitSize = PushString(arena,bitsizeExpr);
  wireArray->varDecl.isWire = true;
  
  InsertDeclaration(wireArray);
}

void VEmitter::WireArray(String name,int count,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  WireArray(name,count,repr);
}

void VEmitter::WireAndAssignJoinBlock(String name,String joinElem,int bitsize){
  VAST* wireAssignBlock = PushVAST(VASTType_WIRE_ASSIGN_BLOCK,arena);

  wireAssignBlock->wireAssignBlock.name = PushString(arena,name);
  wireAssignBlock->wireAssignBlock.bitSize = PushString(arena,"%d",bitsize);
  wireAssignBlock->wireAssignBlock.expressions = PushList<VAST*>(arena);
  wireAssignBlock->wireAssignBlock.joinElem = PushString(arena,joinElem);
  
  InsertDeclaration(wireAssignBlock);
  PushLevel(wireAssignBlock);
}

void VEmitter::WireAndAssignJoinBlock(String name,String joinElem,String bitsize){
  VAST* wireAssignBlock = PushVAST(VASTType_WIRE_ASSIGN_BLOCK,arena);

  wireAssignBlock->wireAssignBlock.name = PushString(arena,name);
  wireAssignBlock->wireAssignBlock.bitSize = PushString(arena,bitsize);
  wireAssignBlock->wireAssignBlock.expressions = PushList<VAST*>(arena);
  wireAssignBlock->wireAssignBlock.joinElem = PushString(arena,joinElem);
  
  InsertDeclaration(wireAssignBlock);
  PushLevel(wireAssignBlock);
}

void VEmitter::Reg(String name,int bitsize){
  VAST* regDecl = PushVAST(VASTType_VAR_DECL,arena);

  regDecl->varDecl.name = PushString(arena,name);
  regDecl->varDecl.bitSize = PushString(arena,"%d",bitsize);
  regDecl->varDecl.isWire = false;
  
  InsertDeclaration(regDecl);
}

void VEmitter::Reg(String name,String bitsize){
  VAST* regDecl = PushVAST(VASTType_VAR_DECL,arena);

  regDecl->varDecl.name = PushString(arena,name);
  regDecl->varDecl.bitSize = PushString(arena,bitsize);
  regDecl->varDecl.isWire = false;
  
  InsertDeclaration(regDecl);
}

void VEmitter::Reg(String name,SymbolicExpression* expr){
  TEMP_REGION(temp,arena);
  String repr = PushRepr(temp,expr);

  Reg(name,repr);
}

void VEmitter::Assign(String name,String expr){
  VAST* assignDecl = PushVAST(VASTType_ASSIGN_DECL,arena);
  
  assignDecl->assignOrSet.name = PushString(arena,name);
  assignDecl->assignOrSet.expr = PushString(arena,expr);

  InsertDeclaration(assignDecl);
}

void VEmitter::Integer(String name){
  VAST* decl = PushVAST(VASTType_INTEGER_DECL,arena);

  decl->name = PushString(arena,name);

  InsertDeclaration(decl);
}

void VEmitter::LocalParam(String name,String defaultValue){
  VAST* decl = PushVAST(VASTType_LOCAL_PARAM,arena);

  decl->localParam.name = PushString(arena,name);
  decl->localParam.defaultValue = PushString(arena,defaultValue);
  
  InsertDeclaration(decl);
}

void VEmitter::DeclareInterface(VerilogModuleInterface* interface,String prefix){
  VAST* decl = PushVAST(VASTType_INTERFACE_DECL,arena);

  decl->interfaceDecl.interface = Copy(interface,arena);
  decl->interfaceDecl.prefix = PushString(arena,prefix);

  InsertDeclaration(decl);
}

void VEmitter::Blank(){
  VAST* blank = PushVAST(VASTType_BLANK,arena);
  InsertDeclaration(blank);
}

void VEmitter::Comment(String expr){
  VAST* commentDecl = PushVAST(VASTType_COMMENT,arena);
  commentDecl->comment = PushString(arena,expr);

  InsertDeclaration(commentDecl);
}

void VEmitter::Expression(String expr){
  VAST* exprDecl = PushVAST(VASTType_EXPR,arena);
  exprDecl->expr.content = PushString(arena,expr);

  VAST* assignBlock = FindFirstVASTType(VASTType_WIRE_ASSIGN_BLOCK,true);
  *assignBlock->wireAssignBlock.expressions->PushElem() = exprDecl;
}

void VEmitter::RawStatement(String stmt){
  VAST* stmtDecl = PushVAST(VASTType_RAW_STATEMENT,arena);
  
  stmtDecl->rawStatement = PushString(arena,stmt);

  InsertDeclaration(stmtDecl);
}

void VEmitter::AlwaysBlock(String posedge1,String posedge2){
  VAST* combDecl = PushVAST(VASTType_ALWAYS_BLOCK,arena);

  combDecl->alwaysBlock.sensitivity = PushArray<String>(arena,2);
  combDecl->alwaysBlock.sensitivity[0] = PushString(arena,posedge1);
  combDecl->alwaysBlock.sensitivity[1] = PushString(arena,posedge2);

  combDecl->alwaysBlock.declarations = PushList<VAST*>(arena);
  
  InsertDeclaration(combDecl);
  PushLevel(combDecl);
}

void VEmitter::InitialBlock(){
  VAST* initialDecl = PushVAST(VASTType_INITIAL_BLOCK,arena);

  initialDecl->initialBlock.declarations = PushList<VAST*>(arena);
  
  InsertDeclaration(initialDecl);
  PushLevel(initialDecl);
}

void VEmitter::CombBlock(){
  VAST* combDecl = PushVAST(VASTType_ALWAYS_BLOCK,arena);

  combDecl->alwaysBlock.sensitivity = {};
  combDecl->alwaysBlock.declarations = PushList<VAST*>(arena);
  
  InsertDeclaration(combDecl);
  PushLevel(combDecl);
}

void VEmitter::Set(String identifier,String expr){
  VAST* setDecl = PushVAST(VASTType_SET,arena);
  
  setDecl->assignOrSet.name = PushString(arena,identifier);
  setDecl->assignOrSet.expr = PushString(arena,expr);

  InsertDeclaration(setDecl);
}

void VEmitter::Increment(String identifier){
  VAST* incrDecl = PushVAST(VASTType_SIMPLE_OP,arena);
  
  incrDecl->simpleOp.identifier = PushString(arena,identifier);
  incrDecl->simpleOp.op = '+';
  
  InsertDeclaration(incrDecl);
}

void VEmitter::SetForced(String identifier,String expr,bool isCombLike){
  VAST* setDecl = PushVAST(VASTType_FORCED_SET,arena);
  
  setDecl->assignOrSet.name = PushString(arena,identifier);
  setDecl->assignOrSet.expr = PushString(arena,expr);
  setDecl->assignOrSet.isForcedCombLike = isCombLike;

  InsertDeclaration(setDecl);
}

void VEmitter::Set(String identifier,int val){
  Set(identifier,StaticFormat("%d",val));
}

void VEmitter::If(String expression){
  VAST* ifAST = PushVAST(VASTType_IF,arena);

  ifAST->ifExpr.ifExpressions = PushList<VASTIf>(arena);
  
  // Pushes the first expr+statement combo
  VASTIf expr = {};
  expr.ifExpression = PushString(arena,expression);
  expr.statements = PushList<VAST*>(arena);

  *ifAST->ifExpr.ifExpressions->PushElem() = expr;
  
  InsertDeclaration(ifAST);
  PushLevel(ifAST);
}

void VEmitter::ElseIf(String expression){
  while(buffer[top-1]->type != VASTType_IF){
    EndBlock();
  }

  VAST* ifAst = buffer[top-1];

  VASTIf expr = {};
  expr.ifExpression = PushString(arena,expression);
  expr.statements = PushList<VAST*>(arena);

  *ifAst->ifExpr.ifExpressions->PushElem() = expr;
}

void VEmitter::Else(){
  // NOTE: Kinda expect this to work but not sure. 
  //       It might just be better to force user to call EndBlock everytime
  while(buffer[top-1]->type != VASTType_IF){
    EndBlock();
  }
  
  VAST* ifAst = buffer[top-1];

  // If the top 'if' already has an else, then we must look further up the chain
  if(ifAst->ifExpr.elseStatements){
    EndBlock();
    while(buffer[top-1]->type != VASTType_IF){
      EndBlock();
    }
    ifAst = buffer[top-1];
  }
  
  ifAst->ifExpr.elseStatements = PushList<VAST*>(arena);
}

void VEmitter::EndIf(){
  while(buffer[top-1]->type != VASTType_IF){
    PopLevel();
  }
  PopLevel();
}

void VEmitter::Loop(String start,String loop,String incr){
  VAST* loopAST = PushVAST(VASTType_LOOP,arena);

  loopAST->loopExpr.statements = PushList<VAST*>(arena);
  loopAST->loopExpr.initExpr = PushString(arena,start);
  loopAST->loopExpr.loopExpr = PushString(arena,loop);
  loopAST->loopExpr.incrExpr = PushString(arena,incr);
    
  InsertDeclaration(loopAST);
  PushLevel(loopAST);
}

void VEmitter::EndLoop(){
  while(buffer[top-1]->type != VASTType_LOOP){
    PopLevel();
  }
  EndBlock();
}

void VEmitter::EndBlock(){
  PopLevel();
}

void VEmitter::StartInstance(String moduleName,String instanceName){
  VAST* instanceAST = PushVAST(VASTType_INSTANCE,arena);

  instanceAST->instance.moduleName = PushString(arena,moduleName);
  instanceAST->instance.name = PushString(arena,instanceName);
  instanceAST->instance.parameters = PushList<Pair<String,String>>(arena);
  instanceAST->instance.portConnections = PushList<VAST*>(arena);

  InsertDeclaration(instanceAST);
  PushLevel(instanceAST);
}

void VEmitter::InstanceParam(String paramName,int paramValue){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.parameters->PushElem();
  p->first = PushString(arena,paramName);
  p->second = PushString(arena,"%d",paramValue);
}

void VEmitter::InstanceParam(String paramName,String paramValue){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.parameters->PushElem();
  p->first = PushString(arena,paramName);
  p->second = PushString(arena,paramValue);
}

void VEmitter::InstanceParam(String paramName,SymbolicExpression* sym){
  TEMP_REGION(temp,arena);

  String repr = PushRepr(temp,sym);
  InstanceParam(paramName,CS(repr));
}

void VEmitter::PortConnect(String portName,String connectionExpr){
  VAST* portInfo = PushVAST(VASTType_PORT_CONNECT,arena);
  portInfo->portConnect.portName = PushString(arena,portName);
  portInfo->portConnect.connectExpr = PushString(arena,connectionExpr);

  InsertPortConnect(portInfo);
}

void VEmitter::PortConnectIndexed(const char* portFormat,int index,const char* connectFormat,int connectIndex){
  VAST* portInfo = PushVAST(VASTType_PORT_CONNECT,arena);
  portInfo->portConnect.portName = PushString(arena,portFormat,index);
  portInfo->portConnect.connectExpr = PushString(arena,connectFormat,connectIndex);

  InsertPortConnect(portInfo);
}

void VEmitter::PortConnectIndexed(const char* portFormat,int index,String connectionExpr){
  VAST* portInfo = PushVAST(VASTType_PORT_CONNECT,arena);
  portInfo->portConnect.portName = PushString(arena,portFormat,index);
  portInfo->portConnect.connectExpr = PushString(arena,connectionExpr);

  InsertPortConnect(portInfo);
}

void VEmitter::PortConnectInterface(VerilogModuleInterface* interface,String interfaceWirePrefix){
  TEMP_REGION(temp,arena);

  StartPortGroup();
  
  for(VerilogPortGroup group : interface->portGroups){
    for(VerilogPortSpec spec : group.ports){
      String connExpression = spec.name;
      if(!Empty(interfaceWirePrefix)){
        connExpression = PushString(temp,"%.*s_%.*s",UN(interfaceWirePrefix),UN(spec.name));
      }

      PortConnect(spec.name,connExpression);
    }
  }

  EndPortGroup();
}

void VEmitter::PortConnect(Array<VerilogPortSpec> specs,String sufix){
  TEMP_REGION(temp,arena);

  StartPortGroup();
  
  for(VerilogPortSpec spec : specs){
    String connExpression = spec.name;

    if(!Empty(sufix)){
      connExpression = PushString(temp,"%.*s%.*s",UN(spec.name),UN(sufix));
    }
    
    PortConnect(spec.name,connExpression);
  }

  EndPortGroup();

}

void VEmitter::EndInstance(){
  Assert(buffer[top-1]->type == VASTType_INSTANCE);
  PopLevel();
}

VAST* PushVAST(VASTType type,Arena* out){
  VAST* res = PushStruct<VAST>(out);
  res->type = type;
  return res;
}

VEmitter* StartVCode(Arena* out){
  VEmitter* emitter = PushStruct<VEmitter>(out);
  emitter->arena = out;
  emitter->buffer = PushArray<VAST*>(out,16);
  emitter->topLevel = PushVAST(VASTType_TOP_LEVEL,out);
  emitter->topLevel->top.declarations = PushList<VAST*>(out);
  emitter->topLevel->top.portConnections = PushList<VAST*>(out);
  emitter->topLevel->top.includes = PushList<String>(out);

  return emitter;
}

VAST* EndVCode(VEmitter* m){
  return m->topLevel; 
}

struct VState{
  bool isComb;
};

void Repr(VAST* top,StringBuilder* b,VState* state,int level);

static void EmitStatementList(StringBuilder* b,ArenaList<VAST*>* statements,VState* state,int level){
  if(statements){
    for(SingleLink<VAST*>* iter = statements->head; iter; iter = iter->next){
      VAST* ast = iter->elem;

      Repr(ast,b,state,level);
      b->PushString("\n");
    }
  }
}

void Repr(VAST* top,StringBuilder* b,VState* state,int level){
  TEMP_REGION(temp,b->arena);

  FULL_SWITCH(top->type){
  case VASTType_TOP_LEVEL:{
    if(!Empty(top->top.timescaleExpression)){
      b->PushString("`timescale %.*s\n\n",UN(top->top.timescaleExpression));
    }

    for(String name : top->top.includes){
      b->PushString("`include \"%.*s\"\n",UN(name));
    }
    
    EmitStatementList(b,top->top.declarations,state,level);

    // Because we want to be able to emit stuff like portmaps and port connection files,
    // the top level must be able to work even if it does not start with a module connection
    // If this starts becoming complicated, need to find an alternate way of doing this.
    for(VAST* ast : top->top.portConnections){
      Repr(ast,b,state,level + 1);
      b->PushString(",\n");
    }
  } break;

  case VASTType_COMMENT:{
    b->PushSpaces(level * 2);

    bool hasNewlines = false;
    for(char ch : top->comment){
      if(ch == '\n'){
        hasNewlines = true;
      }
    }

    if(hasNewlines){
      b->PushString("/* \n %.*s",UN(top->comment));
      b->PushString("\n*/\n");
    } else {
      b->PushString("// %.*s",UN(top->comment));
    }
  } break;

  case VASTType_PORT_GROUP:{
    for(VAST* ast : top->portGroup.portDeclarations){
      Repr(ast,b,state,level + 1);
      b->PushString(",\n");
    }
  } break;
  
  case VASTType_MODULE_DECL:{
    b->PushSpaces(level * 2);
    b->PushString("module %.*s ",UN(top->module.name));

    if(!Empty(top->module.parameters)){
      b->PushString("#(\n");

      bool first = true;
      for(auto p : top->module.parameters){
        if(!first){
          b->PushString(",\n");
        }
        
        b->PushSpaces((level + 1) * 2);

        b->PushString("parameter %.*s = %.*s",UN(p.first),UN(p.second));
        first = false;
      }
      b->PushString("\n");
      b->PushSpaces(level * 2);
      b->PushString(") ");
    }
    
    b->PushString("(\n");

    bool first = true;
    for(VAST* ast : top->module.portDeclarations){
      if(!first){
        b->PushString(",\n");
      }

      Repr(ast,b,state,level + 1);
      first = false;
    }
    b->PushString("\n");
    b->PushSpaces(level * 2);
    b->PushString(");\n");

    EmitStatementList(b,top->module.declarations,state,level + 1);

    b->PushString("\nendmodule");
  } break;

  case VASTType_TASK_DECL: {
    b->PushSpaces(level * 2);

    if(Empty(top->task.portDeclarations)){
      b->PushString("task %.*s;\n",UN(top->task.name));
    } else {
      b->PushString("task %.*s (",UN(top->task.name));

      bool first = true;
      for(VAST* ast : top->task.portDeclarations){
        if(!first){
          b->PushString(",");
        }

        Repr(ast,b,state,0);
        first = false;
      }
      b->PushString(");\n");
    }
    
    b->PushSpaces(level * 2);
    b->PushString("begin\n");
    
    EmitStatementList(b,top->task.declarations,state,level + 1);

    b->PushSpaces(level * 2);
    b->PushString("end\n");

    b->PushSpaces(level * 2);
    b->PushString("endtask\n");
  } break;
  
  case VASTType_INSTANCE:{
    b->PushSpaces(level * 2);
    
    b->PushString("%.*s ",UN(top->instance.moduleName));
    
    if(!Empty(top->instance.parameters)){
      b->PushString("#(\n");
      bool first = true;
      for(auto p : top->instance.parameters){
        if(!first){
          b->PushString(",\n");
        }
        
        b->PushSpaces((level + 1) * 2);

        b->PushString(".%.*s(%.*s)",UN(p.first),UN(p.second));
        first = false;
      }
      b->PushString("\n");
      b->PushSpaces(level * 2);
      b->PushString(") ");
    }
    
    b->PushString("%.*s (\n",UN(top->instance.name));

    bool first = true;
    for(VAST* ast : top->instance.portConnections){
      if(!first){
        b->PushString(",\n");
      }
        
      Repr(ast,b,state,level + 1);
      first = false;
    }
    b->PushString("\n");
    b->PushSpaces(level * 2);
    b->PushString(");\n");
  } break;
  
  case VASTType_PORT_DECL:{
    b->PushSpaces(level * 2);
    if(top->portDecl.isInput){
      b->PushString("input ");
    } else {
      b->PushString("output ");
    }

    b->PushString(top->portDecl.bitSizeExpr);
    b->PushChar(' ');
    b->PushString(top->portDecl.name);
  } break;
  
  case VASTType_INTEGER_DECL:{
    b->PushSpaces(level * 2);
    b->PushString("integer %.*s;",UN(top->name));
  } break;

  case VASTType_LOCAL_PARAM: {
    b->PushSpaces(level * 2);
    
    if(Empty(top->localParam.defaultValue)){
      b->PushString("localparam %.*s;",UN(top->localParam.name));
    } else {
      b->PushString("localparam %.*s = %.*s;",UN(top->localParam.name),UN(top->localParam.defaultValue));
    }
  } break;

  case VASTType_VAR_DECL:{
    b->PushSpaces(level * 2);
    if(top->varDecl.isWire){
      b->PushString("wire ");
    } else {
      b->PushString("reg ");
    }

    b->PushString("[%.*s-1:0] ",UN(top->varDecl.bitSize));

    b->PushString(top->varDecl.name);

    if(!Empty(top->varDecl.arrayDim)){
      b->PushString("[%.*s]",UN(top->varDecl.arrayDim));
    }
    b->PushString(";");
  } break;

  case VASTType_INTERFACE_DECL:{
    VerilogModuleInterface* inter = top->interfaceDecl.interface;
    String prefix = top->interfaceDecl.prefix;

    for(VerilogPortGroup group : inter->portGroups){
      b->PushSpaces(level * 2);
      b->PushString("// %.*s\n",UN(group.name));
      for(VerilogPortSpec spec : group.ports){
        b->PushSpaces(level * 2);

        String repr = PushRepr(temp,spec.size);

        if(spec.dir == WireDir_INPUT){
          b->PushString("reg ");
        } else {
          Assert(spec.dir == WireDir_OUTPUT);
          b->PushString("wire ");
        }
        
        b->PushString("[(%.*s)-1:0] ",UN(repr));

        if(Empty(prefix)){
          b->PushString("%.*s;\n",UN(spec.name));
        } else {
          b->PushString("%.*s_%.*s;\n",UN(prefix),UN(spec.name));
        }
      }
    }
    
  } break;
  
  case VASTType_WIRE_ASSIGN_BLOCK:{
    b->PushSpaces(level * 2);
    b->PushString("wire ");

    b->PushString("[%.*s-1:0] ",UN(top->wireAssignBlock.bitSize));

    b->PushString("%.*s = (",UN(top->wireAssignBlock.name));
    
    bool first = true;
    for(VAST* vast : top->wireAssignBlock.expressions){
      if(!first){
        b->PushString(" %.*s ",UN(top->wireAssignBlock.joinElem));
      }

      Repr(vast,b,state,level);
      first = false;
    }

    b->PushString(");");
  } break;
  
  case VASTType_ASSIGN_DECL:{
    b->PushSpaces(level * 2);
    b->PushString("assign %.*s = %.*s;",UN(top->assignOrSet.name),UN(top->assignOrSet.expr));
  } break;
  
  case VASTType_IF:{
    SingleLink<VASTIf>* iter = top->ifExpr.ifExpressions->head;
    VASTIf ifExpr = iter->elem;

    b->PushSpaces(level * 2);

#if 0
    if(OnlyOneElement(ifExpr.statements) && OnlyOneElement(top->ifExpr.ifExpressions) && Empty(top->ifExpr.elseStatements)){
      b->PushString("if(%.*s) begin ",UN(ifExpr.ifExpression));
      Repr(ifExpr.statements->head->elem,b,state,0);
      b->PushString(" end ");
    } else {
#endif
      b->PushString("if(%.*s) begin\n",UN(ifExpr.ifExpression));

      EmitStatementList(b,ifExpr.statements,state,level + 1);
      
      b->PushSpaces(level * 2);
      b->PushString("end ");
#if 0
    }
#endif      
    for(SingleLink<VASTIf>* otherIter = iter->next; otherIter; otherIter = otherIter->next){
      VASTIf ifExpr = otherIter->elem;
      
      b->PushString("else if(%.*s) begin\n",UN(ifExpr.ifExpression));

      EmitStatementList(b,ifExpr.statements,state,level + 1);
        
      b->PushSpaces(level * 2);
      b->PushString("end ");
    }
    
    if(top->ifExpr.elseStatements){
      b->PushString("else begin\n");

      EmitStatementList(b,top->ifExpr.elseStatements,state,level + 1);
      
      b->PushSpaces(level * 2);
      b->PushString("end");
    }
  } break;

  case VASTType_LOOP:{
    b->PushSpaces(level * 2);

    if(Empty(top->loopExpr.initExpr) && Empty(top->loopExpr.incrExpr)){
      b->PushString("while(%.*s) begin\n",UN(top->loopExpr.loopExpr));
    } else {
      b->PushString("for(%.*s ; %.*s ; %.*s) begin\n",UN(top->loopExpr.initExpr),UN(top->loopExpr.loopExpr),UN(top->loopExpr.incrExpr));
    }

    EmitStatementList(b,top->loopExpr.statements,state,level + 1);
    
    b->PushSpaces(level * 2);
    b->PushString("end");
  } break;
    
  case VASTType_SET:{
    b->PushSpaces(level * 2);
    if(state->isComb){
      b->PushString("%.*s = %.*s;",UN(top->assignOrSet.name),UN(top->assignOrSet.expr));
    } else {
      b->PushString("%.*s <= %.*s;",UN(top->assignOrSet.name),UN(top->assignOrSet.expr));
    }
  } break;

  case VASTType_SIMPLE_OP:{
    b->PushSpaces(level * 2);
    String name = top->simpleOp.identifier;
    char op = top->simpleOp.op;
    
    String eq = "<=";
    if(state->isComb){
      eq = "=";
    }
    
    b->PushString("%.*s %.*s %.*s %c 1;",UN(name),UN(eq),UN(name),op);
  } break;

  case VASTType_FORCED_SET:{
    b->PushSpaces(level * 2);
    if(top->assignOrSet.isForcedCombLike){
      b->PushString("%.*s = %.*s;",UN(top->assignOrSet.name),UN(top->assignOrSet.expr));
    } else {
      b->PushString("%.*s <= %.*s;",UN(top->assignOrSet.name),UN(top->assignOrSet.expr));
    }
  } break;

  
  case VASTType_BLANK: {
    // Do nothing. Outer code should add a new line by itself since we are a declaration
  } break;

  case VASTType_PORT_CONNECT: {
    b->PushSpaces(level * 2);
    b->PushString(".%.*s(%.*s)",UN(top->portConnect.portName),UN(top->portConnect.connectExpr));
  } break;
  
  case VASTType_EXPR:{
    b->PushString("%.*s",UN(top->expr.content));
  } break;

  case VASTType_RAW_STATEMENT: {
    b->PushSpaces(level * 2);
    b->PushString("%.*s;",UN(top->rawStatement));
  } break;
  
  case VASTType_ALWAYS_BLOCK:{
    bool oldIsComb = state->isComb;

    b->PushSpaces(level * 2);
    
    if(top->alwaysBlock.sensitivity.size == 0){
      b->PushString("always @* begin\n");
      state->isComb = true;
    } else if(top->alwaysBlock.sensitivity.size == 2){
      String edge0 = top->alwaysBlock.sensitivity[0];
      String edge1 = top->alwaysBlock.sensitivity[1];
      b->PushString("always @(posedge %.*s,posedge %.*s) begin\n",UN(edge0),UN(edge1));
      state->isComb = false;
    } else {
      NOT_IMPLEMENTED("yet");
    }

    EmitStatementList(b,top->alwaysBlock.declarations,state,level + 1);

    b->PushSpaces(level * 2);
    b->PushString("end\n");

    state->isComb = oldIsComb;
  } break;

  case VASTType_INITIAL_BLOCK:{
    b->PushSpaces(level * 2);

    b->PushString("initial begin\n");

    bool oldIsComb = state->isComb;
    state->isComb = true;
    EmitStatementList(b,top->initialBlock.declarations,state,level + 1);
    state->isComb = oldIsComb;
    
    b->PushSpaces(level * 2);
    b->PushString("end\n");
  } break;
  
  } END_SWITCH();
}

void Repr(VAST* top,StringBuilder* b){
  VState state = {};

  Repr(top,b,&state,0);
}

String EndVCodeAndPrint(VEmitter* v,Arena* out){
  TEMP_REGION(temp,out);

  auto* builder = StartString(temp);
  auto* ast = EndVCode(v);

  Repr(ast,builder);

  return EndString(out,builder);
}
