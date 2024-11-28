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
    return 4;
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '*'){
      return 3;
    } else {
      return 2;
    }
  } break;
  case SymbolicExpressionType_OP:{
    return 1;
  } break;
  }
}

void Print(SymbolicExpression* expr,bool top,int parentBindingStrength){
  if(expr->negative){
    printf("-");
  }

  int bindingStrength = TypeToBingingStrength(expr);
  bool bind = (parentBindingStrength >= bindingStrength);
  //bind = true;
  switch(expr->type){
  case SymbolicExpressionType_VARIABLE: {
    if(expr->negative && top){
      //printf("-");
    }
    printf("%.*s",UNPACK_SS(expr->variable));
  } break;
  case SymbolicExpressionType_LITERAL: {
    if(expr->negative && top){
      //printf("-");
    }
    printf("%d",expr->literal);
  } break;
  case SymbolicExpressionType_OP: {
    bool hasNonOpSon = (expr->left->type != SymbolicExpressionType_OP && expr->right->type != SymbolicExpressionType_OP);
    //hasNonOpSon = true;
    
    if(expr->negative && top){
      //printf("-");
    }
    
    if(hasNonOpSon && !top && bind){
      printf("(");
    }

    Print(expr->left,false,bindingStrength);

    printf("%c",expr->op);
    
    Print(expr->right,false,bindingStrength);
    if(hasNonOpSon && !top && bind){
      printf(")");
    }
  } break;
  case SymbolicExpressionType_ARRAY: {
    if(expr->negative){
      bind = true;
      //printf("-");
    }

    if(!top && bind) printf("(");
    bool first = true;
    for(SymbolicExpression* s : expr->sum){
      if(!first){
        if(s->negative && expr->op == '+'){
          //printf("-"); // Do not print anything  because we already printed the '-'
        } else {
          printf("%c",expr->op);
        }
      } else {
        //if(s->negative && expr->op == '+'){
        //  printf("-");
        //}
      }
      Print(s,false,bindingStrength);
      first = false;
    }
    if(!top && bind) printf(")");
  } break;
  }
  if(top){
    printf("\n");
  }
}

#define EXPECT(TOKENIZER,STR) \
  TOKENIZER->AssertNextToken(STR)

static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out,Arena* temp);

SymbolicExpression* PushLiteral(Arena* out,int value,bool negate = false){
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

SymbolicExpression* PushVariable(Arena* out,String name,bool negate = false){
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
  expr->type = SymbolicExpressionType_ARRAY;
  expr->op = '*';
  expr->negative = negate;

  return expr;
}

SymbolicExpression* PushAddBase(Arena* out,bool negate = false){
  SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);
  *expr = {};
  expr->type = SymbolicExpressionType_ARRAY;
  expr->op = '+';
  expr->negative = negate;

  return expr;
}

static int GetLiteralValue(SymbolicExpression* expr){
  Assert(expr->type == SymbolicExpressionType_LITERAL);
  Assert(expr->literal >= 0);
  
  // TODO: All literals should not be zero and negative at the same time.
  //       For now we just assert everytime we check a literal, we are bound to catch all zones of code where a negative - negative literal is ever built.
  Assert(!expr->negative || expr->literal != 0);
  
  if(expr->negative){
    return -expr->literal;
  } else {
    return expr->literal;
  }
}

static SymbolicExpression* ParseTerm(Tokenizer* tok,Arena* out,Arena* temp){
  Token token = tok->NextToken();

  bool negative = false;
  if(CompareString(token,"-")){
    negative = true;
    token = tok->NextToken();
  }

  if(CompareString(token,"(")){
    SymbolicExpression* exprInside = ParseExpression(tok,out,temp);

    EXPECT(tok,")");

    exprInside->negative = negative;
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
  return nullptr;
}

static SymbolicExpression* ParseMul(Tokenizer* tok,Arena* out,Arena* temp){
  SymbolicExpression* left = ParseTerm(tok,out,temp);
  
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
    
    SymbolicExpression* expr = ParseTerm(tok,out,temp);
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }

  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_ARRAY;
  res->sum = PushArrayFromList(out,expressions);
  res->op = '*';

  return res;
}

static SymbolicExpression* ParseDiv(Tokenizer* tok,Arena* out,Arena* temp){
  SymbolicExpression* left = ParseMul(tok,out,temp);

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

    expr->type = SymbolicExpressionType_OP;
    expr->op = op;
    expr->left = current;
    expr->right = ParseMul(tok,out,temp);

    current = expr;
  }

  return current;
}

static SymbolicExpression* ParseSum(Tokenizer* tok,Arena* out,Arena* temp){
  SymbolicExpression* left = ParseDiv(tok,out,temp);

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
      
    SymbolicExpression* expr = ParseDiv(tok,out,temp);
    expr->negative = negative;
    *expressions->PushElem() = expr;
  }

  if(OnlyOneElement(expressions)){
    return left;
  }
  
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  res->type = SymbolicExpressionType_ARRAY;
  res->sum = PushArrayFromList(out,expressions);
  res->op = '+';

  return res;
}
  
static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out,Arena* temp){
  return ParseSum(tok,out,temp);
}

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out,Arena* temp){
  // TODO: For now, only handle the simplest expressions and parenthesis
  if(!tmpl){
    tmpl = CreateTokenizerTemplate(globalPermanent,"+-*/();",{}); 
  }

  TOKENIZER_REGION(tok,tmpl);

  SymbolicExpression* expr = ParseExpression(tok,out,temp);

  return expr;
}

Array<SymbolicExpression*> GetAllExpressions(SymbolicExpression* top,Arena* out){
  auto GetAllExpressionsRecurse = [](auto recurse,SymbolicExpression* expr,DynamicArray<SymbolicExpression*>& arr) -> void{
    *arr.PushElem() = expr;

    switch(expr->type){
    case SymbolicExpressionType_ARRAY:{
      for(SymbolicExpression* s : expr->sum){
        recurse(recurse,s,arr);
      }
    } break;
    case SymbolicExpressionType_OP:{
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

void CheckIfSymbolicExpressionsShareNodes(SymbolicExpression* left,SymbolicExpression* right,Arena* temp){
  BLOCK_REGION(temp);

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
static SymbolicExpression* Default(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return CopyExpression(expr,out);
  } break;
  case SymbolicExpressionType_ARRAY:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArray<SymbolicExpression*>(out,expr->sum.size);
    for(int i = 0; i <  expr->sum.size; i++){
      SymbolicExpression* s  =  expr->sum[i];
      res->sum[i] = Default(s,out,temp);
    }
    
    return res;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = Default(expr->left,out,temp);
    res->right = Default(expr->right,out,temp);
    
    return res;
  } break;
  }
}

typedef SymbolicExpression* (*ApplyFunction)(SymbolicExpression* expr,Arena* out,Arena* temp);

Array<SymbolicExpression*> ApplyFunctionToArray(ApplyFunction Function,Array<SymbolicExpression*> arr,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  ArenaList<SymbolicExpression*>* builder = PushArenaList<SymbolicExpression*>(temp);
  for(SymbolicExpression* spec : arr){
    *builder->PushElem() = Function(spec,out,temp);
  }

  Array<SymbolicExpression*> result = PushArrayFromList(out,builder);

  return result;
}

SymbolicExpression* ApplyGeneric(SymbolicExpression* expr,Arena* out,Arena* temp,ApplyFunction Function){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;
  case SymbolicExpressionType_ARRAY:{
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->sum){
      *list->PushElem() = Function(s,out,temp);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArrayFromList(out,list);

    return res;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = Function(expr->left,out,temp);
    res->right = Function(expr->right,out,temp); 

    return res;
  } break;
  }
}

// The main ideia is to remove negative considerations from other code and let this function handle everything.
// At the very least, let this handle the majority of cases
SymbolicExpression* NormalizeNegative(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;
  case SymbolicExpressionType_OP:{
    NOT_IMPLEMENTED("yet");
    return expr; // Not recursing, only handles 1 layer, used by Apply functions
  } break;
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '*'){
      Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->sum.size);
      for(int i = 0; i < children.size; i++){
        children[i] = NormalizeNegative(expr->sum[i],out,temp);
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
      return Default(expr,out,temp); // We could move negatives downwards or upwards, but I do not think there is any good way of doing things here.
    }
  } break;
  }
  
  NOT_POSSIBLE();
}



// All functions should work recursively
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = RemoveParenthesis(expr->left,out,temp);
    res->right = RemoveParenthesis(expr->right,out,temp);
    return res;
  } break;
  case SymbolicExpressionType_ARRAY:{
    bool containsChange = false;

    Array<SymbolicExpression*> children = ApplyFunctionToArray(RemoveParenthesis,expr->sum,temp,out);
    
    auto UpliftChildCond = [](char op,SymbolicExpression* child){
      bool res = (child->type == SymbolicExpressionType_ARRAY && (child->op == op || child->sum.size == 1));
      return res;
    };
    
    for(SymbolicExpression* child : children){
      //Print(child);
      if(UpliftChildCond(expr->op,child)){
        containsChange = true;
        break;
      }
    }

    if(containsChange){
      ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);

      for(SymbolicExpression* child : children){
        if(UpliftChildCond(expr->op,child)){
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
    }

    return Default(expr,out,temp);
  } break;
  }
}

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;
  case SymbolicExpressionType_ARRAY:{
    ArenaList<SymbolicExpression*>* childs = PushArenaList<SymbolicExpression*>(temp);
    ArenaList<SymbolicExpression*>* literals = PushArenaList<SymbolicExpression*>(temp);
    
    for(int i = 0; i < expr->sum.size; i++){
      SymbolicExpression* child = NormalizeLiterals(expr->sum[i],out,temp);

      if(child->type == SymbolicExpressionType_LITERAL){
        *literals->PushElem() = child;
      } else {
        *childs->PushElem() = child;
      }
    }
    
    if(Empty(literals)){
      return Default(expr,out,temp);
    }
    
    Array<SymbolicExpression*> allLiterals = PushArrayFromList(temp,literals);
    
    if(allLiterals.size == 1){
      // Remove zero from consideration
      if(GetLiteralValue(allLiterals[0]) == 0){
        // Unless it is the only member
        if(Empty(childs)){
          return Default(expr,out,temp);
        }
#if 1
        if(expr->op == '*'){ // Anything multiplied by zero is zero. The question is whether this logic makes sense here
          return allLiterals[0];
        }
#endif
        
        SymbolicExpression* withoutLiteral= CopyExpression(expr,out);
        withoutLiteral->sum = PushArrayFromList(out,childs);
        return withoutLiteral;
      }

      return Default(expr,out,temp); // No change
    }

    bool isMul = expr->op == '*'; 
    
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

    SymbolicExpression* finalLiteral = PushLiteral(out,finalResult,negate);

    *childs->PushElem() = finalLiteral;

    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArrayFromList(out,childs);
    return res;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* copy = CopyExpression(expr,out);
    copy->left = NormalizeLiterals(expr->left,out,temp);
    copy->right = NormalizeLiterals(expr->right,out,temp);
    return copy;
  } break;
  }
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
  case SymbolicExpressionType_ARRAY:{    
    SymbolicExpression* literal = nullptr;

    auto builder = StartGrowableArray<SymbolicExpression*>(out); 
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
    mul->type = SymbolicExpressionType_ARRAY;
    mul->sum = EndArray(builder);
    mul->op = '*';

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
  case SymbolicExpressionType_OP:{
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
  case SymbolicExpressionType_OP: {
    return ExpressionEqual(left->left,right->left) && ExpressionEqual(left->right,right->right);
  }
  case SymbolicExpressionType_ARRAY: {
    if(left->op != right->op){
      return false;
    }

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
}

SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: //fallthrough
  case SymbolicExpressionType_VARIABLE: //fallthrough
  case SymbolicExpressionType_OP:
    return ApplyGeneric(expr,out,temp,ApplySimilarTermsAddition);

  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '*'){
      return ApplyGeneric(expr,out,temp,ApplySimilarTermsAddition);
    }

    SymbolicExpression* normalized = NormalizeLiterals(expr,out,temp);

    Array<TermsWithLiteralMultiplier> terms = PushArray<TermsWithLiteralMultiplier>(temp,normalized->sum.size);
    for(int i = 0; i <  normalized->sum.size; i++){
      SymbolicExpression* s = normalized->sum[i];
      terms[i] = CollectTermsWithLiteralMultiplier(s,temp);
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
        
        finalMul->type = SymbolicExpressionType_ARRAY;
        finalMul->op = '*';
        finalMul->negative = finalLiteral->negative;
        finalLiteral->negative = false;
        finalMul->sum = subTerms;
        
        *finalExpressions->PushElem() = finalMul;
      }
    }

    if(OnlyOneElement(finalExpressions)){
      return finalExpressions->head->elem;
    }
    
    SymbolicExpression* sum = PushStruct<SymbolicExpression>(out);
    sum->type = SymbolicExpressionType_ARRAY;
    sum->op = '+';
    sum->sum = PushArrayFromList(out,finalExpressions);
    
    return sum;
  } break;
  }
}

SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out,Arena* temp){
  // NOTE: Rule when it comes to negative. Every SymbolicExpression* that is eventually returned must have the negative field set. If any other Apply or such expression is called, (like RemoveParenthesis) then the field is set before calling such functions. Do this and negative should not be a problem. Not sure though.
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;

  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '+'){
      // Nothing to do expect apply distributivity to childs
      ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
      for(SymbolicExpression* spec : expr->sum){
        *expressions->PushElem() = ApplyDistributivity(spec,out,temp);
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_ARRAY;
      res->op = '+';
      res->negative = expr->negative;
      res->sum = PushArrayFromList(out,expressions);

      return res;
    } else if(expr->op == '*'){
      bool containsAddition = false;
      for(SymbolicExpression* spec : expr->sum){
        if(spec->type == SymbolicExpressionType_ARRAY && spec->op == '+'){
          containsAddition = true;
        }
      }

      if(!containsAddition){
        ArenaList<SymbolicExpression*>* muls = PushArenaList<SymbolicExpression*>(temp);
        for(SymbolicExpression* spec : expr->sum){
          *muls->PushElem() = ApplyDistributivity(spec,out,temp);
        }

        SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
        res->type = SymbolicExpressionType_ARRAY;
        res->op = '*';
        res->negative = expr->negative;
        res->sum = PushArrayFromList(out,muls);
        return res;
      }
      
      // A multiplication of sums becomes a sum of multiplications
      ArenaList<SymbolicExpression*>* sums = PushArenaList<SymbolicExpression*>(temp);

      // We only distribute one member and then let ApplyDistributivity on the result handle more distributions.
      // That is because if we ended doing everything here, we would enter the case where (x + y) * (a + b) would create 8 terms instead of the expected 4 terms. [Because we would generate x * (a + b), y * (a + b),a * (x + y) and b * (x + y) which eventually would give us 8 terms.]
      // There is probably a better solution, but I currently only need something that works fine for implementing the Address Gen.
      for(SymbolicExpression* spec : expr->sum){
        if(spec->type == SymbolicExpressionType_ARRAY && spec->op == '+'){
          for(SymbolicExpression* subTerm : spec->sum){
            ArenaList<SymbolicExpression*>* mulTerms = PushArenaList<SymbolicExpression*>(temp);
            *mulTerms->PushElem() = subTerm;
            for(SymbolicExpression* other : expr->sum){
              if(spec == other){
                continue;
              }

              *mulTerms->PushElem() = other;
            }

            SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
            res->type = SymbolicExpressionType_ARRAY;
            res->op = '*';
            res->negative = (subTerm->negative != expr->negative);
            res->sum = PushArrayFromList(out,mulTerms);
            
            *sums->PushElem() = res;
          }
          break; // Only distribute one member at a time
        }
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_ARRAY;
      res->op = '+';
      res->sum = PushArrayFromList(out,sums);

      //Print(res);
      //printf("\n");
      
      //return res;
      //return ApplyDistributivity(res,out,temp);
      return ApplyDistributivity(res,out,temp);
    } else {
      DEBUG_BREAK(); // Unexpected expr->op
      Assert(false);
    }
  } break;
    
  case SymbolicExpressionType_OP: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = ApplyDistributivity(expr->left,out,temp);;
    res->right = ApplyDistributivity(expr->right,out,temp);;
    
    return res;
  } break;
  }
  
  return nullptr;
}

// Maybe this should just take an array and work from there.

#if 0
SymbolicExpression* OrderSymbols(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out,temp);
  } break;
  case SymbolicExpressionType_ARRAY: {
    
  } break;
  case SymbolicExpressionType_OP: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = OrderSymbols(expr->left,out,temp);
    res->right = OrderSymbols(expr->right,out,temp);
  } break;
  }
}
#endif

SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out,Arena* temp){
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
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '+'){
      Array<SymbolicExpression*> sums = PushArray<SymbolicExpression*>(out,expr->sum.size);
      for(int i = 0; i <  expr->sum.size; i++){
        SymbolicExpression* s  =  expr->sum[i];
        sums[i] = Derivate(s,base,out,temp);
      }
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->sum = sums;

      //Print(addExpr);
      return addExpr;
    } else {
      // Derivative of mul is sum of derivative of one term with the remaining terms intact
      int size = expr->sum.size;
      Array<Pair<SymbolicExpression*,Array<SymbolicExpression*>>> mulTerms = AssociateOneToOthers(expr->sum,temp);
      DEBUG_BREAK();
      
      Array<SymbolicExpression*> addTerms = PushArray<SymbolicExpression*>(out,size);
      for(int i = 0; i <  mulTerms.size; i++){
        SymbolicExpression* oneTerm = mulTerms[i].first;
        Array<SymbolicExpression*> otherTerms = mulTerms[i].second;
        
        SymbolicExpression* derivative = Derivate(oneTerm,base,out,temp);
        
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
  case SymbolicExpressionType_OP:{
    NOT_IMPLEMENTED("yet"); // Only care about divisions by constants, since other divisions we probably cannot handle in the address gen.
  } break;
  }

  return nullptr;
}

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,Arena* temp){
  SymbolicExpression* current = expr;
  SymbolicExpression* next = nullptr;

  bool print = true;
  
  // Better way of detecting end
  for(int i = 0; i < 2; i++){
    BLOCK_REGION(temp);
    if(print) printf("%d:\n",i);

    //DEBUG_BREAK();
    next = NormalizeLiterals(current,out,temp);
    CheckIfSymbolicExpressionsShareNodes(current,next,temp);
    current = next;
    if(print) printf("A:");
    if(print) Print(current);

    next = ApplyDistributivity(current,out,temp);
    CheckIfSymbolicExpressionsShareNodes(current,next,temp);
    current = next;
    if(print) printf("B:");
    if(print) Print(current);

#if 1
    next = RemoveParenthesis(current,out,temp);
    //CheckIfSymbolicExpressionsShareNodes(current,next,temp); // RemoveParenthesis does not recurse, so obviously this will fail
    current = next;
    if(print) printf("C:");
    if(print) Print(current);
#endif

    next = ApplySimilarTermsAddition(current,out,temp);
    CheckIfSymbolicExpressionsShareNodes(current,next,temp);
    current = next;
    if(print) printf("D:");
    if(print) Print(current);

#if 1
    next = RemoveParenthesis(current,out,temp);
    CheckIfSymbolicExpressionsShareNodes(current,next,temp); 
    current = next;
    if(print) printf("E:");
    if(print) Print(current);
#endif

    
  }

  return current;
}

void TestPrint(Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  String examples[] = {
    STRING("1 + 2 + (3 + 4) + 5"),
    STRING("(a + (b + (c * d)))"),
    STRING("(a + ((b + c) + d) + a)"),
    STRING("(a + (((((a) + b) + c) + d) + e) + f)"),
    STRING("(1*2*3) + (a*b*c)"),
    STRING("(1*-2*-3) - (-a*b*c)")
  };

  for(String s : examples){
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp,temp2);
    Print(sym);
  }
}

void TestRemoveParenthesis(Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  String examples[] = {
    STRING("a - (((b - c) - e) - f)"),
    STRING("a - (b - (c - (d - e)))")
    //STRING("(a + (((((a) + b) + c) + d) + e) + f)")
    //STRING("1 + 2 + (3 + 4) + 5"),
    //STRING("(a + (b + (c * d)))"),
    //STRING("(a + ((b + c) + d) + a)"),
    //STRING("(1*2*3) + (a*b*c)")
  };

  for(String s : examples){
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp,temp2);

    DEBUG_BREAK();
    SymbolicExpression* normalized = RemoveParenthesis(sym,temp,temp2);
    Print(sym);
    Print(normalized);
    printf("\n");
  }
}

void TestDerivative(Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  String examples[] = {
    STRING("1 + a + b + a * b")
  };

  for(String s : examples){
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp,temp2);

    DEBUG_BREAK();
    SymbolicExpression* deriveA = Derivate(sym,STRING("a"),temp,temp2);
    SymbolicExpression* deriveB = Derivate(sym,STRING("b"),temp,temp2);

    deriveA = RemoveParenthesis(NormalizeLiterals(deriveA,temp,temp2),temp,temp2);
    deriveB = RemoveParenthesis(NormalizeLiterals(deriveB,temp,temp2),temp,temp2);

    Print(deriveA);
    Print(deriveB);
  }
}

void TestSymbolic(Arena* temp,Arena* temp2){
  String examples[] = {
    //STRING("a+b+a-b"),
    STRING("a+b+c+d"),
    STRING("a-a-b-b-2*c - c"),
    STRING("a+a+b+b+2*c + c"),
    STRING("-a-b-(a-b)-(-a+b)-(-(a-b)-(-a+b)+(a-b) + (-a+b))"),
    STRING("(a-b)*(a-b)"),
    STRING("a+b+a-b+a*b+a/b+a-(a-b)+a*(a+b)+a/(a+b)+(a+b) * (a-b)+(a-b)/(a+b)"),
    STRING("a*b + a*b + 2*a*b"),
    STRING("1+2+3+1*20*30"),
    STRING("1 * 2"),
    STRING("a * (x + y)"),
    STRING("(x-y) * a"),
    STRING("a*b*c"),
    STRING("(x-y)"),
    STRING("1 + 2 + 3 + 4"),
    STRING("a+b+a-b"),
    STRING("(a - b) * (x + y)"),
    STRING("2 * a * (x + y) * (a + b) + 2 * x / 10 + (x + y) / (x - y) + 4 * x * y + 1 + 2 + 3 + 4")
  };

#if 0
  TestPrint(temp,temp2);
#endif
#if 0
  TestRemoveParenthesis(temp,temp2);
#endif
  TestDerivative(temp,temp2);
  
#if 0
  for(String s : examples){
    BLOCK_REGION(temp);
    BLOCK_REGION(temp2);
    
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp,temp2);
    
    //SymbolicExpression* normalized = ApplyDistributivity(sym,temp,temp2);
    //SymbolicExpression* normalized = ApplySimilarTermsAddition(sym,temp,temp2);
    //SymbolicExpression* normalized = NormalizeLiterals(sym,temp,temp2);
    SymbolicExpression* normalized = Normalize(sym,temp,temp2);
    Print(sym);
    Print(normalized);
    printf("\n");
  }
#endif
}
