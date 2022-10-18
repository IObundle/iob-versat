#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "stdint.h"

#include "versatPrivate.hpp"
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
   SizedString name;
   int delayStart;
   int delayEnd;
   int portStart;
   int portEnd;
} Var;

Var ParseVar(Tokenizer* tok){
   Token name = tok->NextToken();

   Token peek = tok->PeekToken();

   int delayStart = 0;
   int delayEnd = 0;
   if(CompareToken(peek,"[")){
      tok->AdvancePeek(peek);

      Token delayToken = tok->NextToken();
      delayStart = ParseInt(delayToken);

      peek = tok->PeekToken();
      if(CompareToken(peek,"..")){
         tok->AdvancePeek(peek);
         Token delayEndToken = tok->NextToken();

         delayEnd = ParseInt(delayEndToken);
      } else {
         delayEnd = delayStart;
      }

      tok->AssertNextToken("]");
      peek = tok->PeekToken();
   }

   int portStart = 0;
   int portEnd = 0;
   if(CompareToken(peek,":")){
      tok->AdvancePeek(peek);
      Token portToken = tok->NextToken();
      portStart = ParseInt(portToken);

      peek = tok->PeekToken();
      if(CompareToken(peek,"..")){
         tok->AdvancePeek(peek);
         portToken = tok->NextToken();

         portEnd = ParseInt(portToken);
      } else {
         portEnd = portStart;
      }
   }

   Assert(delayEnd >= delayStart);
   Assert(portEnd >= portStart);

   Var var = {};

   var.name = name;
   var.delayStart = delayStart;
   var.delayEnd = delayEnd;
   var.portStart = portStart;
   var.portEnd = portEnd;

   return var;
}

PortInstance ParseTerm(Versat* versat,Accelerator* circuit,Tokenizer* tok){
   Token peek = tok->PeekToken();
   int negate = 0;
   if(CompareToken(peek,"~")){
      tok->AdvancePeek(peek);
      negate = 1;
   }

   Var var = ParseVar(tok);

   FUInstance* inst = GetInstanceByName(circuit,"%.*s",UNPACK_SS(var.name));
   PortInstance res = {};

   Assert(var.portStart == var.portEnd);

   if(negate){
      FUInstance* negation = CreateFUInstance(circuit,GetTypeByName(versat,MakeSizedString("NOT")),MakeSizedString("not"));

      ConnectUnits(inst,var.portStart,negation,0);

      res.inst = (ComplexFUInstance*) negation;
      res.port = 0;
   } else {
      res.inst = (ComplexFUInstance*) inst;
      res.port = var.portStart;
   }

   return res;
}

FUInstance* ParseExpression(Versat* versat,Accelerator* circuit,Tokenizer* tok,SizedString name){
   PortInstance term1 = ParseTerm(versat,circuit,tok);
   Token op = tok->PeekToken();

   if(CompareToken(op,";")){
      return term1.inst;
   }

   tok->AdvancePeek(op);
   PortInstance term2 = ParseTerm(versat,circuit,tok);

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
   FUInstance* res = CreateFUInstance(circuit,GetTypeByName(versat,typeStr),name);

   ConnectUnits(term1.inst,term1.port,res,0);
   ConnectUnits(term2.inst,term2.port,res,1);

   return res;
}

FUDeclaration* ParseModule(Versat* versat,Tokenizer* tok){
   tok->AssertNextToken("module");

   Token name = tok->NextToken();
   tok->AssertNextToken("(");

   Accelerator* circuit = CreateAccelerator(versat);
   circuit->type = Accelerator::CIRCUIT;

   for(int i = 0; 1; i++){
      Token token = tok->NextToken();

      if(CompareToken(token,")")){
         break;
      }

      Token peek = tok->PeekToken();
      if(!CompareToken(peek,")")){
         tok->AssertNextToken(",");
      }

      FUInstance* inst = CreateFUInstance(circuit,versat->input,token);
      ComplexFUInstance** ptr = circuit->inputInstancePointers.Alloc();

      *ptr = (ComplexFUInstance*) inst;
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
         Token possibleStatic = tok->PeekToken();

         bool isStatic = false;
         if(CompareString(possibleStatic,"static")){
            tok->AdvancePeek(possibleStatic);
            isStatic = true;
         }

         Token type = tok->NextToken();

         Token possibleParameters = tok->PeekToken();
         Token fullParameters = MakeSizedString("");
         if(CompareString(possibleParameters,"#")){
            int index = 0;

            void* mark = tok->Mark();
            while(1){
               token = tok->NextToken();

               if(CompareString(token,"(")){
                  index += 1;
               }
               if(CompareString(token,")")){
                  index -= 1;
                  if(index == 0){
                     break;
                  }
               }
            }
            fullParameters = tok->Point(mark);
         }

         Token name = tok->NextToken();

         FUDeclaration* FUType = GetTypeByName(versat,type);
         FUInstance* inst = CreateFUInstance(circuit,FUType,name);
         inst->isStatic = isStatic;
         inst->parameters = PushString(&versat->permanent,fullParameters);

         Token peek = tok->PeekToken();

         if(CompareString(peek,"(")){
            tok->AdvancePeek(peek);

            Token list = tok->NextFindUntil(")");
            int arguments = 1 + CountSubstring(list,MakeSizedString(","));
            Assert(arguments <= FUType->nConfigs);

            Tokenizer insideList(list,",",{});

            inst->config = PushArray(&versat->permanent,FUType->nConfigs,int);
            SetDefaultConfiguration(inst,inst->config,FUType->nConfigs);

            //inst->savedConfiguration = true;
            for(int i = 0; i < arguments; i++){
               Token arg = insideList.NextToken();

               inst->config[i] = ParseInt(arg);

               if(i != arguments - 1){
                  insideList.AssertNextToken(",");
               }
            }
            Assert(insideList.Done());

            tok->AssertNextToken(")");
            peek = tok->PeekToken();
         }

         if(CompareString(peek,"{")){
            tok->AdvancePeek(peek);

            Token list = tok->NextFindUntil("}");
            int arguments = 1 + CountSubstring(list,MakeSizedString(","));
            Assert(arguments <= (1 << FUType->memoryMapBits));

            Tokenizer insideList(list,",",{});

            inst->memMapped = PushArray(&versat->permanent,1 << FUType->memoryMapBits,int);
            //inst->savedMemory = true;
            for(int i = 0; i < arguments; i++){
               Token arg = insideList.NextToken();

               inst->memMapped[i] = ParseInt(arg);

               if(i != arguments - 1){
                  insideList.AssertNextToken(",");
               }
            }
            Assert(insideList.Done());

            tok->AssertNextToken("}");
            peek = tok->PeekToken();
         }

         tok->AssertNextToken(";");
      } else {
         Var outVar = ParseVar(tok);

         Token peek = tok->NextToken();
         if(CompareToken(peek,"=")){
            FUInstance* inst = ParseExpression(versat,circuit,tok,outVar.name);

            if(CompareString(outVar.name,"out")){
               USER_ERROR;
            }

            tok->AssertNextToken(";");
         } else if(CompareToken(peek,"->")){
            Var inVar = ParseVar(tok);

            int outRange = outVar.portEnd - outVar.portStart + 1;
            int delayRange = outVar.delayEnd - outVar.delayStart + 1;
            int inRange = inVar.portEnd - inVar.portStart + 1;

            if(delayRange != 1 && inRange != delayRange){
               UNHANDLED_ERROR;
            }

            Assert(outRange == inRange || outRange == 1);

            FUInstance* inst1 = GetInstanceByName(circuit,"%.*s",UNPACK_SS(outVar.name));
            FUInstance* inst2 = nullptr;
            #if 1
            if(CompareString(inVar.name,"out")){
               if(!circuit->outputInstance){
                  circuit->outputInstance = (ComplexFUInstance*) CreateFUInstance(circuit,versat->output,MakeSizedString("out"));
               }

               inst2 = circuit->outputInstance;
            } else {
               inst2 = GetInstanceByName(circuit,"%.*s",UNPACK_SS(inVar.name));
            }

            int delayDelta = (delayRange == 1 ? 0 : 1);
            if(outRange == 1){
               for(int i = 0; i < inRange; i++){
                  ConnectUnitsWithDelay(inst1,outVar.portStart,inst2,inVar.portStart + i,-(outVar.delayStart + delayDelta * i));
               }
            } else {
               for(int i = 0; i < inRange; i++){
                  ConnectUnitsWithDelay(inst1,outVar.portStart + i,inst2,inVar.portStart + i,-(outVar.delayStart + delayDelta * i));
               }
            }
            #endif

            tok->AssertNextToken(";");
         } else {
            printf("%.*s\n",peek.size,peek.str);
            fflush(stdout);
            Assert(0);
         }
      }
   }

   return RegisterSubUnit(versat,name,circuit);
}

void ParseVersatSpecification(Versat* versat,const char* filepath){
   Byte* mark = MarkArena(&versat->temp);
   SizedString content = PushFile(&versat->temp,filepath);

   if(content.size < 0){
      printf("Failed to open file, filepath: %s\n",filepath);
      DEBUG_BREAK;
   }

   Tokenizer tokenizer = Tokenizer(content, "#[](){}+:;,*~.",{"->",">>>","<<<",">>","<<",".."});
   Tokenizer* tok = &tokenizer;

   while(!tok->Done()){
      ParseModule(versat,tok);
   }

   PopMark(&versat->temp,mark);
}





































