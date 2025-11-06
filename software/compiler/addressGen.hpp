#pragma once

#include "utils.hpp"
#include "memory.hpp"

#include "embeddedData.hpp"

struct SymbolicExpression;
struct LoopLinearSum;
struct LoopLinearSumTerm;
struct AddressGenDef;

// TODO: We currently do not support loops that start at non zero values. The fix is simple, we can always shift loops from N..M to 0..(M-N) by adding more logic to the expression. Kinda not doing this for now since still have not found an example where this is needed.

// NOTE: The code and the usage of symbolic expressions is really bad performance/memory usage wise. We are also blind to the amount of "extra" work we are doing, meaning that we are probably using megabytes for situations where we could just use a few kilobytes. It's mostly temp memory so not important right now but eventually need to start at least visualizing how "bad" the situation is.

// NOTE: Majority of the approach taken in relation to memory allocations and how much we mutate data is not final, we do not care about things like that currently. More important is to start making the code correct and producing the correct data and later we can rewrite the code to be better in this aspect if needed.

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

template<> struct std::hash<AddressGenInst>{
   std::size_t operator()(AddressGenInst const& s) const noexcept{
     std::size_t res = (int)(s.type) * (int) s.loopsSupported;
     return res;
   }
};

static bool operator==(const AddressGenInst& l,const AddressGenInst& r){
  if(l.type == r.type && l.loopsSupported == r.loopsSupported){
    return true;
  }
  
  return false;
}

struct AccessAndType{
  AddressAccess* access;
  AddressGenInst inst;
};

template<> struct std::hash<AccessAndType>{
   std::size_t operator()(AccessAndType const& s) const noexcept{
     std::size_t res = std::hash<void*>()(s.access) * std::hash<AddressGenInst>()(s.inst);
     return res;
   }
};

static bool operator==(const AccessAndType& l,const AccessAndType& r){
  if(l.access == r.access && l.inst == r.inst){
    return true;
  }
  
  return false;
}

// ======================================
// Representation

void   Repr(StringBuilder* builder,AddressAccess* access);
String PushRepr(Arena* out,AddressAccess* access);
void   Print(AddressAccess* access);

// ======================================
// Compilation 

AddressAccess* CompileAddressGen(AddressGenDef* def,String content);

// ======================================
// Global getter for compiled address gens

AddressAccess* GetAddressGenOrFail(String name);

// ======================================
// Conversion

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out);
AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out);

// ======================================
// Codegen

String GenerateAddressGenCompilationFunction(AccessAndType access,Arena* out);
String GenerateAddressLoadingFunction(String structName,AddressGenInst type,Arena* out);
String GenerateAddressCompileAndLoadFunction(String structName,AddressAccess* access,AddressGenInst type,Arena* out);
String GenerateAddressPrintFunction(AddressAccess* initial,Arena* out);

// ======================================
// LoopLinearSumTerm handling

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term);
