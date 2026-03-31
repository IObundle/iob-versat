#include "verilogParsing.hpp"

#include "parser.hpp"

#include "filesystem.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "templateEngine.hpp"
#include "embeddedData.hpp"
#include "utilsCore.hpp"

#include "symbolic.hpp"

#include <string>

typedef Value (*MathFunction)(Value f,Value g);

struct MathFunctionDescription{
  String name;
  int amountOfParameters;
  MathFunction func;
};
#define MATH_FUNC(...) [](Value f,Value g) -> Value{__VA_ARGS__}

static MathFunctionDescription verilogMathFunctions[] = {
  {"clog2",1,MATH_FUNC(return MakeValue(log2i(f.number));)},
  {"ln",1},
  {"log",1},
  {"exp",1},
  {"sqrt",1},
  {"pow",2},
  {"floor",1},
  {"ceil",1},
  {"sin",1},
  {"cos",1},
  {"tan",1},
  {"asin",1},
  {"acos",1},
  {"atan",1},
  {"atan2",2},
  {"hypot",2},
  {"sinh",1},
  {"cosh",1},
  {"tanh",1},
  {"asinh",1},
  {"acosh",1},
  {"atanh",1}
};

Opt<MathFunctionDescription> GetMathFunction(String name){
  for(int i = 0; i < ARRAY_SIZE(verilogMathFunctions); i++){
    if(CompareString(verilogMathFunctions[i].name,name)){
      return verilogMathFunctions[i];
    }
  }
  return {};
}

static void PrintExpression(StringBuilder* b,VExpr* exp,int level){
  b->PushSpaces(level);
  switch(exp->type){
  case VExpr::UNDEFINED:{
    b->PushString("UNDEFINED\n");
  }break;
  case VExpr::OPERATION:{
    b->PushString("OPERATION\n");
  }break;
  case VExpr::IDENTIFIER:{
    b->PushString("IDENTIFIER\n");
  }break;
  case VExpr::FUNCTION:{
    b->PushString("FUNCTION\n");
  }break;
  case VExpr::LITERAL:{
    Value val = exp->val;
    b->PushString("LITERAL: %ld\n",val.number);
  }break;
  }

  if(exp->op){
    b->PushSpaces(level);
    b->PushString("op: %s\n",exp->op);
  }

  if(exp->id.size){
    b->PushSpaces(level);
    b->PushString("id: %.*s\n",UN(exp->id));
  }

  for(VExpr* subExpressions : exp->expressions){
    PrintExpression(b,subExpressions,level + 2);
    b->PushString("\n");
  }
}

void PrintExpression(VExpr* exp){
  TEMP_REGION(temp,nullptr);

  auto b = StartString(temp);
  PrintExpression(b,exp,0);

  String res = EndString(temp,b);
  printf("%.*s\n",UN(res));
}

SYM_Expr SymbolicExpressionFromVerilog(VExpr* topExpr){
  SYM_Expr res = SYM_Zero;

  FULL_SWITCH(topExpr->type){
  case VExpr::UNDEFINED: {
    Assert(false);
  } break;
  case VExpr::OPERATION: {
    SYM_Expr left = SymbolicExpressionFromVerilog(topExpr->expressions[0]);
    SYM_Expr right = SymbolicExpressionFromVerilog(topExpr->expressions[1]);
    
    switch(topExpr->op[0]){
    case '+':{
      res = left + right;
    } break;
    case '-':{
      res = left - right;
    } break;
    case '*':{
      res = left * right;
    } break;
    case '/':{
      res = left / right;
    } break;
    default:{
      // TODO: Better error message
      NOT_IMPLEMENTED("");
    } break;
    } 
  } break;
  case VExpr::IDENTIFIER: {
    res = SYM_Var(topExpr->id);
  } break;
  case VExpr::FUNCTION: {
    // TODO: Better error message and we probably can do more stuff here
    NOT_IMPLEMENTED("");
  } break;
  case VExpr::LITERAL: {
    res = SYM_Lit(topExpr->val.number);
  } break;
}
  
  return res;
}

SYM_Expr SymbolicExpressionFromVerilog(ExpressionRange range){
  SYM_Expr top = SymbolicExpressionFromVerilog(range.top);
  SYM_Expr bottom = SymbolicExpressionFromVerilog(range.bottom);

  SYM_Expr res = top - bottom + SYM_One;

  return res;
}

ExternalMemorySymbolic Replace(ExternalMemorySymbolic in,TrieMap<String,SYM_Expr>* replacements){
  ExternalMemorySymbolic res = in;
  
  FULL_SWITCH(res.type){
  case ExternalMemoryType_2P:{
    res.tp.bitSizeIn = SYM_Replace(res.tp.bitSizeIn,replacements);
    res.tp.bitSizeOut = SYM_Replace(res.tp.bitSizeOut,replacements);
    res.tp.dataSizeIn = SYM_Replace(res.tp.dataSizeIn,replacements);
    res.tp.dataSizeOut = SYM_Replace(res.tp.dataSizeOut,replacements);
  } break;
  case ExternalMemoryType_DP:{
    res.dp[0].bitSize = SYM_Replace(res.dp[0].bitSize,replacements);
    res.dp[0].dataSizeIn = SYM_Replace(res.dp[0].dataSizeIn,replacements);
    res.dp[0].dataSizeOut = SYM_Replace(res.dp[0].dataSizeOut,replacements);

    res.dp[1].bitSize = SYM_Replace(res.dp[1].bitSize,replacements);
    res.dp[1].dataSizeIn = SYM_Replace(res.dp[1].dataSizeIn,replacements);
    res.dp[1].dataSizeOut = SYM_Replace(res.dp[1].dataSizeOut,replacements);
  } break;
}

  return res;
}

struct ContentState{
  FileContent content;
  
  const char* start;
  const char* ptr;
  const char* end;
};

enum DefineType{
  DefineType_FUNCTION,
  DefineType_SIMPLE_REPLACE
};

struct DefineInfo{
  DefineType type;

  Array<Token> tokens;
  Array<String> args;
  bool disabled;
};

enum VerilogTokenizerStageType{
  VerilogTokenizerStageType_FILE,
  VerilogTokenizerStageType_DEFINE,
  VerilogTokenizerStageType_COND
};

struct VerilogTokenizerStage{
  VerilogTokenizerStageType type; 
 
  // TODO: Union
  DefineInfo* define;
  int currentDefineToken;

  bool condActive;
  bool condDone;

  ContentState state;
};

struct VerilogTokenizerState{
  Arena* arena;

  Array<VerilogTokenizerStage> stageBuffer;
  int currentStage;

  ArenaList<String>* errors;
  TrieMap<String,DefineInfo>* definesMap;

  VerilogTokenizerStage* CurrentStage(){
    return &stageBuffer[currentStage];
  }
  
  VerilogTokenizerStage* PushStage(VerilogTokenizerStageType type){
    currentStage += 1;
    VerilogTokenizerStage* res = &stageBuffer[currentStage];
    *res = {};
    res->type = type;
    return res;
  }

  void ReportError(String err){
    *errors->PushElem() = err;
  }

  void AccumulateErrors(ArenaList<String>* errors){
    for(String str : errors){
      ReportError(str);
    }
  }

  void PopStage(){
    currentStage -= 1;
  }

  bool IsInsideDefine(){
    bool res = (stageBuffer[currentStage].type == VerilogTokenizerStageType_DEFINE);
    return res;
  }

  Token GetNextDefineToken(){
    auto* stage = CurrentStage();
    Assert(stage->type == VerilogTokenizerStageType_DEFINE);

    DefineInfo* def = stage->define;
    Token toReturn = {};
    if(def->type == DefineType_SIMPLE_REPLACE){
      toReturn = def->tokens[stage->currentDefineToken];
      stage->currentDefineToken += 1;

      if(stage->currentDefineToken >= def->tokens.size){
        PopStage();
      } 
    }

    return toReturn;
  }
  
  String ParseString(){
    ContentState* file = GetCurrentFileState();

    const char* start = file->ptr;
    const char* end = file->end;
    
    String asString = {start,(int) (end-start)};
    auto TokenizeFunction = [](void* tokenizerState) -> Token{
      DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
      const char* start = state->ptr;
      const char* end = state->end;

      TokenizeResult res = ParseWhitespace(start,end);
      res |= ParseComments(start,end);
      res |= ParseIdentifier(start,end);
      res |= ParseCString(start,end);

      int size = res.bytesParsed;
      if(size <= 0){
        size = 1;
      }
      state->ptr += size;
      return res.token;
    };

    FREE_ARENA(parsing);
    Parser* parser = StartParsing(TokenizeFunction,asString,parsing);
    
    Token result = parser->ExpectNext(TokenType_C_STRING);
    
    AccumulateErrors(parser->errors);

    file->ptr = result.cString.data + result.cString.size + 1;

    return result.cString;
  }
  
  String ParseOneIdentifier(){
    ContentState* file = GetCurrentFileState();

    const char* start = file->ptr;
    const char* end = file->end;
    
    String asString = {start,(int) (end-start)};
    auto TokenizeFunction = [](void* tokenizerState) -> Token{
      DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
      const char* start = state->ptr;
      const char* end = state->end;

      TokenizeResult res = ParseWhitespace(start,end);
      res |= ParseComments(start,end);
      res |= ParseIdentifier(start,end);

      int size = res.bytesParsed;
      if(size <= 0){
        size = 1;
      }
      state->ptr += size;
      return res.token;
    };
    
    FREE_ARENA(parsing);
    Parser* parser = StartParsing(TokenizeFunction,asString,parsing);
    
    Token result = parser->ExpectNext(TokenType_IDENTIFIER);
    
    AccumulateErrors(parser->errors);

    file->ptr = result.identifier.data + result.identifier.size;

    return result.identifier;
  };

  void ParseDefine(){
    ContentState* file = GetCurrentFileState();

    const char* start = file->ptr;
    const char* end = file->end;
    
    String asString = {start,(int) (end-start)};
    
    auto TokenizeFunction = [](void* tokenizerState) -> Token{
      DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
      const char* start = state->ptr;
      const char* end = state->end;

      TokenizeResult res = ParseNewline(start,end);
      res |= ParseWhitespace(start,end,ParseWhitespaceOptions_NONE);
      res |= ParseComments(start,end);
      res |= ParseVerilogPreprocess(start,end);
      res |= ParseSymbols(start,end);
      res |= ParseNumber(start,end);
      res |= ParseIdentifier(start,end);

      int size = res.bytesParsed;
      if(size <= 0){
        size = 1;
      }
      state->ptr += size;
      return res.token;
    };

    FREE_ARENA(parsing);
    Parser* parser = StartParsing(TokenizeFunction,asString,parsing);

    TEMP_REGION(temp,nullptr);

    parser->IfNextToken(TokenType_VERILOG_DEFINE);
    Token id = parser->ExpectNext(TokenType_IDENTIFIER);
    
    parser->SetOptions(ParsingOptions_SKIP_COMMENTS);
    
    Token peek = parser->PeekToken();
    
    if(peek.type == TokenType_WHITESPACE){
      peek = parser->NextToken();
    }

    if(peek.type == TokenType_NEWLINE){
      // Only define, no text.
      DefineInfo info = {};
      info.type = DefineType_SIMPLE_REPLACE;
      info.tokens = {};
      info.args = {};
      definesMap->Insert(id.identifier,info);

      AccumulateErrors(parser->errors);

      file->ptr = peek.originalData.data;
    } else {
      Array<String> funcArgs = {};
      if(parser->IfNextToken('(')){
        auto argList = PushList<String>(temp);
      
        parser->SetOptions(ParsingOptions_DEFAULT);

        while(!parser->Done()){
          Token t = parser->ExpectNext(TokenType_IDENTIFIER);

          *argList->PushElem() = t.identifier;
        
          if(parser->IfNextToken(',')){
            continue;
          }

          if(parser->IfPeekToken(')')){
            break;
          }
        }
      
        parser->ExpectNext(')');
        funcArgs = PushArray(arena,argList);
      }
      parser->IfNextToken(TokenType_WHITESPACE);

      parser->SetOptions(ParsingOptions_DEFAULT);

      auto list = PushList<Token>(temp);
      
      Token newLineToken = {};
      bool foundNewLine = false;
      bool foundBackslash = false;
      while(!parser->Done()){
        Token token = parser->NextToken();

        if(token.type == '\\'){
          foundBackslash = true;
          continue;
        }

        if(token.type == TokenType_NEWLINE){
          if(foundBackslash){
            foundBackslash= false;
            continue;
          }

          foundNewLine = true;
          newLineToken = token;
          break;
        }

        if(foundBackslash){
          // TODO: How to handle any other type of token after a backslash?
        }

        *list->PushElem() = token;
      }
    
      DefineInfo info = {};
      info.type = DefineType_SIMPLE_REPLACE;
      info.tokens = PushArray(arena,list);
      info.args = funcArgs;
      definesMap->Insert(id.identifier,info);

      int size = newLineToken.originalData.data - start;
      if(!foundNewLine){
        size = end - start;
      }

      AccumulateErrors(parser->errors);

      file->ptr += size;
    }
  }

  void DisableDefine(String defineName){
    DefineInfo* res = definesMap->Get(defineName);

    if(!res){
      return;
    }

    res->disabled = true;
  }

  void StepInsideDefine(String defineName){
    DefineInfo* res = definesMap->Get(defineName);

    if(!res){
      ReportError(SF("Unknown define encountered: %.*s\n",UN(defineName)));
      return;
    }

    if(res->tokens.size == 0){
      return;
    }
    
    VerilogTokenizerStage* stage = PushStage(VerilogTokenizerStageType_DEFINE);
    stage->define = res;
  }

  bool DefineNameExists(String defineName){
    DefineInfo* res = definesMap->Get(defineName);

    if(!res){
      return false;
    }
    
    return res->disabled;
  }

  ContentState* GetCurrentFileState(){
    for(int i = currentStage; i >= 0; i--){
      VerilogTokenizerStage* stage = &stageBuffer[i];

      if(stage->type == VerilogTokenizerStageType_FILE){
        return &stage->state;
      }
    }

    // Stage 0 should always be a file so we should never reach this point ever.
    Assert(false);
    return nullptr;
  }
};


// MARK1

#if 1

static Value Eval(VExpr* expr,TrieMap<String,Value>* map){
  switch(expr->type){
  case VExpr::OPERATION:{
    Value val1 = Eval(expr->expressions[0],map);
    Value val2 = Eval(expr->expressions[1],map);

    switch(expr->op[0]){
    case '+':{
      Value res = MakeValue(val1.number + val2.number);
      return res;
    }break;
    case '-':{
      Value res = MakeValue(val1.number - val2.number);
      return res;
    }break;
    case '*':{
      Value res = MakeValue(val1.number * val2.number);
      return res;
    }break;
    case '/':{
      Value res = MakeValue(val1.number / val2.number);
      return res;
    }break;
    default:{
      NOT_IMPLEMENTED("Implemented as needed");
    } break;
    }
  }break;
  case VExpr::IDENTIFIER:{
    Value* val = map->Get(expr->id);
    if(!map){
      printf("Error, did not find parameter %.*s\n",UN(expr->id));
      NOT_IMPLEMENTED("Need to also report file position and stuff like that, similar to parser tokenizer error");
    }
    
    return *val;
  }break;
  case VExpr::LITERAL:{
    return expr->val;
  }break;
  case VExpr::FUNCTION:{ // Called Command but for verilog it is actually a math function call
    String functionName = expr->id;

    VExpr* argument = expr->expressions[0];
    MathFunctionDescription optDescription = GetMathFunction(functionName).value();

    if(!optDescription.func){
      printf("Verilog Math function not currently implemented: %.*s",UN(functionName));
      exit(0);
    }

    // NOTE: For now, hardcoded to 1 argument functions
    Value argumentValue = Eval(argument,map);
    Value result = optDescription.func(argumentValue,MakeValue());
    
    return result;
  } break;
  case VExpr::UNDEFINED:
    NOT_POSSIBLE("None of these should appear in parameters");
  }

  NOT_POSSIBLE("Implemented as needed");
  return MakeValue();
}

static ExpressionRange ParseRange(Parser* tok,Arena* out);
VExpr* VerilogParseExpression(Parser* parser,Arena* out,int bindingPower = 99);

static Array<ParameterExpression> ParseParameters(Parser* tok,TrieMap<String,Value>* map,Arena* out){
  //TODO: Add type and range to parsing
  /*
	Range currentRange;
	ParameterType type;
   */
  TEMP_REGION(temp,out);

  auto params = PushList<ParameterExpression>(temp);

  // TODO: Not used but must parse it anyway.
  ExpressionRange range = {};
  ParamFlags flags = {};
  while(1){
    Token peek = tok->PeekToken();

    if(peek.type == TokenType_VERILOG_KEYWORD_PARAMETER){
      auto oldOptions = tok->SetOptions(ParsingOptions_SKIP_WHITESPACE);

      Token possibleComment = tok->PeekToken();

      // NOTE: We allow a comment immediatly before the parameter or after.
      // TODO: Need to properly test this.
      if(possibleComment.type == TokenType_VERILOG_KEYWORD_PARAMETER){
        tok->NextToken();
        possibleComment = tok->PeekToken();
      }
      
      tok->SetOptions(oldOptions);

      if(possibleComment.type == TokenType_COMMENT){
        tok->NextToken();

        auto TokenizeFunction = [](void* tokenizerState) -> Token{
          DefaultTokenizerState* state = (DefaultTokenizerState*) tokenizerState;
    
          const char* start = state->ptr;
          const char* end = state->end;

          if(start >= end){
            Token token = {};
            token.type = TokenType_EOF;
            return token;
          }

          TokenizeResult res = ParseWhitespace(start,end);
          res |= ParseComments(start,end);
          res |= ParseSymbols(start,end);
          res |= ParseNumber(start,end);
          res |= ParseIdentifier(start,end);

          int size = res.bytesParsed;
          if(size <= 0 && state->ptr != state->end){
            size = 1;
          }

          state->ptr += size;

          // NOTE: Something very bad must happen to the point where the file is 1 byte after the end.
          //       We expect it to only reach file->end, not file->end + 1
          Assert(state->ptr < state->end + 1);

          return res.token;
        };

        FREE_ARENA(commentParsing);
        Parser* p = StartParsing(TokenizeFunction,possibleComment.comment,commentParsing);

        Token t = p->NextToken();
        if(t.type == TokenType_IDENTIFIER && t.identifier == "versat"){
          p->ExpectNext(':');
          
          while(!p->Done()){
            p->IfNextToken(',');
          
            Token paramFlag = p->ExpectNext(TokenType_IDENTIFIER);
            Opt<ParamFlags> flagOpt = META_ParamToFlag_ReverseMap(paramFlag.identifier);

            if(flagOpt.has_value()){
              // Better support for flag concatenation when using enums.
              flags = (ParamFlags) ((u32) flags | (u32) flagOpt.value());
            } else {
              // TODO: Better error reporting
              printf("%.*s is not a valid param flag",UN(paramFlag.identifier));
            }
          }
        }
        
        for(String err : p->errors){
          tok->ReportError(err);
        }
      }

      tok->IfNextToken(TokenType_VERILOG_KEYWORD_SIGNED);

      range = ParseRange(tok,out);

      Token paramName = tok->NextToken();

      tok->ExpectNext('=');

      Token peek = tok->PeekToken();
      // TODO: Kinda hacky way of parsing strings, we do not care about them but must parse them anyway.
      if(peek.type == '\"'){
        tok->NextToken();
        while(!tok->Done()){
          Token token = tok->PeekToken();

          if(token.type == '\"'){
            break;
          }

          tok->NextToken();
        }
        tok->ExpectNext('\"');
        
        continue;
      }
      
      VExpr* expr = VerilogParseExpression(tok,out);
      Value val = Eval(expr,map);

      map->Insert(paramName.identifier,val);
      
      ParameterExpression* p = params->PushElem();
      p->name = paramName.identifier;
      p->expr = expr;
      p->flags = flags;
    } else if(peek.type == ')'){
      break;
    } else if(peek.type == ';'){ // To parse inside module parameters, technically wrong but harmless
      tok->NextToken();
      break;
    } else if(peek.type == ','){
      tok->NextToken();
      continue;
    }
  }

  return PushArray(out,params);
}

VExpr* VerilogParseExpression(Parser* parser,Arena* out,int bindingPower){
  VExpr* topUnary = nullptr;
  VExpr* innerMostUnary = nullptr;

  VExpr* res = nullptr;
  
  // Parse unary
  while(!parser->Done()){
    VExpr* parsed = nullptr;
    if(!parsed &&  parser->IfNextToken('-')){
      parsed = PushStruct<VExpr>(out);
      parsed->op = "-";
    }

    if(parsed){
      parsed->type = VExpr::OPERATION;
    }

    if(parsed && !topUnary){
      topUnary = parsed;
      innerMostUnary = parsed;
      continue;
    }

    if(parsed){
      innerMostUnary->expressions = PushArray<VExpr*>(out,1);
      innerMostUnary->expressions[0] = parsed;
      innerMostUnary = parsed;
      continue;
    }

    break;
  }

  // Parse atom
  Token peek = parser->PeekToken();
  if(peek.type == '('){
    parser->ExpectNext('(');

    res = VerilogParseExpression(parser,out);

    parser->ExpectNext(')');
  } else if(peek.type == TokenType_NUMBER){
    Token number = parser->ExpectNext(TokenType_NUMBER);
    res = PushStruct<VExpr>(out);

    res->type = VExpr::LITERAL;
    res->val = MakeValue(number.number);
  } else if(peek.type == TokenType_IDENTIFIER){
    Token id = parser->ExpectNext(TokenType_IDENTIFIER);
   
    res = PushStruct<VExpr>(out);
    res->type = VExpr::IDENTIFIER;
    res->id = id.identifier;
  } else if (peek.type == TokenType_C_STRING) {
    Token string = parser->ExpectNext(TokenType_C_STRING);
    
    res = PushStruct<VExpr>(out);
    res->val = MakeValue(string.cString);
    res->type = VExpr::LITERAL;
  } else if(peek.type == '$'){
    VExpr* expr = PushStruct<VExpr>(out);
    *expr = {};

    parser->NextToken();

    expr->id = parser->ExpectNext(TokenType_IDENTIFIER).identifier;

    Opt<MathFunctionDescription> optDescription = GetMathFunction(expr->id);
    Assert(optDescription.has_value());
    MathFunctionDescription description = optDescription.value();

    expr->type = VExpr::FUNCTION;
    expr->expressions = PushArray<VExpr*>(out,description.amountOfParameters);

    parser->ExpectNext('(');
    expr->expressions[0] = VerilogParseExpression(parser,out);
    if(description.amountOfParameters == 2){
      parser->ExpectNext(',');
      expr->expressions[1] = VerilogParseExpression(parser,out);
    }
    
    parser->ExpectNext(')');
    
  } else {
    // TODO: Better error reporting
    parser->ReportUnexpectedToken(peek,{});
  }

  if(topUnary){
    innerMostUnary->expressions = PushArray<VExpr*>(out,1);
    innerMostUnary->expressions[0] = res;

    res = topUnary;
  }

  struct OpInfo{
    TokenType type;
    int bindingPower;
    const char* op;
  };

  // TODO: This should be outside the function itself.
  TEMP_REGION(temp,out);
  auto infos = PushArray<OpInfo>(temp,7);

  // nocheckin: TODO: We are missing a couple of operations and need to double check 
  // TODO: Need to double check binding power
  infos[0] = {TOK_TYPE('&'),0,"&"};
  infos[1] = {TOK_TYPE('|'),0,"|"};
  infos[2] = {TOK_TYPE('^'),0,"^"};

  infos[3]  = {TOK_TYPE('*'),1,"*"};
  infos[4] = {TOK_TYPE('/'),1,"/"};

  infos[5] = {TOK_TYPE('+'),2,"+"};
  infos[6] = {TOK_TYPE('-'),2,"-"};
  
  // Parse binary ops.
  while(!parser->Done()){
    Token peek = parser->PeekToken();

    bool continueOuter = false;
    for(OpInfo info : infos){
      if(peek.type == info.type){
        if(info.bindingPower < bindingPower){
          parser->NextToken();

          VExpr* right = VerilogParseExpression(parser,out,info.bindingPower);
      
          VExpr* op = PushStruct<VExpr>(out);

          op->op = info.op;
          op->type = VExpr::OPERATION;
          op->expressions = PushArray<VExpr*>(out,2);
          op->expressions[0] = res;
          op->expressions[1] = right;

          res = op;
          continueOuter = true;
          break;
        }
      }
    }

    if(continueOuter){
      continue;
    }

    break;
  }

  // Parse ternary ops.
  if(parser->IfNextToken('?')){
    VExpr* first = VerilogParseExpression(parser,out);

    parser->ExpectNext(':');

    VExpr* second = VerilogParseExpression(parser,out);
    
    VExpr* ternary = PushStruct<VExpr>(out);

    ternary->op = "?";
    ternary->type = VExpr::OPERATION;
    ternary->expressions = PushArray<VExpr*>(out,3);
    ternary->expressions[0] = res;
    ternary->expressions[1] = first;
    ternary->expressions[2] = second;

    res = ternary;
  }

  return res;
}

static ExpressionRange ParseRange(Parser* tok,Arena* out){
  static VExpr zeroExpression = {};

  Token peek = tok->PeekToken();

  if(peek.type != '['){ // No range is equal to range [0:0]. Do not known if it's worth/need to  differentiate
    ExpressionRange range = {};

    zeroExpression.type = VExpr::LITERAL;
    zeroExpression.val = MakeValue(0);
    
    range.top = &zeroExpression;
    range.bottom = &zeroExpression;
    
    return range;
  }

  tok->ExpectNext('[');

  ExpressionRange res = {};
  res.top = VerilogParseExpression(tok,out);

  tok->ExpectNext(':');

  res.bottom = VerilogParseExpression(tok,out);
  tok->ExpectNext(']');

  return res;
}

static Module ParseModule(Parser* tok,Arena* out){
  TEMP_REGION(temp,out);

  Module module = {};

  TrieMap<String,Value>* values = PushTrieMap<String,Value>(temp);

  tok->ExpectNext(TokenType_VERILOG_KEYWORD_MODULE);

  module.name = tok->ExpectNext(TokenType_IDENTIFIER).identifier;

  //NewToken peek = C(tok->PeekToken());
  if(tok->IfNextToken('#')){
    tok->ExpectNext('(');
    module.parameters = ParseParameters(tok,values,out);
    tok->ExpectNext(')');
  }

  tok->ExpectNext('(');
  if(!tok->IfPeekToken(')')){

    ArenaList<PortDeclaration>* portList = PushList<PortDeclaration>(temp);
    // Parse ports
    while(!tok->Done()){
      Token peek = tok->PeekToken();

      PortDeclaration port;
      ArenaList<Pair<String,Value>>* attributeList = PushList<Pair<String,Value>>(temp);
    
      if(peek.type == TokenType_VERILOG_ATTRIBUTE_START){
        tok->NextToken();
        while(!tok->Done()){
          Token attributeName = tok->ExpectNext(TokenType_IDENTIFIER);

#if 1
          if(attributeName.type == TokenType_IDENTIFIER){
            if(!Contains(possibleAttributes,attributeName.identifier)){
              printf("ERROR: Do not know attribute named: %.*s\n",UN(attributeName.identifier));
              exit(-1);
            }
          }
#endif

          Token peek = tok->PeekToken();
          if(peek.type == '='){
            tok->NextToken();
            VExpr* expr = VerilogParseExpression(tok,out);
            Value value = Eval(expr,values);

            *attributeList->PushElem() = {attributeName.identifier,value};

            peek = tok->PeekToken();
          } else {
            *attributeList->PushElem() = {attributeName.identifier,MakeValue()};
          }

          if(peek.type == ','){
            tok->NextToken();
            continue;
          }
          if(peek.type == TokenType_VERILOG_ATTRIBUTE_END){
            tok->NextToken();
            break;
          }
        }
      }
      port.attributes = PushHashmapFromList(out,attributeList);

      Token portType = tok->NextToken();
      if(portType.type == TokenType_VERILOG_KEYWORD_INPUT){
        port.type = WireDir_INPUT;
      } else if(portType.type == TokenType_VERILOG_KEYWORD_OUTPUT){
        port.type = WireDir_OUTPUT;
      } else if(portType.type == TokenType_VERILOG_KEYWORD_INOUT){
        port.type = WireDir_INOUT;
      } else {
        UNHANDLED_ERROR("TODO: Should be a handled error");
      }

      // TODO: Add a new function to parser to "ignore" the following list of tokens (loop every time until it doesn't find one from the list), and replace this function here with reg and all the different types it can be
      while(1){
        Token peek = tok->PeekToken();
        if(peek.type == TokenType_VERILOG_KEYWORD_REG){
          tok->NextToken();
          continue;
        }
        if(peek.type == TokenType_VERILOG_KEYWORD_SIGNED){
          tok->NextToken();
          continue;
        }
        break;
      }

      ExpressionRange res = ParseRange(tok,out);
      port.range = res;
      port.name = tok->ExpectNext(TokenType_IDENTIFIER).identifier;

      *portList->PushElem() = port;

      peek = tok->PeekToken();
      if(peek.type == ')'){
        tok->NextToken();
        break;
      }

      tok->ExpectNext(',');
    }
    module.ports = PushArray(out,portList);
  }

  while(!tok->Done()){
    if(tok->IfNextToken(TokenType_VERILOG_KEYWORD_ENDMODULE)){
      break;
    }

    tok->NextToken();
  }

  return module;
}

Token VerilogTokenizer(void* tokenizerState){
  VerilogTokenizerState* state = (VerilogTokenizerState*) tokenizerState;

  TokenizeResult res = {};
  while(1){
    if(state->IsInsideDefine()){
      return state->GetNextDefineToken();
    }
    
    ContentState* file = state->GetCurrentFileState();
      
    const char* start = file->ptr;
    const char* end = file->end;

    res = {};
    if(start >= end){
      res.token.type = TokenType_EOF;
      if(state->currentStage == 0){
        break;
      } else {
        state->PopStage();
        continue;
      }
    }

    res |= ParseWhitespace(start,end);
    res |= ParseComments(start,end);
    res |= ParseVerilogPreprocess(start,end);

    res |= ParseCString(start,end);

    res |= ParseMultiSymbol(start,end,"(*",TokenType_VERILOG_ATTRIBUTE_START);
    res |= ParseMultiSymbol(start,end,"*)",TokenType_VERILOG_ATTRIBUTE_END);

    res |= ParseSymbols(start,end);
    res |= ParseNumber(start,end);
    res |= ParseIdentifier(start,end);

    if(res.token.type == TokenType_IDENTIFIER){
#define VKEYWORD(NAME,TYPE) if(res.token.identifier == NAME){ \
      res.token.type = TYPE; \
      }

VKEYWORD("module",TokenType_VERILOG_KEYWORD_MODULE);
VKEYWORD("endmodule",TokenType_VERILOG_KEYWORD_ENDMODULE);
VKEYWORD("parameter",TokenType_VERILOG_KEYWORD_PARAMETER);
VKEYWORD("signed",TokenType_VERILOG_KEYWORD_SIGNED);
VKEYWORD("input",TokenType_VERILOG_KEYWORD_INPUT);
VKEYWORD("output",TokenType_VERILOG_KEYWORD_OUTPUT);
VKEYWORD("inout",TokenType_VERILOG_KEYWORD_INOUT);
VKEYWORD("reg",TokenType_VERILOG_KEYWORD_REG);
VKEYWORD("wire",TokenType_VERILOG_KEYWORD_WIRE);

#undef VKEYWORD
    }

    TokenType t = res.token.type;

    bool isTokenPreprocess = (t >= TokenType_START_OF_VERILOG_PREPROCESS && 
                              t < TokenType_END_OF_VERILOG_PREPROCESS);
    bool validToken = !isTokenPreprocess;
      
    int bytesToAdvance = res.bytesParsed;

    if(isTokenPreprocess){
      file->ptr += bytesToAdvance;
      bytesToAdvance = 0;
    }

    if(t == TokenType_VERILOG_TIMESCALE){
      // We do not care about timescales. We can just ignore it.
    }
    if(t == TokenType_VERILOG_DEFINE){
      state->ParseDefine();
    } 
    if(t == TokenType_VERILOG_UNDEF){
      String id = state->ParseOneIdentifier();

      state->DisableDefine(id);
    }
    if(t == TokenType_VERILOG_INCLUDE){
      String filename = state->ParseString();

      FileContent content = GetContentsOfFile(filename,FilePurpose_VERILOG_INCLUDE);

      if(content.state == FileContentState_FAILED_TO_LOAD){
        state->ReportError(SF("Cannot find include file '%.*s'",UN(filename)));
      } else {
        VerilogTokenizerStage* newStage = state->PushStage(VerilogTokenizerStageType_FILE);
        
        newStage->state.content = content;
        newStage->state.start = content.content.data;
        newStage->state.ptr = content.content.data;
        newStage->state.end = content.content.data + content.content.size;
      }
    } 
    if(t == TokenType_VERILOG_PREPROCESS){
      String defineIdentifier = res.token.identifier;
        
      state->StepInsideDefine(defineIdentifier);
    }
    if(t == TokenType_VERILOG_IFDEF || t == TokenType_VERILOG_IFNDEF){
      bool checkIfExists = (t == TokenType_VERILOG_IFDEF);
        
      String identifierToCheck = state->ParseOneIdentifier();

      VerilogTokenizerStage* stage = state->PushStage(VerilogTokenizerStageType_COND);

      bool exists = state->DefineNameExists(identifierToCheck);
      stage->type = VerilogTokenizerStageType_COND;
      stage->condActive = (checkIfExists == exists);
    }
    if(t == TokenType_VERILOG_ELSE){
      VerilogTokenizerStage* stage = state->CurrentStage();

      if(stage->type != VerilogTokenizerStageType_COND){
        state->ReportError("Else preprocessing found without associated if");
      } else {
        if(stage->condActive){
          stage->condDone = true;
        }

        if(stage->condDone){
          stage->condActive = false;
        }
      }
    }
    if(t == TokenType_VERILOG_ELSIF){
      VerilogTokenizerStage* stage = state->CurrentStage();

      if(stage->type != VerilogTokenizerStageType_COND){
        state->ReportError("Else preprocessing found without associated if");
      } else {
        String identifierToCheck = state->ParseOneIdentifier();

        if(stage->condActive){
          stage->condDone = true;
        }

        if(stage->condDone){
          stage->condActive = false;
        } else {
          bool exists = state->DefineNameExists(identifierToCheck);
            
          stage->condActive = exists;
        }
      }
    }
    if(t == TokenType_VERILOG_ENDIF){
      VerilogTokenizerStage* stage = state->CurrentStage();
        
      if(stage->type != VerilogTokenizerStageType_COND){
        state->ReportError("Endif found not associated to any if.");
      } else {
        state->PopStage();
      }
    }

    file->ptr += bytesToAdvance;

    // NOTE: Something very bad must happen to the point where the file is 1 byte after the end.
    //       We expect it to only reach file->end, not file->end + 1
    Assert(file->ptr < file->end + 1);
      
    if(validToken){
      break;
    }
  }

  return res.token;
}

Array<Module> ParseVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out){
  TEMP_REGION(temp,out);

#if 0
  Tokenizer tokenizer = Tokenizer(fileContent,"\n:',()[]{}\"+-/*=",{"#(","+:","-:","(*","*)"});
  Tokenizer* tok = &tokenizer;
#endif
  
  FREE_ARENA(tokenizer);
  FREE_ARENA(parsing);

  VerilogTokenizerState* state = PushStruct<VerilogTokenizerState>(tokenizer);

  // nocheckin: TODO: Since we now have the filesystem return the file content, we can just rewrite this function to receive a filepath instead of receiving the content directly.

  state->arena = tokenizer;
  state->definesMap = PushTrieMap<String,DefineInfo>(parsing);
  state->stageBuffer = PushArray<VerilogTokenizerStage>(parsing,16);
  state->errors = PushList<String>(parsing);
  state->currentStage = 0;

  state->stageBuffer[0].type = VerilogTokenizerStageType_FILE;

  ContentState* first = &state->stageBuffer[0].state;

  FileContent fileLike = {};
  fileLike.content = fileContent;
  fileLike.fileName = "";
  fileLike.state = FileContentState_OK;

  first->content = fileLike;
  first->start = fileContent.data;
  first->ptr = first->start;
  first->end = fileContent.data + fileContent.size;

  Parser* parser = StartParsing(VerilogTokenizer,state,parsing);

  ArenaList<Module>* modules = PushList<Module>(temp);

  bool isSource = false;
  while(!parser->Done()){
    Token peek = parser->PeekToken();
    
    if(peek.type == TokenType_VERILOG_ATTRIBUTE_START){
      parser->NextToken();

      Token attribute = parser->ExpectNext(TokenType_IDENTIFIER);

      if(attribute.type == TokenType_IDENTIFIER && attribute.identifier == "source"){
        isSource = true;
      } else {
        // TODO: Report unused attribute.
        //NOT_IMPLEMENTED("Should not give an error"); // Unknown attribute, error for now
      }

      parser->ExpectNext(TokenType_VERILOG_ATTRIBUTE_END);

      continue;
    }

    if(peek.type == TokenType_VERILOG_KEYWORD_MODULE){
      Module module = ParseModule(parser,out);

      module.isSource = isSource;
      *modules->PushElem() = module;
      
      isSource = false;
    }

    parser->NextToken();
  }

  for(String error : state->errors){
    printf("%.*s\n",UN(error));
  }

  for(String error : parser->errors){
    printf("%.*s\n",UN(error));
  }

  return PushArray(out,modules);
}

ModuleInfo ExtractModuleInfo(Module& module,Arena* out){
  TEMP_REGION(temp,out);

  ModuleInfo info = {};

  info.defaultParameters = module.parameters;

  auto inputs = StartArray<PortInfo>(out);
  auto outputs = StartArray<PortInfo>(out);
  auto configs = StartArray<WireExpression>(out);
  auto states = StartArray<WireExpression>(out);

  info.name = module.name;
  info.isSource = module.isSource;

  auto* external = PushTrieMap<ExternalMemoryID,ExternalMemoryInfo>(temp);
  
  for(PortDeclaration decl : module.ports){
    String name = decl.name;
    
    if(CompareString("signal_loop",decl.name)){
      info.singleInterfaces |= SingleInterfaces_SIGNAL_LOOP;
    } else if(CheckFormat("ext_dp_%s_%d_port_%d",decl.name)){
      Array<Value> values = ExtractValues("ext_dp_%s_%d_port_%d",decl.name,temp);

      ExternalMemoryID id = {};
      id.interface = values[1].number;
      id.type = ExternalMemoryType_DP;

      String wire = values[0].str;
      int port = values[2].number;

      Assert(port < 2);

      ExternalMemoryInfo* ext = external->GetOrInsert(id,{});
      if(CompareString(wire,"addr")){
        ext->dp[port].bitSize = decl.range; //SymbolicExpressionFromVerilog(decl.range,out); // decl.range;
      } else if(CompareString(wire,"out")){
        ext->dp[port].dataSizeOut = decl.range;
      } else if(CompareString(wire,"in")){
        ext->dp[port].dataSizeIn = decl.range;
      } else if(CompareString(wire,"write")){
        ext->dp[port].write = true;
      } else if(CompareString(wire,"enable")){
        ext->dp[port].enable = true;
      }
    } else if(CheckFormat("ext_2p_%s",decl.name)){
      ExternalMemoryID id = {};
      id.type = ExternalMemoryType_2P;

      String wire = {};
	  bool out = false;
      if(CheckFormat("ext_2p_%s_%s_%d",decl.name)){
        Array<Value> values = ExtractValues("ext_2p_%s_%s_%d",decl.name,temp);

        wire = values[0].str;
		String outOrIn = values[1].str;
		if(CompareString(outOrIn,"out")){
		  out = true;
		} else if(CompareString(outOrIn,"in")){
		  out = false;
		} else {
		  Assert(false && "Either out or in is mispelled or not present\n");
		}
        id.interface = values[2].number;
      } else if(CheckFormat("ext_2p_%s_%d",decl.name)){
        Array<Value> values = ExtractValues("ext_2p_%s_%d",decl.name,temp);

        wire = values[0].str;
        id.interface = values[1].number;
      } else {
        UNHANDLED_ERROR("TODO: Should be an handled error");
      }

      ExternalMemoryInfo* ext = external->GetOrInsert(id,{});

      if(CompareString(wire,"addr")){
		if(out){
		  ext->tp.bitSizeOut = decl.range;
		} else {
          ext->tp.bitSizeIn = decl.range; // We are using the second port to store the address despite the fact that it's only one port. It just has two addresses.
		}
      } else if(CompareString(wire,"data")){
		if(out){
          ext->tp.dataSizeOut = decl.range;
		} else {
          ext->tp.dataSizeIn = decl.range;
		}
      } else if(CompareString(wire,"write")){
        ext->tp.write = true;
      } else if(CompareString(wire,"read")){
        ext->tp.read = true;
      } else {
        UNHANDLED_ERROR("Should be an handled error");
      }
    } else if(CheckFormat("in%d",decl.name)){
      name = Offset(name,2);
      int index = ParseInt(name);
      Value* delayValue = decl.attributes->Get(VERSAT_LATENCY);

      int delay = 0;
      if(delayValue) delay = delayValue->number;

      inputs[index].delay = delay;
      inputs[index].range = decl.range;
    } else if(CheckFormat("out%d",decl.name)){
      name = Offset(name,3);
      int index = ParseInt(name);
      Value* latencyValue = decl.attributes->Get(VERSAT_LATENCY);

      int latency = 0;
      if(latencyValue) latency = latencyValue->number;

      outputs[index].delay = latency;
      outputs[index].range = decl.range;
    } else if(CheckFormat("delay%d",decl.name)){
      name = Offset(name,5);
      int delay = ParseInt(name);

      info.nDelays = MAX(info.nDelays,delay + 1);
    } else if(  CheckFormat("databus_ready_%d",decl.name)
				|| CheckFormat("databus_valid_%d",decl.name)
				|| CheckFormat("databus_addr_%d",decl.name)
				|| CheckFormat("databus_rdata_%d",decl.name)
				|| CheckFormat("databus_wdata_%d",decl.name)
				|| CheckFormat("databus_wstrb_%d",decl.name)
				|| CheckFormat("databus_len_%d",decl.name)
				|| CheckFormat("databus_last_%d",decl.name)){
      Array<Value> val = ExtractValues("databus_%s_%d",decl.name,temp);

      if(CheckFormat("databus_addr_%d",decl.name)){
        info.databusAddrSize = decl.range;
      }

      info.nIO = val[1].number;
      info.doesIO = true;
    } else if(CheckFormat("rvalid",decl.name)
		   || CheckFormat("valid",decl.name)
		   || CheckFormat("addr",decl.name)
		   || CheckFormat("rdata",decl.name)
		   || CheckFormat("wdata",decl.name)
		   || CheckFormat("wstrb",decl.name)){
      info.memoryMapped = true;

      if(CheckFormat("addr",decl.name)){
        info.memoryMappedBits = decl.range;
      }
    } else if(CheckFormat("clk",decl.name)){
      info.singleInterfaces |= SingleInterfaces_CLK;
    } else if(CheckFormat("rst",decl.name)){
      info.singleInterfaces |= SingleInterfaces_RESET;
    } else if(CheckFormat("run",decl.name)){
      info.singleInterfaces |= SingleInterfaces_RUN;
    } else if(CheckFormat("running",decl.name)){
      info.singleInterfaces |= SingleInterfaces_RUNNING;
    } else if(CheckFormat("done",decl.name)){
      info.singleInterfaces |= SingleInterfaces_DONE;
    } else if(decl.type == WireDir_INPUT){ // Config
      WireExpression* wire = configs.PushElem();

      Value* stageValue = decl.attributes->Get(VERSAT_STAGE);

      VersatStage stage = VersatStage_COMPUTE;
      
      if(stageValue && stageValue->type == ValueType_STRING){
        if(CompareString(stageValue->str,"Write")){
          stage = VersatStage_WRITE;
        } else if(CompareString(stageValue->str,"Read")){
          stage = VersatStage_READ;
        } 
      }
      
      wire->bitSize = decl.range;
      wire->name = decl.name;
      wire->isStatic = decl.attributes->Exists(VERSAT_STATIC);
      wire->stage = stage;
    } else if(decl.type == WireDir_OUTPUT){ // State
      WireExpression* wire = states.PushElem();

      wire->bitSize = decl.range;
      wire->name = decl.name;
    } else {
      NOT_IMPLEMENTED("Implemented as needed, so far all if cases handles all cases so we should never reach here");
    }
  }

  info.configs = configs.AsArray();
  info.states = states.AsArray();
  info.inputs = inputs.AsArray();
  info.outputs = outputs.AsArray();

  if(info.doesIO){
    info.nIO += 1;
  }

  Array<ExternalMemoryInterfaceExpression> interfaces = PushArray<ExternalMemoryInterfaceExpression>(out,external->inserted);
  int index = 0;
  for(Pair<ExternalMemoryID,ExternalMemoryInfo> pair : external){
    ExternalMemoryInterfaceExpression& inter = interfaces[index++];

    inter.interface = pair.first.interface;
    inter.type = pair.first.type;

	switch(inter.type){
	case ExternalMemoryType::ExternalMemoryType_2P:{
	  inter.tp = pair.second.tp;
	} break;
	case ExternalMemoryType::ExternalMemoryType_DP:{
	  inter.dp[0] = pair.second.dp[0];
	  inter.dp[1] = pair.second.dp[1];
	}break;
	}
  }
  info.externalInterfaces = interfaces;

  return info;
}

void ParseVerilogFileTest(){
  TEMP_REGION(temp,nullptr);
  
  FREE_ARENA(tokenizer);
  FREE_ARENA(parsing);

  VerilogTokenizerState* state = PushStruct<VerilogTokenizerState>(tokenizer);

  state->arena = tokenizer;
  state->definesMap = PushTrieMap<String,DefineInfo>(parsing);
  state->stageBuffer = PushArray<VerilogTokenizerStage>(parsing,16);
  state->errors = PushList<String>(parsing);
  state->currentStage = 0;

  state->stageBuffer[0].type = VerilogTokenizerStageType_FILE;

  String tests[] = {
#if 0
{R"FOO(`define A
B
)FOO"},
#endif
#if 0
{R"FOO(`define A B
`A
)FOO"},
#endif
#if 0
{R"FOO(`define A \
B
`A
)FOO"},
#endif
#if 0
{R"FOO(`ifdef TEST
`define A B
`else
`define A C
`endif
`A
`ifdef A
`define D 0
`else
`define D 1
`endif
`D
)FOO"},
#endif
#if 0
{"`include \"Buffer.v\""}
#endif
  };

  for(String test : tests){
    FileContent fileLike = {};
    fileLike.content = test;
    fileLike.fileName = "";
    fileLike.state = FileContentState_OK;

    String content = fileLike.content;
    
    ContentState* first = &state->stageBuffer[0].state;

    first->content = fileLike;
    first->start = content.data;
    first->ptr = first->start;
    first->end = content.data + content.size;

    Parser* p = StartParsing(VerilogTokenizer,state,parsing);

    while(!p->Done()){
      Token t = p->NextToken();
      String res = PARSE_PushDebugRepr(temp,t);
      printf("%.*s\n",UN(res));
    }

  }
}

#endif
