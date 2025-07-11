#include "addressGen.hpp"

#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versatSpecificationParser.hpp"

Pool<AddressAccess> globalAddressGen;

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

  result->external = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(temp,1),PushLiteral(temp,0),maxLoopValue,out);
  result->external->freeTerm = SymbolicDeepCopy(freeTerm,out);
  
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
  terms[1] = PushVariable(out,STRING("VERSAT_DIFF_W"));
  maxLoopValueExpr = SymbolicFunc(STRING("ALIGN"),terms,out);
  
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

  result.length = GetLoopSizeRepr(inner,out);

  if(size == 1){
    result.totalTransferSize = result.length;
    result.amountMinusOne = STRING("0");
    result.addrShift = STRING("0");
  } else {
    // TODO: A bit repetitive, could use some cleanup
    SymbolicExpression* outerLoopSize = GetLoopSize(outer,temp);
    
    result.amountMinusOne = GetLoopSizeRepr(outer,out,true);

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

AddressVParameters InstantiateAccess(AddressAccess* access,int highestExternalLoop,bool doubleLoop,Arena* out){
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

  // NOTE: The reason we need to align is because the increase in AXI_DATA_W forces data to be aligned inside the VUnits memories. The single loop does not care because we only need to access values individually, but the double loop cannot function because it assumes that the data is read and stored in a linear matter while in reality the data is stored in multiples of (AXI_DATA_W/DATA_W). 
  
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

  // TODO: This is stupid. Need to represent the per,incr,iter,shift as a struct and have an array to be able to handle N outputs automatically, hardcoding like this is not the way.
  // TODO: Do not like this being as hardcoded as it is, but we cannot really progress until we start tackling the AXI_DATA_W and start properly testing stuff.
  if(internal.size > 2){
    InternalMemoryAccess l = internal[2]; 
    res.per3 = PushString(out,l.periodExpression);
    res.incr3 = PushString(out,l.incrementExpression);
    res.iter3 = PushString(out,l.iterationExpression);
    res.shift3 = PushString(out,l.shiftExpression);
  } else {
    res.per3 = STRING("0");
    res.incr3 = STRING("0");
    res.iter3 = STRING("0");
    res.shift3 = STRING("0");
  }

  if(internal.size > 3){
    // TODO: Proper error reporting requires us to lift the data up.
    printf("Error instantiating address gen. Contains more loops than the unit is capable of handling\n");
    exit(-1);
  }
  
  return res;
}

AddressAccess* ConvertAddressGenDef(AddressGenDef* def,String content){
  Arena* out = globalPermanent;
  
  TEMP_REGION(temp,out);
 
  // Check if variables inside for loops and inside symbolic expression appear in the input list.

  // TODO: Better error reporting by allowing code to call the ReportError from the spec parser 
  bool anyError = false;
  for(AddressGenForDef loop : def->loops){
    Opt<Token> sameNameAsInput = Find(def->inputs,loop.loopVariable);

    if(sameNameAsInput.has_value()){
      // TODO: We want an error of the type : Loop variable in line: (show line) overshadows variable 'var' in: (show line). Where the show line part is where we print the line with arrows indicating where the mismatched token exists.
      //ReportError2(content,loop.
      
      ReportError2(content,loop.loopVariable,sameNameAsInput.value(),"Loop variable","Overshadows input variable");
      anyError = true;
    }

    Array<String> allStart = GetAllSymbols(loop.start,temp);
    Array<String> allEnd = GetAllSymbols(loop.end,temp);

    for(String str : allStart){
      Token asToken = {};
      asToken = str;
      if(!Contains(def->inputs,asToken)){
        printf("[Error] Loop expression variable does not appear inside input list (did you forget to declare it as input?)\n");
        anyError = true;
      }
    }
    for(String str : allEnd){
      Token asToken = {};
      asToken = str;
      if(!Contains(def->inputs,asToken)){
        printf("[Error] Loop expression variable does not appear inside input list (did you forget to declare it as input?)\n");
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
      printf("[Error] Symbol in expression does not exist (check if name is correct, symbols in expression can only be inputs or loop variables)\n");
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
  for(String str : loopVars){
    normalized = Group(normalized,str,temp);
  }
        
  LoopLinearSum* expr = PushLoopLinearSumEmpty(temp);
  for(int i = 0; i < loopVars.size; i++){
    String var = loopVars[i];

    // TODO: This function is kinda too heavy for what is being implemented.
    Opt<SymbolicExpression*> termOpt = GetMultExpressionAssociatedTo(normalized,var,temp); 

    SymbolicExpression* term = termOpt.value_or(nullptr);
    if(!term){
      term = PushLiteral(temp,0);
    }

    AddressGenForDef loop = def->loops[i];
    LoopLinearSum* sum = PushLoopLinearSumSimpleVar(loop.loopVariable,term,loop.start,loop.end,temp);
    expr = AddLoopLinearSum(sum,expr,temp);
  }

  // Extracts the constant term
  SymbolicExpression* toCalcConst = normalized;

  // TODO: Currently we are not dealing with loops that do not start at zero
  SymbolicExpression* zero = PushLiteral(temp,0);
  for(String str : loopVars){
    toCalcConst = SymbolicReplace(toCalcConst,str,zero,temp);
  }
  toCalcConst = Normalize(toCalcConst,temp);

  LoopLinearSum* freeTerm = PushLoopLinearSumFreeTerm(toCalcConst,temp);
      
  AddressAccess* result = globalAddressGen.Alloc();
  result->name = PushString(out,def->name);
  result->type = def->type;
  result->inputVariableNames = CopyArray<String>(def->inputs,out);
  result->internal = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(temp,1),PushLiteral(temp,0),finalExpression,out);
  result->external = AddLoopLinearSum(expr,freeTerm,out);

  return result;
};
