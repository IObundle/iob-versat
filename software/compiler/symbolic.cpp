#include "symbolic.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"

static TokenizerTemplate* tmpl;

void Print(SymbolicExpression* expr){
  switch(expr->type){
  case SymbolicExpressionType_VARIABLE: {
    printf("%.*s",UNPACK_SS(expr->variable));
  } break;
  case SymbolicExpressionType_LITERAL: {
    printf("%d",expr->literal);
  } break;
  case SymbolicExpressionType_OP: {
    bool hasNonOpSon = (expr->left->type != SymbolicExpressionType_OP && expr->right->type != SymbolicExpressionType_OP);

    if(hasNonOpSon){
      printf("B(");
    }

    Print(expr->left);

    printf(" %c ",expr->op);
    
    Print(expr->right);
    if(hasNonOpSon){
      printf(")");
    }
  } break;
  case SymbolicExpressionType_ARRAY: {
    printf("A(");
    bool first = true;
    for(SymbolicExpression* s : expr->sum){
      if(!first){
        printf(" %c ",expr->op);
      }
      Print(s);
      first = false;
    }
    printf(")");
  } break;
  }
}

#define EXPECT(TOKENIZER,STR) \
  TOKENIZER->AssertNextToken(STR)

static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out,Arena* temp);

static SymbolicExpression* ParseTerm(Tokenizer* tok,Arena* out,Arena* temp){
  Token token = tok->NextToken();

  if(CompareString(token,"-")){
    // NOTE: Probably not a good way. Let's just see where this goes
    SymbolicExpression* literal0 = PushStruct<SymbolicExpression>(out);
    literal0->type = SymbolicExpressionType_LITERAL;
    literal0->literal = 0;
    
    SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);
    expr->type = SymbolicExpressionType_OP;
    expr->op = '-';
    expr->left = literal0;
    expr->right = ParseTerm(tok,out,temp);

    return expr;
  } else if(CompareString(token,"(")){
    SymbolicExpression* exprInside = ParseExpression(tok,out,temp);

    EXPECT(tok,")");

    return exprInside;
  } else {
    if(token.size > 0 && IsNum(token[0])){
      SymbolicExpression* literal = PushStruct<SymbolicExpression>(out);
      literal->type = SymbolicExpressionType_LITERAL;
      literal->literal = ParseInt(token);
      return literal;
    } else {
      SymbolicExpression* variable = PushStruct<SymbolicExpression>(out);
      variable->type = SymbolicExpressionType_VARIABLE;
      variable->variable = PushString(out,token);
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

  if(expressions->head == expressions->tail){
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
  
static SymbolicExpression* ParseSub(Tokenizer* tok,Arena* out,Arena* temp){
  SymbolicExpression* left = ParseDiv(tok,out,temp);

  SymbolicExpression* current = left;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    char op = '\0';
    if(CompareString(peek,"-")){
      op = '-';
    }

    if(op == '\0'){
      break;
    }
    
    tok->AdvancePeek();
      
    SymbolicExpression* expr = PushStruct<SymbolicExpression>(out);

    expr->type = SymbolicExpressionType_OP;
    expr->op = op;
    expr->left = current;
    expr->right = ParseDiv(tok,out,temp);

    current = expr;
  }

  return current;
}

static SymbolicExpression* ParseSum(Tokenizer* tok,Arena* out,Arena* temp){
  SymbolicExpression* left = ParseSub(tok,out,temp);

  ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
  *expressions->PushElem() = left;

  while(!tok->Done()){
    Token peek = tok->PeekToken();
    
    char op = '\0';
    if(CompareString(peek,"+")){
      op = '+';
    }

    if(op == '\0'){
      break;
    }
    
    tok->AdvancePeek();
      
    SymbolicExpression* expr = ParseSub(tok,out,temp);
    *expressions->PushElem() = expr;
  }

  if(expressions->head == expressions->tail){
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
    tmpl = CreateTokenizerTemplate(globalPermanent,"+-*/()",{}); 
  }

  TOKENIZER_REGION(tok,tmpl);

  SymbolicExpression* expr = ParseExpression(tok,out,temp);

  return expr;
}

// This function is not intended to be called at the top. It is not recursive.
// Apply functions should call this function 
static SymbolicExpression* NormalizeArrays(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return expr;
  } break;
  case SymbolicExpressionType_OP:{
    return expr; // Not recursing, only handles 1 layer, used by Apply functions
  } break;
  case SymbolicExpressionType_ARRAY:{
    bool containsChange = false;
    for(SymbolicExpression* child : expr->sum){
      if(child->type == SymbolicExpressionType_ARRAY && child->op == expr->op){
        containsChange = true;
        break;
      }
    }

    if(containsChange){
      GrowableArray<SymbolicExpression*> builder = StartGrowableArray<SymbolicExpression*>(out);

      for(SymbolicExpression* child : expr->sum){
        if(child->type == SymbolicExpressionType_ARRAY && child->op == expr->op){
          for(SymbolicExpression* subChild : child->sum){
            *builder.PushElem() = subChild;
          }
        } else {
          *builder.PushElem() = child;
        }
      }

      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      *res = *expr;
      res->sum = EndArray(builder);

      return res;
      //printf("OIASNDIO\n");
    }

    return expr;
  } break;
  }
}

SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out,Arena* temp){
  auto AllocateOrReuse = [](SymbolicExpression* expr,SymbolicExpression* left,SymbolicExpression* right,Arena* out){
    if(expr->left == left && expr->right == right){
      return expr;
    } else {
      SymbolicExpression* result = PushStruct<SymbolicExpression>(out);
      *result = *expr;
      result->left = left;
      result->right = right;
      return result;
    }
  };
  
  auto IsAddOrSub = [](SymbolicExpression* expr){
    bool res = (expr->type == SymbolicExpressionType_OP && (expr->op == '+' || expr->op == '-'));
    return res;
  };

  auto MakeExpression = [](char op,SymbolicExpression* left,SymbolicExpression* right,Arena* out){
    SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
    res->type = SymbolicExpressionType_OP;
    res->op = op;
    res->left = left;
    res->right = right;
    return res;
  };
  
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return expr;
  } break;

  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '+'){
      ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
      for(SymbolicExpression* spec : expr->sum){
        *expressions->PushElem() = NormalizeArrays(ApplyDistributivity(spec,out,temp),out,temp);
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_ARRAY;
      res->op = '+';
      res->sum = PushArrayFromList(out,expressions);

      return NormalizeArrays(res,out,temp);
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
          *muls->PushElem() = NormalizeArrays(ApplyDistributivity(spec,out,temp),out,temp);
        }

        SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
        res->type = SymbolicExpressionType_ARRAY;
        res->op = '*';
        res->sum = PushArrayFromList(out,muls);
        return NormalizeArrays(res,out,temp);
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

      return NormalizeArrays(ApplyDistributivity(res,out,temp),out,temp);
    } else {
      DEBUG_BREAK(); // Unexpected expr->op
      Assert(false);
    }
  } break;
    
  case SymbolicExpressionType_OP: {
    SymbolicExpression* left = ApplyDistributivity(expr->left,out,temp);
    SymbolicExpression* right = ApplyDistributivity(expr->right,out,temp);

    return AllocateOrReuse(expr,left,right,out);
  } break;
  }
  
  return nullptr;
}
