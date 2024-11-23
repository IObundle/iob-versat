#pragma once

#include "utils.hpp"

struct Tokenizer;
struct Arena;

enum SymbolicExpressionType{
  SymbolicExpressionType_LITERAL,
  SymbolicExpressionType_VARIABLE,
  SymbolicExpressionType_OP,
  SymbolicExpressionType_ARRAY
}; 

struct SymbolicExpression{
  SymbolicExpressionType type;

  // All these should be inside a union. Not handling this for now
  int literal;
  String variable;
  SymbolicExpression* left;
  SymbolicExpression* right;
  char op; // Used by both OP and ARRAY
  Array<SymbolicExpression*> sum;
};

void Print(SymbolicExpression* expr);
SymbolicExpression* ParseSymbolicExpression(Tokenizer* tok,Arena* out,Arena* temp);
SymbolicExpression* ApplyIdentity(SymbolicExpression* expr,Arena* out,Arena* temp);

// This function does not allocate new nodes unless it has to.
// Meaning that memory associated to expr must be kept valid as well as the memory for the return of this function
SymbolicExpression* ApplyDistributivity(SymbolicExpression* expr,Arena* out,Arena* temp);

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
