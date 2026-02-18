#include "CEmitter.hpp"

String Repr(CASTType type){
  switch(type){
  case CASTType_TOP_LEVEL: return "CASTType_TOP_LEVEL";
  case CASTType_FUNCTION: return "CASTType_FUNCTION";
  case CASTType_ASSIGNMENT: return "CASTType_ASSIGNMENT";
  case CASTType_COMMENT: return "CASTType_COMMENT";
  case CASTType_STRUCT_DEF: return "CASTType_STRUCT_DEF";
  case CASTType_STATEMENT: return "CASTType_STATEMENT";
  case CASTType_MEMBER_DECL: return "CASTType_MEMBER_DECL";
  case CASTType_VAR_DECL: return "CASTType_VAR_DECL";
  case CASTType_VAR_DECL_STMT: return "CASTType_VAR_DECL_STMT";
  case CASTType_IF: return "CASTType_IF";
  }

  // TODO: Move this to meta
  NOT_IMPLEMENTED();
  return {};
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
    printf("Did not find CAST of type: %.*s\n",UN(Repr(type)));
    printf("Emitter stack currently has %d levels and is, from zero:\n",top);
    for(int i = 0; i < top; i++){
      printf("%.*s\n",UN(Repr(buffer[i]->type)));
    }
    DEBUG_BREAK_OR_EXIT();
  }
    
  return nullptr;
}

void CEmitter::InsertStatement(CAST* statementCAST){
  if(top == 0){
    *topLevel->top.declarations->PushElem() = statementCAST;
    return;
  }
  
  for(int i = top - 1; i >= 0; i--){
    CAST* cast = buffer[i];
    CASTType type = cast->type;
    
    FULL_SWITCH(type){
    case CASTType_TOP_LEVEL: Assert(false); // Cannot find top level inside this 

    // Non block types
    case CASTType_SWITCH_BLOCK:; // Not treated as a block, switch case are hardcoded to directly add to switches
    case CASTType_ENUM_DEF:;
    case CASTType_ELEM:;
    case CASTType_RAW_STATEMENT:;
    case CASTType_COMMENT:;
    case CASTType_ASSIGNMENT:;  
    case CASTType_VAR_DECL_STMT:; 
    case CASTType_STATEMENT:;
    case CASTType_MEMBER_DECL:;
    case CASTType_VAR_DECL: continue; 
      
    case CASTType_VAR_BLOCK:{
      *cast->varBlock.varElems->PushElem() = statementCAST;
      return;
    } break;

    case CASTType_VAR_DECL_BLOCK:{
      *cast->varDeclBlock.arrayElems->PushElem() = statementCAST;
      return;
    } break;
      
    case CASTType_CASE_BLOCK: {
      *cast->caseBlock.statements->PushElem() = statementCAST;
      return;
    } break;

    case CASTType_UNION_DEF: {
      *cast->structDef.declarations->PushElem() = statementCAST;
      return;
    } break;

    case CASTType_STRUCT_DEF: {
      *cast->structDef.declarations->PushElem() = statementCAST;
      return;
    } break;

    case CASTType_FOREACH_BLOCK: {
      *cast->foreachDecl.statements->PushElem() = statementCAST;
      return;
    } break;
      
    // All the "block" types
    case CASTType_IF: {
      if(cast->ifDecl.elseStatements){
        *cast->ifDecl.elseStatements->PushElem() = statementCAST;
      } else {
        *cast->ifDecl.ifExpressions->tail->elem.statements->PushElem() = statementCAST;
      }
      return;
    } break;

    case CASTType_FUNCTION: {
      *cast->funcDecl.statements->PushElem() = statementCAST;
      return;
    } break;
    } END_SWITCH();
  }
}

CExpr* FixTerm(Array<CExpr> expressions,int& index){
  CExpr* expr = &expressions[index];

  Assert(expr->type == CExprType_VAR || expr->type == CExprType_LITERAL);

  index += 1;
  return expr;
}

CExpr* FixBinaryOp(Array<CExpr> expressions,int& index,Arena* out){
  CExpr* current = FixTerm(expressions,index);

  if(index >= expressions.size){
    return current;
  }

  CExpr* op = &expressions[index];
  while(op->type == CExprType_OP && (op->op == CExprOp_GREATER || op->op == CExprOp_EQUALITY)){
    index += 1;
    CExpr* right = FixTerm(expressions,index);

    op->children = PushArray<CExpr*>(out,2);
    op->children[0] = current;
    op->children[1] = right;
    
    current = op;

    if(index < expressions.size){
      op = &expressions[index];
      continue;
    } else {
      break;
    }
  }

  return current;
}

CExpr* FixAnd(Array<CExpr> expressions,int& index,Arena* out){
  CExpr* current = FixBinaryOp(expressions,index,out);

  if(index >= expressions.size){
    return current;
  }
  
  CExpr* op = &expressions[index];
  while(op->type == CExprType_OP && op->op == CExprOp_AND){
    index += 1;
    CExpr* right = FixBinaryOp(expressions,index,out);

    op->children = PushArray<CExpr*>(out,2);
    op->children[0] = current;
    op->children[1] = right;
    
    current = op;

    if(index < expressions.size){
      op = &expressions[index];
      continue;
    } else {
      break;
    }
  }

  return current;
}


String CEmitter::EndExpressionAsString(Arena* out){
  Array<CExpr> expressions = PushArray(out,expressionList);

  int index = 0;
  CExpr* top = FixAnd(expressions,index,out);

  Assert(index == expressions.size);
  
  auto builder = StartString(out);
  
  auto Recurse = [builder](auto Recurse,CExpr* top) -> void{
    FULL_SWITCH(top->type){
    case CExprType_VAR:{
      builder->PushString("%.*s",UN(top->var));
    } break;
    case CExprType_LITERAL:{
      builder->PushString("%d",top->lit);
    } break;
    case CExprType_OP:{
      builder->PushString("(");
      
      Recurse(Recurse,top->children[0]);

      FULL_SWITCH(top->op){
      case CExprOp_AND:{
        builder->PushString(" && ");
      } break;
      case CExprOp_GREATER:{
        builder->PushString(" > ");
      } break;
      case CExprOp_EQUALITY:{
        builder->PushString(" == ");
      } break;
      } END_SWITCH();

      // NOTE: Assuming we only care about binary operators, 
      Recurse(Recurse,top->children[1]);
      builder->PushString(")");
    } break;
    } END_SWITCH();
  };

  Recurse(Recurse,top);

  String result = EndString(out,builder);
  this->expressionList = nullptr;
  return result;
}

void CEmitter::Struct(String structName){
  CAST* structAST = PushCAST(CASTType_STRUCT_DEF,arena);

  structAST->structDef.name = PushString(arena,structName);
  structAST->structDef.declarations = PushList<CAST*>(arena);
  
  InsertStatement(structAST);
  PushLevel(structAST);
}

void CEmitter::Union(String unionName){
  CAST* unionAST = PushCAST(CASTType_UNION_DEF,arena);

  unionAST->structDef.name = PushString(arena,unionName);
  unionAST->structDef.declarations = PushList<CAST*>(arena);
  
  InsertStatement(unionAST);
  PushLevel(unionAST);
}

void CEmitter::Member(String type,String memberName,int arraySize){
  CAST* argument = PushCAST(CASTType_MEMBER_DECL,arena);

  argument->varDecl.typeName = PushString(arena,type);
  argument->varDecl.varName = PushString(arena,memberName);
  argument->varDecl.arraySize = arraySize;
  
  InsertStatement(argument);
}

void CEmitter::EndStruct(){
  PopLevel();
}

void CEmitter::Enum(String name){
  CAST* enumAST = PushCAST(CASTType_ENUM_DEF,arena);

  enumAST->enumDef.name = PushString(arena,name);
  enumAST->enumDef.nameAndValue = PushList<Pair<String,String>>(arena);
  
  InsertStatement(enumAST);
  PushLevel(enumAST);
}

void CEmitter::EnumMember(String name,String value){
  CAST* enumDecl = FindFirstCASTType(CASTType_ENUM_DEF,false);

  *enumDecl->enumDef.nameAndValue->PushElem() = {PushString(arena,name),PushString(arena,value)};
}

void CEmitter::EndEnum(){
  PopLevel();
}

void CEmitter::Extern(String typeName,String name){
  CAST* externDecl = PushCAST(CASTType_VAR_DECL_STMT,arena);

  externDecl->varDecl.typeName = PushString(arena,typeName);
  externDecl->varDecl.varName = PushString(arena,name);
  externDecl->varDecl.isExtern = true;
  
  InsertStatement(externDecl);
}

void CEmitter::FunctionDeclOnlyBlock(String returnType,String functionName){
  // TODO: C does not support nested functions
  Assert(!FindFirstCASTType(CASTType_FUNCTION,false));

  CAST* function = PushCAST(CASTType_FUNCTION,arena);
  function->funcDecl.returnType = PushString(arena,returnType);
  function->funcDecl.functionName = PushString(arena,functionName);
  function->funcDecl.arguments = PushList<CAST*>(arena);
  function->funcDecl.statements = nullptr; // 
  
  InsertStatement(function);
  PushLevel(function);
}

void CEmitter::FunctionBlock(String returnType,String functionName){
  // TODO: C does not support nested functions
  Assert(!FindFirstCASTType(CASTType_FUNCTION,false));

  CAST* function = PushCAST(CASTType_FUNCTION,arena);
  function->funcDecl.returnType = PushString(arena,returnType);
  function->funcDecl.functionName = PushString(arena,functionName);
  function->funcDecl.arguments = PushList<CAST*>(arena);
  function->funcDecl.statements = PushList<CAST*>(arena);
  
  InsertStatement(function);

  PushLevel(function);
}

void CEmitter::Argument(String type,String name){
  CAST* function = FindFirstCASTType(CASTType_FUNCTION);
  
  CAST* argument = PushCAST(CASTType_VAR_DECL,arena);

  argument->varDecl.typeName = PushString(arena,type);
  argument->varDecl.varName = PushString(arena,name);
  
  *function->funcDecl.arguments->PushElem() = argument;
}

void CEmitter::VarDeclare(String type,String name,String initialValue){
  CAST* declaration = PushCAST(CASTType_VAR_DECL_STMT,arena);

  declaration->varDecl.typeName = PushString(arena,type);
  declaration->varDecl.varName = PushString(arena,name);
  declaration->varDecl.defaultValue = PushString(arena,initialValue);

  InsertStatement(declaration);
}

void CEmitter::VarDeclareBlock(String type,String name,bool isStatic){
  CAST* varBlock = PushCAST(CASTType_VAR_DECL_BLOCK,arena);

  varBlock->varDeclBlock.type = PushString(arena,type);
  varBlock->varDeclBlock.name = PushString(arena,name);
  varBlock->varDeclBlock.arrayElems = PushList<CAST*>(arena);
  varBlock->varDeclBlock.isStatic = isStatic;

  InsertStatement(varBlock);
  PushLevel(varBlock);
}

void CEmitter::ArrayDeclareBlock(String type,String name,bool isStatic){
  CAST* varBlock = PushCAST(CASTType_VAR_DECL_BLOCK,arena);

  varBlock->varDeclBlock.type = PushString(arena,type);
  varBlock->varDeclBlock.name = PushString(arena,name);
  varBlock->varDeclBlock.arrayElems = PushList<CAST*>(arena);
  varBlock->varDeclBlock.isStatic = isStatic;
  varBlock->varDeclBlock.isArray = true;
  
  InsertStatement(varBlock);
  PushLevel(varBlock);
}

void CEmitter::StringElem(String value){
  CAST* elem = PushCAST(CASTType_ELEM,arena);

  elem->content = PushString(arena,"String(\"%.*s\")",UN(value));
  
  InsertStatement(elem);
}

void CEmitter::ForEachBlock(String type,String iterName,String data){
  CAST* forBlock = PushCAST(CASTType_FOREACH_BLOCK,arena);

  forBlock->foreachDecl.type = PushString(arena,type);
  forBlock->foreachDecl.iterName = PushString(arena,iterName);
  forBlock->foreachDecl.data = PushString(arena,data);
  forBlock->foreachDecl.statements = PushList<CAST*>(arena);
  
  InsertStatement(forBlock);
  PushLevel(forBlock);
}

void CEmitter::VarBlock(){
  CAST* varBlock = PushCAST(CASTType_VAR_BLOCK,arena);

  varBlock->varBlock.varElems = PushList<CAST*>(arena);

  InsertStatement(varBlock);
  PushLevel(varBlock);
}

void CEmitter::Elem(String value){
  CAST* elem = PushCAST(CASTType_ELEM,arena);

  elem->content = PushString(arena,value);
  
  InsertStatement(elem);
}

void CEmitter::If(String expression){
  CAST* ifAst = PushCAST(CASTType_IF,arena);

  // Pushes the list for the expr+statement combo
  ifAst->ifDecl.ifExpressions = PushList<CASTIf>(arena);

  // Pushes the first expr+statement combo
  CASTIf expr = {};
  expr.ifExpression = PushString(arena,expression);
  expr.statements = PushList<CAST*>(arena);

  *ifAst->ifDecl.ifExpressions->PushElem() = expr;
  
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
  expr.statements = PushList<CAST*>(arena);

  *ifAst->ifDecl.ifExpressions->PushElem() = expr;
}

void CEmitter::Else(){
  // NOTE: Kinda expect this to work but not sure. 
  //       It might just be better to force user to call EndBlock everytime
  while(buffer[top-1]->type != CASTType_IF){
    EndBlock();
  }
  
  CAST* ifAst = buffer[top-1];

  // If the top 'if' already has an else, then we must look further up the chain
  if(ifAst->ifDecl.elseStatements){
    EndBlock();
    while(buffer[top-1]->type != CASTType_IF){
      EndBlock();
    }
    ifAst = buffer[top-1];
  }
  
  ifAst->ifDecl.elseStatements = PushList<CAST*>(arena);
}

void CEmitter::EndIf(){
  while(buffer[top-1]->type != CASTType_IF){
    EndBlock();
  }
  
  EndBlock();
}

void CEmitter::IfOrElseIf(String expression){
  if(buffer[top-1]->type == CASTType_IF){
    ElseIf(expression);
  } else {
    If(expression);
  }
}

void CEmitter::IfFromExpression(){
  TEMP_REGION(temp,arena);
  String content = EndExpressionAsString(temp);
  If(content);
}

void CEmitter::ElseIfFromExpression(){
  TEMP_REGION(temp,arena);
  String content = EndExpressionAsString(temp);
  ElseIf(content);
}

void CEmitter::IfOrElseIfFromExpression(){
  TEMP_REGION(temp,arena);
  String content = EndExpressionAsString(temp);
  IfOrElseIf(content);
}

void CEmitter::StartExpression(){
  Assert2(expressionList == nullptr,"Already inside an expression, likely an error");
  
  expressionList = PushList<CExpr>(arena);
}

void CEmitter::Var(String name){
  CExpr* expr = expressionList->PushElem();
  expr->type = CExprType_VAR;
  expr->var = name;
}

void CEmitter::Literal(int val){
  CExpr* expr = expressionList->PushElem();
  expr->type = CExprType_LITERAL;
  expr->lit = val;
}

void CEmitter::IsEqual(){
  CExpr* expr = expressionList->PushElem();
  expr->type = CExprType_OP;
  expr->op = CExprOp_EQUALITY;
}

void CEmitter::And(){
  CExpr* expr = expressionList->PushElem();
  expr->type = CExprType_OP;
  expr->op = CExprOp_AND;
}

void CEmitter::GreaterThan(){
  CExpr* expr = expressionList->PushElem();
  expr->type = CExprType_OP;
  expr->op = CExprOp_GREATER;
}

void CEmitter::SwitchBlock(String switchExpr){
  CAST* switchBlock = PushCAST(CASTType_SWITCH_BLOCK,arena);

  switchBlock->switchBlock.expr = PushString(arena,switchExpr);
  switchBlock->switchBlock.cases = PushList<CAST*>(arena);  
  
  InsertStatement(switchBlock);
  PushLevel(switchBlock);
}

void CEmitter::CaseBlock(String caseExpr){
  CAST* caseBlock = PushCAST(CASTType_CASE_BLOCK,arena);

  caseBlock->caseBlock.caseExpr = PushString(arena,caseExpr);
  caseBlock->caseBlock.statements = PushList<CAST*>(arena);
  
  CAST* switchBlock = FindFirstCASTType(CASTType_SWITCH_BLOCK);

  *switchBlock->switchBlock.cases->PushElem() = caseBlock;
  PushLevel(caseBlock);
}

void CEmitter::EndBlock(){
  top -= 1;

  // Is this possible?
  if(top < 0){
    top = 0;
  }
}

void CEmitter::Once(){
  topLevel->top.oncePragma = true;
}

void CEmitter::Include(String filename){
  CAST* include = PushCAST(CASTType_RAW_STATEMENT,arena);

  include->rawData = PushString(arena,"#include \"%.*s\"",UN(filename));
  InsertStatement(include);
}

void CEmitter::Line(){
  CAST* emptyLine = PushCAST(CASTType_RAW_STATEMENT,arena);

  emptyLine->rawData = {};
  
  InsertStatement(emptyLine);
}

void CEmitter::RawLine(String val){
  CAST* rawLine = PushCAST(CASTType_RAW_STATEMENT,arena);

  rawLine->rawData = PushString(arena,val);
  
  InsertStatement(rawLine);
}

void CEmitter::Define(String name){
  CAST* emptyLine = PushCAST(CASTType_RAW_STATEMENT,arena);

  emptyLine->rawData = PushString(arena,"#define %.*s",UN(name));
  
  InsertStatement(emptyLine);
}

void CEmitter::Define(String name,String content){
  CAST* emptyLine = PushCAST(CASTType_RAW_STATEMENT,arena);

  emptyLine->rawData = PushString(arena,"#define %.*s %.*s",UN(name),UN(content));
  
  InsertStatement(emptyLine);
}

void CEmitter::Comment(String comment){
  CAST* commentAst = PushCAST(CASTType_COMMENT,arena);
  commentAst->comment = PushString(arena,comment);

  InsertStatement(commentAst);
}

void CEmitter::Statement(String statement){
  CAST* stmt = PushCAST(CASTType_STATEMENT,arena);

  stmt->simpleStatement = PushString(arena,statement);
  InsertStatement(stmt);
}

void CEmitter::Assignment(String lhs,String rhs){
  // All these functions must be able to be inserted inside the 
  CAST* assign = PushCAST(CASTType_ASSIGNMENT,arena);

  assign->assign.lhs = PushString(arena,lhs);
  assign->assign.rhs = PushString(arena,rhs);

  InsertStatement(assign);
}

void CEmitter::Return(String varToReturn){
  if(Empty(varToReturn)){
    Statement("return");
  } else {
    TEMP_REGION(temp,arena);
    
    String stmt = PushString(temp,"return %.*s",UN(varToReturn));
    Statement(stmt);
  }
}

CEmitter* StartCCode(Arena* freeArena){
  CEmitter* res = PushStruct<CEmitter>(freeArena);

  res->arena = freeArena;
  res->buffer = PushArray<CAST*>(freeArena,16); // NOTE: Can always change to a dynamic array or something similar
  
  res->topLevel = PushCAST(CASTType_TOP_LEVEL,freeArena);
  res->topLevel->top.declarations = PushList<CAST*>(freeArena);
  
  return res;
}

CAST* EndCCode(CEmitter* m){
  while(m->top > 0){
    m->EndBlock();
  }
  
  return m->topLevel;
}

String PushASTRepr(CEmitter* e,Arena* out,bool cppStyle,int level){
  TEMP_REGION(temp,out);
  CAST* ast = EndCCode(e);
  StringBuilder* b = StartString(temp);

  Repr(ast,b,cppStyle,level);

  return EndString(out,b);
}

void Repr(CAST* top,StringBuilder* b,bool cppStyle,int level){
  FULL_SWITCH(top->type){
  case CASTType_TOP_LEVEL: {
    if(top->top.oncePragma){
      b->PushString("#pragma once\n\n");
    }

    bool first = true;
    for(SingleLink<CAST*>* iter = top->top.declarations->head; iter; iter = iter->next){
      // Only new line if we have more than one declaration
      if(!first){
        b->PushString("\n");
      }
      first = false;
      
      CAST* elem = iter->elem;

      Repr(elem,b,cppStyle,level);
    }
  } break;

  case CASTType_ENUM_DEF:{
    b->PushSpaces(level * 2);

    if(cppStyle){
      b->PushString("enum %.*s{\n",UN(top->enumDef.name));
    } else {
      b->PushString("typedef enum{\n");
    }

    bool first = true;
    for(auto p : top->enumDef.nameAndValue){
      if(!first){
        b->PushString(",\n");
      }
      
      b->PushSpaces((level + 1) * 2);

      if(Empty(p.second)){
        b->PushString("%.*s",UN(p.first));
      } else {
        b->PushString("%.*s = %.*s",UN(p.first),UN(p.second));
      }
      
      first = false;
    }

    b->PushString("\n");
    b->PushSpaces(level * 2);
    if(cppStyle){
      b->PushString("};");
    } else {
      b->PushString("} %.*s;",UN(top->enumDef.name));
    }
  } break;
  
  case CASTType_FUNCTION: {
    b->PushString(top->funcDecl.returnType);
    b->PushString(" ");
    b->PushString(top->funcDecl.functionName);
    b->PushString("(");

    bool first = true; 
    for(SingleLink<CAST*>* iter = top->funcDecl.arguments->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      if(first){
        first = false;
      } else {
        b->PushString(",");
      }

      Repr(elem,b,cppStyle);
    }

    if(top->funcDecl.statements == nullptr){
      b->PushString(");");
    } else {
      b->PushString("){\n");
    
      for(SingleLink<CAST*>* iter = top->funcDecl.statements->head; iter; iter = iter->next){
        CAST* elem = iter->elem;

        Repr(elem,b,cppStyle,level + 1);
        b->PushString("\n");
      }

      b->PushString("}");
    }    
  } break;
  case CASTType_VAR_DECL: {
    b->PushString("%.*s %.*s",UN(top->varDecl.typeName),UN(top->varDecl.varName));
  } break;
  case CASTType_MEMBER_DECL:{
    b->PushSpaces(level * 2);
    if(top->varDecl.arraySize > 0){
      b->PushString("%.*s %.*s[%d];",UN(top->varDecl.typeName),UN(top->varDecl.varName),top->varDecl.arraySize);
    } else {
      b->PushString("%.*s %.*s;",UN(top->varDecl.typeName),UN(top->varDecl.varName));
    }
  } break;
  case CASTType_VAR_DECL_STMT: {
    b->PushSpaces(level * 2);
    if(top->varDecl.isExtern){
      b->PushString("extern ");
    }
    if(Empty(top->varDecl.defaultValue)){
      b->PushString("%.*s %.*s;",UN(top->varDecl.typeName),UN(top->varDecl.varName));
    } else {
      b->PushString("%.*s %.*s = %.*s;",UN(top->varDecl.typeName),UN(top->varDecl.varName),UN(top->varDecl.defaultValue));
    }
  } break;
  case CASTType_COMMENT:{
    bool isSingleLine = true;
    for(char ch : top->comment){
      if(ch == '\n'){
        isSingleLine = false;
        break;
      }
    }
    
    if(isSingleLine){
      b->PushSpaces(level * 2);
      b->PushString("// %.*s",UN(top->comment));
    } else {
      TEMP_REGION(temp,b->arena);
      
      b->PushSpaces(level * 2);
      b->PushString("/*\n");
      Array<String> lines = Split(top->comment,'\n',temp);

      for(String line : lines){
        b->PushSpaces((level + 1) * 2);
        b->PushString(line);
        b->PushString("\n");
      }
      
      b->PushSpaces(level * 2);
      b->PushString("*/");
    }
  } break;
  case CASTType_FOREACH_BLOCK: {
    b->PushSpaces(level * 2);

    b->PushString("for(%.*s %.*s : %.*s){\n",UN(top->foreachDecl.type),UN(top->foreachDecl.iterName),UN(top->foreachDecl.data));
    for(SingleLink<CAST*>* subIter = top->foreachDecl.statements->head; subIter; subIter = subIter->next){
      CAST* ast = subIter->elem;

      Repr(ast,b,cppStyle,level + 1);
      b->PushString("\n");
    }
    b->PushSpaces(level * 2);
    b->PushString("}");
  } break;
  case CASTType_IF: {
    SingleLink<CASTIf>* iter = top->ifDecl.ifExpressions->head;
    CASTIf ifExpr = iter->elem;
    
    b->PushSpaces(level * 2);
    b->PushString("if(%.*s){\n",UN(ifExpr.ifExpression));
    if(ifExpr.statements){
      for(SingleLink<CAST*>* iter = ifExpr.statements->head; iter; iter = iter->next){
        CAST* ast = iter->elem;

        Repr(ast,b,cppStyle,level + 1);
        b->PushString("\n");
      }
    }
    
    b->PushSpaces(level * 2);
    b->PushString("} ");
    
    for(SingleLink<CASTIf>* otherIter = iter->next; otherIter; otherIter = otherIter->next){
      CASTIf ifExpr = otherIter->elem;
      
      b->PushString("else if(%.*s){\n",UN(ifExpr.ifExpression));

      if(ifExpr.statements){
        for(SingleLink<CAST*>* subIter = ifExpr.statements->head; subIter; subIter = subIter->next){
          CAST* ast = subIter->elem;

          Repr(ast,b,cppStyle,level + 1);
          b->PushString("\n");
        }
      }
        
      b->PushSpaces(level * 2);
      b->PushString("} ");
    }

    if(top->ifDecl.elseStatements){
      b->PushString("else {\n");

      if(top->ifDecl.elseStatements){
        for(SingleLink<CAST*>* iter = top->ifDecl.elseStatements->head; iter; iter = iter->next){
          CAST* ast = iter->elem;

          Repr(ast,b,cppStyle,level + 1);
          b->PushString("\n");
        }
      }
      
      b->PushSpaces(level * 2);
      b->PushString("}");
    }
  } break;
  case CASTType_STRUCT_DEF:{
    b->PushSpaces(level * 2);

    if(cppStyle || Empty(top->structDef.name)){
      b->PushString("struct %.*s{\n",UN(top->structDef.name));
    } else {
      b->PushString("typedef struct{\n");
    }
    
    for(SingleLink<CAST*>* iter = top->structDef.declarations->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      Repr(elem,b,cppStyle,level + 1);
      b->PushString("\n");
    }
    b->PushSpaces(level * 2);

    if(cppStyle){
      b->PushString("};");
    } else {
      b->PushString("} %.*s;",UN(top->structDef.name));
    }
  } break;

  case CASTType_UNION_DEF:{
    b->PushSpaces(level * 2);

    if(cppStyle || Empty(top->structDef.name)){
      b->PushString("union %.*s{\n",UN(top->structDef.name));
    } else {
      b->PushString("typedef union{\n");
    }
    
    for(SingleLink<CAST*>* iter = top->structDef.declarations->head; iter; iter = iter->next){
      CAST* elem = iter->elem;

      Repr(elem,b,cppStyle,level + 1);
      b->PushString("\n");
    }
    b->PushSpaces(level * 2);

    if(cppStyle){
      b->PushString("};");
    } else {
      b->PushString("} %.*s;",UN(top->structDef.name));
    }
  } break;
  
  case CASTType_STATEMENT:{
    b->PushSpaces(level * 2);
    b->PushString("%.*s;",UN(top->simpleStatement));
  } break;

  case CASTType_VAR_DECL_BLOCK:{
    if(top->varDeclBlock.isStatic){
      b->PushString("static ");
    }
    b->PushString("%.*s %.*s",UN(top->varDeclBlock.type),UN(top->varDeclBlock.name));

    if(top->varDeclBlock.isArray){
      b->PushString("[] = {");
    } else {
      b->PushString(" = {");
    }
    
    bool first = true;
    for(CAST* ast : top->varDeclBlock.arrayElems){
      if(!first){
        if(top->varDeclBlock.isArray){
          b->PushString(",");
        } else {
          b->PushString(",");
        }
      }
      Repr(ast,b,cppStyle,level + 1);
      first = false;
    }
    b->PushString("};");
  } break;

  case CASTType_VAR_BLOCK:{
    b->PushSpaces(level * 2);

    b->PushString("{");
    bool first = true;
    for(CAST* ast : top->varBlock.varElems){
      if(!first){
        b->PushString(",");
      }
      Repr(ast,b,cppStyle,level);
      first = false;
    }

    b->PushString("}");
  } break;

  case CASTType_SWITCH_BLOCK:{
    b->PushSpaces(level * 2);

    b->PushString("switch(%.*s){\n",UN(top->switchBlock.expr));
    for(CAST* ast : top->switchBlock.cases){
      Repr(ast,b,cppStyle,level + 1);
    }

    b->PushSpaces(level * 2);
    b->PushString("}");
  } break;

  case CASTType_CASE_BLOCK:{
    b->PushSpaces(level * 2);
    b->PushString("case %.*s : {\n",UN(top->caseBlock.caseExpr));

    for(CAST* ast : top->caseBlock.statements){
      Repr(ast,b,cppStyle,level + 1);
      b->PushString("\n");
    }
    
    b->PushSpaces(level * 2);
    b->PushString("} break;\n");
  } break;
  
  case CASTType_ELEM:{
    b->PushString("%.*s",UN(top->content));
  } break;
    
  case CASTType_RAW_STATEMENT:{
    b->PushSpaces(level * 2);
    b->PushString("%.*s",UN(top->rawData));
  } break;

  case CASTType_ASSIGNMENT:{
    b->PushSpaces(level * 2);
    b->PushString("%.*s = %.*s;",UN(top->assign.lhs),UN(top->assign.rhs));
  } break;
  } END_SWITCH();
}

