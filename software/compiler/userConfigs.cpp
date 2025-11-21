#include "userConfigs.hpp"

#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"

// TODO: Copied from spec parser, we probably want to move this somewhere and reconciliate everything.
// We probably want a common parsing file (different from parser.hpp) that contains all this logic instead of repeating or separating over multiple files.
static void ReportError(String content,Token faultyToken,String error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("[Error]\n");
  printf("%.*s:\n",UN(error));
  printf("%.*s\n",UN(loc));
  printf("\n");
}

static void ReportError(Tokenizer* tok,Token faultyToken,String error){
  String content = tok->GetContent();
  ReportError(content,faultyToken,error);
}

static bool _ExpectError(Tokenizer* tok,String expected){
  TEMP_REGION(temp,nullptr);

  Token got = tok->NextToken();
  if(!CompareString(got,expected)){
    
    auto builder = StartString(temp);
    builder->PushString("Parser Error.\n Expected to find:  '");
    builder->PushString(PushEscapedString(temp,expected,' '));
    builder->PushString("'\n");
    builder->PushString("  Got:");
    builder->PushString(PushEscapedString(temp,got,' '));
    builder->PushString("\n");
    String text = EndString(temp,builder);
    ReportError(tok,got,StaticFormat("%*s",UN(text))); \
    return true;
  }
  return false;
}

// Macro because we want to return as well
#define EXPECT(TOKENIZER,STR) \
  do{ \
    if(_ExpectError(TOKENIZER,STR)){ \
      return {}; \
    } \
  } while(0)

#define CHECK_IDENTIFIER(ID) \
  if(!IsIdentifier(ID)){ \
    ReportError(tok,ID,StaticFormat("type name '%.*s' is not a valid name",UN(ID))); \
    return {}; \
  }

static Opt<ConfigIdentifier> ParseConfigIdentifier(Tokenizer* tok,Arena* out){
  Token id = tok->PeekToken();
  CHECK_IDENTIFIER(id);
  tok->AdvancePeek();
  
  Token access = {};
  if(tok->IfNextToken(".")){
    access = tok->NextToken();

    CHECK_IDENTIFIER(access);
  }

  ConfigIdentifier res = {};
  res.name = id;
  res.wire = access;
  
  return res;
}

static Opt<ConfigStatement*> ParseConfigStatement(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  Token peek = tok->PeekToken();

  ConfigStatement* stmt = nullptr;

  if(CompareString(peek,"for")){
    // For loop
    NOT_IMPLEMENTED();
  } else {
    // Assignment
    Opt<ConfigIdentifier> lhs = ParseConfigIdentifier(tok,out);
    PROPAGATE(lhs);
    EXPECT(tok,"=");
    
    Array<Token> rhs = TokenizeSymbolicExpression(tok,out);

    if(rhs.size == 0){
      return {};
    }

    //SymbolicExpression* rhs = ParseSymbolicExpression(tok,out);
    //PROPAGATE(rhs);
    EXPECT(tok,";");

    stmt = PushStruct<ConfigStatement>(out);
    stmt->lhs = lhs.value();
    stmt->rhs = rhs;
  }

  return stmt;
}

// TODO: Copy from versatSpec. This function at the very least needs to be moved into a more common place.
//       Furthermore we can make this more generic if we take an enum flag that indicates the type of numbers accepted, if needed.
static Opt<int> ParseNumber(Tokenizer* tok){
  // TODO: We only handle integers, for now.
  Token number = tok->NextToken();

  bool negate = false;
  if(CompareString(number,"-")){
    negate = true;
    number = tok->NextToken();
  }

  for(int i = 0; i < number.size; i++){
    if(!(number[i] >= '0' && number[i] <= '9')){
      ReportError(tok,number,StaticFormat("%.*s is not a valid number",UN(number)));
      return {};
    }
  }

  int res = ParseInt(number);
  if(negate){
    res = -res;
  }
  
  return res;
}

static Opt<ConfigVarDeclaration> ParseConfigVarDeclaration(Tokenizer* tok){
  ConfigVarDeclaration res = {};

  res.name = tok->NextToken();
  CHECK_IDENTIFIER(res.name);

  Token peek = tok->PeekToken();
  if(CompareString(peek,"[")){
    tok->AdvancePeek();

    Opt<int> number = ParseNumber(tok);
    PROPAGATE(number);
    int arraySize = number.value();

    EXPECT(tok,"]");

    res.arraySize = arraySize;
    res.isArray = true;
  }
  
  return res;
}

static Opt<Array<ConfigVarDeclaration>> ParseConfigFunctionArguments(Tokenizer* tok,Arena* out){
  auto array = StartArray<ConfigVarDeclaration>(out);

  EXPECT(tok,"(");

  if(tok->IfNextToken(")")){
    return EndArray(array);
  }
  
  while(!tok->Done()){
    Opt<ConfigVarDeclaration> var = ParseConfigVarDeclaration(tok);
    PROPAGATE(var);

    *array.PushElem() = var.value();
    
    if(tok->IfNextToken(",")){
      continue;
    } else {
      break;
    }
  }

  EXPECT(tok,")");

  return EndArray(array);
}

ConfigFunctionDef* ParseConfigFunction(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  ConfigFunctionDef* res = nullptr;

  if(tok->IfNextToken("config")){
    Token configName = tok->NextToken();
    CHECK_IDENTIFIER(configName);

    Opt<Array<ConfigVarDeclaration>> vars = ParseConfigFunctionArguments(tok,out);
    PROPAGATE(vars);

    EXPECT(tok,"{");

    auto configs = PushArenaList<ConfigStatement*>(temp);
    while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareString(peek,";")){
        tok->AdvancePeek();
        continue;
      }
      if(CompareString(peek,"}")){
        break;
      }
        
      Opt<ConfigStatement*> config = ParseConfigStatement(tok,out);
      PROPAGATE(config);

      *configs->PushElem() = config.value();
    }

    EXPECT(tok,"}");
       
    res = PushStruct<ConfigFunctionDef>(out);
    res->name = configName;
    res->variables = vars.value();
    res->statements = PushArrayFromList(out,configs);
  }  

  return res;
}

bool IsNextTokenConfigFunctionStart(Tokenizer* tok){
  bool res = false;

  res |= tok->IfPeekToken("config");
  res |= tok->IfPeekToken("state");
  res |= tok->IfPeekToken("mem");
  
  return res;
}

// ============================================================================
// Instantiation and manipulation

ConfigFunction* InstantiateConfigFunction(ConfigFunctionDef* def,FUDeclaration* declaration){
  Arena* perm = globalPermanent;

  TEMP_REGION(temp,perm);

  for(ConfigStatement* stmt : def->statements){
    ConfigIdentifier id = stmt->lhs;
    Token name = id.name;
    Token wireName = id.wire;

    String functionName = {};
    
    if(Empty(wireName)){
      // Function invocation only.
      
      
    } else {
      // Symbolic Expression
      SymbolicExpression* expr = ParseSymbolicExpression(stmt->rhs,perm);
      
      if(!expr){
        // TODO: Report error here. Parse function must return the error somewhat.
        return nullptr;
      }


    }
  }
  

#if 0
  // For a simple config
  Array<ConfigVarDeclaration> variables = def->variables;
  Array<Token> variableNames = Extract(variables,temp,&ConfigVarDeclaration::name);
#endif

/*
#if 0  
  LEFT HERE - For composite units we probably want to instantiate stuff directly.
            - We probably want some interface where we can instantiate a ConfigFunction for a given set of arguments.

  Something like:

  if have we configA(x){b.constant = x}
  and then we have configB(y){c = configA(y)};

  we probably want to be able to instantiate configA such that we end up producing something that is similar to:

  configBInst(y){c.constant = y};

  Furthermore, when we end up on the code generation part of the code, I probably want a function that maps from a given full configuration into the associated expression to access the config structure.

  Basically transforms something like 'c.constant' into the expression 'mergeConfig->c.constant' except that we must be able to handle both merge stuff and also composite stuff.

  Remember, there is no limit to composite structs meaning that we could have something like 'c.constant' transform into something like 'a.b.c.constant'
#endif
*/
  
  return nullptr;  
}
