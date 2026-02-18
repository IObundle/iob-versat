#include "addressGen.hpp"

#include "embeddedData.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
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
  return PushRepr(out,GetLoopSize(def,temp,removeOne));
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
    result.addrShift = PushRepr(out,derived);
    
    SymbolicExpression* outerLoopSize = GetLoopSize(outer,temp);
    SymbolicExpression* all = Normalize(SymbolicMult(GetLoopSize(inner,temp),outerLoopSize,temp),temp);

    result.totalTransferSize = PushRepr(out,all);
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
      res.incrementExpression = PushRepr(out,firstDerived);

      if(i == 0){
        if(duty){
          result[0].dutyExpression = "1";
        } else {
          result[0].dutyExpression = PushString(out,PushRepr(temp,loops[0].loopEnd)); // For now, do not care too much about duty. Use a full duty
        }
      }
      
      String firstEnd = PushRepr(temp,l0.loopEnd);
      SymbolicExpression* firstEndSym = ParseSymbolicExpression(firstEnd,temp);

      res.shiftWithoutRemovingIncrement = "0"; // By default
      if(i * 2 + 1 < loops.size){
        LoopLinearSumTerm l1 = loops[i*2 + 1];

        res.iterationExpression = GetLoopSizeRepr(l1,out);
        SymbolicExpression* derived = Normalize(Derivate(expr,l1.var,temp),temp);

        res.shiftWithoutRemovingIncrement = PushRepr(out,derived);
        
        // We handle shifts very easily. We just remove the effects of all the previous period increments and then apply the shift.
        // That way we just have to calculate the derivative in relation to the shift, instead of calculating the change from a period term to a iteration term.
        // We need to subtract 1 because the period increment is only applied (period - 1) times.
        SymbolicExpression* templateSym = ParseSymbolicExpression("-(firstIncrement * (firstEnd - 1)) + term",temp);

        SymbolicExpression* replaced = SymbolicReplace(templateSym,"firstIncrement",firstDerived,temp);
        replaced = SymbolicReplace(replaced,"firstEnd",firstEndSym,temp);
        replaced = SymbolicReplace(replaced,"term",derived,temp);
        replaced = Normalize(replaced,temp,false);
        
        res.shiftExpression = PushRepr(out,replaced);
      } else {
        res.iterationExpression = "0";
        res.shiftExpression = "0";
      }
    }

    CompiledAccess com = {};
    com.internalAccess = result;

    if(duty){
      com.dutyDivExpression = PushRepr(out,duty);
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

  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);
  String start = PushRepr(temp,freeTerm);

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
  
  return PushArray(out,list);
}

Array<Pair<String,String>> InstantiateRead(AddressAccess* access,int highestExternalLoop,bool doubleLoop,int maxLoops,String extVarName,Arena* out){
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

  // ======================================
  // NOTE: VERY IMPORTANT: If a field is not set, then set it to
  //       zero. Do not leave it hanging otherwise future
  //       configuration calls do not change it and the unit gets
  //       misconfigured.
  
  // NOTE: We push the start term to the ext pointer in order to save memory inside the unit. This is being done in a  kinda hacky way, but nothing major.
  String ext_addr = extVarName;
  if(!IsZero(freeTerm)){
    String repr = PushRepr(temp,freeTerm);

    ext_addr = PushString(out,"(((float*) %.*s) + (%.*s))",UN(extVarName),UN(repr));
  }
  
  // TODO: No need for a list, we already know all the memory that we are gonna need
  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);

  if(Empty(compiled.dutyDivExpression)){
    *list->PushElem() = {"extra_delay","0"};
  } else {
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
  
  return PushArray(out,list);
}

static Array<Pair<String,String>> InstantiateMem(AddressAccess* access,int port,bool input,int maxLoops,Arena* out){
  TEMP_REGION(temp,out);
  Array<InternalMemoryAccess> compiled = CompileAccess(access->external,access->dutyDivExpr,temp).internalAccess;
  SymbolicExpression* freeTerm = access->external->freeTerm;

  ArenaList<Pair<String,String>>* list = PushList<Pair<String,String>>(temp);
  String start = PushRepr(temp,freeTerm);

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
  
  return PushArray(out,list);
}

AddressAccess* CompileAddressGen(Array<Token> inputs,Array<AddressGenForDef2> loops,SymbolicExpression* addr,String content){
  Arena* out = globalPermanent;
  
  TEMP_REGION(temp,out);

  // TODO: Issue a warning if a variable is declared but not used.
  // TODO: Better error reporting by allowing code to call the ReportError from the spec parser 
  bool anyError = false;
  for(AddressGenForDef2 loop : loops){
    Opt<Token> sameNameAsInput = Find(inputs,loop.loopVariable);

    if(sameNameAsInput.has_value()){
      ReportError2(content,loop.loopVariable,sameNameAsInput.value(),"Loop variable","Overshadows input variable");
      anyError = true;
    }

    {
      auto tokens = AccumTokens(loop.startSym,temp);
      for(Token tok : tokens){
        if(IsIdentifier(tok) && !Contains(inputs,tok)){
          printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(tok));
          anyError = true;
        }
      }
    }

    {
      auto tokens = AccumTokens(loop.endSym,temp);
      for(Token tok : tokens){
        if(IsIdentifier(tok) && !Contains(inputs,tok)){
          printf("\t[Error] Loop expression variable '%.*s' does not appear inside input list (did you forget to declare it as input?)\n",UN(tok));
          anyError = true;
        }
      }
    }
  }
  
  if(anyError){
    return nullptr;
  }
  
  auto loopVarBuilder = StartArray<String>(temp);
  for(int i = 0; i < loops.size; i++){
    AddressGenForDef2 loop = loops[i];

    *loopVarBuilder.PushElem() = PushString(temp,loop.loopVariable);
  }
  Array<String> loopVars = EndArray(loopVarBuilder);
      
  // Builds expression for the internal address which is basically just a multiplication of all the loops sizes
  SymbolicExpression* loopExpression = PushLiteral(temp,1);
  for(AddressGenForDef2 loop : loops){
    // TODO: Handle parsing errors
    // TODO: Performance, we are parsing this twice, there is another below. Maybe we can join the loops into a single one

    SymbolicExpression* start = SymbolicFromSpecExpression(loop.startSym,temp);
    SymbolicExpression* end = SymbolicFromSpecExpression(loop.endSym,temp);

    SymbolicExpression* diff = SymbolicSub(end,start,temp);

    loopExpression = SymbolicMult(loopExpression,diff,temp);
  }
  SymbolicExpression* finalExpression = Normalize(loopExpression,temp);

  // Building expression for the external address
  // TODO: Handle parsing errors
  SymbolicExpression* symbolicExpr = addr;
  //SymbolicExpression* symbolicExpr = ParseSymbolicExpression(symbolicTokens,globalPermanent);
  Assert(symbolicExpr);
  SymbolicExpression* normalized = Normalize(symbolicExpr,temp);

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

    AddressGenForDef2 loop = loops[i];
    
    // TODO: Performance, we are parsing the start and end stuff twice. This is the second, the first is above.
    SymbolicExpression* start = SymbolicFromSpecExpression(loop.startSym,temp);
    SymbolicExpression* end = SymbolicFromSpecExpression(loop.endSym,temp);
    
    LoopLinearSum* sum = PushLoopLinearSumSimpleVar(loop.loopVariable,term,start,end,temp);
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
      
  AddressAccess* result = PushStruct<AddressAccess>(out);
  result->inputVariableNames = CopyArray<String>(inputs,out);
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
      String repr = PushRepr(temp,GetLoopLinearSumTotalSize(doubleLoop->external,temp));
      c->VarDeclare("int","doubleLoop",repr);
    }

    region(temp){
      String repr2 = PushRepr(temp,GetLoopLinearSumTotalSize(singleLoop->external,temp));
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
      String repr = PushRepr(temp,GetLoopHighestDecider(&term));
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
