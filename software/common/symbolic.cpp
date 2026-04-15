#include "symbolic.hpp"
//#include "globals.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include "parser.hpp"
#include <cstdint>

struct TestCase{
  String input;
  String expectedNormalized;
};

LoopLinearSum* PushLoopLinearSumEmpty(Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  // NOTE: Everything is simpler is freeTerm is already initialized to zero.
  res->freeTerm = SYM_Zero;
  return res;
}

LoopLinearSum* PushLoopLinearSumFreeTerm(SYM_Expr term,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  res->freeTerm = term;
  return res;
}

LoopLinearSum* PushLoopLinearSumSimpleVar(String loopVarName,SYM_Expr term,SYM_Expr start,SYM_Expr end,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  res->terms = PushArray<LoopLinearSumTerm>(out,1);
  res->terms[0].var = PushString(out,loopVarName);
  res->terms[0].term = term;
  res->terms[0].loopStart = start;
  res->terms[0].loopEnd = end;
  
  res->freeTerm = SYM_Zero;

  return res;
}

LoopLinearSum* Copy(LoopLinearSum* in,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  int size = in->terms.size;

  res->terms = PushArray<LoopLinearSumTerm>(out,size);
  for(int i = 0; i < size; i++){
    res->terms[i].var = PushString(out,in->terms[i].var);
    res->terms[i].term = in->terms[i].term;
    res->terms[i].loopStart = in->terms[i].loopStart;
    res->terms[i].loopEnd = in->terms[i].loopEnd;
  }

  res->freeTerm = in->freeTerm;
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
    res->terms[i].term = inner->terms[i].term;
    res->terms[i].loopStart = inner->terms[i].loopStart;
    res->terms[i].loopEnd = inner->terms[i].loopEnd;
  }
  for(int i = 0; i < outerSize; i++){
    res->terms[i + innerSize].var = PushString(out,outer->terms[i].var);
    res->terms[i + innerSize].term = outer->terms[i].term;
    res->terms[i + innerSize].loopStart = outer->terms[i].loopStart;
    res->terms[i + innerSize].loopEnd = outer->terms[i].loopEnd;
  }

  res->freeTerm += outer->freeTerm;

  return res;
}

LoopLinearSum* RemoveLoop(LoopLinearSum* in,int index,Arena* out){
  LoopLinearSum* res = Copy(in,out);

  res->terms = RemoveElement(res->terms,index,out);

  return res;
}

SYM_Expr TransformIntoSymbolicExpression(LoopLinearSum* sum,Arena* out){
  TEMP_REGION(temp,out);

  SYM_Expr res = SYM_Zero;
  for(LoopLinearSumTerm term : sum->terms){
    res += term.term * SYM_Var(term.var);
  }
  res += sum->freeTerm;
  
  return res;
}

SYM_Expr LoopLinearSumTermSize(LoopLinearSumTerm* term,Arena* out){
  SYM_Expr expr = term->loopEnd - term->loopStart;
  return expr;
}

SYM_Expr GetLoopLinearSumTotalSize(LoopLinearSum* in,Arena* out){
  TEMP_REGION(temp,out);

  SYM_Expr expr = SYM_One; 
  for(LoopLinearSumTerm term : in->terms){
    SYM_Expr loopSize = LoopLinearSumTermSize(&term,temp);
    expr = expr * loopSize;
  }
  
  return expr;
}

LoopLinearSum* ReplaceVariables(LoopLinearSum* in,TrieMap<String,SYM_Expr>* varReplace,Arena* out){
  LoopLinearSum* res = PushStruct<LoopLinearSum>(out);

  int size = in->terms.size;

  res->freeTerm = SYM_Replace(in->freeTerm,varReplace);
  res->terms = PushArray<LoopLinearSumTerm>(out,size);

  for(int i = 0; i < size; i++){
    res->terms[i].term = SYM_Replace(in->terms[i].term,varReplace);
    res->terms[i].loopStart = SYM_Replace(in->terms[i].loopStart,varReplace);
    res->terms[i].loopEnd = SYM_Replace(in->terms[i].loopEnd,varReplace);
  }

  return res;
}

void Print(LoopLinearSum* sum,bool printNewLine){
  TEMP_REGION(temp,nullptr);

  int size = sum->terms.size;

  for(int i = size - 1; i >= 0; i--){
    printf("for %.*s ",UN(sum->terms[i].var));
    SYM_Print(sum->terms[i].loopStart);
    printf("..");
    SYM_Print(sum->terms[i].loopEnd);
    printf("\n");
  }

  SYM_Expr fullExpression = TransformIntoSymbolicExpression(sum,temp);
  SYM_Print(fullExpression);

  if(printNewLine){
    printf("\n");
  }
}

void Repr(StringBuilder* builder,LoopLinearSum* sum){
  TEMP_REGION(temp,builder->arena);
  
  int size = sum->terms.size;

  for(int i = size - 1; i >= 0; i--){
    builder->PushString("for %.*s ",UN(sum->terms[i].var));
    SYM_Repr(builder,sum->terms[i].loopStart);
    builder->PushString("..");
    SYM_Repr(builder,sum->terms[i].loopEnd);
    builder->PushString("\n");
  }

  SYM_Expr fullExpression = TransformIntoSymbolicExpression(sum,temp);
  SYM_Repr(builder,fullExpression);
}

String PushRepr(LoopLinearSum* sum,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);
  Repr(builder,sum);
  String res = EndString(out,builder);
  return res;
}

static struct {
  Arena* arena;
  
  SYM_Node* hashTable[1024];
} SYM_State;

SYM_Expr SYM_Nil = {};
SYM_Expr SYM_Zero;
SYM_Expr SYM_One;
SYM_Expr SYM_Two;
SYM_Expr SYM_Eight;

SYM_Expr SYM_AddrW;
SYM_Expr SYM_AxiAddrW;
SYM_Expr SYM_AxiDataW;
SYM_Expr SYM_AxiStrobeW;
SYM_Expr SYM_LenW;
SYM_Expr SYM_DelayW;
SYM_Expr SYM_DataW;
SYM_Expr SYM_DataStrobeW;

inline SYM_Type GetType(SYM_Node* node){
  if(node == nullptr){
    return SYM_Type_LITERAL;
  }

  return node->type;
}

SYM_Type GetType(SYM_Expr in){
  SYM_Node* node = GetPointer(in.node);
  return GetType(node);
}

bool IsLiteral(SYM_Expr in){
  SYM_Node* node = GetPointer(in.node);
  if(node == nullptr){
    return true;
  }

  bool res = (GetType(node) == SYM_Type_LITERAL);
  return res;
}

bool IsVariable(SYM_Expr in){
  SYM_Node* node = GetPointer(in.node);
  bool res = (GetType(node) == SYM_Type_VARIABLE);
  return res;
}

int LiteralValue(SYM_Node* in){
  bool negate = IsNegative(in);
  SYM_Node* node = GetPointer(in);

  if(node == nullptr){
    return 0;
  }

  int res = node->literal;
  if(negate){
    res = -res;
  }

  return res;
}

int LiteralValue(SYM_Expr in){
  Assert(IsLiteral(in));

  return LiteralValue(in.node);
}

String VariableValue(SYM_Expr in){
  Assert(IsVariable(in));
  
  SYM_Node* node = GetPointer(in.node);
  return node->variable;
}


void SortInPlace(Array<SYM_Expr>& toSort){
  for(int i = 0; i < toSort.size; i++){
    for(int j = i + 1; j < toSort.size; j++){
      if(toSort[i] < toSort[j]){
        SWAP(toSort[i],toSort[j]);
      }
    }
  }
};

SYM_Expr CondNegate(SYM_Expr in,bool negate){
  SYM_Expr res = in;
  if(negate){
    res = Negate(res);
  }
  return res;
}

SYM_Expr Abs(SYM_Expr in){
  SYM_Expr res = in;
  if(IsNegative(res)){
    res = Negate(res);
  }

  return res;
}

inline bool IsLiteralZero(SYM_Expr in){
  bool res = (Abs(in) == SYM_Zero);
  return res;
}

inline bool IsLiteralOne(SYM_Expr in){
  bool res = (in == SYM_One);
  return res;
}

inline bool IsLiteralMinusOne(SYM_Expr in){
  bool res = (in == -SYM_One);
  return res;
}

inline bool IsSum(SYM_Expr in){
  SYM_Node* node = GetPointer(in.node);
  bool res = (GetType(node) == SYM_Type_SUM);
  return res;
}

inline bool IsDiv(SYM_Expr in){
  SYM_Node* node = GetPointer(in.node);
  bool res = (GetType(node) == SYM_Type_DIV);
  return res;
}

Array<SYM_Expr> GetChildrenOfMul(SYM_Expr top,Arena* out){
  TEMP_REGION(temp,out);

  bool negateFinal = false;

  auto list = PushList<SYM_Expr>(temp);
  auto Recurse = [list,&negateFinal](auto Recurse,SYM_Expr ptr) -> void{
    SYM_Node* node = GetPointer(ptr.node);

    if(GetType(node) == SYM_Type_MUL){
      if(IsNegative(ptr)){
        negateFinal = !negateFinal;
      }
      
      Recurse(Recurse,node->left);
      Recurse(Recurse,node->right);
    } else {
      *list->PushElem() = ptr;
    }
  };

  Recurse(Recurse,top);

  Array<SYM_Expr> res = PushArray(out,list);
  SortInPlace(res);
  
  // We just shove the negation into the first term
  if(res.size > 0){
    res[0] = CondNegate(res[0],negateFinal);
  }
  
  return res;
}

Array<SYM_Expr> GetChildrenOfSum(SYM_Expr top,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<SYM_Expr>(temp);
  auto Recurse = [list](auto Recurse,SYM_Expr ptr,bool negate) -> void{
    SYM_Node* node = GetPointer(ptr.node);

    negate = (negate != IsNegative(ptr));

    if(GetType(node) == SYM_Type_SUM){
      Recurse(Recurse,node->left,negate);
      Recurse(Recurse,node->right,negate);
    } else {
      SYM_Expr res = Abs(ptr);

      *list->PushElem() = CondNegate(res,negate);
    }
  };

  SYM_Node* node = GetPointer(top.node);
  Assert(GetType(node) == SYM_Type_SUM);

  Recurse(Recurse,top,false);

  Array<SYM_Expr> res = PushArray(out,list);
  SortInPlace(res);
  
  return res;
}

SYM_Expr GetOrAllocateLiteral(int input){
  if(input == 0){
    return {};
  }

  int literal = input;
  
  bool negative = false;
  if(literal < 0){
    literal = -literal;
    negative = true;
  }    

  int hash = literal % 1024;

  SYM_Node* ptr = SYM_State.hashTable[hash];
  SYM_Node* previous = ptr;

  SYM_Node* res = nullptr;
  for(; ptr; previous = ptr,ptr = ptr->hashNext){
    if(ptr->type == SYM_Type_LITERAL && LiteralValue(ptr) == literal){
      res = ptr;
      break;
    }
  }

  if(!res){
    res = PushStruct<SYM_Node>(SYM_State.arena);
    res->type = SYM_Type_LITERAL;
    res->literal = literal;

    if(previous){
      previous->hashNext = res;
    } else {
      SYM_State.hashTable[hash] = res;
    }
  }
  
  SYM_Expr result = {res};

  if(negative){
    result = Negate(result);
  }

  return result;
}

SYM_Expr GetOrAllocateVariable(String name){
  u64 hash = Hash(name) % 1024;

  SYM_Node* ptr = SYM_State.hashTable[hash];
  SYM_Node* previous = ptr;

  SYM_Node* res = nullptr;
  for(; ptr; previous = ptr,ptr = ptr->hashNext){
    if(ptr->type == SYM_Type_VARIABLE && ptr->variable == name){
      res = ptr;
      break;
    }
  }

  if(!res){
    res = PushStruct<SYM_Node>(SYM_State.arena);
    res->type = SYM_Type_VARIABLE;
    res->variable = PushString(SYM_State.arena,name);

    if(previous){
      previous->hashNext = res;
    } else {
      SYM_State.hashTable[hash] = res;
    }
  }
  
  SYM_Expr result = {res};

  return result;
}

SYM_Expr GetOrAllocateFunc(String name,SYM_Expr first,SYM_Expr second){
  u64 hash = (Hash(name) + Hash(first) + Hash(second)) % 1024;

  SYM_Node* ptr = SYM_State.hashTable[hash];
  SYM_Node* previous = ptr;

  SYM_Node* res = nullptr;
  for(; ptr; previous = ptr,ptr = ptr->hashNext){
    if(ptr->type == SYM_Type_VARIABLE && ptr->variable == name){
      res = ptr;
      break;
    }
  }

  if(!res){
    res = PushStruct<SYM_Node>(SYM_State.arena);
    res->type = SYM_Type_FUNC;
    res->first = first;
    res->second = second;
    res->name = PushString(SYM_State.arena,name);

    if(previous){
      previous->hashNext = res;
    } else {
      SYM_State.hashTable[hash] = res;
    }
  }
  
  SYM_Expr result = {res};

  return result;
}


SYM_Expr GetOrAllocateOp(SYM_Type type,SYM_Expr topIn,SYM_Expr bottomIn){
  SYM_Expr top = topIn;
  SYM_Expr bottom = bottomIn;

  bool isCommutative = (type == SYM_Type_SUM || type == SYM_Type_MUL);

  if(isCommutative && top > bottom){
    SWAP(top,bottom);
  }

  u64 hash = (Hash(top)+Hash(bottom)) % 1024;

  SYM_Node* ptr = SYM_State.hashTable[hash];
  SYM_Node* previous = ptr;

  SYM_Node* res = nullptr;
  for(; ptr; previous = ptr,ptr = ptr->hashNext){
    if(ptr->type == type && ptr->top == top && ptr->bottom == bottom){
      res = ptr;
      break;
    }
  }

  if(!res){
    res = PushStruct<SYM_Node>(SYM_State.arena);
    res->type = type;
    res->top = top;
    res->bottom = bottom;

    if(previous){
      previous->hashNext = res;
    } else {
      SYM_State.hashTable[hash] = res;
    }
  }
  
  SYM_Expr result = {res};

  return result;
}

int Compare(SYM_Expr left,SYM_Expr right){
  TEMP_REGION(temp,nullptr);

  // Fast special case
  if(left == right){
    return 0;
  }
  
  if(GetType(left) < GetType(right)){
    return -1;
  }
  if(GetType(left) > GetType(right)){
    return 1;
  }

  Assert(GetType(right) == GetType(left));

  SYM_Type type = GetType(left);

  FULL_SWITCH(type){
  case SYM_Type_LITERAL:{
    int leftLit = LiteralValue(left);
    int rightLit = LiteralValue(right);
    
    return rightLit - leftLit;
  } break;
  case SYM_Type_VARIABLE:{
    String leftVar = VariableValue(left);
    String rightVar = VariableValue(right);

    bool leftNeg = IsNegative(left);
    bool rightNeg = IsNegative(right);

    if(leftNeg && !rightNeg){
      return -1;
    }
    if(leftNeg && rightNeg){
      return Compare(leftVar,rightVar);
    }
    if(!leftNeg && rightNeg){
      return 1;
    }
    if(!leftNeg && !rightNeg){
      return Compare(leftVar,rightVar);
    }
  } break;
  case SYM_Type_SUM: {
    SYM_MultTerms childrenLeft = {GetChildrenOfSum(left,temp)};
    SYM_MultTerms childrenRight = {GetChildrenOfSum(right,temp)};

    return Compare(childrenLeft,childrenRight);
  } break;
  case SYM_Type_MUL: {
    SYM_MultTerms childrenLeft = {GetChildrenOfMul(left,temp)};
    SYM_MultTerms childrenRight = {GetChildrenOfMul(right,temp)};

    return Compare(childrenLeft,childrenRight);
  } break;
  case SYM_Type_DIV: 
  case SYM_Type_FUNC: return false; break;
  }

  NOT_POSSIBLE();
  return false;
}

bool operator<(SYM_Expr left,SYM_Expr right){
  bool res = Compare(left,right) < 0;
  return res;
}

bool operator>(SYM_Expr left,SYM_Expr right){
  bool res = Compare(left,right) > 0;
  return res;
}

u64 Hash(SYM_Expr expr){
  //bool negate = IsNegative(expr.node);
  SYM_Node* node = GetPointer(expr.node);

  int64_t res = 0;
  FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL: {
      res += (int64_t) LiteralValue(node);
    } break;
    case SYM_Type_VARIABLE: {
      res += Hash(node->variable);
    } break;
    case SYM_Type_FUNC: res += Hash(node->name);
    case SYM_Type_MUL:
    case SYM_Type_DIV:
    case SYM_Type_SUM: {
      res += Hash(node->left) + Hash(node->right);
    } break;
  }

  return res;
}

SYM_Expr DoAdd(SYM_Expr left,SYM_Expr right){
  if(right > left){
    SWAP(left,right);
  }

  if(IsLiteralZero(left)){
    return right;
  }
  if(IsLiteralZero(right)){
    return left;
  }

  if(IsLiteral(left) && IsLiteral(right)){
    int leftVal = LiteralValue(left);
    int rightVal = LiteralValue(right);

    return GetOrAllocateLiteral(leftVal + rightVal);
  }

  bool isDivLeft = IsDiv(left);
  bool isDivRight = IsDiv(right);

  if(isDivLeft && !isDivRight){
    SYM_Node* div = GetPointer(left);
    bool negative = IsNegative(left);
    
    return (right * div->bottom + CondNegate(div->top,negative)) / div->bottom;
  }
  if(!isDivLeft && isDivRight){
    SYM_Node* div = GetPointer(right);
    bool negative = IsNegative(right);
    
    return (left * div->bottom + CondNegate(div->top,negative)) / div->bottom;
  }
  if(isDivLeft && isDivRight){
    SYM_Node* divLeft = GetPointer(left);
    SYM_Node* divRight = GetPointer(right);
    
    bool leftNeg = IsNegative(left);
    bool rightNeg = IsNegative(right);

    SYM_Expr topLeft = CondNegate(divLeft->top,leftNeg);
    SYM_Expr topRight = CondNegate(divRight->top,rightNeg);

    return ((topLeft * divRight->bottom) + (topRight * divLeft->bottom)) / (divLeft->bottom * divRight->bottom);
  }

  return GetOrAllocateOp(SYM_Type_SUM,left,right);;
}

SYM_Expr DoSub(SYM_Expr left,SYM_Expr right){
  if(IsLiteralZero(left)){
    return Negate(right);
  }
  if(IsLiteralZero(right)){
    return left;
  }

  return DoAdd(left,Negate(right));
}

SYM_Expr DoMul(SYM_Expr left,SYM_Expr right){
  if(right > left){
    SWAP(left,right);
  }
  
  if(IsLiteralZero(left) || IsLiteralZero(right)){
    return SYM_Zero;
  }
  if(IsLiteralOne(left)){
    return right;
  }
  if(IsLiteralOne(right)){
    return left;
  }
  if(IsLiteralMinusOne(right)){
    return Negate(left);
  }
  if(IsLiteralMinusOne(left)){
    return Negate(right);
  }
  if(IsLiteral(left) && IsLiteral(right)){
    int topLit = LiteralValue(left);
    int bottomLit = LiteralValue(right);

    return GetOrAllocateLiteral(topLit * bottomLit);
  }

  // Distributivity
  bool isSumLeft = IsSum(left);
  bool isSumRight = IsSum(right);

  if(isSumLeft && !isSumRight){
    SYM_Node* sum = GetPointer(left);
    bool neg = IsNegative(left);

    return CondNegate(sum->left,neg) * right + CondNegate(sum->right,neg) * right;
  }
  if(!isSumLeft && isSumRight){
    SYM_Node* sum = GetPointer(right);
    bool neg = IsNegative(right);

    return CondNegate(sum->left,neg) * left + CondNegate(sum->right,neg) * left;
  }
  if(isSumLeft && isSumRight){
    SYM_Node* sumLeft = GetPointer(left);
    bool negLeft = IsNegative(left);

    SYM_Node* sumRight = GetPointer(right);
    bool negRight = IsNegative(right);

    SYM_Expr a = CondNegate(sumLeft->left,negLeft);
    SYM_Expr b = CondNegate(sumLeft->right,negLeft);
    SYM_Expr c = CondNegate(sumRight->left,negRight);
    SYM_Expr d = CondNegate(sumRight->right,negRight);
    
    return a * c + b * c + a * d + b * d;
  }

  bool isDivLeft = IsDiv(left);
  bool isDivRight = IsDiv(right);

  // Division interactions
  if(isDivLeft && !isDivRight){
    SYM_Node* div = GetPointer(left);
    bool negative = IsNegative(left);
    
    return (CondNegate(right,negative) * div->top) / div->bottom;
  }
  if(!isDivLeft && isDivRight){
    SYM_Node* div = GetPointer(right);
    bool negative = IsNegative(right);
    
    return (CondNegate(left,negative) * div->top) / div->bottom;
  }
  if(isDivLeft && isDivRight){
    SYM_Node* divLeft = GetPointer(left);
    SYM_Node* divRight = GetPointer(right);
    
    bool leftNeg = IsNegative(left);
    bool rightNeg = IsNegative(right);
    
    return ((CondNegate(divLeft->top,leftNeg) * CondNegate(divRight->top,rightNeg)) / (divLeft->bottom * divRight->bottom));
  }
  
  return GetOrAllocateOp(SYM_Type_MUL,left,right);
}

SYM_Expr DoDiv(SYM_Expr top,SYM_Expr bottom){
  if(IsLiteralZero(top)){
    return SYM_Zero;
  }
  if(IsLiteralOne(bottom)){
    return top;
  }
  if(IsLiteralMinusOne(bottom)){
    return Negate(top);
  }

  if(IsLiteral(top) && IsLiteral(bottom)){
    int topLit = LiteralValue(top);
    int bottomLit = LiteralValue(bottom);

    if(bottomLit != 0 && topLit % bottomLit == 0){
      return GetOrAllocateLiteral(topLit / bottomLit);
    }
  }

  if(IsNegative(top) && IsNegative(bottom)){
    top = Negate(top);
    bottom = Negate(bottom);
  }
  if(IsNegative(bottom) && !IsNegative(top)){
    top = Negate(top);
    bottom = Negate(bottom);
  }

  if(Abs(top) == Abs(bottom)){
    bool negate = IsNegative(top);
    negate = negate ^ IsNegative(bottom);
    
    return (negate ? -SYM_One : SYM_One);
  }

  if(IsDiv(top)){
    SYM_Node* divTop = GetPointer(top);
    bool negative = IsNegative(top);

    return CondNegate(divTop->top / (divTop->bottom * bottom),negative);
  }
  if(IsDiv(bottom)){
    SYM_Node* divBottom = GetPointer(bottom);
    bool negative = IsNegative(bottom);

    return CondNegate((top * divBottom->bottom) / divBottom->top,negative);
  }

  return GetOrAllocateOp(SYM_Type_DIV,top,bottom);

}

#if 1
SYM_Expr operator+(SYM_Expr left,SYM_Expr right){
  SYM_Expr res = DoAdd(left,right);
  res = SYM_Normalize(res);
  return res;
}

SYM_Expr& operator+=(SYM_Expr& left,SYM_Expr right){
  left = left + right;
  return left;
}

SYM_Expr operator-(SYM_Expr left,SYM_Expr right){
  SYM_Expr res = DoSub(left,right);
  res = SYM_Normalize(res);

  return res;
}

SYM_Expr operator-(SYM_Expr left){
  SYM_Expr res = {Negate(left.node)};
  res = SYM_Normalize(res);

  return res;
}

SYM_Expr operator*(SYM_Expr left,SYM_Expr right){
  SYM_Expr res = DoMul(left,right);
  res = SYM_Normalize(res);

  return res;
}

SYM_Expr operator/(SYM_Expr top,SYM_Expr bottom){
  SYM_Expr res = DoDiv(top,bottom);
  res = SYM_Normalize(res);

  return res;
}
#endif

static SYM_Expr SYM_Func(String name,SYM_Expr first,SYM_Expr second);

SYM_Expr SYM_PosMax(SYM_Expr leftIn,SYM_Expr rightIn){
  SYM_Expr left = SYM_Normalize(leftIn);
  SYM_Expr right = SYM_Normalize(rightIn);

  if(IsNegative(left) && IsNegative(right)){
    return SYM_Zero;
  }

  if(IsNegative(left)){
    return right;
  }
  if(IsNegative(right)){
    return left;
  }

  if(IsLiteralZero(left)){
    return right;
  }
  if(IsLiteralZero(right)){
    return left;
  }

  if(IsLiteral(left) && IsLiteral(right)){
    return SYM_Lit(MAX(LiteralValue(left),LiteralValue(right)));
  }

  return SYM_Func("PosMax",left,right);
}

SYM_Expr SYM_Align(SYM_Expr left,SYM_Expr right){
  // TODO: This name is hardcoded and must match the header generation code.
  return SYM_Func("ALIGN",left,right);
}

SYM_Expr SYM_Var(String name){
  return GetOrAllocateVariable(name);
}

SYM_Expr SYM_Func(String name,SYM_Expr first,SYM_Expr second){
  return GetOrAllocateFunc(name,first,second);
}

SYM_Expr SYM_Lit(int value){
  return GetOrAllocateLiteral(value);
}

SYM_Expr SYM_Replace(SYM_Expr expr,TrieMap<SYM_Expr,SYM_Expr>* replacements){
  auto Recurse = [replacements](auto Recurse,SYM_Expr top) -> SYM_Expr{
    bool negate = IsNegative(top.node);
    SYM_Node* node = GetPointer(top.node);

    SYM_Expr* possibleReplace = replacements->Get(top);

    SYM_Expr res = SYM_Nil;
    if(possibleReplace){
      res = *possibleReplace;
    } else {
      FULL_SWITCH(GetType(node)){
      case SYM_Type_LITERAL: 
      case SYM_Type_VARIABLE: return top;
      case SYM_Type_FUNC:
      case SYM_Type_MUL:
      case SYM_Type_DIV:
      case SYM_Type_SUM: {
        SYM_Expr left = SYM_Replace(node->left,replacements);
        SYM_Expr right = SYM_Replace(node->right,replacements);

        SWITCH(GetType(node)){
        case SYM_Type_FUNC: res = SYM_Func(node->name,left,right); break;
        case SYM_Type_MUL: res = left * right; break;
        case SYM_Type_DIV: res = left / right; break;
        case SYM_Type_SUM: res = left + right; break;
      } 
      } break;
    }
    }
      
    res = CondNegate(res,negate);
    return res;
  };
  
  SYM_Expr replaced = Recurse(Recurse,expr);
  return replaced;
}

SYM_Expr SYM_Replace(SYM_Expr expr,TrieMap<String,String>* replacements){
  TEMP_REGION(temp,nullptr);

  auto map = PushTrieMap<SYM_Expr,SYM_Expr>(temp);

  for(Pair<String,String> p : replacements){
    map->Insert(SYM_Var(p.first),SYM_Var(p.second));
  }

  return SYM_Replace(expr,map);
}

SYM_Expr SYM_Replace(SYM_Expr expr,TrieMap<String,SYM_Expr>* replacements){
  TEMP_REGION(temp,nullptr);

  auto map = PushTrieMap<SYM_Expr,SYM_Expr>(temp);

  for(Pair<String,SYM_Expr> p : replacements){
    map->Insert(SYM_Var(p.first),p.second);
  }

  return SYM_Replace(expr,map);
}

SYM_Expr SYM_Replace(SYM_Expr expr,SYM_Expr toReplace,SYM_Expr replacement){
  TEMP_REGION(temp,nullptr);

  auto map = PushTrieMap<SYM_Expr,SYM_Expr>(temp);
  map->Insert(toReplace,replacement);

  return SYM_Replace(expr,map);
}

SYM_Expr ParseSYM_Expr(Parser* parser,int bindingPower = -1){
  TEMP_REGION(temp,nullptr);

  // Parse unary
  bool negative = false;
  while(!parser->Done()){
    if(parser->IfNextToken('-')){
      negative = !negative;
      continue;
    }

    break;
  }

  // Parse atom
  SYM_Expr res = {};
  
  Token atom = parser->PeekToken();
  if(atom.type == '('){
    parser->ExpectNext('(');

    res = ParseSYM_Expr(parser);

    parser->ExpectNext(')');
  } else if(atom.type == TokenType_NUMBER){
    Token number = parser->ExpectNext(TokenType_NUMBER);
    res = GetOrAllocateLiteral(number.number);
  } else if(atom.type == TokenType_IDENTIFIER){
    parser->NextToken();

    if(parser->IfNextToken('(')){
      SYM_Expr first = ParseSYM_Expr(parser);
      parser->IfNextToken(',');
      SYM_Expr second = ParseSYM_Expr(parser);
      parser->ExpectNext(')');

      SYM_Expr func = GetOrAllocateFunc(atom.identifier,first,second);

      res = func;
    } else {
      res = GetOrAllocateVariable(atom.identifier);
    }
  } else {
    // TODO: Better error reporting
    parser->ReportUnexpectedToken(atom,{});
  }

  if(negative){
    res = -res;
  }

  struct OpInfo{
    TokenType type;
    int bindingPower;
  };

  // TODO: This should be outside the function itself. No point initializing every time.
  auto infos = PushArray<OpInfo>(temp,4);

  // TODO: Need to double check binding power and potentially
  //       implement associativity (how is 'a / b / c / d' supposed to work?)
  infos[0] = {TOK_TYPE('+'),0};
  infos[1] = {TOK_TYPE('-'),0};
  infos[2] = {TOK_TYPE('*'),1};
  infos[3] = {TOK_TYPE('/'),2};
  
  // Parse binary ops.
  while(!parser->Done()){
    Token peek = parser->PeekToken();

    bool continueOuter = false;
    for(OpInfo info : infos){
      if(peek.type == info.type){
        if(info.bindingPower > bindingPower){
          parser->NextToken();

          SYM_Expr right = ParseSYM_Expr(parser,info.bindingPower);

          if(info.type == '+'){
            res = res + right;
          } else if(info.type == '-'){
            res = res - right;
          } else if(info.type == '*'){
            res = res * right;
          } else if(info.type == '/'){
            res = res / right;
          } else {
            Assert(false);
          }

          continueOuter = true;
          break;
        }
      }
    }

    if(continueOuter){
      continue;
    }

    break;
  }
 
  return res;
}

SYM_Expr SYM_Parse(String content){
  FREE_ARENA(parseArena);

  auto tokenizer = [](void* tokenizerState) -> Token {
    DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
    
    const char* start = state->ptr;
    const char* end = state->end;

    TokenizeResult result = ParseWhitespace(start,end);
    result |= ParseComments(start,end);
    result |= ParseSymbols(start,end);
    result |= ParseNumber(start,end);
    result |= ParseIdentifier(start,end);

    state->ptr += result.bytesParsed;
    result.token.originalData.size = result.bytesParsed;

    Token res = result.token;
    //res.originalFile = state->

    return res;
  };
  
  Parser* parser = StartParsing(tokenizer,content,parseArena);
  SYM_Expr expr = ParseSYM_Expr(parser);

  return expr;
}

int TypeToBindingStrength(SYM_Type type){
  FULL_SWITCH(type){
  case SYM_Type_LITERAL: return 1;
  case SYM_Type_VARIABLE: return 1;
  case SYM_Type_SUM: return 2;
  case SYM_Type_MUL: return 3;
  case SYM_Type_DIV: return 4;
  case SYM_Type_FUNC: return 5;
  }
  NOT_POSSIBLE();
}

SYM_Expr GetLeftmostExprOfAddition(SYM_Expr top){
  SYM_Node* node = GetPointer(top.node);

  // NOTE: Do we want to apply negation in this conditions?

  SYM_Expr res = top;

  if(GetType(node) == SYM_Type_SUM){
    res = GetLeftmostExprOfAddition(node->left);
  }
  if(GetType(node) == SYM_Type_MUL){
    res = GetLeftmostExprOfAddition(node->left);
  }

  return res;
}

String SYM_ReprHier(SYM_Expr expr,Arena* out){
  TEMP_REGION(temp,out);

  auto b = StartString(temp);

  auto Recurse = [b](auto Recurse,SYM_Expr top) -> void{
    bool negate = IsNegative(top.node);
    SYM_Node* node = GetPointer(top.node);

    if(negate){
      b->PushString("-");
    }
    
    FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL: b->PushString("%d",LiteralValue(node)); break;
    case SYM_Type_VARIABLE: b->PushString("%.*s",UN(node->variable)); break;
    case SYM_Type_FUNC:
    case SYM_Type_MUL:
    case SYM_Type_DIV:
    case SYM_Type_SUM: {
      b->PushString("(");

      FULL_SWITCH(GetType(node)){
      case SYM_Type_LITERAL: break;
      case SYM_Type_VARIABLE: break;
      case SYM_Type_MUL: b->PushString("*"); break;
      case SYM_Type_DIV: b->PushString("/"); break;
      case SYM_Type_SUM: b->PushString("+"); break;
      case SYM_Type_FUNC: b->PushString(node->name); break;
    }

      b->PushString(",");
      Recurse(Recurse,node->left);
      b->PushString(",");
      Recurse(Recurse,node->right);

      b->PushString(")");
    } break;
  }

  };

  Recurse(Recurse,expr);

  String res = EndString(out,b);
  return res;
}

void SYM_Repr(StringBuilder* b,SYM_Expr expr){
  auto Recurse = [b](auto Recurse,SYM_Expr top,int parentBindingStrength) -> void{
    bool negate = IsNegative(top.node);
    SYM_Node* node = GetPointer(top.node);

    if(negate){
      b->PushString("-");
    }

    int bindingStrength = TypeToBindingStrength(GetType(node));
    bool bind = (parentBindingStrength > bindingStrength) || negate;

    FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL: b->PushString("%d",LiteralValue(node)); break;
    case SYM_Type_VARIABLE: b->PushString(node->variable); break;
    case SYM_Type_MUL:
    case SYM_Type_DIV:
    case SYM_Type_SUM:{
      String op = {};
      SWITCH(GetType(node)){
      case SYM_Type_SUM:{
        op = "+";

        if(IsNegative(node->right.node) ||  IsNegative(GetLeftmostExprOfAddition(node->right).node)){
          op = ""; // The node negative will put the '-'
        }
      } break;
      case SYM_Type_MUL: op = "*"; break;
      case SYM_Type_DIV: op = "/"; break;
      default: NOT_POSSIBLE();
    }

      if(bind){
        b->PushString("(");
      }

      Recurse(Recurse,node->left,bindingStrength);

      b->PushString(op);

      Recurse(Recurse,node->right,bindingStrength);

      if(bind){
        b->PushString(")");
      }
    } break;
    case SYM_Type_FUNC:{
      b->PushString(node->name);
      b->PushString("(");
      Recurse(Recurse,node->first,0);
      b->PushString(",");
      Recurse(Recurse,node->second,0);
      b->PushString(")");
    } break;
  }
  };

  Recurse(Recurse,expr,0);
}

Pair<SYM_Expr,SYM_Expr> SYM_BreakDiv(SYM_Expr in){
  SYM_Expr top = in;
  SYM_Expr bottom = SYM_One;
  
  if(IsDiv(in)){
    bool negate = IsNegative(in.node);
    SYM_Node* node = GetPointer(in.node);

    top = CondNegate(node->top,negate);
    bottom = node->bottom;
  }

  return {top,bottom};
}

String SYM_Repr(SYM_Expr expr,Arena* out){
  TEMP_REGION(temp,out);

  auto b = StartString(temp);

  SYM_Repr(b,expr);

  String res = EndString(out,b);
  return res;
}

void SYM_Print(SYM_Expr expr){
  TEMP_REGION(temp,nullptr);
  String repr = SYM_Repr(expr,temp);
  printf("%.*s\n",UN(repr));
}

// Pushes negations downwards or top for divisions
SYM_Expr RemoveParenthesis(SYM_Expr in){
  bool negate = IsNegative(in.node);
  SYM_Node* node = GetPointer(in.node);

  SYM_Expr res = Abs(in);
  SWITCH(GetType(node)){
  case SYM_Type_SUM:{
    SYM_Expr left = RemoveParenthesis(CondNegate(node->left,negate));
    SYM_Expr right = RemoveParenthesis(CondNegate(node->right,negate));

    negate = false;

    res = DoAdd(left,right);
  } break;
  case SYM_Type_MUL:{
    SYM_Expr left = RemoveParenthesis(CondNegate(node->left,negate));
    SYM_Expr right = RemoveParenthesis(node->right);

    negate = false;

    res = DoMul(left,right);
  } break;
  case SYM_Type_DIV:{
    SYM_Expr top = RemoveParenthesis(CondNegate(node->top,negate));
    SYM_Expr bottom = RemoveParenthesis(node->bottom);

    negate = false;
    
    res = DoDiv(top,bottom);
  } break;
}

  res = CondNegate(res,negate);

  return res;
}

void SYM_Print(SYM_MultPartition part){
  SYM_Print(part.literal);

  for(SYM_Expr expr : part.mults.terms){
    SYM_Print(expr);
  }
}

SYM_MultPartition GetMultPartition(SYM_Expr expr,Arena* out){
  TEMP_REGION(temp,out);

  SYM_MultPartition res = {};

  res.literal = SYM_One;

  bool negateLiteral = false;

  auto list = PushList<SYM_Expr>(temp);

  auto Recurse = [list,&res,&negateLiteral](auto Recurse,SYM_Expr top) -> void{
    bool negate = IsNegative(top.node);
    SYM_Node* node = GetPointer(top.node);

    FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL: res.literal = DoMul(res.literal,top); break;
    case SYM_Type_SUM: 
    case SYM_Type_DIV: 
    case SYM_Type_FUNC: 
    case SYM_Type_VARIABLE:{
      if(negate){
        negateLiteral = !negateLiteral;
      }

      *list->PushElem() = Abs(top); 
    } break;
    case SYM_Type_MUL: {
      if(negate){
        negateLiteral = !negateLiteral;
      }

      Recurse(Recurse,node->left);
      Recurse(Recurse,node->right);
    } break;
  };

  };

  Recurse(Recurse,expr);

  Array<SYM_Expr> result = PushArray(out,list);
  SortInPlace(result);

  res.literal = CondNegate(res.literal,negateLiteral);

  res.mults.terms = result;

  return res;
}

SYM_Partition SplitExpressionBasedOn(SYM_Expr expression,SYM_Expr base){
  TEMP_REGION(temp,nullptr);

  bool negate = IsNegative(expression.node);
  SYM_Node* node = GetPointer(expression.node);

  bool exists = false;

  if(Abs(expression) == Abs(base)){
    exists = true;
  }

  SYM_Expr leftovers = SYM_One;

  FULL_SWITCH(GetType(node)){
  case SYM_Type_LITERAL:{
  } break;
  case SYM_Type_VARIABLE:{
  } break;
  case SYM_Type_SUM:{
  } break;
  case SYM_Type_MUL:{
    Array<SYM_Expr> allMults = GetChildrenOfMul(expression,temp);
    
    int index = -1;
    for(int i = 0; i <  allMults.size; i++){
      SYM_Expr same = allMults[i];
      if(Abs(same) == Abs(base)){
        index = i;
        break;
      }
    }

    if(index != -1){
      for(int i = 0; i <  allMults.size; i++){
        if(i == index){
          continue;
        }

        leftovers = DoMul(leftovers,allMults[i]);
      }
      exists = true;
    }
  } break;
  case SYM_Type_DIV:{
  } break;
  case SYM_Type_FUNC:{
  } break;
  }

  leftovers = CondNegate(leftovers,negate);

  SYM_Partition res = {};
  res.leftovers = leftovers;
  res.exists = exists;

  return res;
}

#if 0
SYM_Expr SYM_Normalize2(SYM_Expr in){
  TEMP_REGION(temp,nullptr);

  SYM_Expr expr = in;

  expr = GetNormalized(expr);

  bool negate = IsNegative(expr.node);
  SYM_Node* node = GetPointer(expr.node);

  if(node->isNormalized){
    return in;
  }

  bool isMul = false;
  FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL: 
    case SYM_Type_VARIABLE: break; 
    case SYM_Type_MUL: isMul = true;
    case SYM_Type_SUM: {
      Array<SYM_Expr> children = {};

      if(isMul){
        children = GetChildrenOfMul(expr,temp);
      } else {
        children = GetChildrenOfSum(expr,temp);
      }
      
      SortInPlace(children);

      SYM_Expr start = (isMul ? SYM_One : SYM_Zero);
      
      for(SYM_Expr child : children){
        if(isMul){
          start = start * child;
        } else {
          start = start + child;
        }
      }

      expr = start;
    } break;

    // TODO: We probably need to implement this. Disabled for now while we check if this approach makes sense
    case SYM_Type_DIV: break;
    case SYM_Type_FUNC: break;
  }

  node->isNormalizedNodeValid = true;
  node->normalizedNode = GetPointer(expr);

  expr = CondNegate(expr,negate);

  return expr;
}
#endif

SYM_Expr NormalizeLiterals(SYM_Expr in){
  TEMP_REGION(temp,nullptr);

  bool negate = IsNegative(in.node);
  SYM_Node* node = GetPointer(in.node);

  SYM_Expr res = Abs(in);
  SWITCH(GetType(node)){
  case SYM_Type_SUM:{
    Array<SYM_Expr> children = GetChildrenOfSum(res,temp);

    Array<bool> seen = PushArray<bool>(temp,children.size);

    TrieMap<SYM_MultTerms,SYM_Expr>* mapMultPartToLiteral = PushTrieMap<SYM_MultTerms,SYM_Expr>(temp);

    int literalAccum = 0;
    for(int i = 0; i <  children.size; i++){
      SYM_Expr& s = children[i];
      s = NormalizeLiterals(s);

      if(IsLiteral(s)){
        literalAccum += LiteralValue(s);
        seen[i] = true;
      } else {
        SYM_MultPartition part = GetMultPartition(s,temp);
        
        GetOrAllocateResult res = mapMultPartToLiteral->GetOrAllocate(part.mults);

        if(res.alreadyExisted){
          *res.data = DoAdd(*res.data,part.literal);
        } else {
          *res.data = part.literal;
        }
      }
    }

    SYM_Expr ptr = SYM_Zero; // GetOrAllocateLiteral(literalAccum);

    for(Pair<SYM_MultTerms,SYM_Expr> p : mapMultPartToLiteral){
      SYM_Expr mult = NormalizeLiterals(p.second);

      for(SYM_Expr expr : p.first.terms){
        mult = DoMul(mult,expr);
      }
      
      ptr = DoAdd(ptr,mult);
    }

    ptr = DoAdd(GetOrAllocateLiteral(literalAccum),ptr);
    
    res = ptr;
  } break;
  case SYM_Type_MUL:{
    Array<SYM_Expr> children = GetChildrenOfMul(res,temp);

    int literalAccum = 1;
    SYM_Expr accumNonLiteral = SYM_One;
    for(int i = 0; i <  children.size; i++){
      SYM_Expr& s = children[i];
      SYM_Expr n = NormalizeLiterals(s);

      if(IsLiteral(n)){
        literalAccum *= LiteralValue(n);
      } else {
        accumNonLiteral = DoMul(accumNonLiteral,n);
      }
    }
    
    accumNonLiteral = DoMul(GetOrAllocateLiteral(literalAccum),accumNonLiteral);

    res = accumNonLiteral;
  } break;
  case SYM_Type_DIV:{
    SYM_Expr top = NormalizeLiterals(node->top);
    SYM_Expr bottom = NormalizeLiterals(node->bottom);
   
    Array<SYM_Expr> topSum = GetChildrenOfMul(top,temp);
    Array<SYM_Expr> bottomSum = GetChildrenOfMul(bottom,temp);

    TrieMap<SYM_Expr,int>* topResult = PushTrieMap<SYM_Expr,int>(temp);
    TrieMap<SYM_Expr,int>* bottomResult = PushTrieMap<SYM_Expr,int>(temp);

    for(SYM_Expr e : topSum){
      auto res = topResult->GetOrAllocate(e);

      if(res.alreadyExisted){
        *res.data += 1;
      } else {
        *res.data = 1;
      }
    }

    for(SYM_Expr e : bottomSum){
      int* topVal = topResult->Get(e);
      
      if(topVal && *topVal > 0){
        *topVal -= 1;
        continue;
      }

      auto res = bottomResult->GetOrAllocate(e);
      if(res.alreadyExisted){
        *res.data += 1;
      } else {
        *res.data = 1;
      }
    }

    SYM_Expr topValue = SYM_One;
    for(Pair<SYM_Expr,int> p : topResult){
      for(int i = 0; i < p.second; i++){
        topValue = DoMul(topValue,p.first);
      }
    }

    SYM_Expr bottomValue = SYM_One;
    for(Pair<SYM_Expr,int> p : bottomResult){
      for(int i = 0; i < p.second; i++){
        bottomValue = DoMul(bottomValue,p.first);
      }
    }

    // Try to find expressions of the form (a*b + b) / b
    // NOTE: Disabled and probably not worth it to enable. Need to fix negative related bugs to put this working
    //       and we do not really gain anything from this other that cleaning up some expressions.
    //       This stuff should be a separate function anyway if we actually care about 
    if(false && GetType(GetPointer(topValue)) == SYM_Type_SUM){
      Array<SYM_Expr> sumTerms = GetChildrenOfSum(topValue,temp);

      Array<SYM_Expr> bottom = GetChildrenOfMul(bottomValue,temp);

      for(SYM_Expr possibleBottom : bottom){
        BLOCK_REGION(temp);
        
        bool possible = true;
        auto list = PushList<SYM_Partition>(temp);
        for(SYM_Expr term : sumTerms){
          SYM_Partition part = SplitExpressionBasedOn(term,possibleBottom);
          
          *list->PushElem() = part;

          if(!part.exists){
            possible = false;
          }
        }

        if(possible){
          topValue = SYM_Zero;

          for(SYM_Partition part : list){
            topValue = DoAdd(topValue,part.leftovers);
          }

          bottomValue = DoDiv(bottomValue,possibleBottom);
        }
      }

      res = DoDiv(NormalizeLiterals(topValue),NormalizeLiterals(bottomValue));
    } else {
      res = DoDiv(topValue,bottomValue);
    }
  } break;
  case SYM_Type_FUNC:{
    // Max is already normalized
  } break;
}

  res = CondNegate(res,negate);

  return res;
}

SYM_Expr OrderTerms(SYM_Expr in){
  TEMP_REGION(temp,nullptr);

  bool negate = IsNegative(in.node);
  SYM_Node* node = GetPointer(in.node);

  bool isMul = false;
  SYM_Expr res = Abs(in);

  FULL_SWITCH(GetType(node)){
  case SYM_Type_MUL: isMul = true; // fallthrough
  case SYM_Type_SUM:{
    Array<SYM_Expr> children = {}; //GetChildrenOfSum(res,temp);

    if(isMul){
      children = GetChildrenOfMul(res,temp);
    } else {
      children = GetChildrenOfSum(res,temp);
    }

    for(SYM_Expr& s : children){
      s = OrderTerms(s);
    }

    SortInPlace(children);

    SYM_Expr ptr = {};
    if(isMul){
      ptr = SYM_One;
    } else {
      ptr = SYM_Zero;
    }

    for(int i = 0; i <  children.size; i++){
      if(isMul){
        ptr = DoMul(ptr,children[i]);
      } else {
        ptr = DoAdd(ptr,children[i]);
      } 
    }

    res = ptr;
  } break;
  case SYM_Type_DIV:{
    res = DoDiv(OrderTerms(node->top),OrderTerms(node->bottom));
  } break;
  case SYM_Type_FUNC:{
    res = SYM_Func(node->name,OrderTerms(node->top),OrderTerms(node->bottom));
  } break;
  case SYM_Type_LITERAL:
  case SYM_Type_VARIABLE: break;
}

  res = CondNegate(res,negate);

  return res;
}

SYM_Expr SYM_Derivate(SYM_Expr expr,String var){
  bool negate = IsNegative(expr.node);
  SYM_Node* node = GetPointer(expr.node);

  SYM_Expr res = SYM_Zero;
  FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL:{
      res = SYM_Zero;
    } break;
    case SYM_Type_VARIABLE:{
      res = SYM_Zero;
      
      if(node->variable == var){
        res = SYM_One;
      }
    } break;
    case SYM_Type_SUM:{
      res = SYM_Derivate(node->left,var) + SYM_Derivate(node->right,var);
    } break;
    case SYM_Type_MUL:{
      res = SYM_Derivate(node->left,var) * node->right + SYM_Derivate(node->right,var) * node->left;
    } break;
    // NOTE: We do not actually want derivations to handle divs or custom functions.
    //       In theory the address gen stuff already removed these from consideration before calling derivate.
    //       So this is more likely an error than a missing feature. If we ever reach this point need to investigate
    //       why
    case SYM_Type_DIV:{
      NOT_IMPLEMENTED();
    } break;
    case SYM_Type_FUNC:{
      res = SYM_Zero;
      //NOT_IMPLEMENTED();
    } break;
  }

  res = CondNegate(res,negate);

  return res;
}

SYM_Expr SYM_Factor(SYM_Expr expr,SYM_Expr commonFactor){
  TEMP_REGION(temp,nullptr);

  bool negate = IsNegative(expr.node);
  SYM_Node* node = GetPointer(expr.node);

  SYM_Expr res = Abs(expr);

  negate = (negate != IsNegative(commonFactor));

  SYM_Expr absCommon = Abs(commonFactor);

  FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL:{
      if(res == absCommon){
        res = absCommon;
      } else {
        res = SYM_Zero;
      }
    } break;
    case SYM_Type_VARIABLE:{
      if(res == absCommon){
        res = SYM_One;
      } else {
        res = SYM_Zero;
      }
    } break;
    case SYM_Type_SUM:{
      res = SYM_Factor(node->left,absCommon) + SYM_Factor(node->right,absCommon);
    } break;
    case SYM_Type_MUL:{
      Array<SYM_Expr> children = GetChildrenOfMul(res,temp);
      
      SYM_Expr* found = nullptr;
      for(SYM_Expr& child : children){
        if(Abs(child) == absCommon){
          found = &child;
          break;
        }
      }

      res = SYM_Zero;
      if(found){
        res = SYM_One;
        for(SYM_Expr& child : children){
          if(&child == found){
            continue;
          }

          res = res * child;
        }

        negate = (negate != IsNegative(*found));
      }
    } break;
    // NOTE: We do not actually want derivations to handle divs or custom functions.
    //       In theory the address gen stuff already removed these from consideration before calling derivate.
    //       So this is more likely an error than a missing feature. If we ever reach this point need to investigate
    //       why
    case SYM_Type_DIV:{
      NOT_IMPLEMENTED();
    } break;
    case SYM_Type_FUNC:{
      NOT_IMPLEMENTED();
    } break;
  }

  res = CondNegate(res,negate);

  return res;
}

bool SYM_IsZeroValue(SYM_Expr expr){
  SYM_Expr normalized = SYM_Normalize(expr);
  bool res = (normalized == SYM_Zero);
  return res;
}

bool SYM_IsOneValue(SYM_Expr expr){
  SYM_Expr normalized = SYM_Normalize(expr);
  bool res = (normalized == SYM_One);
  return res;
}

SYM_EvaluateResult SYM_DebugEvaluate(SYM_Expr top,TrieMap<String,SYM_Expr>* values,Arena* out){
  TEMP_REGION(temp,out);

  auto errorList = PushList<String>(temp);
  bool divByZero = false;

  // NOTE: Evaluation is performed in floating point otherwise the division would cause problems 
  //       simply by the order of evaluation. 4 * 9 / 4 is 9 but if we evaluate the 9 / 4 first as integers
  //       it becomes 9 / 4 = 2 and the final result is 8
  //       Also remember that this is used for debugging purposes. A proper evaluator would just do things in integers
  //       because it would start from a normalized expression.
  auto Recurse = [errorList,values,out,&divByZero](auto Recurse,SYM_Expr expr) -> float {
    bool negate = IsNegative(expr.node);
    SYM_Node* node = GetPointer(expr.node);

    float res = 0;
    FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL:{
      res = (float) LiteralValue(node);
    } break;
    case SYM_Type_VARIABLE:{
      SYM_Expr* val = values->Get(node->variable);
      
      if(!val){
        *errorList->PushElem() = PushString(out,"Variable %.*s does not exist. Evaluator assuming zero value",UN(node->variable));
      } else {
        res = Recurse(Recurse,*val);
      }
    } break;
    case SYM_Type_SUM:{
      float left = Recurse(Recurse,node->left);
      float right = Recurse(Recurse,node->right);

      res = left + right;
    } break;
    case SYM_Type_MUL:{
      float left = Recurse(Recurse,node->left);
      float right = Recurse(Recurse,node->right);

      res = left * right;
    } break;
    case SYM_Type_DIV:{
      float top = Recurse(Recurse,node->top);
      float bottom = Recurse(Recurse,node->bottom);
      
      if(bottom == 0){
        *errorList->PushElem() = PushString(out,"Div by zero detected. Assuming result is one and proceeding");
        divByZero = true;
        res = 1;
      } else {
        res = top / bottom;
      }
    } break;
    case SYM_Type_FUNC:{
      res = 0.0f;
      
      //NOT_IMPLEMENTED();
    } break;
    }

    if(negate){
      res = -res;
    }

    return res;
  };
  
  float result = Recurse(Recurse,top);

  SYM_EvaluateResult res = {};
  res.result = (int) result;
  res.divByZero = divByZero;
  
  return res;
}

SYM_EvaluateResult SYM_ConstantEvaluate(SYM_Expr top){
  TEMP_REGION(temp,nullptr);

  bool divByZero = false;
  bool nonConstantValue = false;

  // NOTE: Evaluation is performed in floating point otherwise the division would cause problems 
  //       simply by the order of evaluation. 4 * 9 / 4 is 9 but if we evaluate the 9 / 4 first as integers
  //       it becomes 9 / 4 = 2 and the final result is 8
  //       Also remember that this is used for debugging purposes. A proper evaluator would just do things in integers
  //       because it would start from a normalized expression.
  auto Recurse = [&divByZero,&nonConstantValue](auto Recurse,SYM_Expr expr) -> int {
    bool negate = IsNegative(expr.node);
    SYM_Node* node = GetPointer(expr.node);

    int res = 0;
    FULL_SWITCH(GetType(node)){
    case SYM_Type_LITERAL:{
      res = LiteralValue(node);
    } break;
    case SYM_Type_VARIABLE:{
      nonConstantValue = true;
    } break;
    case SYM_Type_SUM:{
      int left = Recurse(Recurse,node->left);
      int right = Recurse(Recurse,node->right);

      res = left + right;
    } break;
    case SYM_Type_MUL:{
      int left = Recurse(Recurse,node->left);
      int right = Recurse(Recurse,node->right);

      res = left * right;
    } break;
    case SYM_Type_DIV:{
      int top = Recurse(Recurse,node->top);
      int bottom = Recurse(Recurse,node->bottom);
      
      if(bottom == 0){
        divByZero = true;
        res = 1;
      } else {
        res = top / bottom;
      }
    } break;
    case SYM_Type_FUNC:{
      res = 0;
      
      if(node->name == "PosMax"){
        int first = Recurse(Recurse,node->first);
        int second = Recurse(Recurse,node->second);

        res = MAX(first,second);
      } else {
        NOT_IMPLEMENTED();
      }
    } break;
    }

    if(negate){
      res = -res;
    }

    return res;
  };
  
  int result = Recurse(Recurse,top);

  SYM_EvaluateResult res = {};
  res.result = (int) result;
  res.divByZero = divByZero;
  res.nonConstantValue = nonConstantValue;
  
  return res;
}

Array<String> SYM_GetAllVariables(SYM_Expr top,Arena* out){
  TEMP_REGION(temp,out);

  auto varSet = PushTrieSet<String>(temp);

  auto Recurse = [varSet](auto Recurse,SYM_Expr expr) -> void {
    SYM_Node* node = GetPointer(expr.node);

    FULL_SWITCH(GetType(node)){
    case SYM_Type_VARIABLE:{
      varSet->Insert(node->variable);
    } break;
    case SYM_Type_SUM:  // fallthrough
    case SYM_Type_MUL:  // fallthrough
    case SYM_Type_FUNC: // fallthrough
    case SYM_Type_DIV: {
      Recurse(Recurse,node->left);
      Recurse(Recurse,node->right);
    } break;
    case SYM_Type_LITERAL: break;
    }
  };

  Recurse(Recurse,top);
  
  Array<String> res = PushArray(out,varSet);
  return res;
}

TrieMap<String,SYM_Expr>* SYM_GenerateRandomValues(SYM_Expr top,Arena* out){
  TEMP_REGION(temp,out);

  Array<String> allVars = SYM_GetAllVariables(top,temp);

  TrieMap<String,SYM_Expr>* varsAndValues = PushTrieMap<String,SYM_Expr>(out); 

  for(String str : allVars){
    int randomNumber = RandomNumberBetween(1,10);
    
    if(RandomNumberBetween(0,2) == 1){
      randomNumber = -randomNumber;
    }

    varsAndValues->Insert(str,GetOrAllocateLiteral(randomNumber));
  }

  return varsAndValues;
}

struct NormalizeResult{
  SYM_Expr res;
  bool failedEvaluation;
  bool failedFromDivZero;
  int expected;
  int gotOnError;
  String problematicFunction;
  int functionLine;
};

NormalizeResult NormalizeWhileChecking(SYM_Expr in){
  TEMP_REGION(temp,nullptr);

  TrieMap<String,SYM_Expr>* randomValues = SYM_GenerateRandomValues(in,temp); 
  SYM_EvaluateResult evaluation = SYM_DebugEvaluate(in,randomValues,temp);

  if(evaluation.Error()){
    NormalizeResult res = {};
    res.failedEvaluation = true;
    res.failedFromDivZero = evaluation.divByZero;
    return res;
  }

  int expectedValue = evaluation.result;

  SYM_Expr res = in;
  bool failedEvaluation = false;
  bool failedFromDivZero = false;
  int gotOnError = 0;
  String functionName = {};
  int functionLine = 0;

  SYM_Expr finalRes = res;

  bool print = false;

  // NOTE: Assuming that if the difference is 1 then it is probably a floating point conversion rather than a 
  //       bug in the normalization process.
  //       We are generating a bunch of testcases meaning that if we miss one from a false negative then it is no
  //       big deal. The next random tests will hopefully catch it.
#define _CHECK(FUNCTION) { \
  res = FUNCTION(res); \
  SYM_EvaluateResult evaluation = SYM_DebugEvaluate(res,randomValues,temp); \
  if(print) printf("%.*s\n",UN(SYM_Repr(res,temp))); \
  if(evaluation.Error() || evaluation.result != expectedValue){ \
    if(Abs(evaluation.result - expectedValue) <= 1){ \
    } else if(!failedEvaluation) { \
      functionName = #FUNCTION; \
      functionLine = __LINE__; \
      finalRes = res; \
      failedEvaluation = true; \
      failedFromDivZero = evaluation.divByZero; \
      gotOnError = evaluation.result; \
    } \
  } \
  }

  _CHECK(RemoveParenthesis);
  _CHECK(NormalizeLiterals);
  _CHECK(OrderTerms);
#undef _CHECK

  if(!failedEvaluation){
    finalRes = res;
  }

  NormalizeResult result = {};
  result.res = finalRes;
  result.failedEvaluation = failedEvaluation;
  result.failedFromDivZero = failedFromDivZero;
  result.expected = expectedValue;
  result.gotOnError = gotOnError;
  result.problematicFunction = functionName;
  result.functionLine = functionLine;
  
  return result;
}

SYM_Expr SYM_Normalize(SYM_Expr in){
  SYM_Expr res = in;

  res = RemoveParenthesis(res);
  res = NormalizeLiterals(res);
  res = OrderTerms(res);

  return res;
}

SYM_Expr GenerateRandomExpression(int expectedAmountOfNodes){
  static String vars[16] = {"q","w","e","r","t","y","u","i","o","p","a","s","d","f","g","h"};

  int amountOfNodes = 0;
  auto Recurse = [&amountOfNodes,expectedAmountOfNodes](auto Recurse) -> SYM_Expr {
    int typeRand = RandomNumberBetween(1,expectedAmountOfNodes);
    
    bool simple = (8 * (typeRand + amountOfNodes) > (9 * expectedAmountOfNodes));

    amountOfNodes += 1;

    SYM_Expr res = SYM_Zero;
    if(simple){
      int typeRand = RandomNumberBetween(1,3);
      
      if(typeRand == 1){
        res = GetOrAllocateLiteral(RandomNumberBetween(1,5));
      } else {
        res = GetOrAllocateVariable(vars[RandomNumberBetween(0,16)]);
      }
    } else {
      int typeRand = RandomNumberBetween(0,3);
      
      switch(typeRand){
        case 0: res = GetOrAllocateOp(SYM_Type_SUM,Recurse(Recurse),Recurse(Recurse)); break;
        case 1: res = GetOrAllocateOp(SYM_Type_MUL,Recurse(Recurse),Recurse(Recurse)); break;
        case 2: res = GetOrAllocateOp(SYM_Type_DIV,Recurse(Recurse),Recurse(Recurse)); break;
        // case 3: TODO: FUNC
      }
    }

    bool negate = (RandomNumberBetween(1,3) == 1);
    res = CondNegate(res,negate);

    return res;
  };
  
  SYM_Expr expr = Recurse(Recurse);
  return expr;
}

void SYM_Test(){
  TestCase tests[] = {
#if 0
    // NOTE: Disabled since division by common mult is not currently implemented 
    //       ex : (x + xy) / x will not simplify to 1 + y  
    {"1+8+7+a*6*4/(a*p)","(24+16*p)/p"},
    {"(x*y + x)/x","1+y"},
    {"1+8+7+a*6*4/(a*p/(7+8*8/(e+5+g+9*5)))","(9936+e*16*p+g*16*p+800*p+168*e+168*g)/(e*p+g*p+50*p)"},
#endif
#if 1
    {"a+b","a+b"},
    {"a / b + c / d","(b*c+a*d)/(b*d)"},
    {"(0+2*x)+(2*a-1)*y","-y+2*x+2*a*y"},
    {"1/x * y","y/x"},
    {"(a-b)*(a-b)","b*b-2*a*b+a*a"},
    {"(a+b)*(a+b)","b*b+2*a*b+a*a"},
    {"a * (1 / b)","a/b"},
    {"(1 / b) * a","a/b"},
    {"(a/b) * (c/d)","(a*c)/(b*d)"},
    {"MAX(1+a+2,4)","MAX(3+a,4)"},
    {"(x*y)/x","y"},
    {"(x*x*y)/x","x*y"},
    {"(x*y)/(x*y)","1"},
    {"(x*x*y)/(x*y*y)","x/y"},
    {"x/x","1"},
    {"b / c + a","(b+a*c)/c"},
    {"a + b / c","(b+a*c)/c"},
    {"0/a","0"},
    {"1/a","1/a"},
    {"1/1","1"},
    {"-1/1","-1"},
    {"1/-1","-1"},
    {"-1/-1","1"},
    {"a*b + a*b","2*a*b"},
    {"a+a","2*a"},
    {"a*(b+c)","a*b+a*c"},
    {"a*(-b+c)","-b*a+a*c"},
    {"a*(b-c)","-c*a+a*b"},
    {"a*-(b+c)","-b*a-c*a"},
    {"-a*(b+c)","-b*a-c*a"},
    {"-a*(-b+c)","-c*a+a*b"},
    {"-a*(b-c)","-b*a+a*c"},
    {"-a*-(b+c)","a*b+a*c"},
    {"a + 1","1+a"},
    {"a * 1","a"},
    {"b + a + 1 - 2","-1+a+b"},
    {"(b + a) + 1","1+a+b"},
    {"(b + a) * 1","a+b"},
    {"-(a + b)","-a-b"},
    {"-(-(-(a - b)))","-a+b"},
    {"1+1","2"},
    {"1-1","0"},
    {"a-a","0"},
    {"1+1","2"},
    {"1-1","0"},
    {"a+a","2*a"},
    {"a-a","0"},
    {"-a-a","-2*a"},
    {"a+b","a+b"},
    {"a*b","a*b"},
    {"a+b*c","a+b*c"},
    {"a*b+c","c+a*b"},
    {"a+(a)","2*a"},
    {"a+(b*c)","a+b*c"},
    {"a*(b+c)","a*b+a*c"},
    {"2*3","6"},
    {"a*3*b*c*a*b*c*2","6*a*a*b*b*c*c"},
    {"a+b+c+d","a+b+c+d"},
    {"a+a+b+b+2*c+c","2*b+3*c+2*a"},
    {"a*b+a*b","2*a*b"},
    {"-((1*x)*(3-1))","-2*x"},
    {"-((1*x)*(3-1))+1*y","y-2*x"},
    {"-a*b-a*b","-2*a*b"},
    {"-a-b-(a-b)-(-a+b)-(-(a-b)-(-a+b)+(a-b) + (-a+b))","-a-b"},
    {"a*b + a*b + 2*a*b","4*a*b"},
    {"1+2+3+1*20*30","606"},
    {"a * (x + y)","a*x+a*y"},
    {"-6*(4-1)+5","-13"},
    {"-(1*(x-1))+0","1-x"},
    {"a/b/c","a/(b*c)"},
    {"a/(b/c)","(a*c)/b"},
    {"-a/b","-a/b"},
    {"-a/-b","a/b"},
    {"a/-b","-a/b"},
    {"-(a/b)","-a/b"},
#endif
  };

  TEMP_REGION(temp,nullptr);

#if 0
  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("2*x"),SYM_Variable("x"))));
  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("2*x"),Negate(SYM_Variable("x")))));

  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("x + x*y"),SYM_Variable("x"))));
  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("x + x*y"),Negate(SYM_Variable("x")))));

  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("-x + x*y"),SYM_Variable("x"))));
  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("-x + x*y"),Negate(SYM_Variable("x")))));

  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("-x - x*y"),SYM_Variable("x"))));
  SYM_Print(SYM_Normalize(SYM_Factor(SYM_Parse("-x - x*y"),Negate(SYM_Variable("x")))));
#endif

#if 1
  for(TestCase t : tests){
    SYM_Expr expr = SYM_Parse(t.input);

    String inputRepr = SYM_ReprHier(expr,temp);

    NormalizeResult normalize = NormalizeWhileChecking(expr);

    SYM_Expr res = normalize.res;
    String result = SYM_Repr(res,temp);
    String resultHier = SYM_ReprHier(res,temp);

    if(normalize.failedEvaluation){
      printf("Failed evaluation\n");
    } else if(result != t.expectedNormalized){
      printf("Diff:\n Input:%.*s\n InputRepr:\n%.*s\n Expected:%.*s\n Got:%.*s\n Hier:%.*s\n",UN(t.input),UN(inputRepr),UN(t.expectedNormalized),UN(result),UN(resultHier));
    } else {
      printf("Ok: Input:  %.*s, Result: %.*s\n",UN(t.input),UN(result));
    }
  }
#endif

#if 0
  int checked = 0;
  for(int j = 3; j < 15; j++){
    printf("%d\n",j);
    for(int i = 0; i < 5000; i++){
      SeedRandomNumber(j * 100000 + i);
      SYM_Expr test = GenerateRandomExpression(j);

      //SYM_Print(test);
      NormalizeResult normalize = NormalizeWhileChecking(test);

      if(normalize.failedEvaluation && !normalize.failedFromDivZero){
        printf("Failed normalization evaluation (%.*s:%d) (%d:%d): %d vs %d\n",UN(normalize.problematicFunction),normalize.functionLine,j,i,normalize.expected,normalize.gotOnError);
        SYM_Print(test);
        SYM_Print(normalize.res);
      } else {
        checked += 1;
        //printf("OK\n");
      }

      //return;
    }
  }
  printf("Checked: %d\n",checked);
  return;
#endif
}

void SYM_Init(){
  static Arena arenaInst = InitArena(Megabyte(4));
  SYM_State.arena = &arenaInst;

  SYM_Zero = GetOrAllocateLiteral(0);
  SYM_One = GetOrAllocateLiteral(1);
  SYM_Two = GetOrAllocateLiteral(2);
  SYM_Eight = GetOrAllocateLiteral(8);

  SYM_AddrW = GetOrAllocateVariable("ADDR_W");
  SYM_AxiAddrW = GetOrAllocateVariable("AXI_ADDR_W");
  SYM_AxiDataW = GetOrAllocateVariable("AXI_DATA_W");
  SYM_AxiStrobeW = SYM_AxiDataW / SYM_Eight;
  SYM_LenW = GetOrAllocateVariable("LEN_W");
  SYM_DelayW = GetOrAllocateVariable("DELAY_W");
  SYM_DataW = GetOrAllocateVariable("DATA_W");
  SYM_DataStrobeW = SYM_DataW / SYM_Eight;
}

char* SYM_DebugRepr(SYM_Expr expr){
  TEMP_REGION(temp,nullptr);
  String str = SYM_Repr(expr,temp);

  return SF("%.*s",UN(str));
}

#if 0
SYM_Expr GetNormalized(SYM_Expr in){
  SYM_Node* node = GetPointer(in);
   bool negate = IsNegative(in);

  if(node->isNormalized){
    return in;
  }

  if(node->isNormalizedNodeValid){
    node = node->normalizedNode;
  }

  SYM_Expr res = {node};
  res = CondNegate(res,negate);
  
  return res;
}
#endif
