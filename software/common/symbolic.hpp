#pragma once

#include "utils.hpp"

// TODO: The logic in relation to negative is becoming a bit tiring. If we need to keep expanding this code, might need to find some pattern to simplify things. The main problem is when the structure is changed by any of the functions, in these cases the logic for how negativity is preserved is more complicated than simply copying what we already have.

// TODO: We might have to implement a way of grouping variables, so we can group the loop variables for an address access. This might simplify things somewhat. Need to see further how the code turns out for the address access

// TODO: A lot of places are normalizing when there is no need. The more normalizations we perform on the lower levels the worse performance and memory penalties we incur. Normalization only needs to be performed if needed, otherwise try to keep whatever format we have

struct Tokenizer;
struct Arena;

// ============================================================================
// Symbolic Expressions

enum SymbolicExpressionType{
  SymbolicExpressionType_LITERAL,
  SymbolicExpressionType_VARIABLE,
  SymbolicExpressionType_SUM,
  SymbolicExpressionType_MUL,
  SymbolicExpressionType_DIV,
  SymbolicExpressionType_FUNC
};

enum SymbolicReprType{
  SymbolicReprType_LITERAL,
  SymbolicReprType_VARIABLE,
  SymbolicReprType_OP
};

struct SymbolicReprAtom{
  SymbolicReprType type;
  
  union{
    String variable;
    int literal;
    char op;
  };
};

struct SymbolicExpression{
  SymbolicExpressionType type;
  bool negative;

  // All these should be inside a union. Not handling this for now
  int literal;

  union {
    String variable;
    String name;
  };
    
  // For div, imagine a fraction expression
  struct {
    SymbolicExpression* top;
    SymbolicExpression* bottom;
  };
  
  Array<SymbolicExpression*> terms; // Terms of SUM and MUL. Function arguments for FUNC
};

struct MultPartition{
  SymbolicExpression* base;
  SymbolicExpression* leftovers;
}; 

// ============================================================================
// Loop Linear Sum

struct LoopLinearSumTerm{
  String var;
  SymbolicExpression* term;
  SymbolicExpression* loopStart;
  SymbolicExpression* loopEnd;
};

// A Sum of expressions in the form term * X + term * Y + term * Z + ... + constant, where X,Y and Z are loop variables that range from a start expression to a end expression.
struct LoopLinearSum{
  // 0 is the innermost and size-1 the outermost loop
  Array<LoopLinearSumTerm> terms;
  SymbolicExpression* freeTerm;
};

// ======================================
// Globals

extern SymbolicExpression* SYM_zero;
extern SymbolicExpression* SYM_one;
extern SymbolicExpression* SYM_two;
extern SymbolicExpression* SYM_eight;
extern SymbolicExpression* SYM_thirtyTwo;
extern SymbolicExpression* SYM_dataW;
extern SymbolicExpression* SYM_addrW;
extern SymbolicExpression* SYM_axiAddrW;
extern SymbolicExpression* SYM_axiDataW;
extern SymbolicExpression* SYM_delayW;
extern SymbolicExpression* SYM_lenW;
extern SymbolicExpression* SYM_axiStrobeW;
extern SymbolicExpression* SYM_dataStrobeW;

// ======================================
// Representation

void   Print(SymbolicExpression* expr,bool printNewLine = false);
void   Repr(StringBuilder* builder,SymbolicExpression* expr);
String PushRepr(Arena* out,SymbolicExpression* expr);
char*  DebugRepr(SymbolicExpression* expr);

// ======================================
// High level representation compilation

Array<SymbolicReprAtom> CompileRepresentation(SymbolicExpression* expr,Arena* out);

// ======================================
// Evaluation, mostly debugging purposes for now.

int Evaluate(SymbolicExpression* expr,Hashmap<String,int>* values);

// ======================================
// Low level symbolic building blocks

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

SymbolicExpression* SymbolicFunc(String functionName,Array<SymbolicExpression*> terms,Arena* out);

// ======================================
// Parsing

// nocheckin: Either remove it or move it to new parser.
SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out);
SymbolicExpression* ParseSymbolicExpression(String content,Arena* out);

// ======================================
// Low level manipulation

// Use this function to get the literal value of a literal type expression, takes into account negation.
int GetLiteralValue(SymbolicExpression* expr);
Array<String> GetAllSymbols(SymbolicExpression* expr,Arena* out);

bool ExpressionEqual(SymbolicExpression* left,SymbolicExpression* right);
bool IsZero(SymbolicExpression* expr);

SymbolicExpression* SymbolicDeepCopy(SymbolicExpression* expr,Arena* out);

// Must call normalizeLiteral before calling this.
// Furthermore, the negative is removed from leftovers and pushed onto the literal (base)
MultPartition CollectTermsWithLiteralMultiplier(SymbolicExpression* expr,Arena* out);

MultPartition PartitionMultExpressionOnVariable(SymbolicExpression* expr,String variableToPartition,Arena* out);

// ======================================
// High level symbolic manipulations 

SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out);
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out);
// This function does not allocate new nodes unless it has to.
// Meaning that memory associated to expr must be kept valid as well as the memory for the return of this function
SymbolicExpression* CollectTerms(SymbolicExpression* expr,Arena* out);
SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out);
SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out);
SymbolicExpression* MoveDivToTop(SymbolicExpression* base,Arena* out);
SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out);
SymbolicExpression* SymbolicReplace(SymbolicExpression* base,String varToReplace,SymbolicExpression* replacingExpr,Arena* out);
SymbolicExpression* ReplaceVariables(SymbolicExpression* expr,Hashmap<String,SymbolicExpression*>* values,Arena* out);
SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,bool debugPrint = false);
SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out);

// Are allowed to call normalize
SymbolicExpression* Group(SymbolicExpression* expr,String variableToGroupWith,Arena* out);

void TestSymbolic();

// ============================================================================
// LoopLinearSums

// Expr must be a sum of mul. Assuming that variableName only appears once. TODO: Probably would be best to create a function that first groups everything so that variableName only appears once and then call this function. Or maybe do the grouping inside here if needed.
// TODO: This function is kinda not needed. We only use it to break apart a symbolic expression into a LoopLinearSumTerm, but even then we can do it better because we know the format of the variables inside the symbolic expression and we could simplify this.
Opt<SymbolicExpression*> GetMultExpressionAssociatedTo(SymbolicExpression* expr,String variableName,Arena* out);

// ======================================
// Building

LoopLinearSum* PushLoopLinearSumEmpty(Arena* out);
LoopLinearSum* PushLoopLinearSumFreeTerm(SymbolicExpression* term,Arena* out);
LoopLinearSum* PushLoopLinearSumSimpleVar(String loopVarName,SymbolicExpression* term,SymbolicExpression* start,SymbolicExpression* end,Arena* out);

// ======================================
// Manipulation

LoopLinearSum* Copy(LoopLinearSum* in,Arena* out);
LoopLinearSum* AddLoopLinearSum(LoopLinearSum* inner,LoopLinearSum* outer,Arena* out);
LoopLinearSum* RemoveLoop(LoopLinearSum* in,int index,Arena* out);
SymbolicExpression* TransformIntoSymbolicExpression(LoopLinearSum* sum,Arena* out);

SymbolicExpression* GetLoopLinearSumTotalSize(LoopLinearSum* in,Arena* out);

// ======================================
// Representation

void Print(LoopLinearSum* sum,bool printNewLine = false);
void Repr(StringBuilder* builder,LoopLinearSum* sum);
String PushRepr(LoopLinearSum* sum,Arena* out);
