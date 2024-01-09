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

static String Eval(Block* block,Frame* frame,Arena* temp);
static ValueAndText EvalNonBlockCommand(Command* com,Frame* frame,Arena* temp);
static Expression* ParseExpression(Tokenizer* tok,Arena* temp);

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

  // Inserted after changes, might conflict with previous code that sent a tokenizer that already seen this token
  Byte* mark = tok->Mark();
  tok->AssertNextToken("#{");

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

  tok->AssertNextToken("}");
  com->fullText = tok->Point(mark);
  
  return com;
}

// Crude parser for identifiers
static Expression* ParseIdentifier(Expression* current,Tokenizer* tok,Arena* out){
  void* start = tok->Mark();
  Token firstId = tok->NextToken();

  current->type = Expression::IDENTIFIER;
  current->id = firstId;

  while(1){
    Token token = tok->PeekToken();

    if(CompareString(token,"[")){
      tok->AdvancePeek(token);
      Expression* expr = PushStruct<Expression>(out);
      expr->expressions = PushArray<Expression*>(out,2);

      expr->type = Expression::ARRAY_ACCESS;
      expr->expressions[0] = current;
      expr->expressions[1] = ParseExpression(tok,out);
      expr->text = tok->Point(start);
      expr->approximateLine = tok->lines;
      
      tok->AssertNextToken("]");

      current = expr;
    } else if(CompareString(token,".")){
      tok->AdvancePeek(token);
      Token memberName = tok->NextToken();

      Expression* expr = PushStruct<Expression>(out);
      expr->expressions = PushArray<Expression*>(out,1);

      expr->type = Expression::MEMBER_ACCESS;
      expr->id = memberName;
      expr->expressions[0] = current;
      expr->text = tok->Point(start);
      expr->approximateLine = tok->lines;

      current = expr;
    } else {
      break;
    }
  }

  current->text = tok->Point(start);
  return current;
}

static Expression* ParseAtom(Tokenizer* tok,Arena* out){
  void* start = tok->Mark();

  Expression* expr = PushStruct<Expression>(out);
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
    expr = ParseExpression(tok,out);
    tok->AssertNextToken("}");
  } else if(CompareString(token,"#{")){
    expr->type = Expression::COMMAND;
    expr->command = ParseCommand(tok,out);

    Assert(!IsCommandBlockType(expr->command));
  } else {
    expr->type = Expression::IDENTIFIER;
    expr = ParseIdentifier(expr,tok,out);
  }

  expr->text = tok->Point(start);
  expr->approximateLine = tok->lines;
  return expr;
}

static Expression* ParseFactor(Tokenizer* tok,Arena* out){
  void* start = tok->Mark();

  Token peek = tok->PeekToken();

  Expression* expr = nullptr;
  if(CompareString(peek,"(")){
    tok->AdvancePeek(peek);
    expr = ParseExpression(tok,out);
    tok->AssertNextToken(")");
  } else if(CompareString(peek,"!")){
    tok->AdvancePeek(peek);

    Expression* child = ParseExpression(tok,out);

    expr = PushStruct<Expression>(out);
    expr->expressions = PushArray<Expression*>(out,1);

    expr->type = Expression::OPERATION;
    expr->op = "!";
    expr->expressions[0] = child;

    expr->text = tok->Point(start);
    expr->approximateLine = tok->lines;
  } else {
    expr = ParseAtom(tok,out);
  }

  expr->text = tok->Point(start);
  expr->approximateLine = tok->lines;
  return expr;
}

static Expression* ParseExpression(Tokenizer* tok,Arena* out){
  void* start = tok->Mark();

  Expression* res = ParseOperationType(tok,{{"#"},{"|>"},{"and","or","xor"},{">","<",">=","<=","==","!="},{"+","-"},{"*","/","&","**"}},ParseFactor,out);

  res->text = tok->Point(start);
  return res;
}

static Expression* ParseBlockExpression(Tokenizer* tok,Arena* out){
  Byte* mark = tok->Mark();
  tok->AssertNextToken("@{");

  Expression* expr = ParseExpression(tok,out);

  tok->AssertNextToken("}");
  expr->text = tok->Point(mark);
  
  return expr;
}

struct IndividualBlock{
  String content;
  BlockType type;
  int line;
};

static Array<IndividualBlock> ParseIndividualLine(String line, int lineNumber,Arena* out,Arena* temp){
  if(CompareString(line,"")){
    Array<IndividualBlock> arr = PushArray<IndividualBlock>(out,1);
    arr[0] = {line,BlockType_TEXT,lineNumber};
    return arr;
  }

  Tokenizer tok(line,"}",{"@{","#{"});
  ArenaList<IndividualBlock>* strings = PushArenaList<IndividualBlock>(temp);

  while(1){
    Byte* start = tok.Mark();
    FindFirstResult res = tok.FindFirst({"#{","@{"});
    
    if(res.foundNone){
      String leftover = tok.Finish();
      if(leftover.size > 0){
        *PushListElement(strings) = (IndividualBlock){leftover,BlockType_TEXT,lineNumber};
      }
      break;
    }

    if(res.peekFindNotIncluded.size > 0){
      *PushListElement(strings) = (IndividualBlock){res.peekFindNotIncluded,BlockType_TEXT,lineNumber};
    }

    tok.AdvancePeek(res.peekFindNotIncluded);
    if(CompareString(res.foundFirst,"#{")){
      Byte* mark = tok.Mark();
      Token skip = tok.PeekUntilDelimiterExpression({"#{","@{"},{"}"},0);
      tok.AdvancePeek(skip);
      tok.AssertNextToken("}");
      String command = tok.Point(mark);
      *PushListElement(strings) = (IndividualBlock){command,BlockType_COMMAND,lineNumber};
    } else if(CompareString(res.foundFirst,"@{")){
      Byte* mark = tok.Mark();
      Token skip = tok.PeekUntilDelimiterExpression({"#{","@{"},{"}"},0);
      tok.AdvancePeek(skip);
      tok.AssertNextToken("}");
      String expression = tok.Point(mark);
      *PushListElement(strings) = (IndividualBlock){expression,BlockType_EXPRESSION,lineNumber};
    }
  }

  Array<IndividualBlock> fullySeparated = PushArrayFromList(out,strings);
  return fullySeparated;
}

static void Print(Block* block, int level = 0){
  STACK_ARENA(tempInst,Kilobyte(1));
  Arena* temp = &tempInst; 
  
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
    String escaped = EscapeString(block->textBlock,'_',temp);
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

Array<Block*> ConvertIndividualBlocksIntoHierarchical_(Array<IndividualBlock> blocks,int& index,int level,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
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

      Byte* mark = MarkArena(out);
      Command* command = ParseCommand(&tokenizer,out);

      if(CompareString(command->name,STRING("end"))){
        if(level == 0){
          printf("Error on template engine, found an extra #{end}\n");
          printf("%.*s:%d",UNPACK_SS(globalTemplateName),individualBlock->line);
          Assert(false);
        }
        PopMark(out,mark); // No need to keep parsed command in memory, we do not process #{end}
        goto exit_loop;
      }

      block = PushStruct<Block>(out);
      *block = {};
      
      block->command = command;
      if(IsCommandBlockType(command)){ // Parse subchilds and add them to block.
        block->innerBlocks = ConvertIndividualBlocksIntoHierarchical_(blocks,index,level + 1,out,temp);
      }
    }break;
    case BlockType_EXPRESSION:{
      block = PushStruct<Block>(out);
      *block = {};

      String content = individualBlock->content;
      Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});

      block->expression = ParseBlockExpression(&tokenizer,out);
    }break;
    }

    if(block){
      block->type = individualBlock->type;
      block->line = individualBlock->line;
      *PushListElement(blockList) = block;
    }
  }

 exit_loop:
  Array<Block*> res = PushArrayFromList(out,blockList);
  
  return res;
}

Array<Block*> ConvertIndividualBlocksIntoHierarchical(Array<IndividualBlock> blocks,Arena* out,Arena* temp){
  int index = 0;
  Array<Block*> res = ConvertIndividualBlocksIntoHierarchical_(blocks,index,0,out,temp);

  return res;
}

CompiledTemplate* CompileTemplate(String content,const char* name,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  Byte* mark = MarkArena(out);

  String storedName = PushString(out,STRING(name));
  globalTemplateName = storedName;

  CompiledTemplate* res = PushStruct<CompiledTemplate>(out);

  Tokenizer tokenizer(content,"!()[]{}+-:;.,*~><\"",{"#{","@{","==","!=","**","|>",">=","<=","!="});
  Tokenizer* tok = &tokenizer;
  tok->keepComments = true;

  ArenaList<IndividualBlock>* blockList = PushArenaList<IndividualBlock>(temp);
  Array<String> lines = Split(content,'\n',temp);

  for(int i = 0; i < lines.size; i++){
    String& line = lines[i];
    int lineNumber = i + 1;
    Array<IndividualBlock> sep = ParseIndividualLine(line,lineNumber,temp,out);

    bool containsText = false;
    bool containsCommand = false;
    bool containsExpression = false;
    bool onlyWhitespace = true;
    for(IndividualBlock& s : sep){
      switch(s.type){
      case BlockType_EXPRESSION: containsExpression = true; break;
      case BlockType_COMMAND: containsCommand = true; break;
      case BlockType_TEXT: {
        containsText = true;
        
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
          *PushListElement(blockList) = block;
        }
        Assert(block.type != BlockType_EXPRESSION);
      }
    } else {
      for(IndividualBlock& block : sep){
        *PushListElement(blockList) = block;
      }

      *PushListElement(blockList) = {STRING("\n"),BlockType_TEXT,lineNumber};
    }
  }

  Array<IndividualBlock> blocks = PushArrayFromList(out,blockList);
  Array<Block*> results = ConvertIndividualBlocksIntoHierarchical(blocks,out,temp);

  String totalMemory = PointArena(out,mark);
  res->blocks = results;
  res->totalMemoryUsed = totalMemory.size;
  res->content = content;
  res->name = storedName;

  return res;
}

std::unordered_map<String,PipeFunction> pipeFunctions;

void RegisterPipeOperation(String name,PipeFunction func){
  pipeFunctions.insert({name,func});
}

static Value EvalExpression(Expression* expr,Frame* frame,Arena* temp);

struct DebugValues{
  Expression expr;
  Value val;
};

#ifdef DEBUG_TEMPLATE_ENGINE
static DebugValues savedDebug[64];
static int savedIndex; 
#endif

static Value EvalExpression(Expression* expr,Frame* frame,Arena* temp){
  Value val = {};
  switch(expr->type){
  case Expression::OPERATION:{
    if(expr->op[0] == '|'){ // Pipe operation
      val = EvalExpression(expr->expressions[0],frame,temp);
      
      auto iter = pipeFunctions.find(expr->expressions[1]->id);
      if(iter == pipeFunctions.end()){
        printf("Did not find the following pipe function\n");
        printf("%.*s\n",UNPACK_SS(expr->expressions[1]->id));
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
      FatalError(StaticFormat("Tried to access array at an index greater than size: %d/%d (%.*s)",index.number,IndexableSize(object),UNPACK_SS(object.type->name)),expr->approximateLine);
    }
    val = optVal.value();
  } break;
  case Expression::MEMBER_ACCESS:{
    Value object = EvalExpression(expr->expressions[0],frame,temp);

    Optional<Value> optVal = AccessStruct(object,expr->id);
    if(!optVal){
	  PrintStructDefinition(object.type);
      FatalError(StaticFormat("Tried to access member (%.*s) that does not exist for type (%.*s)",UNPACK_SS(expr->text),UNPACK_SS(object.type->name)),expr->approximateLine);
    }
    val = optVal.value();
  }break;
  default:{
    NOT_IMPLEMENTED;
  } break;
  }

 EvalExpressionEnd:

#ifdef DEBUG_TEMPLATE_ENGINE
  savedDebug[savedIndex].expr = *expr;
  savedDebug[savedIndex].val = val;
  savedIndex = (savedIndex + 1) % 64;
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
    int index = 0;
    bool removeLastOutputSeperator = false;
    for(Iterator iter = Iterate(iterating); HasNext(iter); index += 1,Advance(&iter)){
      SetValue(frame,STRING("index"),MakeValue(index));

      Value val = GetValue(iter);
      SetValue(frame,id,val);

      bool outputSeparator = false;
      for(Block* ptr : block->innerBlocks){
        String text = Eval(ptr,frame,temp);

        if(IsOnlyWhitespace(text)){
          outputArena->used -= text.size;
        } else {
          res.size += text.size; // Push on stack
          outputSeparator = true;
        }
      }

      if(outputSeparator){
        res.size += PushString(outputArena,"%.*s",separator.str.size,separator.str.data).size;
        removeLastOutputSeperator = true;
      }
    }

    if(removeLastOutputSeperator){
      outputArena->used -= separator.str.size;
      res.size -= separator.str.size;
    }
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

      for(Block* ptr : block->innerBlocks){
        res.size += Eval(ptr,frame,temp).size; // Push on stack
      }
    }
  } else if (CompareString(com->name,"if")){
    Value expr = EvalExpression(com->expressions[0],previousFrame,temp);
    Value val = ConvertValue(expr,ValueType::BOOLEAN,nullptr);

    Frame* frame = CreateFrame(previousFrame,temp);
    if(val.boolean){
      for(Block* ptr : block->innerBlocks){
        if(ptr->type == BlockType_COMMAND && CompareString(ptr->command->name,"else")){
          break;
        }
        res.size += Eval(ptr,frame,temp).size; // Push on stack
      }
    } else {
      bool sawElse = false;
      for(Block* ptr : block->innerBlocks){
        if(sawElse){
          res.size += Eval(ptr,frame,temp).size; // Push on stack
        }

        if(ptr->type == BlockType_COMMAND && CompareString(ptr->command->name,"else")){
          Assert(!sawElse); // Cannot have more than one else per if block
          sawElse = true;
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

    for(Block* ptr : block->innerBlocks){
      res.size += Eval(ptr,frame,temp).size; // Push on stack
    }

    debugging = false;
  } else if(CompareString(com->name,"while")){
    Frame* frame = CreateFrame(previousFrame,temp);
    while(ConvertValue(EvalExpression(com->expressions[0],frame,temp),ValueType::BOOLEAN,nullptr).boolean){
      for(Block* ptr : block->innerBlocks){
        res.size += Eval(ptr,frame,temp).size; // Push on stack
      }
    }
  } else if(CompareString(com->name,"define")) {
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
  } else if(CompareString(com->name,"debugMessage")){
    Frame* frame = CreateFrame(previousFrame,temp);
    ArenaMarker marker(outputArena);

    for(Block* ptr : block->innerBlocks){
      String res = Eval(ptr,frame,temp); // Push on stack
      printf("%.*s\n",UNPACK_SS(res));
    }
  } else {
    NOT_IMPLEMENTED;
  }

  return res;
}

extern Array<Pair<String,String>> templateNameToContent; // TODO: Kinda of a quick hack to make this work. Need to revise the way templates are done

// TODO: This is not the prefered way of doing this.
// Maybe just compile the template in malloc'ed memory
// And keep the memory around, it's not like user code would have a reason to free this memory
static CompiledTemplate* savedTemplate = nullptr;
void SetIncludeHeader(CompiledTemplate* tpl,String name){
  savedTemplate = tpl;
}

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

    // Get compiled template and evaluate it.
    CompiledTemplate* templ = savedTemplate;
    for(Block* block : templ->blocks){
      text.size += Eval(block,previousFrame,temp).size;
    }
  } else if(CompareString(com->name,"call")){
    Frame* frame = CreateFrame(previousFrame,temp);
    String id = com->expressions[0]->id;

    Optional<Value> optVal = GetValue(frame,id);
    if(!optVal){
      printf("Failed to find %.*s\n",UNPACK_SS(id));
      DEBUG_BREAK();
    }

    TemplateFunction* func = optVal.value().templateFunction;
    Assert(func->arguments.size == com->expressions.size - 1);

    for(int i = 0; i < func->arguments.size; i++){
      String id = func->arguments[i]->id;

      Value val = EvalExpression(com->expressions[1+i],frame,temp);

      SetValue(frame,id,val);
    }

    for(Block* ptr : func->blocks){
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

  switch(block->type){
  case BlockType_COMMAND:{
    if(IsCommandBlockType(block->command)){
      res = EvalBlockCommand(block,frame,temp);
    } else {
      res = EvalNonBlockCommand(block->command,frame,temp).text;
    }
  }break;
  case BlockType_EXPRESSION:{
    Value val = EvalExpression(block->expression,frame,temp);
    res.size += GetDefaultValueRepresentation(val,outputArena).size;
  } break;
  case BlockType_TEXT:{
    res.size += PushString(outputArena,block->textBlock).size;
  } break;
  }

  Assert(res.size >= 0);
  return res;
}

void InitializeTemplateEngine(Arena* perm){
  globalFrame->table = PushHashmap<String,Value>(perm,99);
  globalFrame->previousFrame = nullptr;
}

void ProcessTemplate(FILE* outputFile,CompiledTemplate* compiledTemplate,Arena* temp){
  BLOCK_REGION(temp);
  Assert(globalFrame && "Call InitializeTemplateEngine first!");

  globalTemplateName = compiledTemplate->name;

  Arena outputArenaInst = SubArena(temp,Megabyte(64));
  outputArena = &outputArenaInst;
  output = outputFile;

  for(Block* block : compiledTemplate->blocks){
    String text = Eval(block,globalFrame,temp);
    fprintf(output,"%.*s",text.size,text.data);
    fflush(output);
  }
}

void PrintTemplate(CompiledTemplate* compiled,Arena* arena){
  NOT_IMPLEMENTED;
}

void PrintAllTypesAndFieldsUsed(CompiledTemplate* compiled,Arena* temp){
  
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
