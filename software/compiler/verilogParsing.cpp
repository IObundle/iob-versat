#include "verilogParsing.hpp"

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

bool PerformDefineSubstitution(StringBuilder* builder,TrieMap<String,MacroDefinition>* macros,String name){
  MacroDefinition* def = macros->Get(name);
  
  if(!def){
    return false;
  }

  String subs = def->content;

  // TODO: Right now, we are not performing substitution for function macros.
  //       Stuff kinda works because we only care about verilog interfaces and currently we do not have any
  //       unit that uses function macros inside the interfaces.
  //       Eventually must fix this.
  
  Tokenizer inside(subs,"`",{});
  while(!inside.Done()){
    Opt<Token> peek = inside.PeekFindUntil("`");

    if(!peek.has_value()){
      break;
    } else {
      inside.AdvancePeekBad(peek.value());
      builder->PushString(peek.value());

      inside.AssertNextToken("`");

      Token name = inside.NextToken();

      PerformDefineSubstitution(builder,macros,name);
    }
  }

  Token finish = inside.Finish();
  builder->PushString(finish);

  return true;
}

void PreprocessVerilogFile_(String fileContent,TrieMap<String,MacroDefinition>* macros,Array<String> includeFilepaths,StringBuilder* builder);

static void DoIfStatement(Tokenizer* tok,TrieMap<String,MacroDefinition>* macros,Array<String> includeFilepaths,StringBuilder* builder){
  Token first = tok->NextToken();
  Token macroName = tok->NextToken();

  bool compareVal = false;
  if(CompareString(first,"ifdef") || CompareString(first,"elsif")){
    compareVal = true;
  } else if(CompareString(first,"ifndef")){
    compareVal = false;
  } else {
    UNHANDLED_ERROR("TODO: Make this a handled error");
  }

  bool exists = macros->Exists(macroName);
  bool doIf = (compareVal == exists);

  // Try to find the edges of the if construct (else, endif or, if find another if recurse.)
  auto mark = tok->Mark();
  while(!tok->Done()){
    Token subContent = tok->Point(mark); // So that we never pass a string with a `endif to preprocess (or any other `directive except the ones that start a block)

    auto subMark = tok->Mark();
    Token token = tok->NextToken();
    if(!CompareString(token,"`")){
      continue;
    }

    Token type = tok->NextToken();
    
    if(CompareString(type,"endif")){
      if(doIf){
        PreprocessVerilogFile_(subContent,macros,includeFilepaths,builder);
      }
      break;
    }

    if(CompareString(type,"else")){
      if(doIf){
        PreprocessVerilogFile_(subContent,macros,includeFilepaths,builder);
        doIf = false;
      } else {
        mark = tok->Mark();
        doIf = true;
      }
      continue;
    }

    if(doIf) {
      if(CompareString(type,"ifdef") || CompareString(type,"ifndef") || CompareString(type,"elsif")){
        tok->Rollback(subMark); // TODO: Not good.
        DoIfStatement(tok,macros,includeFilepaths,builder);
      }
    }
    // otherwise must be some other directive, will be handled automatically in the Preprocess call.
  }
}

void PreprocessVerilogFile_(String fileContent,TrieMap<String,MacroDefinition>* macros,Array<String> includeFilepaths,StringBuilder* builder){
  Tokenizer tokenizer = Tokenizer(fileContent, "():;[]{}`,+-/*\\\"",{});
  Tokenizer* tok = &tokenizer;

  tok->keepComments = true;

  while(!tok->Done()){
    builder->PushString(tok->PeekWhitespace());
    Token peek = tok->PeekToken();

    if(!CompareString(peek,"`")){
      tok->AdvancePeek();
      builder->PushString(peek);
      
      continue;
    }
    tok->AdvancePeek();
    
    Token identifier = tok->PeekToken();
    if(CompareString(identifier,"include")){
      tok->AdvancePeek();
      tok->AssertNextToken("\"");

      Token fileName = tok->NextFindUntil("\"").value();
      tok->AssertNextToken("\"");

      // Open include file
      // TODO: Clean this up with our code and remove std::string
      std::string filename(UN_REVERSE(fileName));
      FILE* file = nullptr;
      for(String str : includeFilepaths){
        std::string string(str.data,str.size);
        
        std::string filepath;
        if(string.back() == '/'){
          filepath = string + filename;
        } else {
          filepath = string + '/' + filename;
        }

        file = OpenFile(filepath.c_str(),"r",FilePurpose_READ_CONTENT);

        if(file){
          break;
        }
      }
      DEFER_CLOSE_FILE(file);
      
      if(!file){
        printf("Couldn't find file: %.*s\n",UN(fileName));
        printf("Looked on the following folders:\n");

        printf("  %s\n",GetCurrentDirectory());
        for(String str : includeFilepaths){
          printf("  %.*s\n",UN(str));
        }

        NOT_IMPLEMENTED("Some error handling here");
      }

      size_t fileSize = GetFileSize(file);

      TEMP_REGION(temp,builder->arena);

      Byte* mem = PushBytes(temp,fileSize + 1);
      size_t amountRead = fread(mem,sizeof(char),fileSize,file);
      
      if(amountRead != fileSize){
        fprintf(stderr,"Verilog Parsing, error reading the full contents of a file\n");
        exit(-1);
      }

      mem[amountRead] = '\0';

      PreprocessVerilogFile_(String((const char*) mem,fileSize),macros,includeFilepaths,builder);
    } else if(CompareString(identifier,"define")){
      tok->AdvancePeek();

      Token defineName = tok->NextToken();
      Token emptySpace = tok->PeekWhitespace();

      if(emptySpace.size == 0){ // Function macro
        // TODO: We need proper argument handling.
        Opt<Token> arguments = tok->PeekFindIncluding(")");

        tok->AdvancePeekBad(arguments.value());
      }

      FindFirstResult search = tok->FindFirst({"\n","//","\\"}).value();
      String first = search.foundFirst;
      Token body = {};

      if(CompareString(first,"//")){ // If single comment newline or slash, the macro does not contain the comment
        body = tok->PeekFindUntil("//").value();
      } else {
        auto mark = tok->Mark();
        Token line = tok->PeekRemainingLine();
        body = line;
        while(!tok->Done()){
          if(line.size == -1){
            line = tok->Finish();
            body = tok->Point(mark);
            break;
          }

          Tokenizer inside(line,"\\",{}); // Handles slices inside comments

          bool hasSlice = false;
          while(!inside.Done()){
            Token t = inside.NextToken();
            if(CompareString(t,"\\")){
              hasSlice = true;
              break;
            }
          }

          if(hasSlice){
            tok->AdvancePeekBad(line);
            body = tok->Point(mark);
            line = tok->PeekRemainingLine();
          } else {
            tok->AdvancePeekBad(line);
            body = tok->Point(mark);
            break;
          }
        }
      }

      macros->Insert(defineName,{body,{}});
    } else if(CompareString(identifier,"undef")){
      tok->AdvancePeek();
      Token defineName = tok->NextToken();

      macros->Remove(defineName);
    } else if(CompareString(identifier,"timescale")){
      tok->AdvanceRemainingLine();
    } else if(CompareString(identifier,"ifdef") || CompareString(identifier,"ifndef")){
      DoIfStatement(tok,macros,includeFilepaths,builder);
      
    } else if(CompareString(identifier,"else")){
      NOT_POSSIBLE("All else and ends should have already been handled inside DoIf");
    } else if(CompareString(identifier,"endif")){
      NOT_POSSIBLE("All else and ends should have already been handled inside DoIf");
    } else if(CompareString(identifier,"elsif")){
      NOT_POSSIBLE("All else and ends should have already been handled inside DoIf");
    } else if(CompareString(identifier,"resetall")){
      macros->Clear();
    } else if(CompareString(identifier,"undefineall")){
      macros->Clear();
    } else {
      tok->AdvancePeek();

      // TODO: Better error handling. Report file position.
      if(!PerformDefineSubstitution(builder,macros,identifier)){
        printf("Do not recognize directive: %.*s\n",UN(identifier));
        NOT_POSSIBLE("Some better error handling here");
      }
    }
  }
}

String PreprocessVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out){
  TEMP_REGION(temp,out);

  TrieMap<String,MacroDefinition>* macros = PushTrieMap<String,MacroDefinition>(temp);

  auto builder = StartString(temp);
  PreprocessVerilogFile_(fileContent,macros,includeFilepaths,builder);

  String res = EndString(out,builder);

  return res;
}

static Expression* VerilogParseAtom(Tokenizer* tok,Arena* out){
  Expression* expr = PushStruct<Expression>(out);
  *expr = {};

  Token peek = tok->PeekToken();

  if(IsNum(peek[0])){
    int res = ParseInt(peek);
    tok->AdvancePeek();

    peek = tok->PeekToken();
    if(CompareString(peek,"'")){
      tok->AdvancePeek();
      Token actualValue = tok->NextToken();
      actualValue.data += 1;
      actualValue.size -= 1;
      res = ParseInt(actualValue);
    }
    
    expr->val = MakeValue(res);
    expr->type = Expression::LITERAL;

    return expr;
  } else if(peek[0] == '"'){
    tok->AdvancePeek();
    Token str = tok->PeekFindUntil("\"").value();
    tok->AdvancePeek();
    tok->AssertNextToken("\"");

    expr->val = MakeValue(str);
    expr->type = Expression::LITERAL;

    return expr;
  }

  Token name = tok->NextToken();

  expr->id = name;
  expr->type = Expression::IDENTIFIER;

  return expr;
}

static Expression* VerilogParseExpression(Tokenizer* tok,Arena* out);

static Expression* VerilogParseFactor(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  if(CompareString(peek,"(")){
    tok->AdvancePeek();
    Expression* expr = VerilogParseExpression(tok,out);
    tok->AssertNextToken(")");
    return expr;
  } else if(peek[0] == '$'){
    tok->AdvancePeek();

    String mathFunctionName = Offset(peek,1);
    
    Opt<MathFunctionDescription> optDescription = GetMathFunction(mathFunctionName);
    
    Assert(optDescription.has_value());

    MathFunctionDescription description = optDescription.value();

    // Verilog math function
    Expression* expr = PushStruct<Expression>(out);
    *expr = {};

    expr->id = mathFunctionName;
    expr->type = Expression::FUNCTION;
    expr->expressions = PushArray<Expression*>(out,description.amountOfParameters);

    tok->AssertNextToken("(");
    expr->expressions[0] = VerilogParseExpression(tok,out);
    if(description.amountOfParameters == 2){
      tok->AssertNextToken(",");
      expr->expressions[1] = VerilogParseExpression(tok,out);
    }
    
    tok->AssertNextToken(")");

    return expr;
  } else {
    Expression* expr = VerilogParseAtom(tok,out);
    return expr;
  }
}

static Value Eval(Expression* expr,TrieMap<String,Value>* map){
  switch(expr->type){
  case Expression::OPERATION:{
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
  case Expression::IDENTIFIER:{
    Value* val = map->Get(expr->id);
    if(!map){
      printf("Error, did not find parameter %.*s\n",UN(expr->id));
      NOT_IMPLEMENTED("Need to also report file position and stuff like that, similar to parser tokenizer error");
    }
    
    return *val;
  }break;
  case Expression::LITERAL:{
    return expr->val;
  }break;
  case Expression::FUNCTION:{ // Called Command but for verilog it is actually a math function call
    String functionName = expr->id;

    Expression* argument = expr->expressions[0];
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
  case Expression::UNDEFINED:
    NOT_POSSIBLE("None of these should appear in parameters");
  }

  NOT_POSSIBLE("Implemented as needed");
  return MakeValue();
}

static ExpressionRange ParseRange(Tokenizer* tok,Arena* out);

static Array<ParameterExpression> ParseParameters(Tokenizer* tok,TrieMap<String,Value>* map,Arena* out){
  //TODO: Add type and range to parsing
  /*
	Range currentRange;
	ParameterType type;
   */

  auto params = StartArray<ParameterExpression>(out);

  // TODO: Not used but must parse it anyway.
  ExpressionRange range = {};
  ParamFlags flags = {};
  while(1){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"parameter")){
      tok->FlushStoredTokens();
      
      bool prev = tok->keepComments;
      tok->keepComments = true;

      tok->NextToken();
      tok->FlushStoredTokens();

      String commentExpression = ParseComment(tok,out);
      
      if(!Empty(commentExpression)){
        Tokenizer comTok(commentExpression,":,",{});
        
        comTok.AssertNextToken("versat");
        comTok.AssertNextToken(":");
        
        while(!comTok.Done()){
          comTok.IfNextToken(",");
          
          Token paramFlag = comTok.NextToken();
          Opt<ParamFlags> flagOpt = META_ParamToFlag_ReverseMap(paramFlag);

          if(flagOpt.has_value()){
            // Better support for flag concatenation when using enums.
            flags = (ParamFlags) ((u32) flags | (u32) flagOpt.value());
          } else {
            // TODO: Better error reporting
            printf("%.*s is not a valid param flag",UN(paramFlag));
          }
        }
      }
      tok->keepComments = prev;
      tok->IfNextToken("signed");

      range = ParseRange(tok,out);

      Token paramName = tok->NextToken();

      tok->AssertNextToken("=");

      Token peek = tok->PeekToken();
      // TODO: Kinda hacky way of parsing strings, we do not care about them but must parse them anyway.
      if(CompareString(peek,"\"")){
        tok->AdvancePeek();
        while(!tok->Done()){
          Token token = tok->PeekToken();

          if(CompareString(token,"\"")){
            break;
          }

          tok->AdvancePeek();
        }
        tok->AssertNextToken("\"");
        
        continue;
      }
      
      Expression* expr = VerilogParseExpression(tok,out);
      Value val = Eval(expr,map);

      map->Insert(paramName,val);
      
      ParameterExpression* p = params.PushElem();
      p->name = paramName;
      p->expr = expr;
      p->flags = flags;
    } else if(CompareString(peek,")")){
      break;
    } else if(CompareString(peek,";")){ // To parse inside module parameters, technically wrong but harmless
      tok->AdvancePeek();
      break;
    } else if(CompareString(peek,",")){
      tok->AdvancePeek();
      continue;
    }
  }

  return EndArray(params);
}

static Expression* VerilogParseExpression(Tokenizer* tok,Arena* out){
  Expression* res = ParseOperationType(tok,{{"+","-"},{"*","/"}},VerilogParseFactor,out);

  return res;
}

static ExpressionRange ParseRange(Tokenizer* tok,Arena* out){
  static Expression zeroExpression = {};

  Token peek = tok->PeekToken();

  if(!CompareString(peek,"[")){ // No range is equal to range [0:0]. Do not known if it's worth/need to  differentiate
    ExpressionRange range = {};

    zeroExpression.type = Expression::LITERAL;
    zeroExpression.val = MakeValue(0);
    
    range.top = &zeroExpression;
    range.bottom = &zeroExpression;
    
    return range;
  }

  tok->AssertNextToken("[");

  ExpressionRange res = {};
  res.top = VerilogParseExpression(tok,out);

  tok->AssertNextToken(":");

  res.bottom = VerilogParseExpression(tok,out);
  tok->AssertNextToken("]");

  return res;
}

static Module ParseModule(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

  Module module = {};

  TrieMap<String,Value>* values = PushTrieMap<String,Value>(temp);

  tok->AssertNextToken("module");

  module.name = tok->NextToken();

  Token peek = tok->PeekToken();
  if(CompareString(peek,"#(")){
    tok->AdvancePeek();
    module.parameters = ParseParameters(tok,values,out);
    tok->AssertNextToken(")");
  }

  tok->AssertNextToken("(");
  if(!tok->IfPeekToken(")")){
  
    ArenaList<PortDeclaration>* portList = PushList<PortDeclaration>(temp);
    // Parse ports
    while(!tok->Done()){
      peek = tok->PeekToken();

      PortDeclaration port;
      ArenaList<Pair<String,Value>>* attributeList = PushList<Pair<String,Value>>(temp);
    
      if(CompareString(peek,"(*")){
        tok->AdvancePeek();
        while(1){
          Token attributeName = tok->NextToken();

          if(!Contains(possibleAttributes,(String) attributeName)){
            printf("ERROR: Do not know attribute named: %.*s\n",UN(attributeName));
            exit(-1);
          }

          peek = tok->PeekToken();
          if(CompareString(peek,"=")){
            tok->AdvancePeek();
            Expression* expr = VerilogParseExpression(tok,out);
            Value value = Eval(expr,values);

            *attributeList->PushElem() = {attributeName,value};

            peek = tok->PeekToken();
          } else {
            *attributeList->PushElem() = {attributeName,MakeValue()};
          }

          if(CompareString(peek,",")){
            tok->AdvancePeek();
            continue;
          }
          if(CompareString(peek,"*)")){
            tok->AdvancePeek();
            break;
          }
        }
      }
      port.attributes = PushHashmapFromList(out,attributeList);

      Token portType = tok->NextToken();
      if(CompareString(portType,"input")){
        port.type = WireDir_INPUT;
      } else if(CompareString(portType,"output")){
        port.type = WireDir_OUTPUT;
      } else if(CompareString(portType,"inout")){
        port.type = WireDir_INOUT;
      } else {
        UNHANDLED_ERROR("TODO: Should be a handled error");
      }

      // TODO: Add a new function to parser to "ignore" the following list of tokens (loop every time until it doesn't find one from the list), and replace this function here with reg and all the different types it can be
      while(1){
        peek = tok->PeekToken();
        if(CompareString("reg",peek)){
          tok->AdvancePeek();
          continue;
        }
        if(CompareString("signed",peek)){
          tok->AdvancePeek();
          continue;
        }
        break;
      }

      ExpressionRange res = ParseRange(tok,out);
      port.range = res;
      port.name = tok->NextToken();

      *portList->PushElem() = port;

      peek = tok->PeekToken();
      if(CompareString(peek,")")){
        tok->AdvancePeek();
        break;
      }

      tok->AssertNextToken(",");
    }
    module.ports = PushArray(out,portList);
  }
  
  // Any inside module parameters
#if 0
  while(1){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"parameter")){
      ParseParameters(tok,values);
    } else if(CompareString(peek,"endmodule")){
      tok->AdvancePeek(peek);
      break;
    } else {
      tok->AdvancePeek(peek);
    }
  }
#endif

  Token skip = tok->PeekFindIncluding("endmodule").value();
  tok->AdvancePeekBad(skip);

  return module;
}

Array<Module> ParseVerilogFile(String fileContent,Array<String> includeFilepaths,Arena* out){
  TEMP_REGION(temp,out);

  Tokenizer tokenizer = Tokenizer(fileContent,"\n:',()[]{}\"+-/*=",{"#(","+:","-:","(*","*)"});
  Tokenizer* tok = &tokenizer;

  ArenaList<Module>* modules = PushList<Module>(temp);

  bool isSource = false;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"(*")){
      tok->AdvancePeek();

      Token attribute = tok->NextToken();
      if(CompareString(attribute,"source")){
        isSource = true;
      } else {
        // TODO: Report unused attribute.
        //NOT_IMPLEMENTED("Should not give an error"); // Unknown attribute, error for now
      }

      tok->AssertNextToken("*)");

      continue;
    }

    if(CompareString(peek,"module")){
      Module module = ParseModule(tok,out);

      module.isSource = isSource;
      *modules->PushElem() = module;

      break; // For now, only parse the first module found
    }

    tok->AdvancePeek();
  }

  return PushArray(out,modules);
}

SymbolicExpression* SymbolicExpressionFromVerilog(Expression* topExpr,Arena* out){
  FULL_SWITCH(topExpr->type){
  case Expression::UNDEFINED: {
    Assert(false);
  } break;
  case Expression::OPERATION: {
    SymbolicExpression* left = SymbolicExpressionFromVerilog(topExpr->expressions[0],out);
    SymbolicExpression* right = SymbolicExpressionFromVerilog(topExpr->expressions[1],out);
    
    switch(topExpr->op[0]){
    case '+':{
      return SymbolicAdd(left,right,out);
    } break;
    case '-':{
      return SymbolicSub(left,right,out);
    } break;
    case '*':{
      return SymbolicMult(left,right,out);
    } break;
    case '/':{
      return SymbolicDiv(left,right,out);
    } break;
    default:{
      // TODO: Better error message
      NOT_IMPLEMENTED("");
    } break;
    } 
  } break;
  case Expression::IDENTIFIER: {
    return PushVariable(out,topExpr->id);
  } break;
  case Expression::FUNCTION: {
    // TODO: Better error message and we probably can do more stuff here
    NOT_IMPLEMENTED("");
  } break;
  case Expression::LITERAL: {
    return PushLiteral(out,topExpr->val.number);
  } break;
} END_SWITCH();
  
  return {};
}

SymbolicExpression* SymbolicExpressionFromVerilog(ExpressionRange range,Arena* out){
  TEMP_REGION(temp,out);
  
  SymbolicExpression* top = SymbolicExpressionFromVerilog(range.top,temp);
  SymbolicExpression* bottom = SymbolicExpressionFromVerilog(range.bottom,temp);
  SymbolicExpression* res = Normalize(SymbolicAdd(SymbolicSub(top,bottom,temp),SYM_one,temp),out);

  return res;
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
    Tokenizer port(decl.name,"",{"in","out","delay","done","rst","clk","run","running","databus"});
    
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
      port.AssertNextToken("in");
      int index = ParseInt(port.NextToken());
      Value* delayValue = decl.attributes->Get(VERSAT_LATENCY);

      int delay = 0;
      if(delayValue) delay = delayValue->number;

      inputs[index].delay = delay;
      inputs[index].range = decl.range;
    } else if(CheckFormat("out%d",decl.name)){
      port.AssertNextToken("out");
      int index = ParseInt(port.NextToken());
      Value* latencyValue = decl.attributes->Get(VERSAT_LATENCY);

      int latency = 0;
      if(latencyValue) latency = latencyValue->number;

      outputs[index].delay = latency;
      outputs[index].range = decl.range;
    } else if(CheckFormat("delay%d",decl.name)){
      port.AssertNextToken("delay");
      int delay = ParseInt(port.NextToken());

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

Value Eval(Expression* expr,Array<ParameterExpression> parameters){
  Value ret = {};
  switch(expr->type){
  case Expression::LITERAL: {
    ret = expr->val;
  } break;
  case Expression::OPERATION: {
    // TODO: Not the first place where we have default operation behaviour. Refactor into a single place
    switch(expr->op[0]){
    case '+': {
      ret = MakeValue(Eval(expr->expressions[0],parameters).number
                      + Eval(expr->expressions[1],parameters).number);
    } break;
    case '-': {
      ret = MakeValue(Eval(expr->expressions[0],parameters).number
                      - Eval(expr->expressions[1],parameters).number);
    } break;
    case '*': {
      ret = MakeValue(Eval(expr->expressions[0],parameters).number
                      * Eval(expr->expressions[1],parameters).number);
    } break;
    case '/': {
      ret = MakeValue(Eval(expr->expressions[0],parameters).number
                      / Eval(expr->expressions[1],parameters).number);
    } break;
    default:{
      printf("%s\n",expr->op);
      NOT_IMPLEMENTED("Implemented as needed");
    } break;
    }
  } break;
  case Expression::IDENTIFIER: {
    String id = expr->id;

    for(ParameterExpression& p : parameters){
      if(CompareString(p.name,id)){
        ret = Eval(p.expr,parameters);
        break;
      }
    }
  } break;
  case Expression::FUNCTION:; 
  case Expression::UNDEFINED:; 
  }

  return ret;
}
