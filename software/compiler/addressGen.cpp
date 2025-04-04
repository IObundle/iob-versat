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

AddressAccess* ConvertAccessSingleLoop(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  AddressAccess* result = Copy(access,out);

  result->internal = Copy(result->external,out);
  result->external = PushLoopLinearSumEmpty(out);
  
  SymbolicExpression* maxLoopValue = EvaluateMaxLinearSumValue(access->external,temp);
  maxLoopValue = Normalize(SymbolicAdd(maxLoopValue,PushLiteral(temp,1),temp),out);

  result->external = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(temp,1),PushLiteral(temp,0),maxLoopValue,out);
  
#if 0
  result->external->terms[0].term = PushLiteral(out,1);
  result->external->terms[0].loopEnd = maxLoopValue;
#endif
  
  return result;
}

AddressAccess* ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(AddressAccess* access,int biggestLoopIndex,Arena* out){
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

AddressAccess* ConvertAccessTo2ExternalLoops(AddressAccess* access,Arena* out){
  int highestConstantIndex = -1;
  SymbolicExpression* highestConstant = nullptr;
  int highestValue = -1;
  for(int i = 0; i <  access->external->terms.size; i++){
    LoopLinearSumTerm term  =  access->external->terms[i];
    Assert(term.term->type == SymbolicExpressionType_LITERAL);
    if(highestValue < term.term->literal){
      highestValue = term.term->literal;
      highestConstantIndex = i;
      highestConstant = term.term;
    }
  }

  return ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(access,highestConstantIndex,out);
}

// We can remove the entire shift external to internal interface. The only thing that we need is the LinearLoopSum and potentially the array of input variables and this function could do everything.
// Meaning that we would move a lot of the complexity from the code gen function to here.
// In fact, we could even move the structs from the specParser into here and just have this handle everything, from breaking apart the symbolic expression to generating the final address gen.
//  Or at least move the code to this file. Because we are gonna generate different expressions depending on runtime parameters, we probably want to start by having the data in the most friendly format at the start but without already having performed the loop conversions since we want to do them depending on runtime parameters.
// Only thing missing is the interface to generate the final expressions.

AddressAccess* ShiftExternalToInternal(AddressAccess* access,int maxInternalLoops,int maxExternalLoops,Arena* out){
  TEMP_REGION(temp,out);
  
  SimulateResult good = SimulateAddressAccess(access,temp);
  AddressAccess* result = nullptr;

  SymbolicExpression* innerMostExpression = access->external->terms[0].term;

  bool isConstantOne = (innerMostExpression->type == SymbolicExpressionType_LITERAL) && GetLiteralValue(innerMostExpression) == 1;

  // NOTE: We are missing the case where it is better to generate a single loop from a two loop expression because the constant of the outermost loop is not large enough.
  
  if(access->external->terms.size > 2){
    result = ConvertAccessTo2ExternalLoops(access,out);
    //result = ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(access,1,out);
  } else if(access->external->terms.size == 2 && !isConstantOne){
    result = ConvertAccessTo2ExternalLoops(access,out);
  } else if(access->external->terms.size == 1 && !isConstantOne){
    result = ConvertAccessSingleLoop(access,out); // Does this work?
  } else {
    result = Copy(access,out); // One term constant == simplest loop possible and already primed 
  }

  printf("\n\n\n\n\n");
  printf("Before:\n");
  Print(access);
  printf("After:\n");
  Print(result);
  
  SimulateResult simulated = SimulateAddressAccess(result,temp);

  if(!Equal(simulated.values,good.values)){
    printf("Different on access");
    Print(access);
    printf("Different from\n");
    Print(result);
    printf("Values expected:\n");
    Print(good.values);
    printf("\n");
    printf("Values got:\n");
    Print(simulated.values);
    printf("\n");

    printf("Good vs got external\n");
    Print(good.externalValues);
    printf("\n");
    Print(simulated.externalValues);
    printf("\n");

    printf("\n");
    printf("Good vs got internal\n");
    Print(good.internalValues);
    printf("\n");
    Print(simulated.internalValues);
    printf("\n");
      
    DEBUG_BREAK();
    Assert(false);
  } else {
    printf("OK\n");
  }

  Print(result);
  return Copy(result,out);
  
  //return result;
}

