#pragma once

#include "utils.hpp"

// TODO: We might have to implement a way of grouping variables, so we can group the loop variables for an address access. This might simplify things somewhat. Need to see further how the code turns out for the address access

// TODO: A lot of places are normalizing when there is no need. The more normalizations we perform on the lower levels the worse performance and memory penalties we incur. Normalization only needs to be performed if needed, otherwise try to keep whatever format we have

struct Tokenizer;
struct Arena;

enum SymbolicExpressionType{
  SymbolicExpressionType_LITERAL,
  SymbolicExpressionType_VARIABLE,
  SymbolicExpressionType_SUM,
  SymbolicExpressionType_MUL,
  SymbolicExpressionType_DIV
};

struct SymbolicExpression{
  SymbolicExpressionType type;
  bool negative;

  // All these should be inside a union. Not handling this for now
  int literal;
  String variable;

  // For div, imagine a fraction expression
  struct {
    SymbolicExpression* top;
    SymbolicExpression* bottom;
  };
  
  Array<SymbolicExpression*> sum;
};

struct MultPartition{
  SymbolicExpression* base;
  SymbolicExpression* leftovers;
}; 

void Print(SymbolicExpression* expr);
String PushRepresentation(SymbolicExpression* expr,Arena* out);

int Evaluate(SymbolicExpression* expr,Hashmap<String,int>* values);

SymbolicExpression* PushLiteral(Arena* out,int value,bool negate = false);
SymbolicExpression* PushVariable(Arena* out,String name,bool negate = false);

SymbolicExpression* SymbolicAdd(SymbolicExpression* left,SymbolicExpression* right,Arena* out);
SymbolicExpression* SymbolicAdd(Array<SymbolicExpression*> terms,Arena* out);
SymbolicExpression* SymbolicAdd(Array<SymbolicExpression*> terms,SymbolicExpression* extra,Arena* out);
SymbolicExpression* SymbolicSub(SymbolicExpression* left,SymbolicExpression* right,Arena* out);
SymbolicExpression* SymbolicMult(SymbolicExpression* left,SymbolicExpression* right,Arena* out);
SymbolicExpression* SymbolicMult(Array<SymbolicExpression*> terms,Arena* out);
SymbolicExpression* SymbolicMult(Array<SymbolicExpression*> terms,SymbolicExpression* extra,Arena* out);
SymbolicExpression* SymbolicDiv(SymbolicExpression* top,SymbolicExpression* bottom,Arena* out);

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out);
SymbolicExpression* ParseSymbolicExpression(String content,Arena* out);

SymbolicExpression* SymbolicDeepCopy(SymbolicExpression* expr,Arena* out);

// Use this function to get the literal value of a literal type expression, takes into account negation.
int GetLiteralValue(SymbolicExpression* expr);

// Must call normalizeLiteral before calling this.
// Furthermore, the negative is removed from leftovers and pushed onto the literal (base)
MultPartition CollectTermsWithLiteralMultiplier(SymbolicExpression* expr,Arena* out);

MultPartition PartitionMultExpressionOnVariable(SymbolicExpression* expr,String variableToPartition,Arena* out);

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out);
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out);
// This function does not allocate new nodes unless it has to.
// Meaning that memory associated to expr must be kept valid as well as the memory for the return of this function
SymbolicExpression* CollectTerms(SymbolicExpression* expr,Arena* out);
SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out);
SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out);

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out);

SymbolicExpression* SymbolicReplace(SymbolicExpression* base,String varToReplace,SymbolicExpression* replacingExpr,Arena* out);

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,bool debugPrint = false);
SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out);

// Are allowed to call normalize
SymbolicExpression* Group(SymbolicExpression* expr,String variableToGroupWith,Arena* out);

void TestSymbolic();

// Expr must be a sum of mul. Assuming that variableName only appears once. TODO: Probably would be best to create a function that first groups everything so that variableName only appears once and then call this function. Or maybe do the grouping inside here if needed.
Opt<SymbolicExpression*> GetMultExpressionAssociatedTo(SymbolicExpression* expr,String variableName,Arena* out);

SymbolicExpression* RemoveMulLiteral(SymbolicExpression* expr,Arena* out);

// A more simpler representation for expressions of the form f() * x + g() * y + ...
// Where x and y are variables and f and g are symbolic expressions that do not contain the x and y variables. 
// Variables are not represented inside the symbolic expression (they are implicit).
// The terms are easily acessible this way, as we do not need to pick apart a single symbolic expression that way.
struct LinearSum{
  // SoA approach, worth it? Or move to a AoS?
  Array<String> variableNames;
  Array<SymbolicExpression*> variableTerms;
  SymbolicExpression* freeTerm;
};

LinearSum* BreakdownSymbolicExpression(SymbolicExpression* expr,Array<String> variables,Arena* out);
SymbolicExpression* TransformIntoSymbolicExpression(LinearSum* sum,Arena* out);
LinearSum* Copy(LinearSum* in,Arena* out);

LinearSum* PushOneVarSum(String baseVar,Arena* out);
LinearSum* AddLinearSum(LinearSum* left,LinearSum* right,Arena* out);
void Print(LinearSum* sum);

struct LoopLinearSumTerm{
  String var;
  SymbolicExpression* term;
  SymbolicExpression* loopStart;
  SymbolicExpression* loopEnd;
};

// This basically plays the role of the SingleAddressAccess.
struct LoopLinearSum{
  // 0 is the innermost and size-1 the outermost loop
  Array<LoopLinearSumTerm> terms;
  SymbolicExpression* freeTerm;
};

LoopLinearSum* PushLoopLinearSumEmpty(Arena* out);
LoopLinearSum* PushLoopLinearSumFreeTerm(SymbolicExpression* term,Arena* out);
LoopLinearSum* PushLoopLinearSumSimpleVar(String loopVarName,SymbolicExpression* term,SymbolicExpression* start,SymbolicExpression* end,Arena* out);

LoopLinearSum* Copy(LoopLinearSum* in,Arena* out);
LoopLinearSum* AddLoopLinearSum(LoopLinearSum* inner,LoopLinearSum* outer,Arena* out);
LoopLinearSum* RemoveLoop(LoopLinearSum* in,int index,Arena* out);
SymbolicExpression* TransformIntoSymbolicExpression(LoopLinearSum* sum,Arena* out);

SymbolicExpression* GetLoopLinearSumTotalSize(LoopLinearSum* in,Arena* out);

void Print(LoopLinearSum* sum);
