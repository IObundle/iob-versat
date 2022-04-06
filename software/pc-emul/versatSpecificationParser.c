#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "stdint.h"

#include "versat.h"
#include "parser.h"

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static char buffer[1024 * 1024];
static int charBufferSize;

static int StringCmp(const char* str1,const char* str2){
   if(str1 == NULL || str2 == NULL){
      if(str1 == NULL && str2 == NULL){
         return 1;
      } else {
         return 0;
      }
   }

   while(*str1 != '\0' && *str2 != '\0'){
      if(*str1 != *str2){
         return 0;
      }

      str1++;
      str2++;
   }

   return (*str1 == *str2);
}

static char* PushString(const char* str,int size){
   char* res = &buffer[charBufferSize];

   for(int i = 0; i < size & *str != '\0'; i++){
      buffer[charBufferSize++] = *(str++);
   }

   buffer[charBufferSize++] = '\0';

   return res;
}

static int IsWhitespace(char ch){
   if(ch == '\n' || ch == ' ' || ch == '\t'){
      return 1;
   }

   return 0;
}

static void ConsumeWhitespace(Tokenizer* tok){
   while(1){
      if(tok->index >= tok->size)
         return;

      char ch = tok->content[tok->index];

      if(ch == '\n'){
         tok->index += 1;
         tok->lines += 1;
         continue;
      }

      if(IsWhitespace(ch)){
         tok->index += 1;
         continue;
      }

      return;
   }
   return;
}

static int SingleChar(char ch){
   char options[] = "[](){}+:;,*~.";

   for(int i = 0; options[i] != '\0'; i++){
      if(options[i] == ch){
         return 1;
      }
   }

   return 0;
}

static int CompareStringNoEnd(const char* strNoEnd,const char* strWithEnd){
   for(int i = 0; strWithEnd[i] != '\0'; i++){
      if(strNoEnd[i] != strWithEnd[i]){
         return 0;
      }
   }

   return 1;
}

static int SpecialChars(Tokenizer* tok,Token* out){
   const char* special[] = {"->",">>>","<<<",">>","<<"};

   for(int i = 0; i < 1; i++){
      if(CompareStringNoEnd(&tok->content[tok->index],special[i])){
         int size = strlen(special[i]);

         memcpy(out->str,special[i],size);
         out->size = size;

         return 1;
      }
   }

   return 0;
}

static Token PeekToken(Tokenizer* tok){
   Token token = {};

   while(1){
      ConsumeWhitespace(tok);

      if(tok->index + 1 >= tok->size){
         break;
      }

      if(tok->content[tok->index] == '/' && tok->content[tok->index + 1] == '/'){
         while(tok->index + 1 < tok->size && tok->content[tok->index] != '\n') tok->index += 1;
         tok->index += 1;
         continue;
      }

      if(tok->content[tok->index] == '/' && tok->content[tok->index + 1] == '*'){
         while(tok->index + 1 < tok->size &&
               !(tok->content[tok->index] == '*' && tok->content[tok->index + 1] == '/')) tok->index += 1;
         tok->index += 2;
         continue;
      }

      break;
   }

   if(SpecialChars(tok,&token)){
      return token;
   }

   char ch = tok->content[tok->index];

   token.str[0] = ch;
   token.size = 1;

   if(SingleChar(ch)){
      return token;
   }

   if(ch == '\"'){ // Special case for strings
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         token.str[i] = ch;
         token.size = (i + 1);

         if(ch == '\"'){
            token.str[token.size] = '\0';
            return token;
         }
      }
   } else {
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         if(SingleChar(ch) || IsWhitespace(ch)){
            token.str[token.size] = '\0';
            return token;
         }

         token.str[i] = ch;
         token.size = (i + 1);
      }
   }

   token.str[token.size] = '\0';

   return token;
}

static Token NextToken(Tokenizer* tok){
   Token res = PeekToken(tok);

   tok->index += res.size;

   #if 0
   printf("%s\n",res.str);
   #endif

   return res;
};

static void StoreToken(Token token,char* buffer){
   memcpy(buffer,token.str,token.size);
   buffer[token.size] = '\0';
}

static int CompareToken(Token tok,const char* str){
   for(int i = 0; i < tok.size; i++){
      if(tok.str[i] != str[i]){
         return 0;
      }
   }

   return (str[tok.size] == '\0');
}

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

static int ParseInt(char* name){
   int res = 0;
   for(int i = 0; name[i] != '\0'; i++){
      Assert(name[i] >= '0' && name[i] <= '9');

      res *= 10;
      res += name[i] - '0';
   }

   return res;
}

typedef struct Var_t{
   HierarchyName name;
   int portStart;
   int portEnd;
} Var;

static Var inputInstances[64];
static int numberInputInstances;

Var ParseVar(Tokenizer* tokenizer){
   Token name = NextToken(tokenizer);

   Token peek = PeekToken(tokenizer);

   int portStart = 0;
   int portEnd = 0;
   if(CompareToken(peek,":")){
      NextToken(tokenizer);
      Token portToken = NextToken(tokenizer);
      portStart = ParseInt(portToken.str);

      peek = PeekToken(tokenizer);
      if(CompareToken(peek,".")){
         NextToken(tokenizer);
         portToken = NextToken(tokenizer);

         portEnd = ParseInt(portToken.str);
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

PortInstance ParseTerm(Versat* versat,Accelerator* circuit,Tokenizer* tokenizer,HierarchyName* circuitName){
   Token peek = PeekToken(tokenizer);
   int negate = 0;
   if(CompareToken(peek,"~")){
      NextToken(tokenizer);
      negate = 1;
   }

   Var var = ParseVar(tokenizer);

   FUInstance* inst = GetInstanceByName(circuit,var.name.str);
   PortInstance res = {};

   Assert(var.portStart == var.portEnd);

   if(negate){
      FUInstance* negation = CreateNamedFUInstance(circuit,GetTypeByName(versat,"NOT"),"not",circuitName);

      ConnectUnits(inst,var.portStart,negation,0);

      res.inst = negation;
      res.port = 0;
   } else {
      res.inst = inst;
      res.port = var.portStart;
   }

   return res;
}

FUInstance* ParseExpression(Versat* versat,Accelerator* circuit,Tokenizer* tokenizer,HierarchyName* circuitName){
   PortInstance term1 = ParseTerm(versat,circuit,tokenizer,circuitName);
   Token op = PeekToken(tokenizer);

   if(CompareToken(op,";")){
      return term1.inst;
   }

   NextToken(tokenizer);
   PortInstance term2 = ParseTerm(versat,circuit,tokenizer,circuitName);

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

   FUInstance* res = CreateNamedFUInstance(circuit,GetTypeByName(versat,typeName),typeName,circuitName);

   ConnectUnits(term1.inst,term1.port,res,0);
   ConnectUnits(term2.inst,term2.port,res,1);

   return res;
}

FUDeclaration* ParseModule(Versat* versat,Tokenizer* tokenizer){
#if MARK
   numberInputInstances = 0;

   Assert(CompareToken(NextToken(tokenizer),"module"));
   Token name = NextToken(tokenizer);
   Assert(CompareToken(NextToken(tokenizer),"("));
   Token token = PeekToken(tokenizer);

   FUDeclaration* circuitInput = GetTypeByName(versat,"circuitInput");
   FUDeclaration* circuitOutput = GetTypeByName(versat,"circuitOutput");

   Accelerator* circuit = CreateAccelerator(versat);

   FUDeclaration decl = {};
   decl.type = FUDeclaration::COMPOSITE;
   decl.circuit = circuit;
   StoreToken(name,decl.name.str);
   Assert(name.size < MAX_NAME_SIZE);

   for(int i = 0; 1; i++){
      token = NextToken(tokenizer);

      if(CompareToken(token,",")){
         continue;
      }

      if(CompareToken(token,")")){
         break;
      }

      FUInstance* inst = CreateNamedFUInstance(circuit,circuitInput,token.str,&decl.name);

      StoreToken(token,inputInstances[numberInputInstances++].name.str);

      FUInstance** ptr = Alloc(circuit->inputInstancePointers,FUInstance*);
      *ptr = inst;
      decl.nInputs += 1;
   }

   Assert(CompareToken(NextToken(tokenizer),"{"));

   decl.delayType = ParseInt(NextToken(tokenizer).str);

   int state = 0;
   while(tokenizer->index < tokenizer->size){
      Token token = PeekToken(tokenizer);

      if(CompareToken(token,"}")){
         NextToken(tokenizer);
         break;
      }

      if(CompareToken(token,"#")){
         NextToken(tokenizer);
         state = 1;
      }

      if(state == 0){
         Token type = NextToken(tokenizer);
         Token name = NextToken(tokenizer);

         FUDeclaration* FUType = GetTypeByName(versat,type.str);
         FUInstance* inst = CreateNamedFUInstance(circuit,FUType,name.str,&decl.name);

         Assert(CompareToken(NextToken(tokenizer),";"));
      } else {
         Var outVar = ParseVar(tokenizer);
#if 1
         Token peek = NextToken(tokenizer);
         if(CompareToken(peek,"=")){
            FUInstance* inst = ParseExpression(versat,circuit,tokenizer,&decl.name);

            strcpy(inst->name.str,outVar.name.str);

            if(strcmp(outVar.name.str,"out") == 0){
               Assert(0);
            }

            Assert(CompareToken(NextToken(tokenizer),";"));
         } else if(CompareToken(peek,"->")){
            Var inVar = ParseVar(tokenizer);

            int outRange = outVar.portEnd - outVar.portStart + 1;
            int inRange = inVar.portEnd - inVar.portStart + 1;

            Assert(outRange == inRange || outRange == 1);

            FUInstance* inst1 = GetInstanceByName(circuit,outVar.name);
            FUInstance* inst2 = NULL;
            #if 1
            if(strcmp(inVar.name.str,"out") == 0){
               decl.nOutputs = maxi(decl.nOutputs - 1,inVar.portEnd) + 1;

               if(!circuit->outputInstance){
                  circuit->outputInstance = CreateNamedFUInstance(circuit,circuitOutput,"out",&decl.name);
               }

               inst2 = circuit->outputInstance;
            } else {
               inst2 = GetInstanceByName(circuit,inVar.name);
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

            Assert(CompareToken(NextToken(tokenizer),";"));
         } else {
            printf("%s\n",peek.str);
            fflush(stdout);
            Assert(0);
         }
#endif
      }
   }

   void* mem = CalculateGraphData(circuit);

#if 0
   ForEach(PortInstance,inst,circuit->outputPortInstances){
      decl.latency = maxi(decl.latency,CalculateLatency(inst->inst,false));
   }
#endif

   free(mem);

   {
   FILE* dotFile = fopen("circuit.dot","w");
   OutputGraphDotFile(circuit,dotFile,1);
   fclose(dotFile);
   }

   return RegisterFU(versat,decl);
#endif
   return NULL;
}

void ParseVersatSpecification(Versat* versat,FILE* file){
   char* buffer = (char*) calloc(256*1024*1024,sizeof(char)); // Calloc to fix valgrind error
   int fileSize = fread(buffer,sizeof(char),256*1024*1024,file);

   Tokenizer tokenizerInst = {};
   Tokenizer* tokenizer = &tokenizerInst;

   tokenizer->content = buffer;
   tokenizer->size = fileSize;

   while(tokenizer->index < tokenizer->size){
      ParseModule(versat,tokenizer);
      PeekToken(tokenizer);
   }

   free(buffer);
}





































