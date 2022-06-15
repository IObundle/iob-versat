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

struct Command{
   char name[32];

   Expression** expressions;
   int nExpressions;
};

struct Block{
   union{
      Token textBlock;
      Command command;
   };
   Block* next;
   Block* nextInner;
   enum {TEXT,COMMAND} type;
};

// Static variables
static std::map<SizedString,Value,CompareFunction> envTable;
static FILE* output;
static Arena* tempArena;

static Expression* ParseExpression(Tokenizer* tok);

// Crude parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok){
   Token firstId = tok->NextToken();

   current->type = Expression::IDENTIFIER;
   current->id = firstId;

   while(1){
      Token token = tok->PeekToken();

      if(CompareString(token,"[")){
         tok->AdvancePeek(token);
         Expression* expr = PushStruct(tempArena,Expression);
         expr->expressions = PushArray(tempArena,2,Expression*);

         expr->type = Expression::ARRAY_ACCESS;
         expr->size = 2;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseExpression(tok);

         tok->AssertNextToken("]");

         current = expr;
      } else if(CompareString(token,".")){
         tok->AdvancePeek(token);
         Token memberName = tok->NextToken();

         Expression* expr = PushStruct(tempArena,Expression);
         expr->expressions = PushArray(tempArena,1,Expression*);

         expr->type = Expression::MEMBER_ACCESS;
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
   Expression* expr = PushStruct(tempArena,Expression);

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
      Token str = tok->NextFindUntil("\"");
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

static Expression* ParseExpression(Tokenizer* tok){
   Expression* res = ParseOperationType(tok,{{"|>"},{"=="},{"+","-"},{"*","/","&","**"}},ParseFactor,tempArena);

   return res;
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
   } else if(CompareToken(name,"debug")){
      com.nExpressions = 1;
   } else {
      Assert(false);
   }

   com.expressions = PushArray(tempArena,com.nExpressions,Expression*);
   for(int i = 0; i < com.nExpressions; i++){
      com.expressions[i] = ParseExpression(tok);
   }

   return com;
}

static Block* Parse(Tokenizer* tok){
   Block* block = PushStruct(tempArena,Block);

   Token token = tok->PeekToken();
   if(CompareString(token,"#{")){
      block->type = Block::COMMAND;

      tok->AdvancePeek(token);
      block->command = ParseCommand(tok);
      tok->AssertNextToken("}");

      if(!IsCommandBlockType(block->command)){
         return block;
      }

      Block* ptr = block;
      while(1){
         Block* child = Parse(tok);
         if(child->type == Block::COMMAND && CompareString(child->command.name,"end")){
            break;
         } else {
            if(block->nextInner == nullptr){
               block->nextInner = child;
               ptr = child;
            } else {
               Assert(!child->next);

               ptr->next = child;
               ptr = child;
            }
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
      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         Print(ptr,level + 1);
      }
   } else {
      printf("Text Size: %d\n",block->textBlock.size);
   }
}

static Value HexValue(Value in){
   Assert(in.type == ValueType::NUMBER);

   int number = in.number;

   Value res = {};
   res.type = ValueType::STRING;
   res.str.str = res.smallBuffer;
   res.str.size = sprintf(res.smallBuffer,"0x%x",number);

   return res;
}

static Value EvalExpression(Expression* expr){
   switch(expr->type){
      case Expression::OPERATION:{
         if(expr->op[0] == '|'){ // Pipe operation
            Value val = EvalExpression(expr->expressions[0]);

            if(CompareString(expr->expressions[1]->id,"Hex")){
               val = HexValue(val);
            } else if(CompareString(expr->expressions[1]->id,"GetHierarchyName")){
               Assert(val.type == ValueType::CUSTOM);

               HierarchyName* name = (HierarchyName*) val.custom;
               char* hier = GetHierarchyNameRepr(*name);

               SizedString buffer = PushString(tempArena,MakeSizedString(hier));

               val.type = ValueType::STRING;
               val.str = buffer;
            } else if(CompareString(expr->expressions[1]->id,"String")){
               Assert(val.type == ValueType::STRING);
               val.type = ValueType::STRING_LITERAL;
            } else {
               Assert(false);
            }

            return val;
         }

         Value val = {};
         val.type = ValueType::NUMBER;

         Value op1 = EvalExpression(expr->expressions[0]);
         Value op2 = EvalExpression(expr->expressions[1]);

         if(expr->op[0] != '='){
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

         switch (expr->op[0]){
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
   }

   return MakeValue();
}

void PrintValue(FILE* file,Value val){
   if(val.type >= (int) ValueType::CUSTOM){
      TypeInfo* info = val.customType.baseType;

      if(val.customType.pointers){
         fprintf(file,"%d",(int) val.custom); // Print all pointers as a number
      } else {
         if(CompareString("SizedString",info->name)){ // TODO: Hackish
            SizedString* ss = (SizedString*) val.custom;

            fprintf(file,"%.*s",ss->size,ss->str);
            return;
         }

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

void Eval(Block* block){
   if(block->type == Block::COMMAND){
      Command com = block->command;

      if(CompareString(com.name,"join")){
         Value separator = EvalExpression(com.expressions[0]);

         Assert(separator.type == ValueType::STRING);


         Assert(com.expressions[2]->type == Expression::IDENTIFIER);
         SizedString id = com.expressions[2]->id;

         Value iterating = EvalExpression(com.expressions[3]);
         bool seenFirst = false;
         for(Iterator iter = Iterate(iterating); HasNext(iter); Advance(&iter)){
            Value val = GetValue(iter);
            envTable[id] = val;

            if(seenFirst) {
               fprintf(output,"%.*s",separator.str.size,separator.str.str);
            } else {
               seenFirst = true;
            }

            for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
               Eval(ptr); // Push on stack
            }
         }

         envTable[MakeSizedString("join")] = MakeValue(false);
      } else if(CompareString(com.name,"for")){
         Assert(com.expressions[0]->type == Expression::IDENTIFIER);
         SizedString id = com.expressions[0]->id;

         Value iterating = EvalExpression(com.expressions[1]);
         for(Iterator iter = Iterate(iterating); HasNext(iter); Advance(&iter)){
            Value val = GetValue(iter);
            envTable[id] = val;

            for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
               Eval(ptr); // Push on stack
            }
         }
      } else if (CompareString(com.name,"if")){
         Value val = ConvertValue(EvalExpression(com.expressions[0]),ValueType::BOOLEAN);

         if(val.boolean){
            for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
               if(ptr->type == Block::COMMAND && strcmp(ptr->command.name,"else") == 0){
                  break;
               }
               Eval(ptr);
            }
         } else {
            for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
               if(ptr->type == Block::COMMAND && strcmp(ptr->command.name,"else") == 0){
                  for(ptr = ptr->next; ptr != nullptr; ptr = ptr->next){
                     Eval(ptr);
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
      } else if(CompareString(com.name,"debug")){
         Value val = EvalExpression(com.expressions[0]);

         if(val.boolean){
            DebugSignal();
         }

         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            Eval(ptr);
         }
      } else {
         Assert(false);
      }
   } else {
      // Print text
      Tokenizer tok(block->textBlock,"[]()*.-/}",{"@{","==","**","|>"});

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
         Expression* expr = ParseExpression(&tok);
         tok.AssertNextToken("}");

         Value val = EvalExpression(expr);

         PrintValue(output,val);
      }
   }
}

void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* arena){
   tempArena = arena;

   SizedString content = PushFile(tempArena,templateFilepath);

   Tokenizer tokenizer(content,"()[]{}+-:;.,*~\"",{"#{","==","**"});
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

   Byte* mark = MarkArena(arena);
   while(!tok->Done()){
      Block* block = Parse(tok);

      Eval(block);
      fflush(output);
   }
   PopMark(arena,mark);

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

void TemplateSetBool(const char* id,bool boolean){
   Value val = {};

   val.type = ValueType::BOOLEAN;
   val.boolean = boolean;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetArray(const char* id,const char* baseType,void* array,int size){
   Value val = {};

   val.customType = GetType(baseType);
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
