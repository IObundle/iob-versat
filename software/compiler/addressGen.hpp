#pragma once

#include "utils.hpp"
#include "memory.hpp"

#include "embeddedData.hpp"

struct SymbolicExpression;
struct LoopLinearSum;
struct LoopLinearSumTerm;

// TODO: We currently do not support loops that start at non zero values. The fix is simple, we can always shift loops from N..M to 0..(M-N) by adding more logic to the expression. Kinda not doing this for now since still have not found an example where this is needed.

// NOTE: The code and the usage of symbolic expressions is really bad performance/memory usage wise. We are also blind to the amount of "extra" work we are doing, meaning that we are probably using megabytes for situations where we could just use a few kilobytes. It's mostly temp memory so not important right now but eventually need to start at least visualizing how "bad" the situation is.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect.

struct AddressAccess{
  LoopLinearSum* internal;
  LoopLinearSum* external;
  
  Array<String> inputVariableNames;
};

struct ExternalMemoryAccess{
  String totalTransferSize;
  String length;
  String amountMinusOne;
  String addrShift;
};

struct InternalMemoryAccess{
  String periodExpression;
  String incrementExpression;

  String iterationExpression;
  String shiftExpression;

  String dutyExpression; // Non empty if it exists

  String shiftWithoutRemovingIncrement; // Shift as if period did not change addr. Useful for current implementation of VRead/VWrites
};

void Print(AddressAccess* access);

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out);
AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out);

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term);

AddressVParameters InstantiateAccess(AddressAccess* access,Arena* out);


