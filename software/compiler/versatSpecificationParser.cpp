#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

#include "versat.hpp"
#include "parser.hpp"
#include "debug.hpp"
#include "debugGUI.hpp"
#include "graph.hpp"
#include "merge.hpp"

typedef Hashmap<String,FUInstance*> InstanceTable;
typedef Hashmap<String,int> InstanceName;

struct Name{
  char name[256];
  FUDeclaration* type;
};

struct ConnectionExtra{
  int portStart;
  int portEnd;
  int delayStart;
  int delayEnd;
};

struct Var{
  String name;
  ConnectionExtra extra;
};

int NumberOfConnections(ConnectionExtra e){
  int connections = e.portEnd - e.portStart + 1;

  return connections;
}
  
int NumberOfConnections(Array<Var> var){
  int connections = 0;
  for(Var& v : var){
    connections += NumberOfConnections(v.extra);
  }
  return connections;
}
  
static Name identifiers[256];
static int identifiersIndex;

int GetIndex(char* name){
  for(int i = 0; i < identifiersIndex; i++){
    if(strcmp(identifiers[i].name,name) == 0){
      return i;
    }
  }

  Assert(0);
  return 0;
}

Var ParseVar(Tokenizer* tok){
  Token name = tok->NextToken();

  Token peek = tok->PeekToken();

  int delayStart = 0;
  int delayEnd = 0;
  if(CompareToken(peek,"[")){
    tok->AdvancePeek(peek);

    Token delayToken = tok->NextToken();

    bool negate = false;
    if(CompareString(delayToken,"-")){
      negate = true;
      delayToken = tok->NextToken();
    }
     
    delayStart = ParseInt(delayToken);

    if(negate) delayStart = -delayStart;
     
    peek = tok->PeekToken();
    if(CompareToken(peek,"..")){
      tok->AdvancePeek(peek);
      Token delayEndToken = tok->NextToken();

      delayEnd = ParseInt(delayEndToken);
    } else {
      delayEnd = delayStart;
    }

    tok->AssertNextToken("]");
    peek = tok->PeekToken();
  }

  int portStart = 0;
  int portEnd = 0;
  if(CompareToken(peek,":")){
    tok->AdvancePeek(peek);
    Token portToken = tok->NextToken();
    portStart = ParseInt(portToken);

    peek = tok->PeekToken();
    if(CompareToken(peek,"..")){
      tok->AdvancePeek(peek);
      portToken = tok->NextToken();

      portEnd = ParseInt(portToken);
    } else {
      portEnd = portStart;
    }
  }

  Assert(delayEnd >= delayStart);
  Assert(portEnd >= portStart);

  Var var = {};

  var.name = name;
  var.extra.delayStart = delayStart;
  var.extra.delayEnd = delayEnd;
  var.extra.portStart = portStart;
  var.extra.portEnd = portEnd;

  return var;
}

Array<Var> ParseGroup(Tokenizer* tok,Arena* arena){
  Token peek = tok->PeekToken();

  if(CompareString(peek,"{")){
    tok->AdvancePeek(peek);

    Byte* mark = MarkArena(arena);
    while(!tok->Done()){
      Var* var = PushStruct<Var>(arena);
      *var = ParseVar(tok);

      Token sepOrEnd = tok->NextToken();

      if(CompareString(sepOrEnd,",")){
        continue;
      } else if(CompareString(sepOrEnd,"}")){
        break;
      } else {
        // TODO: Need to start paying more attention to errors
        Assert("Error\n");
      }
    }
    Array<Var> res = PointArray<Var>(arena,mark);
    return res;
  } else {
    Var var = ParseVar(tok);
    Array<Var> res = PushArray<Var>(arena,1);
    res[0] = var;
    return res;
  }
}

#if 0
PortInstance ParseTerm(Versat* versat,Accelerator* circuit,Tokenizer* tok,InstanceTable& table){
  Token peek = tok->PeekToken();
  int negate = 0;
  if(CompareToken(peek,"~")){
    tok->AdvancePeek(peek);
    negate = 1;
  }

  Var var = ParseVar(tok);

  FUInstance* inst = table.GetOrFail(var.name);
  PortInstance res = {};

  Assert(var.portStart == var.portEnd);

  if(negate){
    FUInstance* negation = CreateFUInstance(circuit,GetTypeByName(versat,STRING("NOT")),STRING("not"),true);

    ConnectUnits(inst,var.portStart,negation,0);

    res.inst = (FUInstance*) negation;
    res.port = 0;
  } else {
    res.inst = (FUInstance*) inst;
    res.port = var.portStart;
  }

  return res;
}
#endif

Expression* ParseAtomS(Tokenizer* tok,Arena* arena){
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

  Expression* expr = PushStruct<Expression>(arena);

  peek = tok->PeekToken();
  if(peek[0] >= '0' && peek[0] <= '9'){
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
    Expression* negateExpr = PushStruct<Expression>(arena);
    negateExpr->op = negateType;
    negateExpr->type = Expression::OPERATION;
    negateExpr->expressions = PushArray<Expression*>(arena,1);
    negateExpr->expressions[0] = expr;

    expr = negateExpr;
  }

  return expr;
}

Expression* SpecParseExpression(Tokenizer* tok,Arena* arena);
Expression* ParseTerm(Tokenizer* tok,Arena* arena){
  Token peek = tok->PeekToken();

  Expression* expr = nullptr;
  if(CompareString(peek,"(")){
    tok->AdvancePeek(peek);
    expr = SpecParseExpression(tok,arena);
    tok->AssertNextToken(")");
  } else {
    expr = ParseAtomS(tok,arena);
  }

  return expr;
}

Expression* SpecParseExpression(Tokenizer* tok,Arena* arena){
  Expression* expr = ParseOperationType(tok,{{"+","-"},{"&","|","^"},{">><",">>","><<","<<"}},ParseTerm,arena);

  return expr;
}

struct PortExpression{
  FUInstance* inst;
  ConnectionExtra extra;
};

int NumberOfConnections(Array<PortExpression> arr){
  int connections = 0;
  for(PortExpression& e : arr){
    connections += NumberOfConnections(e.extra);
  }
  return connections;
}

void ConnectUnit(PortExpression out,PortExpression in){
  FUInstance* inst1 = out.inst;
  FUInstance* inst2 = in.inst;
  ConnectionExtra outE = out.extra;
  ConnectionExtra inE = in.extra;

  int outRange = outE.portEnd - outE.portStart + 1;
  int delayRange = outE.delayEnd - outE.delayStart + 1;
  int inRange = inE.portEnd - inE.portStart + 1;

  bool matches = (delayRange == inRange || delayRange == 1) &&
                 (outRange == inRange   || outRange == 1);

  Assert(matches);

  int delayDelta = (delayRange == 1 ? 0 : 1);
  if(outRange == 1){
    for(int i = 0; i < inRange; i++){
      ConnectUnits(inst1,outE.portStart,inst2,inE.portStart + i,outE.delayStart + delayDelta * i);
    }
  } else {
    for(int i = 0; i < inRange; i++){
      ConnectUnits(inst1,outE.portStart + i,inst2,inE.portStart + i,outE.delayStart + delayDelta * i);
    }
  }
}

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
    int portStart = expr.extra.portStart + portIndex;
    
    ConnectUnits(inst,portStart,instIn,inE.portStart + i); // TODO: For now we are assuming no delay

    portIndex += 1;

    if(portIndex >= NumberOfConnections(expr.extra)){
      unitIndex += 1;
      portIndex = 0;
    }
  }
}

String GetUniqueName(String name,Arena* arena,InstanceName* names){
  int counter = 0;
  String uniqueName = name;
  Byte* mark = MarkArena(arena);
  while(names->Get(uniqueName) != nullptr){
    PopMark(arena,mark);
    uniqueName = PushString(arena,"%.*s_%d",UNPACK_SS(name),counter++);
  }

  names->Insert(uniqueName,0);

  return uniqueName;
}

// Right now, not using the full portion of PortExpression because technically we would need to instantiate multiple things. Not sure if there is a need, when a case occurs then make the change then
PortExpression InstantiateExpression(Versat* versat,Expression* root,Accelerator* circuit,InstanceTable* table,InstanceName* names){
  PortExpression res = {};

  switch(root->type){
    // Just to remove warnings. TODO: Change expression so that multiple locations have their own expression struct, instead of reusing the same one.
  case Expression::UNDEFINED: break;
  case Expression::COMMAND: break;
  case Expression::ARRAY_ACCESS: break;
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
    Tokenizer tok(root->id,"[]:",{".."});

    Var var = ParseVar(&tok);

    FUInstance* inst = table->GetOrFail(var.name);

    res.inst = inst;
    res.extra = var.extra;
  } break;
  case Expression::OPERATION:{
    PortExpression expr0 = InstantiateExpression(versat,root->expressions[0],circuit,table,names);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr0.extra.portStart == expr0.extra.portEnd);
    Assert(expr0.extra.delayStart == expr0.extra.delayEnd);

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

      ConnectUnits(expr0.inst,expr0.extra.portStart,inst,0,expr0.extra.delayStart);

      res.inst = inst;
      res.extra.portEnd  = res.extra.portStart  = 0;
      res.extra.delayEnd = res.extra.delayStart = 0;

      return res;
    } else {
      Assert(root->expressions.size == 2);
    }

    PortExpression expr1 = InstantiateExpression(versat,root->expressions[1],circuit,table,names);

    // Assuming right now very simple cases, no port range and no delay range
    Assert(expr1.extra.portStart == expr1.extra.portEnd);
    Assert(expr1.extra.delayStart == expr1.extra.delayEnd);

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

    ConnectUnits(expr0.inst,expr0.extra.portStart,inst,0,expr0.extra.delayStart);
    ConnectUnits(expr1.inst,expr1.extra.portStart,inst,1,expr0.extra.delayStart);

    res.inst = inst;
    res.extra.portEnd  = res.extra.portStart  = 0;
    res.extra.delayEnd = res.extra.delayStart = 0;
  } break;
  default: {
    Assert(false);
  } break;
  }

  Assert(res.inst);
  return res;
}

FUInstance* ParseExpression(Versat* versat,Accelerator* circuit,Tokenizer* tok,InstanceTable* table,InstanceName* names){
  Arena* arena = &versat->temp;
  Expression* expr = SpecParseExpression(tok,arena);
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

  if(CompareString(peek,"(")){
    tok->AdvancePeek(peek);

    Token list = tok->NextFindUntil(")");
    int arguments = 1 + CountSubstring(list,STRING(","));
    Assert(arguments <= FUType->configInfo.configs.size);

    Tokenizer insideList(list,",",{});

    for(int i = 0; i < arguments; i++){
      Token arg = insideList.NextToken();

      /* inst->config[i] = */ ParseInt(arg); // TODO

      if(i != arguments - 1){
        insideList.AssertNextToken(",");
      }
    }
    SetDefaultConfiguration(inst);
    Assert(insideList.Done());

    tok->AssertNextToken(")");
    peek = tok->PeekToken();
  }

  if(CompareString(peek,"{")){
    tok->AdvancePeek(peek);

    Token list = tok->NextFindUntil("}");
    int arguments = 1 + CountSubstring(list,STRING(","));
    Assert(arguments <= (1 << FUType->memoryMapBits));

    Tokenizer insideList(list,",",{});

    inst->memMapped = PushArray<int>(&versat->permanent,1 << FUType->memoryMapBits).data;
    //inst->savedMemory = true;
    for(int i = 0; i < arguments; i++){
      Token arg =  insideList.NextToken();

      inst->memMapped[i] = ParseInt(arg);

      if(i != arguments - 1){
        insideList.AssertNextToken(",");
      }
    }
    Assert(insideList.Done());

    tok->AssertNextToken("}");
    peek = tok->PeekToken();
  }

  return inst;
}

FUDeclaration* ParseModule(Versat* versat,Tokenizer* tok){
  tok->AssertNextToken("module");

  Token moduleName = tok->NextToken();

  tok->AssertNextToken("(");

  Accelerator* circuit = CreateAccelerator(versat);

  Arena* arena = &versat->temp;
  ArenaMarker marker(arena);

  InstanceTable* table = PushHashmap<String,FUInstance*>(arena,1000);
  InstanceName* names = PushHashmap<String,int>(arena,1000); // TODO: Can be replaced by Set

  int insertedInputs = 0;
  for(int i = 0; 1; i++){
    Token token = tok->NextToken();

    if(CompareToken(token,")")){
      break;
    }

    Token peek = tok->PeekToken();
    if(!CompareToken(peek,")")){
      tok->AssertNextToken(",");
    }

    String name = PushString(&versat->permanent,token);
    FUInstance* input = CreateOrGetInput(circuit,name,insertedInputs++);
    names->Insert(name,0);
    table->Insert(name,input);
  }

  tok->AssertNextToken("{");

  int shareIndex = 0;
  int state = 0;
  FUInstance* outputInstance = nullptr;
  while(!tok->Done()){
    Token token = tok->PeekToken();

    if(CompareToken(token,"}")){
      tok->AdvancePeek(token);
      break;
    }

    if(CompareToken(token,"#")){
      tok->AdvancePeek(token);
      state = 1;
      continue;
    }

    if(state == 0){
      Token possibleQualifier = tok->PeekToken();

      bool isStatic = false;
      if(CompareString(possibleQualifier,"static")){
        tok->AdvancePeek(possibleQualifier);
        isStatic = true;
      }
      if(CompareString(possibleQualifier,"share")){
        tok->AdvancePeek(possibleQualifier);

        Token shareType = tok->NextToken();

        if(CompareString(shareType,"config")){
          Token typeName = tok->NextToken();

          tok->AssertNextToken("{");

          FUDeclaration* type = GetTypeByName(versat,typeName);
          while(1){
            Token possibleName = tok->NextToken();

            if(CompareString(possibleName,"}")){
              break;
            }

            String name = PushString(&versat->permanent,possibleName);
            String uniqueName = GetUniqueName(name,&versat->permanent,names);

            FUInstance* inst = CreateFUInstance(circuit,type,uniqueName);
            table->Insert(name,inst);

            ShareInstanceConfig(inst,shareIndex);

            tok->AssertNextToken(";");
          }
        } else {
          UNHANDLED_ERROR;
        }

        shareIndex += 1;
      } else {
        FUInstance* instance = ParseInstanceDeclaration(versat,tok,circuit,table,names);

        if(isStatic){
          SetStatic(circuit,instance);
        }

        tok->AssertNextToken(";");
      }
    } else {
      BLOCK_REGION(arena);
        
      Array<Var> outPortion = ParseGroup(tok,arena);

      Var outVar = {};
      if(outPortion.size == 1){
        outVar = outPortion[0];
      }

      //Var outVar = ParseVar(tok);

      Token op = tok->NextToken();
      if(CompareToken(op,"=")){
        if(CompareString(outVar.name,"out")){
          USER_ERROR;
        }

        String name = PushString(&versat->permanent,outVar.name);
        FUInstance* inst = ParseExpression(versat,circuit,tok,table,names);
        String uniqueName = GetUniqueName(name,&versat->permanent,names);
        inst->name = PushString(&versat->permanent,uniqueName);

        names->Insert(name,0);
        table->Insert(name,inst);

        tok->AssertNextToken(";");
      } else if(CompareToken(op,"^=")){
        if(CompareString(outVar.name,"out")){
          USER_ERROR;
        }

        String name = PushString(&versat->permanent,outVar.name);

        FUInstance* existing = table->GetOrFail(name);
        FUInstance* inst = ParseExpression(versat,circuit,tok,table,names);

        String uniqueName = GetUniqueName(name,&versat->permanent,names);
        FUInstance* op = CreateFUInstance(circuit,GetTypeByName(versat,STRING("XOR")),uniqueName);

        ConnectUnits(inst,0,op,0,0);     // Care, we are not taking account port ranges and delays and stuff like that.
        ConnectUnits(existing,0,op,1,0);
        names->Insert(name,0);
        table->Insert(name,op);

        tok->AssertNextToken(";");
      } else if(CompareToken(op,"->")){
        BLOCK_REGION(arena);

        // For now only allow one var on the input side
        // Makes it easier to match
        Var inVar = ParseVar(tok);

        Array<PortExpression> ports = PushArray<PortExpression>(arena,outPortion.size);
        for(int i = 0; i < outPortion.size; i++){
          Var& var = outPortion[i];
          ports[i].inst = table->GetOrFail(var.name);
          ports[i].extra = var.extra;
        }

        FUInstance* inst2 = nullptr;
        if(CompareString(inVar.name,"out")){
          if(!outputInstance){
            outputInstance = (FUInstance*) CreateFUInstance(circuit,BasicDeclaration::output,STRING("out"));
          }

          inst2 = outputInstance;
        } else {
          inst2 = table->GetOrFail(inVar.name);
        }

        ConnectUnit(ports,(PortExpression){inst2,inVar.extra});

        tok->AssertNextToken(";");
        // The problem is when using port based expressions. We cannot have variables represent ports, only instances. And it kinda of messes up the way a programmer thinks. If I use a port based expression on the right, the variable only takes the instance
      } else if(CompareToken(op,"->=")){
        Var inVar = ParseVar(tok);

        if(CompareString(inVar.name,"out")){
          USER_ERROR;
        }

        FUInstance* inst1 = table->GetOrFail(outVar.name);
        FUInstance* inst2 = table->GetOrFail(inVar.name);

        ConnectUnit((PortExpression){inst1,outVar.extra},(PortExpression){inst2,inVar.extra});
        table->Insert(outVar.name,inst2);
        tok->AssertNextToken(";");
      } else {
        String line = tok->GetStartOfCurrentLine();

        int start = GetTokenPositionInside(line,op);

        printf("%.*s\n",UNPACK_SS(line));
        String point = GeneratePointingString(start,1,&versat->temp);
        printf("%.*s\n",UNPACK_SS(point));

        fflush(stdout);
        Assert(0);
      }
    }
  }

  // Care to never put out inside the table
  FUInstance** outInTable = table->Get(STRING("out"));
  Assert(!outInTable);

#if 0
  printf("\n\n");
  for(FUInstance* inst : circuit->instances){
    printf("%.*s\n",UNPACK_SS(inst->name));
  }
#endif

  return RegisterSubUnit(versat,PushString(&versat->permanent,moduleName),circuit);
}

FUDeclaration* ParseIterative(Versat* versat,Tokenizer* tok){
  tok->AssertNextToken("iterative");

  Arena* arena = &versat->temp;
  BLOCK_REGION(arena);

  InstanceTable* table = PushHashmap<String,FUInstance*>(arena,1000);
  Set<String>* names = PushSet<String>(arena,1000);

  String name = tok->NextToken();

  Accelerator* iterative = CreateAccelerator(versat);

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
  int latency = ParseInt(latencyStr);

  FUInstance* outputInstance = nullptr;

  Hashmap<PortInstance,FUInstance*>* portInstanceToMux = PushHashmap<PortInstance,FUInstance*>(arena,10);

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
      num = ParseInt(number);
    }

    Var start = ParseVar(tok);

    tok->AssertNextToken("->");

    Var end = ParseVar(tok);
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
    instance.port = end.extra.portEnd;

    Assert(end.extra.portStart == end.extra.portEnd); // For now do not handle ranges.

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
    end.extra.portEnd = end.extra.portStart = num;
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

#if 0
merge MergeTest = Merge0:a | Merge1:b {
   a.m0 - b.m1;
   a.m1 - b.m0;
}
#endif

void ParseMerge(Versat* versat,Tokenizer* tok){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);
  
  tok->AssertNextToken("merge");
  String mergeName = tok->NextToken();
  tok->AssertNextToken("=");

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
  //NOT_IMPLEMENTED;
  //DebugValue(MakeValue(&list));
}  

void ParseVersatSpecification(Versat* versat,String content){
  Tokenizer tokenizer = Tokenizer(content,"%=#[](){}+:;,*~-",{"->=","->",">><","><<",">>","<<","..","^="});
  Tokenizer* tok = &tokenizer;

  while(!tok->Done()){
    Token peek = tok->PeekToken();

    if(CompareString(peek,"debug")){
      tok->AdvancePeek(peek);
      tok->AssertNextToken(";");
      debugFlag = true;
    } else if(CompareString(peek,"module")){
      ParseModule(versat,tok);
    } else if(CompareString(peek,"iterative")){
      ParseIterative(versat,tok);
    } else if(CompareString(peek,"merge")){
      ParseMerge(versat,tok);
    } else {
      tok->AdvancePeek(peek);
    }
  }
}

void ParseVersatSpecification(Versat* versat,const char* filepath){
  ArenaMarker marker(&versat->temp);

  String content = PushFile(&versat->temp,filepath);

  if(content.size < 0){
    printf("Failed to open file, filepath: %s\n",filepath);
    DEBUG_BREAK();
  }

  ParseVersatSpecification(versat,content);
}





