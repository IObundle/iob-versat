#include "versatSpecificationParser.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

#include "memory.hpp"
#include "type.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "parser.hpp"
#include "debug.hpp"
#include "merge.hpp"
#include "accelerator.hpp"

// TODO: Start actually using and storing Tokens, which maintaing location info.
//       That way we can report semantic errors with rich location info.

void ReportError(Tokenizer* tok,String faultyToken,const char* error){
  STACK_ARENA(temp,Kilobyte(1));

  String loc = tok->GetRichLocationError(faultyToken,&temp);

  printf("%s\n",error);
  printf("%.*s\n",UNPACK_SS(loc));
}

bool _ExpectError(Tokenizer* tok,const char* str){
  Token got = tok->NextToken();
  Token expected = STRING(str);
  if(!CompareString(got,expected)){
    STACK_ARENA(temp,Kilobyte(4));
    Byte* mark = MarkArena(&temp);
    PushString(&temp,"Parser Error.\n Expected to find:");
    PushEscapedToken(expected,&temp);
    PushString(&temp,"\n");
    PushString(&temp,"  Got:");
    PushEscapedToken(got,&temp);
    PushString(&temp,"\n");
    String text = PointArena(&temp,mark);
    ReportError(tok,got,StaticFormat("%*s",UNPACK_SS(text))); \
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
  
Optional<int> ParseNumber(Tokenizer* tok){
  // TODO: We only handle integers, for now.
  String number = tok->NextToken();

  bool negate = false;
  if(CompareString(number,"-")){
    negate = true;
    number = tok->NextToken();
  }

  for(int i = 0; i < number.size; i++){
    if(!(number[i] >= '0' && number[i] <= '9')){
      STACK_ARENA(temp,Kilobyte(1)); \
      
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

Optional<Range<int>> ParseRange(Tokenizer* tok){
  Range<int> res = {};

  Optional<int> n1 = ParseNumber(tok);
  PROPAGATE(n1);

  if(!tok->IfNextToken("..")){
    res.start = n1.value();
    res.end = n1.value();
    return res;
  }

  Optional<int> n2 = ParseNumber(tok);
  PROPAGATE(n2);

  res.start = n1.value();
  res.end = n2.value();
  return res;
}

Optional<Var> ParseVar(Tokenizer* tok){
  Var var = {};

  Token name = tok->NextToken();

  Token peek = tok->PeekToken();
  if(CompareToken(peek,"[")){
    tok->AdvancePeek(peek);

    Optional<Range<int>> range = ParseRange(tok);
    PROPAGATE(range);

    var.isArrayAccess = true;
    var.index = range.value();

    EXPECT(tok,"]");
  }
  
  int delayStart = 0;
  int delayEnd = 0;
  if(CompareToken(peek,"{")){
    tok->AdvancePeek(peek);

    Optional<Range<int>> rangeOpt = ParseRange(tok);
    PROPAGATE(rangeOpt);
    delayStart = rangeOpt.value().start;
    delayEnd = rangeOpt.value().end;

    EXPECT(tok,"}");
    peek = tok->PeekToken();
  }

  int portStart = 0;
  int portEnd = 0;
  if(CompareToken(peek,":")){
    tok->AdvancePeek(peek);

    Optional<Range<int>> rangeOpt = ParseRange(tok);
    PROPAGATE(rangeOpt);
    portStart = rangeOpt.value().start;
    portEnd = rangeOpt.value().end;
  }

  // TODO: This should not be an assert.
  //       Either a semantic error or we make it so that all the ranges should always go up.
  Assert(delayEnd >= delayStart);
  Assert(portEnd >= portStart);

  var.name = name;
  var.extra.delay.start = delayStart;
  var.extra.delay.end = delayEnd;
  var.extra.port.start = portStart;
  var.extra.port.end = portEnd;

  return var;
}

// A group can also be a single var. It does not necessary mean that it must be of the form {...}
Optional<VarGroup> ParseGroup(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  if(CompareString(peek,"{")){
    tok->AdvancePeek(peek);

    Byte* mark = MarkArena(out);
    while(!tok->Done()){
      Var* var = PushStruct<Var>(out);
      Optional<Var> optVar = ParseVar(tok);
      PROPAGATE(optVar);

      *var = optVar.value();

      Token sepOrEnd = tok->NextToken();
      if(CompareString(sepOrEnd,",")){
        continue;
      } else if(CompareString(sepOrEnd,"}")){
        break;
      } else {
        ReportError(tok,sepOrEnd,"Unexpected token"); // TODO: Implement a Expect for multiple expected tokens.
        return {};
      }
    }
    VarGroup res = PointArray<Var>(out,mark);
    return res;
  } else {
    Optional<Var> optVar = ParseVar(tok);
    PROPAGATE(optVar);

    Var var = optVar.value();
    VarGroup res = PushArray<Var>(out,1);
    res[0] = var;
    return res;
  }
}

Expression* ParseAtomS(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  bool negate = false;
  const char* negateType = nullptr;
  if(CompareToken(peek,"~")){
    negate = true;
    negateType = "~";
    tok->AdvancePeek(peek);
  }
  if(CompareToken(peek,"-")){
    negate = true;
    negateType = "-";
    tok->AdvancePeek(peek);
  }

  Expression* expr = PushStruct<Expression>(out);

  peek = tok->PeekToken();
  if(peek[0] >= '0' && peek[0] <= '9'){  // TODO: Need to call ParseNumber
    tok->AdvancePeek(peek);

    int digit = ParseInt(peek);

    expr->type = Expression::LITERAL;
    expr->val = MakeValue(digit);
  } else {
    expr->type = Expression::IDENTIFIER;
    void* mark = tok->Mark();
    ParseVar(tok);
    expr->id = tok->Point(mark);
  }

  if(negate){
    Expression* negateExpr = PushStruct<Expression>(out);
    negateExpr->op = negateType;
    negateExpr->type = Expression::OPERATION;
    negateExpr->expressions = PushArray<Expression*>(out,1);
    negateExpr->expressions[0] = expr;

    expr = negateExpr;
  }

  return expr;
}

Expression* SpecParseExpression(Tokenizer* tok,Arena* out);
Expression* ParseTerm(Tokenizer* tok,Arena* out){
  Token peek = tok->PeekToken();

  Expression* expr = nullptr;
  if(CompareString(peek,"(")){
    tok->AdvancePeek(peek);
    expr = SpecParseExpression(tok,out);
    tok->AssertNextToken(")");
  } else {
    expr = ParseAtomS(tok,out);
  }

  return expr;
}

Expression* SpecParseExpression(Tokenizer* tok,Arena* out){
  Expression* expr = ParseOperationType(tok,{{"+","-"},{"&","|","^"},{">><",">>","><<","<<"}},ParseTerm,out);

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
  Byte* mark = MarkArena(out);
  while(names->Exists(uniqueName)){
    PopMark(out,mark);
    uniqueName = PushString(out,"%.*s_%d",UNPACK_SS(name),counter++);
  }

  names->Insert(uniqueName);

  return uniqueName;
}

String GetActualArrayName(String baseName,int index,Arena* out){
  return PushString(out,"%.*s_%d",UNPACK_SS(baseName),index);
}

// Right now, not using the full portion of PortExpression because technically we would need to instantiate multiple things. Not sure if there is a need, when a case occurs then make the change then
PortExpression InstantiateExpression(Versat* versat,Expression* root,Accelerator* circuit,InstanceTable* table,InstanceName* names){
  PortExpression res = {};

  switch(root->type){
    // Just to remove warnings. TODO: Change expression so that multiple locations have their own expression struct, instead of reusing the same one.
  case Expression::UNDEFINED: break;
  case Expression::COMMAND: break;
  case Expression::ARRAY_ACCESS: LOCATION(); break;
  case Expression::MEMBER_ACCESS: break;
  case Expression::LITERAL:{
    int number = root->val.number;

    String toSearch = PushString(&versat->temp,"N%d",number);

    FUInstance** found = table->Get(toSearch);

    if(!found){
      String permName = PushString(&versat->permanent,"%.*s",UNPACK_SS(toSearch));
      String uniqueName = GetUniqueName(permName,&versat->permanent,names);

      FUInstance* digitInst = (FUInstance*) CreateFUInstance(circuit,GetTypeByName(versat,STRING("Literal")),uniqueName);
      digitInst->literal = number;
      table->Insert(permName,digitInst);
      res.inst = digitInst;
    } else {
      res.inst = *found;
    }
  }break;
  case Expression::IDENTIFIER:{
    Tokenizer tok(root->id,"[]{}:",{".."});

    // TODO: We are leaky parsing with evaluation.
    STACK_ARENA(temp,Kilobyte(1));
    Optional<Var> optVar = ParseVar(&tok);
    Assert(optVar.has_value()); // TODO: Handle this

    Var var = optVar.value();
    
    String name = var.name;

    if(var.isArrayAccess){
      name = GetActualArrayName(var.name,var.index.bottom,&temp);
    }
    
    FUInstance* inst = table->GetOrFail(name);

    res.inst = inst;
    res.extra = var.extra;
  } break;
  case Expression::OPERATION:{
    PortExpression expr0 = InstantiateExpression(versat,root->expressions[0],circuit,table,names);

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

      String permName = GetUniqueName(typeName,&versat->permanent,names);
      FUInstance* inst = CreateFUInstance(circuit,GetTypeByName(versat,typeName),permName);

      ConnectUnits(expr0.inst,expr0.extra.port.start,inst,0,expr0.extra.delay.start);

      res.inst = inst;
      res.extra.port.end  = res.extra.port.start  = 0;
      res.extra.delay.end = res.extra.delay.start = 0;

      return res;
    } else {
      Assert(root->expressions.size == 2);
    }

    PortExpression expr1 = InstantiateExpression(versat,root->expressions[1],circuit,table,names);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr1.extra.port.start == expr1.extra.port.end);
    Assert(expr1.extra.delay.start == expr1.extra.delay.end);

    String op = STRING(root->op);
    const char* typeName;
    if(CompareToken(op,"&")){
      typeName = "AND";
    } else if(CompareToken(op,"|")){
      typeName = "OR";
    } else if(CompareToken(op,"^")){
      typeName = "XOR";
    } else if(CompareToken(op,">><")){
      typeName = "RHR";
    } else if(CompareToken(op,">>")){
      typeName = "SHR";
    } else if(CompareToken(op,"><<")){
      typeName = "RHL";
    } else if(CompareToken(op,"<<")){
      typeName = "SHL";
    } else if(CompareToken(op,"+")){
      typeName = "ADD";
    } else if(CompareToken(op,"-")){
      typeName = "SUB";
    } else {
      printf("%.*s\n",UNPACK_SS(op));
      Assert(false);
    }

    String typeStr = STRING(typeName);
    FUDeclaration* type = GetTypeByName(versat,typeStr);
    String uniqueName = GetUniqueName(type->name,&versat->permanent,names);

    FUInstance* inst = CreateFUInstance(circuit,type,uniqueName);

    ConnectUnits(expr0.inst,expr0.extra.port.start,inst,0,expr0.extra.delay.start);
    ConnectUnits(expr1.inst,expr1.extra.port.start,inst,1,expr0.extra.delay.start);

    res.inst = inst;
    res.extra.port.end  = res.extra.port.start  = 0;
    res.extra.delay.end = res.extra.delay.start = 0;
  } break;
  default: {
    Assert(false);
  } break;
  }

  Assert(res.inst);
  return res;
}

FUInstance* ParseExpression(Versat* versat,Accelerator* circuit,Tokenizer* tok,InstanceTable* table,InstanceName* names){
  Arena* temp = &versat->temp;
  Expression* expr = SpecParseExpression(tok,temp);
  PortExpression inst = InstantiateExpression(versat,expr,circuit,table,names);

  return inst.inst;
}

FUInstance* ParseInstanceDeclaration(Versat* versat,Tokenizer* tok,Accelerator* circuit,InstanceTable* table,InstanceName* names){
  Token type = tok->NextToken();

  Token possibleParameters = tok->PeekToken();
  Token fullParameters = STRING("");
  if(CompareString(possibleParameters,"#")){
    fullParameters = ExtendToken(possibleParameters,tok->PeekIncludingDelimiterExpression({"("},{")"},0));
    tok->AdvancePeek(fullParameters);
  }
  
  String name = PushString(&versat->permanent,tok->NextToken());
  String uniqueName = GetUniqueName(name,&versat->permanent,names);

  FUDeclaration* FUType = GetTypeByName(versat,type);
  FUInstance* inst = CreateFUInstance(circuit,FUType,uniqueName);

  table->Insert(name,inst);
  inst->parameters = PushString(&versat->permanent,fullParameters);

  Token peek = tok->PeekToken();

  return inst;
}

Optional<VarDeclaration> ParseDeclaration(Tokenizer* tok){
  String name = tok->NextToken();
  
  if(!CheckValidName(name)){
    ReportError(tok,name,StaticFormat("'%.*s' is not a valid name",UNPACK_SS(name)));
    return {};
  }

  VarDeclaration res = {};
  res.name = name;

  String peek = tok->PeekToken();
  if(CompareString(peek,"[")){
    tok->AdvancePeek(peek);

    Optional<int> number = ParseNumber(tok);
    PROPAGATE(number);
    int arraySize = number.value();

    EXPECT(tok,"}");

    res.arraySize = arraySize;
    res.isArray = true;
  }
  
  return res;
}

Optional<ConnectionDef> ParseConnection(Tokenizer* tok,Arena* out){
  ConnectionDef def = {};

  Optional<VarGroup> optOutPortion = ParseGroup(tok,out);
  PROPAGATE(optOutPortion);

  def.output = optOutPortion.value();

  Token type = tok->NextToken();
  if(CompareString(type,"=")){
    def.type = ConnectionDef::EQUALITY;
  } else if(CompareString(type,"^=")){
    // TODO
  } else if(CompareString(type,"->")){
    def.type = ConnectionDef::CONNECTION;
  } else {
    ReportError(tok,type,StaticFormat("Did not expect %.*s",UNPACK_SS(type)));
    return {};
  }
  
  if(def.type == ConnectionDef::EQUALITY){
    def.expression = SpecParseExpression(tok,out);
  } else if(def.type == ConnectionDef::CONNECTION){
    Optional<VarGroup> optInPortion = ParseGroup(tok,out);
    PROPAGATE(optOutPortion);

    def.input = optInPortion.value();
  }

  EXPECT(tok,";");
  
  return def;
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

Optional<Array<VarDeclaration>> ParseVarDeclarationGroup(Tokenizer* tok,Arena* out){
  DynamicArray<VarDeclaration> array = StartArray<VarDeclaration>(out);

  EXPECT(tok,"(");

  while(!tok->Done()){
    String peek = tok->PeekToken();

    if(CompareString(peek,")")){
      break;
    }

    Optional<VarDeclaration> var = ParseDeclaration(tok);
    PROPAGATE(var);

    *array.PushElem() = var.value();
    
    tok->IfNextToken(",");
  }
  
  EXPECT(tok,")");

  return EndArray(array);
}

Optional<InstanceDeclaration> ParseInstanceDeclaration(Tokenizer* tok,Arena* out){
  InstanceDeclaration res = {};

  String potentialModifier = tok->PeekToken();

  if(CompareString(potentialModifier,"static")){
    tok->AdvancePeek(potentialModifier);
    res.modifier = InstanceDeclaration::STATIC;
  } else if(CompareString(potentialModifier,"share")){
    tok->AdvancePeek(potentialModifier);
    res.modifier = InstanceDeclaration::SHARE_CONFIG;
    
    EXPECT(tok,"config"); // Only choice that is enabled, for now

    res.typeName = tok->NextToken();
    if(!IsValidIdentifier(res.typeName)){ // NOTE: Not sure if types should share same restrictions on names
      ReportError(tok,res.typeName,StaticFormat("type name '%.*s' is not a valid name",UNPACK_SS(res.typeName)));
      return {};
    }

    EXPECT(tok,"{");

    DynamicArray<String> array = StartArray<String>(out);
    
    while(!tok->Done()){
      String peek = tok->PeekToken();

      if(CompareString(peek,"}")){
        break;
      }

      String identifier = tok->NextToken();
      if(!IsValidIdentifier(identifier)){
        ReportError(tok,identifier,StaticFormat("name '%.*s' is not a valid name",UNPACK_SS(identifier)));
        // Keep going
      }

      *array.PushElem() = identifier;
      
      EXPECT(tok,";");
    }
    res.declarationNames = EndArray(array);
    
    EXPECT(tok,"}");
    return res;
  }

  res.typeName = tok->NextToken();
  if(!IsValidIdentifier(res.typeName)){ // NOTE: Not sure if types should share same restrictions on names
    ReportError(tok,res.typeName,StaticFormat("'%.*s' is not a valid type name",UNPACK_SS(res.typeName)));
    return {};
  }

  String possibleParameters = tok->PeekToken();
  if(CompareString(possibleParameters,"#")){
    String fullParameters = ExtendToken(possibleParameters,tok->PeekIncludingDelimiterExpression({"("},{")"},0));
    tok->AdvancePeek(fullParameters);
    res.parameters = fullParameters;
  }
  
  String identifier = tok->NextToken();
  if(!IsValidIdentifier(identifier)){
    ReportError(tok,identifier,StaticFormat("'%.*s' is not a valid instance name",UNPACK_SS(identifier)));
    return {};
  }
  
  res.declarationNames = PushArray<String>(out,1);
  res.declarationNames[0] = identifier;

  EXPECT(tok,";");

  return res;
}

// Any returned String points to tokenizer content.
// As long as tokenizer is valid, strings returned by this function are also valid.
Optional<ModuleDef> ParseModuleDef(Tokenizer* tok,Arena* out,Arena* temp){
  ModuleDef def = {};

  tok->AssertNextToken("module");

  def.name = tok->NextToken();
  if(!IsValidIdentifier(def.name)){
    ReportError(tok,def.name,StaticFormat("module name '%.*s' is not a valid name",UNPACK_SS(def.name)));
    return {}; // TODO: We could try to keep going and find more errors
  }
  
  Optional<Array<VarDeclaration>> optVar = ParseVarDeclarationGroup(tok,out);
  PROPAGATE(optVar);

  def.inputs = optVar.value();  

  ArenaList<InstanceDeclaration>* decls = PushArenaList<InstanceDeclaration>(temp);
  EXPECT(tok,"{");
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"#")){
      break;
    }

    Optional<InstanceDeclaration> optDecl = ParseInstanceDeclaration(tok,out);
    PROPAGATE(optDecl); // TODO: We could try to keep going and find more errors

    *PushListElement(decls) = optDecl.value();
  }
  def.declarations = PushArrayFromList(out,decls);

  EXPECT(tok,"#");
  
  ArenaList<ConnectionDef>* cons = PushArenaList<ConnectionDef>(temp);
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"}")){
      break;
    }

    Optional<ConnectionDef> optCon = ParseConnection(tok,out);
    PROPAGATE(optCon); // TODO: We could try to keep going and find more errors
    
    *PushListElement(cons) = optCon.value();
  }
  def.connections = PushArrayFromList(out,cons);
  
  return def;
}

FUDeclaration* ParseIterative(Versat* versat,Tokenizer* tok){
  tok->AssertNextToken("iterative");

  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  InstanceTable* table = PushHashmap<String,FUInstance*>(temp,1000);
  Set<String>* names = PushSet<String>(temp,1000);

  String moduleName = tok->NextToken();
  String name = PushString(&versat->permanent,moduleName);

  Accelerator* iterative = CreateAccelerator(versat,name);

  tok->AssertNextToken("(");
  // Arguments
  int insertedInputs = 0;
  while(1){
    Token argument = tok->PeekToken();

    if(CompareString(argument,")")){
      break;
    }
    tok->AdvancePeek(argument);

    Token peek = tok->PeekToken();
    if(CompareString(peek,",")){
      tok->AdvancePeek(peek);
    }

    String name = PushString(&versat->permanent,argument);

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

    FUDeclaration* type = GetTypeByName(versat,instanceTypeName);
    String name = PushString(&versat->permanent,instanceName);

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
      tok->AdvancePeek(peek);
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
  
  return RegisterIterativeUnit(versat,iterative,unit,latency,name);
}

TypeAndInstance ParseTypeAndInstance(Tokenizer* tok){
  String typeName = tok->NextToken();

  String instanceName = {};
  if(tok->IfNextToken(":")){
    instanceName = tok->NextToken();
  }

  TypeAndInstance res = {};
  res.typeName = typeName;
  res.instanceName = instanceName;
  
  return res;
}

void ParseMerge(Versat* versat,Tokenizer* tok){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);
  
  tok->AssertNextToken("merge");
  String mergeNameToken = tok->NextToken();
  String mergeName = PushString(&versat->permanent,mergeNameToken);
  tok->AssertNextToken("=");

  PRINT_STRING(mergeNameToken);

  ArenaList<TypeAndInstance>* list = PushArenaList<TypeAndInstance>(temp);
  *PushListElement(list) = ParseTypeAndInstance(tok);
  
  // TODO: At the very least put a check to see if it is a valid type name.
  //       Perform type verification afterwards
  
  while(!tok->Done()){
    Token next = tok->NextToken();

    if(CompareString(next,"{")){
      // TODO: Need to finish parsing this part
      while(!tok->Done()){
        String first = tok->NextToken();
        tok->AssertNextToken("-");
        String second = tok->NextToken();
        tok->AssertNextToken(";");

        if(tok->IfNextToken("}")){
          break;
        }
      }
      
      break;
    } else if(CompareString(next,"|")){
      *PushListElement(list) = ParseTypeAndInstance(tok);
    } else if(CompareString(next,";")){
      break;
    } else {
      // User error.
      Assert(false);
    }
  }

  int amount = Size(list);
  Array<FUDeclaration*> declarations = PushArray<FUDeclaration*>(temp,amount);
  int index = 0;
  FOREACH_LIST_INDEXED(ListedStruct<TypeAndInstance>*,iter,list->head,index){
    FUDeclaration* type = GetTypeByName(versat,iter->elem.typeName);
    Assert(type); // Should report error to user, but technically GetTypeByName already does that, for now
    declarations[index] = type;
  }

  Merge(versat,declarations,mergeName);
}  

enum ConnectionType{
  ConnectionType_SINGLE,
  ConnectionType_PORT_RANGE,
  ConnectionType_ARRAY_RANGE,
  ConnectionType_DELAY_RANGE,
  ConnectionType_ERROR
};

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
  return {};
}

bool IsValidGroup(VarGroup group){
  // TODO: Wether we can match the group or not.
  //       It depends on wether the ranges line up or not. 
  for(Var& var : group){
    if(GetConnectionInfo(var).first == ConnectionType_ERROR){
      return false;
    }
  }

  return true;
}

int NumberOfConnections(VarGroup group){
  Assert(IsValidGroup(group));

  int count = 0;

  for(Var& var : group){
    count += GetConnectionInfo(var).second;
  }

  return count;
}

struct GroupIterator{
  VarGroup group;
  int groupIndex;
  int varIndex; // Either port, delay or array unit.
};

GroupIterator IterateGroup(VarGroup group){
  GroupIterator iter = {};
  iter.group = group;
  return iter;
}

bool HasNext(GroupIterator iter){
  if(iter.groupIndex >= iter.group.size){
    return false;
  }

  return true;
}

Var Next(GroupIterator& iter){
  Assert(HasNext(iter));

  Var var = iter.group[iter.groupIndex];
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
    default: NOT_IMPLEMENTED("IMPLEMENT AS NEEDED");
    }

    iter.varIndex += 1;
    if(iter.varIndex >= maxCount){
      iter.varIndex = 0;
      iter.groupIndex += 1;
    }
  }
  
  return res;
}

FUDeclaration* InstantiateModule(Versat* versat,ModuleDef def,Arena* perm,Arena* temp){
  Accelerator* circuit = CreateAccelerator(versat,def.name);

  InstanceTable* table = PushHashmap<String,FUInstance*>(temp,1000);
  InstanceName* names = PushSet<String>(temp,1000);

  // TODO: Need to handle and report errors.
  //       Need to report errors with rich location info
  int insertedInputs = 0;
  for(VarDeclaration& decl : def.inputs){
    if(decl.isArray){
      for(int i = 0; i < decl.arraySize; i++){
        String actualName = GetActualArrayName(decl.name,i,perm);
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
    FUDeclaration* type = GetTypeByName(versat,decl.typeName);
    
    switch(decl.modifier){
    case InstanceDeclaration::SHARE_CONFIG:{
      for(String& name : decl.declarationNames){
        FUInstance* inst = CreateFUInstance(circuit,type,name);
        inst->parameters = PushString(perm,decl.parameters);
        table->Insert(name,inst);
        ShareInstanceConfig(inst,shareIndex);
      }
      shareIndex += 1;
    } break;

    case InstanceDeclaration::NONE:
    case InstanceDeclaration::STATIC:
    {
      Assert(decl.declarationNames.size == 1);

      String name = decl.declarationNames[0];
      FUInstance* inst = CreateFUInstance(circuit,type,name);
      inst->parameters = PushString(perm,decl.parameters);
      table->Insert(name,inst);

      if(decl.modifier == InstanceDeclaration::STATIC){
        SetStatic(circuit,inst);
      }
    } break;
    }
  }

  FUInstance* outputInstance = nullptr;
  for(ConnectionDef& decl : def.connections){
    if(decl.type == ConnectionDef::EQUALITY){
      // Make sure that not a single equality value is named out.

      // TODO: Need to match ranges.
      Assert(decl.output.size == 1);
      Var outVar = decl.output[0];

      String name = PushString(&versat->permanent,outVar.name);

      PortExpression portExpression = InstantiateExpression(versat,decl.expression,circuit,table,names);
      FUInstance* inst = portExpression.inst;
      String uniqueName = GetUniqueName(name,&versat->permanent,names);
      inst->name = PushString(&versat->permanent,uniqueName);

      names->Insert(name);
      table->Insert(name,inst);
/*
      } else if(CompareToken(op,"^=")){
        if(CompareString(outVar.name,"out")){
          USER_ERROR("TODO: Should be a handled error");
        }

        String name = PushString(&versat->permanent,outVar.name);

        FUInstance* existing = table->GetOrFail(name);
        FUInstance* inst = ParseExpression(versat,circuit,tok,table,names);

        String uniqueName = GetUniqueName(name,&versat->permanent,names);
        FUInstance* op = CreateFUInstance(circuit,GetTypeByName(versat,STRING("XOR")),uniqueName);

        ConnectUnits(inst,0,op,0,0);     // Care, we are not taking account port ranges and delays and stuff like that.
        ConnectUnits(existing,0,op,1,0);
        names->Insert(name);
        table->Insert(name,op);

        tok->AssertNextToken(";");
*/
    } else if(decl.type == ConnectionDef::CONNECTION){
      // For now only allow one var on the input side
      if(decl.input.size != 1){
        GroupIterator iter = IterateGroup(decl.input);
        while(HasNext(iter)){
          Var var = Next(iter);

          PRINT_STRING(var.name);

          printf("%d\n",var.extra.port.start);
        }
      }

      Assert(NumberOfConnections(decl.output) == NumberOfConnections(decl.input));

      GroupIterator out = IterateGroup(decl.output);
      GroupIterator in  = IterateGroup(decl.input);

      while(HasNext(out) && HasNext(in)){
        Var outVar = Next(out);
        Var inVar = Next(in);
        
        Assert(inVar.extra.delay.high == 0); // For now, inputs cannot have delay.

        FUInstance* outInstance = table->GetOrFail(outVar.name);
        FUInstance* inInstance = nullptr;

        if(CompareString(inVar.name,"out")){
          if(!outputInstance){
            outputInstance = (FUInstance*) CreateFUInstance(circuit,BasicDeclaration::output,STRING("out"));
          }

          inInstance = outputInstance;
        } else {
          inInstance = table->GetOrFail(inVar.name);
        }

        ConnectUnits(outInstance,outVar.extra.port.low,inInstance,inVar.extra.port.low,outVar.extra.delay.low);
      }

      Assert(HasNext(out) == HasNext(in));
    }
  }
  
  // Care to never put 'out' inside the table
  FUInstance** outInTable = table->Get(STRING("out"));
  Assert(!outInTable);

  return RegisterSubUnit(versat,circuit);
}

void ParseVersatSpecification(Versat* versat,String content){
  Tokenizer tokenizer = Tokenizer(content,"%=#[](){}+:;,*~-",{"->=","->",">><","><<",">>","<<","..","^="});
  Tokenizer* tok = &tokenizer;

  Arena* perm = &versat->permanent;
  Arena* temp = &versat->temp;

  BLOCK_REGION(temp);
  
  bool anyError = false;
  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"debug")){
      tok->AdvancePeek(peek);
      tok->AssertNextToken(";");
      debugFlag = true;
    } else if(CompareString(peek,"module")){
      Optional<ModuleDef> moduleDef = {};
      region(perm){
        moduleDef = ParseModuleDef(tok,temp,perm);
      }

      if(moduleDef.has_value()){
        ModuleDef def = moduleDef.value();
        InstantiateModule(versat,def,perm,temp);
      } else {
        anyError |= true;
      }
    } else if(CompareString(peek,"iterative")){
      // TODO: Change to separate parsing and instantiating
      ParseIterative(versat,tok);
    } else if(CompareString(peek,"merge")){
      // TODO: Change to separate parsing and instantiating
      ParseMerge(versat,tok);
    } else {
      // TODO: Report error, 
      tok->AdvancePeek(peek);
    }
  }

  if(anyError){
    printf("Error occurred\n");
  }
}

void ParseVersatSpecification(Versat* versat,const char* filepath){
  BLOCK_REGION(&versat->temp);

  String content = PushFile(&versat->temp,filepath);

  if(content.size < 0){
    printf("Failed to open file, filepath: %s\n",filepath);
    DEBUG_BREAK();
  }

  ParseVersatSpecification(versat,content);
}





