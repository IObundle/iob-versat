#pragma once

#include "utils.hpp"
#include "memory.hpp"

struct SymbolicExpression;

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect.

// I want everything using symbolic expressions.
struct LoopDefinition{
  String loopVariableName;
  SymbolicExpression* start;
  SymbolicExpression* end;
};

struct SingleAddressAccess{
  // 0 is the outermost loop, size - 1 is the innermost.
  Array<LoopDefinition> loops;
  SymbolicExpression* address;
};

struct AddressAccess{
  SingleAddressAccess* external;
  SingleAddressAccess* internal;
  
  Array<String> inputVariableNames;
};

SingleAddressAccess CopySingleAddressAccess(SingleAddressAccess in,Arena* out);

void PrintAccess(AddressAccess* access);
AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxLoopsTotal,int maxExternalLoops,Arena* out);

