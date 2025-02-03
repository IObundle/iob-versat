#include "templateEngine.hpp"

#include "memory.hpp"
#include "parser.hpp"
#include "stdlib.h"
#include "stdio.h"

#include <cstring>
#include <printf.h>
#include <unordered_map>

#include "utils.hpp"
#include "type.hpp"
#include "utilsCore.hpp"
#include "debug.hpp"

/*

What I should tackle first:

  Only common.tpl including is working. For now there is no need for full includes
  Command could just be an expression.
  Maybe should check the seperation of expressions instead of reusing the parser one.
  #{} should never print. @{} should always print. The logic should be unified and enforced, right now #{} gets parsed as commands and @{} gets parsed as expressions but realistically we could figure out if its a command or an expression by finding if the first id is a command id or not (but if so we cannot have identifiers with the same name as a command).

 */

//#define DEBUG_TEMPLATE_ENGINE

static ArenaList<TemplateRecord>* recordList;
static String globalTemplateName = {}; // The current name that is being parsed / evaluated. For error reporting reasons. TODO: Do not like it, not a big deal for now. Only change if needed
extern Array<Pair<String,String>> templateNameToContent; // TODO: Kinda of a quick hack to make this work. Need to revise the way templates are done

static std::unordered_map<String,PipeFunction> pipeFunctions;

static Value EvalExpression(Expression* expr,Frame* frame,Arena* out);

// TODO: This is not the prefered way of doing this.
// Maybe just compile the template in malloc'ed memory
// And keep the memory around, it's not like user code would have a reason to free this memory
static CompiledTemplate* savedTemplate = nullptr;
void SetIncludeHeader(CompiledTemplate* tpl,String name){
  savedTemplate = tpl;
}

bool operator==(const TemplateRecord& r0,const TemplateRecord& r1){
  if(r0.type != r1.type){
    return false;
  }

  bool res = false;
  switch(r0.type){
  case TemplateRecordType_FIELD:{
    res = (r0.structType == r1.structType) && CompareString(r0.fieldName,r1.fieldName);
  } break;
  case TemplateRecordType_IDENTIFER:{
    res = (r0.identifierType == r1.identifierType) && CompareString(r0.identifierName,r1.identifierName);
  } break;
  }
  
  return res;
}

static Opt<Value> GetValue(Frame* frame,String var){
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

static void CreateValue(Frame* frame,String id,Value val){
  frame->table->Insert(id,val);
}

static void SetValue(Frame* frame,String id,Value val){
  Value* possible = ValueExists(frame,id);
  if(possible){
    *possible = val;
  } else {
    frame->table->Insert(id,val);
  }
}

static Frame* CreateFrame(Frame* previous,Arena* out){
  Frame* frame = PushStruct<Frame>(out);
  frame->table = PushHashmap<String,Value>(out,16); // Testing a fixed hashmap for now.
  frame->previousFrame = previous;
  return frame;
}

static String Eval(Block* block,Frame* frame);
static ValueAndText EvalNonBlockCommand(Command* com,Frame* frame);
static Expression* ParseExpression(Tokenizer* tok,Arena* out);

// Static variables
static bool debugging = false;
static Frame globalFrameInst = {};
static Frame* globalFrame = &globalFrameInst;
static FILE* output;
static Arena* outputArena;

static void FatalError(const char* reason,int approximateLine){
  printf("Template: error on template (%.*s:%d):\n",UNPACK_SS(globalTemplateName),approximateLine);
  printf("\t%s\n",reason);
  DEBUG_BREAK();
}

static void SetExpressionLine(Expression* top,int line){
  top->approximateLine = line;
  if(top->type == Expression::COMMAND){
    for(Expression* e : top->expressions){
      SetExpressionLine(e,line);
    }
  }

  for(Expression* expr : top->expressions){
    SetExpressionLine(expr,line);
  }
}

static CommandDefinition commandDefinitions[] = {{STRING("join"),3,CommandType_JOIN,true},
                                                 {STRING("for"),2,CommandType_FOR,true},
                                                 {STRING("if"),1,CommandType_IF,true},
                                                 {STRING("end"),0,CommandType_END,false},
                                                 {STRING("set"),2,CommandType_SET,false},
                                                 {STRING("let"),2,CommandType_LET,false},
                                                 {STRING("inc"),1,CommandType_INC,false},
                                                 {STRING("else"),0,CommandType_ELSE,false},
                                                 {STRING("debug"),1,CommandType_DEBUG,true},
                                                 {STRING("include"),1,CommandType_INCLUDE,false},
                                                 {STRING("define"),-1,CommandType_DEFINE,true},
                                                 {STRING("call"),-1,CommandType_CALL,false},
                                                 {STRING("while"),1,CommandType_WHILE,true},
                                                 {STRING("return"),1,CommandType_RETURN,false},
                                                 {STRING("format"),-1,CommandType_FORMAT,false},
                                                 {STRING("debugMessage"),0,CommandType_DEBUG_MESSAGE,true},
                                                 {STRING("debugBreak"),0,CommandType_DEBUG_BREAK,false}};

static Command* ParseCommand(Tokenizer* tok,Arena* out){
  auto mark = tok->Mark();
  tok->AssertNextToken("#{");

  Command* com = PushStruct<Command>(out);
  *com = {};

  Token commandName = tok->NextToken();
  for(unsigned int i = 0; i < ARRAY_SIZE(commandDefinitions); i++){
    CommandDefinition* ptr = &commandDefinitions[i];

    if(CompareString(commandName,ptr->name)){
      com->definition = ptr;
      break;
    }
  }
  if(!com->definition){
    printf("Internal template engine error, did not find command: %.*s\n",UNPACK_SS(commandName));
    exit(-1);
  }
  
  int actualExpressionSize = com->definition->numberExpressions;
  if(actualExpressionSize == -1){
    TEMP_REGION(temp,out);

    ArenaList<Expression*>* list = PushArenaList<Expression*>(temp);
    while(!tok->Done()){
      Token peek = tok->PeekToken();
      if(CompareString(peek,"}")){
        break;
      }

      *list->PushElem() = ParseExpression(tok,out);
    }

    com->expressions = PushArrayFromList(out,list);
  } else {
    com->expressions = PushArray<Expression*>(out,actualExpressionSize);
    for(int i = 0; i < com->expressions.size; i++){
      com->expressions[i] = ParseExpression(tok,out);
    }
  }

  tok->AssertNextToken("}");
  com->fullText = tok->Point(mark);
  
  return com;
}

// Crude parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok,Arena* out){
  auto start = tok->Mark();
  Token firstId = tok->NextToken();

  current->type = Expression::IDENTIFIER;
  current->id = firstId;

  while(1){
    Token token = tok->PeekToken();

    if(CompareString(token,"[")){
      tok->AdvancePeek();
      Expression* expr = PushStruct<Expression>(out);
      *expr = {};
      expr->expressions = PushArray<Expression*>(out,2);

      expr->type = Expression::ARRAY_ACCESS;
      expr->expressions[0] = current;
      expr->expressions[1] = ParseExpression(tok,out);
      expr->text = tok->Point(start);
      
      tok->AssertNextToken("]");

      current = expr;
    } else if(CompareString(token,".")){
      tok->AdvancePeek();
      Token memberName = tok->NextToken();

      Expression* expr = PushStruct<Expression>(out);
      *expr = {};
      expr->expressions = PushArray<Expression*>(out,1);

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

static Expression* ParseAtom(Tokenizer* tok,Arena* out){
  auto start = tok->Mark();

  Expression* expr = PushStruct<Expression>(out);
  *expr = {};
  expr->type = Expression::LITERAL;

  Token token = tok->PeekToken();
  if(token[0] >= '0' && token[0] <= '9'){
    tok->AdvancePeek();
    int num = 0;

    for(int i = 0; i < token.size; i++){
      num *= 10;
      num += token[i] - '0';
    }

    expr->val = MakeValue(num);
  } else if(token[0] == '\"'){
    tok->AdvancePeek();
    Token str = tok->NextFindUntil("\"").value();
    tok->AssertNextToken("\"");

    expr->val = MakeValue(str);
  } else if(CompareString(token,"@{")){
    tok->AdvancePeek();
    expr = ParseExpression(tok,out);
    tok->AssertNextToken("}");
  } else if(CompareString(token,"#{")){
    expr->type = Expression::COMMAND;
    expr->command = ParseCommand(tok,out);
    
    Assert(!expr->command->definition->isBlockType);
  } else {
    expr->type = Expression::IDENTIFIER;
    expr = ParseIdentifier(expr,tok,out);
  }

  expr->text = tok->Point(start);
  return expr;
}

static Expression* ParseFactor(Tokenizer* tok,Arena* out){
  auto start = tok->Mark();

  Token peek = tok->PeekToken();

  Expression* expr = nullptr;
  if(CompareString(peek,"(")){
    tok->AdvancePeek();
    expr = ParseExpression(tok,out);
    tok->AssertNextToken(")");
  } else if(CompareString(peek,"!")){
    tok->AdvancePeek();

    Expression* child = ParseExpression(tok,out);

    expr = PushStruct<Expression>(out);
    *expr = {};
    expr->expressions = PushArray<Expression*>(out,1);

    expr->type = Expression::OPERATION;
    expr->op = "!";
    expr->expressions[0] = child;

    expr->text = tok->Point(start);
  } else {
    expr = ParseAtom(tok,out);
  }

  expr->text = tok->Point(start);
  return expr;
}

static Expression* ParseExpression(Tokenizer* tok,Arena* out){
  auto start = tok->Mark();

  Expression* res = ParseOperationType(tok,{{"#"},{"|>"},{"and","or","xor"},{">","<",">=","<=","==","!="},{"+","-"},{"*","/","&","**"}},ParseFactor,out);

  res->text = tok->Point(start);
  return res;
}

static Expression* ParseBlockExpression(Tokenizer* tok,int line,Arena* out){
  auto mark = tok->Mark();
  tok->AssertNextToken("@{");

  Expression* expr = ParseExpression(tok,out);
  SetExpressionLine(expr,line);
  
  tok->AssertNextToken("}");
  expr->text = tok->Point(mark);
  
  return expr;
}

static Array<IndividualBlock> ParseIndividualLine(String line, int lineNumber,Arena* out){
  TEMP_REGION(temp,out);
  if(CompareString(line,"")){
    Array<IndividualBlock> arr = PushArray<IndividualBlock>(out,1);
    arr[0] = {line,BlockType_TEXT,lineNumber};
    return arr;
  }

  Tokenizer tok(line,"}",{"@{","#{"});
  tok.keepComments = true;
  ArenaList<IndividualBlock>* strings = PushArenaList<IndividualBlock>(temp);

  while(1){
    Opt<FindFirstResult> res = tok.FindFirst({"#{","@{"});
    
    if(!res.has_value()){
      String leftover = tok.Finish();
      if(leftover.size > 0){
        *strings->PushElem() = (IndividualBlock){leftover,BlockType_TEXT,lineNumber};
      }
      break;
    }

    FindFirstResult result = res.value();
    if(result.peekFindNotIncluded.size > 0){
      *strings->PushElem() = (IndividualBlock){result.peekFindNotIncluded,BlockType_TEXT,lineNumber};
    }

    tok.AdvancePeekBad(result.peekFindNotIncluded);
    if(CompareString(result.foundFirst,"#{")){
      auto mark = tok.Mark();
      tok.AdvanceDelimiterExpression({"#{","@{"},{"}"},0);
      String command = tok.Point(mark);
      *strings->PushElem() = (IndividualBlock){command,BlockType_COMMAND,lineNumber};
    } else if(CompareString(result.foundFirst,"@{")){
      auto mark = tok.Mark();
      tok.AdvanceDelimiterExpression({"#{","@{"},{"}"},0);
      String expression = tok.Point(mark);
      *strings->PushElem() = (IndividualBlock){expression,BlockType_EXPRESSION,lineNumber};
    }
  }

  Array<IndividualBlock> fullySeparated = PushArrayFromList(out,strings);
  for(IndividualBlock d : fullySeparated){
    Assert(d.content.size > 0);
  }

  return fullySeparated;
}

static void Print(Block* block, int level = 0){
  TEMP_REGION(temp,nullptr);
  
  if(block == nullptr){
    return;
  }

  for(int i = 0; i < level*2; i++){
    printf(" ");
  }

  switch(block->type){
  case BlockType_COMMAND:{
    printf("Comm: %.*s:%d\n",UNPACK_SS(block->command->fullText),block->line);
    for(Block* ptr : block->innerBlocks){
      Print(ptr,level + 1);
    }
  } break;
  case BlockType_EXPRESSION:{
    printf("Expr: %.*s:%d\n",UNPACK_SS(block->expression->text),block->line);
  } break;
  case BlockType_TEXT:{
    String escaped = PushEscapedString(temp,block->textBlock,'_');
    printf("Text: %.*s:%d\n",UNPACK_SS(escaped),block->line);
  } break;
  }
}

String GetCommandFromIndividualBlock(IndividualBlock* block){
  Assert(block->type == BlockType_COMMAND);

  Tokenizer tok(block->content,"",{"#{"});

  tok.AssertNextToken("#{");
  String command = tok.NextToken();

  return command;
}

Array<Block*> ConvertIndividualBlocksIntoHierarchical_(Array<IndividualBlock> blocks,int& index,int level,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<Block*>* blockList = PushArenaList<Block*>(temp);

  for(; index < blocks.size;){
    IndividualBlock* individualBlock = &blocks[index];
    index += 1;
    
    Block* block = nullptr;
    switch(individualBlock->type){
    case BlockType_TEXT:{
      block = PushStruct<Block>(out);
      *block = {};
      block->textBlock = individualBlock->content;
    }break;
    case BlockType_COMMAND:{
      String content = individualBlock->content;
      Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});

      auto mark = MarkArena(out);
      Command* command = ParseCommand(&tokenizer,out);

      for(Expression* expr : command->expressions){
        SetExpressionLine(expr,individualBlock->line);
      }
      
      if(command->definition->type == CommandType_END){
        if(level == 0){
          printf("Error on template engine, found an extra #{end}\n");
          printf("%.*s:%d",UNPACK_SS(globalTemplateName),individualBlock->line);
          Assert(false);
        }
        PopMark(mark); // No need to keep parsed command in memory, we do not process #{end}
        goto exit_loop;
      }

      block = PushStruct<Block>(out);
      *block = {};
      
      block->command = command;
      if(command->definition->isBlockType){ // Parse subchilds and add them to block.
        block->innerBlocks = ConvertIndividualBlocksIntoHierarchical_(blocks,index,level + 1,out);
      }
    }break;
    case BlockType_EXPRESSION:{
      block = PushStruct<Block>(out);
      *block = {};

      String content = individualBlock->content;
      Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});

      block->expression = ParseBlockExpression(&tokenizer,individualBlock->line,out);
    }break;
    }

    if(block){
      block->type = individualBlock->type;
      block->line = individualBlock->line;
      block->fullText = individualBlock->content;
      *blockList->PushElem() = block;
    }
  }

 exit_loop:
  Array<Block*> res = PushArrayFromList(out,blockList);
  
  return res;
}

Array<Block*> ConvertIndividualBlocksIntoHierarchical(Array<IndividualBlock> blocks,Arena* out){
  TEMP_REGION(temp,out);
  int index = 0;
  Array<Block*> res = ConvertIndividualBlocksIntoHierarchical_(blocks,index,0,out);

  return res;
}

CompiledTemplate* CompileTemplate(String content,const char* name,Arena* out){
  TEMP_REGION(temp,out);

  auto startMemoryUsed = MemoryUsage(out);

  String storedName = PushString(out,STRING(name));
  globalTemplateName = storedName;

  CompiledTemplate* res = PushStruct<CompiledTemplate>(out);
  *res = {};
  
  Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
  Tokenizer* tok = &tokenizer;
  tok->keepComments = true;

  ArenaList<IndividualBlock>* blockList = PushArenaList<IndividualBlock>(temp);
  Array<String> lines = Split(content,'\n',temp);
  
  for(int i = 0; i < lines.size; i++){
    String& line = lines[i];
    int lineNumber = i + 1;
    Array<IndividualBlock> sep = ParseIndividualLine(line,lineNumber,temp);
    
    bool containsCommand = false;
    bool containsExpression = false;
    bool onlyWhitespace = true;
    for(IndividualBlock& s : sep){
      switch(s.type){
      case BlockType_EXPRESSION: containsExpression = true; break;
      case BlockType_COMMAND: containsCommand = true; break;
      case BlockType_TEXT: {
        if(onlyWhitespace && !IsOnlyWhitespace(s.content)){
          onlyWhitespace = false;
        }
      } break;
      }
    }

    bool onlyNakedCommands = (containsCommand && onlyWhitespace && !containsExpression);

    if(onlyNakedCommands){
      for(IndividualBlock& block : sep){
        if(block.type == BlockType_COMMAND){
          *blockList->PushElem() = block;
        }
        Assert(block.type != BlockType_EXPRESSION);
      }
    } else {
      for(IndividualBlock& block : sep){
        *blockList->PushElem() = block;
      }

      *blockList->PushElem() = {STRING("\n"),BlockType_TEXT,lineNumber};
    }
  }

  Array<IndividualBlock> blocks = PushArrayFromList(out,blockList);
  Array<Block*> results = ConvertIndividualBlocksIntoHierarchical(blocks,out);

  res->blocks = results;
  res->totalMemoryUsed = MemoryUsage(out) - startMemoryUsed;
  res->content = content;
  res->name = storedName;

  return res;
}

void RegisterPipeOperation(String name,PipeFunction func){
  pipeFunctions.insert({name,func});
}

static Value EvalExpression(Expression* expr,Frame* frame,Arena* out){
  Value val = {};
  switch(expr->type){
  case Expression::OPERATION:{
    if(expr->op[0] == '|'){ // Pipe operation
      val = EvalExpression(expr->expressions[0],frame,out);
      
      auto iter = pipeFunctions.find(expr->expressions[1]->id);
      if(iter == pipeFunctions.end()){
        printf("Did not find the following pipe function\n");
        printf("%.*s\n",UNPACK_SS(expr->expressions[1]->id));
        USER_ERROR("Program error, fix template or register pipe");
      }

      PipeFunction func = iter->second;
      val = func(val,out);

      goto EvalExpressionEnd;
    }

    if(CompareString(expr->op,"!")){
      Value op = EvalExpression(expr->expressions[0],frame,out);
      bool boolean = ConvertValue(op,ValueType::BOOLEAN,nullptr).boolean;

      val = MakeValue(!boolean);
      goto EvalExpressionEnd;
    }

    // Two or more op operations
    Value op1 = EvalExpression(expr->expressions[0],frame,out);

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

    Value op2 = EvalExpression(expr->expressions[1],frame,out);

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
      String first = ConvertValue(op1,ValueType::SIZED_STRING,out).str;
      String second = ConvertValue(op2,ValueType::SIZED_STRING,out).str;

      Arena leaky = InitArena(first.size + second.size + 1); // TODO: Leaky
      String res = PushString(&leaky,"%.*s%.*s",UNPACK_SS(first),UNPACK_SS(second));

      val = MakeValue(res);
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
    }
  } break;
  case Expression::LITERAL:{
    val = expr->val;
  } break;
  case Expression::COMMAND:{
    Command* com = expr->command;

    Assert(!com->definition->isBlockType);

    val = EvalNonBlockCommand(com,frame).val;
  } break;
  case Expression::IDENTIFIER:{
    Opt<Value> iter = GetValue(frame,expr->id);
    
    if(!iter){
      LogFatal(LogModule::TEMPLATE,"Did not find '%.*s' in template '%.*s'",UNPACK_SS(expr->id),UNPACK_SS(globalTemplateName));
    }

    // Only register top frame accesses
    Type* type = iter.value().type;
    Opt<Value> globalOpt = GetValue(globalFrame,expr->id);
    if(globalOpt.has_value()){
      TemplateRecord* record = recordList->PushElem();
      record->type = TemplateRecordType_IDENTIFER;
      record->identifierType = type;
      record->identifierName = expr->id;
    }
    
    val = iter.value();
  } break;
  case Expression::ARRAY_ACCESS:{
    Value object = EvalExpression(expr->expressions[0],frame,out);
    Value index  = EvalExpression(expr->expressions[1],frame,out);

    Assert(index.type == ValueType::NUMBER);

    Opt<Value> optVal = AccessObjectIndex(object,index.number);
    if(!optVal){
      FatalError(StaticFormat("Tried to access array at an index greater than size: %d/%d (%.*s)",index.number,IndexableSize(object),UNPACK_SS(object.type->name)),expr->approximateLine);
    }
    val = optVal.value();
  } break;
  case Expression::MEMBER_ACCESS:{
    Value object = EvalExpression(expr->expressions[0],frame,out);

    Opt<Value> optVal = AccessStruct(object,expr->id);
    if(!optVal){
	  PrintStructDefinition(object.type);
      FatalError(StaticFormat("Tried to access member (%.*s) that does not exist for type (%.*s)",UNPACK_SS(expr->text),UNPACK_SS(object.type->name)),expr->approximateLine);
    }

    TemplateRecord* record = recordList->PushElem();
    record->type = TemplateRecordType_FIELD;
    record->structType = object.type;
    record->fieldName = expr->id;

    val = optVal.value();
  }break;
  case Expression::UNDEFINED: NOT_POSSIBLE();
  }

 EvalExpressionEnd:

  return val;
}

static String EvalBlockCommand(Block* block,Frame* previousFrame){
  TEMP_REGION(temp,nullptr);
  Command* com = block->command;
  auto mark = StartString(outputArena);

  Assert(com->definition->isBlockType);

  switch(com->definition->type){
  case CommandType_JOIN:{
    Frame* frame = CreateFrame(previousFrame,temp);
    Value separator = EvalExpression(com->expressions[0],frame,temp);

    Assert(separator.type == ValueType::STRING || separator.type == ValueType::CONST_STRING);

    Assert(com->expressions[1]->type == Expression::IDENTIFIER);
    String id = com->expressions[1]->id;
    
    Value iterating = EvalExpression(com->expressions[2],frame,temp);
    int index = 0;
    bool removeLastOutputSeperator = false;
    for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
      CreateValue(frame,STRING("index"),MakeValue(index));
      
      Value val = GetValue(iter);
      SetValue(frame,id,val);

      // TODO: the format of the text becomes hard to read and there's
      //       little that can be done because we have little control
      //       in regards to the output arena usage. We could just have a 2 step
      //       approach where we just have a more complex builder process that allow us to 
      //       perform more logic over the text blocks
      bool outputSeparator = false;
      for(Block* ptr : block->innerBlocks){
        String text = Eval(ptr,frame);

        if(IsOnlyWhitespace(text)){
          outputArena->used -= text.size;
        } else {
          outputSeparator = true;
        }
      }

      if(outputSeparator){
        PushString(outputArena,"%.*s",separator.str.size,separator.str.data);
        removeLastOutputSeperator = true;
      }
    }

    if(removeLastOutputSeperator){
      outputArena->used -= separator.str.size;
    }
  } break;
  case CommandType_FOR:{
    Frame* frame = CreateFrame(previousFrame,temp);
    Assert(com->expressions[0]->type == Expression::IDENTIFIER);
    String id = com->expressions[0]->id;

    Value iterating = EvalExpression(com->expressions[1],frame,temp);
    int index = 0;
    for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
      SetValue(frame,STRING("index"),MakeValue(index));

      Value val = GetValue(iter);
      SetValue(frame,id,val);

      for(Block* ptr : block->innerBlocks){
        Eval(ptr,frame); // Push on stack
      }
    }
  } break;
  case CommandType_IF:{
    Value expr = EvalExpression(com->expressions[0],previousFrame,temp);
    Value val = ConvertValue(expr,ValueType::BOOLEAN,nullptr);

    Frame* frame = CreateFrame(previousFrame,temp);
    if(val.boolean){
      for(Block* ptr : block->innerBlocks){
        if(ptr->type == BlockType_COMMAND && ptr->command->definition->type == CommandType_ELSE){
          break;
        }
        Eval(ptr,frame); // Push on stack
      }
    } else {
      bool sawElse = false;
      for(Block* ptr : block->innerBlocks){
        if(sawElse){
          Eval(ptr,frame); // Push on stack
        }

        if(ptr->type == BlockType_COMMAND && ptr->command->definition->type == CommandType_ELSE){
          Assert(!sawElse); // Cannot have more than one else per if block
          sawElse = true;
        }
      }
    }
  } break;
  case CommandType_DEBUG:{
    Frame* frame = CreateFrame(previousFrame,temp);
    Value val = EvalExpression(com->expressions[0],frame,temp);

    if(val.boolean){
      DEBUG_BREAK();
      debugging = true;
    }

    for(Block* ptr : block->innerBlocks){
      Eval(ptr,frame); // Push on stack
    }

    debugging = false;
  } break;
  case CommandType_WHILE:{
    Frame* frame = CreateFrame(previousFrame,temp);
    while(ConvertValue(EvalExpression(com->expressions[0],frame,temp),ValueType::BOOLEAN,nullptr).boolean){
      for(Block* ptr : block->innerBlocks){
        Eval(ptr,frame); // Push on stack
      }
    }
  } break;
  case CommandType_DEFINE:{
    String id = com->expressions[0]->id;

    TemplateFunction* func = (TemplateFunction*) malloc(sizeof(TemplateFunction)); // TODO: Cannot Push to temp. This should be dealt with at Parse time, not eval time.
    
    func->arguments.data = &com->expressions[1];
    func->arguments.size = com->expressions.size - 1;
    func->blocks = block->innerBlocks;

    Value val = {};
    val.templateFunction = func;
    val.type = ValueType::TEMPLATE_FUNCTION;
    val.isTemp = true;

    SetValue(previousFrame,id,val);
  } break;
  case CommandType_DEBUG_MESSAGE:{
    Frame* frame = CreateFrame(previousFrame,temp);
    BLOCK_REGION(outputArena);

    for(Block* ptr : block->innerBlocks){
      String result = Eval(ptr,frame); // Push on stack
      printf("%.*s\n",UNPACK_SS(result));
    }
  } break;
  case CommandType_END:; 
  case CommandType_SET:; 
  case CommandType_LET:; 
  case CommandType_INC:; 
  case CommandType_ELSE:; 
  case CommandType_INCLUDE:; 
  case CommandType_CALL:; 
  case CommandType_RETURN:;
  case CommandType_FORMAT:;
  case CommandType_DEBUG_BREAK:;
    NOT_POSSIBLE("NonBlock command"); // TODO: Maybe join eval non block with block together.
  }

  String res = EndString(mark);
  return res;
}

static ValueAndText EvalNonBlockCommand(Command* com,Frame* previousFrame){
  TEMP_REGION(temp,nullptr);
  Value val = MakeValue();
  auto mark = StartString(outputArena);

  Assert(!com->definition->isBlockType);
  switch(com->definition->type){
  case CommandType_SET:{
    val = EvalExpression(com->expressions[1],previousFrame,temp);
    Assert(com->expressions[0]->type == Expression::IDENTIFIER);
    // TODO: Should give an error if id does not exist
    SetValue(previousFrame,com->expressions[0]->id,val);
  } break;
  case CommandType_LET:{
    val = EvalExpression(com->expressions[1],previousFrame,temp);
    Assert(com->expressions[0]->type == Expression::IDENTIFIER);
    CreateValue(previousFrame,com->expressions[0]->id,val);
  } break;
  case CommandType_INC:{
    val = EvalExpression(com->expressions[0],previousFrame,temp);

    Assert(val.type == ValueType::NUMBER);

    val.number += 1;

    SetValue(previousFrame,com->expressions[0]->id,val);
  } break;
  case CommandType_INCLUDE:{
    Value filenameString = EvalExpression(com->expressions[0],previousFrame,temp);
    Assert(filenameString.type == ValueType::STRING || filenameString.type == ValueType::CONST_STRING);

    String content = {};
    for(Pair<String,String>& nameToContent : templateNameToContent){
      if(CompareString(nameToContent.first,filenameString.str)){
        content = nameToContent.second;
        break;
      }
    }

    Assert(content.size);

    // Get compiled template and evaluate it.
    CompiledTemplate* templ = savedTemplate;
    for(Block* block : templ->blocks){
      Eval(block,previousFrame);
    }
  } break;
  case CommandType_CALL:{
    Frame* frame = CreateFrame(previousFrame,temp);
    String id = com->expressions[0]->id;

    Opt<Value> optVal = GetValue(frame,id);
    if(!optVal){
      printf("Failed to find %.*s\n",UNPACK_SS(id));
      DEBUG_BREAK();
    }

    TemplateFunction* func = optVal.value().templateFunction;
    Assert(func->arguments.size == com->expressions.size - 1);

    for(int i = 0; i < func->arguments.size; i++){
      String id = func->arguments[i]->id;

      Value val = EvalExpression(com->expressions[1+i],frame,temp);

      CreateValue(frame,id,val);
    }

    for(Block* ptr : func->blocks){
      Eval(ptr,frame);
    }

    optVal = GetValue(frame,STRING("return"));
    if(optVal.has_value()){
      val = optVal.value();
    }
  } break;
  case CommandType_RETURN:{
    val = EvalExpression(com->expressions[0],previousFrame,temp);

    SetValue(previousFrame,STRING("return"),val);
  } break;
  case CommandType_FORMAT:{
    Frame* frame = CreateFrame(previousFrame,temp);
    Value formatExpr = EvalExpression(com->expressions[0],frame,temp);

    Assert(formatExpr.type == ValueType::SIZED_STRING || formatExpr.type == ValueType::STRING || formatExpr.type == ValueType::CONST_STRING);

    String format = formatExpr.str;

    Tokenizer tok(format,"{}",{});
    while(!tok.Done()){
      Opt<Token> simpleText = tok.PeekFindUntil("{");

      if(!simpleText.has_value()){
        simpleText = tok.Finish();
      }

      tok.AdvancePeekBad(simpleText.value());

      PushString(outputArena,simpleText.value());

      if(tok.Done()){
        break;
      }

      tok.AssertNextToken("{");
      Token indexText = tok.NextToken();
      tok.AssertNextToken("}");

      int index = ParseInt(indexText);

      Assert(index < com->expressions.size + 1);

      Value val = ConvertValue(EvalExpression(com->expressions[index + 1],frame,temp),ValueType::SIZED_STRING,temp);

      PushString(outputArena,val.str);
    }
  } break;
  case CommandType_DEBUG_BREAK:{
    DEBUG_BREAK();
  } break;
  case CommandType_END:{
    printf("Error: founded a strain end in the template\n");
    Assert(false);
  } break;
  case CommandType_JOIN:
  case CommandType_FOR:
  case CommandType_IF:
  case CommandType_ELSE:
  case CommandType_DEBUG:
  case CommandType_DEFINE:
  case CommandType_WHILE:
  case CommandType_DEBUG_MESSAGE:
    NOT_POSSIBLE("Block command"); // TODO: Maybe join eval non block with block together.
  }

  ValueAndText res = {};
  res.val = val;
  res.text = EndString(mark);

  return res;
}

static String Eval(Block* block,Frame* frame){
  TEMP_REGION(temp,nullptr);

  auto mark = StartString(outputArena);

  switch(block->type){
  case BlockType_COMMAND:{
    if(block->command->definition->isBlockType){
      EvalBlockCommand(block,frame);
    } else {
      EvalNonBlockCommand(block->command,frame);
    }
  }break;
  case BlockType_EXPRESSION:{
    Value val = EvalExpression(block->expression,frame,temp);
    GetDefaultValueRepresentation(val,outputArena);
  } break;
  case BlockType_TEXT:{
    PushString(outputArena,block->textBlock);
  } break;
  }

  String res = EndString(mark);
  Assert(res.size >= 0);
  return res;
}

void InitializeTemplateEngine(Arena* perm){
  globalFrame->table = PushHashmap<String,Value>(perm,99);
  globalFrame->previousFrame = nullptr;
}

String RemoveRepeatedNewlines(String text,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartStringBuilder(temp);
  int state = 0;
  for(int i = 0; i < text.size; i++){
    switch(state){
    case 0: {
      if(text[i] == '\n'){
        state = 1;
      }
      builder->PushString("%c",text[i]);
    } break;
    case 1: {
      if(text[i] == '\n'){
        state = 2;
      } else {
        state = 0;
      }
      builder->PushString("%c",text[i]);
    } break;
    case 2:{
      if(text[i] == '\n'){
      } else {
        builder->PushString("%c",text[i]);
        if(!std::isspace(text[i])){
          state = 0;
        }
      }
    } break;
    }
  }

  return EndString(out,builder);
}

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  Assert(globalFrame && "Call InitializeTemplateEngine first!");

  globalTemplateName = compiledTemplate->name;

#ifdef DEBUG_TEMPLATE_ENGINE
  Arena recordSpaceInst = SubArena(temp,Megabyte(1));
  Arena* recordSpace = &recordSpaceInst;
  TrieMap<Type*,Array<bool>>* records = PushTrieMap<Type*,Array<bool>>(recordSpace);
#endif
  
  outputArena = temp2;
  output = outputFile;

  auto markOutput = StartString(outputArena);
  
  Frame* top = CreateFrame(globalFrame,temp);
  for(Block* block : compiledTemplate->blocks){
    BLOCK_REGION(debugArena);
    recordList = PushArenaList<TemplateRecord>(debugArena);

    Eval(block,top);

#ifdef DEBUG_TEMPLATE_ENGINE
    for(ListedStruct<TemplateRecord>* ptr = recordList->head; ptr; ptr = ptr->next){
      TemplateRecord record = ptr->elem;
      GetOrAllocateResult<Array<bool>> res = records->GetOrAllocate(record.structType);

      if(!res.alreadyExisted){
        *res.data = PushArray<bool>(recordSpace,record.structType->members.size);
        Memset(*res.data,false);
      }

      for(int i = 0; i < record.structType->members.size; i++){
        Member m = record.structType->members[i];
        if(CompareString(m.name,record.fieldName)){
          (*res.data)[i] = true;
          break;
        }
      }
    }
#endif
  }

  String fullText = EndString(markOutput);
  String treated = RemoveRepeatedNewlines(fullText,temp);
  fprintf(output,"%.*s",UNPACK_SS(treated));
  fflush(output);
  
#ifdef DEBUG_TEMPLATE_ENGINE
  for(auto p : records){
    Type* t = p.first;
    Array<bool> arr = p.second;
    Array<Member> members = t->members;

    printf("%.*s\n",UNPACK_SS(t->name));
    for(int i = 0; i < arr.size; i++){
      printf("  %.*s: ",UNPACK_SS(members[i].name));
      if(arr[i]){
        printf("Used\n");
      } else {
        printf("Not used\n");
      }
    }
  }
#endif
}

void PrintFrames(Frame* frame){
  Frame* ptr = frame;
  int index = 0;
  while(ptr){
    for(Pair<String,Value*> p : ptr->table){
      printf("%d %.*s\n",index,UNPACK_SS(p.first));
    }
    index += 1;
    ptr = ptr->previousFrame;
  }
}

Hashmap<String,Value>* GetAllTemplateValues(){
  return globalFrame->table;
}

void ClearTemplateEngine(){
  globalFrame->table->Clear();
}

void TemplateSetCustom(const char* id,Value val){
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

void TemplateSetArray(const char* id,const char* baseType,int size,void* array){
  Value val = {};

  val.type = GetArrayType(GetType(STRING(baseType)),size);
  val.custom = array;

  SetValue(globalFrame,STRING(id),val);
}

// TODO: Change so that we integrate this fully with the normal process template function.
#if 0
static Arena testInst = {};
static Arena* testArena = &testInst;
static Hashmap<Type*,Array<bool>>* fieldsPerTypeSeen = nullptr;
static void CheckUnusedFieldsOrTypes(CompiledTemplate* tpl){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  if(fieldsPerTypeSeen == nullptr){
    testInst = InitArena(Megabyte(1));
    fieldsPerTypeSeen = PushHashmap<Type*,Array<bool>>(testArena,99);
  }
  
  Array<TemplateRecord> records = RecordTypesAndFieldsUsed(tpl,temp);

  for(TemplateRecord& r : records){
    if(r.type == TemplateRecordType_FIELD){
      Type* structType = r.structType;
      GetOrAllocateResult res = fieldsPerTypeSeen->GetOrAllocate(structType);

      Array<bool>* arr = res.data;
      if(!res.alreadyExisted){
        *arr = PushArray<bool>(testArena,structType->members.size);
        Memset(*arr,false);
      }

      for(int i = 0; i < structType->members.size; i++){
        Member m = structType->members[i];
        if(CompareString(m.name,r.fieldName)){
          (*arr)[i] = true;
          break;
        }
      }
    }
  }

  for(Pair<Type*,Array<bool>> p : fieldsPerTypeSeen){
    Type* structType = p.first;
    Array<bool>& arr = p.second;

    printf("%.*s\n",UNPACK_SS(structType->name));
    for(int i = 0; i < arr.size; i++){
      Member m = structType->members[i];
      if(arr[i]){
        printf("  Used: %.*s\n",UNPACK_SS(m.name));
      } else {
        printf("  Not:  %.*s\n",UNPACK_SS(m.name));
      }        
    }
  }
}

static void CheckIfTemplateUsesAllValues(CompiledTemplate* tpl){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Array<TemplateRecord> records = RecordTypesAndFieldsUsed(tpl,temp2);

  Hashmap<String,Value>* valuesUsed = GetAllTemplateValues();
  Hashmap<String,bool>* seen = PushHashmap<String,bool>(temp2,valuesUsed->nodesUsed);

  for(Pair<String,Value> p : valuesUsed){
    seen->Insert(p.first,false);
  }
  
  for(TemplateRecord r : records){
    if(r.type == TemplateRecordType_IDENTIFER){
      seen->Insert(r.identifierName,true);
    }
  }

  for(Pair<String,bool> p : seen){
    if(p.second){
      printf("Seen: %.*s\n",UNPACK_SS(p.first));
    }
    if(!p.second){
      printf("Not : %.*s\n",UNPACK_SS(p.first));
    }
  }
}
#endif
