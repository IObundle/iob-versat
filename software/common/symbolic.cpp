#include "symbolic.hpp"
//#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

static TokenizerTemplate* tmpl;

int TypeToBindingStrength(SymbolicExpression* expr){
  switch(expr->type){
  case SymbolicExpressionType_VARIABLE:
  case SymbolicExpressionType_LITERAL:
    return 1;
  case SymbolicExpressionType_SUM:
    return 2;
  case SymbolicExpressionType_MUL:
    return 3;
  case SymbolicExpressionType_DIV:{
    return 4;
  case SymbolicExpressionType_FUNC:{
    return 5;
  } break;
  } break;
  }
  NOT_POSSIBLE();
}

static char GetOperationSymbol(SymbolicExpression* expr){
  switch(expr->type){
  case SymbolicExpressionType_FUNC: Assert(false);
  case SymbolicExpressionType_SUM: return '+';
  case SymbolicExpressionType_MUL: return '*';
  case SymbolicExpressionType_DIV: return '/';
  default: return ' ';
  }
}

void PushSymbolicRepr(ArenaList<SymbolicReprAtom>* builder,int literal){
  SymbolicReprAtom* atom = builder->PushElem();
  atom->type = SymbolicReprType_LITERAL;
  atom->literal = literal;
};

void PushSymbolicRepr(ArenaList<SymbolicReprAtom>* builder,String variable){
  SymbolicReprAtom* atom = builder->PushElem();
  atom->type = SymbolicReprType_VARIABLE;
  atom->variable = variable;
};

void PushSymbolicRepr(ArenaList<SymbolicReprAtom>* builder,char op){
  SymbolicReprAtom* atom = builder->PushElem();
  atom->type = SymbolicReprType_OP;
  atom->op = op;
};

void CompileRepresentationRecursive(ArenaList<SymbolicReprAtom>* b,SymbolicExpression* expr,bool top,int parentBindingStrength){
  if(expr->negative){
    PushSymbolicRepr(b,'-');
  }

  int bindingStrength = TypeToBindingStrength(expr);
  bool bind = (parentBindingStrength >= bindingStrength);

  //bind = true;
  switch(expr->type){
  case SymbolicExpressionType_FUNC: {
    PushSymbolicRepr(b,'(');

    bool first = true;
    for(SymbolicExpression* terms : expr->terms){
      if(first){
        first = false;
      } else {
        PushSymbolicRepr(b,',');
      }
      CompileRepresentationRecursive(b,terms,false,0);
    }
    PushSymbolicRepr(b,')');

  } break;
  case SymbolicExpressionType_VARIABLE: {
    PushSymbolicRepr(b,expr->variable);
  } break;
  case SymbolicExpressionType_LITERAL: {
    PushSymbolicRepr(b,expr->literal);
  } break;
  case SymbolicExpressionType_DIV: {
    if(expr->negative){
      bind = true;
    }
    
    bool hasNonOpSon = (expr->top->type != SymbolicExpressionType_DIV && expr->bottom->type != SymbolicExpressionType_DIV);
    if(hasNonOpSon && !top && bind){
      PushSymbolicRepr(b,'(');
    }

    CompileRepresentationRecursive(b,expr->top,false,bindingStrength);
    PushSymbolicRepr(b,'/');

    CompileRepresentationRecursive(b,expr->bottom,false,bindingStrength);
    if(hasNonOpSon && !top && bind){
      PushSymbolicRepr(b,')');
    }
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
  //case SymbolicExpressionType_ARRAY: {
    if(expr->negative){
      bind = true;
    }

    if(bind){
      PushSymbolicRepr(b,'(');
    }

    bool first = true;
    for(SymbolicExpression* s : expr->terms){
      if(!first){
        if(s->negative && expr->type == SymbolicExpressionType_SUM){
          //builder.PushString("-"); // Do not print anything  because we already printed the '-'
        } else {
          PushSymbolicRepr(b,GetOperationSymbol(expr));
        }
      } else {
        //if(s->negative && expr->op == '+'){
        //  builder.PushString("-");
        //}
      }
      CompileRepresentationRecursive(b,s,false,bindingStrength);
      first = false;
    }
    if(bind){
      PushSymbolicRepr(b,')');
    }
  } break;
  }
}

Array<SymbolicReprAtom> CompileRepresentation(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<SymbolicReprAtom>(temp);
  
  CompileRepresentationRecursive(list,expr,true,0);
  return PushArray(out,list);
}

static void BuildRepresentation(StringBuilder* builder,SymbolicExpression* expr,bool top,int parentBindingStrength){
  if(expr->negative){
    builder->PushString("-");
  }

  int bindingStrength = TypeToBindingStrength(expr);
  bool bind = (parentBindingStrength >= bindingStrength);

  //bind = true;
  switch(expr->type){
  case SymbolicExpressionType_FUNC: {
    builder->PushString("%.*s(",UN(expr->name));

    bool first = true;
    for(SymbolicExpression* terms : expr->terms){
      if(first){
        first = false;
      } else {
        builder->PushChar(',');
      }
      BuildRepresentation(builder,terms,false,0);
    }
    builder->PushString(")");
  } break;
  case SymbolicExpressionType_VARIABLE: {
    builder->PushString("%.*s",UN(expr->variable));
  } break;
  case SymbolicExpressionType_LITERAL: {
    builder->PushString("%d",expr->literal);
  } break;
  case SymbolicExpressionType_DIV: {
    if(expr->negative){
      bind = true;
    }
    
    bool hasNonOpSon = (expr->top->type != SymbolicExpressionType_DIV && expr->bottom->type != SymbolicExpressionType_DIV);
    if(hasNonOpSon && !top && bind){
      builder->PushString("(");
    }

    BuildRepresentation(builder,expr->top,false,bindingStrength);

    builder->PushString("/");
    
    BuildRepresentation(builder,expr->bottom,false,bindingStrength);
    if(hasNonOpSon && !top && bind){
      builder->PushString(")");
    }
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
  //case SymbolicExpressionType_ARRAY: {
    if(expr->negative){
      bind = true;
    }

    if(bind){
      builder->PushString("(");
    }

    bool first = true;
    for(SymbolicExpression* s : expr->terms){
      if(!first){
        if(s->negative && expr->type == SymbolicExpressionType_SUM){
          //builder.PushString("-"); // Do not print anything  because we already printed the '-'
        } else {
          builder->PushString("%c",GetOperationSymbol(expr));
        }
      } else {
        //if(s->negative && expr->op == '+'){
        //  builder.PushString("-");
        //}
      }
      BuildRepresentation(builder,s,false,bindingStrength);
      first = false;
    }
    if(bind){
      builder->PushString(")");
    }
  } break;
  }
}

String PushRepr(Arena* out,SymbolicExpression* expr){
  TEMP_REGION(temp,out);
  
  auto builder = StartString(temp);
  BuildRepresentation(builder,expr,true,0);
  return EndString(out,builder);
}

void Print(SymbolicExpression* expr,bool printNewLine){
  TEMP_REGION(temp,nullptr);
  
  String res = PushRepr(temp,expr);
  printf("%.*s",UN(res));

  if(printNewLine){
    printf("\n");
  }
}

char* DebugRepr(SymbolicExpression* expr){
  TEMP_REGION(temp,nullptr);

  String str = PushRepr(temp,expr);

  return SF("%.*s",UN(str));
}

void Repr(StringBuilder* builder,SymbolicExpression* expr){
  BuildRepresentation(builder,expr,true,0);
}

static void PrintAST(SymbolicExpression* expr,int level = 0){
  for(int i = 0; i < level; i++){
    printf("  ");
  }

  if(expr->negative){
    printf("NEG ");
  }
    
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:{
    printf("LIT: %d",expr->literal);
  } break;
  case SymbolicExpressionType_VARIABLE: {
    printf("VAR: %.*s",UN(expr->variable));
  } break;
  case SymbolicExpressionType_FUNC: {
    printf("FUNC: %.*s {\n",UN(expr->name));
    for(SymbolicExpression* child : expr->terms){
      PrintAST(child,level + 1);
      printf("\n");
    }
    
    for(int i = 0; i < level; i++){
      printf("  ");
    }
    printf("}");
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    printf("ARR: %c {\n",GetOperationSymbol(expr));

    for(SymbolicExpression* child : expr->terms){
      PrintAST(child,level + 1);
      printf("\n");
    }
    
    for(int i = 0; i < level; i++){
      printf("  ");
    }
    printf("}");
  } break;
  case SymbolicExpressionType_DIV:{
    printf("OP: DIV\n");

    PrintAST(expr->top,level + 1);
    PrintAST(expr->bottom,level + 1);
  } break;
  }

  if(level == 0){
    printf("\n"); // Assuming that no one wants to print after an entire ast
  }
}

int Evaluate(SymbolicExpression* expr,Hashmap<String,int>* values){
  int val = 0;
  switch(expr->type){
  case SymbolicExpressionType_FUNC: {
    //WARN_CODE();

    return 0;
  } break;
  case SymbolicExpressionType_LITERAL:{
    val = expr->literal;
  } break;
  case SymbolicExpressionType_VARIABLE:{
    val = values->GetOrFail(expr->variable);
  } break;
  case SymbolicExpressionType_DIV:{
    int left = Evaluate(expr->top,values);
    int right = Evaluate(expr->bottom,values);

    if(right == 0){
      PRINTF_WITH_LOCATION("Division by zero\n");
      DEBUG_BREAK_OR_EXIT();
      return 0;
    }

    val = (left / right);
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL:{
    bool isMul = (expr->type == SymbolicExpressionType_MUL);
    int value = isMul ? 1 : 0;

    for(SymbolicExpression* child : expr->terms){
      int subVal = Evaluate(child,values);

      if(isMul){
        value *= subVal;
      } else {
        value += subVal;
      }
    }

    val = value;
  } break;
  }

  if(expr->negative){
    val = -val;
  }

  return val;
}

#define EXPECT(TOKENIZER,STR) \
  TOKENIZER->AssertNextToken(STR)

static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out);

SymbolicExpression* PushLiteral(Arena* out,int value,bool negate){
  SymbolicExpression* literal = PushStruct<SymbolicExpression>(out);
  literal->type = SymbolicExpressionType_LITERAL;

  if(value < 0){
    value = -value;
    negate = !negate;
  }

  literal->literal = value;
  literal->negative = negate;
  return literal;
}

SymbolicExpression* PushVariable(Arena* out,String name,bool negate){
  SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);
  *expr = {};
  expr->type = SymbolicExpressionType_VARIABLE;
  expr->variable = PushString(out,name);
  expr->negative = negate;

  return expr;
}

SymbolicExpression* PushMulBase(Arena* out,bool negate = false){
  SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);
  *expr = {};
  expr->type = SymbolicExpressionType_MUL;
  expr->negative = negate;

  return expr;
}

SymbolicExpression* PushAddBase(Arena* out,bool negate = false){
  SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);
  *expr = {};
  expr->type = SymbolicExpressionType_SUM;
  expr->negative = negate;

  return expr;
}

int GetLiteralValue(SymbolicExpression* expr){
  Assert(expr->type == SymbolicExpressionType_LITERAL);
  Assert(expr->literal >= 0);
  
  // TODO: All literals should not be zero and negative at the same time.
  //       For now we just assert everytime we check a literal, we are bound to catch all zones of code where a negative - negative literal is ever built.
  //Assert(!expr->negative || expr->literal != 0);
  
  if(expr->negative){
    return -expr->literal;
  } else {
    return expr->literal;
  }
}

bool IsZero(SymbolicExpression* expr){
  if(expr->type == SymbolicExpressionType_LITERAL && expr->literal == 0){
    return true;
  }

  return false;
}

SymbolicExpression* SymbolicAdd(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  // TODO: We are always creating a new node even when it might be possible not to.
  //       Assuming that the normalization function is capable of collapsing everything into one, this should not pose a problem, as long as we normalize the end result, but not sure and nevertheless this is a bit slower.
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = PushArray<SymbolicExpression*>(out,2);

  res->terms[0] = left;
  res->terms[1] = right;

  return res;
}

SymbolicExpression* SymbolicAdd(Array<SymbolicExpression*> terms,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = CopyArray(terms,out);
  
  return res;
}

SymbolicExpression* SymbolicAdd(Array<SymbolicExpression*> terms,SymbolicExpression* extra,Arena* out){
  if(terms.size == 0){
    return SymbolicDeepCopy(extra,out);
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = PushArray<SymbolicExpression*>(out,terms.size + 1);

  Memcpy(res->terms,terms);
  res->terms[terms.size] = SymbolicDeepCopy(extra,out);

  return res;
}

SymbolicExpression* SymbolicSub(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  // TODO: We are always creating a new node even when it might be possible not to.
  //       Assuming that the normalization function is capable of collapsing everything into one, this should not pose a problem, as long as we normalize the end result, but not sure and nevertheless this is a bit slower.
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = PushArray<SymbolicExpression*>(out,2);

  SymbolicExpression* actualRight = SymbolicDeepCopy(right,out);
  actualRight->negative = !actualRight->negative;
  
  res->terms[0] = left;
  res->terms[1] = actualRight;

  return res;
}

SymbolicExpression* SymbolicMult(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->terms = PushArray<SymbolicExpression*>(out,2);

  res->terms[0] = left;
  res->terms[1] = right;

  return res;
}

SymbolicExpression* SymbolicMult(Array<SymbolicExpression*> terms,Arena* out){
  // TODO: Cannot make my mind whether terms.size == 0 is handled here or not.
  //       Only found cases where it is easier to handle here because 1 is mult identity but any case where this makes it harder and change to the assert and let higher code deal with this.
  if(terms.size == 0){
    return PushLiteral(out,1);
  }
  //Assert(terms.size > 0); // We do not handle zero terms at this level.

  if(terms.size == 1){
    return SymbolicDeepCopy(terms[0],out);
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->terms = CopyArray(terms,out);
  
  return res;
}

SymbolicExpression* SymbolicMult(Array<SymbolicExpression*> terms,SymbolicExpression* extra,Arena* out){
  if(terms.size == 0){
    return SymbolicDeepCopy(extra,out);
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->terms = PushArray<SymbolicExpression*>(out,terms.size + 1);

  Memcpy(res->terms,terms);
  res->terms[terms.size] = SymbolicDeepCopy(extra,out);

  return res;
}

SymbolicExpression* SymbolicDiv(SymbolicExpression* top,SymbolicExpression* bottom,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_DIV;

  res->top = top;
  res->bottom = bottom;

  return res;
}

SymbolicExpression* SymbolicFunc(String functionName,Array<SymbolicExpression*> terms,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_FUNC;
  res->name = PushString(out,functionName);
  res->terms = CopyArray(terms,out);
  
  return res;
}
  
static SymbolicExpression* ParseTerm(Tokenizer* tok,Arena* out){
  Token token = tok->NextToken();
  
  bool negative = false;
  if(CompareString(token,"-")){
    negative = true;
    token = tok->NextToken();
  }

  if(CompareString(token,"(")){
    SymbolicExpression* exprInside = ParseExpression(tok,out);

    EXPECT(tok,")");

    exprInside->negative = (exprInside->negative != negative);
    return exprInside;
  } else {
    if(token.size > 0 && IsNum(token[0])){
      return PushLiteral(out,ParseInt(token),negative);
    } else {
      Token peek = tok->PeekToken();
      if(CompareString(peek,"(")){
        // Function definition.
        tok->AdvancePeek();

        TEMP_REGION(temp,out);
        
        auto argList = PushList<SymbolicExpression*>(temp);
        while(!tok->Done()){
          SymbolicExpression* argument = ParseExpression(tok,out);
          *argList->PushElem() = argument;
          
          if(tok->IfPeekToken(")")){
            break;
          }

          tok->IfNextToken(",");
        }
        tok->AssertNextToken(")");

        SymbolicExpression* func = PushStruct<SymbolicExpression>(out);
        func->type = SymbolicExpressionType_FUNC;
        func->name = PushString(out,token);
        func->terms = PushArray(out,argList);
        func->negative = negative;

        return func;
      } else {
        SymbolicExpression* variable = PushStruct<SymbolicExpression>(out);
        variable->type = SymbolicExpressionType_VARIABLE;
        variable->variable = PushString(out,token);
        variable->negative = negative;
        return variable;
      }
    }
  }

  NOT_POSSIBLE();
}

static SymbolicExpression* ParseDiv(Tokenizer* tok,Arena* out){
  SymbolicExpression* left = ParseTerm(tok,out);

  SymbolicExpression* current = left;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    char op = '\0';
    if(CompareString(peek,"/")){
      op = '/';
    }

    if(op == '\0'){
      break;
    }
    
    tok->AdvancePeek();
      
    SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);

    expr->type = SymbolicExpressionType_DIV;
    expr->top = current;
    expr->bottom = ParseTerm(tok,out);

    current = expr;
  }

  return current;
}

static SymbolicExpression* ParseMul(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseDiv(tok,out);
  
  ArenaList<SymbolicExpression*>* expressions = PushList<SymbolicExpression*>(temp);
  *expressions->PushElem() = left;

  while(!tok->Done()){
    Token peek = tok->PeekToken();
    
    char op = '\0';
    if(CompareString(peek,"*")){
      op = '*';
    }

    if(op == '\0'){
      break;
    }
    
    tok->AdvancePeek();
    
    SymbolicExpression* expr = ParseDiv(tok,out);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->terms = PushArray(out,expressions);

  return res;
}

static SymbolicExpression* ParseSum(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseMul(tok,out);

  ArenaList<SymbolicExpression*>* expressions = PushList<SymbolicExpression*>(temp);
  *expressions->PushElem() = left;

  while(!tok->Done()){
    Token peek = tok->PeekToken();

    bool negative = false;
    char op = '\0';
    if(CompareString(peek,"+")){
      op = '+';
    } else if(CompareString(peek,"-")){
      negative = true;
      op = '+';
    }

    if(op == '\0'){
      break;
    }
    
    tok->AdvancePeek();
      
    SymbolicExpression* expr = ParseMul(tok,out);
    expr->negative = (expr->negative != negative);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = PushArray(out,expressions);

  return res;
}
  
static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out){
  return ParseSum(tok,out);
}

SymbolicExpression* ParseExpression(Array<Token> tokens,int& index,Arena* out);

static SymbolicExpression* ParseTerm(Array<Token> tokens,int& index,Arena* out){
  Token token = tokens[index++];
  
  bool negative = false;
  if(CompareString(token,"-")){
    negative = true;
    token = tokens[index++];
  }

  if(CompareString(token,"(")){
    SymbolicExpression* exprInside = ParseExpression(tokens,index,out);
    
    Assert(tokens[index++] == ")");

    exprInside->negative = (exprInside->negative != negative);
    return exprInside;
  } else {
    if(token.size > 0 && IsNum(token[0])){
      return PushLiteral(out,ParseInt(token),negative);
    } 

    if(index < tokens.size && tokens[index] == "("){
      // Function definition.
      index += 1;

      TEMP_REGION(temp,out);
        
      auto argList = PushList<SymbolicExpression*>(temp);
      while(index < tokens.size){
        SymbolicExpression* argument = ParseExpression(tokens,index,out);
        *argList->PushElem() = argument;
          
        if(tokens[index] == ")"){
          break;
        }

        if(tokens[index] == ","){
          index += 1;
        }
      }
      Assert(tokens[index++] == ")");

      SymbolicExpression* func = PushStruct<SymbolicExpression>(out);
      func->type = SymbolicExpressionType_FUNC;
      func->name = PushString(out,token);
      func->terms = PushArray(out,argList);
      func->negative = negative;

      return func;
    } else {
      SymbolicExpression* variable = PushStruct<SymbolicExpression>(out);
      variable->type = SymbolicExpressionType_VARIABLE;
      variable->variable = PushString(out,token);
      variable->negative = negative;
      return variable;
    }
  }

  return nullptr;
}

static SymbolicExpression* ParseDiv(Array<Token> tokens,int& index,Arena* out){
  SymbolicExpression* left = ParseTerm(tokens,index,out);

  SymbolicExpression* current = left;
  while(index < tokens.size){
    Token peek = tokens[index];

    char op = '\0';
    if(CompareString(peek,"/")){
      op = '/';
    }

    if(op == '\0'){
      break;
    }
    
    index += 1;
      
    SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);

    expr->type = SymbolicExpressionType_DIV;
    expr->top = current;
    expr->bottom = ParseTerm(tokens,index,out);

    current = expr;
  }

  return current;
}

static SymbolicExpression* ParseMul(Array<Token> tokens,int& index,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseDiv(tokens,index,out);
  
  ArenaList<SymbolicExpression*>* expressions = PushList<SymbolicExpression*>(temp);
  *expressions->PushElem() = left;

  while(index < tokens.size){
    Token peek = tokens[index];
    
    char op = '\0';
    if(CompareString(peek,"*")){
      op = '*';
    }

    if(op == '\0'){
      break;
    }
    
    index += 1;
    
    SymbolicExpression* expr = ParseDiv(tokens,index,out);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->terms = PushArray(out,expressions);

  return res;
}

static SymbolicExpression* ParseSum(Array<Token> tokens,int& index,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseMul(tokens,index,out);

  ArenaList<SymbolicExpression*>* expressions = PushList<SymbolicExpression*>(temp);
  *expressions->PushElem() = left;

  while(index < tokens.size){
    Token peek = tokens[index];

    bool negative = false;
    char op = '\0';
    if(CompareString(peek,"+")){
      op = '+';
    } else if(CompareString(peek,"-")){
      negative = true;
      op = '+';
    }

    if(op == '\0'){
      break;
    }

    index += 1;
      
    SymbolicExpression* expr = ParseMul(tokens,index,out);
    expr->negative = (expr->negative != negative);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->terms = PushArray(out,expressions);

  return res;
}

SymbolicExpression* ParseExpression(Array<Token> tokens,int& index,Arena* out){
  return ParseSum(tokens,index,out);
}

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out){
  // TODO: We need to find a way of solving the problem of creating a tokenizer template once and be done with it.
  //       We probably want to move this somewhat to the META data, no point in doing this at runtime.
  TEMP_REGION(temp,out);
  tmpl = CreateTokenizerTemplate(temp,",+-*/();[]",{".."});
  
  TOKENIZER_REGION(tok,tmpl);

  SymbolicExpression* expr = ParseExpression(tok,out);

  return expr;
}

SymbolicExpression* ParseSymbolicExpressionTest(String content,Arena* out);

SymbolicExpression* ParseSymbolicExpression(String content,Arena* out){
  TEMP_REGION(temp,out);
  
  Tokenizer tok(content,"",{});
  
  // nocheckin
  ParseSymbolicExpressionTest(content,temp);

  return ParseSymbolicExpression(&tok,out);
}

Array<SymbolicExpression*> GetAllExpressions(SymbolicExpression* top,Arena* out){
  auto GetAllExpressionsRecurse = [](auto recurse,SymbolicExpression* expr,GrowableArray<SymbolicExpression*>& arr) -> void{
    *arr.PushElem() = expr;

    switch(expr->type){
    case SymbolicExpressionType_FUNC: // fallthrough
    case SymbolicExpressionType_SUM: // fallthrough
    case SymbolicExpressionType_MUL: {
      for(SymbolicExpression* s : expr->terms){
        recurse(recurse,s,arr);
      }
    } break;
    case SymbolicExpressionType_DIV:{
      recurse(recurse,expr->top,arr);
      recurse(recurse,expr->bottom,arr);
    } break;
    case SymbolicExpressionType_LITERAL:;
    case SymbolicExpressionType_VARIABLE:;
    }
  };
 
  auto builder = StartArray<SymbolicExpression*>(out);
  GetAllExpressionsRecurse(GetAllExpressionsRecurse,top,builder);

  return EndArray(builder);
}

void CheckIfSymbolicExpressionsShareNodes(SymbolicExpression* left,SymbolicExpression* right){
  TEMP_REGION(temp,nullptr);

  Array<SymbolicExpression*> allLeft = GetAllExpressions(left,temp);
  Array<SymbolicExpression*> allRight = GetAllExpressions(right,temp);

  for(SymbolicExpression* x : allLeft){
    for(SymbolicExpression* y : allRight){
      if(x == y){
        printf("Node sharing on symbolic expressions\n");
        DEBUG_BREAK_OR_EXIT();
      }
    }
  }
}

// Children are not copied in this, check deep copy function
static SymbolicExpression* CopyExpression(SymbolicExpression* in,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  *res = *in;

  // Need to copy the string, the string memory is our responsibility.
  if(in->type == SymbolicExpressionType_VARIABLE || in->type == SymbolicExpressionType_FUNC){
    res->variable = PushString(out,res->variable);
  }
  
  return res;
}

// By default just copy the things. Since we are using arenas, allocations and deallocations are basically free anyway. We only care about the final expression, so we can just allocate a bunch of nodes that are immediatly deallocated. The extra copy is the worst part, but since we only care about simple expressions for now, probably nothing that needs attention for now. Can always add a better implementation later.
SymbolicExpression* SymbolicDeepCopy(SymbolicExpression* expr,Arena* out){
  if(expr == nullptr){
    return nullptr;
  }
  
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return CopyExpression(expr,out);
  } break;
  case SymbolicExpressionType_FUNC:
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->terms = PushArray<SymbolicExpression*>(out,expr->terms.size);
    for(int i = 0; i <  expr->terms.size; i++){
      res->terms[i] = SymbolicDeepCopy(expr->terms[i],out);
    }
    
    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->top = SymbolicDeepCopy(expr->top,out);
    res->bottom = SymbolicDeepCopy(expr->bottom,out);
    
    return res;
  } break;
  }
  NOT_POSSIBLE();
}

typedef SymbolicExpression* (*ApplyFunction)(SymbolicExpression* expr,Arena* out);
typedef SymbolicExpression* (*ApplyNonRecursiveFunction)(SymbolicExpression* expr,Arena* out);

Array<SymbolicExpression*> ApplyFunctionToArray(ApplyFunction Function,Array<SymbolicExpression*> arr,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<SymbolicExpression*>* builder = PushList<SymbolicExpression*>(temp);
  for(SymbolicExpression* spec : arr){
    *builder->PushElem() = Function(spec,out);
  }

  Array<SymbolicExpression*> result = PushArray(out,builder);

  return result;
}

SymbolicExpression* ApplyNonRecursive(SymbolicExpression* expr,Arena* out,ApplyNonRecursiveFunction Function){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return Function(expr,out);
  } break;
  case SymbolicExpressionType_FUNC: // fallthrough
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* list = PushList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->terms){
      *list->PushElem() = ApplyNonRecursive(s,out,Function);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->terms = PushArray(out,list);

    res = Function(res,out);
    
    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->top = ApplyNonRecursive(expr->top,out,Function);
    res->bottom = ApplyNonRecursive(expr->bottom,out,Function); 

    res = Function(res,out);
    
    return res;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ApplyGeneric(SymbolicExpression* expr,Arena* out,ApplyFunction Function){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_FUNC:
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* list = PushList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->terms){
      *list->PushElem() = Function(s,out);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->terms = PushArray(out,list);

    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->top = Function(expr->top,out);
    res->bottom = Function(expr->bottom,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

// The main ideia is to remove negative considerations from other code and let this function handle everything.
// At the very least, let this handle the majority of cases
SymbolicExpression* NormalizeNegative(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
  case SymbolicExpressionType_FUNC: // fallthrough
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_DIV:{
    NOT_IMPLEMENTED("yet");
    return expr; // Not recursing, only handles 1 layer, used by Apply functions
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    if(expr->type == SymbolicExpressionType_MUL){
      Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->terms.size);
      for(int i = 0; i < children.size; i++){
        children[i] = NormalizeNegative(expr->terms[i],out);
      }

      bool negative = expr->negative;
      for(SymbolicExpression* s : children){
        if(s->negative){
          s->negative = false;
          negative = !negative;
        }
      }

      SymbolicExpression* copy = CopyExpression(expr,out);
      copy->negative = negative;
      copy->terms = children;

      return copy;
    } else {
      return SymbolicDeepCopy(expr,out); // We could move negatives downwards or upwards, but I do not think there is any good way of doing things here.
    }
  } break;
  }
  
  NOT_POSSIBLE();
}

// TODO: Should this function be renamed to NormalizeStructure? Not sure if that is what we are doing or not.

// All functions should work recursively
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_FUNC: // fallthrough
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->top = RemoveParenthesis(expr->top,out);
    res->bottom = RemoveParenthesis(expr->bottom,out);
    return res;
  } break;
  case SymbolicExpressionType_SUM:
    if(expr->terms.size == 1){
      SymbolicExpression* onlyOne = RemoveParenthesis(expr->terms[0],out);
      onlyOne->negative = (onlyOne->negative != expr->negative);
      return onlyOne;
    }
    
    if(expr->negative){
      ArenaList<SymbolicExpression*>* list = PushList<SymbolicExpression*>(temp);
      for(SymbolicExpression* child : expr->terms){
        SymbolicExpression* negated = SymbolicDeepCopy(child,out);
        negated->negative = !negated->negative;

        *list->PushElem() = negated;
      }

      SymbolicExpression* res = CopyExpression(expr,out);
      res->negative = false;
      res->terms = PushArray(out,list);
      return res;
    }

    // fallthrough
  case SymbolicExpressionType_MUL: {
    if(expr->terms.size == 1){
      SymbolicExpression* onlyOne = RemoveParenthesis(expr->terms[0],out);
      onlyOne->negative = (onlyOne->negative != expr->negative);
      return onlyOne;
    }
    
    Array<SymbolicExpression*> children = ApplyFunctionToArray(RemoveParenthesis,expr->terms,out);
    ArenaList<SymbolicExpression*>* list = PushList<SymbolicExpression*>(temp);
    
    for(SymbolicExpression* child : children){
      bool uplift = (child->type == expr->type);
      if(uplift){
        for(SymbolicExpression* subChild : child->terms){
          if(child->negative){
            SymbolicExpression* negated = CopyExpression(subChild,out);
            negated->negative = !negated->negative;

            *list->PushElem() = negated;
          } else {
            *list->PushElem() = subChild;
          }
        }
      } else {
        *list->PushElem() = child;
      }
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->terms = PushArray(out,list);
    
    return res;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ApplyNegation(SymbolicExpression* expr,bool negate){
  if(negate){
    expr->negative = !expr->negative;
  }

  return expr;
}

// TODO: Should this recurse? For now, only specific functions call this.
SymbolicExpression* SortTerms(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
    // TODO: Eventually add SUM?
  case SymbolicExpressionType_MUL: {
    int size = expr->terms.size;

    if(size <= 1){
      return expr;
    }
    
    // Literals, if exist, always appear first.
    // Assuming only 1 literal, since this is supposed to be called for normalized expressions
    bool didSwap = false;
    for(int i = 0; i < size; i++){
      if(expr->terms[i]->type == SymbolicExpressionType_LITERAL){
        SWAP(expr->terms[0],expr->terms[i]);
        didSwap = true;
      }
    }

    int firstIndex = (didSwap ? 1 : 0);
    for(int i = firstIndex; i < size; i++){
      if(expr->terms[i]->type != SymbolicExpressionType_VARIABLE){
        continue;
      }
      
      for(int ii = i + 1; ii < size; ii++){
        if(expr->terms[ii]->type != SymbolicExpressionType_VARIABLE){
          continue;
        }
        
        if(CompareStringOrdered(expr->terms[i]->variable,expr->terms[ii]->variable) < 0){
          SWAP(expr->terms[i],expr->terms[ii]);
        }
      }
    }
  } break;
  default:;
  }

  return expr;
}

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_FUNC: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_LITERAL: {
    // NOTE: Literals are always positive and negation of the expr is the normal form to represent negatives.
    SymbolicExpression* copy = SymbolicDeepCopy(expr,out);
    if(copy->literal < 0){
      copy->literal = -copy->literal;
      copy->negative = !copy->negative;
    }
    return copy;
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* childs = PushList<SymbolicExpression*>(temp);
    ArenaList<SymbolicExpression*>* literals = PushList<SymbolicExpression*>(temp);
    
    for(int i = 0; i < expr->terms.size; i++){
      SymbolicExpression* child = NormalizeLiterals(expr->terms[i],out);

      if(child->type == SymbolicExpressionType_LITERAL){
        *literals->PushElem() = child;
      } else {
        *childs->PushElem() = child;
      }
    }

    // How is this possible? empty literals and childs means that we must return zero, no?
    // The only way this happens is if expr->sum.size is zero which I do not believe should be possible.
    // TODO: Need to check if this code is being hit and in what situations
    if(Empty(literals)){
      if(Empty(childs)){
        return SymbolicDeepCopy(expr,out);
      }
      
      SymbolicExpression* copy = CopyExpression(expr,out);
      copy->terms = PushArray(out,childs);
      return copy;
    }
    
    Array<SymbolicExpression*> allLiterals = PushArray(temp,literals);
    
    bool isMul = (expr->type == SymbolicExpressionType_MUL);
    
    int finalResult = 0;
    if(isMul){
      finalResult = 1;
    }

    for(SymbolicExpression* child : allLiterals){
      int val = GetLiteralValue(child);
      if(isMul){
        finalResult *= val;
      } else {
        finalResult += val;
      }
    }

#if 0 
    if(expr->negative){
      finalResult = -finalResult;
    }
#endif    
    
    if(isMul){
      if(Empty(childs)){
        return PushLiteral(out,finalResult,expr->negative);
      } else if(finalResult == 0){
        return PushLiteral(out,0);
      } else {
        SymbolicExpression* res = CopyExpression(expr,out);
        // No point appending a literal 1 in a multiplication.
        if(finalResult != 1){
          SymbolicExpression* finalLiteral = PushLiteral(out,finalResult);

          *childs->PushElem() = finalLiteral;
        }

        res->terms = PushArray(out,childs);
        SortTerms(res,out);

        // Kinda on an hack, no point returning a sum/mult of a single term
        // Extremely good hack, in the sense that it does not even work.
        // Something related to the negation not being good.
        // TODO: Eventually fix this. The logic must return simple terms if possible, a sum/mult of 1 element is not good.
#if 0
        if(res->sum.size == 1){
          return ApplyNegation(res->sum[0],negate);
        }
#endif
        
        return res;
      }
    } else {
      SymbolicExpression* res = CopyExpression(expr,out);
      if(finalResult != 0){
        SymbolicExpression* finalLiteral = PushLiteral(out,finalResult);

        *childs->PushElem() = finalLiteral;
      }

      res->terms = PushArray(out,childs);

      // Kinda on an hack, no point returning a sum/mult of a single term
      // TODO: We can probably simplify the logic in here. Remember, if only 1 term then we want to return it, not make a sum/mult of 1 term.
#if 0
      if(res->sum.size == 1){
        return ApplyNegation(res->sum[0],negate);
      }
#endif
      
      return res;
    }
  } break;
  case SymbolicExpressionType_DIV:{
    auto GetMultPartition = [](SymbolicExpression* expr,Arena* out) -> MultPartition{
      MultPartition res = {};

      switch(expr->type){
      case SymbolicExpressionType_LITERAL:{
        res.base = expr;
        //res.leftovers = PushLiteral(out,1);
        return res;
      } break;
      case SymbolicExpressionType_MUL:{
        for(int i = 0; i <  expr->terms.size; i++){
          SymbolicExpression* child  =  expr->terms[i];
          if(child->type == SymbolicExpressionType_LITERAL){
            res.base = child;
            res.leftovers = SymbolicMult(RemoveElement(expr->terms,i,out),out);
            
            return res;
          }
        }
      } break;
      }

      return res;
    };
    
    SymbolicExpression* top = NormalizeLiterals(expr->top,out);
    SymbolicExpression* bottom = NormalizeLiterals(expr->bottom,out);
    
    MultPartition topLiteral = GetMultPartition(top,out);
    MultPartition bottomLiteral = GetMultPartition(bottom,out);

    if(IsZero(top)){
      return PushLiteral(out,0);
    }
    
    // NOTE: Not properly tested. Care when using divs.
    if(topLiteral.base && bottomLiteral.base){
      int top = topLiteral.base->literal;
      int bottom = bottomLiteral.base->literal;

      if(top % bottom == 0){
        SymbolicExpression* literalResult = PushLiteral(out,top / bottom);

        if(bottomLiteral.leftovers){
          SymbolicExpression* newBottom = bottomLiteral.leftovers;

          if(topLiteral.leftovers){
            SymbolicExpression* newTop = SymbolicMult(topLiteral.leftovers,literalResult,out);
            return SymbolicDiv(newTop,newBottom,out);
          } else {
            SymbolicExpression* newTop = literalResult;
            return SymbolicDiv(newTop,newBottom,out);
          }
        } else {
          if(topLiteral.leftovers){
            return SymbolicMult(topLiteral.leftovers,literalResult,out);
          } else {
            return literalResult;
          }
        }
      } else {
        // Nothing we can do, we do not support decimal values.
      }
    }

    SymbolicExpression* copy = CopyExpression(expr,out);
    copy->top = top;
    copy->bottom = bottom;
    return copy;

  } break;
  }
  NOT_POSSIBLE();
}

// Must call normalizeLiteral before calling this one. 
MultPartition CollectTermsWithLiteralMultiplier(SymbolicExpression* expr,Arena* out){
  //Assert(expr->type == SymbolicExpressionType_ARRAY && expr->op == '*');
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:{
    MultPartition res = {};
    res.base = expr;

    SymbolicExpression* mul = PushMulBase(out);
    mul->terms = PushArray<SymbolicExpression*>(out,0);
    res.leftovers = mul;
    
    //res.mulTerms = expr;
    return res;
  } break;
  case SymbolicExpressionType_FUNC:
    // WARN_CODE();
    // Treating this the same as a variable
  case SymbolicExpressionType_VARIABLE: {
    MultPartition res = {};
    res.base = PushLiteral(out,1);

    SymbolicExpression* cp = CopyExpression(expr,out);
    
    SymbolicExpression* mul = PushMulBase(out);
    mul->terms = PushArray<SymbolicExpression*>(out,1);
    mul->terms[0] = cp;

    if(cp->negative){
      res.base->negative = !res.base->negative;
      cp->negative = false;
    }
    
    res.leftovers = mul;
    
    return res;
  }
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    SymbolicExpression* literal = nullptr;

    auto builder = StartArray<SymbolicExpression*>(out); 
    for(SymbolicExpression* s : expr->terms){
      if(s->type == SymbolicExpressionType_LITERAL){
        Assert(literal == nullptr); // This function is only intended to handle 1 literal.
        literal = s;
      } else {
        *builder.PushElem() = s;
      }
    }

    if(!literal){
      literal = PushLiteral(out,1);
    }
  
    MultPartition result = {};
    result.base = CopyExpression(literal,out);

    SymbolicExpression* mul = PushStruct<SymbolicExpression>(out);
    mul->type = SymbolicExpressionType_MUL;
    mul->terms = EndArray(builder);

    // Push negative to the mul expression
    bool negative = literal->negative != expr->negative;
    for(SymbolicExpression* s : mul->terms){
      if(s->negative){
        negative = !negative;
        s->negative = false;
      }
    }
    mul->negative = false;
    result.base->negative = negative;

    result.leftovers = mul;
    return result;
  } break;
  case SymbolicExpressionType_DIV:{
    // TODO: This is not goodish. Something weird is being done here that kinda of breaks the flow of the code. I think the fact that we must return an expression of type '*' works but is more error prone than we actually need.
    MultPartition result = {};

    SymbolicExpression* thisCopy = CopyExpression(expr,out);
    thisCopy->negative = false;
    SymbolicExpression* mul = PushMulBase(out);
    mul->terms = PushArray<SymbolicExpression*>(out,1);
    mul->terms[0] = thisCopy;
    
    if(expr->top->type == SymbolicExpressionType_LITERAL){
      SymbolicExpression* cp = CopyExpression(expr->top,out);
      cp->negative = (cp->negative != expr->negative);

      result.base = cp;
      result.leftovers = mul;
      
      return result;
    } else {
      SymbolicExpression* cp = CopyExpression(expr->top,out);
      cp->negative = (cp->negative != expr->negative);

      result.base = PushLiteral(out,1,expr->negative);
      result.leftovers = mul;

      return result;
    }
  } break;
  }
  NOT_POSSIBLE();
}

static bool ContainsVariable(SymbolicExpression* expr,String variable){
  switch(expr->type){
  case SymbolicExpressionType_FUNC: {
    NOT_IMPLEMENTED("What should this return???");
  } break;
  case SymbolicExpressionType_LITERAL:
    return false;
  case SymbolicExpressionType_VARIABLE:{
    return CompareString(expr->variable,variable);
  } break;
  case SymbolicExpressionType_MUL:
  case SymbolicExpressionType_SUM: {
    for(SymbolicExpression* child : expr->terms){
      if(ContainsVariable(child,variable)){
        return true;
      }
    }
  } break;
  case SymbolicExpressionType_DIV:{
    return (ContainsVariable(expr->top,variable) || ContainsVariable(expr->bottom,variable));
  } break;
  }

  return false;
}

MultPartition PartitionMultExpressionOnVariable(SymbolicExpression* expr,String variableToPartition,Arena* out){
  TEMP_REGION(temp,out);

  if(expr->type != SymbolicExpressionType_MUL){
    return {};
  }

  SymbolicExpression* variable = nullptr;
  auto leftovers = StartArray<SymbolicExpression*>(temp);

  for(SymbolicExpression* child : expr->terms){
    if(ContainsVariable(child,variableToPartition)){
      Assert(!variable); // Can only handle one variable currently. Address gen can only handle linear equations anyway.
      variable = child;
    } else {
      *leftovers.PushElem() = child;
    }
  }

  Array<SymbolicExpression*> left = EndArray(leftovers);
  
  MultPartition res = {};
  res.base = variable;
  res.leftovers = SymbolicMult(left,out);

  if(expr->negative){
    res.base->negative = !res.base->negative;
  }
  
  return res;
}

// Performs no form of normalization or canonicalization whatsoever.
// A simple equality for use by other functions
bool ExpressionEqual(SymbolicExpression* left,SymbolicExpression* right){
  if(left == right){
    return true;
  }
  if(!left || !right){
    return false;
  }
  
  if(left->type != right->type){
    return false;
  }

  SymbolicExpressionType type = left->type;
  if(left->negative != right->negative){
    // TODO: This is stupid. Makes more sense to always normalize a literal to make sure that negative zero is never possible
    if(type == SymbolicExpressionType_LITERAL && GetLiteralValue(left) == 0 && GetLiteralValue(right) == 0){
      return true;
    }

    return false;
  }
  
  switch(left->type){
  case SymbolicExpressionType_LITERAL: return (GetLiteralValue(left) == GetLiteralValue(right));
  case SymbolicExpressionType_VARIABLE: return CompareString(left->variable,right->variable);
  case SymbolicExpressionType_DIV: {
    return ExpressionEqual(left->top,right->top) && ExpressionEqual(left->bottom,right->bottom);
  }
  case SymbolicExpressionType_FUNC:
    if(!CompareString(left->name,right->name)){
      return false;
    }
    // fallthrough
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    if(left->terms.size != right->terms.size){
      return false;
    }

    int size = left->terms.size;
    for(int i = 0; i < size; i++){
      if(!ExpressionEqual(left->terms[i],right->terms[i])){
        return false;
      }
    }

    return true;
  }
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_FUNC: //fallthrough
  case SymbolicExpressionType_LITERAL: //fallthrough
  case SymbolicExpressionType_VARIABLE: //fallthrough
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
  case SymbolicExpressionType_DIV: {
    SymbolicExpression* top = ApplySimilarTermsAddition(expr->top,out);
    SymbolicExpression* bottom = ApplySimilarTermsAddition(expr->bottom,out);

    if(top->type == SymbolicExpressionType_VARIABLE && bottom->type == SymbolicExpressionType_VARIABLE){
      if(ExpressionEqual(top,bottom)){
        return PushLiteral(out,1);
      }
    }
    // TODO: We currently only handle the case where div bottom only contains a literal. We are missing the bottom div expr being a mul.
    if(top->type == SymbolicExpressionType_MUL && bottom->type == SymbolicExpressionType_VARIABLE){
      Array<SymbolicExpression*> terms = top->terms;

      for(int i = 0; i <  terms.size; i++){
        SymbolicExpression* expr = terms[i];

        if(ExpressionEqual(expr,bottom)){
          Array<SymbolicExpression*> leftOvers = RemoveElement(terms,i,out);

          if(leftOvers.size > 0){
            return SymbolicMult(leftOvers,out);
          } else {
            return PushLiteral(out,1);
          }
        }
      }
    }
    
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
  } break;
  case SymbolicExpressionType_MUL: {
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
  } break;
    
  case SymbolicExpressionType_SUM:{
    SymbolicExpression* normalized = NormalizeLiterals(expr,out);
    
    if(normalized->type != SymbolicExpressionType_SUM){
      return normalized;
    }
    
    Array<MultPartition> terms = PushArray<MultPartition>(temp,normalized->terms.size);
    for(int i = 0; i <  normalized->terms.size; i++){
      SymbolicExpression* s = normalized->terms[i];
      terms[i] = CollectTermsWithLiteralMultiplier(s,out);
    }

    Array<bool> termUsed = PushArray<bool>(temp,terms.size);
    Memset(termUsed,false);

    ArenaList<SymbolicExpression*>* finalExpressions = PushList<SymbolicExpression*>(temp);
    for(int i = 0; i < terms.size; i++){
      if(termUsed[i]){
        continue;
      }

      MultPartition left = terms[i];

      int finalLiteralValue = GetLiteralValue(left.base);
      for(int j = i + 1; j < terms.size; j++){
        if(termUsed[j]){
          continue;
        }
        
        MultPartition right = terms[j];
        
        if(ExpressionEqual(left.leftovers,right.leftovers)){
          termUsed[j] = true;
          finalLiteralValue += GetLiteralValue(right.base);
        }
      }
      
      if(finalLiteralValue != 0){
        SymbolicExpression* finalLiteral = PushLiteral(out,finalLiteralValue);
        SymbolicExpression* finalMul = PushStruct<SymbolicExpression>(out);
      
        Array<SymbolicExpression*> subTerms = PushArray<SymbolicExpression*>(out,left.leftovers->terms.size + 1);
        subTerms[0] = finalLiteral;
        for(int i = 0; i < left.leftovers->terms.size; i++){
          subTerms[i + 1] = left.leftovers->terms[i];
        }
        
        finalMul->type = SymbolicExpressionType_MUL;
        finalMul->negative = finalLiteral->negative;
        finalLiteral->negative = false;
        finalMul->terms = subTerms;
        
        *finalExpressions->PushElem() = finalMul;
      }
    }

    if(OnlyOneElement(finalExpressions)){
      return finalExpressions->head->elem;
    }
    
    if(Empty(finalExpressions)){
      return PushLiteral(out,0);
    }

    SymbolicExpression* sum = PushStruct<SymbolicExpression>(out);
    sum->type = SymbolicExpressionType_SUM;
    sum->terms = PushArray(out,finalExpressions);

    Assert(sum->terms.size > 0);
    
    return sum;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out){
  // NOTE: Rule when it comes to negative. Every SymbolicExpression* that is eventually returned must have the negative field set. If any other Apply or such expression is called, (like RemoveParenthesis) then the field is set before calling such functions. Do this and negative should not be a problem. Not sure though.
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_FUNC: //fallthrough
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;

  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->terms.size);
    for(int i = 0; i < children.size; i++){
      children[i] = ApplyDistributivity(expr->terms[i],out);
    }

    if(expr->type == SymbolicExpressionType_SUM){
      // Nothing to do except apply distributivity to childs
      SymbolicExpression* res = CopyExpression(expr,out);
      res->terms = children;
      
      return res;
    } else if(expr->type == SymbolicExpressionType_MUL){
      bool containsAddition = false;
      for(SymbolicExpression* spec : expr->terms){
        if(spec->type == SymbolicExpressionType_SUM){
          containsAddition = true;
        }
      }

      if(!containsAddition){
        ArenaList<SymbolicExpression*>* muls = PushList<SymbolicExpression*>(temp);
        for(SymbolicExpression* spec : expr->terms){
          *muls->PushElem() = ApplyDistributivity(spec,out);
        }

        SymbolicExpression* res = CopyExpression(expr,out);
        res->terms = children;
        return res;
      }
      
      // A multiplication of sums becomes a sum of multiplications
      ArenaList<SymbolicExpression*>* sums = PushList<SymbolicExpression*>(temp);

      // We only distribute one member and then let ApplyDistributivity on the result handle more distributions.
      // That is because if we ended doing everything here, we would enter the case where (x + y) * (a + b) would create 8 terms instead of the expected 4 terms. [Because we would generate x * (a + b), y * (a + b),a * (x + y) and b * (x + y) which eventually would give us 8 terms.]
      // There is probably a better solution, but I currently only need something that works fine for implementing the Address Gen.
      for(SymbolicExpression* spec : expr->terms){
        if(spec->type == SymbolicExpressionType_SUM){
          // Spec is the array of additions. (Ex: (a + b) in the (a + b) * (x + y) example

          // For a and b.
          for(SymbolicExpression* subTerm : spec->terms){
            // For each member in the additions array
            
            ArenaList<SymbolicExpression*>* mulTerms = PushList<SymbolicExpression*>(temp);
            *mulTerms->PushElem() = SymbolicDeepCopy(subTerm,out);
            for(SymbolicExpression* other : expr->terms){
              if(spec == other){
                continue;
              }

              *mulTerms->PushElem() = SymbolicDeepCopy(other,out);
            }

            SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
            res->type = SymbolicExpressionType_MUL;
            res->terms = PushArray(out,mulTerms);
            
            *sums->PushElem() = res;
          }
          break; // Only distribute one member at a time
        }
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_SUM;
      res->negative = expr->negative;
      res->terms = PushArray(out,sums);

      return res;
    } else {
      Assert(false);
    }
  } break;
    
  case SymbolicExpressionType_DIV: {
    // TODO: Division can be distributed, right?  But still not sure if we need it, we do want to move division up but need to see more usecases before starting looking at this.
    
    SymbolicExpression* res = CopyExpression(expr,out);
    res->top = ApplyDistributivity(expr->top,out);
    res->bottom = ApplyDistributivity(expr->bottom,out);
    
    return res;
  } break;
  }
  
  return nullptr;
}

SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_FUNC:{
    // TODO: This is being hit. Versat is working but need to check this more thoroughly
    // WARN_CODE(); // Functions are a very special case that we insert ourselves and derivate is only used to transform the initial expression into multiple individual loops so we should never reach this point. If this triggers, do a more careful analysis to what is happening.
    return PushLiteral(out,0); // Treating all functions as a literal.
  } break;
  case SymbolicExpressionType_LITERAL:
    return PushLiteral(out,0);
  case SymbolicExpressionType_VARIABLE: {
    if(CompareString(expr->variable,base)){
      return PushLiteral(out,1);
    } else {
      return PushLiteral(out,0);
    }
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    if(expr->type == SymbolicExpressionType_SUM){
      Array<SymbolicExpression*> sums = PushArray<SymbolicExpression*>(out,expr->terms.size);
      for(int i = 0; i <  expr->terms.size; i++){
        SymbolicExpression* s  =  expr->terms[i];
        sums[i] = Derivate(s,base,out);
      }
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->terms = sums;

      return addExpr;
    } else {
      // Derivative of mul is sum of derivative of one term with the remaining terms intact
      int size = expr->terms.size;
      Array<Pair<SymbolicExpression*,Array<SymbolicExpression*>>> mulTerms = AssociateOneToOthers(expr->terms,temp);
      
      Array<SymbolicExpression*> addTerms = PushArray<SymbolicExpression*>(out,size);
      for(int i = 0; i <  mulTerms.size; i++){
        SymbolicExpression* oneTerm = mulTerms[i].first;
        Array<SymbolicExpression*> otherTerms = mulTerms[i].second;
        
        SymbolicExpression* derivative = Derivate(oneTerm,base,out);
        
        auto builder = StartArray<SymbolicExpression*>(out);
        *builder.PushElem() = derivative;
        builder.PushArray(otherTerms);
        auto sumArray = EndArray(builder);
        
        SymbolicExpression* mulExpr = PushMulBase(out);
        mulExpr->terms = sumArray;
        addTerms[i] = mulExpr;
      }
     
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->terms = addTerms;

      return addExpr;
    }
  } break;
  case SymbolicExpressionType_DIV:{
    // Assuming that divisions are only done by constants. Address gen cannot handle otherwise.
    return PushLiteral(out,0);
  } break;
  }

  return nullptr;
}

SymbolicExpression* Group(SymbolicExpression* expr,String variableToGroupWith,Arena* out){
  TEMP_REGION(temp,out);

  switch(expr->type){
  case SymbolicExpressionType_FUNC:{
    return expr;
  } break;
  case SymbolicExpressionType_LITERAL:
    return expr;
  case SymbolicExpressionType_VARIABLE:{
    return expr;
  } break;
  case SymbolicExpressionType_SUM: {
    // This is the most important case, where we have a sum of expressions and we want to 

    Array<SymbolicExpression*> res = PushArray<SymbolicExpression*>(temp,expr->terms.size);

    for(int i = 0; i <  expr->terms.size; i++){
      SymbolicExpression* child  =  expr->terms[i];

      res[i] = Group(child,variableToGroupWith,out);
    }

    auto candidates = StartArray<MultPartition>(temp);
    auto leftovers = StartArray<SymbolicExpression*>(temp);

    for(SymbolicExpression* child : res){
      MultPartition partition = PartitionMultExpressionOnVariable(child,variableToGroupWith,temp);
      
      if(partition.base){
        *candidates.PushElem() = partition;
      } else {
        *leftovers.PushElem() = child;
      }
    }

    Array<SymbolicExpression*> left = EndArray(leftovers);
    Array<MultPartition> toGroup = EndArray(candidates);

    if(toGroup.size <= 1){
      return SymbolicAdd(res,out);
    }
    
    SymbolicExpression* termsInside = toGroup[0].leftovers;
    auto offsetted = Offset(toGroup,1);
    for(MultPartition part : offsetted){
      if(part.base->negative){
        termsInside = SymbolicSub(termsInside,part.leftovers,out);
      } else {
        termsInside = SymbolicAdd(termsInside,part.leftovers,out);
      }
    }

    SymbolicExpression* terms = Normalize(termsInside,out);
    SymbolicExpression* grouped = SymbolicMult(terms,toGroup[0].base,out);
    
    return SymbolicAdd(left,grouped,out);
  } break;
  case SymbolicExpressionType_MUL: {
    return expr;
  } break;
  case SymbolicExpressionType_DIV:{
    return expr;
  } break;
  }

  return expr;
}

SymbolicExpression* SymbolicReplace(SymbolicExpression* base,String varToReplace,SymbolicExpression* replacingExpr,Arena* out){
  switch(base->type){
  case SymbolicExpressionType_LITERAL:
    return SymbolicDeepCopy(base,out);
  case SymbolicExpressionType_VARIABLE: {
    if(CompareString(base->variable,varToReplace)){
      return SymbolicDeepCopy(replacingExpr,out);
    } else {
      return SymbolicDeepCopy(base,out);
    }
  } break;
  case SymbolicExpressionType_FUNC:
    //WARN_CODE(); // Treating this the same as the sum or mul case.
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->terms.size);
    for(int i = 0; i < children.size; i++){
      children[i] = SymbolicReplace(base->terms[i],varToReplace,replacingExpr,out);
    }

    SymbolicExpression* copy = CopyExpression(base,out);
    copy->terms = children;

    return copy;
  }
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(base,out);
    res->top = SymbolicReplace(base->top,varToReplace,replacingExpr,out);
    res->bottom = SymbolicReplace(base->bottom,varToReplace,replacingExpr,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ReplaceVariables(SymbolicExpression* base,Hashmap<String,SymbolicExpression*>* values,Arena* out){
  switch(base->type){
  case SymbolicExpressionType_LITERAL:
    return SymbolicDeepCopy(base,out);
  case SymbolicExpressionType_VARIABLE: {
    SymbolicExpression** exists = values->Get(base->variable);
    
    if(exists){
      return SymbolicDeepCopy(*exists,out);
    } else {
      return SymbolicDeepCopy(base,out);
    }
  } break;
  case SymbolicExpressionType_FUNC:
    //WARN_CODE(); // Treating this the same as the sum or mul case.
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->terms.size);
    for(int i = 0; i < children.size; i++){
      children[i] = ReplaceVariables(base->terms[i],values,out);
    }

    SymbolicExpression* copy = CopyExpression(base,out);
    copy->terms = children;

    return copy;
  }
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(base,out);
    res->top = ReplaceVariables(base->top,values,out);
    res->bottom = ReplaceVariables(base->bottom,values,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* MoveDivToTop(SymbolicExpression* base,Arena* out){
  switch(base->type){
  case SymbolicExpressionType_LITERAL: {
    return SymbolicDeepCopy(base,out);
  } break;
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(base,out);
  } break;
  case SymbolicExpressionType_FUNC: {
    return SymbolicDeepCopy(base,out);
  } break;
  case SymbolicExpressionType_SUM: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->terms.size);
    for(int i = 0; i < children.size; i++){
      children[i] = MoveDivToTop(base->terms[i],out);
    }

    SymbolicExpression* copy = CopyExpression(base,out);
    copy->terms = children;

    // TODO: Need to handle this.
    //NOT_IMPLEMENTED("Need to check if there something that we can do here");
    
    return copy;
  } break;
  case SymbolicExpressionType_MUL: {
    SymbolicExpression* divLike = nullptr;
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->terms.size);
    for(int i = 0; i < children.size; i++){
      children[i] = MoveDivToTop(base->terms[i],out);
      if(children[i]->type == SymbolicExpressionType_DIV){
        divLike = children[i];
      }
    }

    if(!divLike){
      SymbolicExpression* copy = CopyExpression(base,out);
      copy->terms = children;

      return copy;
    }

    // We have at least one div. We multiple all the expressions to 
    SymbolicExpression* topExpr = divLike->top;
    SymbolicExpression* bottomExpr = divLike->bottom;
    
    for(int i = 0; i < children.size; i++){
      if(children[i] == divLike){
        continue;
      }

      if(children[i]->type == SymbolicExpressionType_DIV){
        topExpr = SymbolicMult(topExpr,children[i]->top,out);
        bottomExpr = SymbolicMult(bottomExpr,children[i]->bottom,out);
      } else {
        topExpr = SymbolicMult(topExpr,children[i],out);
      }
    }

    return SymbolicDiv(topExpr,bottomExpr,out);
  }
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(base,out);
    res->top = MoveDivToTop(base->top,out);
    res->bottom = MoveDivToTop(base->bottom,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

Array<String> ExtractAllVariables(SymbolicExpression* top,Arena* out){
  TEMP_REGION(temp,out);
  auto Recurse = [](auto Recurse,SymbolicExpression* expr,ArenaList<String>* accum){
    switch(expr->type){
    case SymbolicExpressionType_LITERAL:{
      return;
    } break;
    case SymbolicExpressionType_VARIABLE:{
      *accum->PushElem() = expr->variable;
    } break;
    case SymbolicExpressionType_DIV:{
      Recurse(Recurse,expr->top,accum);
      Recurse(Recurse,expr->bottom,accum);
    } break;
    case SymbolicExpressionType_FUNC: // fallthrough
    case SymbolicExpressionType_SUM: // fallthrough
    case SymbolicExpressionType_MUL:{
      for(SymbolicExpression* child : expr->terms){
        Recurse(Recurse,child,accum);
      }
    } break;
    }
  };
  
  auto* accum = PushList<String>(temp);
  Recurse(Recurse,top,accum);
  Array<String> res = PushArray(out,accum);

  return res;
}

void CheckIfCorrect(SymbolicExpression* start,SymbolicExpression* end){
  TEMP_REGION(temp,nullptr);

  Array<String> allVariables = ExtractAllVariables(start,temp);
  int varSize = allVariables.size;

  for(int i = 0; i < 100; i++){
    Hashmap<String,int>* values = PushHashmap<String,int>(temp,varSize);

    for(int ii = 0; ii < varSize; ii++){
      int randomValue = (int) (GetRandomNumber() % 100);
      values->Insert(allVariables[ii],randomValue);
    }

    int startVal = Evaluate(start,values);
    int endVal = Evaluate(end,values);

    if(startVal != endVal){
      printf("Different values for the expressions:\n");
      Print(start,true);
      Print(end,true);
      return;
    }
  }
}

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,bool debugPrint){
  TEMP_REGION(temp,out);
  SymbolicExpression* current = expr;
  SymbolicExpression* next = nullptr;

  if(debugPrint){
    printf("Norm start:\n");
    PrintAST(expr);
  }

  bool debugPrintAST = false;
  
  // TODO: Better way of detecting end, should be a check if expression did not change after an entire loop
  for(int i = 0; i < 10; i++){
    BLOCK_REGION(temp);
    if(debugPrint) printf("%d:\n",i);

    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Normalize Literals:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Normalize Literals:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);
    
    next = ApplyDistributivity(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Apply distributivity:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Remove paran:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Remove paran:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = ApplySimilarTermsAddition(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("SimilarTerms:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next); 
    current = next;
    if(debugPrint) printf("Remove paran:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    // Logic inside similar term might add superflouous constants, so we normalize literals again
    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Normalize Literals:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);

    next = MoveDivToTop(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("MoveDivToTop:\n");
    if(debugPrint) {Print(current); printf("\n");}
    if(debugPrintAST) PrintAST(current);
  }

  current = ApplyNonRecursive(current,out,SortTerms);

  //CheckIfCorrect(expr,current);
  
  return current;
}

struct TestCase{
  String input;
  String expectedNormalized;
};

void TestSymbolic(){
  TEMP_REGION(temp,nullptr);

  TestCase cases[] = {
    {"a+b+c+d","a+b+c+d"},
    {"a-a-b-b-2*c-c","-(2*b-(3*c))"},
    {"a+a+b+b+2*c+c","2*a+2*b+3*c"},
    {"a*b + a * b","2*a*b"},
    {"-a * b - a * b","-(2*a*b)"},
    {"-((1*x)*(3-1))+1*y","-(2*x+y)"},
    {"-a-b-(a-b)-(-a+b)-(-(a-b)-(-a+b)+(a-b) + (-a+b))","-(a)-(b)"},
    {"(a-b)*(a-b)","a*a-(2*a*b)+b*b"},
    {"a*b + a*b + 2*a*b","4*a*b"},
    {"1+2+3+1*20*30","606"},
    {"a * (x + y)","a*x+a*y"},
    {"(0+2*x)+(2*a-1)*y","2*x+2*a*y-(y)"},
    {"-6*(4-1)+5","-13"},
    {"-(1*(x-1))+0","-(x)+1"},
    {"ALIGN(1,2)","ALIGN(1,2)"},
    {"1/x * y","y/x"},
    {"x/x","1"},
    {"(x*y)/x","y"},
  };

  bool printNormalizeProcess = true;
 
  printf("\n");
  for(TestCase c : cases){
    BLOCK_REGION(temp);
    SymbolicExpression* sym = ParseSymbolicExpression(c.input,temp);
    SymbolicExpression* normalized = Normalize(sym,temp);

    String repr = PushRepr(temp,normalized);

    if(CompareString(repr,c.expectedNormalized)){
      printf("OK");
    } else {
      printf("Different\n");      

      printf("Start:");
      Print(sym);
      printf("\nEnd  :");
      Print(normalized);

#if 1
      printf("\nExpec:%.*s\n",UN(c.expectedNormalized));
      PrintAST(normalized);

      if(printNormalizeProcess){
        printf("Proccess:\n");
        Normalize(sym,temp,true);
        printf("\n");
      }
#endif
    }

    printf("\n");
  }
}

Array<String> GetAllSymbols(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  
  Array<SymbolicExpression*> allExpressions = GetAllExpressions(expr,temp);

  TrieSet<String>* set = PushTrieSet<String>(temp);
  
  for(SymbolicExpression* expr : allExpressions){
    if(expr->type == SymbolicExpressionType_VARIABLE){
      set->Insert(expr->variable);
    }
  }
  
  return PushArray(out,set);
}

Opt<SymbolicExpression*> GetMultExpressionAssociatedTo(SymbolicExpression* expr,String variableName,Arena* out){
  FULL_SWITCH(expr->type){
  case SymbolicExpressionType_LITERAL:{
    return {};
  } break;
  case SymbolicExpressionType_FUNC:{
    NOT_IMPLEMENTED("yet");
  } break;
  case SymbolicExpressionType_VARIABLE:{
    if(CompareString(expr->variable,variableName)){
      return ApplyNegation(PushLiteral(out,1),expr->negative);
    }
  } break;
  case SymbolicExpressionType_SUM:{
    for(SymbolicExpression* child : expr->terms){
      Opt<SymbolicExpression*> res = GetMultExpressionAssociatedTo(child,variableName,out);
      if(res.has_value()){
        return res;
      }
    }
  } break;
  case SymbolicExpressionType_MUL:{
    int size = expr->terms.size;
    int foundIndex = -1;

    for(int i = 0; i < size; i++){
      SymbolicExpression* child = expr->terms[i];

      if(child->type == SymbolicExpressionType_VARIABLE && CompareString(child->variable,variableName)){
        foundIndex = i;
        break;
      }
    }

    if(foundIndex == -1){
      return {};
    }
    
    if(size == 1){
      return ApplyNegation(PushLiteral(out,1),expr->negative);
    }

    if(size - 1 == 1){
      int childrenIndex = (1 - foundIndex);
      return expr->terms[childrenIndex];
    }
    
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,size - 1);

    int inserted = 0;
    for(int i = 0; i < size; i++){
      SymbolicExpression* child = expr->terms[i];
      
      if(i == foundIndex){
        continue;
      }
      children[inserted++] = child;
    }
    
    SymbolicExpression* res = PushMulBase(out,expr->negative);
    res->terms = children;

    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    Opt<SymbolicExpression*> top = GetMultExpressionAssociatedTo(expr->top,variableName,out);

    if(top.has_value()){
      return SymbolicDiv(top.value(),expr->bottom,out);
    }

    Opt<SymbolicExpression*> bottom = GetMultExpressionAssociatedTo(expr->bottom,variableName,out);

    // TODO: Need to figure out this one. Assuming that the normalization process was good, we should probably not reawch this point if the expression is linear, right?. Are there any cases where we can still do something?
    if(bottom.has_value()){
      NOT_IMPLEMENTED("What do we do for this case? The expression is no longer linear, right?");
    }
    
    return {};
  } break;
  }
  return {};
}

LoopLinearSum* PushLoopLinearSumEmpty(Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  // NOTE: Everything is simpler is freeTerm is already initialized to zero.
  res->freeTerm = PushLiteral(out,0);
  return res;
}

LoopLinearSum* PushLoopLinearSumFreeTerm(SymbolicExpression* term,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  res->freeTerm = SymbolicDeepCopy(term,out);
  return res;
}

LoopLinearSum* PushLoopLinearSumSimpleVar(String loopVarName,SymbolicExpression* term,SymbolicExpression* start,SymbolicExpression* end,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  res->terms = PushArray<LoopLinearSumTerm>(out,1);
  res->terms[0].var = PushString(out,loopVarName);
  res->terms[0].term = SymbolicDeepCopy(term,out);
  res->terms[0].loopStart = SymbolicDeepCopy(start,out);
  res->terms[0].loopEnd = SymbolicDeepCopy(end,out);
  
  res->freeTerm = PushLiteral(out,0);

  return res;
}

LoopLinearSum* Copy(LoopLinearSum* in,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  int size = in->terms.size;

  res->terms = PushArray<LoopLinearSumTerm>(out,size);
  for(int i = 0; i < size; i++){
    res->terms[i].var = PushString(out,in->terms[i].var);
    res->terms[i].term = SymbolicDeepCopy(in->terms[i].term,out);
    res->terms[i].loopStart = SymbolicDeepCopy(in->terms[i].loopStart,out);
    res->terms[i].loopEnd = SymbolicDeepCopy(in->terms[i].loopEnd,out);
  }

  res->freeTerm = SymbolicDeepCopy(in->freeTerm,out);
  return res;
}

LoopLinearSum* AddLoopLinearSum(LoopLinearSum* inner,LoopLinearSum* outer,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  int innerSize = inner->terms.size;
  int outerSize = outer->terms.size;
  int totalSize = innerSize + outerSize;

  res->terms = PushArray<LoopLinearSumTerm>(out,totalSize);
  for(int i = 0; i < innerSize; i++){
    res->terms[i].var = PushString(out,inner->terms[i].var);
    res->terms[i].term = SymbolicDeepCopy(inner->terms[i].term,out);
    res->terms[i].loopStart = SymbolicDeepCopy(inner->terms[i].loopStart,out);
    res->terms[i].loopEnd = SymbolicDeepCopy(inner->terms[i].loopEnd,out);
  }
  for(int i = 0; i < outerSize; i++){
    res->terms[i + innerSize].var = PushString(out,outer->terms[i].var);
    res->terms[i + innerSize].term = SymbolicDeepCopy(outer->terms[i].term,out);
    res->terms[i + innerSize].loopStart = SymbolicDeepCopy(outer->terms[i].loopStart,out);
    res->terms[i + innerSize].loopEnd = SymbolicDeepCopy(outer->terms[i].loopEnd,out);
  }

  res->freeTerm = Normalize(SymbolicAdd(inner->freeTerm,outer->freeTerm,out),out);

  return res;
}

LoopLinearSum* RemoveLoop(LoopLinearSum* in,int index,Arena* out){
  LoopLinearSum* res = Copy(in,out);

  res->terms = RemoveElement(res->terms,index,out);

  return res;
}

SymbolicExpression* TransformIntoSymbolicExpression(LoopLinearSum* sum,Arena* out){
  TEMP_REGION(temp,out);

  int size = sum->terms.size;
  Array<SymbolicExpression*> individualTermsArray = PushArray<SymbolicExpression*>(temp,size + 1);

  // TODO: We probably can simplify this function. The rest of the code appears to be able to handle normalization of this expression and it appears that we do not have to be as careful when creating the expression as we previously where.
  
  // The expression is carefully constructed in order to avoid the normalization process removing the grouping of variables. Easier than changing the normalization process to preserve groupings in some situations and not in others.
  int inserted = 0;
  for(LoopLinearSumTerm term : sum->terms){
    SymbolicExpression* normalized = Normalize(term.term,temp);
    SymbolicExpression* fullTerm = nullptr;

    // NOTE: Since no normalization, need to take into account identity of multiplication. Similar to NOTE below
    if(normalized->type == SymbolicExpressionType_LITERAL && GetLiteralValue(normalized) == 1){
      fullTerm = PushVariable(temp,term.var);
    } else {
      fullTerm = SymbolicMult(normalized,PushVariable(temp,term.var),temp);
    }

    individualTermsArray[inserted++] = fullTerm; 
  }

  // NOTE: Since we do not normalize the final expression, need to take into account the identity of addition, so that we do not end up with an extra +0 for no reason
  if(sum->freeTerm->type == SymbolicExpressionType_LITERAL && GetLiteralValue(sum->freeTerm) == 0){
    individualTermsArray.size -= 1;
  } else {
    individualTermsArray[inserted++] = SymbolicDeepCopy(sum->freeTerm,temp);
  }
    
  // We do not normalize this expression in order to keep all the variables inside the same
  SymbolicExpression* res = SymbolicAdd(individualTermsArray,temp);

  // Everything is in temp arena, final copy to out arena.
  res = SymbolicDeepCopy(res,out);
  
  return res;
}

SymbolicExpression* LoopLinearSumTermSize(LoopLinearSumTerm* term,Arena* out){
  SymbolicExpression* expr = SymbolicSub(term->loopEnd,term->loopStart,out);
  return expr;
}

SymbolicExpression* GetLoopLinearSumTotalSize(LoopLinearSum* in,Arena* out){
  TEMP_REGION(temp,out);

  SymbolicExpression* expr = PushLiteral(temp,1); 
  for(LoopLinearSumTerm term : in->terms){
    SymbolicExpression* loopSize = LoopLinearSumTermSize(&term,temp);
    expr = SymbolicMult(expr,loopSize,temp);
  }
  
  SymbolicExpression* result = Normalize(expr,out);
  return result;
}

void Print(LoopLinearSum* sum,bool printNewLine){
  TEMP_REGION(temp,nullptr);

  int size = sum->terms.size;

  for(int i = size - 1; i >= 0; i--){
    printf("for %.*s ",UN(sum->terms[i].var));
    Print(sum->terms[i].loopStart);
    printf("..");
    Print(sum->terms[i].loopEnd);
    printf("\n");
  }

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(sum,temp);
  Print(fullExpression);

  if(printNewLine){
    printf("\n");
  }
}

void Repr(StringBuilder* builder,LoopLinearSum* sum){
  TEMP_REGION(temp,builder->arena);
  
  int size = sum->terms.size;

  for(int i = size - 1; i >= 0; i--){
    builder->PushString("for %.*s ",UN(sum->terms[i].var));
    Repr(builder,sum->terms[i].loopStart);
    builder->PushString("..");
    Repr(builder,sum->terms[i].loopEnd);
    builder->PushString("\n");
  }

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(sum,temp);
  Repr(builder,fullExpression);
}

String PushRepr(LoopLinearSum* sum,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);
  Repr(builder,sum);
  String res = EndString(out,builder);
  return res;
}

// ============================================================================
// Global special Symbolic Expressions 

static SymbolicExpression SYM_INST_zero      = {.type = SymbolicExpressionType_LITERAL,.literal = 0};
static SymbolicExpression SYM_INST_one       = {.type = SymbolicExpressionType_LITERAL,.literal = 1};
static SymbolicExpression SYM_INST_two       = {.type = SymbolicExpressionType_LITERAL,.literal = 2};
static SymbolicExpression SYM_INST_eight     = {.type = SymbolicExpressionType_LITERAL,.literal = 8};
static SymbolicExpression SYM_INST_thirtyTwo = {.type = SymbolicExpressionType_LITERAL,.literal = 32};

static SymbolicExpression SYM_INST_dataW    = {.type = SymbolicExpressionType_VARIABLE,.variable = "DATA_W"};
static SymbolicExpression SYM_INST_addrW    = {.type = SymbolicExpressionType_VARIABLE,.variable = "ADDR_W"};
static SymbolicExpression SYM_INST_axiAddrW = {.type = SymbolicExpressionType_VARIABLE,.variable = "AXI_ADDR_W"};
static SymbolicExpression SYM_INST_axiDataW = {.type = SymbolicExpressionType_VARIABLE,.variable = "AXI_DATA_W"};
static SymbolicExpression SYM_INST_delayW   = {.type = SymbolicExpressionType_VARIABLE,.variable = "DELAY_W"};
static SymbolicExpression SYM_INST_lenW     = {.type = SymbolicExpressionType_VARIABLE,.variable = "LEN_W"};

SymbolicExpression* SYM_zero = &SYM_INST_zero;
SymbolicExpression* SYM_one = &SYM_INST_one;
SymbolicExpression* SYM_two = &SYM_INST_two;
SymbolicExpression* SYM_eight = &SYM_INST_eight;
SymbolicExpression* SYM_thirtyTwo = &SYM_INST_thirtyTwo;

SymbolicExpression* SYM_dataW = &SYM_INST_dataW;
SymbolicExpression* SYM_addrW = &SYM_INST_addrW;
SymbolicExpression* SYM_axiAddrW = &SYM_INST_axiAddrW;
SymbolicExpression* SYM_axiDataW = &SYM_INST_axiDataW;
SymbolicExpression* SYM_delayW = &SYM_INST_delayW;
SymbolicExpression* SYM_lenW = &SYM_INST_lenW;

static SymbolicExpression  SYM_INST_axiStrobeW  = {.type = SymbolicExpressionType_DIV,.top = SYM_axiDataW,.bottom = SYM_eight};
static SymbolicExpression  SYM_INST_dataStrobeW = {.type = SymbolicExpressionType_DIV,.top = SYM_dataW,.bottom = SYM_eight};

SymbolicExpression* SYM_axiStrobeW = &SYM_INST_axiStrobeW;
SymbolicExpression* SYM_dataStrobeW = &SYM_INST_dataStrobeW;




#include "newParser.hpp"

SymbolicExpression* ParseSymbolicExpressionTest(String content,Arena* out){
  return nullptr;
#if 0
  TEMP_REGION(temp,out);

  FREE_ARENA(parseArena);

  auto tokenizer = [](const char* start,const char* end) -> TokenizeResult {
    TokenizeResult result = ParseWhitespace(start,end);
    result |= ParseSymbols(start,end);
    result |= ParseNumber(start,end);
    result |= ParseIdentifier(start,end);
    return result;
  };
  
  Parser* parser = StartParsing(content,tokenizer,parseArena);

  DEBUG_BREAK();
  
  while(!parser->Done()){
    NewToken tok = parser->NextToken();

    String repr = PushRepr(temp,tok);
    printf("%.*s\n",UN(repr));
  }

  return nullptr;
#endif
}
