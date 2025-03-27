#include "symbolic.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

static TokenizerTemplate* tmpl;

int TypeToBingingStrength(SymbolicExpression* expr){
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
  } break;
  }
  NOT_POSSIBLE();
}

static char GetOperationSymbol(SymbolicExpression* expr){
  switch(expr->type){
  case SymbolicExpressionType_SUM: return '+';
  case SymbolicExpressionType_MUL: return '*';
  case SymbolicExpressionType_DIV: return '/';
  default: return ' ';
  }
}

void BuildRepresentation(StringBuilder* builder,SymbolicExpression* expr,bool top,int parentBindingStrength){
  if(expr->negative){
    builder->PushString("-");
  }

  int bindingStrength = TypeToBingingStrength(expr);
  bool bind = (parentBindingStrength >= bindingStrength);
  //bind = true;
  switch(expr->type){
  case SymbolicExpressionType_VARIABLE: {
    builder->PushString("%.*s",UNPACK_SS(expr->variable));
  } break;
  case SymbolicExpressionType_LITERAL: {
    builder->PushString("%d",expr->literal);
  } break;
  case SymbolicExpressionType_DIV: {
    bool hasNonOpSon = (expr->left->type != SymbolicExpressionType_DIV && expr->right->type != SymbolicExpressionType_DIV);
    if(hasNonOpSon && !top && bind){
      builder->PushString("(");
    }

    BuildRepresentation(builder,expr->left,false,bindingStrength);

    builder->PushString("/");
    
    BuildRepresentation(builder,expr->right,false,bindingStrength);
    if(hasNonOpSon && !top && bind){
      builder->PushString(")");
    }
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
  //case SymbolicExpressionType_ARRAY: {
    if(bind){
      builder->PushString("(");
    }

    bool first = true;
    for(SymbolicExpression* s : expr->sum){
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

String PushRepresentation(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  
  auto builder = StartString(temp);
  BuildRepresentation(builder,expr,true,0);
  return EndString(out,builder);
}

void Print(SymbolicExpression* expr){
  TEMP_REGION(temp,nullptr);
  
  String res = PushRepresentation(expr,temp);
  printf("%.*s\n",UNPACK_SS(res));
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
    printf("VAR: %.*s",UNPACK_SS(expr->variable));
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    printf("ARR: %c {\n",GetOperationSymbol(expr));

    for(SymbolicExpression* child : expr->sum){
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

    PrintAST(expr->left,level + 1);
    PrintAST(expr->right,level + 1);
  } break;
  }

  if(level == 0){
    printf("\n"); // Assuming that no one wants to print after an entire ast
  }
}

int Evaluate(SymbolicExpression* expr,Hashmap<String,int>* values){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:{
    return expr->literal;
  } break;
  case SymbolicExpressionType_VARIABLE:{
    return values->GetOrFail(expr->variable);
  } break;
  case SymbolicExpressionType_DIV:{
    int left = Evaluate(expr->left,values);
    int right = Evaluate(expr->right,values);

    if(right == 0){
      PRINTF_WITH_LOCATION("Division by zero");
      DEBUG_BREAK();
    }

    return (left / right);
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL:{
    bool isMul = (expr->type == SymbolicExpressionType_MUL);
    int value = isMul ? 1 : 0;

    for(SymbolicExpression* child : expr->sum){
      int subVal = Evaluate(child,values);

      if(isMul){
        value *= subVal;
      } else {
        value += subVal;
      }
    }

    return value;
    
  } break;
  }

  NOT_POSSIBLE("All cases handled");
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

static int GetLiteralValue(SymbolicExpression* expr){
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

SymbolicExpression* SymbolicAdd(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  // TODO: We are always creating a new node even when it might be possible not to.
  //       Assuming that the normalization function is capable of collapsing everything into one, this should not pose a problem, as long as we normalize the end result, but not sure and nevertheless this is a bit slower.
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->sum = PushArray<SymbolicExpression*>(out,2);

  res->sum[0] = left;
  res->sum[1] = right;

  return res;
}

SymbolicExpression* SymbolicSub(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  // TODO: We are always creating a new node even when it might be possible not to.
  //       Assuming that the normalization function is capable of collapsing everything into one, this should not pose a problem, as long as we normalize the end result, but not sure and nevertheless this is a bit slower.
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->sum = PushArray<SymbolicExpression*>(out,2);

  SymbolicExpression* actualRight = SymbolicDeepCopy(right,out);
  actualRight->negative = !actualRight->negative;
  
  res->sum[0] = left;
  res->sum[1] = actualRight;

  return res;
}

SymbolicExpression* SymbolicMult(SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->sum = PushArray<SymbolicExpression*>(out,2);

  res->sum[0] = left;
  res->sum[1] = right;

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
      SymbolicExpression* variable = PushStruct<SymbolicExpression>(out);
      variable->type = SymbolicExpressionType_VARIABLE;
      variable->variable = PushString(out,token);
      variable->negative = negative;
      return variable;
    }
  }

  NOT_POSSIBLE();
}

static SymbolicExpression* ParseMul(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseTerm(tok,out);
  
  ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
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
    
    SymbolicExpression* expr = ParseTerm(tok,out);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_MUL;
  res->sum = PushArrayFromList(out,expressions);

  return res;
}

static SymbolicExpression* ParseDiv(Tokenizer* tok,Arena* out){
  SymbolicExpression* left = ParseMul(tok,out);

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
    expr->left = current;
    expr->right = ParseMul(tok,out);

    current = expr;
  }

  return current;
}

static SymbolicExpression* ParseSum(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* left = ParseDiv(tok,out);

  ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
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
      
    SymbolicExpression* expr = ParseDiv(tok,out);
    expr->negative = (expr->negative != negative);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_SUM;
  res->sum = PushArrayFromList(out,expressions);

  return res;
}
  
static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out){
  return ParseSum(tok,out);
}

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out){
  // TODO: For now, only handle the simplest expressions and parenthesis
  if(!tmpl){
    tmpl = CreateTokenizerTemplate(globalPermanent,"+-*/();[]",{}); // TODO: It is impossible to fully abstract the tokenizer as long as we have the template not being correctly constructed. We need a way of describing the things that we want (including digits), instead of just describing what symbols are or not allowed
  }

  TOKENIZER_REGION(tok,tmpl);

  SymbolicExpression* expr = ParseExpression(tok,out);

  return expr;
}

SymbolicExpression* ParseSymbolicExpression(String content,Arena* out){
  Tokenizer tok(content,"",{});

  return ParseSymbolicExpression(&tok,out);
}

Array<SymbolicExpression*> GetAllExpressions(SymbolicExpression* top,Arena* out){
  auto GetAllExpressionsRecurse = [](auto recurse,SymbolicExpression* expr,GrowableArray<SymbolicExpression*>& arr) -> void{
    *arr.PushElem() = expr;

    switch(expr->type){
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
      for(SymbolicExpression* s : expr->sum){
        recurse(recurse,s,arr);
      }
    } break;
    case SymbolicExpressionType_DIV:{
      recurse(recurse,expr->left,arr);
      recurse(recurse,expr->right,arr);
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
        DEBUG_BREAK();
        Assert(false);
      }
    }
  }
}

// Children are not copied, since they must come from the recursive function logic
static SymbolicExpression* CopyExpression(SymbolicExpression* in,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  *res = *in;
  return res;
}

// By default just copy the things. Since we are using arenas, allocations and deallocations are basically free anyway. We only care about the final expression, so we can just allocate a bunch of nodes that are immediatly deallocated. The extra copy is the worst part, but since we only care about simple expressions for now, probably nothing that needs attention for now. Can always add a better implementation later.
SymbolicExpression* SymbolicDeepCopy(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return CopyExpression(expr,out);
  } break;
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArray<SymbolicExpression*>(out,expr->sum.size);
    for(int i = 0; i <  expr->sum.size; i++){
      SymbolicExpression* s  =  expr->sum[i];
      res->sum[i] = SymbolicDeepCopy(s,out);
    }
    
    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = SymbolicDeepCopy(expr->left,out);
    res->right = SymbolicDeepCopy(expr->right,out);
    
    return res;
  } break;
  }
  NOT_POSSIBLE();
}

typedef SymbolicExpression* (*ApplyFunction)(SymbolicExpression* expr,Arena* out);
typedef SymbolicExpression* (*ApplyNonRecursiveFunction)(SymbolicExpression* expr,Arena* out);

Array<SymbolicExpression*> ApplyFunctionToArray(ApplyFunction Function,Array<SymbolicExpression*> arr,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<SymbolicExpression*>* builder = PushArenaList<SymbolicExpression*>(temp);
  for(SymbolicExpression* spec : arr){
    *builder->PushElem() = Function(spec,out);
  }

  Array<SymbolicExpression*> result = PushArrayFromList(out,builder);

  return result;
}

SymbolicExpression* ApplyNonRecursive(SymbolicExpression* expr,Arena* out,ApplyNonRecursiveFunction Function){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return Function(expr,out);
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->sum){
      *list->PushElem() = ApplyNonRecursive(s,out,Function);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArrayFromList(out,list);

    res = Function(res,out);
    
    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = ApplyNonRecursive(expr->left,out,Function);
    res->right = ApplyNonRecursive(expr->right,out,Function); 

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
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->sum){
      *list->PushElem() = Function(s,out);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArrayFromList(out,list);

    return res;
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = Function(expr->left,out);
    res->right = Function(expr->right,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

// The main ideia is to remove negative considerations from other code and let this function handle everything.
// At the very least, let this handle the majority of cases
SymbolicExpression* NormalizeNegative(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
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
      Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->sum.size);
      for(int i = 0; i < children.size; i++){
        children[i] = NormalizeNegative(expr->sum[i],out);
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
      copy->sum = children;

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
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = RemoveParenthesis(expr->left,out);
    res->right = RemoveParenthesis(expr->right,out);
    return res;
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    if(expr->sum.size == 1){
      SymbolicExpression* onlyOne = RemoveParenthesis(expr->sum[0],out);
      onlyOne->negative = (onlyOne->negative != expr->negative);
      return onlyOne;
    }
    
    Array<SymbolicExpression*> children = ApplyFunctionToArray(RemoveParenthesis,expr->sum,out);
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    
    for(SymbolicExpression* child : children){
      bool uplift = (child->type == expr->type);
      if(uplift){
        for(SymbolicExpression* subChild : child->sum){
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
    res->sum = PushArrayFromList(out,list);
    
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
    int size = expr->sum.size;

    if(size <= 1){
      return expr;
    }
    
    // Literals, if exist, always appear first.
    // Assuming only 1 literal, since this is supposed to be called for normalized expressions
    bool didSwap = false;
    for(int i = 0; i < size; i++){
      if(expr->sum[i]->type == SymbolicExpressionType_LITERAL){
        SWAP(expr->sum[0],expr->sum[i]);
        didSwap = true;
      }
    }

    int firstIndex = (didSwap ? 1 : 0);
    for(int i = firstIndex; i < size; i++){
      if(expr->sum[i]->type != SymbolicExpressionType_VARIABLE){
        continue;
      }
      
      for(int ii = i + 1; ii < size; ii++){
        if(expr->sum[ii]->type != SymbolicExpressionType_VARIABLE){
          continue;
        }
        
        if(CompareStringOrdered(expr->sum[i]->variable,expr->sum[ii]->variable) < 0){
          SWAP(expr->sum[i],expr->sum[ii]);
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
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;
  case SymbolicExpressionType_SUM: // fallthrough
  case SymbolicExpressionType_MUL: {
    ArenaList<SymbolicExpression*>* childs = PushArenaList<SymbolicExpression*>(temp);
    ArenaList<SymbolicExpression*>* literals = PushArenaList<SymbolicExpression*>(temp);
    
    for(int i = 0; i < expr->sum.size; i++){
      SymbolicExpression* child = NormalizeLiterals(expr->sum[i],out);

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
      copy->sum = PushArrayFromList(out,childs);
      return copy;
    }
    
    Array<SymbolicExpression*> allLiterals = PushArrayFromList(temp,literals);
    Array<SymbolicExpression*> allChilds = PushArrayFromList(temp,childs);
    
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

    bool negate = false;
    if(finalResult < 0){
      finalResult = -finalResult;
      negate = true;
    }

    if(isMul){
      if(finalResult == 0){
        return PushLiteral(out,0);
      } else {
        SymbolicExpression* res = CopyExpression(expr,out);
        if(finalResult != 1){
          SymbolicExpression* finalLiteral = PushLiteral(out,finalResult,negate);

          *childs->PushElem() = finalLiteral;
        }

        res->sum = PushArrayFromList(out,childs);
        SortTerms(res,out);
        
        return res;
      }
    } else {
      SymbolicExpression* res = CopyExpression(expr,out);
      if(finalResult != 0){
        SymbolicExpression* finalLiteral = PushLiteral(out,finalResult,negate);

        *childs->PushElem() = finalLiteral;
      }

      res->sum = PushArrayFromList(out,childs);
      return res;
    }
  } break;
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* copy = CopyExpression(expr,out);
    copy->left = NormalizeLiterals(expr->left,out);
    copy->right = NormalizeLiterals(expr->right,out);
    return copy;
  } break;
  }
  NOT_POSSIBLE();
}

// Must call normalizeLiteral before calling this one. 
TermsWithLiteralMultiplier CollectTermsWithLiteralMultiplier(SymbolicExpression* expr,Arena* out){
  //Assert(expr->type == SymbolicExpressionType_ARRAY && expr->op == '*');
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:{
    TermsWithLiteralMultiplier res = {};
    res.literal = expr;

    SymbolicExpression* mul = PushMulBase(out);
    mul->sum = PushArray<SymbolicExpression*>(out,0);
    res.mulTerms = mul;
    
    //res.mulTerms = expr;
    return res;
  } break;
  case SymbolicExpressionType_VARIABLE: {
    TermsWithLiteralMultiplier res = {};
    res.literal = PushLiteral(out,1);

    SymbolicExpression* cp = CopyExpression(expr,out);
    
    SymbolicExpression* mul = PushMulBase(out);
    mul->sum = PushArray<SymbolicExpression*>(out,1);
    mul->sum[0] = cp;

    if(cp->negative){
      res.literal->negative = !res.literal->negative;
      cp->negative = false;
    }
    
    res.mulTerms = mul;
    
    return res;
  }
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    SymbolicExpression* literal = nullptr;

    auto builder = StartArray<SymbolicExpression*>(out); 
    for(SymbolicExpression* s : expr->sum){
      if(s->type == SymbolicExpressionType_LITERAL){
        Assert(literal == nullptr);
        literal = s;
      } else {
        *builder.PushElem() = s;
      }
    }

    if(!literal){
      literal = PushLiteral(out,1);
    }
  
    TermsWithLiteralMultiplier result = {};
    result.literal = CopyExpression(literal,out);

    SymbolicExpression* mul = PushStruct<SymbolicExpression>(out);
    mul->type = SymbolicExpressionType_MUL;
    mul->sum = EndArray(builder);

    // Push negative to the mul expression
    bool negative = literal->negative != expr->negative;
    for(SymbolicExpression* s : mul->sum){
      if(s->negative){
        negative = !negative;
        s->negative = false;
      }
    }
    mul->negative = false;
    result.literal->negative = negative;

    result.mulTerms = mul;
    return result;
  } break;
  case SymbolicExpressionType_DIV:{
    // TODO: This is not goodish. Something weird is being done here that kinda of breaks the flow of the code. I think the fact that we must return an expression of type '*' works but is more error prone than we actually need.
    TermsWithLiteralMultiplier result = {};

    SymbolicExpression* thisCopy = CopyExpression(expr,out);
    thisCopy->negative = false;
    SymbolicExpression* mul = PushMulBase(out);
    mul->sum = PushArray<SymbolicExpression*>(out,1);
    mul->sum[0] = thisCopy;
    
    if(expr->left->type == SymbolicExpressionType_LITERAL){
      SymbolicExpression* cp = CopyExpression(expr->left,out);
      cp->negative = (cp->negative != expr->negative);

      result.literal = cp;
      result.mulTerms = mul;
      
      return result;
    } else {
      SymbolicExpression* cp = CopyExpression(expr->left,out);
      cp->negative = (cp->negative != expr->negative);

      result.literal = PushLiteral(out,1,expr->negative);
      result.mulTerms = mul;

      return result;
    }
  } break;
  }
  NOT_POSSIBLE();
}

// Performs no form of normalization or canonicalization whatsoever.
// A simple equality for use by other functions
bool ExpressionEqual(SymbolicExpression* left,SymbolicExpression* right){
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
    return ExpressionEqual(left->left,right->left) && ExpressionEqual(left->right,right->right);
  }
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    if(left->sum.size != right->sum.size){
      return false;
    }

    int size = left->sum.size;
    for(int i = 0; i < size; i++){
      if(!ExpressionEqual(left->sum[i],right->sum[i])){
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
  case SymbolicExpressionType_LITERAL: //fallthrough
  case SymbolicExpressionType_VARIABLE: //fallthrough
  case SymbolicExpressionType_DIV:
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
  case SymbolicExpressionType_MUL: {
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
  } break;
    
  case SymbolicExpressionType_SUM:{
    SymbolicExpression* normalized = NormalizeLiterals(expr,out);

    if(normalized->type != SymbolicExpressionType_SUM){
      return normalized;
    }
    
    Array<TermsWithLiteralMultiplier> terms = PushArray<TermsWithLiteralMultiplier>(temp,normalized->sum.size);
    for(int i = 0; i <  normalized->sum.size; i++){
      SymbolicExpression* s = normalized->sum[i];
      terms[i] = CollectTermsWithLiteralMultiplier(s,out);
    }

    Array<bool> termUsed = PushArray<bool>(temp,terms.size);
    Memset(termUsed,false);

    ArenaList<SymbolicExpression*>* finalExpressions = PushArenaList<SymbolicExpression*>(temp);
    for(int i = 0; i < terms.size; i++){
      if(termUsed[i]){
        continue;
      }

      TermsWithLiteralMultiplier left = terms[i];

      int finalLiteralValue = GetLiteralValue(left.literal);
      for(int j = i + 1; j < terms.size; j++){
        if(termUsed[j]){
          continue;
        }
        
        TermsWithLiteralMultiplier right = terms[j];
        
        if(ExpressionEqual(left.mulTerms,right.mulTerms)){
          termUsed[j] = true;
          finalLiteralValue += GetLiteralValue(right.literal);
        }
      }
      
      if(finalLiteralValue != 0){
        SymbolicExpression* finalLiteral = PushLiteral(out,finalLiteralValue);
        SymbolicExpression* finalMul = PushStruct<SymbolicExpression>(out);
      
        Array<SymbolicExpression*> subTerms = PushArray<SymbolicExpression*>(out,left.mulTerms->sum.size + 1);
        subTerms[0] = finalLiteral;
        for(int i = 0; i < left.mulTerms->sum.size; i++){
          subTerms[i + 1] = left.mulTerms->sum[i];
        }
        
        finalMul->type = SymbolicExpressionType_MUL;
        finalMul->negative = finalLiteral->negative;
        finalLiteral->negative = false;
        finalMul->sum = subTerms;
        
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
    sum->sum = PushArrayFromList(out,finalExpressions);

    Assert(sum->sum.size > 0);
    
    return sum;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out){
  // NOTE: Rule when it comes to negative. Every SymbolicExpression* that is eventually returned must have the negative field set. If any other Apply or such expression is called, (like RemoveParenthesis) then the field is set before calling such functions. Do this and negative should not be a problem. Not sure though.
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return SymbolicDeepCopy(expr,out);
  } break;

  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->sum.size);
    for(int i = 0; i < children.size; i++){
      children[i] = ApplyDistributivity(expr->sum[i],out);
    }

    if(expr->type == SymbolicExpressionType_SUM){
      // Nothing to do except apply distributivity to childs
      SymbolicExpression* res = CopyExpression(expr,out);
      res->sum = children;
      
      return res;
    } else if(expr->type == SymbolicExpressionType_MUL){
      bool containsAddition = false;
      for(SymbolicExpression* spec : expr->sum){
        if(spec->type == SymbolicExpressionType_SUM){
          containsAddition = true;
        }
      }

      if(!containsAddition){
        ArenaList<SymbolicExpression*>* muls = PushArenaList<SymbolicExpression*>(temp);
        for(SymbolicExpression* spec : expr->sum){
          *muls->PushElem() = ApplyDistributivity(spec,out);
        }

        SymbolicExpression* res = CopyExpression(expr,out);
        res->sum = children;
        return res;
      }
      
      // A multiplication of sums becomes a sum of multiplications
      ArenaList<SymbolicExpression*>* sums = PushArenaList<SymbolicExpression*>(temp);

      // We only distribute one member and then let ApplyDistributivity on the result handle more distributions.
      // That is because if we ended doing everything here, we would enter the case where (x + y) * (a + b) would create 8 terms instead of the expected 4 terms. [Because we would generate x * (a + b), y * (a + b),a * (x + y) and b * (x + y) which eventually would give us 8 terms.]
      // There is probably a better solution, but I currently only need something that works fine for implementing the Address Gen.
      for(SymbolicExpression* spec : expr->sum){
        if(spec->type == SymbolicExpressionType_SUM){
          // Spec is the array of additions. (Ex: (a + b) in the (a + b) * (x + y) example

          // For a and b.
          for(SymbolicExpression* subTerm : spec->sum){
            // For each member in the additions array
            
            ArenaList<SymbolicExpression*>* mulTerms = PushArenaList<SymbolicExpression*>(temp);
            *mulTerms->PushElem() = SymbolicDeepCopy(subTerm,out);
            for(SymbolicExpression* other : expr->sum){
              if(spec == other){
                continue;
              }

              *mulTerms->PushElem() = SymbolicDeepCopy(other,out);
            }

            SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
            res->type = SymbolicExpressionType_MUL;
            res->sum = PushArrayFromList(out,mulTerms);
            
            *sums->PushElem() = res;
          }
          break; // Only distribute one member at a time
        }
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_SUM;
      res->negative = expr->negative;
      res->sum = PushArrayFromList(out,sums);

      return res;
    } else {
      Assert(false);
    }
  } break;
    
  case SymbolicExpressionType_DIV: {
    // TODO: Division can be distributed, right?  But still not sure if we need it, we do want to move division up but need to see more usecases before starting looking at this.
    
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = ApplyDistributivity(expr->left,out);
    res->right = ApplyDistributivity(expr->right,out);
    
    return res;
  } break;
  }
  
  return nullptr;
}

SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
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
      Array<SymbolicExpression*> sums = PushArray<SymbolicExpression*>(out,expr->sum.size);
      for(int i = 0; i <  expr->sum.size; i++){
        SymbolicExpression* s  =  expr->sum[i];
        sums[i] = Derivate(s,base,out);
      }
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->sum = sums;

      return addExpr;
    } else {
      // Derivative of mul is sum of derivative of one term with the remaining terms intact
      int size = expr->sum.size;
      Array<Pair<SymbolicExpression*,Array<SymbolicExpression*>>> mulTerms = AssociateOneToOthers(expr->sum,temp);
      
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
        mulExpr->sum = sumArray;
        addTerms[i] = mulExpr;
      }
     
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->sum = addTerms;

      //Print(addExpr);
      return addExpr;
    }
  } break;
  case SymbolicExpressionType_DIV:{
    NOT_IMPLEMENTED("yet"); // Only care about divisions by constants, since other divisions we probably cannot handle in the address gen.
  } break;
  }

  return nullptr;
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
  case SymbolicExpressionType_SUM:
  case SymbolicExpressionType_MUL: {
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->sum.size);
    for(int i = 0; i < children.size; i++){
      children[i] = SymbolicReplace(base->sum[i],varToReplace,replacingExpr,out);
    }

    SymbolicExpression* copy = CopyExpression(base,out);
    copy->sum = children;

    return copy;
  }
  case SymbolicExpressionType_DIV:{
    SymbolicExpression* res = CopyExpression(base,out);
    res->left = SymbolicReplace(base->left,varToReplace,replacingExpr,out);
    res->right = SymbolicReplace(base->right,varToReplace,replacingExpr,out); 

    return res;
  } break;
  }
  NOT_POSSIBLE();
}

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,bool debugPrint){
  TEMP_REGION(temp,out);
  SymbolicExpression* current = expr;
  SymbolicExpression* next = nullptr;

  if(debugPrint){
    printf("Norm start:\n");
    PrintAST(expr);
  }
  
  // Better way of detecting end, should be a check if expression did not change after an entire loop
  for(int i = 0; i < 3; i++){
    BLOCK_REGION(temp);
    if(debugPrint) printf("%d:\n",i);

    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Normalize Literals:\n");
    if(debugPrint) PrintAST(current);

    next = ApplyDistributivity(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Apply distributivity:\n");
    if(debugPrint) PrintAST(current);

    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Remove paran:\n");
    if(debugPrint) PrintAST(current);

    next = ApplySimilarTermsAddition(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("SimilarTerms:\n");
    if(debugPrint) PrintAST(current);

    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next); 
    current = next;
    if(debugPrint) printf("Remove paran:\n");
    if(debugPrint) PrintAST(current);

    // Logic inside similar term might add superflouous constants, so we normalize literals again
    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(debugPrint) printf("Normalize Literals:\n");
    if(debugPrint) PrintAST(current);
  }

  current = ApplyNonRecursive(current,out,SortTerms);
  
  return current;
}

struct TestCase{
  String input;
  String expectedNormalized;
};

#if 1
void TestSymbolic(){
  TEMP_REGION(temp,nullptr);

  TestCase cases[] = {
    {STRING("a+b+c+d"),STRING("a+b+c+d")},
    {STRING("a-a-b-b-2*c-c"),STRING("-2*b-3*c")},
    {STRING("a+a+b+b+2*c+c"),STRING("2*a+2*b+3*c")},
    {STRING("a*b + a * b"),STRING("2*a*b")},
    {STRING("-a * b - a * b"),STRING("-2*a*b")},
    {STRING("-((1*x)*(3-1))+1*y"),STRING("-2*x+y")},
    {STRING("-a-b-(a-b)-(-a+b)-(-(a-b)-(-a+b)+(a-b) + (-a+b))"),STRING("-a-b")},
    {STRING("(a-b)*(a-b)"),STRING("a*a-2*a*b+b*b")},
    {STRING("a*b + a*b + 2*a*b"),STRING("4*a*b")},
    {STRING("1+2+3+1*20*30"),STRING("606")},
    {STRING("a * (x + y)"),STRING("a*x+a*y")}
  };

  bool printNormalizeProcess = false;
 
  printf("\n");
  for(TestCase c : cases){
    BLOCK_REGION(temp);
    SymbolicExpression* sym = ParseSymbolicExpression(c.input,temp);
    SymbolicExpression* normalized = Normalize(sym,temp);

    String repr = PushRepresentation(normalized,temp);

    if(CompareString(repr,c.expectedNormalized)){
      printf("OK");
    } else {
      printf("Different\n");      

      printf("Start:");
      Print(sym);
      printf("End  :");
      Print(normalized);
      printf("Expec:%.*s\n",UNPACK_SS(c.expectedNormalized));
      PrintAST(normalized);

      if(printNormalizeProcess){
        printf("Proccess:\n");
        Normalize(sym,temp,true);
        printf("\n");
      }
    }

    printf("\n");
  }
}
#endif

// Returns first found, assuming that the expression was normalized beforehand otherwise gonna miss literals
static Opt<int> FindLiteralIndex(SymbolicExpression* expr){
  int size = expr->sum.size;
  for(int i = 0; i < size; i++){
    SymbolicExpression* child = expr->sum[i];
    if(child->type == SymbolicExpressionType_LITERAL){
      return i;
    }
  }

  return {};
}

Opt<SymbolicExpression*> GetMultExpressionAssociatedTo(SymbolicExpression* expr,String variableName,Arena* out){
  switch(expr->type){
  case SymbolicExpressionType_SUM:{
    for(SymbolicExpression* child : expr->sum){
      Opt<SymbolicExpression*> res = GetMultExpressionAssociatedTo(child,variableName,out);
      if(res.has_value()){
        return res;
      }
    }
  } break;
  case SymbolicExpressionType_MUL:{
    int size = expr->sum.size;
    int foundIndex = -1;

    for(int i = 0; i < size; i++){
      SymbolicExpression* child = expr->sum[i];

      if(child->type == SymbolicExpressionType_VARIABLE && CompareString(child->variable,variableName)){
        foundIndex = i;
        break;
      }
    }

    if(size == 1){
      return ApplyNegation(PushLiteral(out,1),expr->negative);
    }

    if(size - 1 == 1){
      int childrenIndex = (1 - foundIndex);
      return expr->sum[childrenIndex];
    }
    
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,size - 1);

    int inserted = 0;
    for(int i = 0; i < size; i++){
      SymbolicExpression* child = expr->sum[i];
      
      if(i == foundIndex){
        continue;
      }
      children[inserted++] = child;
    }
    
    SymbolicExpression* res = PushMulBase(out,expr->negative);
    res->sum = children;

    return res;
  } break;
  default: return {};
  }
  return {};
}

SymbolicExpression* RemoveMulLiteral(SymbolicExpression* expr,Arena* out){
  Assert(expr->type == SymbolicExpressionType_MUL);
  int size = expr->sum.size;
  bool negative = expr->negative;

  Opt<int> literalIndexOpt = FindLiteralIndex(expr);
  
  if(!literalIndexOpt.has_value()){
    return SymbolicDeepCopy(expr,out);
  }

  int index = literalIndexOpt.value();

  negative ^= expr->sum[index]->negative;

  if(size == 1){
    DEBUG_BREAK(); // We are primarely using this function on address gen in which this should not happen. Debug break to see if anything is missing, but eventually this function might be used elsewhere and that is ok, remove this then.
    return PushLiteral(out,0);
  }

  Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,size - 1);
  
  int inserted = 0;
  for(int i = 0; i < size; i++){
    SymbolicExpression* child = expr->sum[i];
      
    if(i == index){
      continue;
    }
    children[inserted++] = child;
  }
  
  SymbolicExpression* res = PushMulBase(out,negative);
  res->sum = children;

  return res;
}
