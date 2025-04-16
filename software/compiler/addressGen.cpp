#include "addressGen.hpp"

#include "memory.hpp"
#include "symbolic.hpp"
#include "utilsCore.hpp"

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

Array<int> SimulateSingleAddressAccess(LoopLinearSum* access,Hashmap<String,int>* extraEnv,Arena* out){
  TEMP_REGION(temp,out);

  auto Recurse = [](auto Recurse,Hashmap<String,int>* env,GrowableArray<int>& values,LoopLinearSum* loops,SymbolicExpression* addr,int loopIndex) -> void{
    if(loopIndex < 0){
      int val = Evaluate(addr,env);
      *values.PushElem() = val;
    } else {
      LoopLinearSumTerm def = loops->terms[loopIndex];
      int loopStart = Evaluate(def.loopStart,env);
      int loopEnd = Evaluate(def.loopEnd,env);

      Assert(loopStart <= loopEnd);
      
      for(int i = loopStart; i < loopEnd; i++){
        env->Insert(def.var,i);

        Recurse(Recurse,env,values,loops,addr,loopIndex - 1);
      }
    }
  };

  Hashmap<String,int>* env = PushHashmap<String,int>(temp,access->terms.size + extraEnv->nodesUsed);

  for(auto p : extraEnv){
    env->Insert(p.first,*p.second);
  }
  
  GrowableArray<int> builder = StartArray<int>(out);
  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(access,temp);
  
  Recurse(Recurse,env,builder,access,fullExpression,access->terms.size - 1);
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
    extraEnv->Insert(s,5);
  }
  
  Array<int> externalValues = SimulateSingleAddressAccess(access->external,extraEnv,out);
  Array<int> internalValues = SimulateSingleAddressAccess(access->internal,extraEnv,out);
  
  Array<int> result = PushArray<int>(out,internalValues.size);
  int inserted = 0;
  for(int index : internalValues){
    if(index < externalValues.size){
      result[inserted++] = externalValues[index];
    }
  }

  SimulateResult res = {};
  res.externalValues = externalValues;
  res.internalValues = internalValues;
  res.values = result;

  return res;
}

AddressAccess* Copy(AddressAccess* in,Arena* out){
  AddressAccess* res = PushStruct<AddressAccess>(out);

  res->external = Copy(in->external,out);
  res->internal = Copy(in->internal,out);
  res->inputVariableNames = CopyArray(in->inputVariableNames,out);

  return res;
}

SymbolicExpression* LoopMaximumValue(LoopLinearSumTerm term,Arena* out){
  TEMP_REGION(temp,out);
  
  SymbolicExpression* maxVal = SymbolicSub(term.loopEnd,PushLiteral(temp,1),temp);

  return Normalize(maxVal,out);
}

void Repr(StringBuilder* builder,AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  builder->PushString("External:\n");
  Repr(builder,access->external);
  builder->PushString("\n\nInternal:\n");
  Repr(builder,access->internal);
  builder->PushString("\n");
}

void Print(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  printf("External:\n");
  Print(access->external);
  printf("\nInternal:\n");
  Print(access->internal);
  printf("\n");
}

SymbolicExpression* EvaluateMaxLinearSumValue(LoopLinearSum* sum,Arena* out){
  TEMP_REGION(temp,out);
  
  SymbolicExpression* val = SymbolicDeepCopy(sum->freeTerm,temp);

  for(int i = 0; i <  sum->terms.size; i++){
    LoopLinearSumTerm term  =  sum->terms[i];

    SymbolicExpression* maxLoopValue = LoopMaximumValue(term,temp);
    SymbolicExpression* maxTermValue = SymbolicMult(term.term,maxLoopValue,temp);
    
    val = SymbolicAdd(val,maxTermValue,temp);
  }

  val = Normalize(val,out);

  return val;
}

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  SymbolicExpression* freeTerm = access->external->freeTerm;
  
  AddressAccess* result = Copy(access,out);
  result->external->freeTerm = PushLiteral(temp,0); // Pretend that the free term does not exist
  
  result->internal = Copy(result->external,out);
  result->external = PushLoopLinearSumEmpty(out);
  
  SymbolicExpression* maxLoopValue = EvaluateMaxLinearSumValue(access->external,temp);
  maxLoopValue = Normalize(SymbolicAdd(maxLoopValue,PushLiteral(temp,1),temp),out);

  result->external = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(temp,1),PushLiteral(temp,0),maxLoopValue,out);
  result->external->freeTerm = SymbolicDeepCopy(freeTerm,out);
  
  return result;
}

AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out){
  AddressAccess* result = Copy(access,out);

  LoopLinearSum* internal = result->internal;
  LoopLinearSum* external = result->external;
  
  // We pretent that the free term does not exist and then shift the initial address of the final external expression by the free term.
  SymbolicExpression* freeTerm = external->freeTerm;
  external->freeTerm = PushLiteral(out,0);
  
  int highestConstantIndex = biggestLoopIndex;
  SymbolicExpression* highestConstant = access->external->terms[biggestLoopIndex].term;

  LoopLinearSum temp = {};
  temp.terms = RemoveElement(external->terms,highestConstantIndex,out);
  temp.freeTerm = external->freeTerm;

  SymbolicExpression* val = EvaluateMaxLinearSumValue(&temp,out);
  SymbolicExpression* maxLoopValueExpr = Normalize(SymbolicAdd(val,PushLiteral(out,1),out),out);
  
  result->internal = Copy(external,out);
  result->internal->terms[highestConstantIndex].term = maxLoopValueExpr; //PushLiteral(out,maxLoopValue);
  
  LoopLinearSum* innermostExternal = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(out,1),PushLiteral(out,0),maxLoopValueExpr,out);

  // NOTE: Not sure about the end of this loop. We carry it directly but need to do further tests to make sure that this works fine.
  LoopLinearSum* outerMostExternal = PushLoopLinearSumSimpleVar(STRING("y"),highestConstant,PushLiteral(out,0),external->terms[highestConstantIndex].loopEnd,out);
  
  result->external = AddLoopLinearSum(innermostExternal,outerMostExternal,out);
  result->external->freeTerm = SymbolicDeepCopy(freeTerm,out);
  
  return result;
}

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term){
  return term->term;
}

static ExternalMemoryAccess CompileExternalMemoryAccess(LoopLinearSum* access,Arena* out){
  TEMP_REGION(temp,out);

  int size = access->terms.size;

  Assert(size <= 2);
  // Maybe add some assert to check if the innermost loop contains a constant of 1.
  //Assert(Constant(access) == 1);

  ExternalMemoryAccess result = {};

  LoopLinearSumTerm inner = access->terms[0];
  LoopLinearSumTerm outer = access->terms[access->terms.size - 1];

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(access,temp);
  
  auto GetLoopSize = [](LoopLinearSumTerm def,Arena* out,bool removeOne = false) -> SymbolicExpression*{
    TEMP_REGION(temp,out);

    SymbolicExpression* diff = SymbolicSub(def.loopEnd,def.loopStart,temp);
    
    if(removeOne){
      diff = SymbolicSub(diff,PushLiteral(temp,1),temp);
    }

    SymbolicExpression* final = Normalize(diff,out);
    return final;
  };
  
  auto GetLoopSizeRepr = [GetLoopSize](LoopLinearSumTerm def,Arena* out,bool removeOne = false){
    TEMP_REGION(temp,out);
    return PushRepresentation(GetLoopSize(def,temp,removeOne),out);
  };

  if(size == 1){
    result.length = GetLoopSizeRepr(inner,out);
    result.totalTransferSize = result.length;
    result.amountMinusOne = STRING("0");
    result.addrShift = STRING("0");
  } else {
    // TODO: A bit repetitive, could use some cleanup
    SymbolicExpression* outerLoopSize = GetLoopSize(outer,temp);
    
    result.length = GetLoopSizeRepr(inner,out);
    result.amountMinusOne = GetLoopSizeRepr(outer,out,true);

    // NOTE: Derivation inside a LoopLinearSum is just getting the values for the term.
    SymbolicExpression* derived = Normalize(Derivate(fullExpression,outer.var,temp),temp);

    result.addrShift = PushRepresentation(derived,out);
    
    SymbolicExpression* all = Normalize(SymbolicMult(GetLoopSize(inner,temp),outerLoopSize,temp),temp);

    result.totalTransferSize = PushRepresentation(all,out);
  }
  
  return result;
}

static Array<InternalMemoryAccess> CompileInternalAccess(LoopLinearSum* access,Arena* out){
  TEMP_REGION(temp,out);
  
  auto GetLoopSize = [](LoopLinearSumTerm def,Arena* out,bool removeOne = false) -> SymbolicExpression*{
    TEMP_REGION(temp,out);

    SymbolicExpression* diff = SymbolicSub(def.loopEnd,def.loopStart,temp);
    
    if(removeOne){
      diff = SymbolicSub(diff,PushLiteral(temp,1),temp);
    }

    SymbolicExpression* final = Normalize(diff,temp);
    return final;
  };
  
  auto GetLoopSizeRepr = [GetLoopSize](LoopLinearSumTerm def,Arena* out,bool removeOne = false){
    TEMP_REGION(temp,out);
    return PushRepresentation(GetLoopSize(def,temp,removeOne),out);
  };
  
  auto GenerateLoopExpressionPairSymbolic = [GetLoopSizeRepr](Array<LoopLinearSumTerm> loops,
                                                              SymbolicExpression* expr,bool ext,Arena* out) -> Array<InternalMemoryAccess>{
    TEMP_REGION(temp,out);

    int loopSize = (loops.size + 1) / 2;
    Array<InternalMemoryAccess> result = PushArray<InternalMemoryAccess>(out,loopSize);

    // TODO: Need to normalize and push the entire expression into a division.
    //       So we can then always extract a duty
    SymbolicExpression* duty = nullptr;
    if(expr->type == SymbolicExpressionType_DIV){
      duty = expr->bottom;
      expr = expr->top;
    }
    
    for(int i = 0; i < loopSize; i++){
      LoopLinearSumTerm l0 = loops[i*2];

      InternalMemoryAccess& res = result[i];
      res = {};

      String loopSizeRepr = GetLoopSizeRepr(l0,out);
      
      SymbolicExpression* derived = Derivate(expr,l0.var,temp);
      SymbolicExpression* firstDerived = Normalize(derived,temp);
      SymbolicExpression* periodSym = ParseSymbolicExpression(loopSizeRepr,temp);
        
      res.periodExpression = loopSizeRepr;
      res.incrementExpression = PushRepresentation(firstDerived,out);

      if(i == 0){
        if(duty){
          // For an expression where increment is something like 1/4.
          // That means that we want 4 * period with a duty of 1.
          // Basically have to reorder the division.
          SymbolicExpression* dutyExpression = SymbolicDiv(periodSym,duty,temp);

          result[0].dutyExpression = PushRepresentation(dutyExpression,out);
        } else {
          result[0].dutyExpression = PushString(out,PushRepresentation(loops[0].loopEnd,temp)); // For now, do not care too much about duty. Use a full duty
        }
      }
      
      String firstEnd = PushRepresentation(l0.loopEnd,out);
      SymbolicExpression* firstEndSym = ParseSymbolicExpression(firstEnd,temp);

      res.shiftWithoutRemovingIncrement = STRING("0"); // By default
      if(i * 2 + 1 < loops.size){
        LoopLinearSumTerm l1 = loops[i*2 + 1];

        res.iterationExpression = GetLoopSizeRepr(l1,out);
        SymbolicExpression* derived = Normalize(Derivate(expr,l1.var,temp),temp);

        res.shiftWithoutRemovingIncrement = PushRepresentation(derived,out);
        
        // We handle shifts very easily. We just remove the effects of all the previous period increments and then apply the shift.
        // That way we just have to calculate the derivative in relation to the shift, instead of calculating the change from a period term to a iteration term.
        // We need to subtract 1 because the period increment is only applied (period - 1) times.
        SymbolicExpression* templateSym = ParseSymbolicExpression(STRING("-(firstIncrement * (firstEnd - 1)) + term"),temp);

        SymbolicExpression* replaced = SymbolicReplace(templateSym,STRING("firstIncrement"),firstDerived,temp);
        replaced = SymbolicReplace(replaced,STRING("firstEnd"),firstEndSym,temp);
        replaced = SymbolicReplace(replaced,STRING("term"),derived,temp);
        replaced = Normalize(replaced,temp,false);
        
        res.shiftExpression = PushRepresentation(replaced,out);
      } else {
        if(ext){
          res.iterationExpression = STRING("1");
        } else {
          res.iterationExpression = STRING("0");
        }
        res.shiftExpression = STRING("0");
      }
    }

    return result;
  };

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(access,temp);
  
  Array<InternalMemoryAccess> res = GenerateLoopExpressionPairSymbolic(access->terms,fullExpression,false,out);
  
  return res;
}

AddressVParameters InstantiateAccess(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);
  ExternalMemoryAccess external = CompileExternalMemoryAccess(access->external,temp);
  Array<InternalMemoryAccess> internal = CompileInternalAccess(access->internal,temp);

  AddressVParameters res = {};
  
  SymbolicExpression* freeTerm = access->external->freeTerm;
    
  if(IsZero(freeTerm)){
    freeTerm = access->internal->freeTerm;
  } else {
    Assert(IsZero(access->internal->freeTerm)); // NOTE: I do not think it is possible for both external and internal to have free terms.
  }

  // NOTE: We push the start term to the ext pointer in order to save memory inside the unit. This is being done in a  kinda hacky way, but nothing major.
  String ext_addr = STRING("ext"); // TODO: Should be a parameter or something, not randomly hardcoded here
  if(!IsZero(freeTerm)){
    String repr = PushRepresentation(freeTerm,temp);

    ext_addr = PushString(out,"(((float*) ext) + (%.*s))",UNPACK_SS(repr));
  }

  res.start = STRING("0");
  
  res.ext_addr = ext_addr;
  res.length = PushString(out,"(%.*s) * sizeof(float)",UNPACK_SS(external.length));
  res.amount_minus_one = PushString(out,external.amountMinusOne);
  res.addr_shift = PushString(out,"(%.*s) * sizeof(float)",UNPACK_SS(external.addrShift));

  res.enabled = STRING("1");
  res.pingPong = STRING("1");

  {
    InternalMemoryAccess l = internal[0]; 
    res.per = PushString(out,l.periodExpression);
    res.incr = PushString(out,l.incrementExpression);
    res.duty = PushString(out,l.dutyExpression);
    res.iter = PushString(out,l.iterationExpression);
    res.shift = PushString(out,l.shiftExpression);
  }
    
  if(internal.size > 1){
    InternalMemoryAccess l = internal[1]; 
    res.per2 = PushString(out,l.periodExpression);
    res.incr2 = PushString(out,l.incrementExpression);
    res.iter2 = PushString(out,l.iterationExpression);
    res.shift2 = PushString(out,l.shiftExpression);
  } else {
    res.per2 = STRING("0");
    res.incr2 = STRING("0");
    res.iter2 = STRING("0");
    res.shift2 = STRING("0");
  }

  return res;

  //return InstantiateAccess(external,internal,out);
}
