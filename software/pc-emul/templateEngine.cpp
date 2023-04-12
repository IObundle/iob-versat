#include "templateEngine.hpp"

#include "stdlib.h"
#include "stdio.h"

#include <map>
#include <cstring>

#include "utils.hpp"
#include "type.hpp"
#include "debug.hpp"
#include "versatPrivate.hpp"

struct ValueAndText{
   Value val;
   String text;
};

struct CompareFunction : public std::binary_function<String, String, bool> {
public:
   bool operator() (String str1, String str2) const{
      for(int i = 0; i < std::min(str1.size,str2.size); i++){
         if(str1[i] < str2[i]){
            return true;
         }
         if(str1[i] > str2[i]){
            return false;
         }
      }

      return (str1.size < str2.size);
   }
};

void ParseAndEvaluate(String content);

static Expression* ParseExpression(Tokenizer* tok);
static String Eval(Block* block);
static ValueAndText EvalNonBlockCommand(Command* com);

// Static variables
static bool debugging = false;
static std::map<String,Value,CompareFunction> envTable;
static FILE* output;
static Arena* tempArena;
static Arena* outputArena;
static const char* filepath;

static bool IsCommandBlockType(Command* com){
   static const char* notBlocks[] = {"set","end","inc","else","include","call","return","format","debugBreak"};

   for(unsigned int i = 0; i < ARRAY_SIZE(notBlocks); i++){
      if(CompareString(com->name,notBlocks[i])){
         return false;
      }
   }

   return true;
}

static Command* ParseCommand(Tokenizer* tok){
   Command* com = PushStruct<Command>(tempArena);

   com->name = tok->NextToken();

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
                                                            {"call",-1},
                                                            {"while",1},
                                                            {"return",1},
                                                            {"format",-1},
                                                            {"debugMessage",0},
                                                            {"debugBreak",0}};

   bool found = false;
   int commandSize = -1;
   for(unsigned int i = 0; i < ARRAY_SIZE(commands); i++){
      auto command = commands[i];

      if(CompareString(com->name,command.name)){
         commandSize = command.nExpressions;
         found = true;
         break;
      }
   }
   Assert(found);

   if(commandSize == -1){
      Token peek = tok->PeekUntilDelimiterExpression({"{","@{","#{"},{"}"},1);

      Tokenizer arguments(peek,"}",{"@{","#{"});

      commandSize = 0;
      while(!arguments.Done()){
         Token token = arguments.NextToken();

         if(CompareString(token,"@{")){
            Token insidePeek = arguments.PeekUntilDelimiterExpression({"@{"},{"}"},1);
            arguments.AdvancePeek(insidePeek);
            arguments.AssertNextToken("}");
         } else if(CompareString(token,"#{")){
            Token insidePeek = arguments.PeekUntilDelimiterExpression({"#{"},{"}"},1);
            arguments.AdvancePeek(insidePeek);
            arguments.AssertNextToken("}");
         }

         commandSize += 1;
      }
   }

   com->expressions = PushArray<Expression*>(tempArena,commandSize);
   for(int i = 0; i < com->expressions.size; i++){
      com->expressions[i] = ParseExpression(tok);
   }

   return com;
}

// Crude parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok){
   void* start = tok->Mark();
   Token firstId = tok->NextToken();

   current->type = Expression::IDENTIFIER;
   current->id = firstId;

   while(1){
      Token token = tok->PeekToken();

      if(CompareString(token,"[")){
         tok->AdvancePeek(token);
         Expression* expr = PushStruct<Expression>(tempArena);
         expr->expressions = PushArray<Expression*>(tempArena,2);

         expr->type = Expression::ARRAY_ACCESS;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseExpression(tok);

         tok->AssertNextToken("]");

         current = expr;
      } else if(CompareString(token,".")){
         tok->AdvancePeek(token);
         Token memberName = tok->NextToken();

         Expression* expr = PushStruct<Expression>(tempArena);
         expr->expressions = PushArray<Expression*>(tempArena,1);

         expr->type = Expression::MEMBER_ACCESS;
         expr->id = memberName;
         expr->expressions[0] = current;

         current = expr;
      } else {
         break;
      }
   }

   current->text = tok->Point(start);
   return current;
}

static Expression* ParseAtom(Tokenizer* tok,Arena* arena){
   void* start = tok->Mark();

   Expression* expr = PushStruct<Expression>(arena);
   expr->type = Expression::LITERAL;

   Token token = tok->PeekToken();
   if(token[0] >= '0' && token[0] <= '9'){
      tok->AdvancePeek(token);
      int num = 0;

      for(int i = 0; i < token.size; i++){
         num *= 10;
         num += token[i] - '0';
      }

      expr->val = MakeValue(num);
   } else if(token[0] == '\"'){
      tok->AdvancePeek(token);
      Token str = tok->NextFindUntil("\"");
      tok->AssertNextToken("\"");

      expr->val = MakeValue(str);
   } else if(CompareString(token,"@{")){
      tok->AdvancePeek(token);
      expr = ParseExpression(tok);
      tok->AssertNextToken("}");
   } else if(CompareString(token,"#{")){
      tok->AdvancePeek(token);

      expr->type = Expression::COMMAND;
      expr->command = ParseCommand(tok);

      Assert(!IsCommandBlockType(expr->command));
      tok->AssertNextToken("}");
   } else {
      expr->type = Expression::IDENTIFIER;
      expr = ParseIdentifier(expr,tok);
   }

   expr->text = tok->Point(start);
   return expr;
}

static Expression* ParseFactor(Tokenizer* tok,Arena* arena){
   void* start = tok->Mark();

   Token peek = tok->PeekToken();

   Expression* expr = nullptr;
   if(CompareString(peek,"(")){
      tok->AdvancePeek(peek);
      expr = ParseExpression(tok);
      tok->AssertNextToken(")");
   } else if(CompareString(peek,"!")){
      tok->AdvancePeek(peek);

      Expression* child = ParseExpression(tok);

      expr = PushStruct<Expression>(arena);
      expr->expressions = PushArray<Expression*>(arena,1);

      expr->type = Expression::OPERATION;
      expr->op = "!";
      expr->expressions[0] = child;

      expr->text = tok->Point(start);
   } else {
      expr = ParseAtom(tok,arena);
   }

   expr->text = tok->Point(start);
   return expr;
}

static Expression* ParseExpression(Tokenizer* tok){
   void* start = tok->Mark();

   #if 0
   Token peek = tok->PeekToken();
   if(CompareString(peek,"!")){
      tok->AdvancePeek(peek);

      Expression* child = ParseExpression(tok);

      Expression* expr = PushStruct(tempArena,Expression);
      expr->expressions = PushStruct(tempArena,Expression*);

      expr->type = Expression::OPERATION;
      expr->op = "!";
      expr->size = 1;
      expr->expressions[0] = child;

      expr->text = tok->Point(start);
      return expr;
   }
   #endif

   Expression* res = ParseOperationType(tok,{{"#"},{"|>"},{"and","or","xor"},{">","<",">=","<=","==","!="},{"+","-"},{"*","/","&","**"}},ParseFactor,tempArena);

   res->text = tok->Point(start);
   return res;
}

static Block* Parse(Tokenizer* tok){
   Block* block = PushStruct<Block>(tempArena);
   *block = {};

   void* start = tok->Mark();

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
         if(child->type == Block::COMMAND && CompareString(child->command->name,"end")){
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

   block->fullText = tok->Point(start);

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
      printf("Command  : %.*s\n",UNPACK_SS(block->command->name));
      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         Print(ptr,level + 1);
      }
   } else {
      printf("Text Size: %d\n",block->textBlock.size);
   }
}

static Value HexValue(Value in){
   static char buffer[128];

   int number = ConvertValue(in,ValueType::NUMBER,nullptr).number;

   int size = sprintf(buffer,"0x%x",number);

   Value res = {};
   res.type = ValueType::STRING;
   res.str = PushString(tempArena,String{buffer,size});

   return res;
}

static Value EscapeString(Value val){
   Assert(val.type == ValueType::SIZED_STRING || val.type == ValueType::STRING);
   Assert(val.isTemp);

   String str = val.str;

   String escaped = PushString(tempArena,str);
   char* view = (char*) escaped.data;

   for(int i = 0; i < escaped.size; i++){
      char ch = view[i];

      if(   (ch >= 'a' && ch <= 'z')
         || (ch >= 'A' && ch <= 'Z')
         || (ch >= '0' && ch <= '9')){
         continue;
      } else {
         view[i] = '_'; // Replace any foreign symbol with a underscore
      }
   }

   Value res = val;
   res.str = escaped;

   return res;
}

int CountNonOperationChilds(Accelerator* accel){
   if(accel == nullptr){
      return 0;
   }

   int count = 0;
   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->inst->declaration->type == FUDeclaration::COMPOSITE){
         count += CountNonOperationChilds(ptr->inst->declaration->fixedDelayCircuit);
      }

      if(!ptr->inst->declaration->isOperation && ptr->inst->declaration->type != FUDeclaration::SPECIAL){
         count += 1;
      }
   }

   return count;
}

static Value EvalExpression(Expression* expr){
   switch(expr->type){
      case Expression::OPERATION:{
         if(expr->op[0] == '|'){ // Pipe operation
            Value val = EvalExpression(expr->expressions[0]);

            if(CompareString(expr->expressions[1]->id,"Hex")){
               val = HexValue(val);
            } else if(CompareString(expr->expressions[1]->id,"String")){
               Assert(val.type == ValueType::STRING);
               val.literal = true;
            } else if(CompareString(expr->expressions[1]->id,"CountNonOperationChilds")){
               Accelerator* accel = *((Accelerator**) val.custom);

               val = MakeValue(CountNonOperationChilds(accel));
            } else if(CompareString(expr->expressions[1]->id,"Identify")){
               val = EscapeString(val);
            } else {
               NOT_IMPLEMENTED;
            }

            return val;
         }

         if(CompareString(expr->op,"!")){
            Value op = EvalExpression(expr->expressions[0]);
            bool val = ConvertValue(op,ValueType::BOOLEAN,nullptr).boolean;

            return MakeValue(!val);
         }

         // Two or more op operations
         Value op1 = EvalExpression(expr->expressions[0]);

         // Short circuit
         if(CompareString(expr->op,"and")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;

            if(!bool1){
               return MakeValue(false);
            }
         } else if(CompareString(expr->op,"or")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;

            if(bool1){
               return MakeValue(true);
            }
         }

         Value op2 = EvalExpression(expr->expressions[1]);

         if(CompareString(expr->op,"==")){
            return MakeValue(Equal(op1,op2));
         } else if(CompareString(expr->op,"!=")){
            return MakeValue(!Equal(op1,op2));
         }else if(CompareString(expr->op,"and")){
            // At this point bool1 is true
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            return MakeValue(bool2);
         } else if(CompareString(expr->op,"or")){
            // At this point bool1 is false
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            return MakeValue(bool2);
         } else if(CompareString(expr->op,"xor")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            return MakeValue((bool1 && !bool2) || (!bool1 && bool2));
         } else if(CompareString(expr->op,"#")){
            String first = ConvertValue(op1,ValueType::SIZED_STRING,tempArena).str;
            String second = ConvertValue(op2,ValueType::SIZED_STRING,tempArena).str;

            String res = PushString(tempArena,"%.*s%.*s",UNPACK_SS(first),UNPACK_SS(second));

            return MakeValue(res);
         }

         Value val = {};
         val.type = ValueType::NUMBER;

         int val1 = ConvertValue(op1,ValueType::NUMBER,nullptr).number;
         int val2 = ConvertValue(op2,ValueType::NUMBER,nullptr).number;

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
               NOT_IMPLEMENTED;
            }break;
         }

         return val;
      } break;
      case Expression::LITERAL:{
         return expr->val;
      } break;
      case Expression::COMMAND:{
         Command* com = expr->command;

         Assert(!IsCommandBlockType(com));

         Value result = EvalNonBlockCommand(com).val;

         return result;
      } break;
      case Expression::IDENTIFIER:{
         auto iter = envTable.find(expr->id);

         if(iter == envTable.end()){
            printf("Didn't find identifier: %.*s\n",UNPACK_SS(expr->id));
            printf("%.*s\n",UNPACK_SS(expr->text));
            DEBUG_BREAK();
         }

         return iter->second;
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

         Value res = AccessStruct(object,expr->id);

         return res;
      }break;
      default:{
         NOT_IMPLEMENTED;
      } break;
   }

   return MakeValue();
}

static String EvalBlockCommand(Block* block){
   Command* com = block->command;
   String res = {};
   res.data = (char*) MarkArena(outputArena);

   if(CompareString(com->name,"join")){
      Value separator = EvalExpression(com->expressions[0]);

      Assert(separator.type == ValueType::STRING);

      Assert(com->expressions[2]->type == Expression::IDENTIFIER);
      String id = com->expressions[2]->id;

      Value iterating = EvalExpression(com->expressions[3]);
      int counter = 0;
      int index = 0;
      for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
         envTable[STRING("index")] = MakeValue(index);

         Value val = GetValue(iter);
         envTable[id] = val;

         bool outputSeparator = false;
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            String text = Eval(ptr);

            if(!CheckStringOnlyWhitespace(text)){
               res.size += text.size; // Push on stack
               outputSeparator = true;
            }
         }

         if(outputSeparator){
            res.size += PushString(outputArena,"%.*s",separator.str.size,separator.str.data).size;
         }

         counter += 1;
      }
      if(counter == 0){
         return res;
      }

      outputArena->used -= separator.str.size;
      res.size -= separator.str.size;
   } else if(CompareString(com->name,"for")){
      Assert(com->expressions[0]->type == Expression::IDENTIFIER);
      String id = com->expressions[0]->id;

      Value iterating = EvalExpression(com->expressions[1]);
      int index = 0;
      for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
         envTable[STRING("index")] = MakeValue(index);

         Value val = GetValue(iter);
         envTable[id] = val;

         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            res.size += Eval(ptr).size; // Push on stack
         }
      }
   } else if (CompareString(com->name,"if")){
      Value val = ConvertValue(EvalExpression(com->expressions[0]),ValueType::BOOLEAN,nullptr);

      if(val.boolean){
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            if(ptr->type == Block::COMMAND && CompareString(ptr->command->name,"else")){
               break;
            }
            res.size += Eval(ptr).size; // Push on stack
         }
      } else {
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            if(ptr->type == Block::COMMAND && CompareString(ptr->command->name,"else")){
               for(ptr = ptr->next; ptr != nullptr; ptr = ptr->next){
                  res.size += Eval(ptr).size; // Push on stack
               }
               break;
            }
         }
      }
   } else if(CompareString(com->name,"debug")){
      Value val = EvalExpression(com->expressions[0]);

      if(val.boolean){
         DEBUG_BREAK();
         debugging = true;
      }

      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         res.size += Eval(ptr).size; // Push on stack
      }

      debugging = false;
   } else if(CompareString(com->name,"while")){
      while(ConvertValue(EvalExpression(com->expressions[0]),ValueType::BOOLEAN,nullptr).boolean){
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            res.size += Eval(ptr).size; // Push on stack
         }
      }
   } else if(CompareString(com->name,"define")) {
      String id = com->expressions[0]->id;

      TemplateFunction* func = PushStruct<TemplateFunction>(tempArena);

      func->arguments = &com->expressions[1];
      func->numberArguments = com->expressions.size - 1;
      func->block = block->nextInner;

      Value val = {};
      val.templateFunction = func;
      val.type = ValueType::TEMPLATE_FUNCTION;
      val.isTemp = true;

      envTable[id] = val;
   } else if(CompareString(com->name,"debugMessage")){
      ArenaMarker marker(outputArena);

      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         String res = Eval(ptr); // Push on stack
         printf("%.*s\n",UNPACK_SS(res));
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return res;
}

static ValueAndText EvalNonBlockCommand(Command* com){
   Value val = MakeValue();
   String text = {};
   text.data = (char*) MarkArena(outputArena);

   if(CompareString(com->name,"set")){
      val = EvalExpression(com->expressions[1]);

      Assert(com->expressions[0]->type == Expression::IDENTIFIER);

      envTable[com->expressions[0]->id] = val;
   } else if(CompareString(com->name,"inc")){
      val = EvalExpression(com->expressions[0]);

      Assert(val.type == ValueType::NUMBER);

      val.number += 1;

      envTable[com->expressions[0]->id] = val;
   } else if(CompareString(com->name,"include")){
      char buffer[4096];

      Value filenameString = EvalExpression(com->expressions[0]);
      Assert(filenameString.type == ValueType::STRING);

      strcpy(buffer,filepath);
      String path = PathGoUp(buffer);

      String filename = filenameString.str;
      sprintf(&buffer[path.size],"/%.*s",UNPACK_SS(filename));

      String content = PushFile(tempArena,buffer);
      Assert(content.size >= 0);

      ParseAndEvaluate(content);
   } else if(CompareString(com->name,"call")){
      String id = com->expressions[0]->id;

      auto iter = envTable.find(id);
      if(iter == envTable.end()){
         printf("Failed to find %.*s\n",UNPACK_SS(id));
         DEBUG_BREAK();
      }

      //auto savedTable = envTable;

      TemplateFunction* func = iter->second.templateFunction;

      Assert(func->numberArguments == com->expressions.size - 1);

      for(int i = 0; i < func->numberArguments; i++){
         String id = func->arguments[i]->id;

         Value val = EvalExpression(com->expressions[1+i]);

         envTable[id] = val;
      }

      for(Block* ptr = func->block; ptr != nullptr; ptr = ptr->next){
         text.size += Eval(ptr).size;
      }

      val = envTable[STRING("return")];

      //envTable = savedTable;
   } else if(CompareString(com->name,"return")){
      val = EvalExpression(com->expressions[0]);

      envTable[STRING("return")] = val;
   } else if(CompareString(com->name,"format")){
      Value formatExpr = EvalExpression(com->expressions[0]);

      Assert(formatExpr.type == ValueType::SIZED_STRING || formatExpr.type == ValueType::STRING);

      String format = formatExpr.str;

      Tokenizer tok(format,"{}",{});
      Byte* mark = MarkArena(outputArena);
      text.data = (char*) mark;

      while(!tok.Done()){
         Token simpleText = tok.PeekFindUntil("{");

         if(simpleText.size == -1){
            simpleText = tok.Finish();
         }

         tok.AdvancePeek(simpleText);

         text.size += PushString(outputArena,simpleText).size;

         if(tok.Done()){
            break;
         }

         tok.AssertNextToken("{");
         Token indexText = tok.NextToken();
         tok.AssertNextToken("}");

         int index = ParseInt(indexText);

         Assert(index < com->expressions.size + 1);

         Value val = ConvertValue(EvalExpression(com->expressions[index + 1]),ValueType::SIZED_STRING,tempArena);

         text.size += PushString(outputArena,val.str).size;
      }
   } else if(CompareString(com->name,"debugBreak")){
      DEBUG_BREAK();
   } else if(CompareString(com->name,"end")){
      printf("Error: founded a strain end in the template\n");
      Assert(false);
   } else {
      NOT_IMPLEMENTED;
   }

   ValueAndText res = {};
   res.val = val;
   res.text = text;

   return res;
}

static String Eval(Block* block){
   String res = {};
   res.data = (char*) MarkArena(outputArena);

   if(block->type == Block::COMMAND){
      if(IsCommandBlockType(block->command)){
         res = EvalBlockCommand(block);
      } else {
         res = EvalNonBlockCommand(block->command).text;
      }
   } else {
      // Print text
      Tokenizer tok(block->textBlock,"!()[]{}+-:;.,*~><\"",{"@{","#{","==","**","|>",">=","<=","!="});

      tok.keepComments = true;
      while(1){
         Token text = tok.PeekFindUntil("@{");

         if(text.size == -1){
            text = tok.Finish();
         }
         tok.AdvancePeek(text);

         //fprintf(output,"%.*s",text.size,text.str);

         res.size += PushString(outputArena,text).size;

         if(tok.Done()){
            break;
         }

         tok.AssertNextToken("@{");
         Expression* expr = ParseExpression(&tok);
         tok.AssertNextToken("}");

         Value val = EvalExpression(expr);

         res.size += GetDefaultValueRepresentation(val,outputArena).size;

         //PrintValue(output,val);
      }
   }

   Assert(res.size >= 0);
   return res;
}

void ParseAndEvaluate(String content){
   Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
   Tokenizer* tok = &tokenizer;

   tok->keepComments = true;

   while(!tok->Done()){
      Block* block = Parse(tok);

      String text = Eval(block);
      fprintf(output,"%.*s",text.size,text.data);
      fflush(output);
   }
}

CompiledTemplate* CompileTemplate(const char* templateFilepath,Arena* arena){
   Byte* mark = MarkArena(arena);

   CompiledTemplate* res = PushStruct<CompiledTemplate>(arena);

   const char* filepath = PushString(arena,"%s",templateFilepath).data;
   PushNullByte(arena);

   tempArena = arena;
   String content = PushFile(arena,templateFilepath);

   Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
   Tokenizer* tok = &tokenizer;
   tok->keepComments = true;

   Block* initial = Parse(tok);
   Block* ptr = initial;
   while(!tok->Done()){
      Assert(ptr->next == nullptr);
      ptr->next = Parse(tok);
      ptr = ptr->next;
   }

   String totalMemory = PointArena(arena,mark);
   res->filepath = filepath;
   res->blocks = initial;
   res->totalMemoryUsed = totalMemory.size;
   res->content = content;

   return res;
}

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate,Arena* arena){
   ArenaMarker marker(arena);
   tempArena = arena;

   Arena outputArenaInst = SubArena(arena,Megabyte(64));
   outputArena = &outputArenaInst;
   output = outputFile;
   filepath = compiledTemplate->filepath;

   for(Block* block = compiledTemplate->blocks; block; block = block->next){
      String text = Eval(block);
      fprintf(output,"%.*s",text.size,text.data);
      fflush(output);
   }

   envTable.clear();
}

#if 0
void ProcessTemplate(FILE* outputFile,const char* templateFilepath,Arena* arena){
   ArenaMarker marker(arena);
   tempArena = arena;

   filepath = templateFilepath;

   Arena outputArenaInst = SubArena(arena,Megabyte(64));
   outputArena = &outputArenaInst;

   #if 0
      output = stdout;
   #endif
   #if 0
      output = fopen("/dev/null","w");
   #endif
   #if 1
      output = outputFile;
   #endif

   String content = PushFile(tempArena,templateFilepath);

   ParseAndEvaluate(content);

   envTable.clear();
}
#endif

void TemplateSetCustom(const char* id,void* entity,const char* typeName){
   Value val = MakeValue(entity,typeName);

   envTable[STRING(id)] = val;
}

void TemplateSetNumber(const char* id,int number){
   envTable[STRING(id)] = MakeValue(number);
}

void TemplateSet(const char* id,void* ptr){
   Value val = {};
   val.custom = ptr;
   val.type = ValueType::NUMBER;
   envTable[STRING(id)] = val;
}

void TemplateSetString(const char* id,const char* str){
   envTable[STRING(id)] = MakeValue(STRING(str));
}

void TemplateSetBool(const char* id,bool boolean){
   envTable[STRING(id)] = MakeValue(boolean);
}

void TemplateSetArray(const char* id,const char* baseType,void* array,int size){
   Value val = {};

   val.type = GetArrayType(GetType(STRING(baseType)),size);
   val.custom = array;

   envTable[STRING(id)] = val;
}
