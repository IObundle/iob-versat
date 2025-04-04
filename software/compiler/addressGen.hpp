#pragma once

#include "utils.hpp"
#include "memory.hpp"

struct SymbolicExpression;
struct LinearSum;
struct LoopLinearSum;
struct LoopLinearSumTerm;

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

struct AddressAccess{
  LoopLinearSum* internal;
  LoopLinearSum* external;
  
  Array<String> inputVariableNames;
};

void Print(AddressAccess* access);
AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxInternalLoops,int maxExternalLoops,Arena* out);

AddressAccess* ConvertAccessSingleLoop(AddressAccess* access,Arena* out);



// NOTE: If I have a small constant but the loop is huge, doesnt that affect the decision for the biggest term?

// Ex:
// for y 0..2
// for x 0..100
// addr = 2 * x + 5*y

// The problem is that, for this case, what we want is to do only one loop.
// Can I figure out an expression that tells me if a one loop is better than a two loop?
// Since I can always generate a 2 loop expression and a one loop expression from any N>=2 loop initial address generator, I can also always check their "size" by calculating the amount of values needed by the external access of each address generator.
// Meaning that we do not actually need anything fancy. Generate the 2 loop version, generate the 1 loop version, create an expression that compares the amount of elements that each reads and select the one whose expression is smaller, following the exact same approach taken by the selector.

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term);
AddressAccess* ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(AddressAccess* access,int biggestLoopIndex,Arena* out);
