#include "templateEngine.hpp"

#include "stdlib.h"
#include "stdio.h"

#include <map>
#include <cstring>
#include <functional>

#include "utils.hpp"
#include "type.hpp"

void ParseAndEvaluate(SizedString content);

static bool debugging = false;

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

// Static variables
static std::map<SizedString,Value,CompareFunction> envTable;
static FILE* output;
static Arena* tempArena;
static const char* filepath;

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
   Token peek = tok->PeekToken();

   if(CompareString(peek,"!")){
      tok->AdvancePeek(peek);

      Expression* child = ParseExpression(tok);

      Expression* expr = PushStruct(tempArena,Expression);
      expr->expressions = PushArray(tempArena,1,Expression*);

      expr->type = Expression::OPERATION;
      expr->op = "!";
      expr->size = 1;
      expr->expressions[0] = child;

      return expr;
   }

   Expression* res = ParseOperationType(tok,{{"|>"},{"and","or","xor"},{">","<",">=","<=","=="},{"+","-"},{"*","/","&","**"}},ParseFactor,tempArena);

   return res;
}

static bool IsCommandBlockType(Command com){
   static const char* notBlocks[] = {"set","end","inc","else","include","call"};

   for(unsigned int i = 0; i < ARRAY_SIZE(notBlocks); i++){
      if(CompareString(com.name,notBlocks[i])){
         return false;
      }
   }

   return true;
}

static Command ParseCommand(Tokenizer* tok){
   Command com = {};

   Token name = tok->NextToken();
   StoreToken(name,com.name);

   struct {const char* name;int nExpressions;} commands[] = {{"join",4},
                                                            {"for",2},
                                                            {"if",1},
                                                            {"end",0},
                                                            {"set",2},
                                                            {"inc",1},
                                                            {"else",0},
                                                            {"debug",1},
                                                            {"include",1},
                                                            {"define",-1},
                                                            {"call",-1}};

   bool found = false;

   for(unsigned int i = 0; i < ARRAY_SIZE(commands); i++){
      auto command = commands[i];

      if(CompareString(name,command.name)){
         com.nExpressions = command.nExpressions;
         found = true;
         break;
      }
   }
   Assert(found);

   if(com.nExpressions == -1){
      Token peek = tok->PeekFindUntil("}");

      Tokenizer arguments(peek,"",{});

      com.nExpressions = 0;
      while(!arguments.Done()){
         arguments.NextToken();
         com.nExpressions += 1;
      }
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
   int number = ConvertValue(in,ValueType::NUMBER).number;

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
               Assert(val.type == GetType("HierarchyName"));

               HierarchyName* name = (HierarchyName*) val.custom;
               char* hier = GetHierarchyNameRepr(*name);

               SizedString buffer = PushString(tempArena,MakeSizedString(hier));

               val.type = ValueType::STRING;
               val.str = buffer;
            } else if(CompareString(expr->expressions[1]->id,"String")){
               Assert(val.type == ValueType::STRING);
               val.literal = true;
            } else {
               Assert(false);
            }

            return val;
         }

         if(CompareString(expr->op,"!")){
            Value op = EvalExpression(expr->expressions[0]);
            bool val = ConvertValue(op,ValueType::BOOLEAN).boolean;

            return MakeValue(!val);
         }

         // Two or more op operations
         Value op1 = EvalExpression(expr->expressions[0]);
         Value op2 = EvalExpression(expr->expressions[1]);

         if(CompareString(expr->op,"==")){
            if(EqualValues(op1,op2)){
               return MakeValue(true);
            } else {
               return MakeValue(false);
            }
         } else if(CompareString(expr->op,"and")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN).boolean;
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN).boolean;

            return MakeValue(bool1 && bool2);
         } else if(CompareString(expr->op,"or")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN).boolean;
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN).boolean;

            return MakeValue(bool1 || bool2);
         } else if(CompareString(expr->op,"xor")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN).boolean;
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN).boolean;

            return MakeValue((bool1 && !bool2) || (!bool1 && bool2));
         }

         Value val = {};
         val.type = ValueType::NUMBER;

         int val1 = ConvertValue(op1,ValueType::NUMBER).number;
         int val2 = ConvertValue(op2,ValueType::NUMBER).number;

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
            case '&':{
               val.number = val1 & val2;
            }break;
            case '>':{
               if(expr->op[1] == '='){
                  val.boolean = (val1 >= val2);
               } else {
                  val.boolean = (val1 > val2);
               }
            }break;
            case '<':{
               if(expr->op[1] == '='){
                  val.boolean = (val1 <= val2);
               } else {
                  val.boolean = (val1 < val2);
               }
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
         auto iter = envTable.find(expr->id);

         Assert(iter != envTable.end());

         return iter->second;
      } break;
      case Expression::ARRAY_ACCESS:{
         Value object = EvalExpression(expr->expressions[0]);
         Value index  = EvalExpression(expr->expressions[1]);

         Assert(index.type == ValueType::NUMBER);

         if(debugging){
            Wire* wires = (Wire*) object.custom;

            printf("%p %d\n",wires,index.number);
            FlushStdout();
         }

         Value res = AccessObjectIndex(object,index.number);

         if(debugging){
            Wire* wire = (Wire*) res.custom;

            printf("%s %d\n",wire->name,wire->bitsize);
         }

         return res;
      } break;
      case Expression::MEMBER_ACCESS:{
         Value object = EvalExpression(expr->expressions[0]);

         Value res = AccessObject(object,expr->id);

         return res;
      }break;
      default:{
         Assert(false);
      } break;
   }

   return MakeValue();
}

static void PrintValue(FILE* file,Value in){
   Value val = CollapseArrayIntoPtr(in);

   if(val.type == ValueType::NUMBER){
      fprintf(file,"%d",val.number);
   } else if(val.type == ValueType::STRING){
      if(val.literal){
         fprintf(file,"\"");
      }
      fprintf(file,"%.*s",val.str.size,val.str.str);
      if(val.literal){
         fprintf(file,"\"");
      }
   } else if(val.type == ValueType::CHAR){
      fprintf(file,"%c",val.ch);
   } else if(val.type == ValueType::SIZED_STRING){
      fprintf(file,"%.*s",val.str.size,val.str.str);
   } else {
      Assert(false);
   }
}

static void Eval(Block* block){
   if(block->type == Block::COMMAND){
      Command com = block->command;

      if(CompareString(com.name,"join")){
         Value separator = EvalExpression(com.expressions[0]);

         Assert(separator.type == ValueType::STRING);

         Assert(com.expressions[2]->type == Expression::IDENTIFIER);
         SizedString id = com.expressions[2]->id;

         Value iterating = EvalExpression(com.expressions[3]);
         bool seenFirst = false;
         int counter = 0;
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

            counter += 1;
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
            debugging = true;
         }

         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            Eval(ptr);
         }

         debugging = false;
      } else if(CompareString(com.name,"include")){
         char buffer[4096];

         Value filenameString = EvalExpression(com.expressions[0]);
         Assert(filenameString.type == ValueType::STRING);

         strcpy(buffer,filepath);
         SizedString path = PathGoUp(buffer);

         SizedString filename = filenameString.str;
         sprintf(&buffer[path.size],"/%.*s",filename.size,filename.str);

         SizedString content = PushFile(tempArena,buffer);
         Assert(content.size >= 0);

         ParseAndEvaluate(content);
      } else if(CompareString(com.name,"define")) {
         SizedString id = com.expressions[0]->id;

         TemplateFunction* func = PushStruct(tempArena,TemplateFunction);

         func->arguments = &com.expressions[1];
         func->numberArguments = com.nExpressions - 1;
         func->block = block->nextInner;

         Value val = {};
         val.templateFunction = func;
         val.type = ValueType::TEMPLATE_FUNCTION;
         val.isTemp = true;

         envTable[id] = val;
      } else if(CompareString(com.name,"call")){
         SizedString id = com.expressions[0]->id;

         auto iter = envTable.find(id);
         if(iter == envTable.end()){
            printf("Failed to find %.*s\n",UNPACK_SS(id));
            Assert(false);
         }

         auto savedTable = envTable;

         TemplateFunction* func = iter->second.templateFunction;

         Assert(func->numberArguments == com.nExpressions - 1);

         for(int i = 0; i < func->numberArguments; i++){
            SizedString id = func->arguments[i]->id;

            Value val = EvalExpression(com.expressions[1+i]);

            envTable[id] = val;
         }

         #if 1
         for(Block* ptr = func->block; ptr != nullptr; ptr = ptr->next){
            Eval(ptr);
         }
         #endif

         envTable = savedTable;
      } else {
         Assert(false);
      }
   } else {
      // Print text
      Tokenizer tok(block->textBlock,"!()[]{}+-:;.,*~><\"",{"@{","==","**","|>",">=","<="});

      tok.keepComments = true;

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

void ParseAndEvaluate(SizedString content){
   Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","==","**",">=","<="});
   Tokenizer* tok = &tokenizer;

   tok->keepComments = true;

   while(!tok->Done()){
      Block* block = Parse(tok);

      Eval(block);
      fflush(output);
   }
}

void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* arena){
   tempArena = arena;
   filepath = templateFilepath;

   #if 0
      output = stdout;
   #endif
   #if 0
      output = fopen("/dev/null","w");
   #endif
   #if 1
      output = outputFile;
   #endif

   SizedString content = PushFile(tempArena,templateFilepath);

   Byte* mark = MarkArena(tempArena);
   ParseAndEvaluate(content);
   PopMark(tempArena,mark);

   envTable.clear();
}

void TemplateSetCustom(const char* id,void* entity,const char* typeName){
   Value val = {};

   val.type = GetType(typeName);
   val.custom = entity;
   val.isTemp = false;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetNumber(const char* id,int number){
   envTable[MakeSizedString(id)] = MakeValue(number);
}

void TemplateSetString(const char* id,const char* str){
   envTable[MakeSizedString(id)] = MakeValue(str);
}

void TemplateSetBool(const char* id,bool boolean){
   envTable[MakeSizedString(id)] = MakeValue(boolean);
}

void TemplateSetArray(const char* id,const char* baseType,void* array,int size){
   Value val = {};

   val.type = GetType(baseType,false,true);
   val.array = array;
   val.size = size;
   val.isTemp = true;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetInstancePool(const char* id,Pool<FUInstance>* pool){
   Value val = {};

   val.type = ValueType::POOL;
   val.pool = pool;
   val.isTemp = true;

   envTable[MakeSizedString(id)] = val;
}

void TemplateSetInstanceVector(const char* id,std::vector<FUInstance*>* vec){
   Value val = {};

   Type* FUInstancePointer = GetType("FUInstance",true,false);

   val.type = GetType(FUInstancePointer,false,true);
   val.array = vec->data();
   val.size = vec->size();
   val.isTemp = true;

   envTable[MakeSizedString(id)] = val;
}
