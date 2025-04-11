#pragma once

#include "utils.hpp"
#include "memory.hpp"

struct SymbolicExpression;
struct LinearSum;
struct LoopLinearSum;
struct LoopLinearSumTerm;

// TODO: We currently do not support loops that start at non zero values. The fix is simple, we can always shift loops from N..M to 0..(M-N) by adding more logic to the expression. Kinda not doing this for now since still have not found an example where this is needed.

// NOTE: The code and the usage of symbolic expressions is really bad performance/memory usage wise. We are also blind to the amount of "extra" work we are doing, meaning that we are probably using megabytes for situations where we could just use a few kilobytes. It's mostly temp memory so not important right now but eventually need to start at least visualizing how "bad" the situation is.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect.

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


SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term);
AddressAccess* ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(AddressAccess* access,int biggestLoopIndex,Arena* out);
