#include "symbolic.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"

static TokenizerTemplate* tmpl;

void Print(SymbolicExpression* expr,bool top){
  switch(expr->type){
  case SymbolicExpressionType_VARIABLE: {
    if(expr->negative && top){
      printf("-");
    }
    printf("%.*s",UNPACK_SS(expr->variable));
  } break;
  case SymbolicExpressionType_LITERAL: {
    if(expr->negative && top){
      printf("-");
    }
    printf("%d",expr->literal);
  } break;
  case SymbolicExpressionType_OP: {
    bool hasNonOpSon = (expr->left->type != SymbolicExpressionType_OP && expr->right->type != SymbolicExpressionType_OP);

    if(hasNonOpSon){
      printf("B(");
    }

    Print(expr->left,false);

    printf("%c",expr->op);
    
    Print(expr->right,false);
    if(hasNonOpSon){
      printf(")");
    }
  } break;
  case SymbolicExpressionType_ARRAY: {
    printf("A(");
    bool first = true;
    for(SymbolicExpression* s : expr->sum){
      if(!first){
        if(s->negative && expr->op == '+'){
          printf("-");
        } else {
          printf("%c",expr->op);
        }
      } else {
        if(s->negative && expr->op == '+'){
          printf("-");
        }
      }
      Print(s,false);
      first = false;
    }
    printf(")");
  } break;
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
    tmpl = CreateTokenizerTemplate(globalPermanent,"+-*/()",{}); 
  }

  TOKENIZER_REGION(tok,tmpl);

  SymbolicExpression* expr = ParseExpression(tok,out,temp);

  return expr;
}

static SymbolicExpression* CopyExpression(SymbolicExpression* in,Arena* out){
  SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
  *res = *in;
  return res;
}

// This function is not intended to be called at the top. It is not recursive.
// Apply functions should call this function 
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out,Arena* temp){
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

    auto UpliftChildCond = [](char op,SymbolicExpression* child){
      bool res = (child->type == SymbolicExpressionType_ARRAY && (child->op == op || child->sum.size == 1));
      return res;
    };
    
    for(SymbolicExpression* child : expr->sum){
      if(UpliftChildCond(expr->op,child)){
        containsChange = true;
        break;
      }
    }

    if(containsChange){
      ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);

      for(SymbolicExpression* child : expr->sum){
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

    return expr;
  } break;
  }
}

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out,Arena* temp){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return expr;
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
      return expr;
    }
    
    Array<SymbolicExpression*> allLiterals = PushArrayFromList(temp,literals);
    
    if(allLiterals.size == 1){
      // Remove zero from consideration
      if(GetLiteralValue(allLiterals[0]) == 0){
        // Unless it is the only member
        if(Empty(childs)){
          return expr;
        }

        SymbolicExpression* copy = CopyExpression(expr,out);
        copy->sum = PushArrayFromList(out,childs);
        return copy;
      }

      return expr; // No change
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

    SymbolicExpression* copy = CopyExpression(expr,out);
    copy->sum = PushArrayFromList(out,childs);
    return copy;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* copy = CopyExpression(expr,out);
    copy->left = NormalizeLiterals(expr->left,out,temp);
    copy->right = NormalizeLiterals(expr->right,out,temp);
    return copy;
  } break;
  }
}

struct TermsWithLiteralMultiplier{
  // Negative is removed from mulTerms and pushed to literal
  SymbolicExpression* mulTerms; 
  SymbolicExpression* literal;
  //bool isDiv;
};

// Must call normalizeLiteral before calling this one. 
TermsWithLiteralMultiplier CollectTermsWithLiteralMultiplier(SymbolicExpression* expr,Arena* out){
  //Assert(expr->type == SymbolicExpressionType_ARRAY && expr->op == '*');
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:{
    TermsWithLiteralMultiplier res = {};
    res.literal = expr;
    res.mulTerms = expr;
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

SymbolicExpression* AllocateOrReuse(SymbolicExpression* expr,SymbolicExpression* left,SymbolicExpression* right,Arena* out){
  if(expr->left == left && expr->right == right){
    return expr;
  } else {
    SymbolicExpression* result = PushStruct<SymbolicExpression>(out);
    *result = *expr;
    result->left = left;
    result->right = right;
    return result;
  }
}

typedef SymbolicExpression* (*ApplyFunction)(SymbolicExpression* expr,Arena* out,Arena* temp);

SymbolicExpression* ApplyGeneric(SymbolicExpression* expr,Arena* out,Arena* temp,ApplyFunction Function){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return expr;
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
    SymbolicExpression* left = Function(expr->left,out,temp);
    SymbolicExpression* right = Function(expr->right,out,temp);

    return AllocateOrReuse(expr,left,right,out);
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
        //finalMul->negative = false;
        finalMul->negative = finalLiteral->negative;
        finalLiteral->negative = false;
        finalMul->sum = subTerms;

        //Print(finalMul);
        //printf("\n");
        
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
      // Nothing to do expect apply distributivity to childs
      ArenaList<SymbolicExpression*>* expressions = PushArenaList<SymbolicExpression*>(temp);
      for(SymbolicExpression* spec : expr->sum){
        *expressions->PushElem() = RemoveParenthesis(ApplyDistributivity(spec,out,temp),out,temp);
      }
      
      SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
      res->type = SymbolicExpressionType_ARRAY;
      res->op = '+';
      res->negative = expr->negative;
      res->sum = PushArrayFromList(out,expressions);

      return RemoveParenthesis(res,out,temp);
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
          *muls->PushElem() = RemoveParenthesis(ApplyDistributivity(spec,out,temp),out,temp);
        }

        SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
        res->type = SymbolicExpressionType_ARRAY;
        res->op = '*';
        res->negative = expr->negative;
        res->sum = PushArrayFromList(out,muls);
        return RemoveParenthesis(res,out,temp);
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
      return RemoveParenthesis(ApplyDistributivity(res,out,temp),out,temp);
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

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,Arena* temp){
  SymbolicExpression* current = expr;

  // Better way of detecting end
  for(int i = 0; i < 2; i++){
    BLOCK_REGION(temp);
    printf("%d:\n",i);
    current = NormalizeLiterals(current,out,temp);
    Print(current);
    printf("\n");
    current = ApplyDistributivity(current,out,temp);
    Print(current);
    printf("\n");
    current = RemoveParenthesis(current,out,temp);
    Print(current);
    printf("\n");
    current = ApplySimilarTermsAddition(current,out,temp);
    Print(current);
    printf("\n");
    current = RemoveParenthesis(current,out,temp);
    Print(current);
    printf("\n");
  }

  return current;
}
