#include "VerilogEmitter.hpp"
#include "utilsCore.hpp"

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
    case VASTType_SET:
    case VASTType_EXPR:
    case VASTType_ASSIGN_DECL:
    case VASTType_INSTANCE:;
    case VASTType_BLANK:
    case VASTType_PORT_DECL: continue;

    case VASTType_WIRE_ASSIGN_BLOCK:{
      *cast->wireAssignBlock.expressions->PushElem() = declarationAST;
      return;
    } break;

    case VASTType_IF:{
      *cast->ifExpr.declarations->PushElem() = declarationAST;
      return;
    } break;
      
    case VASTType_COMB_DECL: {
      *cast->alwaysBlock.declarations->PushElem() = declarationAST;
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
    printf("Did not find VAST of type: %.*s\n",UNPACK_SS(Repr(type)));
    printf("Emitter stack currently has %d levels and is, from zero:\n",top);
    for(int i = 0; i < top; i++){
      printf("%.*s\n",UNPACK_SS(Repr(buffer[i]->type)));
    }
    DEBUG_BREAK();
  }
    
  return nullptr;
}

void VEmitter::Timescale(const char* timeUnit,const char* timePrecision){
  topLevel->top.timescaleExpression = PushString(arena,"%s / %s",timeUnit,timePrecision);
}

  // Module definition
void VEmitter::Module(String name){
  VAST* decl = PushVAST(VASTType_MODULE_DECL,arena);

  decl->module.name = PushString(arena,name);
  decl->module.parameters = PushArenaList<Pair<String,String>>(arena);
  decl->module.portDeclarations = PushArenaList<VAST*>(arena);
  decl->module.declarations = PushArenaList<VAST*>(arena);
  
  InsertDeclaration(decl);
  PushLevel(decl);
}

void VEmitter::DeclParam(const char* name,int value){
  VAST* decl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  Pair<String,String>* p = decl->module.parameters->PushElem();
  p->first = PushString(arena,"%s",name);
  p->second = PushString(arena,"%d",value);
}
  
void VEmitter::Input(const char* name,int bitsize){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,"%s",name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  decl->portDecl.isInput = true;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Input(String name,int bitsize){
  Input(StaticFormat("%.*s",UNPACK_SS(name)),bitsize);
}

void VEmitter::Input(const char* name,const char* expr){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,"%s",name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%s-1:0]",expr);
  decl->portDecl.isInput = true;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::InputIndexed(const char* format,int index,int bitsize){
  Input(StaticFormat(format,index),bitsize);
}

void VEmitter::InputIndexed(const char* format,int index,const char* expression){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,format,index);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%s-1:0]",expression);
  decl->portDecl.isInput = true;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Output(const char* name,int bitsize){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,"%s",name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Output(const char* name,const char* expr){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,"%s",name);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%s-1:0]",expr);
  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Output(String name,int bitsize){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,"%.*s",UNPACK_SS(name));
  decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::OutputIndexed(const char* format,int index,int bitsize){
  Output(StaticFormat(format,index),bitsize);
}

void VEmitter::OutputIndexed(const char* format,int index,const char* expression){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,format,index);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%s-1:0]",expression);
  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Wire(const char* name,int bitsize){
  VAST* wireDecl = PushVAST(VASTType_VAR_DECL,arena);

  wireDecl->varDecl.name = PushString(arena,"%s",name);
  wireDecl->varDecl.arrayDim = {};
  wireDecl->varDecl.bitSize = PushString(arena,"%d",bitsize);
  wireDecl->varDecl.isWire = true;
  
  InsertDeclaration(wireDecl);
}

void VEmitter::WireArray(const char* name,int count,int bitsize){
  VAST* wireArray = PushVAST(VASTType_VAR_DECL,arena);

  wireArray->varDecl.name = PushString(arena,"%s",name);
  wireArray->varDecl.arrayDim = PushString(arena,"%d",count);
  wireArray->varDecl.bitSize = PushString(arena,"%d",bitsize);
  wireArray->varDecl.isWire = true;
  
  InsertDeclaration(wireArray);
}

void VEmitter::WireAndAssignJoinBlock(const char* name,const char* joinElem,int bitsize){
  VAST* wireAssignBlock = PushVAST(VASTType_WIRE_ASSIGN_BLOCK,arena);

  wireAssignBlock->wireAssignBlock.name = PushString(arena,"%s",name);
  wireAssignBlock->wireAssignBlock.bitSize = PushString(arena,"%d",bitsize);
  wireAssignBlock->wireAssignBlock.expressions = PushArenaList<VAST*>(arena);
  wireAssignBlock->wireAssignBlock.joinElem = PushString(arena,"%s",joinElem);
  
  InsertDeclaration(wireAssignBlock);
  PushLevel(wireAssignBlock);
}

void VEmitter::Reg(const char* name,int bitsize){
  VAST* regDecl = PushVAST(VASTType_VAR_DECL,arena);

  regDecl->varDecl.name = PushString(arena,"%s",name);
  regDecl->varDecl.bitSize = PushString(arena,"%d",bitsize);
  regDecl->varDecl.isWire = false;
  
  InsertDeclaration(regDecl);
}

void VEmitter::Assign(const char* name,const char* expr){
  VAST* assignDecl = PushVAST(VASTType_ASSIGN_DECL,arena);
  
  assignDecl->assignOrSet.name = PushString(arena,"%s",name);
  assignDecl->assignOrSet.expr = PushString(arena,"%s",expr);

  InsertDeclaration(assignDecl);
}

void VEmitter::Assign(String name,String expr){
  VAST* assignDecl = PushVAST(VASTType_ASSIGN_DECL,arena);
  
  assignDecl->assignOrSet.name = PushString(arena,name);
  assignDecl->assignOrSet.expr = PushString(arena,expr);

  InsertDeclaration(assignDecl);
}

void VEmitter::Blank(){
  VAST* blank = PushVAST(VASTType_BLANK,arena);
  InsertDeclaration(blank);
}

void VEmitter::Expression(const char* expr){
  VAST* exprDecl = PushVAST(VASTType_EXPR,arena);
  exprDecl->expr.content = PushString(arena,"%s",expr);

  VAST* assignBlock = FindFirstVASTType(VASTType_WIRE_ASSIGN_BLOCK,true);
  *assignBlock->wireAssignBlock.expressions->PushElem() = exprDecl;
}

void VEmitter::CombBlock(){
  VAST* combDecl = PushVAST(VASTType_COMB_DECL,arena);

  combDecl->alwaysBlock.sensitivity = nullptr;
  combDecl->alwaysBlock.declarations = PushArenaList<VAST*>(arena);
  
  InsertDeclaration(combDecl);
  PushLevel(combDecl);
}

void VEmitter::Set(const char* identifier,const char* expr){
  VAST* setDecl = PushVAST(VASTType_SET,arena);
  
  setDecl->assignOrSet.name = PushString(arena,"%s",identifier);
  setDecl->assignOrSet.expr = PushString(arena,"%s",expr);

  InsertDeclaration(setDecl);
}

void VEmitter::Set(String identifier,String expr){
  VAST* setDecl = PushVAST(VASTType_SET,arena);
  
  setDecl->assignOrSet.name = PushString(arena,identifier);
  setDecl->assignOrSet.expr = PushString(arena,expr);

  InsertDeclaration(setDecl);
}

void VEmitter::Set(const char* identifier,int val){
  Set(identifier,StaticFormat("%d",val));
}

void VEmitter::If(const char* expr){
  VAST* ifDecl = PushVAST(VASTType_IF,arena);
  
  ifDecl->ifExpr.expr = PushString(arena,"%s",expr);
  ifDecl->ifExpr.declarations = PushArenaList<VAST*>(arena);
  
  InsertDeclaration(ifDecl);
  PushLevel(ifDecl);
}

void VEmitter::EndIf(){
  while(buffer[top-1]->type != VASTType_IF){
    PopLevel();
  }
  PopLevel();
}
  
void VEmitter::EndBlock(){
  PopLevel();
}

void VEmitter::StartInstance(const char* moduleName,const char* instanceName){
  VAST* instanceAST = PushVAST(VASTType_INSTANCE,arena);

  instanceAST->instance.moduleName = PushString(arena,"%s",moduleName);
  instanceAST->instance.name = PushString(arena,"%s",instanceName);
  instanceAST->instance.parameters = PushArenaList<Pair<String,String>>(arena);
  instanceAST->instance.portConnections = PushArenaList<Pair<String,String>>(arena);

  InsertDeclaration(instanceAST);
  PushLevel(instanceAST);
}

void VEmitter::StartInstance(String moduleName,const char* instanceName){
  VAST* instanceAST = PushVAST(VASTType_INSTANCE,arena);

  instanceAST->instance.moduleName = PushString(arena,moduleName);
  instanceAST->instance.name = PushString(arena,"%s",instanceName);
  instanceAST->instance.parameters = PushArenaList<Pair<String,String>>(arena);
  instanceAST->instance.portConnections = PushArenaList<Pair<String,String>>(arena);

  InsertDeclaration(instanceAST);
  PushLevel(instanceAST);
}

void VEmitter::InstanceParam(const char* paramName,int paramValue){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.parameters->PushElem();
  p->first = PushString(arena,"%s",paramName);
  p->second = PushString(arena,"%d",paramValue);
}
  
void VEmitter::PortConnect(const char* portName,const char* connectionExpr){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.portConnections->PushElem();
  p->first = PushString(arena,"%s",portName);
  p->second = PushString(arena,"%s",connectionExpr);
}

void VEmitter::PortConnect(String portName,String connectionExpr){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.portConnections->PushElem();
  p->first = PushString(arena,portName);
  p->second = PushString(arena,connectionExpr);
}

void VEmitter::PortConnectIndexed(const char* portFormat,int index,const char* connectionExpr){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.portConnections->PushElem();
  p->first = PushString(arena,portFormat,index);
  p->second = PushString(arena,"%s",connectionExpr);
}

void VEmitter::PortConnectIndexed(const char* portFormat,int index,const char* connectFormat,int connectIndex){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.portConnections->PushElem();
  p->first = PushString(arena,portFormat,index);
  p->second = PushString(arena,connectFormat,connectIndex);
}

void VEmitter::PortConnectIndexed(const char* portFormat,int index,String connectionExpr){
  VAST* decl = FindFirstVASTType(VASTType_INSTANCE,true);

  Pair<String,String>* p = decl->instance.portConnections->PushElem();
  p->first = PushString(arena,portFormat,index);
  p->second = PushString(arena,connectionExpr);
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
  emitter->topLevel->top.declarations = PushArenaList<VAST*>(out);
  
  return emitter;
}

VAST* EndVCode(VEmitter* m){
  return m->topLevel; 
}

void Repr(VAST* top,StringBuilder* b,int level){
  FULL_SWITCH(top->type){
  case VASTType_TOP_LEVEL:{
    if(!Empty(top->top.timescaleExpression)){
      b->PushString("`timescale %.*s\n\n",UNPACK_SS(top->top.timescaleExpression));
    }
      
    for(VAST* ast : top->top.declarations){
      Repr(ast,b,level);
      b->PushString("\n");
    }
  } break;
  
  case VASTType_MODULE_DECL:{
    b->PushSpaces(level * 2);
    b->PushString("module %.*s ",UNPACK_SS(top->module.name));

    if(!Empty(top->module.parameters)){
      b->PushString("#(\n");

      bool first = true;
      for(auto p : top->module.parameters){
        if(!first){
          b->PushString(",\n");
        }
        
        b->PushSpaces((level + 1) * 2);

        b->PushString("parameter %.*s = %.*s",UNPACK_SS(p.first),UNPACK_SS(p.second));
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

      Repr(ast,b,level + 1);
      first = false;
    }
    b->PushString("\n");
    b->PushSpaces(level * 2);
    b->PushString(");\n");

    for(VAST* ast : top->module.declarations){
      Repr(ast,b,level + 1);
      b->PushString("\n");
    }

    b->PushString("endmodule\n");
  } break;
  
  case VASTType_INSTANCE:{
    b->PushSpaces(level * 2);
    
    b->PushString("%.*s",UNPACK_SS(top->instance.moduleName));
    
    if(!Empty(top->instance.parameters)){
      b->PushString(" #(\n");
      bool first = true;
      for(auto p : top->instance.parameters){
        if(!first){
          b->PushString(",\n");
        }
        
        b->PushSpaces((level + 1) * 2);

        b->PushString(".%.*s(%.*s)",UNPACK_SS(p.first),UNPACK_SS(p.second));
        first = false;
      }
      b->PushString("\n");
      b->PushSpaces(level * 2);
      b->PushString(")\n");
    }
    
    b->PushSpaces(level * 2);
    b->PushString("%.*s\n",UNPACK_SS(top->instance.name));
    
    b->PushSpaces(level * 2);
    b->PushString("(\n");

    bool first = true;
    for(auto p : top->instance.portConnections){
      if(!first){
        b->PushString(",\n");
      }
        
      b->PushSpaces((level + 1) * 2);

      b->PushString(".%.*s(%.*s)",UNPACK_SS(p.first),UNPACK_SS(p.second));
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
  
  case VASTType_VAR_DECL:{
    b->PushSpaces(level * 2);
    if(top->varDecl.isWire){
      b->PushString("wire ");
    } else {
      b->PushString("reg ");
    }

    b->PushString("[%.*s-1:0] ",UNPACK_SS(top->varDecl.bitSize));

    b->PushString(top->varDecl.name);

    if(!Empty(top->varDecl.arrayDim)){
      b->PushString("[%.*s-1:0]",UNPACK_SS(top->varDecl.arrayDim));
    }
    b->PushString(";");
  } break;
  
  case VASTType_WIRE_ASSIGN_BLOCK:{
    b->PushSpaces(level * 2);
    b->PushString("wire ");

    b->PushString("[%.*s-1:0] ",UNPACK_SS(top->wireAssignBlock.bitSize));

    b->PushString("%.*s = (",UNPACK_SS(top->wireAssignBlock.name));
    
    bool first = true;
    for(VAST* vast : top->wireAssignBlock.expressions){
      if(!first){
        b->PushString(" %.*s ",UNPACK_SS(top->wireAssignBlock.joinElem));
      }

      Repr(vast,b,level);
      first = false;
    }

    b->PushString(");");
  } break;
  
  case VASTType_ASSIGN_DECL:{
    b->PushSpaces(level * 2);
    b->PushString("assign %.*s = %.*s;",UNPACK_SS(top->assignOrSet.name),UNPACK_SS(top->assignOrSet.expr));
  } break;
  
  case VASTType_IF:{
    b->PushSpaces(level * 2);
    b->PushString("if(%.*s) begin\n",UNPACK_SS(top->ifExpr.expr));

    for(VAST* ast : top->ifExpr.declarations){
      Repr(ast,b,level + 1);
      b->PushString("\n");
    }
    
    b->PushSpaces(level * 2);
    b->PushString("end");
  } break;
  
  case VASTType_SET:{
    b->PushSpaces(level * 2);
    b->PushString("%.*s = %.*s;",UNPACK_SS(top->assignOrSet.name),UNPACK_SS(top->assignOrSet.expr));
  } break;
  
  case VASTType_BLANK: {
    // Do nothing. Outer code should add a new line by itself since we are a declaration
  } break;

  case VASTType_EXPR:{
    b->PushString("%.*s",UNPACK_SS(top->expr.content));
  } break;
  
  case VASTType_COMB_DECL:{
    b->PushString("always @* begin\n");

    for(VAST* ast : top->alwaysBlock.declarations){
      Repr(ast,b,level + 1);
      b->PushString("\n");
    }
    b->PushString("end\n");
  } break;
  

  } END_SWITCH();
}
