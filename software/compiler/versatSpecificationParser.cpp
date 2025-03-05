#include "versatSpecificationParser.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

#include "configurations.hpp"
#include "declaration.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "type.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "parser.hpp"
#include "debug.hpp"
#include "merge.hpp"
#include "accelerator.hpp"

// TODO: Rework expression parsing to support error reporting similar to module diff.
//       A simple form of synchronization after detecting an error would vastly improve error reporting
//       Change iterative and merge to follow module def.
//       Need to detect multiple inputs to the same port and report error.
//       Transform is not correctly implemented.
//         We are thinking in ports when we should think in connection sources and sinks.
//         It's probably better to implement a specific syntax for transforms in order to prevent confusion.
//         It's probably better to swap the Iterator approach with a function that generates an array.
//           Then we can implement transformations as functions that take arrays and output arrays.
//       Error reporting is very generic. Implement more specific forms.
//       This parser is still not production ready. At the very least all the Asserts should be removed and replaced
//         by actual error reporting. Not a single Assert is programmer error detector, all the current ones are
//         user error that most be reported.

void ReportError(String content,Token faultyToken,const char* error){
  TEMP_REGION(temp,nullptr);

  String loc = GetRichLocationError(content,faultyToken,temp);

  printf("\n");
  printf("%s\n",error);
  printf("%.*s\n",UNPACK_SS(loc));
  printf("\n");
}

void ReportError(Tokenizer* tok,Token faultyToken,const char* error){
  String content = tok->GetContent();
  ReportError(content,faultyToken,error);
}

bool _ExpectError(Tokenizer* tok,const char* str){
  TEMP_REGION(temp,nullptr);

  Token got = tok->NextToken();
  String expected = STRING(str);
  if(!CompareString(got,expected)){
    
    auto builder = StartString(temp);
    builder->PushString("Parser Error.\n Expected to find:  '");
    builder->PushString(PushEscapedString(temp,expected,' '));
    builder->PushString("'\n");
    builder->PushString("  Got:");
    builder->PushString(PushEscapedString(temp,got,' '));
    builder->PushString("\n");
    String text = EndString(temp,builder);
    ReportError(tok,got,StaticFormat("%*s",UNPACK_SS(text))); \
    return true;
  }
  return false;
}

void _UnexpectError(Token token){
  TEMP_REGION(temp,nullptr);
  String content = PushString(temp,"At pos: %d:%d, did not expect to get: \"%.*s\"",token.loc.start.line,token.loc.start.column,UNPACK_SS(token));  
  printf("%.*s\n",UNPACK_SS(content));
}

static bool IsValidIdentifier(String str){
  auto CheckSingle = [](char ch){
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_');
  };

  if(str.size <= 0){
    return false;
  }

  if(!CheckSingle(str.data[0])){
    return false;
  }

  for(int i = 1; i < str.size; i++){
    char ch = str.data[i];
    if(!CheckSingle(ch) && !(ch >= '0' && ch <= '9')){
      return false;
    }
  }

  return true;
}

// Macro because we want to return as well
#define EXPECT(TOKENIZER,STR) \
  do{ \
    if(_ExpectError(TOKENIZER,STR)){ \
      return {}; \
    } \
  } while(0)


#define UNEXPECTED(TOK) \
  do{ \
    _UnexpectError(TOK); \
    return {}; \
  } while(0)

#define CHECK_IDENTIFIER(ID) \
  if(!IsValidIdentifier(ID)){ \
    ReportError(tok,ID,StaticFormat("type name '%.*s' is not a valid name",UNPACK_SS(ID))); \
    return {}; \
  }

Opt<int> ParseNumber(Tokenizer* tok){
  // TODO: We only handle integers, for now.
  Token number = tok->NextToken();

  bool negate = false;
  if(CompareString(number,"-")){
    negate = true;
    number = tok->NextToken();
  }

  for(int i = 0; i < number.size; i++){
    if(!(number[i] >= '0' && number[i] <= '9')){
      ReportError(tok,number,StaticFormat("%.*s is not a valid number",UNPACK_SS(number)));
      return {};
    }
  }

  int res = ParseInt(number);
  if(negate){
    res = -res;
  }
  
  return res;
}

Opt<Range<int>> ParseRange(Tokenizer* tok){
  Range<int> res = {};

  Opt<int> n1 = ParseNumber(tok);
  PROPAGATE(n1);

  if(!tok->IfNextToken("..")){
    res.start = n1.value();
    res.end = n1.value();
    return res;
  }

  Opt<int> n2 = ParseNumber(tok);
  PROPAGATE(n2);

  res.start = n1.value();
  res.end = n2.value();
  
  return res;
}

Opt<Var> ParseVar(Tokenizer* tok){
  Var var = {};

  Token name = tok->NextToken();

  Token peek = tok->PeekToken();
  if(CompareString(peek,"[")){
    tok->AdvancePeek();

    Opt<Range<int>> range = ParseRange(tok);
    PROPAGATE(range);

    var.isArrayAccess = true;
    var.index = range.value();

    EXPECT(tok,"]");
  }
  
  int delayStart = 0;
  int delayEnd = 0;
  peek = tok->PeekToken();
  if(CompareString(peek,"{")){
    tok->AdvancePeek();

    Opt<Range<int>> rangeOpt = ParseRange(tok);
    PROPAGATE(rangeOpt);
    delayStart = rangeOpt.value().start;
    delayEnd = rangeOpt.value().end;

    EXPECT(tok,"}");
    peek = tok->PeekToken();
  }

  int portStart = 0;
  int portEnd = 0;
  peek = tok->PeekToken();
  if(CompareString(peek,":")){
    tok->AdvancePeek();

    Opt<Range<int>> rangeOpt = ParseRange(tok);
    PROPAGATE(rangeOpt);
    portStart = rangeOpt.value().start;
    portEnd = rangeOpt.value().end;
  }

  var.name = name;
  var.extra.delay.start = delayStart;
  var.extra.delay.end = delayEnd;
  var.extra.port.start = portStart;
  var.extra.port.end = portEnd;

  return var;
}

// A group can also be a single var. It does not necessary mean that it must be of the form {...}
Opt<VarGroup> ParseVarGroup(Tokenizer* tok,Arena* out){
  auto tokMark = tok->Mark();

  Token peek = tok->PeekToken();
  
  if(CompareString(peek,"{")){
    tok->AdvancePeek();

    auto arr = StartArray<Var>(out);
    while(!tok->Done()){
      Var* var = arr.PushElem();
      Opt<Var> optVar = ParseVar(tok);
      PROPAGATE(optVar);

      *var = optVar.value();

      Token sepOrEnd = tok->NextToken();
      if(CompareString(sepOrEnd,",")){
        continue;
      } else if(CompareString(sepOrEnd,"}")){
        break;
      } else {
        UNEXPECTED(sepOrEnd);
      }
    }
    VarGroup res = {};
    res.vars = EndArray(arr);
    res.fullText = tok->Point(tokMark);
    return res;
  } else {
    Opt<Var> optVar = ParseVar(tok);
    PROPAGATE(optVar);

    Var var = optVar.value();
    VarGroup res = {};
    res.fullText = tok->Point(tokMark);
    res.vars = PushArray<Var>(out,1);
    res.vars[0] = var;
    return res;
  }
}

SpecExpression* ParseAtom(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  bool negate = false;
  const char* negateType = nullptr;
  if(CompareString(peek,"~")){
    negate = true;
    negateType = "~";
    tok->AdvancePeek();
  }
  if(CompareString(peek,"-")){
    negate = true;
    negateType = "-";
    tok->AdvancePeek();
  }

  SpecExpression* expr = PushStruct<SpecExpression>(out);
  
  peek = tok->PeekToken();
  if(peek[0] >= '0' && peek[0] <= '9'){  // TODO: Need to call ParseNumber
    tok->AdvancePeek();

    int digit = ParseInt(peek);

    expr->type = SpecExpression::LITERAL;
    expr->val = MakeValue(digit);
  } else {
    expr->type = SpecExpression::VAR;
    Opt<Var> optVar = ParseVar(tok);
    PROPAGATE(optVar);

    expr->var = optVar.value();
  }

  if(negate){
    SpecExpression* negateExpr = PushStruct<SpecExpression>(out);
    negateExpr->op = negateType;
    negateExpr->type = SpecExpression::OPERATION;
    negateExpr->expressions = PushArray<SpecExpression*>(out,1);
    negateExpr->expressions[0] = expr;

    expr = negateExpr;
  }

  return expr;
}

SpecExpression* ParseSpecExpression(Tokenizer* tok,Arena* out);
SpecExpression* ParseTerm(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  SpecExpression* expr = nullptr;
  if(CompareString(peek,"(")){
    tok->AdvancePeek();
    expr = ParseSpecExpression(tok,out);
    tok->AssertNextToken(")");
  } else {
    expr = ParseAtom(tok,out);
  }

  return expr;
}

SpecExpression* ParseSpecExpression(Tokenizer* tok,Arena* out){
  SpecExpression* expr = ParseOperationType<SpecExpression>(tok,{{"+","-"},{"&","|","^"},{">><",">>","><<","<<"}},ParseTerm,out);

  return expr;
}

// TODO: This is bad. Remove this
int NumberOfConnections(ConnectionExtra e){
  int connections = e.port.end - e.port.start + 1;

  return connections;
}
  
int NumberOfConnections(Array<PortExpression> arr){
  int connections = 0;
  for(PortExpression& e : arr){
    connections += NumberOfConnections(e.extra);
  }
  return connections;
}

// TODO: After cleaning up iterative and merge, remove these
void ConnectUnit(PortExpression out,PortExpression in){
  FUInstance* inst1 = out.inst;
  FUInstance* inst2 = in.inst;
  ConnectionExtra outE = out.extra;
  ConnectionExtra inE = in.extra;

  int outRange = outE.port.end - outE.port.start + 1;
  int delayRange = outE.delay.end - outE.delay.start + 1;
  int inRange = inE.port.end - inE.port.start + 1;

  bool matches = (delayRange == inRange || delayRange == 1) &&
                 (outRange == inRange   || outRange == 1);

  Assert(matches);

  int delayDelta = (delayRange == 1 ? 0 : 1);
  if(outRange == 1){
    for(int i = 0; i < inRange; i++){
      ConnectUnits(inst1,outE.port.start,inst2,inE.port.start + i,outE.delay.start + delayDelta * i);
    }
  } else {
    for(int i = 0; i < inRange; i++){
      ConnectUnits(inst1,outE.port.start + i,inst2,inE.port.start + i,outE.delay.start + delayDelta * i);
    }
  }
}

// TODO: After cleaning up iterative and merge, remove these
void ConnectUnit(Array<PortExpression> outs, PortExpression in){
  if(outs.size == 1){
    // TODO: The 1-to-1 connection implements more connection logic than this function currently implements
    //       Still not have a good ideia how to handle more complex cases, implement them as they appear.
    //       Specially how to match everything together: groups + port ranges + delays.
    ConnectUnit(outs[0],in);
    return;
  }

  ConnectionExtra inE = in.extra;

  int outRange = NumberOfConnections(outs);
  int inRange = NumberOfConnections(in.extra);

  Assert(outRange == inRange);

  FUInstance* instIn = in.inst;
  int portIndex = 0;
  int unitIndex = 0;
  for(int i = 0; i < inRange; i++){
    PortExpression& expr = outs[unitIndex];

    FUInstance* inst = expr.inst;
    int portStart = expr.extra.port.start + portIndex;
    
    ConnectUnits(inst,portStart,instIn,inE.port.start + i); // TODO: For now we are assuming no delay

    portIndex += 1;

    if(portIndex >= NumberOfConnections(expr.extra)){
      unitIndex += 1;
      portIndex = 0;
    }
  }
}

String GetUniqueName(String name,Arena* out,InstanceName* names){
  int counter = 0;
  String uniqueName = name;
  auto mark = MarkArena(out);
  while(names->Exists(uniqueName)){
    PopMark(mark);
    uniqueName = PushString(out,"%.*s_%d",UNPACK_SS(name),counter++);
  }

  names->Insert(uniqueName);

  return uniqueName;
}

String GetActualArrayName(String baseName,int index,Arena* out){
  return PushString(out,"%.*s_%d",UNPACK_SS(baseName),index);
}

// Right now, not using the full portion of PortExpression because technically we would need to instantiate multiple things. Not sure if there is a need, when a case occurs then make the change then
PortExpression InstantiateSpecExpression(SpecExpression* root,Accelerator* circuit,InstanceTable* table,InstanceName* names){
  TEMP_REGION(temp,nullptr);
  Arena* perm = globalPermanent;
  PortExpression res = {};

  switch(root->type){
    // Just to remove warnings. TODO: Change expression so that multiple locations have their own expression struct, instead of reusing the same one.
  case SpecExpression::UNDEFINED: break;
  case SpecExpression::LITERAL:{
    int number = root->val.number;

    String toSearch = PushString(temp,"N%d",number);

    FUInstance** found = table->Get(toSearch);

    if(!found){
      String permName = PushString(perm,"%.*s",UNPACK_SS(toSearch));
      String uniqueName = GetUniqueName(permName,perm,names);

      FUInstance* digitInst = (FUInstance*) CreateFUInstance(circuit,GetTypeByName(STRING("Literal")),uniqueName);
      digitInst->literal = number;
      table->Insert(permName,digitInst);
      res.inst = digitInst;
    } else {
      res.inst = *found;
    }
  }break;
  case SpecExpression::VAR:{
    Var var = root->var;  
    String name = var.name;

    if(var.isArrayAccess){
      name = GetActualArrayName(var.name,var.index.bottom,globalPermanent);
    }
    
    FUInstance* inst = table->GetOrFail(name);

    res.inst = inst;
    res.extra = var.extra;
  } break;
  case SpecExpression::OPERATION:{
    PortExpression expr0 = InstantiateSpecExpression(root->expressions[0],circuit,table,names);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr0.extra.port.start == expr0.extra.port.end);
    Assert(expr0.extra.delay.start == expr0.extra.delay.end);

    if(root->expressions.size == 1){
      Assert(root->op[0] == '~' || root->op[0] == '-');

      String typeName = {};

      switch(root->op[0]){
      case '~':{
        typeName = STRING("NOT");
      }break;
      case '-':{
        typeName = STRING("NEG");
      }break;
      }

      String permName = GetUniqueName(typeName,perm,names);
      FUInstance* inst = CreateFUInstance(circuit,GetTypeByName(typeName),permName);

      ConnectUnits(expr0.inst,expr0.extra.port.start,inst,0,expr0.extra.delay.start);

      res.inst = inst;
      res.extra.port.end  = res.extra.port.start  = 0;
      res.extra.delay.end = res.extra.delay.start = 0;

      return res;
    } else {
      Assert(root->expressions.size == 2);
    }

    PortExpression expr1 = InstantiateSpecExpression(root->expressions[1],circuit,table,names);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr1.extra.port.start == expr1.extra.port.end);
    Assert(expr1.extra.delay.start == expr1.extra.delay.end);

    String op = STRING(root->op);
    const char* typeName;
    if(CompareString(op,"&")){
      typeName = "AND";
    } else if(CompareString(op,"|")){
      typeName = "OR";
    } else if(CompareString(op,"^")){
      typeName = "XOR";
    } else if(CompareString(op,">><")){
      typeName = "RHR";
    } else if(CompareString(op,">>")){
      typeName = "SHR";
    } else if(CompareString(op,"><<")){
      typeName = "RHL";
    } else if(CompareString(op,"<<")){
      typeName = "SHL";
    } else if(CompareString(op,"+")){
      typeName = "ADD";
    } else if(CompareString(op,"-")){
      typeName = "SUB";
    } else {
      // TODO: Proper error reporting
      printf("%.*s\n",UNPACK_SS(op));
      Assert(false);
    }

    String typeStr = STRING(typeName);
    FUDeclaration* type = GetTypeByName(typeStr);
    String uniqueName = GetUniqueName(type->name,perm,names);

    FUInstance* inst = CreateFUInstance(circuit,type,uniqueName);

    ConnectUnits(expr0.inst,expr0.extra.port.start,inst,0,expr0.extra.delay.start);
    ConnectUnits(expr1.inst,expr1.extra.port.start,inst,1,expr1.extra.delay.start);

    res.inst = inst;
    res.extra.port.end  = res.extra.port.start  = 0;
    res.extra.delay.end = res.extra.delay.start = 0;
  } break;
  }

  Assert(res.inst);
  return res;
}

Opt<VarDeclaration> ParseVarDeclaration(Tokenizer* tok){
  VarDeclaration res = {};

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

Array<Token> CheckAndParseConnectionTransforms(Tokenizer* tok,Arena* out){
  auto arr = StartArray<Token>(out);
  while(!tok->Done()){
    auto mark = tok->Mark();
    Token first = tok->NextToken();
    Token second = tok->NextToken();
    
    if(!CompareString(second,"->")){
      tok->Rollback(mark);
      break;
    }

    *arr.PushElem() = first;
  }
  return EndArray(arr);
}

Opt<ConnectionDef> ParseConnection(Tokenizer* tok,Arena* out){
  ConnectionDef def = {};

  Opt<VarGroup> optOutPortion = ParseVarGroup(tok,out);
  PROPAGATE(optOutPortion);

  def.output = optOutPortion.value();

  Token type = tok->NextToken();
  if(CompareString(type,"=")){
    def.type = ConnectionDef::EQUALITY;
  } else if(CompareString(type,"^=")){
    // TODO: Only added this to make it easier to port a piece of C code.
    //       Do not know if needed or worth it to add.
  } else if(CompareString(type,"->")){
    def.transforms = CheckAndParseConnectionTransforms(tok,out);
    def.type = ConnectionDef::CONNECTION;
  } else {
    UNEXPECTED(type);
  }
  
  if(def.type == ConnectionDef::EQUALITY){
    def.expression = ParseSpecExpression(tok,out);
  } else if(def.type == ConnectionDef::CONNECTION){
    Opt<VarGroup> optInPortion = ParseVarGroup(tok,out);
    PROPAGATE(optInPortion);

    def.input = optInPortion.value();
  }

  EXPECT(tok,";");
  
  return def;
}

Opt<Array<VarDeclaration>> ParseModuleInputDeclaration(Tokenizer* tok,Arena* out){
  auto array = StartArray<VarDeclaration>(out);

  EXPECT(tok,"(");

  if(tok->IfNextToken(")")){
    return EndArray(array);
  }
  
  while(!tok->Done()){
    Opt<VarDeclaration> var = ParseVarDeclaration(tok);
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

Opt<InstanceDeclaration> ParseInstanceDeclaration(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  InstanceDeclaration res = {};

  while(1){
    Token potentialModifier = tok->PeekToken();

    if(CompareString(potentialModifier,"static")){
      if(res.modifier == InstanceDeclarationType_SHARE_CONFIG){
        ReportError(tok,potentialModifier,"We already seen a static modifier. Versat currently does not support static and share at the same time inside the same modifier");
        return {};
      }

      tok->AdvancePeek();
      res.modifier = InstanceDeclarationType_STATIC;
    } else if(CompareString(potentialModifier,"share")){
      if(res.modifier == InstanceDeclarationType_STATIC){
        ReportError(tok,potentialModifier,"We already seen a static modifier. Versat currently does not support static and share at the same time inside the same modifier");
        return {};
      }

      tok->AdvancePeek();

      EXPECT(tok,"(");

      // We only support one, so no need to parse a list for now.
      // When adding delay we are gonna need a list here.
      EXPECT(tok,"config"); // Only choice that is enabled, for now

      EXPECT(tok,")");
      res.modifier = InstanceDeclarationType_SHARE_CONFIG;
      
      res.typeName = tok->NextToken();
      CHECK_IDENTIFIER(res.typeName);

      if(tok->IfNextToken("(")){
#if 0
        res.negateShareNames = false;
        if(tok->IfNextToken("~")){
          res.negateShareNames = true;
        }
#endif

        // TODO: For now, we assume that every wire specified inside the spec file is a negative (remove share).
        auto toShare = StartArray<Token>(out);
        while(!tok->Done()){
          Token name = tok->NextToken();
          CHECK_IDENTIFIER(name);

          *toShare.PushElem() = name;

          if(tok->IfNextToken(",")){
            continue;
          } else {
            break;
          }
        }
      
        EXPECT(tok,")");

        res.shareNames = EndArray(toShare);
      }

      EXPECT(tok,"{");

      auto array = StartArray<VarDeclaration>(out);
    
      while(!tok->Done()){
        String peek = tok->PeekToken();

        if(CompareString(peek,"}")){
          break;
        }

        Opt<VarDeclaration> optVarDecl = ParseVarDeclaration(tok);
        PROPAGATE(optVarDecl);
      
        *array.PushElem() = optVarDecl.value();
      
        EXPECT(tok,";");
      }
      res.declarations = EndArray(array);
    
      EXPECT(tok,"}");
      return res;
    } else if(CompareString(potentialModifier,"using")){
      tok->AdvancePeek();

      EXPECT(tok,"(");

      auto array = StartArray<Token>(out);
      while(!tok->Done()){
        Token name = tok->NextToken();
        CHECK_IDENTIFIER(name);

        *array.PushElem() = name;
        
        if(tok->IfNextToken(")")){
          break;
        }
        EXPECT(tok,",");
      }
      res.addressGenUsed = EndArray(array);
    } else {
      break;
    }
  }

  res.typeName = tok->NextToken();
  CHECK_IDENTIFIER(res.typeName);

  Token possibleParameters = tok->PeekToken();
  auto list = PushArenaList<Pair<String,String>>(temp);
  if(CompareString(possibleParameters,"#")){
    tok->AdvancePeek();
    EXPECT(tok,"(");

    while(!tok->Done()){
      EXPECT(tok,".");
      String parameterName = tok->NextToken();

      EXPECT(tok,"(");

      String content = {};

      Opt<Token> remaining = tok->NextFindUntil(")");
      PROPAGATE(remaining);
      content = remaining.value();
      
      EXPECT(tok,")");
      
      String savedParameter = PushString(out,parameterName);
      String savedString = PushString(out,content);
      *list->PushElem() = {savedParameter,savedString}; 

      if(tok->IfNextToken(",")){
        continue;
      }

      break;
    }
    EXPECT(tok,")");

    res.parameters = PushArrayFromList(out,list);
  }

  Opt<VarDeclaration> optVarDecl = ParseVarDeclaration(tok);
  PROPAGATE(optVarDecl);

  res.declarations = PushArray<VarDeclaration>(out,1);
  res.declarations[0] = optVarDecl.value();

  EXPECT(tok,";");

  return res;
}

// Any returned String points to tokenizer content.
// As long as tokenizer is valid, strings returned by this function are also valid.
Opt<ModuleDef> ParseModuleDef(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  ModuleDef def = {};

  tok->AssertNextToken("module");

  def.name = tok->NextToken();
  CHECK_IDENTIFIER(def.name);
  
  Opt<Array<VarDeclaration>> optVar = ParseModuleInputDeclaration(tok,out);
  PROPAGATE(optVar);

  def.inputs = optVar.value();

  Token peek = tok->PeekToken();
  if(CompareString(peek,"->")){
    tok->AdvancePeek();
    def.numberOutputs = tok->NextToken();
    // TODO : Need to check that numberOutputs is actually a number.
  }
  
  ArenaList<InstanceDeclaration>* decls = PushArenaList<InstanceDeclaration>(temp);
  EXPECT(tok,"{");
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,";")){
      tok->AdvancePeek();
      continue;
    }

    if(CompareString(peek,"#")){
      break;
    }
    
    Opt<InstanceDeclaration> optDecl = ParseInstanceDeclaration(tok,out);
    PROPAGATE(optDecl); // TODO: We could try to keep going and find more errors

    *decls->PushElem() = optDecl.value();
  }
  def.declarations = PushArrayFromList(out,decls);

  EXPECT(tok,"#");
  
  ArenaList<ConnectionDef>* cons = PushArenaList<ConnectionDef>(temp);
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,";")){
      tok->AdvancePeek();
      continue;
    }

    if(CompareString(peek,"}")){
      break;
    }

    Opt<ConnectionDef> optCon = ParseConnection(tok,out);
    PROPAGATE(optCon); // TODO: We could try to keep going and find more errors
    
    *cons->PushElem() = optCon.value();
  }
  EXPECT(tok,"}");
  def.connections = PushArrayFromList(out,cons);
  
  return def;
}

Opt<TransformDef> ParseTransformDef(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  TransformDef def = {};

  tok->AssertNextToken("transform");

  def.name = tok->NextToken();
  CHECK_IDENTIFIER(def.name);
  
  EXPECT(tok,"{");

  ArenaList<ConnectionDef>* cons = PushArenaList<ConnectionDef>(temp);
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"}")){
      break;
    }

    Opt<ConnectionDef> optCon = ParseConnection(tok,out);
    PROPAGATE(optCon); // TODO: We could try to keep going and find more errors
    
    *cons->PushElem() = optCon.value();
  }
  def.connections = PushArrayFromList(out,cons);
  EXPECT(tok,"}");
  
  return def;
}

FUDeclaration* ParseIterative(Tokenizer* tok){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
#if 0
  Arena* perm = globalPermanent;
  tok->AssertNextToken("iterative");

  BLOCK_REGION(temp);

  InstanceTable* table = PushHashmap<String,FUInstance*>(temp,1000);
  Set<String>* names = PushSet<String>(temp,1000);

  String moduleName = tok->NextToken();
  String name = PushString(perm,moduleName);

  Accelerator* iterative = CreateAccelerator(name,AcceleratorPurpose_MODULE);

  tok->AssertNextToken("(");
  // Arguments
  int insertedInputs = 0;
  while(1){
    Token argument = tok->PeekToken();

    if(CompareString(argument,")")){
      break;
    }
    tok->AdvancePeek();

    Token peek = tok->PeekToken();
    if(CompareString(peek,",")){
      tok->AdvancePeek();
    }

    String name = PushString(perm,argument);

    table->Insert(name,CreateOrGetInput(iterative,name,insertedInputs++));
  }
  tok->AssertNextToken(")");
  tok->AssertNextToken("{");
  
  FUInstance* unit = nullptr;
  // Instance instantiation;
  while(1){
    if(tok->IfPeekToken("#")){
      break;
    }

    Token instanceTypeName = tok->NextToken();
    Token instanceName = tok->NextToken();
    tok->AssertNextToken(";");

    FUDeclaration* type = GetTypeByName(instanceTypeName);
    String name = PushString(perm,instanceName);

    FUInstance* created = CreateFUInstance(iterative,type,name);
    table->Insert(name,created);
    
    if(!unit){
      unit = created;
    }
  }
  tok->AssertNextToken("#");

  String latencyStr = tok->NextToken();
  int latency = ParseInt(latencyStr); // TODO: Need to have actual error handling

  FUInstance* outputInstance = nullptr;

  Hashmap<PortInstance,FUInstance*>* portInstanceToMux = PushHashmap<PortInstance,FUInstance*>(temp,10);

  FUDeclaration* type = BasicDeclaration::stridedMerge;
  int index = 0;
  // For in
  while(1){
    if(tok->IfPeekToken("}")){
      break;
    }

    Token peek = tok->PeekToken();

    int num = -1;
    if(CompareString(peek,"%")){
      tok->AdvancePeek();
      String number = tok->NextToken();
      num = ParseInt(number); // TODO: Need to have actual error handling
    }

    Var start = ParseVar(tok).value();  // TODO: Handle errors

    tok->AssertNextToken("->");

    Var end = ParseVar(tok).value();  // TODO: Handle errors
    tok->AssertNextToken(";");

    FUInstance* inst1 = nullptr;
    FUInstance* inst2 = nullptr;

    inst1 = table->GetOrFail(start.name);

    if(CompareString(end.name,"out")){
      if(!outputInstance){
        outputInstance = (FUInstance*) CreateFUInstance(iterative,BasicDeclaration::output,STRING("out"));
        table->Insert(STRING("out"),outputInstance);
      }

      inst2 = outputInstance;
    } else {
      inst2 = table->GetOrFail(end.name);
    }

    if(num == -1){
      ConnectUnit((PortExpression){inst1,start.extra},(PortExpression){inst2,end.extra});
      continue;
    }

    PortInstance instance = {};
    instance.inst = (FUInstance*) inst2;
    instance.port = end.extra.port.end;

    Assert(end.extra.port.start == end.extra.port.end); // For now do not handle ranges.

    GetOrAllocateResult<FUInstance*> res = portInstanceToMux->GetOrAllocate(instance);
    
    if(!res.alreadyExisted){
      static String names[] = {STRING("Merge0"),
                               STRING("Merge1"),
                               STRING("Merge2"),
                               STRING("Merge3"),
                               STRING("Merge4"),
                               STRING("Merge5"),
                               STRING("Merge6"),
                               STRING("Merge7")};

      *res.data = CreateFUInstance(iterative,type,names[index]);
      table->Insert(names[index],*res.data);
      index += 1;

      ConnectUnit((PortExpression){*res.data,start.extra},(PortExpression){inst2,end.extra});
    }

    Assert(num >= 0);
    end.extra.port.end = end.extra.port.start = num;
    ConnectUnit((PortExpression){inst1,start.extra},(PortExpression){*res.data,end.extra});
  }
  tok->AssertNextToken("}");

  return RegisterIterativeUnit(iterative,unit,latency,name,temp,temp2);
#endif
  NOT_POSSIBLE();
}

Opt<TypeAndInstance> ParseTypeAndInstance(Tokenizer* tok){
  Token typeName = tok->NextToken();

  CHECK_IDENTIFIER(typeName);

  tok->PeekToken();

  Token instanceName = {};
  if(tok->IfNextToken(":")){
    instanceName = tok->NextToken();

    CHECK_IDENTIFIER(instanceName);
  }

  TypeAndInstance res = {};
  res.typeName = typeName;
  res.instanceName = instanceName;
  
  return res;
}

Opt<HierarchicalName> ParseHierarchicalName(Tokenizer* tok){
  Token topInstance = tok->NextToken();

  CHECK_IDENTIFIER(topInstance);

  EXPECT(tok,".");

  Opt<Var> var = ParseVar(tok);
  PROPAGATE(var);

  HierarchicalName res = {};
  res.instanceName = topInstance;
  res.subInstance = var.value();

  return res;
}

Opt<MergeDef> ParseMerge(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);
  
  tok->AssertNextToken("merge");

  Array<Token> mergeModifiers = {};
  if(tok->IfNextToken("(")){
    auto tokenList = PushArenaList<Token>(temp);
    
    while(!tok->Done()){
      Token peek = tok->PeekToken();
      
      if(CompareString(peek,")")){
        break;
      }

      CHECK_IDENTIFIER(peek);
      *tokenList->PushElem() = peek;

      tok->AdvancePeek();
      
      tok->IfNextToken(",");
    }

    mergeModifiers = PushArrayFromList(out,tokenList);
    
    EXPECT(tok,")");
  }

  Token mergeName = tok->NextToken();
  CHECK_IDENTIFIER(mergeName);
  
  EXPECT(tok,"=");

  ArenaList<TypeAndInstance>* declarationList = PushArenaList<TypeAndInstance>(temp);
  while(!tok->Done()){
    Opt<TypeAndInstance> optType = ParseTypeAndInstance(tok);
    PROPAGATE(optType); // TODO: Maybe Synchronize? At this point it is a bit of a problem trying to keep the parser working

    *declarationList->PushElem() = optType.value();

    Token peek = tok->PeekToken();
    if(CompareString(peek,"|")){
      tok->AdvancePeek();
      continue;
    } else if(CompareString(peek,"{")){
      break;
    } else if(CompareString(peek,";")){
      tok->AdvancePeek();
      break;
    }
  }
  Array<TypeAndInstance> declarations = PushArrayFromList(out,declarationList);

  Array<SpecNode> specNodes = {};
  if(tok->IfNextToken("{")){
    ArenaList<SpecNode>* specList = PushArenaList<SpecNode>(temp);
    while(!tok->Done()){
      Token peek = tok->PeekToken();
      if(CompareString(peek,"}")){
        break;
      }

      Opt<HierarchicalName> leftSide = ParseHierarchicalName(tok);
      PROPAGATE(leftSide);

      EXPECT(tok,"-");

      Opt<HierarchicalName> rightSide = ParseHierarchicalName(tok);
      PROPAGATE(rightSide);

      EXPECT(tok,";");

      *specList->PushElem() = {leftSide.value(),rightSide.value()};
    }
    specNodes = PushArrayFromList(out,specList);

    EXPECT(tok,"}");
  }
  
  auto specificsArr = StartArray<SpecificMergeNode>(out);
  for(SpecNode node : specNodes){
    int firstIndex = -1;
    int secondIndex = -1;
    for(int i = 0; i < declarations.size; i++){
      TypeAndInstance& decl = declarations[i];
      if(CompareString(node.first.instanceName,decl.instanceName)){
        firstIndex = i;
      } 
      if(CompareString(node.second.instanceName,decl.instanceName)){
        secondIndex = i;
      } 
    }

    if(firstIndex == -1){
      Assert(false);
      // ReportError
    }
    if(secondIndex == -1){
      Assert(false);
      // ReportError
    }

    *specificsArr.PushElem() = {firstIndex,node.first.subInstance.name,secondIndex,node.second.subInstance.name};
  }
  Array<SpecificMergeNode> specifics = EndArray(specificsArr);

  MergeDef result = {};
  result.name = mergeName;
  result.declarations = declarations;
  result.specifics = specifics;
  result.mergeModifiers = mergeModifiers;
  
  return result;
}

FUDeclaration* InstantiateMerge(MergeDef def){
  TEMP_REGION(temp,nullptr);
  
  auto declArr = StartArray<FUDeclaration*>(temp);
  for(TypeAndInstance tp : def.declarations){
    FUDeclaration* decl = GetTypeByNameOrFail(tp.typeName); // TODO: Rewrite stuff so that at this point we know that the type must exist
    *declArr.PushElem() = decl;
  }
  Array<FUDeclaration*> decl = EndArray(declArr);

  String name = PushString(globalPermanent,def.name);

  bool error = false;
  MergeModifier modifier = MergeModifier_NONE;

  for(Token t : def.mergeModifiers){
    Opt<MergeModifier> parsed = META_mergeModifiers_ReverseMap(t);

    if(!parsed.has_value()){
      printf("Error, merge does not support option: %.*s\n",UNPACK_SS(t));
      error = true;
    }

    modifier = (MergeModifier) (modifier | parsed.value());
  }

  if(error){
    return nullptr;
  }
  
  return Merge(decl,name,def.specifics,modifier);
}

int GetRangeCount(Range<int> range){
  Assert(range.end >= range.start);
  return (range.end - range.start + 1);
}

// Connection type and number of connections
Pair<ConnectionType,int> GetConnectionInfo(Var var){
  int indexCount = GetRangeCount(var.index);
  int portCount = GetRangeCount(var.extra.port);
  int delayCount = GetRangeCount(var.extra.delay);

  if(indexCount == 1 && portCount == 1 && delayCount == 1){
    return {ConnectionType_SINGLE,1};
  }

  // We cannot have more than one range at the same time because otherwise how do we decide how to connnect them?
  if((indexCount != 1 && portCount != 1) ||
     (indexCount != 1 && delayCount != 1) ||
     (portCount != 1 && delayCount != 1)){
    return {ConnectionType_ERROR,0};
  }
  
  if(var.isArrayAccess && indexCount != 1){
    return {ConnectionType_ARRAY_RANGE,indexCount};
  }

  if(portCount != 1){
    return {ConnectionType_PORT_RANGE,portCount};
  } else if(delayCount != 1){
    return {ConnectionType_DELAY_RANGE,delayCount};
  }

  NOT_POSSIBLE("Every condition should have been checked by now");
}

bool IsValidGroup(VarGroup group){
  // TODO: Wether we can match the group or not.
  //       It depends on wether the ranges line up or not. 
  for(Var& var : group.vars){
    if(GetConnectionInfo(var).first == ConnectionType_ERROR){
      return false;
    }
  }

  return true;
}

int NumberOfConnections(VarGroup group){
  Assert(IsValidGroup(group));

  int count = 0;

  for(Var& var : group.vars){
    count += GetConnectionInfo(var).second;
  }

  return count;
}

GroupIterator IterateGroup(VarGroup group){
  GroupIterator iter = {};
  iter.group = group;
  return iter;
}

bool HasNext(GroupIterator iter){
  if(iter.groupIndex >= iter.group.vars.size){
    return false;
  }

  return true;
}

Var Next(GroupIterator& iter){
  Assert(HasNext(iter));

  Var var = iter.group.vars[iter.groupIndex];
  Pair<ConnectionType,int> info = GetConnectionInfo(var);
  
  ConnectionType type = info.first;
  int maxCount = info.second;

  Assert(type != ConnectionType_ERROR);

  Var res = var;

  if(type == ConnectionType_SINGLE){
    iter.groupIndex += 1;
  } else {
    switch(type){
    case ConnectionType_SINGLE: break;
    case ConnectionType_ERROR: break;
    case ConnectionType_PORT_RANGE:{
      int portBase = var.extra.port.start;
    
      res.extra.port.start = portBase + iter.varIndex; 
      res.extra.port.end = portBase + iter.varIndex; 
    } break;
    case ConnectionType_DELAY_RANGE:{
      int delayBase = var.extra.delay.start;
    
      res.extra.delay.start = delayBase + iter.varIndex; 
      res.extra.delay.end = delayBase + iter.varIndex; 
    } break;
    case ConnectionType_ARRAY_RANGE:{
      int indexBase = var.index.start;

      res.index.start = indexBase + iter.varIndex; 
      res.index.end = indexBase + iter.varIndex; 
    } break;
    }

    iter.varIndex += 1;
    if(iter.varIndex >= maxCount){
      iter.varIndex = 0;
      iter.groupIndex += 1;
    }
  }
  
  return res;
}

static std::unordered_map<String,Transformation> transformations = {};

Opt<int> ApplyTransforms(String content,int outPortStart,Array<Token> transforms){
  // TODO(Performance): Trying to find transforms all the time is wasteful.
  int outPort = outPortStart;
  for(Token& transformToken : transforms){
    auto iter = transformations.find(transformToken);

    if(iter == transformations.end()){
      ReportError(content,transformToken,"Following transformation does not exist");
      return {};
    }

    Transformation transform = iter->second;

    Array<int> map = transform.map;
    if(outPort >= transform.inputs){
      const char* str = StaticFormat("Port connection (%d) outside transformation range (%d)",outPort,map.size);
      ReportError(content,transformToken,str);
      return {};
    }
          
    outPort = map[outPort];
  }

  return outPort;
}

static bool InstantiateTransform(Tokenizer* tok,TransformDef def){
  TEMP_REGION(temp,nullptr);
  Arena* perm = globalPermanent;

  if(transformations.find(def.name) != transformations.end()){
    ReportError(tok,def.name,"Trasform already exists");
    return true;
  }

  Hashmap<String,int>* table = PushHashmap<String,int>(temp,1000);
  int portIndex = 0;
  bool error = false;
  for(VarDeclaration& decl : def.inputs){
    if(CompareString(decl.name,STRING("out"))){
      ReportError(tok,decl.name,"Cannot use special out unit as module input");
      error = true;
    }

    if(decl.isArray){
      for(int i = 0; i < decl.arraySize; i++){
        String actualName = GetActualArrayName(decl.name,i,temp);
        table->Insert(actualName,portIndex++);
      }
    } else {
      table->Insert(decl.name,portIndex++);
    }
  }

  Set<int>* inputsSeen = PushSet<int>(temp,1000);
  Hashmap<int,int>* transformation = PushHashmap<int,int>(temp,1000);
  for(ConnectionDef& decl : def.connections){
    int nOutConnections = NumberOfConnections(decl.output);
    int nInConnections = NumberOfConnections(decl.input);

    // TODO: Proper error report by making VarGroup a proper struct that stores a token for the entire parsed text.
    
    if(nOutConnections != nInConnections){
      printf("Number of connections missmatch %d to %d\n",nOutConnections,nInConnections);
      Assert(false);
    }

    GroupIterator out = IterateGroup(decl.output);
    GroupIterator in  = IterateGroup(decl.input);
    while(HasNext(out) && HasNext(in)){
      BLOCK_REGION(temp);

      Var outVar = Next(out);
      Var inVar = Next(in);
        
      Assert(inVar.extra.delay.high == 0); // For now, inputs cannot have delay.

      String outName = outVar.name;
      if(outVar.isArrayAccess){
        outName = GetActualArrayName(outName,outVar.index.low,temp);
      }
      String inName = inVar.name;
      if(inVar.isArrayAccess){
        inName = GetActualArrayName(inName,inVar.index.low,temp);
      }

      if(CompareString(outName,STRING("out"))){
        ReportError(tok,outVar.name,"Cannot use special out unit as an input");
        error = true;
        break;
      }

      if(!CompareString(inName,STRING("out"))){
        ReportError(tok,inVar.name,"Only out is allowed as input of connection");
        error = true;
        break;
      }

      if(!table->Exists(outName)){
        ReportError(tok,outVar.name,"Did not find the following input");
        error = true;
        break;
      }
        
      int outPort = table->GetOrFail(outName);

      Opt<int> optOutPort = ApplyTransforms(tok->GetContent(),outPort,decl.transforms);
      if(!optOutPort.has_value()){
        error = true;
        break;
      }
        
      outPort = optOutPort.value();
      int inPort  = inVar.extra.port.low;

      if(inputsSeen->ExistsOrInsert(inPort)){
        ReportError(tok,inVar.name,"Input already was set once before. Cannot connect two ports to one input");
        error = true;
        break;
      }

      transformation->Insert(outPort,inPort);
    }
      
    Assert(HasNext(out) == HasNext(in));
  }

  Array<int> transform = PushArray<int>(perm,transformation->nodesUsed);
  int inputs = 0;
  int outputs = 0;
  for(Pair<int,int*> p : transformation){
    transform[p.first] = *p.second;
    inputs = std::max(inputs,p.first);
    outputs = std::max(outputs,*p.second);
  }
  
  String storedString = PushString(perm,def.name);

  Transformation trans = {};
  trans.map = transform;
  trans.inputs = inputs + 1; // Plus one because inputs is an index
  trans.outputs = outputs + 1;

  transformations.insert({storedString,trans});
  return error;
}

FUInstance* CreateFUInstanceWithParameters(Accelerator* accel,FUDeclaration* type,String name,InstanceDeclaration decl){
  FUInstance* inst = CreateFUInstance(accel,type,name);
  
  for(auto pair : decl.parameters){
    bool result = SetParameter(inst,pair.first,pair.second);

    if(!result){
      printf("Warning: Parameter %.*s for instance %.*s in module %.*s does not exist\n",UNPACK_SS(pair.first),UNPACK_SS(inst->name),UNPACK_SS(accel->name));
    }
  }

  return inst;
}

FUDeclaration* InstantiateModule(String content,ModuleDef def){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  Arena* perm = globalPermanent;
  Accelerator* circuit = CreateAccelerator(def.name,AcceleratorPurpose_MODULE);

  InstanceTable* table = PushHashmap<String,FUInstance*>(temp,1000);
  InstanceName* names = PushSet<String>(temp,1000);
  bool error = false;

  ArenaList<Pair<String,int>>* allArrayDefinitons = PushArenaList<Pair<String,int>>(temp);
  // TODO: Need to detect when multiple instances with same name
  int insertedInputs = 0;
  for(VarDeclaration& decl : def.inputs){
    if(CompareString(decl.name,STRING("out"))){
      ReportError(content,decl.name,"Cannot use special out unit as module input");
      error = true;
    }
    
    if(decl.isArray){
      *allArrayDefinitons->PushElem() = {PushString(perm,decl.name),decl.arraySize};
      for(int i = 0; i < decl.arraySize; i++){
        String actualName = GetActualArrayName(decl.name,i,temp);
        FUInstance* input = CreateOrGetInput(circuit,actualName,insertedInputs++);
        names->Insert(actualName);
        table->Insert(actualName,input);
      }
    } else {
      FUInstance* input = CreateOrGetInput(circuit,decl.name,insertedInputs++);
      names->Insert(decl.name);
      table->Insert(decl.name,input);
    }
  }

  int shareIndex = 0;
  for(InstanceDeclaration& decl : def.declarations){
    for(VarDeclaration& var : decl.declarations){
      if(CompareString(var.name,STRING("out"))){
        ReportError(content,var.name,"Cannot use special out unit as module input");
        error = true;
        break;
      }
    }
    
    FUDeclaration* type = GetTypeByName(decl.typeName);
    if(type == nullptr){
      ReportError(content,decl.typeName,"Given type was not found");
      error = true;
      // TODO: In order to collect more errors, we could keep running using a type that contains infinite inputs and outputs. 
      break; // For now skip over.
    }
    
    switch(decl.modifier){
    case InstanceDeclarationType_SHARE_CONFIG:{
      for(VarDeclaration& varDecl : decl.declarations){
        if(varDecl.isArray){
          *allArrayDefinitons->PushElem() = {PushString(perm,varDecl.name),varDecl.arraySize};

          for(int i = 0; i < varDecl.arraySize; i++){
            String actualName = GetActualArrayName(varDecl.name,i,temp);
            FUInstance* inst = CreateFUInstanceWithParameters(circuit,type,actualName,decl);

            inst->addressGenUsed = CopyArray<String,Token>(decl.addressGenUsed,perm); // TODO: Should be accelerator arena
            
            table->Insert(actualName,inst);
            ShareInstanceConfig(inst,shareIndex);

            for(Token partialShareName : decl.shareNames){
              bool foundOne = false;
              for(int ii = 0; ii < inst->declaration->configs.size; ii++){
                if(inst->declaration->configs[ii].name == partialShareName){
                  inst->isSpecificConfigShared[ii] = false;
                  foundOne = true;
                }
              }

              if(!foundOne){
                // TODO: Add location to the warnings using the data stored in the tokens.
                printf("Warning, inst '%.*s' of type '%.*s' does not have configuration of name '%.*s' to share\n",UNPACK_SS(inst->name),UNPACK_SS(inst->declaration->name),UNPACK_SS(partialShareName));
              }
            }
          }
        } else {
          // TODO: Missing sharing, right?
          FUInstance* inst = CreateOrGetInput(circuit,varDecl.name,insertedInputs++);
          inst->addressGenUsed = CopyArray<String,Token>(decl.addressGenUsed,perm); // TODO: Should be accelerator arena

          names->Insert(varDecl.name);
          table->Insert(varDecl.name,inst);
        }
      }
      shareIndex += 1;
    } break;

    case InstanceDeclarationType_NONE:
    case InstanceDeclarationType_STATIC:
    {
      Assert(decl.declarations.size == 1);
      
      VarDeclaration varDecl = decl.declarations[0];
      
      if(varDecl.isArray){
        *allArrayDefinitons->PushElem() = {PushString(perm,varDecl.name),varDecl.arraySize};
        for(int i = 0; i < varDecl.arraySize; i++){
          String actualName = GetActualArrayName(varDecl.name,i,temp);

          FUInstance* inst = CreateFUInstanceWithParameters(circuit,type,actualName,decl);
          inst->addressGenUsed = CopyArray<String,Token>(decl.addressGenUsed,perm); // TODO: Should be accelerator arena

          table->Insert(actualName,inst);

          if(decl.modifier == InstanceDeclarationType_STATIC){
            SetStatic(circuit,inst);
          }
        }
      } else {
        FUInstance* inst = CreateFUInstanceWithParameters(circuit,type,varDecl.name,decl);
        inst->addressGenUsed = CopyArray<String,Token>(decl.addressGenUsed,perm); // TODO: Should be accelerator arena

        table->Insert(varDecl.name,inst);

        if(decl.modifier == InstanceDeclarationType_STATIC){
          SetStatic(circuit,inst);
        }
      }
    } break;
    }
  }

  FUInstance* outputInstance = nullptr;
  for(ConnectionDef& decl : def.connections){
    if(decl.type == ConnectionDef::EQUALITY){
      // Make sure that not a single equality value is named out.

      Assert(decl.output.vars.size == 1); // Only allow one for equality, for now
      Var outVar = decl.output.vars[0];

      if(CompareString(outVar.name,STRING("out"))){
        ReportError(content,outVar.name,"Cannot use special out unit as input in an equality");
        error = true;
        break;
      }
      
      String name = outVar.name;
      if(outVar.isArrayAccess){
        name = GetActualArrayName(name,outVar.index.low,temp);
      }
      
      PortExpression portSpecExpression = InstantiateSpecExpression(decl.expression,circuit,table,names);
      FUInstance* inst = portSpecExpression.inst;
      String uniqueName = GetUniqueName(name,perm,names);
      inst->name = PushString(perm,uniqueName);

      names->Insert(name);
      table->Insert(name,inst);
    } else if(decl.type == ConnectionDef::CONNECTION){
      // For now only allow one var on the input side

      int nOutConnections = NumberOfConnections(decl.output);
      int nInConnections = NumberOfConnections(decl.input);

      // TODO: Proper error report by making VarGroup a proper struct that stores a token for the entire parsed text.
      if(nOutConnections != nInConnections){
        const char* text = StaticFormat("Number of connections missmatch %d to %d\n",nOutConnections,nInConnections);
        ReportError(content,decl.output.fullText,text);
        Assert(false);
      }

      GroupIterator out = IterateGroup(decl.output);
      GroupIterator in  = IterateGroup(decl.input);

      while(HasNext(out) && HasNext(in)){
        BLOCK_REGION(temp);

        Var outVar = Next(out);
        Var inVar = Next(in);
        
        Assert(inVar.extra.delay.high == 0); // For now, inputs cannot have delay.

        String outName = outVar.name;
        if(outVar.isArrayAccess){
          outName = GetActualArrayName(outName,outVar.index.low,temp);
        }
        String inName = inVar.name;
        if(inVar.isArrayAccess){
          inName = GetActualArrayName(inName,inVar.index.low,temp);
        }

        if(CompareString(outName,STRING("out"))){
          ReportError(content,outVar.name,"Cannot use special out unit as an input");
          error = true;
          break;
        }
        
        FUInstance** optOutInstance = table->Get(outName);
        if(optOutInstance == nullptr){
          ReportError(content,outVar.name,"Did not find the following instance");
          error = true;
          break;
        }
        
        FUInstance** optInInstance = nullptr;

        if(CompareString(inName,"out")){
          if(!outputInstance){
            outputInstance = (FUInstance*) CreateFUInstance(circuit,BasicDeclaration::output,STRING("out"));
          }

          optInInstance = &outputInstance;
        } else {
          optInInstance = table->Get(inName);
        }

        if(optInInstance == nullptr){
          ReportError(content,outVar.name,"Did not find the following instance");
          error = true;
          break;
        }
        
        FUInstance* outInstance = *optOutInstance;
        FUInstance* inInstance = *optInInstance;

        int outPort = outVar.extra.port.low;

        Opt<int> optOutPort = ApplyTransforms(content,outPort,decl.transforms);
        if(!optOutPort.has_value()){
          error = true;
          break;
        }
        
        outPort = optOutPort.value();
        int inPort  = inVar.extra.port.low;
        ConnectUnits(outInstance,outPort,inInstance,inPort,outVar.extra.delay.low);
      }
      
      Assert(HasNext(out) == HasNext(in));
    }
  }

  if(error){
    // TODO: Better error handling.
    printf("Error on InstantiateModule\n");
  }
  
  // Care to never put 'out' inside the table
  FUInstance** outInTable = table->Get(STRING("out"));
  Assert(!outInTable);

  // TODO: We have the parsing working correctly. I think the best way of approaching this is to first rewrite the config code to use the array everywhere (remove the configPos variable and replace it with an array for each configuration inside a unit).
  //       When we have everything working and the normal tests also passing after the change, only then can we try to add the partial share test and start working towards that.
  //       Two things that I probably will have to do: augment the debugger to display arrays using a newline (occupy more vertical space still inside the same column row).
  //       Change most of the code that used configPos and configSize to use values directly.
  //       After the change, each config should have it's own config position. The config code cannot assume that everything works continuous (although normal units will probably not change). 
  
  FUDeclaration* res = RegisterSubUnit(circuit,SubUnitOptions_BAREBONES);
  res->definitionArrays = PushArrayFromList(perm,allArrayDefinitons);
  return res;
}

void Synchronize(Tokenizer* tok,BracketList<const char*> syncPoints){
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    for(const char* point : syncPoints){
      if(CompareString(peek,point)){
        return;
      }
    }

    tok->AdvancePeek();
  }
}

Array<TypeDefinition> ParseVersatSpecification(String content,Arena* out){
  TEMP_REGION(temp,out);
  Tokenizer tokenizer = Tokenizer(content,".%=#[](){}+:;,*~-",{"->=","->",">><","><<",">>","<<","..","^="});
  Tokenizer* tok = &tokenizer;

  ArenaList<TypeDefinition>* typeList = PushArenaList<TypeDefinition>(temp);
  
  bool anyError = false;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"module")){
      Opt<ModuleDef> moduleDef = ParseModuleDef(tok,out);

      if(moduleDef.has_value()){
        TypeDefinition def = {};
        def.type = DefinitionType_MODULE;
        def.module = moduleDef.value();

        *typeList->PushElem() = def;
      } else {
        anyError = true;
      }
    } else if(CompareString(peek,"merge")){
      Opt<MergeDef> mergeDef = ParseMerge(tok,out);
      
      if(mergeDef.has_value()){
        TypeDefinition def = {};
        def.type = DefinitionType_MERGE;
        def.merge = mergeDef.value();

        *typeList->PushElem() = def;
      } else {
        anyError = true;
      }
    } else if(CompareString(peek,"AddressGen")){
      Opt<AddressGenDef> def = ParseAddressGen(tok,globalPermanent);
      
      if(def.has_value()){
        AddressGenDef* buffer = addressGens.Alloc();
        *buffer = def.value();
      } else {
        anyError = true;
      }
    } else {
      // TODO: Report error, 
      ReportError(tok,peek,"Unexpected token in global scope");
      tok->AdvancePeek();
      Synchronize(tok,{"debug","module","iterative","merge","transform"});
    }
  }

  if(anyError){
    exit(-1);
  }

  return PushArrayFromList(out,typeList);
}

Array<Token> TypesUsed(TypeDefinition def,Arena* out){
  TEMP_REGION(temp,out);

  switch(def.type){
  case DefinitionType_MERGE: {
    // TODO: How do we deal with same types being used?
    //       Do we just ignore it?
    Array<Token> result = Extract(def.merge.declarations,out,&TypeAndInstance::typeName);
    
    return result;
  } break;
  case DefinitionType_MODULE: {
    Array<Token> result = Extract(def.module.declarations,temp,&InstanceDeclaration::typeName);

    return Unique(result,out);
  } break;
  case DefinitionType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  }; 
  default: Assert(false);
  }

  return {};
}

FUDeclaration* InstantiateBarebonesSpecifications(String content,TypeDefinition def){
  switch(def.type){
  case DefinitionType_MERGE: {
    return InstantiateMerge(def.merge);
  } break;
  case DefinitionType_MODULE: {
    return InstantiateModule(content,def.module);
  } break;
  case DefinitionType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  }; 
  default: Assert(false);
  }

  return nullptr;
}
  
FUDeclaration* InstantiateSpecifications(String content,TypeDefinition def){
  switch(def.type){
  case DefinitionType_MERGE: {
    return InstantiateMerge(def.merge);
  } break;
  case DefinitionType_MODULE: {
    return InstantiateModule(content,def.module);
  } break;
  case DefinitionType_ITERATIVE:{
    NOT_IMPLEMENTED("yet");
  }; 
  default: Assert(false);
  }

  return nullptr;
}

Opt<AddressGenDef> ParseAddressGen(Tokenizer* tok,Arena* out){
  TEMP_REGION(temp,out);

  EXPECT(tok,"AddressGen");

  Token typeStr = tok->NextToken();

  AddressGenType type;
  if(CompareString(typeStr,"Mem")){
    type = AddressGenType_MEM;
  } else if(CompareString(typeStr,"VReadLoad")){
    type = AddressGenType_VREAD_LOAD;
  } else if(CompareString(typeStr,"VReadOutput")){
    type = AddressGenType_VREAD_OUTPUT;
  } else if(CompareString(typeStr,"VWriteInput")){
    type = AddressGenType_VWRITE_INPUT;
  } else if(CompareString(typeStr,"VWriteStore")){
    type = AddressGenType_VWRITE_STORE;
  }

  Token name = tok->NextToken();
  CHECK_IDENTIFIER(name);

  Array<Token> inputsArr = {};
  if(tok->IfNextToken("(")){
    ArenaList<Token>* inputs = PushArenaList<Token>(temp);

    while(!tok->Done()){
      Token name = tok->NextToken();
      CHECK_IDENTIFIER(name);
      *inputs->PushElem() = name;
      
      if(tok->IfNextToken(",")){
        continue;
      } else {
        break;
      }
    }

    inputsArr = PushArrayFromList(out,inputs);
    EXPECT(tok,")");
  }

  ArenaList<AddressGenForDef>* loops = PushArenaList<AddressGenForDef>(temp);
  SymbolicExpression* symbolic = nullptr;
  SymbolicExpression* internalSymbolic = nullptr;
  SymbolicExpression* externalSymbolic = nullptr;
  Token externalName = {};
  while(!tok->Done()){
    Token construct = tok->NextToken();
    if(CompareString(construct,"for")){

      Token loopVariable = tok->NextToken();
      CHECK_IDENTIFIER(loopVariable);

      Token start = tok->NextToken();
      EXPECT(tok,"..");
      Token end = tok->NextToken();

      *loops->PushElem() = (AddressGenForDef){.loopVariable = loopVariable,.start = start,.end = end};
    } else if(CompareString(construct,"addr")){
      EXPECT(tok,"=");

      auto symbolicExpression = ParseSymbolicExpression(tok,out);
      PROPAGATE_POINTER_OPT(symbolicExpression);
      symbolic = symbolicExpression;
      
      EXPECT(tok,";");
      break;
    } else {      
      EXPECT(tok,"[");
      auto symbolicExpression = ParseSymbolicExpression(tok,out);
      EXPECT(tok,"]");
      
      PROPAGATE_POINTER_OPT(symbolicExpression);
      
      EXPECT(tok,"=");
      Token other = tok->NextToken();

      EXPECT(tok,"[");
      auto symbolicExpression2 = ParseSymbolicExpression(tok,out);
      EXPECT(tok,"]");

      EXPECT(tok,";");

      PROPAGATE_POINTER_OPT(symbolicExpression2);
      
      if(CompareString(construct,"mem")){
        internalSymbolic = symbolicExpression;
        externalSymbolic = symbolicExpression2;
        externalName = PushString(out,other);
      } else {
        internalSymbolic = symbolicExpression2;
        externalSymbolic = symbolicExpression;
        externalName = PushString(out,construct);
      }
      
      break;
    }
  }
 
  AddressGenDef def = {};
  def.name = name;
  def.type = type;
  def.inputs = inputsArr;
  def.loops = PushArrayFromList(out,loops);
  def.externalName = externalName;
  def.symbolic = symbolic;
  def.symbolicInternal = internalSymbolic;
  def.symbolicExternal = externalSymbolic;
  
  return def;
}

// TODO: This function and the non generic instantiate need to be merged into the same flow.
String InstantiateGenericAddressGen(AddressGen gen,String typeStructName,Arena* out){
  TEMP_REGION(temp,out);

  Array<AddressGenLoopSpecificatonSym> loopSpecSymbolic = gen.loopSpecSymbolic;
  Array<AddressGenLoopSpecificatonSym> internalSpecSym = gen.internalSpecSym;
  Array<AddressGenLoopSpecificatonSym> externalSpecSym = gen.externalSpecSym;

  String constantExpr = gen.constantExpr;
  Array<Token> constants = gen.constants;

  String name = gen.name;
  String externalName = gen.externalName;
  AddressGenType type = gen.type;
  
  auto PushFunctionNameAndFirstArgument = [](StringBuilder* builder,const char* type,String addressGenName,String structName) -> void {
    //builder->PushString("static void ");
    builder->PushString("%s_",type);
    builder->PushString("%.*s(volatile %.*sConfig* config",UNPACK_SS(addressGenName),UNPACK_SS(structName));
  };
  
  auto builder = StartString(temp);

  builder->PushString("#ifdef __cplusplus\n");

#if 0
  builder->PushString("static int LoopSize_%.*s_%.*s(int temp",UNPACK_SS(name),UNPACK_SS(typeStructName));
  for(Token t : constants){
    builder->PushString(",int %.*s",UNPACK_SS(t));
  }
  builder->PushString("){\n");

  builder->PushString("   return 1");
  for(String repr : gen.loopRepr){
    builder->PushString(" * %.*s",UNPACK_SS(repr));
  }

  builder->PushString(";\n");
  builder->PushString("}\n\n\n");
#endif
  
  // TODO: All these different types should be removed and a generic approach used. The only difference is the logic for VRead/VWrite different usages (output vs access memory) and the fact that the units have differences in the name of the signals, which is not a reason to have multiple different types. Need to map loop to name
  switch(type){
  case AddressGenType_MEM:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    //builder->PushString("static void Configure_%.*s(volatile %.*sConfig* config",UNPACK_SS(name),UNPACK_SS(typeStructName));
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");

    if(!Empty(constantExpr)){
      builder->PushString("   config->start = %.*s;\n",UNPACK_SS(constantExpr));
    }
    
    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->period%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->iterations%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;
  case AddressGenType_VREAD_LOAD:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    //builder->PushString("static void Configure_%.*s(volatile %.*sConfig* config",UNPACK_SS(name),UNPACK_SS(typeStructName));

    for(Token t : constants){
      if(CompareString(t,"ext")){
        builder->PushString(",iptr %.*s",UNPACK_SS(t));
      } else {
        builder->PushString(",int %.*s",UNPACK_SS(t));
      }
    }
    builder->PushString("){\n");

    Assert(!Empty(externalName)); // TODO: Probably a parser error or something. Weird to see it here
    builder->PushString("   config->ext_addr = %.*s;\n",UNPACK_SS(externalName));
    
    AddressGenLoopSpecificatonSym in = internalSpecSym[0];

    builder->PushString("   config->read_per = %.*s;\n",UNPACK_SS(in.periodExpression));
    builder->PushString("   config->read_incr = %.*s;\n",UNPACK_SS(in.incrementExpression));

    builder->PushString("   config->read_duty = %.*s;\n",UNPACK_SS(in.dutyExpression));
    builder->PushString("   config->read_iter = %.*s;\n",UNPACK_SS(in.iterationExpression));
    builder->PushString("   config->read_shift = %.*s;\n",UNPACK_SS(in.shiftExpression));

    //for(int i = 0; i < externalSpec.size; i++){
    AddressGenLoopSpecificatonSym ext = externalSpecSym[0];
    builder->PushString("   config->read_length = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.periodExpression));
    builder->PushString("   config->read_amount_minus_one = (%.*s) - 1;\n",UNPACK_SS(ext.iterationExpression));
    builder->PushString("   config->read_addr_shift = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.shiftWithoutRemovingIncrement));

    builder->PushString("   config->read_enabled = 1;\n");
    builder->PushString("   config->pingPong = 1;\n");

#if 0
    builder->PushString("}\n");
    
    // TODO: This is kinda of hardcoded, but since we will eventually have to deal with address generation when trying to integrate this with merged accelertors, we will only handle this then.
    // TODO: We are also putting the output values here.
    // Start of simulation
    builder->PushString("static int Simulate_%.*s_%.*s(iptr* arrayToFill,int arrayMaxSize,volatile %.*sConfig* config){\n",UNPACK_SS(name),UNPACK_SS(typeStructName),UNPACK_SS(typeStructName));
    builder->PushString("   AddressGenArguments args = {};\n");

    builder->PushString("   args.start_address = config->ext_addr;\n");
    builder->PushString("   args.ext_addr = config->ext_addr;\n");
    builder->PushString("   args.period = config->output_per;\n");
    builder->PushString("   args.incr = config->output_incr;\n");
    builder->PushString("   args.duty = config->output_duty;\n");
    builder->PushString("   args.iterations = config->output_iter;\n");
    builder->PushString("   args.shift = config->output_shift;\n");
    builder->PushString("   args.length = config->read_length;\n");
    builder->PushString("   args.amount_minus_one = config->read_amount_minus_one;\n");
    builder->PushString("   args.addr_shift = config->read_addr_shift;\n");

    builder->PushString("   return SimulateAddressGen(arrayToFill,arrayMaxSize,args);\n");
#endif
    
  } break;

  case AddressGenType_VREAD_OUTPUT:{
    // TODO: Basically a copy of MEM but with the different named signals, since MEM has slight differences in naming conventions.
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");
    
    if(!Empty(constantExpr)){
      builder->PushString("   config->output_start = %.*s;\n",UNPACK_SS(constantExpr));
    }

    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->output_per%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->output_incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->output_duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->output_iter%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->output_shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;

  case AddressGenType_VWRITE_INPUT:{
    // TODO: Basically a copy of VREAD_OUTPUT and MEM but with the different named signals, since MEM has slight differences in naming conventions.
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");

    if(!Empty(constantExpr)){
      builder->PushString("   config->input_start = %.*s;\n",UNPACK_SS(constantExpr));
    }
    
    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->input_per%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->input_incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->input_duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->input_iter%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->input_shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;

  case AddressGenType_VWRITE_STORE:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);

    for(Token t : constants){
      if(CompareString(t,"ext")){
        builder->PushString(",iptr %.*s",UNPACK_SS(t));
      } else {
        builder->PushString(",int %.*s",UNPACK_SS(t));
      }
    }
    builder->PushString("){\n");

    Assert(!Empty(externalName)); // TODO: Probably a parser error or something. Weird to see it here
    builder->PushString("   config->ext_addr = %.*s;\n",UNPACK_SS(externalName));
    
    AddressGenLoopSpecificatonSym in = internalSpecSym[0]; 
    builder->PushString("   config->write_per = %.*s;\n",UNPACK_SS(in.periodExpression));
    builder->PushString("   config->write_incr = %.*s;\n",UNPACK_SS(in.incrementExpression));
    builder->PushString("   config->write_duty = %.*s;\n",UNPACK_SS(in.dutyExpression));
    builder->PushString("   config->write_iter = %.*s;\n",UNPACK_SS(in.iterationExpression));
    builder->PushString("   config->write_shift = %.*s;\n",UNPACK_SS(in.shiftExpression));

    AddressGenLoopSpecificatonSym ext = externalSpecSym[0];

    builder->PushString("   config->write_length = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.periodExpression));
    builder->PushString("   config->write_amount_minus_one = (%.*s) - 1;\n",UNPACK_SS(ext.iterationExpression));
    builder->PushString("   config->write_addr_shift = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.shiftWithoutRemovingIncrement));

    builder->PushString("   config->write_enabled = 1;\n");
    builder->PushString("   config->pingPong = 1;\n");

  } break;

  }
  builder->PushString("}\n");

  builder->PushString("#endif //__cplusplus\n");
  
  return EndString(out,builder);
}

String InstantiateAddressGen(AddressGen gen,String typeStructName,Arena* out){
  TEMP_REGION(temp,out);

  Array<AddressGenLoopSpecificatonSym> loopSpecSymbolic = gen.loopSpecSymbolic;
  Array<AddressGenLoopSpecificatonSym> internalSpecSym = gen.internalSpecSym;
  Array<AddressGenLoopSpecificatonSym> externalSpecSym = gen.externalSpecSym;

  String constantExpr = gen.constantExpr;
  Array<Token> constants = gen.constants;

  String name = gen.name;
  String externalName = gen.externalName;
  AddressGenType type = gen.type;

  auto PushFunctionNameAndFirstArgument = [](StringBuilder* builder,const char* type,String addressGenName,String structName) -> void {
    //builder->PushString("static void ");
    builder->PushString("%s_",type);
    builder->PushString("%.*s_%.*s(volatile %.*sConfig* config",UNPACK_SS(addressGenName),UNPACK_SS(structName),UNPACK_SS(structName));
  };
  
  auto builder = StartString(temp);

  builder->PushString("static int LoopSize_%.*s_%.*s(int temp",UNPACK_SS(name),UNPACK_SS(typeStructName));
  for(Token t : constants){
    builder->PushString(",int %.*s",UNPACK_SS(t));
  }
  builder->PushString("){\n");

  builder->PushString("   return 1");
  for(String repr : gen.loopRepr){
    builder->PushString(" * %.*s",UNPACK_SS(repr));
  }

  builder->PushString(";\n");
  builder->PushString("}\n\n\n");
  
  // TODO: All these different types should be removed and a generic approach used. The only difference is the logic for VRead/VWrite different usages (output vs access memory) and the fact that the units have differences in the name of the signals, which is not a reason to have multiple different types. Need to map loop to name
  switch(type){
  case AddressGenType_MEM:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    //builder->PushString("static void Configure_%.*s(volatile %.*sConfig* config",UNPACK_SS(name),UNPACK_SS(typeStructName));
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");

    if(!Empty(constantExpr)){
      builder->PushString("   config->start = %.*s;\n",UNPACK_SS(constantExpr));
    }
    
    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->period%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->iterations%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;
  case AddressGenType_VREAD_LOAD:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    //builder->PushString("static void Configure_%.*s(volatile %.*sConfig* config",UNPACK_SS(name),UNPACK_SS(typeStructName));

    for(Token t : constants){
      if(CompareString(t,"ext")){
        builder->PushString(",iptr %.*s",UNPACK_SS(t));
      } else {
        builder->PushString(",int %.*s",UNPACK_SS(t));
      }
    }
    builder->PushString("){\n");

    Assert(!Empty(externalName)); // TODO: Probably a parser error or something. Weird to see it here
    builder->PushString("   config->ext_addr = %.*s;\n",UNPACK_SS(externalName));
    
    AddressGenLoopSpecificatonSym in = internalSpecSym[0];

    builder->PushString("   config->read_per = %.*s;\n",UNPACK_SS(in.periodExpression));
    builder->PushString("   config->read_incr = %.*s;\n",UNPACK_SS(in.incrementExpression));

    builder->PushString("   config->read_duty = %.*s;\n",UNPACK_SS(in.dutyExpression));
    builder->PushString("   config->read_iter = %.*s;\n",UNPACK_SS(in.iterationExpression));
    builder->PushString("   config->read_shift = %.*s;\n",UNPACK_SS(in.shiftExpression));

    //for(int i = 0; i < externalSpec.size; i++){
    AddressGenLoopSpecificatonSym ext = externalSpecSym[0];
    builder->PushString("   config->read_length = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.periodExpression));
    builder->PushString("   config->read_amount_minus_one = (%.*s) - 1;\n",UNPACK_SS(ext.iterationExpression));
    builder->PushString("   config->read_addr_shift = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.shiftWithoutRemovingIncrement));
    builder->PushString("   config->read_enabled = 1;\n");
    builder->PushString("   config->pingPong = 1;\n");

    builder->PushString("}\n");
    
    // TODO: This is kinda of hardcoded, but since we will eventually have to deal with address generation when trying to integrate this with merged accelertors, we will only handle this then.
    // TODO: We are also putting the output values here.
    // Start of simulation
    builder->PushString("static int Simulate_%.*s_%.*s(iptr* arrayToFill,int arrayMaxSize,volatile %.*sConfig* config){\n",UNPACK_SS(name),UNPACK_SS(typeStructName),UNPACK_SS(typeStructName));
    builder->PushString("   AddressGenArguments args = {};\n");

    builder->PushString("   args.start_address = config->ext_addr;\n");
    builder->PushString("   args.ext_addr = config->ext_addr;\n");
    builder->PushString("   args.period = config->output_per;\n");
    builder->PushString("   args.incr = config->output_incr;\n");
    builder->PushString("   args.duty = config->output_duty;\n");
    builder->PushString("   args.iterations = config->output_iter;\n");
    builder->PushString("   args.shift = config->output_shift;\n");
    builder->PushString("   args.length = config->read_length;\n");
    builder->PushString("   args.amount_minus_one = config->read_amount_minus_one;\n");
    builder->PushString("   args.addr_shift = config->read_addr_shift;\n");

    builder->PushString("   return SimulateAddressGen(arrayToFill,arrayMaxSize,args);\n");
    
  } break;

  case AddressGenType_VREAD_OUTPUT:{
    // TODO: Basically a copy of MEM but with the different named signals, since MEM has slight differences in naming conventions.
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");
    
    if(!Empty(constantExpr)){
      builder->PushString("   config->output_start = %.*s;\n",UNPACK_SS(constantExpr));
    }

    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->output_per%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->output_incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->output_duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->output_iter%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->output_shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;

  case AddressGenType_VWRITE_INPUT:{
    // TODO: Basically a copy of VREAD_OUTPUT and MEM but with the different named signals, since MEM has slight differences in naming conventions.
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);
    for(Token t : constants){
      builder->PushString(",int %.*s",UNPACK_SS(t));
    }
    builder->PushString("){\n");

    if(!Empty(constantExpr)){
      builder->PushString("   config->input_start = %.*s;\n",UNPACK_SS(constantExpr));
    }
    
    for(int i = 0; i < loopSpecSymbolic.size; i++){
      AddressGenLoopSpecificatonSym l = loopSpecSymbolic[i]; 

      char loopIndex = (i == 0) ? ' ' : '1' + i;
      
      builder->PushString("   config->input_per%c = %.*s;\n",loopIndex,UNPACK_SS(l.periodExpression));
      builder->PushString("   config->input_incr%c = %.*s;\n",loopIndex,UNPACK_SS(l.incrementExpression));

      if(!Empty(l.dutyExpression)){
        builder->PushString("   config->input_duty = %.*s;\n",UNPACK_SS(l.dutyExpression));
      }
      builder->PushString("   config->input_iter%c = %.*s;\n",loopIndex,UNPACK_SS(l.iterationExpression));
      builder->PushString("   config->input_shift%c = %.*s;\n",loopIndex,UNPACK_SS(l.shiftExpression));
    }
  } break;

  case AddressGenType_VWRITE_STORE:{
    builder->PushString("static void ");
    PushFunctionNameAndFirstArgument(builder,"Configure",name,typeStructName);

    for(Token t : constants){
      if(CompareString(t,"ext")){
        builder->PushString(",iptr %.*s",UNPACK_SS(t));
      } else {
        builder->PushString(",int %.*s",UNPACK_SS(t));
      }
    }
    builder->PushString("){\n");

    Assert(!Empty(externalName)); // TODO: Probably a parser error or something. Weird to see it here
    builder->PushString("   config->ext_addr = %.*s;\n",UNPACK_SS(externalName));
    
    AddressGenLoopSpecificatonSym in = internalSpecSym[0]; 
    builder->PushString("   config->write_per = %.*s;\n",UNPACK_SS(in.periodExpression));
    builder->PushString("   config->write_incr = %.*s;\n",UNPACK_SS(in.incrementExpression));
    builder->PushString("   config->write_duty = %.*s;\n",UNPACK_SS(in.dutyExpression));
    builder->PushString("   config->write_iter = %.*s;\n",UNPACK_SS(in.iterationExpression));
    builder->PushString("   config->write_shift = %.*s;\n",UNPACK_SS(in.shiftExpression));

    builder->PushString("   config->write_enabled = 1;\n");
    builder->PushString("   config->pingPong = 1;\n");

    AddressGenLoopSpecificatonSym ext = externalSpecSym[0];

    builder->PushString("   config->write_length = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.periodExpression));
    builder->PushString("   config->write_amount_minus_one = (%.*s) - 1;\n",UNPACK_SS(ext.iterationExpression));
    builder->PushString("   config->write_addr_shift = (%.*s) * sizeof(float);\n",UNPACK_SS(ext.shiftWithoutRemovingIncrement));
  } break;

  }
  builder->PushString("}\n");
  
  return EndString(out,builder);
}

AddressGen CompileAddressGenDef(AddressGenDef def,Arena* out){
  TEMP_REGION(temp,out);
  
  TrieSet<Token>* constants = PushTrieSet<Token>(temp);
  TrieSet<Token>* loopVariables = PushTrieSet<Token>(temp);
  
  for(Token t : def.inputs){
    constants->Insert(t);
  }
  
  for(AddressGenForDef d : def.loops){
    loopVariables->Insert(d.loopVariable);
  }

  Array<AddressGenFor> loops = PushArray<AddressGenFor>(out,def.loops.size); 
  for(int i = 0; i < def.loops.size; i++){
    AddressGenForDef d = def.loops[i];

    AddressGenFor result = {};
    result.loopVariable = d.loopVariable;
    if(d.start.size > 0 && IsNum(d.start[0])){
      result.start.isId = false;
      result.start.number = ParseInt(d.start);
    } else {
      result.start.isId = true;
      result.start.identifier = d.start;
    }

    if(d.end.size > 0 && IsNum(d.end[0])){
      result.end.isId = false;
      result.end.number = ParseInt(d.end);
    } else {
      result.end.isId = true;
      result.end.identifier = d.end;
    }

    loops[i] = result;
  }

  auto GetRepr = [](IdOrNumber n,Arena* out){
    if(n.isId){
      return PushString(out,"%.*s",UNPACK_SS(n.identifier));
    } else {
      return PushString(out,"%d",n.number);
    }
  }; 
  
  auto GetLoopSizeRepr = [GetRepr](AddressGenFor f,Arena* out){
    TEMP_REGION(temp,out);
    auto builder = StartString(temp);

    builder->PushString("(");
    builder->PushString(GetRepr(f.end,temp));
    builder->PushString(" - ");
    builder->PushString(GetRepr(f.start,temp));
    builder->PushString(")");

    return EndString(out,builder);
  };

  Reverse(loops);

  auto GenerateLoopExpressionPairSymbolic = [GetRepr,GetLoopSizeRepr]
    (AddressGenDef def,Array<AddressGenFor> loops,SymbolicExpression* expr,bool ext,Arena* out) -> Array<AddressGenLoopSpecificatonSym>{
    TEMP_REGION(temp,out);

    int loopSize = (loops.size + 1) / 2;
    Array<AddressGenLoopSpecificatonSym> result = PushArray<AddressGenLoopSpecificatonSym>(out,loopSize);

    // TODO: Need to normalize and push the entire expression into a division.
    //       So we can then always extract a duty
    SymbolicExpression* duty = nullptr;
    if(expr->type == SymbolicExpressionType_OP){
      duty = expr->right;
      expr = expr->left;
    }
    
    for(int i = 0; i < loopSize; i++){
      AddressGenFor l0 = loops[i*2];

      AddressGenLoopSpecificatonSym& res = result[i];
      res = {};

      String loopSizeRepr = GetLoopSizeRepr(l0,out);
      
      SymbolicExpression* derived = Derivate(expr,l0.loopVariable,temp);
      SymbolicExpression* firstDerived = Normalize(derived,temp);
      SymbolicExpression* periodSym = ParseSymbolicExpression(loopSizeRepr,temp);
        
      res.periodExpression = loopSizeRepr;
      res.incrementExpression = PushRepresentation(firstDerived,out);

      if(i == 0){
        if(duty){
          // For an expression where increment is something like 1/4.
          // That means that we want 4 * period with a duty of 1.
          // Basically have to reverse the division.

          SymbolicExpression* templateSym = ParseSymbolicExpression(STRING("(period) / duty"),temp);
          SymbolicExpression* dutyExpression = SymbolicReplace(templateSym,STRING("period"),periodSym,temp);

          dutyExpression = SymbolicReplace(dutyExpression,STRING("duty"),duty,temp);
          
          result[0].dutyExpression = PushRepresentation(dutyExpression,out);
        } else {
          result[0].dutyExpression = PushString(out,GetRepr(loops[0].end,temp)); // For now, do not care too much about duty. Use a full duty
        }
      }
      
      String firstEnd = GetRepr(l0.end,out);
      SymbolicExpression* firstEndSym = ParseSymbolicExpression(firstEnd,temp);

      res.shiftWithoutRemovingIncrement = STRING("0"); // By default
      if(i * 2 + 1 < loops.size){
        AddressGenFor l1 = loops[i*2 + 1];

        res.iterationExpression = GetLoopSizeRepr(l1,out);
        SymbolicExpression* derived = Normalize(Derivate(expr,l1.loopVariable,temp),temp);

        res.shiftWithoutRemovingIncrement = PushRepresentation(derived,out);
        
        // We handle shifts very easily. We just remove the effects of all the previous period increments and then apply the shift.
        // That way we just have to calculate the derivative in relation to the shift, instead of calculating the change from a period term to a iteration term.
        // We need to subtract 1 because the period increment is only applied (period - 1) times.
        SymbolicExpression* templateSym = ParseSymbolicExpression(STRING("-(firstIncrement * (firstEnd - 1)) + term"),temp);
        
        SymbolicExpression* replaced = SymbolicReplace(templateSym,STRING("firstIncrement"),firstDerived,temp);
        replaced = SymbolicReplace(replaced,STRING("firstEnd"),firstEndSym,temp);
        replaced = SymbolicReplace(replaced,STRING("term"),derived,temp);
        replaced = Normalize(replaced,temp);
        
        res.shiftExpression = PushRepresentation(replaced,out);
      } else {
        if(ext){
          res.iterationExpression = STRING("1");
        } else {
          res.iterationExpression = STRING("0");
        }
        res.shiftExpression = STRING("0");
      }
    }

    return result;
  };
  
  Array<AddressGenLoopSpecificatonSym> loopSpecSymbolic;
  Array<AddressGenLoopSpecificatonSym> internalSpecSym;
  Array<AddressGenLoopSpecificatonSym> externalSpecSym;

  if(def.symbolic){
    loopSpecSymbolic = GenerateLoopExpressionPairSymbolic(def,loops,def.symbolic,false,out);
  }
  if(def.symbolicInternal){
    internalSpecSym = GenerateLoopExpressionPairSymbolic(def,loops,def.symbolicInternal,true,out);
  }
  if(def.symbolicExternal){
    externalSpecSym = GenerateLoopExpressionPairSymbolic(def,loops,def.symbolicExternal,true,out);
  }

  // Calculate start by replacing every loop variable with their initial value.
  String constantExpr = {};
  if(def.symbolic){
    SymbolicExpression* current = def.symbolic;
    if(current->type == SymbolicExpressionType_OP){
      current = current->left;
    }
    
    // TODO: Should be the start value, but since we only using zero, for now simplify.
    //       A lot of code needs to be moved first and we probably need more tests before we start tackling this issues anyway. Not a priority for now.
    SymbolicExpression* toReplaceWith = ParseSymbolicExpression(STRING("0"),temp);
    for(AddressGenFor loop : loops){
      current = SymbolicReplace(current,loop.loopVariable,toReplaceWith,temp);
    }

    current = Normalize(current,temp);
    constantExpr = PushRepresentation(current,out);
  }

  AddressGen res = {};
  res.loopSpecSymbolic = loopSpecSymbolic;
  res.externalSpecSym = externalSpecSym;
  res.internalSpecSym = internalSpecSym;
  res.constantExpr = constantExpr;
  res.name = def.name;
  res.externalName = def.externalName;
  res.type = def.type;
  res.constants = PushArrayFromSet(out,constants);

  res.loopRepr = PushArray<String>(out,loops.size);
  for(int i = 0; i < loops.size; i++){
    res.loopRepr[i] = GetLoopSizeRepr(loops[i],out);
  }
  
  return res;
}
