#include "addressGen.hpp"

#include "symbolic.hpp"

void SimulateAddressAccess(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

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

  Hashmap<String,int>* externalEnv = PushHashmap<String,int>(temp,access->externalLoops.size);
  GrowableArray<int> externalBuilder = StartArray<int>(temp);
  Recurse(Recurse,externalEnv,externalBuilder,access->externalLoops,access->externalAddress,0);
  Array<int> externalValues = EndArray(externalBuilder);

  Hashmap<String,int>* internalEnv = PushHashmap<String,int>(temp,access->internalLoops.size);
  GrowableArray<int> internalBuilder = StartArray<int>(temp);
  Recurse(Recurse,internalEnv,internalBuilder,access->internalLoops,access->internalAddress,0);
  Array<int> internalValues = EndArray(internalBuilder);

  printf("External contains %d values\n",externalValues.size);
  printf("Internal contains %d values\n",internalValues.size);
  
  for(int index : internalValues){
    printf("%d ",externalValues[index]);
  }

  printf("\n");
}

AddressAccess* RemoveConstantFromInnerLoop(AddressAccess* access,Arena* out){
  TEMP_REGION(temp,out);

  int size = access->externalLoops.size;
  String innerLoopVariableName = access->externalLoops[size-1].loopVariableName;

  Array<SymbolicExpression*> individualSumTerms = access->externalAddress->sum;
  
  int termIndexThatContainsVariable = -1;
  SymbolicExpression* term = nullptr;
  for(int i = 0; i < individualSumTerms.size; i++){
    Opt<SymbolicExpression*> termOpt = GetMultExpressionAssociatedTo(individualSumTerms[i],innerLoopVariableName,temp);
    if(termOpt.has_value()){
      termIndexThatContainsVariable = i;
      term = termOpt.value();
      break;
    }
  }

  if(!term){
    printf("Called remove constant from inner loop but access does not appear to have a constant on inner loop:\n");
    PrintAccess(access);
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
    res->negative = access->externalAddress->negative;
    res->sum = children;

    SymbolicExpression* end = access->externalLoops[size-1].end;
    SymbolicExpression* offset = Normalize(SymbolicAdd(end,PushLiteral(out,-1),temp),out);
    
    access->externalLoops[size-1].end = Normalize(SymbolicAdd(SymbolicMult(term,offset,temp),PushLiteral(out,1),temp),out);
    access->externalAddress = res;
    
    SymbolicExpression* alteredInner = Normalize(SymbolicMult(access->internalAddress,term,temp),out);
    access->internalAddress = alteredInner;
  }
  
  return access;
}

AddressAccess* TryToSimplifyLoop(AddressAccess* access,Arena* out){

}

AddressAccess* RemoveInnerLoop(AddressAccess* access,Arena* out){
  
  
}

int TotalLoopSize(AddressAccess* access){

}

String LoopVariableName(AddressAccess* access,int loopIndex,bool externalLoop = true){
  
}

void PrintAccess(AddressAccess* access){
  TEMP_REGION(temp,nullptr);

  printf("\n");
  {
    printf("External:\n");
    for(LoopDefinition def : access->externalLoops){
      String start = PushRepresentation(def.start,temp);
      String end = PushRepresentation(def.end,temp);

      printf("for %.*s %.*s..%.*s\n",UNPACK_SS(def.loopVariableName),UNPACK_SS(start),UNPACK_SS(end));
    }

    String addrFunc = PushRepresentation(access->externalAddress,temp);
  
    printf("addr = %.*s;\n\n",UNPACK_SS(addrFunc));
  }

  {
    printf("Internal:\n");
    for(LoopDefinition def : access->internalLoops){
      String start = PushRepresentation(def.start,temp);
      String end = PushRepresentation(def.end,temp);

      printf("for %.*s %.*s..%.*s\n",UNPACK_SS(def.loopVariableName),UNPACK_SS(start),UNPACK_SS(end));
    }
  
    String addrFunc = PushRepresentation(access->internalAddress,temp);
  
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

  AddressAccess* second = RemoveInnerLoop(access,out);
  printf("Second:\n\n");
  PrintAccess(second);

  return second;
  
  // The first loop must be a burst and therefore cannot contain any constant multiplier 
  if(access->externalLoops.size > maxExternalLoops){
    String variableName = LoopVariableName(access,0,true);
  }
  
  while(access->externalLoops.size > maxExternalLoops){
    AddressAccess* simple = TryToSimplifyLoop(access,out);

    if(simple){
      access = simple;
      continue;
    }
    
    access = RemoveInnerLoop(access,out);
  }
  
  if(TotalLoopSize(access) > maxLoopsTotal){
    // Can we do something to remove an external loop without adding an internal loop?
  }
  
}
