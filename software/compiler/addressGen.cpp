#include "addressGen.hpp"

#include "memory.hpp"
#include "symbolic.hpp"

static String GetDefaultVarNameForLoop(int loopIndex,int amountOfLoops,bool fromInnermost){
  String vars[] = {
    STRING("x"),
    STRING("y"),
    STRING("z"),
    STRING("w"),
    STRING("a"),
    STRING("b"),
    STRING("c"),
    STRING("d"),
    STRING("e")
  };

  if(fromInnermost){
    loopIndex = amountOfLoops - loopIndex;
  }
    
  Assert(loopIndex >= 0);
  if(loopIndex > ARRAY_SIZE(vars)){
    printf("Error, either add more var names or put an hard cap on the amount of loops supported\n");
    Assert(false);
  }

  return vars[loopIndex];
}

Array<int> SimulateSingleAddressAccess(SingleAddressAccess* access,Hashmap<String,int>* extraEnv,Arena* out){
  TEMP_REGION(temp,out);

  auto Recurse = [](auto Recurse,Hashmap<String,int>* env,GrowableArray<int>& values,Array<LoopDefinition>& loops,SymbolicExpression* addr,int loopIndex) -> void{
    if(loopIndex >= loops.size){
      int val = Evaluate(addr,env);
      *values.PushElem() = val;
    } else {
      LoopDefinition def = loops[loopIndex];
      int loopStart = Evaluate(def.start,env);
      int loopEnd = Evaluate(def.end,env);

      Assert(loopStart <= loopEnd);
      
      for(int i = loopStart; i < loopEnd; i++){
        env->Insert(def.loopVariableName,i);

        Recurse(Recurse,env,values,loops,addr,loopIndex + 1);
      }
    }
  };

  Hashmap<String,int>* env = PushHashmap<String,int>(temp,access->loops.size + extraEnv->nodesUsed);

  for(auto p : extraEnv){
    env->Insert(p.first,*p.second);
  }

  GrowableArray<int> builder = StartArray<int>(out);
  Recurse(Recurse,env,builder,access->loops,access->address,0);
  Array<int> values = EndArray(builder);

  return values;
}

struct SimulateResult{
  Array<int> externalValues;
  Array<int> internalValues;
  Array<int> values;
};

SimulateResult SimulateAddressAccess(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  Hashmap<String,int>* extraEnv = PushHashmap<String,int>(temp,access->inputVariableNames.size);
  for(String s : access->inputVariableNames){
    extraEnv->Insert(s,2);
  }
  
  Array<int> externalValues = SimulateSingleAddressAccess(access->external,extraEnv,out);
  Array<int> internalValues = SimulateSingleAddressAccess(access->internal,extraEnv,out);

  Array<int> result = PushArray<int>(out,internalValues.size);
  int inserted = 0;
  for(int index : internalValues){
    result[inserted++] = externalValues[index];
  }
  
  SimulateResult res = {};
  res.externalValues = externalValues;
  res.internalValues = internalValues;
  res.values = result;

  return res;
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

    if(terms[i]->type == SymbolicExpressionType_VARIABLE && CompareString(terms[i]->variable,varName)){
      termIndexThatContainsVariable = i;
      term = PushLiteral(out,1);
      break;
    }
  }

  return {term,termIndexThatContainsVariable};
}

SingleAddressAccess* AddOutermostLoop(SingleAddressAccess* access,String loopVarName,
                                SymbolicExpression* start,SymbolicExpression* end,
                                SymbolicExpression* loopTerm,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);

  Array<LoopDefinition> newLoops = AddElement(result->loops,0,out);

  newLoops[0].loopVariableName = loopVarName;
  newLoops[0].start = start;
  newLoops[0].end = end;

  result->loops = newLoops;
  
  SymbolicExpression* addedTerm = Normalize(SymbolicAdd(access->address,loopTerm,temp),out);
  result->address = addedTerm;

  return result;
}

SingleAddressAccess* DivideLoopEnd(SingleAddressAccess* access,int loopIndex,SymbolicExpression* div,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);

  LoopDefinition& def = result->loops[loopIndex];

  def.end = Normalize(SymbolicDiv(def.end,div,temp),out);

  return result;
}

SingleAddressAccess* SetLoopEnd(SingleAddressAccess* access,int loopIndex,SymbolicExpression* term,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);
  LoopDefinition& def = result->loops[loopIndex];
  def.end = Normalize(term,out);

  return result;
}


SymbolicExpression* LoopMaximumValue(SingleAddressAccess* access,int loopIndex,Arena* out){
  TEMP_REGION(temp,out);
  
  SymbolicExpression* maxVal = SymbolicSub(access->loops[loopIndex].end,PushLiteral(temp,1),temp);

  return Normalize(maxVal,out);
}

// The constant that is associated to the term of a loop.
// TODO: Technically is more than the constant but we will eventually fix this. It's more semantics than anything at this point.
SymbolicExpression* LoopTermConstant(SingleAddressAccess* access,int loopIndex,Arena* out){
  TEMP_REGION(temp,out);

  LoopDefinition def = access->loops[loopIndex];
  String varName = def.loopVariableName;

  Array<SymbolicExpression*> individualSumTerms = access->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,varName,temp);
  
  return Normalize(p.first,out);
}

SingleAddressAccess* RemoveLoopConstant(SingleAddressAccess* access,int loopIndex,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);
  LoopDefinition& def = result->loops[loopIndex];
  String varName = def.loopVariableName;

  Array<SymbolicExpression*> individualSumTerms = access->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,varName,temp);

  Array<SymbolicExpression*> otherTerms = RemoveElement(access->address->sum,p.second,temp);

  if(otherTerms.size > 0){
    SymbolicExpression* fullExpression = SymbolicAdd(PushVariable(out,varName),SymbolicAdd(otherTerms,temp),temp);

    result->address = Normalize(fullExpression,out);
  } else {
    result->address = PushVariable(out,varName);
  }

  return result;
}

SingleAddressAccess* SetLoopTerm(SingleAddressAccess* access,int loopIndex,SymbolicExpression* termWithoutVariable,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);

  LoopDefinition def = access->loops[loopIndex];
  String varName = def.loopVariableName;

  // TODO: HACK, for the cases where the symbolic expression is not a sum but only one term (variable).
  if(access->address->type == SymbolicExpressionType_VARIABLE){
    Array<SymbolicExpression*> terms = PushArray<SymbolicExpression*>(out,1);
    terms[0] = access->address;
    access->address = SymbolicAdd(terms,out);
  }
  
  Array<SymbolicExpression*> individualSumTerms = access->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,varName,temp);

  if(!p.first){
    printf("Error, did not find loop variable inside expression and logic cannot handle this right now\n");
    DEBUG_BREAK();
  }
  
  Array<SymbolicExpression*> otherTerms = RemoveElement(access->address->sum,p.second,temp);

  if(otherTerms.size > 0){
    SymbolicExpression* multRes = SymbolicMult(PushVariable(temp,varName),termWithoutVariable,temp);
    SymbolicExpression* fullExpression = SymbolicAdd(multRes,SymbolicAdd(otherTerms,temp),temp);

    result->address = Normalize(fullExpression,out);

    return result;
  } else {
    SymbolicExpression* multRes = SymbolicMult(PushVariable(temp,varName),termWithoutVariable,temp);
    result->address = Normalize(multRes,out);
    return result;
  }
}
  
SingleAddressAccess* MultLoopTerm(SingleAddressAccess* access,int loopIndex,SymbolicExpression* multTerm,Arena* out){
  TEMP_REGION(temp,out);

  SingleAddressAccess* result = CopySingleAddressAccess(access,out);

  LoopDefinition def = access->loops[loopIndex];
  String varName = def.loopVariableName;

  // TODO: HACK, for the cases where the symbolic expression is not a sum but only one term (variable).
  if(access->address->type == SymbolicExpressionType_VARIABLE){
    Array<SymbolicExpression*> terms = PushArray<SymbolicExpression*>(out,1);
    terms[0] = access->address;
    access->address = SymbolicAdd(terms,out);
  }
  
  Array<SymbolicExpression*> individualSumTerms = access->address->sum;
  Pair<SymbolicExpression*,int> p = GetTermAndIndexAssociatedToVariable(individualSumTerms,varName,temp);

  if(!p.first){
    printf("Error, did not find loop variable inside expression and logic cannot handle this right now\n");
    DEBUG_BREAK();
  }
  
  Array<SymbolicExpression*> otherTerms = RemoveElement(access->address->sum,p.second,temp);

  if(otherTerms.size > 0){
    SymbolicExpression* multRes = SymbolicMult(PushVariable(temp,varName),SymbolicMult(multTerm,p.first,temp),temp);
    SymbolicExpression* fullExpression = SymbolicAdd(multRes,SymbolicAdd(otherTerms,temp),temp);

    result->address = Normalize(fullExpression,out);

    return result;
  } else {
    SymbolicExpression* multRes = SymbolicMult(PushVariable(temp,varName),SymbolicMult(multTerm,p.first,temp),temp);
    result->address = Normalize(multRes,out);
    return result;
  }
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
    // Remove the constant from the mul term.
    Array<SymbolicExpression*> children = PushArray<SymbolicExpression*>(out,individualSumTerms.size);
    for(int i = 0; i <  individualSumTerms.size; i++){
      SymbolicExpression* individual = individualSumTerms[i];

      if(i == termIndexThatContainsVariable){
        children[i] = RemoveMulLiteral(individual,out);
      } else {
        children[i] = individual;
      }
    }
 
    SymbolicExpression* res = SymbolicAdd(children,out);
    res->negative = result->external->address->negative;

    // New loop end is the maximum value generated by the innermost loop
    SymbolicExpression* end = result->external->loops[size-1].end;
    SymbolicExpression* one = PushLiteral(temp,1);
    SymbolicExpression* endMinusOne = SymbolicSub(end,one,temp);

    SymbolicExpression* newLoopEnd = SymbolicAdd(SymbolicMult(term,endMinusOne,temp),one,temp);
    
    result->external = SetLoopEnd(result->external,size-1,newLoopEnd,out);
    result->external = RemoveLoopConstant(result->external,size-1,out);
    
    int newInternalSize = result->internal->loops.size + 1;
    
    // Need to add a new loop and a new term to internal access.
    
    SymbolicExpression* newVar = PushVariable(out,STRING("y"),false);
    SymbolicExpression* newTerm = SymbolicMult(newLoopEnd,newVar,temp);
    SymbolicExpression* loopEnd = Normalize(SymbolicDiv(result->internal->loops[0].end,PushLiteral(temp,2),out),out);
    
    result->internal = AddOutermostLoop(result->internal,STRING("y"),PushLiteral(out,0),loopEnd,newTerm,out);
    result->internal = MultLoopTerm(result->internal,1,term,out);

    // HACK? : Why divide by 2, should be a number that depends on something but hard to figure out with such simple examples.
    //result->internal = DivideLoopEnd(result->internal,1,PushLiteral(temp,2),out);
    result->internal = SetLoopEnd(result->internal,1,PushLiteral(temp,2),out);
  }
  
  return result;
}

// Actually, the second inner loop is "merged" with the first inner loop
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

  // Need to calculate the maximum value that innermost loop can take here.
  // Assuming that inner loop does not contain constant.

  // Max value only taking into account outer loop.
  SymbolicExpression* constantTerm = LoopTermConstant(result->external,size-2,temp);
  SymbolicExpression* maxLoop = LoopMaximumValue(result->external,size-2,temp);

  SymbolicExpression* loopEndValue = result->external->loops[size-2].end;
  
  SymbolicExpression* maxOuterValue = Normalize(SymbolicMult(constantTerm,maxLoop,temp),out);

  SymbolicExpression* maxInnerLoop = LoopMaximumValue(result->external,size-1,temp);
  SymbolicExpression* maxValue = SymbolicAdd(maxOuterValue,maxInnerLoop,temp);
  SymbolicExpression* fixed = SymbolicAdd(maxValue,PushLiteral(out,1),temp);
  
  result->external->loops[size-1].end = Normalize(fixed,out);
  //DEBUG_BREAK();
  
  // Remove outer loop and update the remaining loop.
  //result->external->loops[size-1].end = mult;
  result->external->loops = RemoveElement(result->external->loops,size-2,out);

  Array<SymbolicExpression*> removedTerm = RemoveElement(individualSumTerms,termIndexThatContainsVariable,out);

  SymbolicExpression* ext = SymbolicAdd(removedTerm,out);
  ext->negative = result->external->address->negative;
  
  result->external->address = Normalize(ext,out);

  SymbolicExpression* originalLoopEnd = result->internal->loops[0].end;
  
  //result->internal->loops[0].end = Normalize(SymbolicDiv(result->internal->loops[0].end,PushLiteral(out,2),temp),out);

  result->internal->loops[0].end = Normalize(loopEndValue,out);

  //printf("%d\n",result->internal->loops.size - 2);
  result->internal = SetLoopTerm(result->internal,0,constantTerm,out);
  
  Array<LoopDefinition> internal = AddElement(result->internal->loops,0,out);

  String varName = GetDefaultVarNameForLoop(internal.size - 1,0,false);
  
  internal[0].loopVariableName = varName;
  internal[0].start = PushLiteral(out,0);

  internal[0].end = Normalize(SymbolicDiv(originalLoopEnd,loopEndValue,temp),out);
  
  SymbolicExpression* loopVarVar = PushVariable(out,varName,false);
  SymbolicExpression* newTerm = SymbolicMult(fixed,loopVarVar,out);
  
  result->internal->loops = internal;
  result->internal->address = Normalize(SymbolicAdd(result->internal->address,newTerm,out),out);
  
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

  PrintAccess(access);
  SimulateResult good = SimulateAddressAccess(access,temp);

  AddressAccess* result = RemoveConstantFromInnerLoop(access,out);

  PrintAccess(result);
  
  int loopIndex = 0;
  while(1){
    SimulateResult simulated = SimulateAddressAccess(result,temp);

    if(!Equal(simulated.values,good.values)){
      printf("Different on loopIndex: %d\n",loopIndex);
      PrintAccess(access);
      printf("Different from\n");
      PrintAccess(result);
      DEBUG_BREAK();
      Assert(false);
    } else {
      printf("OK\n");
    }

    if(result->external->loops.size <= maxExternalLoops){
      break;
    }
    
    result = RemoveInnerLoop(result,out);
    loopIndex += 1;
  } 

  PrintAccess(result);
  
  return result;
}

