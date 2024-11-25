#pragma once

#include "utils.hpp"

struct Tokenizer;
struct Arena;

enum SymbolicExpressionType{
  SymbolicExpressionType_LITERAL,
  SymbolicExpressionType_VARIABLE,
  SymbolicExpressionType_OP, // For now this is only div. addition and multiply use ARRAY and sub does not exist.
  SymbolicExpressionType_ARRAY // Arrays are for + and *. Since they are commutative, easier to handle them in an array setting. 
};

struct SymbolicExpression{
  SymbolicExpressionType type;

  // All these should be inside a union. Not handling this for now
  int literal;
  String variable;
  SymbolicExpression* left;
  SymbolicExpression* right;
  bool negative;
  char op; // Used by both OP and ARRAY
  Array<SymbolicExpression*> sum;
};

// TODO: It might be better to replace all the multiplications with something based on this approach.
//       It is nice to only have a place to store negation (inside the literal).
//       The only problem is the interaction with DIV.
//       There is maybe some opportunity of representing negation as just the multiplication by a negative literal. Instead of having a negative flag, we just make it so negation is basically putting a mul by -1. Would this make things simpler? Idk, adding one more layer of AST seems to do the opposite. Maybe it would be easier if we did not add any more layer. Make it so that literals are numbers, variables are a number and a string, additions are an array, and multiplication are a number and an array.

//       Anyway, need to handle division properly. Only when division is working and we can simplify the majority of equations into a nice form should we even start wondering about simplifying the code. Division might break a lot of our assumptions right now, no point in rushing, because the code is still malleable.

struct TermsWithLiteralMultiplier{
  // Negative is removed from mulTerms and pushed to literal
  SymbolicExpression* mulTerms; 
  SymbolicExpression* literal;
  //bool isDiv;
};

void Print(SymbolicExpression* expr,bool top = true);

SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out,Arena* temp);
SymbolicExpression* NormalizeLiterals(SymbolicExpression* expr,Arena* out,Arena* temp);
SymbolicExpression* RemoveParenthesis(SymbolicExpression* expr,Arena* out,Arena* temp);
// This function does not allocate new nodes unless it has to.
// Meaning that memory associated to expr must be kept valid as well as the memory for the return of this function
SymbolicExpression* CollectTerms(SymbolicExpression* expr,Arena* out,Arena* temp);
SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out,Arena* temp);
SymbolicExpression* ApplySimilarTermsAddition(SymbolicExpression* expr,Arena* out,Arena* temp);

SymbolicExpression* Normalize(SymbolicExpression* expr,Arena* out,Arena* temp);
SymbolicExpression* Derivate(SymbolicExpression* expr,String base,Arena* out,Arena* temp);

/*
  For now, we store additions in a array and everything else in a AST like format.
  Or, maybe what we want is to store equal operations into an array.
  
// The fundamental reason why I want to collect all the terms into an array is beacuse of commutativity. By having everything into an array, it is easier to figure out how to 

// Let's imagine that we have an AST like view of the expression.

If we stored additions in an array, we could have

(A,B,C)

A:*
a   *
  2  (x,y)

B: a

C:
a + b

What operations do we need?
Distributive.
Push all divisions into a single final division. (To handle duty)



*/
