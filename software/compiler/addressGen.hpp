#pragma once

#include "utils.hpp"
#include "memory.hpp"

#include "embeddedData.hpp"

#include "symbolic.hpp"
#include "parser.hpp"

struct CEmitter;
struct SpecExpression;
struct MathExpression;
struct Env;

// TODO: We currently do not support loops that start at non zero values. The fix is simple, we can always shift loops from N..M to 0..(M-N) by adding more logic to the expression. Kinda not doing this for now since still have not found an example where this is needed.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect if needed.

struct AddressGenForDef{
  String loopVariable;
  MathExpression* startSym;
  MathExpression* endSym;
};

// nocheckin: TODO: Is there a point to separating the internal and external stuff at this point?
//                  We could just store a single loop and only do the separating afterwards.
//                  We would probably simplify a bunch of things
struct AddressAccess{
  String name;
  LoopLinearSum* internal;
  LoopLinearSum* external;

  SYM_Expr dutyDivExpr; // Any expression of the form (A/B) is broken up, this var saves B and the internal/external LoopLinearSum take the A part. For a non div expression this stores '1'.
  
  Array<String> inputVariableNames;
  Array<String> loopVars;
};

struct ExternalMemoryAccess{
  String totalTransferSize;
  String length;
  String amountMinusOne;
  String addrShift;
};

struct InternalMemoryAccess{
  SYM_Expr periodExpression;
  SYM_Expr incrementExpression;

  SYM_Expr iterationExpression;
  SYM_Expr shiftExpression;

  SYM_Expr dutyExpression;

  SYM_Expr shiftWithoutRemovingIncrement; // Shift as if period did not change addr. Useful for current implementation of VRead/VWrites
};

struct CompiledAccess{
  Array<InternalMemoryAccess> internalAccess;
  SYM_Expr dutyDivExpression; // The actual value of the division (For expression on the form A/B, store B)
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
AddressAccess* CompileAddressGen(Env* env,Array<Token> inputs,Array<AddressGenForDef> loops,SYM_Expr addr,String content);

// ======================================
// Conversion

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out);
AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out);

// ======================================
// 

AddressAccess* ReplaceVariables(AddressAccess* in,TrieMap<String,SYM_Expr>* varReplace,Array<String> newInputVariableNames,Arena* out);

// ======================================
// Code emission

void EmitReadStatements(CEmitter* m,AccessAndType access,String varName,String extVarName);
void EmitMemStatements(CEmitter* m,AccessAndType access,String varName);
void EmitGenStatements(CEmitter* m,AccessAndType access,String varName);

// ======================================
// LoopLinearSumTerm handling

SYM_Expr GetLoopHighestDecider(LoopLinearSumTerm* term);

