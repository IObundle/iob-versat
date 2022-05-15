#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "stdint.h"

#include "versat.hpp"
#include "parser.hpp"

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

typedef struct Name_t{
   char name[256];
   FUDeclaration* type;
} Name;

static Name identifiers[256];
static int identifiersIndex;

int GetIndex(char* name){
   for(int i = 0; i < identifiersIndex; i++){
      if(strcmp(identifiers[i].name,name) == 0){
         return i;
      }
   }

   Assert(0);
   return 0;
}

typedef struct Var_t{
   HierarchyName name;
   int portStart;
   int portEnd;
} Var;

Var ParseVar(Tokenizer* tok){
   Token name = tok->NextToken();

   Token peek = tok->PeekToken();

   int portStart = 0;
   int portEnd = 0;
   if(CompareToken(peek,":")){
      tok->AdvancePeek(peek);
      Token portToken = tok->NextToken();
      portStart = ParseInt(portToken);

      peek = tok->PeekToken();
      if(CompareToken(peek,".")){
         tok->AdvancePeek(peek);
         portToken = tok->NextToken();

         portEnd = ParseInt(portToken);
      } else {
         portEnd = portStart;
      }
   }

   Var var = {};

   StoreToken(name,var.name.str);
   var.portStart = portStart;
   var.portEnd = portEnd;

   return var;
}

PortInstance ParseTerm(Versat* versat,Accelerator* circuit,Tokenizer* tok,HierarchyName* circuitName){
   Token peek = tok->PeekToken();
   int negate = 0;
   if(CompareToken(peek,"~")){
      tok->AdvancePeek(peek);
      negate = 1;
   }

   Var var = ParseVar(tok);

   FUInstance* inst = GetInstance(circuit,{var.name.str});
   PortInstance res = {};

   Assert(var.portStart == var.portEnd);

   if(negate){
      FUInstance* negation = CreateShallowNamedFUInstance(circuit,GetTypeByName(versat,MakeSizedString("NOT")),MakeSizedString("not"),circuitName);

      ConnectUnits(inst,var.portStart,negation,0);

      res.inst = negation;
      res.port = 0;
   } else {
      res.inst = inst;
      res.port = var.portStart;
   }

   return res;
}

FUInstance* ParseExpression(Versat* versat,Accelerator* circuit,Tokenizer* tok,HierarchyName* circuitName){
   PortInstance term1 = ParseTerm(versat,circuit,tok,circuitName);
   Token op = tok->PeekToken();

   if(CompareToken(op,";")){
      return term1.inst;
   }

   tok->AdvancePeek(op);
   PortInstance term2 = ParseTerm(versat,circuit,tok,circuitName);

   const char* typeName;
   if(CompareToken(op,"&")){
      typeName = "AND";
   } else if(CompareToken(op,"|")){
      typeName = "OR";
   } else if(CompareToken(op,"^")){
      typeName = "XOR";
   } else if(CompareToken(op,">>>")){
      typeName = "RHR";
   } else if(CompareToken(op,">>")){
      typeName = "SHR";
   } else if(CompareToken(op,"<<<")){
      typeName = "RHL";
   } else if(CompareToken(op,"<<")){
      typeName = "SHL";
   } else if(CompareToken(op,"+")){
      typeName = "ADD";
   } else {
      printf("%s\n",op.str);
      fflush(stdout);
      Assert(0);
   }

   SizedString typeStr = MakeSizedString(typeName);
   FUInstance* res = CreateShallowNamedFUInstance(circuit,GetTypeByName(versat,typeStr),typeStr,circuitName);

   ConnectUnits(term1.inst,term1.port,res,0);
   ConnectUnits(term2.inst,term2.port,res,1);

   return res;
}

void TagInputs(Accelerator* accel,FUInstance* inst){
   if(inst->tag || inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
      return;
   }

   inst->tag = 1;

   for(int i = 0; i < inst->tempData->numberInputs; i++){
      TagInputs(accel,inst->tempData->inputs[i].inst.inst);
   }
}

// Checks if there is a connection between source and sink, that doesn't pass through a source and sink unit
bool ExistsConnection(Accelerator* accel, FUInstance* source, FUInstance* sink){
   LockAccelerator(accel);

   for(FUInstance* inst : accel->instances){
      inst->tag = 0;
   }

   TagInputs(accel,sink);

   return (bool) source->tag;
}

FUDeclaration* ParseModule(Versat* versat,Tokenizer* tok){
   tok->AssertNextToken("module");

   Token name = tok->NextToken();
   tok->AssertNextToken("(");

   FUDeclaration* circuitInput = GetTypeByName(versat,MakeSizedString("circuitInput"));
   FUDeclaration* circuitOutput = GetTypeByName(versat,MakeSizedString("circuitOutput"));

   Accelerator* circuit = CreateAccelerator(versat);

   FUDeclaration decl = {};
   decl.type = FUDeclaration::COMPOSITE;
   decl.circuit = circuit;
   StoreToken(name,decl.name.str);
   Assert(name.size < MAX_NAME_SIZE);

   for(int i = 0; 1; i++){
      Token token = tok->NextToken();

      if(CompareToken(token,",")){
         continue;
      }

      if(CompareToken(token,")")){
         break;
      }

      FUInstance* inst = CreateShallowNamedFUInstance(circuit,circuitInput,token,&decl.name);
      FUInstance** ptr = circuit->inputInstancePointers.Alloc();

      *ptr = inst;
      decl.nInputs += 1;
   }

   tok->AssertNextToken("{");

   int state = 0;
   while(!tok->Done()){
      Token token = tok->PeekToken();

      if(CompareToken(token,"}")){
         tok->AdvancePeek(token);
         break;
      }

      if(CompareToken(token,"#")){
         tok->AdvancePeek(token);
         state = 1;
      }

      if(state == 0){
         Token type = tok->NextToken();
         Token name = tok->NextToken();

         FUDeclaration* FUType = GetTypeByName(versat,type);
         FUInstance* inst = CreateShallowNamedFUInstance(circuit,FUType,name,&decl.name);

         tok->AssertNextToken(";");
      } else {
         Var outVar = ParseVar(tok);
#if 1
         Token peek = tok->NextToken();
         if(CompareToken(peek,"=")){
            FUInstance* inst = ParseExpression(versat,circuit,tok,&decl.name);

            strcpy(inst->name.str,outVar.name.str);

            if(strcmp(outVar.name.str,"out") == 0){
               Assert(0);
            }

            tok->AssertNextToken(";");
         } else if(CompareToken(peek,"->")){
            Var inVar = ParseVar(tok);

            int outRange = outVar.portEnd - outVar.portStart + 1;
            int inRange = inVar.portEnd - inVar.portStart + 1;

            Assert(outRange == inRange || outRange == 1);

            FUInstance* inst1 = GetInstance(circuit,{outVar.name.str});
            FUInstance* inst2 = nullptr;
            #if 1
            if(CompareString(inVar.name.str,"out")){
               decl.nOutputs = maxi(decl.nOutputs - 1,inVar.portEnd) + 1;

               if(!circuit->outputInstance){
                  circuit->outputInstance = CreateShallowNamedFUInstance(circuit,circuitOutput,MakeSizedString("out"),&decl.name);
               }

               inst2 = circuit->outputInstance;
            } else {
               inst2 = GetInstance(circuit,{inVar.name.str});
            }

            if(outRange == 1){
               for(int i = 0; i < inRange; i++){
                  ConnectUnits(inst1,outVar.portStart,inst2,inVar.portStart + i);
               }
            } else {
               for(int i = 0; i < inRange; i++){
                  ConnectUnits(inst1,outVar.portStart + i,inst2,inVar.portStart + i);
               }
            }
            #endif

            tok->AssertNextToken(";");
         } else {
            printf("%s\n",peek.str);
            fflush(stdout);
            Assert(0);
         }
#endif
      }
   }

   CalculateDelay(versat,circuit);

   if(circuit->outputInstance){
      decl.latency = circuit->cyclesPerRun;
   }

   {
   FILE* dotFile = fopen("circuit.dot","w");
   OutputGraphDotFile(circuit,dotFile,1);
   fclose(dotFile);
   }

   for(FUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      decl.nConfigs += d->nConfigs;
      decl.nStates += d->nStates;
      decl.memoryMapBytes += AlignNextPower2(d->memoryMapBytes); // Order of entities affect size, need to look into it
      decl.nDelays += d->nDelays;
      decl.extraDataSize += d->extraDataSize;
   }

   decl.configWires = (Wire*) calloc(decl.nConfigs,sizeof(Wire));
   decl.stateWires  = (Wire*) calloc(decl.nStates,sizeof(Wire));

   int configIndex = 0;
   int stateIndex = 0;
   for(FUInstance* inst : circuit->instances){
      FUDeclaration* d = inst->declaration;

      for(int i = 0; i < d->nConfigs; i++){
         decl.configWires[configIndex++] = d->configWires[i];
      }
      for(int i = 0; i < d->nStates; i++){
         decl.stateWires[stateIndex++] = d->stateWires[i];
      }
   }

   #if 0
   if(CompareToken(name,"SHA")){
      for(FUInstance* inst : circuit->instances){
         printf("%s:%d\n",inst->name.str,inst->delay);
      }
      FlushStdout();
   }
   #endif

   #if 0
   bool computeUnitType = (bool) circuit->inputInstancePointers.Size();
   for(FUInstance** instPtr : circuit->inputInstancePointers){
      computeUnitType &= ExistsConnection(circuit,*instPtr,circuit->outputInstance);
   }
   #endif

   // TODO: Change unit delay type inference. Only care about delay type to upper levels.
   // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
   #if 1
   bool hasSourceDelay = false;
   bool hasSinkDelay = false;
   #endif
   bool implementsDone = false;

   #if 0
   if(!computeUnitType){
      if(decl.nInputs && decl.nOutputs){
         decl.delayType = (DelayType) (DelayType::DELAY_TYPE_SINK_DELAY | DelayType::DELAY_TYPE_SOURCE_DELAY);
      } else if(decl.nInputs) {
         decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
      } else if(decl.nOutputs){
         decl.delayType = DelayType::DELAY_TYPE_SOURCE_DELAY;
      }
   }
   #endif

   LockAccelerator(circuit);
   for(FUInstance* inst : circuit->instances){
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      }

      if(CHECK_DELAY(inst,DelayType::DELAY_TYPE_IMPLEMENTS_DONE)){
         implementsDone = true;
      }
      #if 1
      if(inst->tempData->nodeType == GraphComputedData::TAG_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
      }
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE){
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      #endif
   }

   #if 1
   if(hasSourceDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SOURCE_DELAY);
   }
   if (hasSinkDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SINK_DELAY);
   }
   #endif

   if(implementsDone){
      decl.delayType = (DelayType) ((int)decl.delayType | (int)DelayType::DELAY_TYPE_IMPLEMENTS_DONE);
   }

   FUDeclaration* res = RegisterFU(versat,decl);

   #if 1
   {
   char buffer[256];
   sprintf(buffer,"src/%s.v",decl.name.str);
   FILE* sourceCode = fopen(buffer,"w");
   OutputCircuitSource(versat,decl,circuit,sourceCode);
   fclose(sourceCode);
   }
   #endif

   return res;
}

void ParseVersatSpecification(Versat* versat,FILE* file){
   char* buffer = (char*) calloc(256*1024*1024,sizeof(char)); // Calloc to fix valgrind error
   int fileSize = fread(buffer,sizeof(char),256*1024*1024,file);

   Tokenizer tokenizer = Tokenizer(buffer,fileSize, "[](){}+:;,*~.",{"->",">>>","<<<",">>","<<"});
   Tokenizer* tok = &tokenizer;

   while(!tok->Done()){
      ParseModule(versat,tok);
   }

   free(buffer);
}





































