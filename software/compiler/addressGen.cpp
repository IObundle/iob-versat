#include "addressGen.hpp"

#include "symbolic.hpp"

Array<int> SimulateSingleAddressAccess(SingleAddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  auto Recurse = [](auto Recurse,Hashmap<String,int>* env,GrowableArray<int>& values,Array<LoopDefinition>& loops,SymbolicExpression* addr,int loopIndex) -> void{
    if(loopIndex >= loops.size){
      int val = Evaluate(addr,env);
      *values.PushElem() = val;
    } else {
      LoopDefinition def = loops[loopIndex];
      for(int i = 0; i < def.end->literal; i++){
        env->Insert(def.loopVariableName,i);

        Recurse(Recurse,env,values,loops,addr,loopIndex + 1);
      }
    }
  };

  Hashmap<String,int>* env = PushHashmap<String,int>(temp,access->loops.size);
  GrowableArray<int> builder = StartArray<int>(out);
  Recurse(Recurse,env,builder,access->loops,access->address,0);
  Array<int> values = EndArray(builder);

  return values;
}

void SimulateAddressAccess(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  Array<int> externalValues = SimulateSingleAddressAccess(access->external,temp);
  Array<int> internalValues = SimulateSingleAddressAccess(access->internal,temp);

  printf("External contains %d values\n",externalValues.size);

#if 0
  for(int index : externalValues){
    printf("%d ",index);
  }
#endif
  
  printf("Internal contains %d values\n",internalValues.size);

#if 0
  for(int index : internalValues){
    printf("%d ",index);
  }

  printf("\n");
#endif
  
  for(int index : internalValues){
    printf("%d ",externalValues[index]);
  }

  printf("\n");
}

// SymbolicExpression* is a new expression that contains the leftover terms.
Pair<SymbolicExpression*,int> GetTermAndIndexAssociatedToVariable(Array<SymbolicExpression*> terms,String varName,Arena* out){
  TEMP_REGION(temp,out);

  int termIndexThatContainsVariable = -1;
  SymbolicExpression* term = nullptr;
  for(int i = 0; i < terms.size; i++){
    Opt<SymbolicExpression*> termOpt = GetMultExpressionAssociatedTo(terms[i],varName,temp);
    if(termOpt.has_value()){
      termIndexThatContainsVariable = i;
      term = Normalize(termOpt.value(),out);
      break;
    }
  }

  return {term,termIndexThatContainsVariable};
}

SingleAddressAccess* CopySingleAddressAccess(SingleAddressAccess* in,Arena* out){
  SingleAddressAccess* res = PushStruct<SingleAddressAccess>(out);

  int size = in->loops.size;
  res->loops = PushArray<LoopDefinition>(out,size);
  for(int i = 0; i < size; i++){
    res->loops[i].loopVariableName = PushString(out,in->loops[i].loopVariableName);
    res->loops[i].start = SymbolicDeepCopy(in->loops[i].start,out);
    res->loops[i].end = SymbolicDeepCopy(in->loops[i].end,out);
  }
  
  res->address = SymbolicDeepCopy(in->address,out);

  return res;
}

AddressAccess* CopyAddressAccess(AddressAccess* in,Arena* out){
  AddressAccess* res = PushStruct<AddressAccess>(out);

  res->external = CopySingleAddressAccess(in->external,out);
  res->internal = CopySingleAddressAccess(in->internal,out);
  res->inputVariableNames = CopyArray(in->inputVariableNames,out);

  return res;
}

AddressAccess* RemoveConstantFromInnerLoop(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  AddressAccess* result = CopyAddressAccess(access,out);
  
  int size = result->external->loops.size;
  String innerLoopVariableName = result->external->loops[size-1].loopVariableName;

  Array<SymbolicExpression*> individualSumTerms = result->external->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,innerLoopVariableName,temp);

  SymbolicExpression* term = p.first;
  int termIndexThatContainsVariable = p.second;

  if(!term){
    printf("Called remove constant from inner loop but access does not appear to have a constant on inner loop:\n");
    PrintAccess(result);
    DEBUG_BREAK();
  }

  if(term->type == SymbolicExpressionType_LITERAL){
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,individualSumTerms.size);
    for(int i = 0; i <  individualSumTerms.size; i++){
      SymbolicExpression* individual  =  individualSumTerms[i];

      if(i == termIndexThatContainsVariable){
        children[i] = RemoveMulLiteral(individual,out);
      } else {
        children[i] = individual;
      }
    }

    SymbolicExpression* res = PushStruct<SymbolicExpression>(out);
    res->type = SymbolicExpressionType_SUM;
    res->negative = result->external->address->negative;
    res->sum = children;

    SymbolicExpression* end = result->external->loops[size-1].end;
    SymbolicExpression* offset = Normalize(SymbolicAdd(end,PushLiteral(out,-1),temp),out);

    // TODO: For the current example, we are not deriving the smallest loop possible. In order to derive the smallest we would need to add an internal loop.
    offset = end;
    
    result->external->loops[size-1].end = Normalize(SymbolicAdd(SymbolicMult(term,offset,temp),PushLiteral(out,0),temp),out);
    result->external->address = res;
    
    SymbolicExpression* alteredInner = Normalize(SymbolicMult(result->internal->address,term,temp),out);
    result->internal->address = alteredInner;
  }
  
  return result;
}

AddressAccess* RemoveInnerLoop(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  AddressAccess* result = CopyAddressAccess(access,out);

  int size = result->external->loops.size;

  // Not enough loops.
  if(size - 2 < 0){
    return result;
  }

  LoopDefinition def = result->external->loops[size-2];
  String loopVar = def.loopVariableName;
  
  Array<SymbolicExpression*> individualSumTerms = result->external->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,loopVar,temp);

  SymbolicExpression* term = p.first;
  int termIndexThatContainsVariable = p.second;

  if(!term){
    // NOTE: This techically can happen, but what do we do in this cases I still need to figure out. Later.
    printf("Something wrong happened, we did not find the term for a loop inside the address expression\n");
    DEBUG_BREAK();
  }

  SymbolicExpression* maxLoopValue = def.end;
  SymbolicExpression* mult = Normalize(SymbolicMult(maxLoopValue,term,temp),out);

  //DEBUG_BREAK();
  
  // Remove outer loop and update the remaining loop.
  result->external->loops[size-1].end = mult;
  result->external->loops = RemoveElement(result->external->loops,size-2,out);

  Array<SymbolicExpression*> removedTerm = RemoveElement(individualSumTerms,termIndexThatContainsVariable,out);

  SymbolicExpression* ext = PushStruct<SymbolicExpression>(temp);
  ext->type = SymbolicExpressionType_MUL;
  ext->sum = removedTerm;
  ext->negative = result->external->address->negative;
  
  result->external->address = Normalize(ext,out);
  
  result->internal->loops[0].end = Normalize(SymbolicDiv(result->internal->loops[0].end,PushLiteral(temp,2),temp),out);

  Array<LoopDefinition> internal = AddElement(result->internal->loops,0,out);

  internal[0].loopVariableName = STRING("y");
  internal[0].start = PushLiteral(out,0);
  internal[0].end = PushLiteral(out,2);

  SymbolicExpression* loopVarVar = PushVariable(out,STRING("y"),false);
  SymbolicExpression* newTerm = SymbolicMult(loopVarVar,term,out);
  
  result->internal->loops = internal;

  result->internal->address = Normalize(SymbolicAdd(result->internal->address,newTerm,temp),out);
  
  return result;
}

void PrintAccess(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  printf("\n");
  {
    printf("External:\n");
    for(LoopDefinition def : access->external->loops){
      String start = PushRepresentation(def.start,temp);
      String end = PushRepresentation(def.end,temp);

      printf("for %.*s %.*s..%.*s\n",UNPACK_SS(def.loopVariableName),UNPACK_SS(start),UNPACK_SS(end));
    }

    String addrFunc = PushRepresentation(access->external->address,temp);
  
    printf("addr = %.*s;\n\n",UNPACK_SS(addrFunc));
  }

  {
    printf("Internal:\n");
    for(LoopDefinition def : access->internal->loops){
      String start = PushRepresentation(def.start,temp);
      String end = PushRepresentation(def.end,temp);

      printf("for %.*s %.*s..%.*s\n",UNPACK_SS(def.loopVariableName),UNPACK_SS(start),UNPACK_SS(end));
    }
  
    String addrFunc = PushRepresentation(access->internal->address,temp);
  
    printf("addr = %.*s;\n\n",UNPACK_SS(addrFunc));

  }
}

AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxLoopsTotal,int maxExternalLoops,Arena* out){
  TEMP_REGION(temp,out);

  printf("Before:\n\n");
  PrintAccess(access);

  SimulateAddressAccess(access);
  
  AddressAccess* first = RemoveConstantFromInnerLoop(access,out);
  printf("First:\n\n");
  PrintAccess(first);
  
  // The problem is that we are not generating the correct loops for the internal access.
  // If we want to minimize the external loop, we need to add one loop to the internal.
  // We cannot keep the 1 loop internal unless we read more data from external.
  SimulateAddressAccess(first);

  AddressAccess* second = RemoveInnerLoop(first,out);
  printf("Second:\n\n");
  PrintAccess(second);

  SimulateAddressAccess(second);

  return first;
  return second;
}
