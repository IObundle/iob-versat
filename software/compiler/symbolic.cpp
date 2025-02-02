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
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '+'){
      return 2;
    } else {
      return 3;
    }
  } break;
  case SymbolicExpressionType_OP:{
    return 4;
  } break;
  }
  UNREACHABLE();
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
  case SymbolicExpressionType_OP: {
    bool hasNonOpSon = (expr->left->type != SymbolicExpressionType_OP && expr->right->type != SymbolicExpressionType_OP);
    if(hasNonOpSon && !top && bind){
      builder->PushString("(");
    }

    BuildRepresentation(builder,expr->left,false,bindingStrength);

    builder->PushString("%c",expr->op);
    
    BuildRepresentation(builder,expr->right,false,bindingStrength);
    if(hasNonOpSon && !top && bind){
      builder->PushString(")");
    }
  } break;
  case SymbolicExpressionType_ARRAY: {
    builder->PushString("(");

    bool first = true;
    for(SymbolicExpression* s : expr->sum){
      if(!first){
        if(s->negative && expr->op == '+'){
          //builder.PushString("-"); // Do not print anything  because we already printed the '-'
        } else {
          builder->PushString("%c",expr->op);
        }
      } else {
        //if(s->negative && expr->op == '+'){
        //  builder.PushString("-");
        //}
      }
      BuildRepresentation(builder,s,false,bindingStrength);
      first = false;
    }
    builder->PushString(")");
  } break;
  }
}

String PushRepresentation(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  
  auto builder = StartStringBuilder(temp);
  BuildRepresentation(builder,expr,true,0);
  return EndString(out,builder);
}

void Print(SymbolicExpression* expr){
  TEMP_REGION(temp,nullptr);
  
  String res = PushRepresentation(expr,temp);
  printf("%.*s\n",UNPACK_SS(res));
}

#define EXPECT(TOKENIZER,STR) \
  TOKENIZER->AssertNextToken(STR)

static SymbolicExpression* ParseExpression(Tokenizer* tok,Arena* out);

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
  return nullptr;
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
  res->type = SymbolicExpressionType_ARRAY;
  res->sum = PushArrayFromList(out,expressions);
  res->op = '*';

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

    expr->type = SymbolicExpressionType_OP;
    expr->op = op;
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
  res->type = SymbolicExpressionType_ARRAY;
  res->sum = PushArrayFromList(out,expressions);
  res->op = '+';

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
 
  auto builder = StartGrowableArray<SymbolicExpression*>(out);
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
static SymbolicExpression* Default(SymbolicExpression* expr,Arena* out){
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
      res->sum[i] = Default(s,out);
    }
    
    return res;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = Default(expr->left,out);
    res->right = Default(expr->right,out);
    
    return res;
  } break;
  }
  UNREACHABLE();
}

typedef SymbolicExpression* (*ApplyFunction)(SymbolicExpression* expr,Arena* out);

Array<SymbolicExpression*> ApplyFunctionToArray(ApplyFunction Function,Array<SymbolicExpression*> arr,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<SymbolicExpression*>* builder = PushArenaList<SymbolicExpression*>(temp);
  for(SymbolicExpression* spec : arr){
    *builder->PushElem() = Function(spec,out);
  }

  Array<SymbolicExpression*> result = PushArrayFromList(out,builder);

  return result;
}

SymbolicExpression* ApplyGeneric(SymbolicExpression* expr,Arena* out,ApplyFunction Function){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;
  case SymbolicExpressionType_ARRAY:{
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    for(SymbolicExpression* s : expr->sum){
      *list->PushElem() = Function(s,out);
    }

    SymbolicExpression* res = CopyExpression(expr,out);
    res->sum = PushArrayFromList(out,list);

    return res;
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = Function(expr->left,out);
    res->right = Function(expr->right,out); 

    return res;
  } break;
  }
  UNREACHABLE();
}

// The main ideia is to remove negative considerations from other code and let this function handle everything.
// At the very least, let this handle the majority of cases
SymbolicExpression* NormalizeNegative(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;
  case SymbolicExpressionType_OP:{
    NOT_IMPLEMENTED("yet");
    return expr; // Not recursing, only handles 1 layer, used by Apply functions
  } break;
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '*'){
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
      return Default(expr,out); // We could move negatives downwards or upwards, but I do not think there is any good way of doing things here.
    }
  } break;
  }
  
  UNREACHABLE();
}

// All functions should work recursively
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = RemoveParenthesis(expr->left,out);
    res->right = RemoveParenthesis(expr->right,out);
    return res;
  } break;
  case SymbolicExpressionType_ARRAY:{
    if(expr->sum.size == 1){
      SymbolicExpression* onlyOne = RemoveParenthesis(expr->sum[0],out);
      onlyOne->negative = (onlyOne->negative != expr->negative);
      return onlyOne;
    }
    
    Array<SymbolicExpression*> children = ApplyFunctionToArray(RemoveParenthesis,expr->sum,out);
    ArenaList<SymbolicExpression*>* list = PushArenaList<SymbolicExpression*>(temp);
    
    for(SymbolicExpression* child : children){
      bool uplift = (child->type == SymbolicExpressionType_ARRAY && child->op == expr->op);
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
  UNREACHABLE();
}

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: // fallthrough
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;
  case SymbolicExpressionType_ARRAY:{
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
    
    if(Empty(literals)){
      if(Empty(childs)){
        return Default(expr,out);
      }
      
      SymbolicExpression* copy = CopyExpression(expr,out);
      copy->sum = PushArrayFromList(out,childs);
      return copy;
    }
    
    Array<SymbolicExpression*> allLiterals = PushArrayFromList(temp,literals);
    
    if(allLiterals.size == 1){
      // Remove zero from consideration
      if(GetLiteralValue(allLiterals[0]) == 0){
        // Unless it is the only member
        if(Empty(childs)){
          return Default(expr,out);
        }
        if(expr->op == '*'){ // Anything multiplied by zero is zero. The question is whether this logic makes sense here
          return allLiterals[0];
        }
        
        SymbolicExpression* withoutLiteral= CopyExpression(expr,out);
        withoutLiteral->sum = PushArrayFromList(out,childs);
        return withoutLiteral;
      }

      return Default(expr,out); // No change
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
    copy->left = NormalizeLiterals(expr->left,out);
    copy->right = NormalizeLiterals(expr->right,out);
    return copy;
  } break;
  }
  UNREACHABLE();
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
  UNREACHABLE();
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
  UNREACHABLE();
}

SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL: //fallthrough
  case SymbolicExpressionType_VARIABLE: //fallthrough
  case SymbolicExpressionType_OP:
    return ApplyGeneric(expr,out,ApplySimilarTermsAddition);

  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '*'){
      return ApplyGeneric(expr,out,ApplySimilarTermsAddition);
    }

    SymbolicExpression* normalized = NormalizeLiterals(expr,out);

    if(normalized->type != SymbolicExpressionType_ARRAY){
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
    
    if(Empty(finalExpressions)){
      return PushLiteral(out,0);
    }

    SymbolicExpression* sum = PushStruct<SymbolicExpression>(out);
    sum->type = SymbolicExpressionType_ARRAY;
    sum->op = '+';
    sum->sum = PushArrayFromList(out,finalExpressions);

    Assert(sum->sum.size > 0);
    
    return sum;
  } break;
  }
  UNREACHABLE();
}

SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out){
  // NOTE: Rule when it comes to negative. Every SymbolicExpression* that is eventually returned must have the negative field set. If any other Apply or such expression is called, (like RemoveParenthesis) then the field is set before calling such functions. Do this and negative should not be a problem. Not sure though.
  TEMP_REGION(temp,out);
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;

  case SymbolicExpressionType_ARRAY:{
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,expr->sum.size);
    for(int i = 0; i < children.size; i++){
      children[i] = ApplyDistributivity(expr->sum[i],out);
    }

    if(expr->op == '+'){
      // Nothing to do except apply distributivity to childs
      SymbolicExpression* res = CopyExpression(expr,out);
      res->sum = children;
      
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
        if(spec->type == SymbolicExpressionType_ARRAY && spec->op == '+'){
          // Spec is the array of additions. (Ex: (a + b) in the (a + b) * (x + y) example

          // For a and b.
          for(SymbolicExpression* subTerm : spec->sum){
            // For each member in the additions array
            
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
      res->negative = expr->negative;
      res->sum = PushArrayFromList(out,sums);

      return res;
    } else {
      Assert(false);
    }
  } break;
    
  case SymbolicExpressionType_OP: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = ApplyDistributivity(expr->left,out);
    res->right = ApplyDistributivity(expr->right,out);
    
    return res;
  } break;
  }
  
  return nullptr;
}

// Maybe this should just take an array and work from there.

#if 0
SymbolicExpression* OrderSymbols(SymbolicExpression* expr,Arena* out){
  switch(expr->type){
  case SymbolicExpressionType_LITERAL:
  case SymbolicExpressionType_VARIABLE: {
    return Default(expr,out);
  } break;
  case SymbolicExpressionType_ARRAY: {
    
  } break;
  case SymbolicExpressionType_OP: {
    SymbolicExpression* res = CopyExpression(expr,out);
    res->left = OrderSymbols(expr->left,out);
    res->right = OrderSymbols(expr->right,out);
  } break;
  }
}
#endif

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
  case SymbolicExpressionType_ARRAY:{
    if(expr->op == '+'){
      Array<SymbolicExpression*> sums = PushArray<SymbolicExpression*>(out,expr->sum.size);
      for(int i = 0; i <  expr->sum.size; i++){
        SymbolicExpression* s  =  expr->sum[i];
        sums[i] = Derivate(s,base,out);
      }
      SymbolicExpression* addExpr = PushAddBase(out);
      addExpr->sum = sums;

      //Print(addExpr);
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
        
        auto builder = StartGrowableArray<SymbolicExpression*>(out);
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

SymbolicExpression* SymbolicReplace(SymbolicExpression* base,String varToReplace,SymbolicExpression* replacingExpr,Arena* out){
  switch(base->type){
  case SymbolicExpressionType_LITERAL:
    return Default(base,out);
  case SymbolicExpressionType_VARIABLE: {
    if(CompareString(base->variable,varToReplace)){
      return Default(replacingExpr,out);
    } else {
      return Default(base,out);
    }
  } break;
  case SymbolicExpressionType_ARRAY:{
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,base->sum.size);
    for(int i = 0; i < children.size; i++){
      children[i] = SymbolicReplace(base->sum[i],varToReplace,replacingExpr,out);
    }

    SymbolicExpression* copy = CopyExpression(base,out);
    copy->sum = children;

    return copy;
  }
  case SymbolicExpressionType_OP:{
    SymbolicExpression* res = CopyExpression(base,out);
    res->left = SymbolicReplace(base->left,varToReplace,replacingExpr,out);
    res->right = SymbolicReplace(base->right,varToReplace,replacingExpr,out); 

    return res;
  } break;
  }
  UNREACHABLE();
}

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out){
  TEMP_REGION(temp,out);
  SymbolicExpression* current = expr;
  SymbolicExpression* next = nullptr;

  bool print = false;
  
  // Better way of detecting end
  for(int i = 0; i < 2; i++){
    BLOCK_REGION(temp);
    if(print) printf("%d:\n",i);

    next = NormalizeLiterals(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(print) printf("A:");
    if(print) Print(current);

#if 0
    next = ApplyDistributivity(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(print) printf("B:");
    if(print) Print(current);
#endif
    
#if 1
    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next); // RemoveParenthesis does not recurse, so obviously this will fail
    current = next;
    if(print) printf("C:");
    if(print) Print(current);
#endif

    next = ApplySimilarTermsAddition(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next);
    current = next;
    if(print) printf("D:");
    if(print) Print(current);

#if 1
    next = RemoveParenthesis(current,out);
    CheckIfSymbolicExpressionsShareNodes(current,next); 
    current = next;
    if(print) printf("E:");
    if(print) Print(current);
#endif
  }

  return current;
}

struct InputAndExpected{
  String in;
  String expected;
}; 

void TestPrint(){
  TEMP_REGION(temp,nullptr);
  
  String examples[] = {
    STRING("-(2*(1*inputChannels))+1*tileWidthWithInputChannels"),
    STRING("1+2*3"),
    STRING("1*(2+3)"),
    STRING("1+2*3"),
    STRING("1*2+3"),
    STRING("1+2+(3+4)+5"),
    STRING("a+(b+c*d)"),
    STRING("a+((b+c)+d)+a"),
    STRING("a+((((a+b)+c)+d)+e)+f"),
    STRING("1*2*3+a*b*c"),
    STRING("1*-2*-3-(-a*b*c)")
  };

  for(int i = 0; i <  ARRAY_SIZE(examples); i++){
    String s  =  examples[i];
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);
    String repr = PushRepresentation(sym,temp);
    if(!CompareString(repr,s)){
      printf("TestPrint %d failed\n Is: %.*s\n Got:%.*s\n",i,UNPACK_SS(s),UNPACK_SS(repr));
    }
  }
}

void TestDestributive(){
  TEMP_REGION(temp,nullptr);

  InputAndExpected examples[] = {
    {STRING("-(a*(1-1))"),STRING("-((1*a)+(-1*a))")}
  };

  for(int i = 0; i <  ARRAY_SIZE(examples); i++){
    BLOCK_REGION(temp);
    
    InputAndExpected s  =  examples[i];
    Tokenizer tok(s.in,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);

    SymbolicExpression* result = ApplyDistributivity(sym,temp);
    //Print(result);
    String repr = PushRepresentation(result,temp);
    if(!CompareString(repr,s.expected)){
      printf("TestDestributive %d failed\n Input:%.*s\n Good: %.*s\n Got:  %.*s\n",i,UNPACK_SS(s.in),UNPACK_SS(s.expected),UNPACK_SS(repr));
    }
  }
}

void TestRemoveParenthesis(){
  TEMP_REGION(temp,nullptr);

  String examples[] = {
    STRING("(-(2*(1*inputChannels)))+1*tileWidthWithInputChannels")
    //STRING("a - (((b - c) - e) - f)"),
    //STRING("a - (b - (c - (d - e)))")
    //STRING("(a + (((((a) + b) + c) + d) + e) + f)")
    //STRING("1 + 2 + (3 + 4) + 5"),
    //STRING("(a + (b + (c * d)))"),
    //STRING("(a + ((b + c) + d) + a)"),
    //STRING("(1*2*3) + (a*b*c)")
  };

  for(String s : examples){
    BLOCK_REGION(temp);

    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);

    SymbolicExpression* normalized = RemoveParenthesis(sym,temp);
    Print(sym);
    Print(normalized);
    printf("\n");
  }
}

void TestDerivative(){
  TEMP_REGION(temp,nullptr);

  String examples[] = {
    STRING("1 + a + b + a * b")
  };

  for(String s : examples){
    BLOCK_REGION(temp);

    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);

    SymbolicExpression* deriveA = Derivate(sym,STRING("a"),temp);
    SymbolicExpression* deriveB = Derivate(sym,STRING("b"),temp);

    deriveA = RemoveParenthesis(NormalizeLiterals(deriveA,temp),temp);
    deriveB = RemoveParenthesis(NormalizeLiterals(deriveB,temp),temp);

    Print(deriveA);
    Print(deriveB);
  }
}

void TestReplace(){
  TEMP_REGION(temp,nullptr);

  String examples[] = {
    STRING("1 + a + b + a * b")
  };

  String replacingString = STRING("c");

  for(String s : examples){
    BLOCK_REGION(temp);

    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);
    
    Tokenizer tok2(replacingString,"",{});
    SymbolicExpression* toReplaceWith = ParseSymbolicExpression(&tok2,temp);
                                                                
    SymbolicExpression* replaced1 = SymbolicReplace(sym,STRING("a"),toReplaceWith,temp);
    SymbolicExpression* replaced2 = SymbolicReplace(sym,STRING("b"),toReplaceWith,temp);

    replaced1 = RemoveParenthesis(NormalizeLiterals(replaced1,temp),temp);
    replaced2 = RemoveParenthesis(NormalizeLiterals(replaced2,temp),temp);
    
    Print(replaced1);
    Print(replaced2);
  }
}

#if 0
void TestSymbolic(){
  TEMP_REGION(temp,nullptr);

  String examples[] = {
    STRING("-((1*inputChannels)*(3-1))+1*tileWidthWithInputChannels")
//STRING("0+(0*9*nMacs+0*id*nMacs+0*id*9)+(0*9*nMacs+0*outC*nMacs+0*outC*9)+0")
#if 0
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
#endif
  };
  
  TestDestributive(temp);

#if 0
  TestRemoveParenthesis(temp);
  TestPrint(temp);
  TestDerivative(temp);

  //TestReplace(temp);
#endif
  
#if 0
  for(String s : examples){
    BLOCK_REGION(temp);
    
    Tokenizer tok(s,"",{});
    SymbolicExpression* sym = ParseSymbolicExpression(&tok,temp);
    
    //SymbolicExpression* normalized = ApplyDistributivity(sym,temp);
    //SymbolicExpression* normalized = ApplySimilarTermsAddition(sym,temp);
    //SymbolicExpression* normalized = NormalizeLiterals(sym,temp);
    SymbolicExpression* normalized = Normalize(sym,temp);
    Print(sym);
    Print(normalized);
    printf("\n");
  }
#endif
}
#endif
