#include "templateEngine.hpp"

#include "stdlib.h"
#include "stdio.h"

#include <map>
#include <cstring>
#include <functional>

#include "utils.hpp"
#include "type.hpp"

struct CompareFunction : public std::binary_function<SizedString, SizedString, bool> {
public:
   bool operator() (SizedString str1, SizedString str2) const{
      for(int i = 0; i < mini(str1.size,str2.size); i++){
         if(str1.str[i] < str2.str[i]){
            return true;
         }
         if(str1.str[i] > str2.str[i]){
            return false;
         }
      }

      return (str1.size < str2.size);
   }
};

#define MAX_ARGUMENTS 32

struct Command{
   char name[32];

   Expression* expressions[MAX_ARGUMENTS];
   int nExpressions;
};

struct Block{
   union{
      Token textBlock;
      Command command;
   };
   Block* innerBlocks[MAX_ARGUMENTS];
   int numberInnerBlocks;
   enum {TEXT,COMMAND,COUNT} type;
};

// Static variables
static std::map<SizedString,Value,CompareFunction> envTable;
static FILE* output;
static Block blockBuffer[1024*1024];
static int blockSize = 0;
static Expression expressionBuffer[1024*1024];
static int expressionSize = 0;

static void ResetVariables(){
   // TODO: Some code depends on this clearing process (depends on buffer being cleared)
   //       Maybe change buffer allocations for a vector or a pool
   #if 1
   for(int i = 0; i < blockSize; i++){
      blockBuffer[i] = (Block){};
   }
   for(int i = 0; i < expressionSize; i++){
      expressionBuffer[i] = (Expression){};
   }
   #endif

   output = 0;
   blockSize = 0;
   expressionSize = 0;
}

static Expression* ParseExpression(Tokenizer* tok);

// Crude LR parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok){
   Token firstId = tok->NextToken();

   current->type = Expression::IDENTIFIER;
   current->id = firstId;

   while(1){
      Token token = tok->PeekToken();

      if(CompareString(token,"[")){
         tok->AdvancePeek(token);
         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::ARRAY_ACCESS;
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseExpression(tok);

         tok->AssertNextToken("]");

         current = expr;
      } else if(CompareString(token,".")){
         tok->AdvancePeek(token);
         Token memberName = tok->NextToken();

         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::MEMBER_ACCESS;
         expr->id = memberName;
         expr->size = 1;
         expr->expressions[0] = current;

         current = expr;
      } else if(CompareString(token,"->")){
         tok->AdvancePeek(token);
         Token memberName = tok->NextToken();

         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::POINTER_ACCESS;
         expr->id = memberName;
         expr->size = 1;
         expr->expressions[0] = current;

         current = expr;
      } else {
         break;
      }
   }

   return current;
}

static Expression* ParseAtom(Tokenizer* tok){
   Expression* expr = &expressionBuffer[expressionSize++];

   Token token = tok->PeekToken();

   expr->type = Expression::LITERAL;
   if(token.str[0] >= '0' && token.str[0] <= '9'){
      tok->AdvancePeek(token);
      int num = 0;

      for(int i = 0; i < token.size; i++){
         num *= 10;
         num += token.str[i] - '0';
      }

      expr->val.type = ValueType::NUMBER;
      expr->val.number = num;
   } else if(token.str[0] == '\"'){
      tok->AdvancePeek(token);
      Token str = tok->PeekFindUntil("\"");
      tok->AdvancePeek(str);
      tok->AssertNextToken("\"");

      expr->val.str = str;
      expr->val.type = ValueType::STRING;
   } else {
      expr->type = Expression::IDENTIFIER;
      expr = ParseIdentifier(expr,tok);
   }

   return expr;
}

static Expression* ParseFactor(Tokenizer* tok){
   Token peek = tok->PeekToken();

   if(CompareString(peek,"(")){
      tok->AdvancePeek(peek);
      Expression* expr = ParseExpression(tok);
      tok->AssertNextToken(")");
      return expr;
   } else {
      Expression* expr = ParseAtom(tok);
      return expr;
   }
}

static Expression* ParseTerm(Tokenizer* tok){
   Expression* current = ParseFactor(tok);

   while(1){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"*") || CompareString(peek,"/") || CompareString(peek,"&") || CompareString(peek,"**")){
         tok->AdvancePeek(peek);
         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::OPERATION;
         if(CompareString(peek,"**")){
            expr->op = 'p';
         } else {
            expr->op = peek.str[0];
         }
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseTerm(tok);

         current = expr;
      } else {
         break;
      }
   }

   return current;
}

static Expression* ParseAddition(Tokenizer* tok){
   Expression* current = ParseTerm(tok);

   while(1){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"+") || CompareString(peek,"-")){
         tok->AdvancePeek(peek);
         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::OPERATION;
         expr->op = peek.str[0];
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseTerm(tok);

         current = expr;
      } else {
         break;
      }
   }

   return current;
}

static Expression* ParseExpression(Tokenizer* tok){
   Expression* current = ParseAddition(tok);

   while(1){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"==")){
         tok->AdvancePeek(peek);
         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::OPERATION;
         expr->op = peek.str[0];
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseAddition(tok);

         current = expr;
      } else {
         break;
      }
   }

   return current;
}

static bool IsCommandBlockType(Command com){
   if(CompareString(com.name,"set")){
      return false;
   } else if(CompareString(com.name,"end")){
      return false;
   } else if(CompareString(com.name,"inc")){
      return false;
   } else if(CompareString(com.name,"else")){
      return false;
   } else {
      return true;
   }
}

static Command ParseCommand(Tokenizer* tok){
   // TODO: parse command
   Command com = {};

   Token name = tok->NextToken();
   StoreToken(name,com.name);

   if(CompareToken(name,"join")){
      com.nExpressions = 4;
   } else if(CompareToken(name,"for")){
      com.nExpressions = 2;
   } else if(CompareToken(name,"if")){
      com.nExpressions = 1;
   } else if(CompareToken(name,"end")){
      com.nExpressions = 0;
   } else if(CompareToken(name,"set")){
      com.nExpressions = 2;
   } else if(CompareToken(name,"inc")){
      com.nExpressions = 1;
   } else if(CompareToken(name,"else")){
      com.nExpressions = 0;
   } else {
      Assert(false);
   }

   for(int i = 0; i < com.nExpressions; i++){
      com.expressions[i] = ParseExpression(tok);
   }

   return com;
}

static Block* Parse(Tokenizer* tok){
   Block* block = &blockBuffer[blockSize++];

   Token token = tok->PeekToken();

   if(CompareString(token,"#{")){
      block->type = Block::COMMAND;

      tok->AdvancePeek(token);
      block->command = ParseCommand(tok);
      tok->AssertNextToken("}");

      if(!IsCommandBlockType(block->command)){
         return block;
      }

      for(int i = 0; i < MAX_ARGUMENTS; i++){
         Block* child = Parse(tok);
         if(child->type == Block::COMMAND && CompareString(child->command.name,"end")){
            break;
         } else {
            block->innerBlocks[block->numberInnerBlocks++] = child;
         }
      }
   } else {
      block->type = Block::TEXT;

      block->textBlock = tok->PeekFindUntil("#{");

      if(block->textBlock.size == -1){
         block->textBlock = tok->Finish();
      }
      tok->AdvancePeek(block->textBlock);
   }

   return block;
}

static void Print(Block* block, int level = 0){
   #define PRINT_LEVEL() \
   for(int i = 0; i < level*2; i++){ \
      printf(" "); \
   }

   if(block == nullptr){
      return;
   }

   PRINT_LEVEL();
   if(block->type == Block::COMMAND){
      printf("Command  : %s\n",block->command.name);
      for(int i = 0; i < MAX_ARGUMENTS; i++){
         Print(block->innerBlocks[i],level + 1);
      }
   } else {
      printf("Text Size: %d\n",block->textBlock.size);
   }
}

static Value HexValue(Value in){
   Assert(in.type == ValueType::NUMBER);

   int number = in.number;

   char* mem = (char*) calloc(16,sizeof(char)); // TODO: Will leak, but only a small hack, for now

   Value res = {};
   res.type = ValueType::STRING;
   res.str.str = mem;
   res.str.size = sprintf(mem,"0x%x",number);

   return res;
}

static Value EvalExpression(Expression* expr){
   switch(expr->type){
      case Expression::OPERATION:{
         if(expr->op == '|'){ // Pipe operation
            Value val = EvalExpression(expr->expressions[0]);

            if(CompareString(expr->expressions[1]->id,"Hex")){
               val = HexValue(val);
            } else if(CompareString(expr->expressions[1]->id,"GetHierarchyName")){
               Assert(val.type == ValueType::CUSTOM);

               HierarchyName* name = (HierarchyName*) val.custom;

               char* hier = GetHierarchyNameRepr(*name);
               int size = strlen(hier);

               char* buffer = (char*) calloc(size + 1,sizeof(char)); // TODO: will leak
               strcpy(buffer,hier);

               val.type = ValueType::STRING;
               val.str.str = buffer;
               val.str.size = size;

            } else {
               Assert(false);
            }

            return val;
         }

         Value val = {};
         val.type = ValueType::NUMBER;

         Value op1 = EvalExpression(expr->expressions[0]);
         Value op2 = EvalExpression(expr->expressions[1]);

         if(expr->op != '='){
            if(op1.customType.pointers){
               op1.type = ValueType::NUMBER;

               int view = *((int*) op1.custom);
               op1.number = view;
            }
            if(op2.customType.pointers){
               op2.type = ValueType::NUMBER;
               int view = *((int*) op2.custom);
               op2.number = view;
            }

            Assert(op1.type == ValueType::NUMBER && op2.type == ValueType::NUMBER);
         }

         int val1 = op1.number;
         int val2 = op2.number;

         switch (expr->op){
            case '+':{
               val.number = val1 + val2;
            }break;
            case '-':{
               val.number = val1 - val2;
            }break;
            case '*':{
               val.number = val1 * val2;
            }break;
            case '/':{
               val.number = val1 / val2;
            }break;
            case '=':{
               val.boolean = EqualValues(op1,op2);
            }break;
            case '&':{
               val.number = val1 & val2;
            }break;
            case 'p':{
               val.number = val1;

               for(int i = 0; i < val2; i++){
                  val.number *= val1;
               }
            } break;
            default:{
               DebugSignal();
            }break;
         }

         return val;
      }break;
      case Expression::LITERAL:{
         return expr->val;
      } break;
      case Expression::IDENTIFIER:{
         Value entry = envTable[expr->id];

         return entry;
      } break;
      case Expression::ARRAY_ACCESS:{
         Value object = EvalExpression(expr->expressions[0]);
         Value index  = EvalExpression(expr->expressions[1]);

         Assert(index.type == ValueType::NUMBER);

         Value res = AccessObjectIndex(object,index.number);

         return res;
      } break;
      case Expression::MEMBER_ACCESS:{
         Value object = EvalExpression(expr->expressions[0]);

         Value res = AccessObject(object,expr->id);

         return res;
      }break;
      case Expression::POINTER_ACCESS:{
         Value object = EvalExpression(expr->expressions[0]);

         Value res = AccessObjectPointer(object,expr->id);

         return res;
      }break;
   }

   return MakeValue();
}

void PrintValue(FILE* file,Value val){
   if(val.type >= (int) ValueType::CUSTOM){
      TypeInfo* info = GetTypeInfo(val.customType);

      if(val.customType.pointers){
         fprintf(file,"%d",(int) val.custom); // Print all pointers as a number
      } else {
         DebugSignal();
      }

      return;
   }

   switch(val.type){
   case ValueType::NUMBER:{
      fprintf(file,"%d",val.number);
   }break;
   case ValueType::STRING:{
      fprintf(file,"%.*s",val.str.size,val.str.str);
   }break;
   case ValueType::ARRAY:{
      fprintf(file,"<%d>",(int) val.array);
   }break;
   case ValueType::STRING_LITERAL:{
      fprintf(file,"\"%.*s\"",val.str.size,val.str.str);
   }break;
   case ValueType::CHAR:{
      fprintf(file,"%c",val.ch);
   }break;
   default:{
      DebugSignal();
   }break;
   }
}

static Expression* ParseExpressionReplacement(Tokenizer* tok){
   Expression* current = ParseExpression(tok);

   while(1){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"|")){
         tok->AdvancePeek(peek);
         Expression* expr = &expressionBuffer[expressionSize++];

         expr->type = Expression::OPERATION;
         expr->op = '|'; // Not or, but pipe
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseTerm(tok);

         current = expr;
      } else {
         break;
      }
   }

   return current;
}

void Eval(Block* block){
   if(block->type == Block::COMMAND){
      Command com = block->command;

      if(CompareString(com.name,"join")){
         Value separator = EvalExpression(com.expressions[0]);

         Assert(separator.type == ValueType::STRING);

         bool seenFirst = false;

         // TODO: ALMOST EXACT COPY OF FOR, HACK FOR NOW, CHANGE LATER
         Assert(com.expressions[2]->type == Expression::IDENTIFIER);
         SizedString id = com.expressions[2]->id;

         Value iterating = EvalExpression(com.expressions[3]);

         if(iterating.type == ValueType::NUMBER){
            for(Value i = MakeValue(0); i.number < iterating.number; i.number++){
               envTable[id] = i;

               if(seenFirst) {
                  fprintf(output,"%.*s",separator.str.size,separator.str.str);
               } else {
                  seenFirst = true;
               }

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
               i = envTable[id];
            }
         } else if(iterating.type == ValueType::POOL){ // TODO: currently hardcoded to Pool<FUInstance> since it's only use case
            for(FUInstance* inst : *iterating.pool){
               Value val = {};

               val.type = ValueType::CUSTOM;
               val.customType = GetType("FUInstance");
               val.custom = inst;

               envTable[id] = val;

               if(seenFirst) {
                  fprintf(output,"%.*s",separator.str.size,separator.str.str);
               } else {
                  seenFirst = true;
               }

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else if(iterating.type == ValueType::VECTOR){
            for(FUInstance* inst : *iterating.vec){
               Value val = {};

               val.type = ValueType::CUSTOM;
               val.customType = GetType("FUInstance");
               val.custom = inst;

               envTable[id] = val;

               if(seenFirst) {
                  fprintf(output,"%.*s",separator.str.size,separator.str.str);
               } else {
                  seenFirst = true;
               }

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else if(iterating.type == ValueType::ARRAY){
            for(int i = 0; i < iterating.size; i++){
               Value val = MakeValue(iterating.array[i]);
               envTable[id] = val;

               if(seenFirst) {
                  fprintf(output,"%.*s",separator.str.size,separator.str.str);
               } else {
                  seenFirst = true;
               }

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else {
            Assert(false);
         }
         // END OF COPY OF FOR

         envTable[MakeSizedString("join")] = MakeValue(false);
      } else if(CompareString(com.name,"for")){
         Assert(com.expressions[0]->type == Expression::IDENTIFIER);
         SizedString id = com.expressions[0]->id;

         Value iterating = EvalExpression(com.expressions[1]);

         if(iterating.type == ValueType::NUMBER){
            for(Value i = MakeValue(0); i.number < iterating.number; i.number++){
               envTable[id] = i;
               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
               i = envTable[id];
            }
         } else if(iterating.type == ValueType::POOL){ // TODO: currently hardcoded to Pool<FUInstance> since it's only use case
            for(FUInstance* inst : *iterating.pool){
               Value val = {};

               val.type = ValueType::CUSTOM;
               val.customType = GetType("FUInstance");
               val.custom = inst;

               envTable[id] = val;

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else if(iterating.type == ValueType::VECTOR){
               for(FUInstance* inst : *iterating.vec){
               Value val = {};

               val.type = ValueType::CUSTOM;
               val.customType = GetType("FUInstance");
               val.custom = inst;

               envTable[id] = val;

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else if(iterating.type == ValueType::ARRAY){
            for(int i = 0; i < iterating.size; i++){
               Value val = MakeValue(iterating.array[i]);
               envTable[id] = val;

               for(int ii = 0; ii < block->numberInnerBlocks; ii++){
                  Eval(block->innerBlocks[ii]);
               }
            }
         } else {
            Assert(false);
         }
      } else if (CompareString(com.name,"if")){
         Value val = EvalExpression(com.expressions[0]);

         if(val.boolean){
            for(int ii = 0; ii < block->numberInnerBlocks; ii++){
               if(block->innerBlocks[ii]->type == Block::COMMAND && strcmp(block->innerBlocks[ii]->command.name,"else") == 0){
                  break;
               }
               Eval(block->innerBlocks[ii]);
            }
         } else {
            int index;
            for(index = 0; index < block->numberInnerBlocks; index++){
               if(block->innerBlocks[index]->type == Block::COMMAND && strcmp(block->innerBlocks[index]->command.name,"else") == 0){
                  for(int ii = index + 1; ii < block->numberInnerBlocks; ii++){
                     Eval(block->innerBlocks[ii]);
                  }
                  break;
               }
            }
         }
      } else if(CompareString(com.name,"set")){
         Value val = EvalExpression(com.expressions[1]);

         Assert(com.expressions[0]->type == Expression::IDENTIFIER);

         envTable[com.expressions[0]->id] = val;
      } else if(CompareString(com.name,"inc")){
         Value val = EvalExpression(com.expressions[0]);

         Assert(val.type == ValueType::NUMBER);

         val.number += 1;

         envTable[com.expressions[0]->id] = val;
      } else {
         Assert(false);
      }
   } else {
      // Print text
      Tokenizer tok(block->textBlock.str,block->textBlock.size,"[]()*.-/}",{"@{","->","==","**"});

      while(1){
         Token text = tok.PeekFindUntil("@{");

         if(text.size == -1){
            text = tok.Finish();
         }
         tok.AdvancePeek(text);

         fprintf(output,"%.*s",text.size,text.str);

         if(tok.Done()){
            break;
         }

         tok.AssertNextToken("@{");
         Expression* expr = ParseExpressionReplacement(&tok);
         tok.AssertNextToken("}");

         Value val = EvalExpression(expr);

         PrintValue(output,val);
      }
   }
}

void ProcessTemplate(FILE* outputFile,const char* templateFilepath){
   ResetVariables();

   FILE* file = fopen(templateFilepath,"r");

   if(!file){
      printf("Error opening template: %s\n",templateFilepath);
      return;
   }

   char* buffer = (char*) calloc(256*1024*1024,sizeof(char)); // Calloc to fix valgrind error
   int fileSize = fread(buffer,sizeof(char),256*1024*1024,file);
   fclose(file);

   buffer[fileSize] = '\0';
   Tokenizer tokenizer(buffer,fileSize,"()[]{}+-:;.,*~\"",{"#{","->","==","**"});
   Tokenizer* tok = &tokenizer;

   #if 0
      output = stdout;
   #endif
   #if 0
      output = fopen("/dev/null","w");
   #endif
   #if 1
      output = outputFile;
   #endif

   while(!tok->Done()){
      Block* block = Parse(tok);

      Eval(block);
      fflush(output);
   }

   free(buffer);
   envTable.clear();
}

void TemplateSetCustom(const char* id,void* entity,const char* typeName){
   Value val = {};

   val.type = ValueType::CUSTOM;
   val.customType = GetType(typeName);
   val.custom = entity;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetNumber(const char* id,int number){
   Value val = {};

   val.type = ValueType::NUMBER;
   val.number = number;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetString(const char* id,const char* str){
   Value val = {};

   val.type = ValueType::STRING;
   val.str = MakeSizedString(str);

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetArray(const char* id,int* array,int size){
   Value val = {};

   val.type = ValueType::ARRAY;
   val.array = array;
   val.size = size;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetInstancePool(const char* id,Pool<FUInstance>* pool){
   Value val = {};

   val.type = ValueType::POOL;
   val.pool = pool;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetInstanceVector(const char* id,std::vector<FUInstance*>* vec){
   Value val = {};

   val.type = ValueType::VECTOR;
   val.vec = vec;

   envTable[MakeSizedString(id)] = val;
}
