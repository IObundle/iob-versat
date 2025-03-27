#pragma once

#include "utils.hpp"

// TODO: We might have to implement a way of grouping variables, so we can group the loop variables for an address access. This might simplify things somewhat. Need to see further how the code turns out for the address access

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
  SymbolicExpression* left;
  SymbolicExpression* right;
  Array<SymbolicExpression*> sum;
};

struct MultPartition{
  SymbolicExpression* base;
  Array<SymbolicExpression*> leftovers;
}; 

struct TermsWithLiteralMultiplier{
  // Negative is removed from mulTerms and pushed to literal
  SymbolicExpression* mulTerms; 
  SymbolicExpression* literal;
};

void Print(SymbolicExpression* expr);
String PushRepresentation(SymbolicExpression* expr,Arena* out);

int Evaluate(SymbolicExpression* expr,Hashmap<String,int>* values);

SymbolicExpression* PushLiteral(Arena* out,int value,bool negate = false);
SymbolicExpression* PushVariable(Arena* out,String name,bool negate = false);

SymbolicExpression* SymbolicAdd(SymbolicExpression* left,SymbolicExpression* right,Arena* out);
SymbolicExpression* SymbolicSub(SymbolicExpression* left,SymbolicExpression* right,Arena* out);
SymbolicExpression* SymbolicMult(SymbolicExpression* left,SymbolicExpression* right,Arena* out);

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out);
SymbolicExpression* ParseSymbolicExpression(String content,Arena* out);

SymbolicExpression* SymbolicDeepCopy(SymbolicExpression* expr,Arena* out);

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

void TestSymbolic();

// Expr must be a sum of mul. Assuming that variableName only appears once. TODO: Probably would be best to create a function that first groups everything so that variableName only appears once and then call this function. Or maybe do the grouping inside here if needed.
Opt<SymbolicExpression*> GetMultExpressionAssociatedTo(SymbolicExpression* expr,String variableName,Arena* out);

SymbolicExpression* RemoveMulLiteral(SymbolicExpression* expr,Arena* out);
