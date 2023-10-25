#include "templateEngine.hpp"

#include "stdlib.h"
#include "stdio.h"

#include <cstring>
#include <unordered_map>

#include "utils.hpp"
#include "type.hpp"

//#define DEBUG_TEMPLATE_ENGINE

static String globalTemplateName = {}; // The current name that is being parsed / evaluated. For error reporting reasons. TODO: Do not like it, not a big deal for now. Only change if needed

struct ValueAndText{
   Value val;
   String text;
};

struct Frame{
   Hashmap<String,Value>* table;
   Frame* previousFrame;
};

static Optional<Value> GetValue(Frame* frame,String var){
   Frame* ptr = frame;

   while(ptr){
      Value* possible = ptr->table->Get(var);
      if(possible){
         return *possible;
      } else {
         ptr = ptr->previousFrame;
      }
   }
   return {};
}

static Value* ValueExists(Frame* frame,String id){
   Frame* ptr = frame;

   while(ptr){
      Value* possible = ptr->table->Get(id);
      if(possible){
         return possible;
      }
      ptr = ptr->previousFrame;
   }
   return nullptr;
}

static void SetValue(Frame* frame,String id,Value val){
   Value* possible = ValueExists(frame,id);
   if(possible){
      *possible = val;
   } else {
      frame->table->Insert(id,val);
   }
}

static Frame* CreateFrame(Frame* previous,Arena* arena){
   Frame* frame = PushStruct<Frame>(arena);
   frame->table = PushHashmap<String,Value>(arena,16); // Testing a fixed hashmap for now.
   frame->previousFrame = previous;
   return frame;
}

static void ParseAndEvaluate(String content,Frame* frame,Arena* temp);
static String Eval(Block* block,Frame* frame,Arena* temp);
static ValueAndText EvalNonBlockCommand(Command* com,Frame* frame,Arena* temp);
static Expression* ParseExpression(Tokenizer* tok,Arena* temp);

// Static variables
static bool debugging = false;
static Frame globalFrameInst = {};
static Frame* globalFrame = &globalFrameInst;
static FILE* output;
static Arena* outputArena;

static void FatalError(const char* reason){
   printf("Template: error on template (%.*s):\n",UNPACK_SS(globalTemplateName));
   printf("\t%s\n",reason);
   DEBUG_BREAK();
}

static bool IsCommandBlockType(Command* com){
   static const char* notBlocks[] = {"set","end","inc","else","include","call","return","format","debugBreak"};

   for(unsigned int i = 0; i < ARRAY_SIZE(notBlocks); i++){
      if(CompareString(com->name,notBlocks[i])){
         return false;
      }
   }

   return true;
}

static Command* ParseCommand(Tokenizer* tok,Arena* out){
   Command* com = PushStruct<Command>(out);

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
            Token insidePeek = arguments.PeekIncludingDelimiterExpression({"@{"},{"}"},1);
            arguments.AdvancePeek(insidePeek);
         } else if(CompareString(token,"#{")){
            Token insidePeek = arguments.PeekIncludingDelimiterExpression({"#{"},{"}"},1);
            arguments.AdvancePeek(insidePeek);
         }

         commandSize += 1;
      }
   }

   com->expressions = PushArray<Expression*>(out,commandSize);
   for(int i = 0; i < com->expressions.size; i++){
      com->expressions[i] = ParseExpression(tok,out);
   }

   return com;
}

// Crude parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok,Arena* temp){
   void* start = tok->Mark();
   Token firstId = tok->NextToken();

   current->type = Expression::IDENTIFIER;
   current->id = firstId;

   while(1){
      Token token = tok->PeekToken();

      if(CompareString(token,"[")){
         tok->AdvancePeek(token);
         Expression* expr = PushStruct<Expression>(temp);
         expr->expressions = PushArray<Expression*>(temp,2);

         expr->type = Expression::ARRAY_ACCESS;
         expr->expressions[0] = current;
         expr->expressions[1] = ParseExpression(tok,temp);
         expr->text = tok->Point(start);

         tok->AssertNextToken("]");

         current = expr;
      } else if(CompareString(token,".")){
         tok->AdvancePeek(token);
         Token memberName = tok->NextToken();

         Expression* expr = PushStruct<Expression>(temp);
         expr->expressions = PushArray<Expression*>(temp,1);

         expr->type = Expression::MEMBER_ACCESS;
         expr->id = memberName;
         expr->expressions[0] = current;
         expr->text = tok->Point(start);

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
      expr = ParseExpression(tok,arena);
      tok->AssertNextToken("}");
   } else if(CompareString(token,"#{")){
      tok->AdvancePeek(token);

      expr->type = Expression::COMMAND;
      expr->command = ParseCommand(tok,arena);

      Assert(!IsCommandBlockType(expr->command));
      tok->AssertNextToken("}");
   } else {
      expr->type = Expression::IDENTIFIER;
      expr = ParseIdentifier(expr,tok,arena);
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
      expr = ParseExpression(tok,arena);
      tok->AssertNextToken(")");
   } else if(CompareString(peek,"!")){
      tok->AdvancePeek(peek);

      Expression* child = ParseExpression(tok,arena);

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

static Expression* ParseExpression(Tokenizer* tok,Arena* temp){
   void* start = tok->Mark();

   Expression* res = ParseOperationType(tok,{{"#"},{"|>"},{"and","or","xor"},{">","<",">=","<=","==","!="},{"+","-"},{"*","/","&","**"}},ParseFactor,temp);

   res->text = tok->Point(start);
   return res;
}

static Block* Parse(Tokenizer* tok,Arena* out){
   Block* block = PushStruct<Block>(out);
   *block = {};

   void* start = tok->Mark();

   Token token = tok->PeekToken();
   if(CompareString(token,"#{")){
      block->type = Block::COMMAND;

      tok->AdvancePeek(token);
      block->command = ParseCommand(tok,out);
      tok->AssertNextToken("}");

      if(!IsCommandBlockType(block->command)){
         return block;
      }

      Block* ptr = block;
      while(1){
         Block* child = Parse(tok,out);
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

std::unordered_map<String,PipeFunction> pipeFunctions;

void RegisterPipeOperation(String name,PipeFunction func){
   pipeFunctions.insert({name,func});
}

static Value EvalExpression(Expression* expr,Frame* frame,Arena* temp);

static Value EvalExpression(Expression* expr,Frame* frame,Arena* temp){
   #ifdef DEBUG_TEMPLATE_ENGINE
   printf("%.*s:\n",UNPACK_SS(expr->text));
   #endif // DEBUG_TEMPLATE_ENGINE

   Value val = {};
   switch(expr->type){
      case Expression::OPERATION:{
         if(expr->op[0] == '|'){ // Pipe operation
            val = EvalExpression(expr->expressions[0],frame,temp);

            auto iter = pipeFunctions.find(expr->expressions[1]->id);
            if(iter == pipeFunctions.end()){
               NOT_IMPLEMENTED;
            }

            PipeFunction func = iter->second;
            val = func(val,temp);

            goto EvalExpressionEnd;
         }

         if(CompareString(expr->op,"!")){
            Value op = EvalExpression(expr->expressions[0],frame,temp);
            bool boolean = ConvertValue(op,ValueType::BOOLEAN,nullptr).boolean;

            val = MakeValue(!boolean);
            goto EvalExpressionEnd;
         }

         // Two or more op operations
         Value op1 = EvalExpression(expr->expressions[0],frame,temp);

         // Short circuit
         if(CompareString(expr->op,"and")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;

            if(!bool1){
               val = MakeValue(false);
               goto EvalExpressionEnd;
            }
         } else if(CompareString(expr->op,"or")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;

            if(bool1){
               val = MakeValue(true);
               goto EvalExpressionEnd;
            }
         }

         Value op2 = EvalExpression(expr->expressions[1],frame,temp);

         if(CompareString(expr->op,"==")){
            val = MakeValue(Equal(op1,op2));
            goto EvalExpressionEnd;
         } else if(CompareString(expr->op,"!=")){
            val = MakeValue(!Equal(op1,op2));
            goto EvalExpressionEnd;
         }else if(CompareString(expr->op,"and")){
            // At this point bool1 is true
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            val = MakeValue(bool2);
            goto EvalExpressionEnd;
         } else if(CompareString(expr->op,"or")){
            // At this point bool1 is false
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            val = MakeValue(bool2);
            goto EvalExpressionEnd;
         } else if(CompareString(expr->op,"xor")){
            bool bool1 = ConvertValue(op1,ValueType::BOOLEAN,nullptr).boolean;
            bool bool2 = ConvertValue(op2,ValueType::BOOLEAN,nullptr).boolean;

            val = MakeValue((bool1 && !bool2) || (!bool1 && bool2));
            goto EvalExpressionEnd;
         } else if(CompareString(expr->op,"#")){
            String first = ConvertValue(op1,ValueType::SIZED_STRING,temp).str;
            String second = ConvertValue(op2,ValueType::SIZED_STRING,temp).str;

            String res = PushString(temp,"%.*s%.*s",UNPACK_SS(first),UNPACK_SS(second));

            val =  MakeValue(res);
            goto EvalExpressionEnd;
         }

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
      } break;
      case Expression::LITERAL:{
         val = expr->val;
      } break;
      case Expression::COMMAND:{
         Command* com = expr->command;

         Assert(!IsCommandBlockType(com));

         val = EvalNonBlockCommand(com,frame,temp).val;
      } break;
      case Expression::IDENTIFIER:{
         Optional<Value> iter = GetValue(frame,expr->id);

         if(!iter){
            LogFatal(LogModule::TEMPLATE,"Did not find %.*s in template: %.*s",UNPACK_SS(expr->id),UNPACK_SS(globalTemplateName));
            DEBUG_BREAK();
         }

         val = iter.value();
      } break;
      case Expression::ARRAY_ACCESS:{
         Value object = EvalExpression(expr->expressions[0],frame,temp);
         Value index  = EvalExpression(expr->expressions[1],frame,temp);

         Assert(index.type == ValueType::NUMBER);

         Optional<Value> optVal = AccessObjectIndex(object,index.number);
         if(!optVal){
            FatalError(StaticFormat("Tried to access array at an index greater than size: %d/%d (%.*s)",index.number,IndexableSize(object),UNPACK_SS(object.type->name)));
         }
         val = optVal.value();
      } break;
      case Expression::MEMBER_ACCESS:{
         Value object = EvalExpression(expr->expressions[0],frame,temp);

         Optional<Value> optVal = AccessStruct(object,expr->id);
         if(!optVal){
		   PrintStructDefinition(object.type);
           FatalError(StaticFormat("Tried to access member (%.*s) that does not exist for type (%.*s)",UNPACK_SS(expr->text),UNPACK_SS(object.type->name)));
         }
         val = optVal.value();
      }break;
      default:{
         NOT_IMPLEMENTED;
      } break;
   }

EvalExpressionEnd:

   #ifdef DEBUG_TEMPLATE_ENGINE
   printf("%.*s\n",UNPACK_SS(val.type->name));
   #endif // DEBUG_TEMPLATE_ENGINE

   return val;
}

static String EvalBlockCommand(Block* block,Frame* previousFrame,Arena* temp){
   Command* com = block->command;
   String res = {};
   res.data = (char*) MarkArena(outputArena);

   if(CompareString(com->name,"join")){
      Frame* frame = CreateFrame(previousFrame,temp);
      Value separator = EvalExpression(com->expressions[0],frame,temp);

      Assert(separator.type == ValueType::STRING);

      Assert(com->expressions[2]->type == Expression::IDENTIFIER);
      String id = com->expressions[2]->id;

      Value iterating = EvalExpression(com->expressions[3],frame,temp);
      int counter = 0;
      int index = 0;
      for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
         SetValue(frame,STRING("index"),MakeValue(index));

         Value val = GetValue(iter);
         SetValue(frame,id,val);

         bool outputSeparator = false;
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            String text = Eval(ptr,frame,temp);

            if(!CheckStringOnlyWhitespace(text)){
               res.size += text.size; // Push on stack
               outputSeparator = true;
            }
         }

         if(outputSeparator){
            res.size += PushString(outputArena,"%.*s",separator.str.size,separator.str.data).size;
            counter += 1;
         }
      }
      if(counter == 0){
         return res;
      }

      outputArena->used -= separator.str.size;
      res.size -= separator.str.size;
      Assert(res.size >= 0);
   } else if(CompareString(com->name,"for")){
      Frame* frame = CreateFrame(previousFrame,temp);
      Assert(com->expressions[0]->type == Expression::IDENTIFIER);
      String id = com->expressions[0]->id;

      Value iterating = EvalExpression(com->expressions[1],frame,temp);
      int index = 0;
      for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
         SetValue(frame,STRING("index"),MakeValue(index));

         Value val = GetValue(iter);
         SetValue(frame,id,val);

         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            res.size += Eval(ptr,frame,temp).size; // Push on stack
         }
      }
   } else if (CompareString(com->name,"if")){
      Value expr = EvalExpression(com->expressions[0],previousFrame,temp);
      Value val = ConvertValue(expr,ValueType::BOOLEAN,nullptr);

      Frame* frame = CreateFrame(previousFrame,temp);
      if(val.boolean){
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            if(ptr->type == Block::COMMAND && CompareString(ptr->command->name,"else")){
               break;
            }
            res.size += Eval(ptr,frame,temp).size; // Push on stack
         }
      } else {
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            if(ptr->type == Block::COMMAND && CompareString(ptr->command->name,"else")){
               for(ptr = ptr->next; ptr != nullptr; ptr = ptr->next){
                  res.size += Eval(ptr,frame,temp).size; // Push on stack
               }
               break;
            }
         }
      }
   } else if(CompareString(com->name,"debug")){
      Frame* frame = CreateFrame(previousFrame,temp);
      Value val = EvalExpression(com->expressions[0],frame,temp);

      if(val.boolean){
         DEBUG_BREAK();
         debugging = true;
      }

      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         res.size += Eval(ptr,frame,temp).size; // Push on stack
      }

      debugging = false;
   } else if(CompareString(com->name,"while")){
      Frame* frame = CreateFrame(previousFrame,temp);
      while(ConvertValue(EvalExpression(com->expressions[0],frame,temp),ValueType::BOOLEAN,nullptr).boolean){
         for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
            res.size += Eval(ptr,frame,temp).size; // Push on stack
         }
      }
   } else if(CompareString(com->name,"define")) {
      String id = com->expressions[0]->id;

      TemplateFunction* func = PushStruct<TemplateFunction>(temp);

      func->arguments = &com->expressions[1];
      func->numberArguments = com->expressions.size - 1;
      func->block = block->nextInner;

      Value val = {};
      val.templateFunction = func;
      val.type = ValueType::TEMPLATE_FUNCTION;
      val.isTemp = true;

      SetValue(previousFrame,id,val);
   } else if(CompareString(com->name,"debugMessage")){
      Frame* frame = CreateFrame(previousFrame,temp);
      ArenaMarker marker(outputArena);

      for(Block* ptr = block->nextInner; ptr != nullptr; ptr = ptr->next){
         String res = Eval(ptr,frame,temp); // Push on stack
         printf("%.*s\n",UNPACK_SS(res));
      }
   } else {
      NOT_IMPLEMENTED;
   }

   return res;
}

extern Array<Pair<String,String>> templateNameToContent; // TODO: Kinda of a quick hack to make this work. Need to revise the way templates are done

static ValueAndText EvalNonBlockCommand(Command* com,Frame* previousFrame,Arena* temp){
   Value val = MakeValue();
   String text = {};
   text.data = (char*) MarkArena(outputArena);

   if(CompareString(com->name,"set")){
	 // TODO: Maybe change set to act as a declaration and initializar + setter and separate them.
	 //       Something like, set fails if variable does not exist. Use "let" to create variable which fails if it exists. That way we stop having problems with variables declaration and initialization
     val = EvalExpression(com->expressions[1],previousFrame,temp);

      Assert(com->expressions[0]->type == Expression::IDENTIFIER);

      SetValue(previousFrame,com->expressions[0]->id,val);
   } else if(CompareString(com->name,"inc")){
      val = EvalExpression(com->expressions[0],previousFrame,temp);

      Assert(val.type == ValueType::NUMBER);

      val.number += 1;

      SetValue(previousFrame,com->expressions[0]->id,val);
   } else if(CompareString(com->name,"include")){
      Value filenameString = EvalExpression(com->expressions[0],previousFrame,temp);
      Assert(filenameString.type == ValueType::STRING);

      String content = {};
      for(Pair<String,String>& nameToContent : templateNameToContent){
         if(CompareString(nameToContent.first,filenameString.str)){
            content = nameToContent.second;
            break;
         }
      }

      Assert(content.size);

      ParseAndEvaluate(content,previousFrame,temp); // Use previous frame. Include should techically be global
   } else if(CompareString(com->name,"call")){
      Frame* frame = CreateFrame(previousFrame,temp);
      String id = com->expressions[0]->id;

      Optional<Value> optVal = GetValue(frame,id);
      if(!optVal){
         printf("Failed to find %.*s\n",UNPACK_SS(id));
         DEBUG_BREAK();
      }

      TemplateFunction* func = optVal.value().templateFunction;

      Assert(func->numberArguments == com->expressions.size - 1);

      for(int i = 0; i < func->numberArguments; i++){
         String id = func->arguments[i]->id;

         Value val = EvalExpression(com->expressions[1+i],frame,temp);

         SetValue(frame,id,val);
      }

      for(Block* ptr = func->block; ptr != nullptr; ptr = ptr->next){
         text.size += Eval(ptr,frame,temp).size;
      }

      optVal = GetValue(frame,STRING("return"));
      if(optVal){
         val = optVal.value();
      }
   } else if(CompareString(com->name,"return")){
      val = EvalExpression(com->expressions[0],previousFrame,temp);

      SetValue(previousFrame,STRING("return"),val);
   } else if(CompareString(com->name,"format")){
      Frame* frame = CreateFrame(previousFrame,temp);
      Value formatExpr = EvalExpression(com->expressions[0],frame,temp);

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

         Value val = ConvertValue(EvalExpression(com->expressions[index + 1],frame,temp),ValueType::SIZED_STRING,temp);

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

static String Eval(Block* block,Frame* frame,Arena* temp){
   String res = {};
   res.data = (char*) MarkArena(outputArena);

   if(block->type == Block::COMMAND){
      if(IsCommandBlockType(block->command)){
         res = EvalBlockCommand(block,frame,temp);
      } else {
         res = EvalNonBlockCommand(block->command,frame,temp).text;
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

         res.size += PushString(outputArena,text).size;

         if(tok.Done()){
            break;
         }

         tok.AssertNextToken("@{");
         Expression* expr = ParseExpression(&tok,temp);
         tok.AssertNextToken("}");

         Value val = EvalExpression(expr,frame,temp);

         res.size += GetDefaultValueRepresentation(val,outputArena).size;
      }
   }

   Assert(res.size >= 0);
   return res;
}

void InitializeTemplateEngine(Arena* perm){
   globalFrame->table = PushHashmap<String,Value>(perm,99);
   globalFrame->previousFrame = nullptr;
}

void ParseAndEvaluate(String content,Frame* frame,Arena* temp){
   Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
   Tokenizer* tok = &tokenizer;

   tok->keepComments = true;

   while(!tok->Done()){
      Block* block = Parse(tok,temp);

      String text = Eval(block,frame,temp);
      fprintf(output,"%.*s",text.size,text.data);
      fflush(output);
   }
}

CompiledTemplate* CompileTemplate(String content,const char* name,Arena* arena){
   Byte* mark = MarkArena(arena);

   String storedName = PushString(arena,STRING(name));
   globalTemplateName = storedName;

   CompiledTemplate* res = PushStruct<CompiledTemplate>(arena);

   Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
   Tokenizer* tok = &tokenizer;
   tok->keepComments = true;

   Block* initial = Parse(tok,arena);
   Block* ptr = initial;
   while(!tok->Done()){
      Assert(ptr->next == nullptr);
      ptr->next = Parse(tok,arena);
      ptr = ptr->next;
   }

   String totalMemory = PointArena(arena,mark);
   res->blocks = initial;
   res->totalMemoryUsed = totalMemory.size;
   res->content = content;
   res->name = storedName;

   return res;
}

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate,Arena* arena){
   Assert(globalFrame && "Call InitializeTemplateEngine first!");

   globalTemplateName = compiledTemplate->name;

   ArenaMarker marker(arena);
   Arena outputArenaInst = SubArena(arena,Megabyte(64));
   outputArena = &outputArenaInst;
   output = outputFile;

   for(Block* block = compiledTemplate->blocks; block; block = block->next){
      String text = Eval(block,globalFrame,arena);
      fprintf(output,"%.*s",text.size,text.data);
      fflush(output);
   }
}

void ClearTemplateEngine(){
   globalFrame->table->Clear();
}

void TemplateSetCustom(const char* id,void* entity,const char* typeName){
   Value val = MakeValue(entity,typeName);

   SetValue(globalFrame,STRING(id),val);
}

void TemplateSetNumber(const char* id,int number){
   SetValue(globalFrame,STRING(id),MakeValue(number));
}

void TemplateSet(const char* id,void* ptr){
   Value val = {};
   val.custom = ptr;
   val.type = ValueType::NUMBER;
   SetValue(globalFrame,STRING(id),val);
}

void TemplateSetString(const char* id,const char* str){
   SetValue(globalFrame,STRING(id),MakeValue(STRING(str)));
}

void TemplateSetString(const char* id,String str){
   SetValue(globalFrame,STRING(id),MakeValue(str));
}

void TemplateSetBool(const char* id,bool boolean){
   SetValue(globalFrame,STRING(id),MakeValue(boolean));
}

void TemplateSetArray(const char* id,const char* baseType,void* array,int size){
   Value val = {};

   val.type = GetArrayType(GetType(STRING(baseType)),size);
   val.custom = array;

   SetValue(globalFrame,STRING(id),val);
}
