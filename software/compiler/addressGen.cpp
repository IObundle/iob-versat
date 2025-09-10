#include "addressGen.hpp"

#include "embeddedData.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versatSpecificationParser.hpp"
#include "CEmitter.hpp"

Pool<AddressAccess> globalAddressGen;

struct {
  int port;
  bool dir;
  String name;
} memInfo[] = {
  {0,false,"Output_0"},
  {0,true,"Input_0"},
  {1,false,"Output_1"},
  {1,true,"Input_1"}
};

AddressAccess* GetAddressGenOrFail(String name){
  for(AddressAccess* gen : globalAddressGen){
    if(CompareString(gen->name,name)){
      return gen;
    }
  }

  Assert(false);
  return nullptr;
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
  TEMP_REGION(temp,builder->arena);

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
  if(access->dutyDivExpr){
    printf("Duty:\n");
    Print(access->dutyDivExpr);
    printf("\n");
  }
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
  
  SymbolicExpression* maxLoopValue = EvaluateMaxLinearSumValue(result->internal,temp);
  maxLoopValue = Normalize(SymbolicAdd(maxLoopValue,PushLiteral(temp,1),temp),out);

  result->external = PushLoopLinearSumSimpleVar("x",PushLiteral(temp,1),PushLiteral(temp,0),maxLoopValue,out);
  result->external->freeTerm = SymbolicDeepCopy(freeTerm,out);
  result->dutyDivExpr = SymbolicDeepCopy(access->dutyDivExpr,out);
  
  return result;
}

AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out){
  AddressAccess* result = Copy(access,out);

  LoopLinearSum* external = result->external;
  
  // We pretent that the free term does not exist and then shift the initial address of the final external expression by the free term.
  SymbolicExpression* freeTerm = external->freeTerm;
  external->freeTerm = PushLiteral(out,0);
  
  int highestConstantIndex = biggestLoopIndex;
  SymbolicExpression* highestConstant = access->external->terms[biggestLoopIndex].term;

  LoopLinearSum temp = {};
  temp.terms = RemoveElement(external->terms,highestConstantIndex,out);
  temp.freeTerm = external->freeTerm;

  // We perform a loop "evaluation" here. 
  SymbolicExpression* val = EvaluateMaxLinearSumValue(&temp,out);
  SymbolicExpression* maxLoopValueExpr = Normalize(SymbolicAdd(val,PushLiteral(out,1),out),out);

  Array<SymbolicExpression*> terms = PushArray<SymbolicExpression*>(out,2);
  terms[0] = maxLoopValueExpr;
  terms[1] = PushVariable(out,"VERSAT_DIFF_W");
  maxLoopValueExpr = SymbolicFunc("ALIGN",terms,out);
  
  result->internal = Copy(external,out);
  result->internal->terms[highestConstantIndex].term = maxLoopValueExpr; //PushLiteral(out,maxLoopValue);
  
  LoopLinearSum* innermostExternal = PushLoopLinearSumSimpleVar("x",PushLiteral(out,1),PushLiteral(out,0),maxLoopValueExpr,out);

  // NOTE: Not sure about the end of this loop. We carry it directly but need to do further tests to make sure that this works fine.
  LoopLinearSum* outerMostExternal = PushLoopLinearSumSimpleVar("y",highestConstant,PushLiteral(out,0),external->terms[highestConstantIndex].loopEnd,out);
  
  result->external = AddLoopLinearSum(innermostExternal,outerMostExternal,out);
  result->external->freeTerm = SymbolicDeepCopy(freeTerm,out);
  result->dutyDivExpr = SymbolicDeepCopy(access->dutyDivExpr,out);
  
  return result;
}

SymbolicExpression* GetLoopHighestDecider(LoopLinearSumTerm* term){
  return term->term;
}

static SymbolicExpression* GetLoopSize(LoopLinearSumTerm def,Arena* out,bool removeOne = false){
  TEMP_REGION(temp,out);

  SymbolicExpression* diff = SymbolicSub(def.loopEnd,def.loopStart,temp);
    
  if(removeOne){
    diff = SymbolicSub(diff,PushLiteral(temp,1),temp);
  }
    
  SymbolicExpression* final = Normalize(diff,out);
  return final;
};

String GetLoopSizeRepr(LoopLinearSumTerm def,Arena* out,bool removeOne = false){
  TEMP_REGION(temp,out);
  return PushRepresentation(GetLoopSize(def,temp,removeOne),out);
};

static ExternalMemoryAccess CompileExternalMemoryAccess(LoopLinearSum* access,SymbolicExpression* dutyExpr,Arena* out){
  TEMP_REGION(temp,out);

  int size = access->terms.size;

  Assert(size <= 2);

  ExternalMemoryAccess result = {};

  LoopLinearSumTerm inner = access->terms[0];
  LoopLinearSumTerm outer = access->terms[access->terms.size - 1];

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(access,temp);
  
  result.length = GetLoopSizeRepr(inner,out);
  
  if(size == 1){
    result.totalTransferSize = result.length;
    result.amountMinusOne = "0";
    result.addrShift = "0";
  } else {
    SymbolicExpression* derived = Normalize(Derivate(fullExpression,outer.var,temp),temp);
    result.addrShift = PushRepresentation(derived,out);
    
    SymbolicExpression* outerLoopSize = GetLoopSize(outer,temp);
    SymbolicExpression* all = Normalize(SymbolicMult(GetLoopSize(inner,temp),outerLoopSize,temp),temp);

    result.totalTransferSize = PushRepresentation(all,out);
    result.amountMinusOne = GetLoopSizeRepr(outer,out,true);
  }
  
  return result;
}

static CompiledAccess CompileAccess(LoopLinearSum* access,SymbolicExpression* dutyDiv,Arena* out){
  TEMP_REGION(temp,out);
  
  auto GenerateLoopExpressionPairSymbolic = [dutyDiv](Array<LoopLinearSumTerm> loops,
                                                      SymbolicExpression* expr,Arena* out) -> CompiledAccess{
    TEMP_REGION(temp,out);

    // TODO: Because we are adding an extra loop, there is a possibility of failure since the unit might not support enough loops to implement this. For the SingleLoop VS DoubleLoop the problem does not occur because we know that Singleloop is possible and DoubleLoop is easier on the address gen of the internal loop which is the limitting factor.
    //       Overall, we need to push this stuff upwards, so that we can simplify the code. It is easier to check and handle address gens that are to big before starting to emit stuff and writing to files.
    // NOTE: The only thing that we need to do is to add +1 to the amount of loops if a unit contains a duty expression. The failure is exactly the same, "address gen contains more loops than the unit is capable of handling"
    SymbolicExpression* duty = dutyDiv;
    if(duty){
      // In order to solve duty, we want to use two loops for the innermost loop. The first loop will have a period equal to the duty division expression and a duty of 1.
      // The second loop will have the expression of the original loop.
      Array<LoopLinearSumTerm> newLoops = PushArray<LoopLinearSumTerm>(temp,loops.size + 1);

      for(int i = 0; i < loops.size; i++){
        newLoops[i+1] = loops[i];
      }

      newLoops[0].var = "NONE";
      newLoops[0].term = PushLiteral(temp,1);
      newLoops[0].loopStart = PushLiteral(temp,0);
      newLoops[0].loopEnd = duty;

      //newLoops[1].loopEnd = SymbolicDiv(newLoops[1].loopEnd,duty,temp);
      
      loops = newLoops;
    }
    
    int loopSize = (loops.size + 1) / 2;
    Array<InternalMemoryAccess> result = PushArray<InternalMemoryAccess>(out,loopSize);
    
    for(int i = 0; i < loopSize; i++){
      LoopLinearSumTerm l0 = loops[i*2];

      InternalMemoryAccess& res = result[i];
      res = {};

      SymbolicExpression* derived = Derivate(expr,l0.var,temp);
      SymbolicExpression* firstDerived = Normalize(derived,temp);
        
      res.periodExpression = GetLoopSizeRepr(l0,out);
      res.incrementExpression = PushRepresentation(firstDerived,out);

      if(i == 0){
        if(duty){
          result[0].dutyExpression = "1";
        } else {
          result[0].dutyExpression = PushString(out,PushRepresentation(loops[0].loopEnd,temp)); // For now, do not care too much about duty. Use a full duty
        }
      }
      
      String firstEnd = PushRepresentation(l0.loopEnd,out);
      SymbolicExpression* firstEndSym = ParseSymbolicExpression(firstEnd,temp);

      res.shiftWithoutRemovingIncrement = "0"; // By default
      if(i * 2 + 1 < loops.size){
        LoopLinearSumTerm l1 = loops[i*2 + 1];

        res.iterationExpression = GetLoopSizeRepr(l1,out);
        SymbolicExpression* derived = Normalize(Derivate(expr,l1.var,temp),temp);

        res.shiftWithoutRemovingIncrement = PushRepresentation(derived,out);
        
        // We handle shifts very easily. We just remove the effects of all the previous period increments and then apply the shift.
        // That way we just have to calculate the derivative in relation to the shift, instead of calculating the change from a period term to a iteration term.
        // We need to subtract 1 because the period increment is only applied (period - 1) times.
        SymbolicExpression* templateSym = ParseSymbolicExpression("-(firstIncrement * (firstEnd - 1)) + term",temp);

        SymbolicExpression* replaced = SymbolicReplace(templateSym,"firstIncrement",firstDerived,temp);
        replaced = SymbolicReplace(replaced,"firstEnd",firstEndSym,temp);
        replaced = SymbolicReplace(replaced,"term",derived,temp);
        replaced = Normalize(replaced,temp,false);
        
        res.shiftExpression = PushRepresentation(replaced,out);
      } else {
        res.iterationExpression = "0";
        res.shiftExpression = "0";
      }
    }

    CompiledAccess com = {};
    com.internalAccess = result;

    if(duty){
      com.dutyDivExpression = PushRepresentation(duty,out);
    }
    
    return com;
  };

  SymbolicExpression* fullExpression = TransformIntoSymbolicExpression(access,temp);
  fullExpression = Normalize(fullExpression,temp);
  
  CompiledAccess res = GenerateLoopExpressionPairSymbolic(access->terms,fullExpression,out);
  
  return res;
}

static Array<Pair<String,String>> InstantiateGen(AddressAccess* access,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);
  Array<InternalMemoryAccess> compiled = CompileAccess(access->external,access->dutyDivExpr,temp).internalAccess;
  SymbolicExpression* freeTerm = access->external->freeTerm;

  ArenaList<Pair<String,String>>* list = PushArenaList<Pair<String,String>>(temp);
  String start = PushRepresentation(freeTerm,temp);

  *list->PushElem() = {"start",start};

  {
    InternalMemoryAccess l = compiled[0]; 

    *list->PushElem() = {"duty",PushString(out,l.dutyExpression)};
    *list->PushElem() = {"per",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift",PushString(out,l.shiftExpression)};
  }
    
  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(compiled.size > 1){
    InternalMemoryAccess l = compiled[1]; 
    *list->PushElem() = {"per2",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr2",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter2",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift2",PushString(out,l.shiftExpression)};
  } else if(maxLoops > 1){
    *list->PushElem() = {"per2","0"};
    *list->PushElem() = {"incr2","0"};
    *list->PushElem() = {"iter2","0"};
    *list->PushElem() = {"shift2","0"};
  }

  if(compiled.size > 2){
    InternalMemoryAccess l = compiled[2]; 
    *list->PushElem() = {"per3",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr3",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter3",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift3",PushString(out,l.shiftExpression)};
  } else if(maxLoops > 2){
    *list->PushElem() = {"per3","0"};
    *list->PushElem() = {"incr3","0"};
    *list->PushElem() = {"iter3","0"};
    *list->PushElem() = {"shift3","0"};
  }
  
  if(compiled.size > maxLoops){
    // TODO: Proper error reporting requires us to lift the data up.
    printf("[ERROR] Address gen contains %d loops but unit can only handle a maximum of 3\n",compiled.size);
    exit(-1);
  }
  
  return PushArrayFromList(out,list);
}

static Array<Pair<String,String>> InstantiateRead(AddressAccess* access,int highestExternalLoop,bool doubleLoop,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);

  ExternalMemoryAccess external = CompileExternalMemoryAccess(access->external,access->dutyDivExpr,temp);
  CompiledAccess compiled = CompileAccess(access->internal,access->dutyDivExpr,temp);

  auto internal = compiled.internalAccess;
  
  SymbolicExpression* freeTerm = access->external->freeTerm;
    
  if(IsZero(freeTerm)){
    freeTerm = access->internal->freeTerm;
  } else {
    Assert(IsZero(access->internal->freeTerm)); // NOTE: I do not think it is possible for both external and internal to have free terms.
  }
  
  // NOTE: We push the start term to the ext pointer in order to save memory inside the unit. This is being done in a  kinda hacky way, but nothing major.
  String ext_addr = "ext"; // TODO: Should be a parameter or something, not randomly hardcoded here
  if(!IsZero(freeTerm)){
    String repr = PushRepresentation(freeTerm,temp);

    ext_addr = PushString(out,"(((float*) ext) + (%.*s))",UN(repr));
  }
  
  // TODO: No need for a list, we already know all the memory that we are gonna need
  ArenaList<Pair<String,String>>* list = PushArenaList<Pair<String,String>>(temp);

  if(!Empty(compiled.dutyDivExpression)){
    *list->PushElem() = {"extra_delay",PushString(out,"(%.*s) - 1",UN(compiled.dutyDivExpression))};
  }
  
  *list->PushElem() = {"start","0"};
  *list->PushElem() = {"ext_addr",ext_addr};
  *list->PushElem() = {"length",PushString(out,"(%.*s) * sizeof(float)",UN(external.length))};

  // NOTE: The reason we need to align is because the increase in AXI_DATA_W forces data to be aligned inside the VUnits memories. The single loop does not care because we only need to access values individually, but the double loop cannot function because it assumes that the data is read and stored in a linear matter while in reality the data is stored in multiples of (AXI_DATA_W/DATA_W). 

  *list->PushElem() = {"amount_minus_one",PushString(out,external.amountMinusOne)};
  *list->PushElem() = {"addr_shift",PushString(out,"(%.*s) * sizeof(float)",UN(external.addrShift))};

  *list->PushElem() = {"enabled","1"};
  *list->PushElem() = {"pingPong","1"};

  {
    InternalMemoryAccess l = internal[0]; 

    *list->PushElem() = {"duty",PushString(out,l.dutyExpression)};
    *list->PushElem() = {"per",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift",PushString(out,l.shiftExpression)};
  }
    
  if(internal.size > 1){
    InternalMemoryAccess l = internal[1]; 
    *list->PushElem() = {"per2",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr2",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter2",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift2",PushString(out,l.shiftExpression)};
  } else if(maxLoops > 1) {
    *list->PushElem() = {"per2","0"};
    *list->PushElem() = {"incr2","0"};
    *list->PushElem() = {"iter2","0"};
    *list->PushElem() = {"shift2","0"};
  }

  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(internal.size > 2){
    InternalMemoryAccess l = internal[2]; 
    *list->PushElem() = {"per3",PushString(out,l.periodExpression)};
    *list->PushElem() = {"incr3",PushString(out,l.incrementExpression)};
    *list->PushElem() = {"iter3",PushString(out,l.iterationExpression)};
    *list->PushElem() = {"shift3",PushString(out,l.shiftExpression)};
  } else if(maxLoops > 2){
    *list->PushElem() = {"per3","0"};
    *list->PushElem() = {"incr3","0"};
    *list->PushElem() = {"iter3","0"};
    *list->PushElem() = {"shift3","0"};
  }

  if(internal.size > maxLoops){
    // TODO: Proper error reporting requires us to lift the data up.
    printf("[ERROR] Address gen contains more loops than the unit is capable of handling\n");
    exit(-1);
  }
  
  return PushArrayFromList(out,list);
}

static Array<Pair<String,String>> InstantiateMem(AddressAccess* access,int port,bool input,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);
  Array<InternalMemoryAccess> compiled = CompileAccess(access->external,access->dutyDivExpr,temp).internalAccess;
  SymbolicExpression* freeTerm = access->external->freeTerm;

  ArenaList<Pair<String,String>>* list = PushArenaList<Pair<String,String>>(temp);
  String start = PushRepresentation(freeTerm,temp);

  if(port == 0){
    *list->PushElem() = {"startA",start};
  } else {
    *list->PushElem() = {"startB",start};
  }
    
  if(input){
    if(port == 0){
      *list->PushElem() = {"in0_wr","1"};
    } else {
      *list->PushElem() = {"in1_wr","1"};
    }
  } else {
    if(port == 0){
      *list->PushElem() = {"in0_wr","0"};
    } else {
      *list->PushElem() = {"in1_wr","0"};
    }
  }
  
  {
    InternalMemoryAccess l = compiled[0]; 

    if(port == 0){
      *list->PushElem() = {"dutyA",PushString(out,l.dutyExpression)};
      *list->PushElem() = {"perA",PushString(out,l.periodExpression)};
      *list->PushElem() = {"incrA",PushString(out,l.incrementExpression)};
      *list->PushElem() = {"iterA",PushString(out,l.iterationExpression)};
      *list->PushElem() = {"shiftA",PushString(out,l.shiftExpression)};
    } else {
      *list->PushElem() = {"dutyB",PushString(out,l.dutyExpression)};
      *list->PushElem() = {"perB",PushString(out,l.periodExpression)};
      *list->PushElem() = {"incrB",PushString(out,l.incrementExpression)};
      *list->PushElem() = {"iterB",PushString(out,l.iterationExpression)};
      *list->PushElem() = {"shiftB",PushString(out,l.shiftExpression)};
    }
  }
    
  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(compiled.size > 1){
    InternalMemoryAccess l = compiled[1]; 

    if(port == 0){
      *list->PushElem() = {"per2A",PushString(out,l.periodExpression)};
      *list->PushElem() = {"incr2A",PushString(out,l.incrementExpression)};
      *list->PushElem() = {"iter2A",PushString(out,l.iterationExpression)};
      *list->PushElem() = {"shift2A",PushString(out,l.shiftExpression)};
    } else {
      *list->PushElem() = {"per2B",PushString(out,l.periodExpression)};
      *list->PushElem() = {"incr2B",PushString(out,l.incrementExpression)};
      *list->PushElem() = {"iter2B",PushString(out,l.iterationExpression)};
      *list->PushElem() = {"shift2B",PushString(out,l.shiftExpression)};
    }
  } else if(maxLoops > 1){
    if(port == 0){
      *list->PushElem() = {"per2A","0"};
      *list->PushElem() = {"incr2A","0"};
      *list->PushElem() = {"iter2A","0"};
      *list->PushElem() = {"shift2A","0"};
    } else {
      *list->PushElem() = {"per2B","0"};
      *list->PushElem() = {"incr2B","0"};
      *list->PushElem() = {"iter2B","0"};
      *list->PushElem() = {"shift2B","0"};
    }
  }
  
  if(compiled.size > maxLoops){
    // TODO: Proper error reporting requires us to lift the data up.
    printf("[ERROR] Address gen contains more loops than the unit is capable of handling\n");
    exit(-1);
  }
  
  return PushArrayFromList(out,list);
}

AddressAccess* CompileAddressGen(AddressGenDef* def,String content){
  Arena* out = globalPermanent;
  
  TEMP_REGION(temp,out);
 
  // Check if variables inside for loops and inside symbolic expression appear in the input list.

  // TODO: Better error reporting by allowing code to call the ReportError from the spec parser 
  bool anyError = false;
  for(AddressGenForDef loop : def->loops){
    Opt<Token> sameNameAsInput = Find(def->inputs,loop.loopVariable);

    if(sameNameAsInput.has_value()){
      ReportError2(content,loop.loopVariable,sameNameAsInput.value(),"Loop variable","Overshadows input variable");
      anyError = true;
    }

    Array<String> allStart = GetAllSymbols(loop.start,temp);
    Array<String> allEnd = GetAllSymbols(loop.end,temp);

    for(String str : allStart){
      Token asToken = {};
      asToken = str;
      if(!Contains(def->inputs,asToken)){
        printf("On address gen: %.*s:%d\n",UN(def->name),def->name.loc.start.line);
        printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(str));
        anyError = true;
      }
    }
    for(String str : allEnd){
      Token asToken = {};
      asToken = str;
      if(!Contains(def->inputs,asToken)){
        printf("On address gen: %.*s:%d\n",UN(def->name),def->name.loc.start.line);
        printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(str));
        anyError = true;
      }
    }
  }

  auto list = PushArenaList<Token>(temp);
  for(AddressGenForDef loop : def->loops){
    *list->PushElem() = loop.loopVariable;
  }
  auto allVariables = PushArrayFromList(temp,list);
  
  Array<String> symbSymbols = GetAllSymbols(def->symbolic,temp);
  for(String str : symbSymbols){
    Token asToken = {};
    asToken = str;
    if(!Contains(def->inputs,asToken) && !Contains(allVariables,asToken)){
      printf("On address gen: %.*s:%d\n",UN(def->name),def->name.loc.start.line);
      printf("\t[Error] Symbol '%.*s' inside address expression does not exist (check if name is correct, symbols inside expressions can only be inputs or loop variables)\n",UN(str));
      anyError = true;
    }
  }
  
  if(anyError){
    return nullptr;
  }
  
  auto loopVarBuilder = StartArray<String>(temp);
  for(int i = 0; i <  def->loops.size; i++){
    AddressGenForDef loop  =  def->loops[i];

    *loopVarBuilder.PushElem() = PushString(temp,loop.loopVariable);
  }
  Array<String> loopVars = EndArray(loopVarBuilder);
      
  // Builds expression for the internal address which is basically just a multiplication of all the loops sizes
  SymbolicExpression* loopExpression = PushLiteral(temp,1);
  for(AddressGenForDef loop : def->loops){
    SymbolicExpression* diff = SymbolicSub(loop.end,loop.start,temp);

    loopExpression = SymbolicMult(loopExpression,diff,temp);
  }
  SymbolicExpression* finalExpression = Normalize(loopExpression,temp);

  // Building expression for the external address
  SymbolicExpression* normalized = Normalize(def->symbolic,temp);

  SymbolicExpression* fullExpr = normalized;
  SymbolicExpression* dutyDiv = nullptr;
  if(normalized->type == SymbolicExpressionType_DIV){
      fullExpr = normalized->top;
      dutyDiv = normalized->bottom;
  }
  
  // NOTE: If this hits, we probaby need to improve the normalization of divs. It should always be possible to normalize a symbolic expression into a (A/B) format, I think.
  Assert(fullExpr->type != SymbolicExpressionType_DIV);
  
  for(String str : loopVars){
    fullExpr = Group(fullExpr,str,temp);
  }
        
  LoopLinearSum* expr = PushLoopLinearSumEmpty(temp);
  for(int i = 0; i < loopVars.size; i++){
    String var = loopVars[i];

    // TODO: This function is kinda too heavy for what is being implemented.
    Opt<SymbolicExpression*> termOpt = GetMultExpressionAssociatedTo(fullExpr,var,temp); 

    SymbolicExpression* term = termOpt.value_or(nullptr);
    if(!term){
      term = PushLiteral(temp,0);
    }

    AddressGenForDef loop = def->loops[i];

    SymbolicExpression* end = loop.end;
    
    LoopLinearSum* sum = PushLoopLinearSumSimpleVar(loop.loopVariable,term,loop.start,end,temp);
    expr = AddLoopLinearSum(sum,expr,temp);
  }
  
  // Extracts the constant term
  SymbolicExpression* toCalcConst = fullExpr;

  // TODO: Currently we are not dealing with loops that do not start at zero
  SymbolicExpression* zero = PushLiteral(temp,0);
  for(String str : loopVars){
    toCalcConst = SymbolicReplace(toCalcConst,str,zero,temp);
  }
  toCalcConst = Normalize(toCalcConst,temp);

  LoopLinearSum* freeTerm = PushLoopLinearSumFreeTerm(toCalcConst,temp);
      
  AddressAccess* result = globalAddressGen.Alloc();
  result->name = PushString(out,def->name);
  result->inputVariableNames = CopyArray<String>(def->inputs,out);
  result->internal = PushLoopLinearSumSimpleVar("x",PushLiteral(temp,1),PushLiteral(temp,0),finalExpression,out);
  result->external = AddLoopLinearSum(expr,freeTerm,out);
  result->dutyDivExpr = SymbolicDeepCopy(dutyDiv,out);
  
  return result;
};

static void EmitDebugAddressGenInfo(AddressAccess* access,CEmitter* c){
  TEMP_REGION(temp,c->arena);

  auto builder = StartString(temp);
  Repr(builder,access->internal);
  String internalStr = EndString(temp,builder);

  builder = StartString(temp);
  Repr(builder,access->external);
  String externalStr = EndString(temp,builder);

  c->Comment("[DEBUG] Internal address");
  c->Comment(internalStr);
  c->Comment("[DEBUG] External address");
  c->Comment(externalStr);
}

static String GenerateReadCompilationFunction(AccessAndType access,Arena* out){
  TEMP_REGION(temp,out);

  String varName = "args";
  AddressAccess* initial = access.access;
  int maxLoops = access.inst.loopsSupported;
  
  auto EmitStoreAddressGenIntoConfig = [varName](CEmitter* emitter,Array<Pair<String,String>> params) -> void{
    TEMP_REGION(temp,emitter->arena);
          
    for(int i = 0; i < params.size; i++){
      String str = params[i].first;
      
      String t = PushString(temp,"%.*s.%.*s",UN(varName),UN(str));
      String v = params[i].second;

      // TODO: Kinda hacky
      if(CompareString(str,"ext_addr")){
        v = PushString(temp,"(iptr) (%.*s)",UN(v));
      }

      emitter->Assignment(t,v);
    }
  };

  auto EmitDoubleOrSingleLoopCode = [maxLoops,EmitStoreAddressGenIntoConfig](CEmitter* c,int loopIndex,AddressAccess* access){
    TEMP_REGION(temp,c->arena);
    
    // TODO: The way we handle the free term is kinda sketchy.
    // NOTE: The problem is that the convert access functions do not know how to handle duty.
    AddressAccess* doubleLoop = ConvertAccessTo2External(access,loopIndex,temp);
    AddressAccess* singleLoop = ConvertAccessTo1External(access,temp);

    region(temp){
      String repr = PushRepresentation(GetLoopLinearSumTotalSize(doubleLoop->external,temp),temp);
      c->VarDeclare("int","doubleLoop",repr);
    }

    region(temp){
      String repr2 = PushRepresentation(GetLoopLinearSumTotalSize(singleLoop->external,temp),temp);
      c->VarDeclare("int","singleLoop",repr2);
    }

    // TODO: Maybe it would be better to just not generate single or double loop if we can check that one is always gonna be better than the other, right?
    c->If("(!forceSingleLoop && forceDoubleLoop) || (!forceSingleLoop && (doubleLoop < singleLoop))");
    c->Comment("Double is smaller (better)");
    region(temp){
      StringBuilder* b = StartString(temp);
      EmitDebugAddressGenInfo(doubleLoop,c);
      Repr(b,doubleLoop);

      Array<Pair<String,String>> params = InstantiateRead(doubleLoop,loopIndex,true,maxLoops,temp);
      EmitStoreAddressGenIntoConfig(c,params);
    }

    c->Else();
    c->Comment("Single is smaller (better)");
    region(temp){
      StringBuilder* b = StartString(temp);
      EmitDebugAddressGenInfo(singleLoop,c);
      Repr(b,singleLoop);

      Array<Pair<String,String>> params = InstantiateRead(singleLoop,-1,false,maxLoops,temp);
      EmitStoreAddressGenIntoConfig(c,params);
    }

    c->EndIf();
  };
  
  auto Recurse = [EmitDoubleOrSingleLoopCode,&initial](auto Recurse,int loopIndex,CEmitter* c,Arena* out) -> void{
    TEMP_REGION(temp,out);

    LoopLinearSum* external = initial->external;
          
    int totalSize = external->terms.size;
    int leftOverSize = totalSize - loopIndex;

    // Last member must generate an 'else' instead of a 'else if'
    if(leftOverSize > 1){
      c->StartExpression();
      for(int i = loopIndex + 1; i < totalSize; i++){
        if(i != loopIndex + 1){
          c->And();
        }

        c->Var(PushString(temp,"a%d",loopIndex));
        c->GreaterThan();
        c->Var(PushString(temp,"a%d",i));
      }
      
      if(loopIndex == 0){
        c->IfFromExpression();
      } else {
        // The other 'ifs' are 'elseifs' of the (loopIndex == 0) 'if'.
        c->ElseIfFromExpression();
      }

      c->Comment(PushString(temp,"Loop var %.*s is the largest",UN(external->terms[loopIndex].var)));
      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);
      
      Recurse(Recurse,loopIndex + 1,c,out);
    } else {
      c->Else();

      c->Comment(PushString(temp,"Loop var %.*s is the largest",UN(external->terms[loopIndex].var)));
      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);

      c->EndIf();
    }
  };

  String addressGenName = initial->name;
  Array<String> inputVars = initial->inputVariableNames;
  
  CEmitter* m = StartCCode(temp);

  EmitDebugAddressGenInfo(initial,m);
  
  String functionName = PushString(temp,"CompileVUnit_%.*s_Ext",UN(addressGenName));
  m->FunctionBlock("static AddressVArguments",functionName);
  m->Argument("void*","ext");
  
  for(String input : inputVars){
    m->Argument("int",input);
  }
  m->VarDeclare("AddressVArguments",varName,"{}");
  
  if(initial->external->terms.size > 1){
    for(int i = 0; i <  initial->external->terms.size; i++){
      LoopLinearSumTerm term  =  initial->external->terms[i];
      String repr = PushRepresentation(GetLoopHighestDecider(&term),temp);
      String name = PushString(temp,"a%d",i);
      String comment = PushString(temp,"Loop var: %.*s",UN(term.var));
      m->Comment(comment);
      m->VarDeclare("int",name,repr);
    }
  
    Recurse(Recurse,0,m,temp);
  } else {
    EmitDoubleOrSingleLoopCode(m,0,initial);
  }
  
  m->Return(varName);
  m->EndBlock();
  CAST* ast = EndCCode(m);

  StringBuilder* b = StartString(temp);
  Repr(ast,b,false);
  String data = EndString(out,b);

  return data;
}

static String GenerateGenCompilationFunction(AccessAndType access,Arena* out){
  TEMP_REGION(temp,out);

  String varName = "args";
  AddressAccess* initial = access.access;
  
  auto EmitStoreAddressGenIntoConfig = [varName](CEmitter* emitter,Array<Pair<String,String>> params) -> void{
    TEMP_REGION(temp,emitter->arena);
          
    for(int i = 0; i < params.size; i++){
      String str = params[i].first;
      
      String t = PushString(temp,"%.*s.%.*s",UN(varName),UN(str));
      String v = params[i].second;

      emitter->Assignment(t,v);
    }
  };

  String addressGenName = initial->name;
  Array<String> inputVars = initial->inputVariableNames;
  
  CEmitter* m = StartCCode(temp);

  auto builder = StartString(temp);
  Repr(builder,initial->external);
  String addressStr = EndString(temp,builder);

  m->Comment("[DEBUG] Address");
  m->Comment(addressStr);
  
  String functionName = PushString(temp,"CompileVUnit_%.*s",UN(addressGenName));
  m->FunctionBlock("static AddressGenArguments",functionName);

  for(String input : inputVars){
    m->Argument("int",input);
  }

  m->VarDeclare("AddressGenArguments",varName,"{}");

  Array<Pair<String,String>> params = InstantiateGen(initial,access.inst.loopsSupported,temp);
  EmitStoreAddressGenIntoConfig(m,params);
  
  m->Return(varName);
  m->EndBlock();
  CAST* ast = EndCCode(m);

  StringBuilder* b = StartString(temp);
  Repr(ast,b,false);
  String data = EndString(out,b);
  
  return data;
}

static String GenerateMemCompilationFunction(AccessAndType access,Arena* out){
  TEMP_REGION(temp,out);

  String varName = "args";
  AddressAccess* initial = access.access;
  
  auto EmitStoreAddressGenIntoConfig = [varName](CEmitter* emitter,Array<Pair<String,String>> params) -> void{
    TEMP_REGION(temp,emitter->arena);
          
    for(int i = 0; i < params.size; i++){
      String str = params[i].first;
      
      String t = PushString(temp,"%.*s.%.*s",UN(varName),UN(str));
      String v = params[i].second;

      emitter->Assignment(t,v);
    }
  };

  String addressGenName = initial->name;
  Array<String> inputVars = initial->inputVariableNames;
  
  CEmitter* m = StartCCode(temp);

  auto builder = StartString(temp);
  Repr(builder,initial->external);
  String addressStr = EndString(temp,builder);

  m->Comment("[DEBUG] Address");
  m->Comment(addressStr);

  for(int i = 0; i < ARRAY_SIZE(memInfo); i++){
    auto info = memInfo[i];

    String functionName = PushString(temp,"CompileVUnit_%.*s_%.*s",UN(addressGenName),UN(info.name));
    m->FunctionBlock("static AddressMemArguments",functionName);

    for(String input : inputVars){
      m->Argument("int",input);
    }

    m->VarDeclare("AddressMemArguments",varName,"{}");

    Array<Pair<String,String>> params = InstantiateMem(initial,info.port,info.dir,access.inst.loopsSupported,temp);
    EmitStoreAddressGenIntoConfig(m,params);
  
    m->Return(varName);
    m->EndBlock();

  }
  
  CAST* ast = EndCCode(m);

  StringBuilder* b = StartString(temp);
  Repr(ast,b,false);
  String data = EndString(out,b);
  
  return data;
}

String GenerateAddressGenCompilationFunction(AccessAndType access,Arena* out){
  FULL_SWITCH(access.inst.type){
  case AddressGenType_READ:{
    return GenerateReadCompilationFunction(access,out);
  } break;
  case AddressGenType_GEN:{
    return GenerateGenCompilationFunction(access,out);
  } break;
  case AddressGenType_MEM:{
    return GenerateMemCompilationFunction(access,out);
  } break;
  };

  return {};
}

String GenerateAddressLoadingFunction(String structName,AddressGenInst inst,Arena* out){
  TEMP_REGION(temp,out);

  AddressGenType type = inst.type;
  
  CEmitter* m = StartCCode(temp);

  String loadFunctionName = PushString(temp,"LoadVUnit_%.*s",UN(structName));
  m->FunctionBlock("static void",loadFunctionName);
  String argName = PushString(temp,"volatile %.*sConfig*",UN(structName));
  String varName = "config";
  m->Argument(argName,varName);

  auto EmitAssign = [m,temp](String str){
    String lhs = PushString(temp,"config->%.*s",UN(str));
    String rhs = PushString(temp,"args.%.*s",UN(str));
    m->Assignment(lhs,rhs);
  };
  
  FULL_SWITCH(type){
  case AddressGenType_GEN:{
    m->Argument("AddressGenArguments","args");
    for(int i = 0; i < META_AddressGenBaseParameters_Members.size; i++){
      String str = META_AddressGenBaseParameters_Members[i];
      EmitAssign(str);
    }
    for(int i = 2; i < inst.loopsSupported + 1; i++){
      for(String format : AddressGenExtraFormat){
        String inst = PushString(temp,format.data,i);
        EmitAssign(inst);
      }
    }
  } break;
  case AddressGenType_READ:{
    m->Argument("AddressVArguments","args");

    for(int i = 0; i <  META_AddressVParameters_Members.size; i++){
      String str = META_AddressVParameters_Members[i];
      EmitAssign(str);
    }
    for(int i = 2; i < inst.loopsSupported + 1; i++){
      for(String format : AddressGenExtraFormat){
        String inst = PushString(temp,format.data,i);
        EmitAssign(inst);
      }
    }
  } break;
  case AddressGenType_MEM:{
    m->Argument("AddressMemArguments","args");

    for(int i = 0; i <  META_AddressMemParameters_Members.size; i++){
      String str = META_AddressMemParameters_Members[i];
      EmitAssign(str);
    }
    for(int i = 2; i < inst.loopsSupported + 1; i++){
      for(String format : AddressGenMemExtraFormat){
        String inst = PushString(temp,format.data,i);
        EmitAssign(inst);
      }
    }
  } break;
  }

  CAST* ast = EndCCode(m);

  StringBuilder* strBuilder = StartString(temp);
  Repr(ast,strBuilder);
  String data = EndString(out,strBuilder);

  return data;
}

String GenerateAddressCompileAndLoadFunction(String structName,AddressAccess* access,AddressGenInst inst,Arena* out){
  TEMP_REGION(temp,out);

  AddressGenType type = inst.type;
  CEmitter* m = StartCCode(temp);
      
  if(type == AddressGenType_MEM){
    for(int i = 0; i < ARRAY_SIZE(memInfo); i++){
      auto info = memInfo[i];
      
      String functionName = PushString(temp,"%.*s_%.*s_%.*s",UN(access->name),UN(structName),UN(info.name));
      String loadFunctionName = PushString(temp,"LoadVUnit_%.*s",UN(structName));

      Array<String> inputVars = access->inputVariableNames;

      m->FunctionBlock("static void",functionName);

      String argName = PushString(temp,"volatile %.*sConfig*",UN(structName));
      String varName = "config";
      m->Argument(argName,varName);
      
      for(String input : inputVars){
        m->Argument("int",input);
      }

      auto strBuilder = StartString(temp);
      strBuilder->PushString("CompileVUnit_%.*s_%.*s(",UN(access->name),UN(info.name));

      bool addComma = false;
      for(String input : inputVars){
        if(addComma){
          strBuilder->PushString(",");
        }
        addComma = true;
        strBuilder->PushString(input);
      }
      strBuilder->PushString(")");
      String functionCall = EndString(temp,strBuilder);

      String load = PushString(temp,"%.*s(%.*s,args)",UN(loadFunctionName),UN(varName));

      m->Assignment("AddressMemArguments args",functionCall);
      
      m->Statement(load);
      m->EndBlock();
    }

    CAST* ast = EndCCode(m);

    auto strBuilder = StartString(temp);
    Repr(ast,strBuilder);
    String data = EndString(out,strBuilder);

    return data;
  }
    
  String functionName = PushString(temp,"%.*s_%.*s",UN(access->name),UN(structName));
  String loadFunctionName = PushString(temp,"LoadVUnit_%.*s",UN(structName));

  Array<String> inputVars = access->inputVariableNames;

  m->FunctionBlock("static void",functionName);

  String argName = PushString(temp,"volatile %.*sConfig*",UN(structName));
  String varName = "config";
  m->Argument(argName,varName);

  FULL_SWITCH(type){
  case AddressGenType_READ: {
    m->Argument("void*","ext");
  } break;
  case AddressGenType_GEN: {
  } break;
  case AddressGenType_MEM:{
    Assert(false);
  } break;
  }

  for(String input : inputVars){
    m->Argument("int",input);
  }

  auto strBuilder = StartString(temp);

  // TODO: This is bad and was rushed, but the bigger problem is how the CEmitter does not have an function expression builder, kinda like the one used for the 'if' construction of expressions.
  bool addComma = true;
  FULL_SWITCH(type){
  case AddressGenType_READ: {
    strBuilder->PushString("CompileVUnit_%.*s_Ext(ext",UN(access->name));
  } break;
  case AddressGenType_GEN: {
    strBuilder->PushString("CompileVUnit_%.*s(",UN(access->name));
    addComma = false;
  } break;
  case AddressGenType_MEM:{
    Assert(false);
  } break;
  }

  for(String input : inputVars){
    if(addComma){
      strBuilder->PushString(",");
    }
    addComma = true;
    strBuilder->PushString(input);
  }
  strBuilder->PushString(")");
  String functionCall = EndString(temp,strBuilder);

  String load = PushString(temp,"%.*s(%.*s,args)",UN(loadFunctionName),UN(varName));

  FULL_SWITCH(type){
  case AddressGenType_READ: {
    m->Assignment("AddressVArguments args",functionCall);
  } break;
  case AddressGenType_GEN: {
    m->Assignment("AddressGenArguments args",functionCall);
  } break;
  case AddressGenType_MEM:{
    Assert(false);
  } break;
  }
      
  m->Statement(load);
  CAST* ast = EndCCode(m);

  strBuilder = StartString(temp);
  Repr(ast,strBuilder);
  String data = EndString(out,strBuilder);
  return data;
}

String GenerateAddressPrintFunction(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  CEmitter* m = StartCCode(temp);

  Array<String> inputVars = access->inputVariableNames;

  String functionName = PushString(temp,"Print_%.*s",UN(access->name));
  m->FunctionBlock("static void",functionName);
  for(String input : inputVars){
    m->Argument("int",input);
  }

  for(String input : inputVars){
    String statement = PushString(temp,"printf(\"%.*s:%%d\\n\",%.*s)",UN(input),UN(input));
    m->Statement(statement);
  }
  
  CAST* ast = EndCCode(m);

  auto strBuilder = StartString(temp);
  Repr(ast,strBuilder);
  String data = EndString(out,strBuilder);
  return data;
}
