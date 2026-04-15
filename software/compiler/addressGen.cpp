#include "addressGen.hpp"

#include "embeddedData.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versatSpecificationParser.hpp"
#include "CEmitter.hpp"

AddressAccess* Copy(AddressAccess* in,Arena* out){
  AddressAccess* res = PushStruct<AddressAccess>(out);

  res->external = Copy(in->external,out);
  res->internal = Copy(in->internal,out);
  res->inputVariableNames = CopyArray(in->inputVariableNames,out);

  return res;
}

SYM_Expr LoopMaximumValue(LoopLinearSumTerm term){
  SYM_Expr maxVal = term.loopEnd - SYM_One;

  return maxVal;
}

void Repr(StringBuilder* builder,AddressAccess* access){
  TEMP_REGION(temp,builder->arena);

  builder->PushString("External:\n");
  Repr(builder,access->external);
  builder->PushString("\n\nInternal:\n");
  Repr(builder,access->internal);
  builder->PushString("\n");
}

String PushRepr(Arena* out,AddressAccess* access){
  TEMP_REGION(temp,out);
  
  // TODO: Performance
  auto b = StartString(temp);
  Repr(b,access);
  return EndString(out,b);
}

void Print(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  printf("External:\n");
  Print(access->external);
  printf("\nInternal:\n");
  Print(access->internal);
  printf("\n");
  if(!SYM_IsOneValue(access->dutyDivExpr)){
    printf("Duty:\n");
    SYM_Print(access->dutyDivExpr);
    printf("\n");
  }
}

SYM_Expr EvaluateMaxLinearSumValue(LoopLinearSum* sum){
  SYM_Expr val = sum->freeTerm;

  for(int i = 0; i <  sum->terms.size; i++){
    LoopLinearSumTerm term  =  sum->terms[i];

    SYM_Expr maxLoopValue = LoopMaximumValue(term);
    SYM_Expr maxTermValue = term.term * maxLoopValue;
    
    val += maxTermValue;
  }

  return val;
}

AddressAccess* ConvertAccessTo1External(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  SYM_Expr freeTerm = access->external->freeTerm;
  
  AddressAccess* result = Copy(access,out);
  result->external->freeTerm = SYM_Zero; // Pretend that the free term does not exist
  
  result->internal = Copy(result->external,out);
  result->external = PushLoopLinearSumEmpty(out);
  
  SYM_Expr maxLoopValue = EvaluateMaxLinearSumValue(result->internal);
  maxLoopValue += SYM_One;

  result->external = PushLoopLinearSumSimpleVar("x",SYM_One,SYM_Zero,maxLoopValue,out);
  result->external->freeTerm = freeTerm;
  result->dutyDivExpr = access->dutyDivExpr;
  
  return result;
}

AddressAccess* ConvertAccessTo2External(AddressAccess* access,int biggestLoopIndex,Arena* out){
  AddressAccess* result = Copy(access,out);

  LoopLinearSum* external = result->external;
  
  // We pretent that the free term does not exist and then shift the initial address of the final external expression by the free term.
  SYM_Expr freeTerm = external->freeTerm;
  external->freeTerm = SYM_Zero;
  
  int highestConstantIndex = biggestLoopIndex;
  SYM_Expr highestConstant = access->external->terms[biggestLoopIndex].term;

  LoopLinearSum oneSort = {};
  oneSort.terms = RemoveElement(external->terms,highestConstantIndex,out);
  oneSort.freeTerm = external->freeTerm;

  // We perform a loop "evaluation" here. 
  SYM_Expr val = EvaluateMaxLinearSumValue(&oneSort);
  SYM_Expr maxLoopValueExpr = val + SYM_One;

  maxLoopValueExpr = SYM_Align(maxLoopValueExpr,SYM_Var("VERSAT_DIFF_W"));
  
  result->internal = Copy(external,out);
  result->internal->terms[highestConstantIndex].term = maxLoopValueExpr; //PushLiteral(out,maxLoopValue);
  
  LoopLinearSum* innermostExternal = PushLoopLinearSumSimpleVar("x",SYM_One,SYM_Zero,maxLoopValueExpr,out);

  // NOTE: Not sure about the end of this loop. We carry it directly but need to do further tests to make sure that this works fine.
  LoopLinearSum* outerMostExternal = PushLoopLinearSumSimpleVar("y",highestConstant,SYM_Zero,external->terms[highestConstantIndex].loopEnd,out);
  
  result->external = AddLoopLinearSum(innermostExternal,outerMostExternal,out);
  result->external->freeTerm = freeTerm;
  result->dutyDivExpr = access->dutyDivExpr;
  
  return result;
}

AddressAccess* ReplaceVariables(AddressAccess* in,TrieMap<String,SYM_Expr>* varReplace,Array<String> newInputVariableNames,Arena* out){
  AddressAccess* res = PushStruct<AddressAccess>(out);
  
  res->name = PushString(out,in->name);
  res->internal = ReplaceVariables(in->internal,varReplace,out);
  res->external = ReplaceVariables(in->external,varReplace,out);
  res->dutyDivExpr = SYM_Replace(in->dutyDivExpr,varReplace);
  res->inputVariableNames = CopyArray(newInputVariableNames,out);
  res->loopVars = CopyArray(in->loopVars,out);
  
  return res;
}

SYM_Expr GetLoopHighestDecider(LoopLinearSumTerm* term){
  return term->term;
}

SYM_Expr GetLoopSize(LoopLinearSumTerm def,bool removeOne = false){
  SYM_Expr diff = def.loopEnd - def.loopStart;
    
  if(removeOne){
    diff = diff - SYM_One;
  }
    
  return diff;
};

String GetLoopSizeRepr(LoopLinearSumTerm def,Arena* out,bool removeOne = false){
  return SYM_Repr(GetLoopSize(def,removeOne),out);
};

static ExternalMemoryAccess CompileExternalMemoryAccess(LoopLinearSum* access,SYM_Expr dutyExpr,Arena* out){
  TEMP_REGION(temp,out);

  int size = access->terms.size;

  Assert(size <= 2);

  ExternalMemoryAccess result = {};

  LoopLinearSumTerm inner = access->terms[0];
  LoopLinearSumTerm outer = access->terms[access->terms.size - 1];

  SYM_Expr fullExpression = TransformIntoSymbolicExpression(access,temp);
  
  result.length = GetLoopSizeRepr(inner,out);
  
  if(size == 1){
    result.totalTransferSize = result.length;
    result.amountMinusOne = "0";
    result.addrShift = "0";
  } else {
    SYM_Expr derived = SYM_Derivate(fullExpression,outer.var);
    result.addrShift = SYM_Repr(derived,out);
    
    SYM_Expr outerLoopSize = GetLoopSize(outer);
    SYM_Expr all = GetLoopSize(inner) * outerLoopSize;

    result.totalTransferSize = SYM_Repr(all,out);
    result.amountMinusOne = GetLoopSizeRepr(outer,out,true);
  }
  
  return result;
}

static CompiledAccess CompileAccess(LoopLinearSum* access,SYM_Expr dutyDiv,Arena* out){
  TEMP_REGION(temp,out);
  
  auto GenerateLoopExpressionPairSymbolic = [dutyDiv](Array<LoopLinearSumTerm> loops,
                                                      SYM_Expr expr,Arena* out) -> CompiledAccess{
    TEMP_REGION(temp,out);

    // TODO: Because we are adding an extra loop, there is a possibility of failure since the unit might not support enough loops to implement this. For the SingleLoop VS DoubleLoop the problem does not occur because we know that Singleloop is possible and DoubleLoop is easier on the address gen of the internal loop which is the limitting factor.
    //       Overall, we need to push this stuff upwards, so that we can simplify the code. It is easier to check and handle address gens that are to big before starting to emit stuff and writing to files.
    // NOTE: The only thing that we need to do is to add +1 to the amount of loops if a unit contains a duty expression. The failure is exactly the same, "address gen contains more loops than the unit is capable of handling"
    if(!SYM_IsOneValue(dutyDiv)){
      // In order to solve duty, we want to use two loops for the innermost loop. The first loop will have a period equal to the duty division expression and a duty of 1.
      // The second loop will have the expression of the original loop.
      Array<LoopLinearSumTerm> newLoops = PushArray<LoopLinearSumTerm>(temp,loops.size + 1);

      for(int i = 0; i < loops.size; i++){
        newLoops[i+1] = loops[i];
      }

      newLoops[0].var = "NONE";
      newLoops[0].term = SYM_One;
      newLoops[0].loopStart = SYM_Zero;
      newLoops[0].loopEnd = dutyDiv;
      
      loops = newLoops;
    }
    
    int loopSize = (loops.size + 1) / 2;
    Array<InternalMemoryAccess> result = PushArray<InternalMemoryAccess>(out,loopSize);
    
    for(int i = 0; i < loopSize; i++){
      LoopLinearSumTerm l0 = loops[i*2];

      InternalMemoryAccess& res = result[i];
      res = {};

      SYM_Expr derived = SYM_Derivate(expr,l0.var);
      SYM_Expr firstDerived = derived;
        
      res.periodExpression = GetLoopSize(l0);
      res.incrementExpression = firstDerived;
      
      SYM_Expr firstEndSym = l0.loopEnd;

      res.shiftWithoutRemovingIncrement = SYM_Zero; // By default
      if(i * 2 + 1 < loops.size){
        LoopLinearSumTerm l1 = loops[i*2 + 1];

        res.iterationExpression = GetLoopSize(l1);
        SYM_Expr derived = SYM_Derivate(expr,l1.var);

        res.shiftWithoutRemovingIncrement = derived;
        
        // We handle shifts very easily. We just remove the effects of all the previous period increments and then apply the shift.
        // That way we just have to calculate the derivative in relation to the shift, instead of calculating the change from a period term to a iteration term.
        // We need to subtract 1 because the period increment is only applied (period - 1) times.

        res.shiftExpression = -(firstDerived * (firstEndSym - SYM_One)) + derived;
      } else {
        res.iterationExpression = SYM_Zero;
        res.shiftExpression = SYM_Zero;
      }

      if(i == 0){
        if(SYM_IsOneValue(dutyDiv)){
          result[0].dutyExpression = result[0].periodExpression;
        } else {
          result[0].dutyExpression = result[0].periodExpression / dutyDiv; 
        }
      }
    }

    CompiledAccess com = {};
    com.internalAccess = result;
    com.dutyDivExpression = dutyDiv;
    
    return com;
  };

  SYM_Expr fullExpression = TransformIntoSymbolicExpression(access,temp);
  CompiledAccess res = GenerateLoopExpressionPairSymbolic(access->terms,fullExpression,out);
  
  return res;
}

static Array<Pair<String,String>> InstantiateGen(AddressAccess* access,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);
  Array<InternalMemoryAccess> compiled = CompileAccess(access->external,access->dutyDivExpr,temp).internalAccess;
  SYM_Expr freeTerm = access->external->freeTerm;

  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);
  String start = SYM_Repr(freeTerm,out);

  *list->PushElem() = {"start",start};

  {
    InternalMemoryAccess l = compiled[0]; 

    *list->PushElem() = {"duty",SYM_Repr(l.dutyExpression,out)};
    *list->PushElem() = {"per",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift",SYM_Repr(l.shiftExpression,out)};
  }
    
  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(compiled.size > 1){
    InternalMemoryAccess l = compiled[1]; 
    *list->PushElem() = {"per2",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr2",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter2",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift2",SYM_Repr(l.shiftExpression,out)};
  } else if(maxLoops > 1){
    *list->PushElem() = {"per2","0"};
    *list->PushElem() = {"incr2","0"};
    *list->PushElem() = {"iter2","0"};
    *list->PushElem() = {"shift2","0"};
  }

  if(compiled.size > 2){
    InternalMemoryAccess l = compiled[2]; 
    *list->PushElem() = {"per3",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr3",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter3",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift3",SYM_Repr(l.shiftExpression,out)};
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
  
  return PushArray(out,list);
}

Array<Pair<String,String>> InstantiateRead(AddressAccess* access,int highestExternalLoop,bool doubleLoop,int maxLoops,String extVarName,Arena* out){
  TEMP_REGION(temp,out);

  ExternalMemoryAccess external = CompileExternalMemoryAccess(access->external,access->dutyDivExpr,temp);
  CompiledAccess compiled = CompileAccess(access->internal,access->dutyDivExpr,temp);

  auto internal = compiled.internalAccess;
  
  SYM_Expr freeTerm = access->external->freeTerm;
    
  if(freeTerm == SYM_Zero){
    freeTerm = access->internal->freeTerm;
  } else {
    // NOTE: I do not think it is possible for both external and internal to have free terms.
    Assert(access->internal->freeTerm == SYM_Zero);
  }

  // ======================================
  // NOTE: VERY IMPORTANT: If a field is not set, then set it to
  //       zero. Do not leave it hanging otherwise future
  //       configuration calls do not change it and the unit gets
  //       misconfigured.
  
  // NOTE: We push the start term to the ext pointer in order to save memory inside the unit. This is being done in a  kinda hacky way, but nothing major.
  String ext_addr = extVarName;
  if(freeTerm != SYM_Zero){
    String repr = SYM_Repr(freeTerm,temp);

    ext_addr = PushString(out,"(((float*) %.*s) + (%.*s))",UN(extVarName),UN(repr));
  }
  
  // TODO: No need for a list, we already know all the memory that we are gonna need
  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);

  *list->PushElem() = {"extra_delay",SYM_Repr(compiled.dutyDivExpression - SYM_One,out)};
  
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

    *list->PushElem() = {"duty",SYM_Repr(l.dutyExpression,out)};
    *list->PushElem() = {"per",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift",SYM_Repr(l.shiftExpression,out)};
  }
    
  if(internal.size > 1){
    InternalMemoryAccess l = internal[1]; 
    *list->PushElem() = {"per2",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr2",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter2",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift2",SYM_Repr(l.shiftExpression,out)};
  } else if(maxLoops > 1) {
    *list->PushElem() = {"per2","0"};
    *list->PushElem() = {"incr2","0"};
    *list->PushElem() = {"iter2","0"};
    *list->PushElem() = {"shift2","0"};
  }

  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(internal.size > 2){
    InternalMemoryAccess l = internal[2]; 
    *list->PushElem() = {"per3",SYM_Repr(l.periodExpression,out)};
    *list->PushElem() = {"incr3",SYM_Repr(l.incrementExpression,out)};
    *list->PushElem() = {"iter3",SYM_Repr(l.iterationExpression,out)};
    *list->PushElem() = {"shift3",SYM_Repr(l.shiftExpression,out)};
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
  
  return PushArray(out,list);
}

static Array<Pair<String,String>> InstantiateMem(AddressAccess* access,int port,bool input,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);
  Array<InternalMemoryAccess> compiled = CompileAccess(access->external,access->dutyDivExpr,temp).internalAccess;
  SYM_Expr freeTerm = access->external->freeTerm;

  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);
  String start = SYM_Repr(freeTerm,out);

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
  
  if(compiled.size > 0){
    InternalMemoryAccess l = compiled[0]; 

    if(port == 0){
      *list->PushElem() = {"dutyA",SYM_Repr(l.dutyExpression,out)};
      *list->PushElem() = {"perA",SYM_Repr(l.periodExpression,out)};
      *list->PushElem() = {"incrA",SYM_Repr(l.incrementExpression,out)};
      *list->PushElem() = {"iterA",SYM_Repr(l.iterationExpression,out)};
      *list->PushElem() = {"shiftA",SYM_Repr(l.shiftExpression,out)};
    } else {
      *list->PushElem() = {"dutyB",SYM_Repr(l.dutyExpression,out)};
      *list->PushElem() = {"perB",SYM_Repr(l.periodExpression,out)};
      *list->PushElem() = {"incrB",SYM_Repr(l.incrementExpression,out)};
      *list->PushElem() = {"iterB",SYM_Repr(l.iterationExpression,out)};
      *list->PushElem() = {"shiftB",SYM_Repr(l.shiftExpression,out)};
    }
  } else {
    // NOTE: Assume that the an empty loop is the same as a one iteration loop
    if(port == 0){
      *list->PushElem() = {"dutyA","1"};
      *list->PushElem() = {"perA","1"};
      *list->PushElem() = {"incrA","1"};
      *list->PushElem() = {"iterA","0"};
      *list->PushElem() = {"shiftA","0"};
    } else {
      *list->PushElem() = {"dutyB","1"};
      *list->PushElem() = {"perB","1"};
      *list->PushElem() = {"incrB","1"};
      *list->PushElem() = {"iterB","0"};
      *list->PushElem() = {"shiftB","0"};
    }
  }
    
  // TODO: This is stupid. We can just put some loop logic in here. Do this when tests are stable and quick changes are easy to do.
  if(compiled.size > 1){
    InternalMemoryAccess l = compiled[1]; 

    if(port == 0){
      *list->PushElem() = {"per2A",SYM_Repr(l.periodExpression,out)};
      *list->PushElem() = {"incr2A",SYM_Repr(l.incrementExpression,out)};
      *list->PushElem() = {"iter2A",SYM_Repr(l.iterationExpression,out)};
      *list->PushElem() = {"shift2A",SYM_Repr(l.shiftExpression,out)};
    } else {
      *list->PushElem() = {"per2B",SYM_Repr(l.periodExpression,out)};
      *list->PushElem() = {"incr2B",SYM_Repr(l.incrementExpression,out)};
      *list->PushElem() = {"iter2B",SYM_Repr(l.iterationExpression,out)};
      *list->PushElem() = {"shift2B",SYM_Repr(l.shiftExpression,out)};
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
  
  return PushArray(out,list);
}

AddressAccess* CompileAddressGen(Env* env,Array<Token> inputs,Array<AddressGenForDef> loops,SYM_Expr addr,String content){
  Arena* out = globalPermanent;
  
  TEMP_REGION(temp,out);

  Array<String> asString = PushArray<String>(out,inputs.size);

  for(int i = 0; i <  inputs.size; i++){
    Token t = inputs[i];
    asString[i] = t.identifier;
  }

  // TODO: Issue a warning if a variable is declared but not used.
  // TODO: Better error reporting by allowing code to call the ReportError from the spec parser 
  bool anyError = false;
  for(AddressGenForDef loop : loops){
    Opt<String> sameNameAsInput = Find(asString,loop.loopVariable);

    if(sameNameAsInput.has_value()){
      //ReportErrorGoodTokenExists(content,loop.loopVariable,sameNameAsInput.value(),"Loop variable","Overshadows input variable");
      anyError = true;
    }

#if 0
    {
      auto tokens = AccumTokens(loop.startSym,temp);
      for(NewToken tok : tokens){
        if(!Contains(inputs,tok)){
          printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(tok.identifier));
          anyError = true;
        }
      }
    }

    {
      auto tokens = AccumTokens(loop.endSym,temp);
      for(NewToken tok : tokens){
        if(!Contains(inputs,tok)){
          printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(tok.identifier));
          anyError = true;
        }
      }
    }
#endif
  }
  
  if(anyError){
    return nullptr;
  }
  
  auto loopVarBuilder = PushList<String>(temp);
  for(int i = 0; i < loops.size; i++){
    AddressGenForDef loop = loops[i];

    *loopVarBuilder->PushElem() = PushString(temp,loop.loopVariable);
  }
  Array<String> loopVars = PushArray(out,loopVarBuilder);

  Array<SYM_Expr> loopEnd = PushArray<SYM_Expr>(temp,loops.size);
  
  SYM_Expr symbolicExpr = addr;

  // Builds expression for the internal address which is basically just a multiplication of all the loops sizes
  SYM_Expr loopExpression = SYM_One;
  for(int i = 0; i <  loops.size; i++){
    AddressGenForDef loop = loops[i];
    // TODO: Handle parsing errors
    SYM_Expr start = env->SymbolicFromMathExpression(loop.startSym);
    loopEnd[i] = env->SymbolicFromMathExpression(loop.endSym);

    // NOTE: We transform loops so that they always start at zero:
    //       - for x a..b {x} <===> for x 0..b-a {(x+a)} 
    if(start != SYM_Zero){
      loopEnd[i] = loopEnd[i] - start;

      SYM_Expr loopVar = SYM_Var(loop.loopVariable);
      symbolicExpr = SYM_Replace(symbolicExpr,loopVar,loopVar + start);
    }

    SYM_Expr diff = loopEnd[i];
    loopExpression = loopExpression * diff;

  }
  SYM_Expr finalExpression = loopExpression;

  // Building expression for the external address
  // TODO: Handle parsing errors
  SYM_Expr normalized = symbolicExpr;

  Pair<SYM_Expr,SYM_Expr> pair = SYM_BreakDiv(normalized);

  SYM_Expr fullExpr = pair.first;
  SYM_Expr dutyDiv = pair.second;

  LoopLinearSum* expr = PushLoopLinearSumEmpty(temp);
  for(int i = 0; i < loopVars.size; i++){
    String var = loopVars[i];

    SYM_Expr term = SYM_Factor(fullExpr,SYM_Var(var));

    AddressGenForDef loop = loops[i];
    
    LoopLinearSum* sum = PushLoopLinearSumSimpleVar(loop.loopVariable,term,SYM_Zero,loopEnd[i],temp);
    expr = AddLoopLinearSum(sum,expr,temp);
  }
  
  // Extracts the constant term
  SYM_Expr toCalcConst = fullExpr;

  for(String str : loopVars){
    toCalcConst = SYM_Replace(toCalcConst,SYM_Var(str),SYM_Zero);
  }
  toCalcConst = toCalcConst;

  LoopLinearSum* freeTerm = PushLoopLinearSumFreeTerm(toCalcConst,temp);
      
  AddressAccess* result = PushStruct<AddressAccess>(out);
  result->inputVariableNames = asString;
  result->internal = PushLoopLinearSumSimpleVar("x",SYM_One,SYM_Zero,finalExpression,out);
  result->external = AddLoopLinearSum(expr,freeTerm,out);
  result->dutyDivExpr = dutyDiv;
  result->loopVars = loopVars;
  
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

void EmitReadStatements(CEmitter* m,AccessAndType access,String varName,String extVarName){
  TEMP_REGION(temp,nullptr);

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

  auto EmitDoubleOrSingleLoopCode = [extVarName,maxLoops,EmitStoreAddressGenIntoConfig](CEmitter* c,int loopIndex,AddressAccess* access){
    TEMP_REGION(temp,c->arena);
    
    // TODO: The way we handle the free term is kinda sketchy.
    // NOTE: The problem is that the convert access functions do not know how to handle duty.
    AddressAccess* doubleLoop = ConvertAccessTo2External(access,loopIndex,temp);
    AddressAccess* singleLoop = ConvertAccessTo1External(access,temp);

    region(temp){
      String repr = SYM_Repr(GetLoopLinearSumTotalSize(doubleLoop->external,temp),temp);
      c->VarDeclare("int","doubleLoop",repr);
    }

    region(temp){
      String repr2 = SYM_Repr(GetLoopLinearSumTotalSize(singleLoop->external,temp),temp);
      c->VarDeclare("int","singleLoop",repr2);
    }

    // TODO: Maybe it would be better to just not generate single or double loop if we can check that one is always gonna be better than the other, right?
    c->If("(!forceSingleLoop && forceDoubleLoop) || (!forceSingleLoop && (doubleLoop < singleLoop))");
    c->Comment("Double is smaller (better)");
    region(temp){
      StringBuilder* b = StartString(temp);
      EmitDebugAddressGenInfo(doubleLoop,c);
      Repr(b,doubleLoop);

      Array<Pair<String,String>> params = InstantiateRead(doubleLoop,loopIndex,true,maxLoops,extVarName,temp);
      EmitStoreAddressGenIntoConfig(c,params);
    }

    c->Else();
    c->Comment("Single is smaller (better)");
    region(temp){
      StringBuilder* b = StartString(temp);
      EmitDebugAddressGenInfo(singleLoop,c);
      Repr(b,singleLoop);

      Array<Pair<String,String>> params = InstantiateRead(singleLoop,-1,false,maxLoops,extVarName,temp);
      EmitStoreAddressGenIntoConfig(c,params);
    }

    c->EndIf();
  };
  
  auto Recurse = [EmitDoubleOrSingleLoopCode,&initial](auto Recurse,int loopIndex,CEmitter* c) -> void{
    TEMP_REGION(temp,nullptr);

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

        c->Var(PushString(temp,"_VERSAT_a%d",loopIndex));
        c->GreaterThan();
        c->Var(PushString(temp,"_VERSAT_a%d",i));
      }
      
      if(loopIndex == 0){
        c->IfFromExpression();
      } else {
        // The other 'ifs' are 'elseifs' of the (loopIndex == 0) 'if'.
        c->ElseIfFromExpression();
      }

      c->Comment(PushString(temp,"Loop var %.*s is the largest",UN(external->terms[loopIndex].var)));
      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);
      
      Recurse(Recurse,loopIndex + 1,c);
    } else {
      c->Else();

      c->Comment(PushString(temp,"Loop var %.*s is the largest",UN(external->terms[loopIndex].var)));
      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);

      c->EndIf();
    }
  };

  if(initial->external->terms.size > 1){
    for(int i = 0; i <  initial->external->terms.size; i++){
      LoopLinearSumTerm term  =  initial->external->terms[i];
      String repr = SYM_Repr(GetLoopHighestDecider(&term),temp);
      String name = PushString(temp,"_VERSAT_a%d",i);
      String comment = PushString(temp,"Loop var: %.*s",UN(term.var));

      m->Comment(comment);
      m->VarDeclare("int",name,repr);
    }
  
    Recurse(Recurse,0,m);
  } else {
    EmitDoubleOrSingleLoopCode(m,0,initial);
  }
}

void EmitMemStatements(CEmitter* m,AccessAndType access,String varName){
  TEMP_REGION(temp,nullptr);

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

  Assert(access.dir != Direction_NONE);

  String addressStr = PushRepr(initial->external,temp);
  m->Comment("[DEBUG] Address");
  m->Comment(addressStr);

  Array<Pair<String,String>> params = InstantiateMem(initial,access.port,access.dir == Direction_INPUT,access.inst.loopsSupported,temp);
  EmitStoreAddressGenIntoConfig(m,params);
}

void EmitGenStatements(CEmitter* m,AccessAndType access,String varName){
  TEMP_REGION(temp,nullptr);

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

  String addressStr = PushRepr(initial->external,temp);

  m->Comment("[DEBUG] Address");
  m->Comment(addressStr);

  Array<Pair<String,String>> params = InstantiateGen(initial,access.inst.loopsSupported,temp);
  EmitStoreAddressGenIntoConfig(m,params);
}
