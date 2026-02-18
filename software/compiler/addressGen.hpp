#pragma once

#include "utils.hpp"
#include "memory.hpp"
#include "parser.hpp"

#include "embeddedData.hpp"

struct SymbolicExpression;
struct LoopLinearSum;
struct LoopLinearSumTerm;
struct CEmitter;
struct SpecExpression;

// TODO: We currently do not support loops that start at non zero values. The fix is simple, we can always shift loops from N..M to 0..(M-N) by adding more logic to the expression. Kinda not doing this for now since still have not found an example where this is needed.

// NOTE: The code and the usage of symbolic expressions is really bad performance/memory usage wise. We are also blind to the amount of "extra" work we are doing, meaning that we are probably using megabytes for situations where we could just use a few kilobytes. It's mostly temp memory so not important right now but eventually need to start at least visualizing how "bad" the situation is.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect if needed.

struct AddressGenForDef{
  Token loopVariable;
  Array<Token> startSym;
  Array<Token> endSym;
};

struct AddressGenForDef2{
  Token loopVariable;
  SpecExpression* startSym;
  SpecExpression* endSym;
};

struct AddressAccess{
  String name;
  LoopLinearSum* internal;
  LoopLinearSum* external;

  SymbolicExpression* dutyDivExpr; // Any expression of the form (A/B) is broken up, this var saves B and the internal/external LoopLinearSum take the A part (otherwise this is nullptr). This simplifies a lot of things, since we only care about B at the very end. 
  
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

struct CompiledAccess{
  Array<InternalMemoryAccess> internalAccess;
  String dutyDivExpression;
};

struct AddressGenInst{
  AddressGenType type;
  int loopsSupported;
};

struct AccessAndType{
  AddressAccess* access;
  AddressGenInst inst;

  // For mem type of accesses. TODO: Check if we actually want data here or not.
  int port;
  Direction dir;
};

// ======================================
// Misc (Probably gonna move these around eventually)

Array<Pair<String,String>> InstantiateRead(AddressAccess* access,int highestExternalLoop,bool doubleLoop,int maxLoops,String extVarName,Arena* out);

// ======================================
// Representation

void   Repr(StringBuilder* builder,AddressAccess* access);
String PushRepr(Arena* out,AddressAccess* access);
void   Print(AddressAccess* access);

// ======================================
// Compilation 

// TODO: We probably want to take in an Env* so that we can check stuff and we probably want to move this to the spec parser. No reason for other code to have token and to depend on parser stuff.
AddressAccess* CompileAddressGen(Array<Token> inputs,Array<AddressGenForDef2> loops,SymbolicExpression* addr,String content);

// ======================================
// Conversion

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out);
AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out);

// ======================================
// Code emission

void EmitReadStatements(CEmitter* m,AccessAndType access,String varName,String extVarName);
void EmitMemStatements(CEmitter* m,AccessAndType access,String varName);
void EmitGenStatements(CEmitter* m,AccessAndType access,String varName);

// ======================================
// LoopLinearSumTerm handling

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term);

