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
    *topLevel->declarations->PushElem() = declarationAST;
    return;
  }

  for(int i = top - 1; i >= 0; i--){
    VAST* cast = buffer[i];
    VASTType type = cast->type;

    FULL_SWITCH(type){
    case VASTType_INSTANCE: continue;
    case VASTType_PORT_DECL: continue;
    case VASTType_MODULE_DECL: {
      NOT_IMPLEMENTED("Need to store normal declarations here");
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
  topLevel->timescaleExpression = PushString(arena,"%s / %s",timeUnit,timePrecision);
}

  // Module definition
void VEmitter::Module(String name){
  VAST* decl = PushVAST(VASTType_MODULE_DECL,arena);

  decl->module.name = PushString(arena,name);
  decl->module.parameters = PushArenaList<Pair<String,String>>(arena);
  decl->module.portDeclarations = PushArenaList<VAST*>(arena);

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

  if(bitsize == 1){
    decl->portDecl.bitSizeExpr = {};
  } else {
    decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  }

  decl->portDecl.isInput = true;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::Input(String name,int bitsize){
  Input(StaticFormat("%.*s",UNPACK_SS(name)),bitsize);
}

void VEmitter::IndexedInput(const char* format,int index,int bitsize){
  Input(StaticFormat(format,index),bitsize);
}

void VEmitter::IndexedInput(const char* format,int index,const char* expression){
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

  if(bitsize == 1){
    decl->portDecl.bitSizeExpr = {};
  } else {
    decl->portDecl.bitSizeExpr = PushString(arena,"[%d-1:0]",bitsize);
  }

  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
}

void VEmitter::IndexedOutput(const char* format,int index,int bitsize){
  Output(StaticFormat(format,index),bitsize);
}


void VEmitter::IndexedOutput(const char* format,int index,const char* expression){
  VAST* moduleDecl = FindFirstVASTType(VASTType_MODULE_DECL,true);

  VAST* decl = PushVAST(VASTType_PORT_DECL,arena);
  decl->portDecl.name = PushString(arena,format,index);
  decl->portDecl.bitSizeExpr = PushString(arena,"[%s-1:0]",expression);

  decl->portDecl.isInput = false;

  *moduleDecl->module.portDeclarations->PushElem() = decl;
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
  emitter->topLevel->declarations = PushArenaList<VAST*>(out);
  
  return emitter;
}

VAST* EndVCode(VEmitter* m){
  return m->topLevel; 
}

void Repr(VAST* top,StringBuilder* b,int level){
  FULL_SWITCH(top->type){
  case VASTType_TOP_LEVEL:{
    for(VAST* ast : top->declarations){
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
  } END_SWITCH();
}
