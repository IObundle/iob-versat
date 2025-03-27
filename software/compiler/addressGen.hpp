#pragma once

#include "utils.hpp"
#include "memory.hpp"

struct SymbolicExpression;

// I want everything using symbolic expressions.

struct LoopDefinition{
  String loopVariableName;
  SymbolicExpression* start;
  SymbolicExpression* end;
};

struct AddressAccess{
  // 0 is the outermost loop, size - 1 is the innermost.
  Array<LoopDefinition> externalLoops;
  Array<LoopDefinition> internalLoops;

  Array<String> inputVariableNames;
  
  SymbolicExpression* externalAddress;
  SymbolicExpression* internalAddress;
};

void PrintAccess(AddressAccess* access);

AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxLoopsTotal,int maxExternalLoops,Arena* out);
