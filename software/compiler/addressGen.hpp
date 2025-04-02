#pragma once

#include "utils.hpp"
#include "memory.hpp"

struct SymbolicExpression;
struct LinearSum;
struct LoopLinearSum;

// NOTE: The code and the usage of symbolic expressions is really bad performance/memory usage wise. We are also blind to the amount of "extra" work we are doing, meaning that we are probably using megabytes for situations where we could just use a few kilobytes. Not important right now but eventually need to start at least visualizing how "bad" the situation is.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect.

// NOTE: The entire approach that we are taking is starting to become clubersome when having to deal with variables inside the expressions. The problem is that we are tackling a lot of complexity, mainly when having to deal with the fact that what we really want is to have every variable mult term easily separated and easily accessible, when in reality, because everything is inside a single symbolic expression, accessing individual terms is actually hard.

// I think that the best approach is to remove the single expression and to start storing one expression per loop variable plus a single expression for any constant. That way we can easily access the mult terms that we actually care about. The expression will not contain the loop variable (there is no need, we already know that we have one) and as such we are operation on the "leftovers" immediatly.

// I want everything using symbolic expressions.
struct LoopDefinition{
  String loopVariableName;
  SymbolicExpression* start;
  SymbolicExpression* end;
};

// TODO: This sucks. We would like for 0 to be the innermost and size-1 to be the outermost. Makes more sense logically.
// TODO: REMOVE this after LoopLinearSum change.
struct SingleAddressAccess{
  // 0 is the outermost loop, size - 1 is the innermost.
  Array<LoopDefinition> loops;
  SymbolicExpression* address;
  
  Array<SymbolicExpression*> variableTerms;
  SymbolicExpression* freeTerm;
};

struct AddressAccess{
  LoopLinearSum* internal;
  LoopLinearSum* external;
  
  Array<String> inputVariableNames;
};

SingleAddressAccess* CopySingleAddressAccess(SingleAddressAccess* in,Arena* out);

void PrintAccess(AddressAccess* access);
AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxLoopsTotal,int maxExternalLoops,Arena* out);

