#include "emitter.hpp"

String Repr(CASTType type){
  switch(type){
  case CASTType_TOP_LEVEL: return STRING("CASTType_TOP_LEVEL");
  case CASTType_FUNCTION: return STRING("CASTType_FUNCTION");
  case CASTType_VAR_DECL: return STRING("VAR_DECL");
  case CASTType_IF: return STRING("IF");
  }

  return STRING("No repr for the given type, check if forgot to implement it: %d\n",(int) type);
}

bool IsBlockType(CASTType type){
  switch(type){
  case CASTType_VAR_DECL:; 
  case CASTType_TOP_LEVEL: return false;

    
    
  case CASTType_FUNCTION:
  case CASTType_IF: return true;
  }

  NOT_POSSIBLE();
}

CAST* PushCAST(CASTType type,Arena* out){
  CAST* res = PushStruct<CAST>(out);
  res->type = type;
  return res;
}

void CEmitter::PushLevel(CAST* level){
  Assert(level->type != CASTType_TOP_LEVEL);
  
  buffer[top++] = level;
}

void CEmitter::PopLevel(){
  Assert(top > 0);
  --top;
}

CAST* CEmitter::FindFirstCASTType(CASTType type,bool errorIfNotFound){
  if(type == CASTType_TOP_LEVEL){
    return topLevel;
  }
    
  for(i8 i = top - 1; i >= 0; i--){
    if(buffer[i]->type == type){
      return buffer[i];
    }
  }

  if(errorIfNotFound){
    printf("Did not find CAST of type: %.*s\n",UNPACK_SS(Repr(type)));
    printf("Emitter stack currently has %d levels and is, from zero:\n",top);
    for(int i = 0; i < top; i++){
      printf("%.*s\n",UNPACK_SS(Repr(buffer[i]->type)));
    }
    DEBUG_BREAK();
  }
    
  return nullptr;
}

void CEmitter::InsertStatement(CAST* statementCAST){
  for(int i = top - 1; i >= 0; i--){
    CAST* cast = buffer[i];
    CASTType type = cast->type;

    switch(type){
    case CASTType_TOP_LEVEL: Assert(false); // Cannot find top level inside this 

    // Non block types
    case CASTType_VAR_DECL: continue; 

    // All the "block" types
    case CASTType_IF: {
      if(cast->elseStatements){
        *cast->elseStatements->PushElem() = statementCAST;
      } else {
        *cast->ifExpressions->tail->elem.statements->PushElem() = statementCAST;
      }
      return;
    } break;
    case CASTType_FUNCTION: {
      *cast->statements->PushElem() = statementCAST;
      return;
    } break;
    }
  }
}

void CEmitter::Function(String returnType,String functionName){
  Assert(!FindFirstCASTType(CASTType_FUNCTION,false));

  CAST* top = FindFirstCASTType(CASTType_TOP_LEVEL,arena);
    
  CAST* function = PushCAST(CASTType_FUNCTION,arena);
  function->returnType = PushString(arena,returnType);
  function->functionName = PushString(arena,functionName);
  function->arguments = PushArenaList<CAST*>(arena);
  function->statements = PushArenaList<CAST*>(arena);
    
  PushLevel(function);

  *top->functions->PushElem() = function;    
}

void CEmitter::If(String expression){
  CAST* ifAst = PushCAST(CASTType_IF,arena);

  // Pushes the list for the expr+statement combo
  ifAst->ifExpressions = PushArenaList<CASTIf>(arena);

  // Pushes the first expr+statement combo
  CASTIf expr = {};
  expr.ifExpression = PushString(arena,expression);
  expr.statements = PushArenaList<CAST*>(arena);

  *ifAst->ifExpressions->PushElem() = expr;
  
  InsertStatement(ifAst);
  PushLevel(ifAst);
}

void CEmitter::ElseIf(String expression){
  // NOTE: Kinda expect this to work but not sure. 
  while(buffer[top-1]->type != CASTType_IF){
    EndBlock();
  }

  CAST* ifAst = buffer[top-1];
  
  CASTIf expr = {};
  expr.ifExpression = PushString(arena,expression);
  expr.statements = PushArenaList<CAST*>(arena);

  *ifAst->ifExpressions->PushElem() = expr;
}

void CEmitter::Else(){
  // NOTE: Kinda expect this to work but not sure. 
  while(buffer[top-1]->type != CASTType_IF){
    EndBlock();
  }

  CAST* ifAst = buffer[top-1];

  ifAst->elseStatements = PushArenaList<CAST*>(arena);
}

void CEmitter::EndBlock(){
  for(int i = top - 1; i >= 0; i--){
    if(IsBlockType(buffer[i]->type)){
      top = i - 1;
      break;
    }
  }

  // Is this possible?
  if(top < 0){
    top = 0;
  }
}

void CEmitter::Assignment(String lhs,String rhs){
  // All these functions must be able to be inserted inside the 
  CAST* assign = PushCAST(CASTType_ASSIGNMENT,arena);

  assign->lhs = PushString(arena,lhs);
  assign->rhs = PushString(arena,rhs);

  DEBUG_BREAK();
  InsertStatement(assign);
}

CEmitter* StartCCode(Arena* out){
  CEmitter* res = PushStruct<CEmitter>(out);

  res->arena = out;
  res->buffer = PushArray<CAST*>(out,99); // NOTE: Can always change to a dynamic array or something similar
  
  res->topLevel = PushCAST(CASTType_TOP_LEVEL,out);
  res->topLevel->functions = PushArenaList<CAST*>(out);

  return res;
}

CAST* EndCCode(CEmitter* m){
  while(m->top > 0){
    m->EndBlock();
  }
  
  return m->topLevel;
}

void Repr(CAST* top,StringBuilder* b,int level){
  switch(top->type){
  case CASTType_TOP_LEVEL: {
    for(SingleLink<CAST*>* iter = top->functions->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      Repr(elem,b);
      b->PushString("\n");
    }
  } break;
  case CASTType_FUNCTION: {
    b->PushString(top->returnType);
    b->PushString(" ");
    b->PushString(top->functionName);
    b->PushString("(");

    bool first = true; 
    for(SingleLink<CAST*>* iter = top->arguments->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      if(first){
        first = false;
      } else {
        b->PushString(", ");
      }

      Repr(elem,b);
    }

    b->PushString("){\n");
    
    for(SingleLink<CAST*>* iter = top->statements->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      Repr(elem,b,level + 1);
    }

    b->PushString("}");
    
  } break;
  case CASTType_VAR_DECL: {
    b->PushString("%.*s %.*s",UNPACK_SS(top->typeName),UNPACK_SS(top->varName));
  } break;
  case CASTType_IF: {
    SingleLink<CASTIf>* iter = top->ifExpressions->head;
    CASTIf ifExpr = iter->elem;
    
    b->PushSpaces(level * 2);
    b->PushString("if(%.*s){\n",UNPACK_SS(ifExpr.ifExpression));
    if(ifExpr.statements){
      for(SingleLink<CAST*>* iter = ifExpr.statements->head; iter; iter = iter->next){
        CAST* ast = iter->elem;

        Repr(ast,b,level + 1);
        b->PushString("\n");
      }
    }
    
    b->PushSpaces(level * 2);
    b->PushString("} ");
    
    for(SingleLink<CASTIf>* otherIter = iter->next; otherIter; otherIter = otherIter->next){
      CASTIf ifExpr = otherIter->elem;
      
      b->PushString("else if(%.*s){\n",UNPACK_SS(ifExpr.ifExpression));

      if(ifExpr.statements){
        for(SingleLink<CAST*>* subIter = ifExpr.statements->head; subIter; subIter = subIter->next){
          CAST* ast = subIter->elem;

          Repr(ast,b,level + 1);
          b->PushString("\n");
        }
      }
        
      b->PushSpaces(level * 2);
      b->PushString("} ");
    }

    if(top->elseStatements){
      b->PushString("else {\n");

      if(top->elseStatements){
        for(SingleLink<CAST*>* iter = top->elseStatements->head; iter; iter = iter->next){
          CAST* ast = iter->elem;

          Repr(ast,b,level + 1);
          b->PushString("\n");
        }
      }
      
      b->PushSpaces(level * 2);
      b->PushString("}");
    }

    b->PushString("\n");

  } break;
  case CASTType_ASSIGNMENT:{
    b->PushSpaces(level * 2);
    b->PushString("%.*s = %.*s;",UNPACK_SS(top->lhs),UNPACK_SS(top->rhs));
  } break;
  }
  
}

