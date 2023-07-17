#include "verilogParsing.hpp"

#include <cstdio>

#include "memory.hpp"

#include "templateEngine.hpp"

void PerformDefineSubstitution(Arena* output,MacroMap& macros,String name){
   String subs = macros[name];

   Tokenizer inside(subs,"`",{});

   while(!inside.Done()){
      Token peek = inside.PeekFindUntil("`");

      if(peek.size < 0){
         break;
      } else {
         inside.AdvancePeek(peek);
         PushString(output,peek);

         inside.AssertNextToken("`");

         Token name = inside.NextToken();

         PerformDefineSubstitution(output,macros,name);
      }
   }

   Token finish = inside.Finish();
   PushString(output,finish);
}

void PreprocessVerilogFile_(Arena* output, String fileContent,MacroMap& macros,std::vector<String>* includeFilepaths,Arena* temp);

static void DoIfStatement(Arena* output,Tokenizer* tok,MacroMap& macros,std::vector<String>* includeFilepaths,Arena* temp){
   Token first = tok->NextToken();
   Token macroName = tok->NextToken();

   bool compareVal = false;
   if(CompareString(first,"`ifdef") || CompareString(first,"`elsif")){
      compareVal = true;
   } else if(CompareString(first,"`ifndef")){
      compareVal = false;
   } else {
      UNHANDLED_ERROR;
   }

   bool exists = (macros.find(macroName) != macros.end());
   bool doIf = (compareVal == exists);

   void* mark = tok->Mark();
   while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareString(peek,"`endif")){
         if(doIf){
            Token content = tok->Point(mark);
            PreprocessVerilogFile_(output,content,macros,includeFilepaths,temp);
         }
         tok->AdvancePeek(peek);
         break;
      }

      if(CompareString(peek,"`else")){
         if(doIf){
            Token content = tok->Point(mark);
            PreprocessVerilogFile_(output,content,macros,includeFilepaths,temp);
         } else {
            tok->AdvancePeek(peek);
            mark = tok->Mark();
            doIf = true;
         }
         continue;
      }

      if(CompareString(peek,"`ifdef") || CompareString(peek,"`ifndef") || CompareString(first,"`elsif")){
         DoIfStatement(output,tok,macros,includeFilepaths,temp);
      } else {
         tok->AdvancePeek(peek);
      }
   }
}

void PreprocessVerilogFile_(Arena* output, String fileContent,MacroMap& macros,std::vector<String>* includeFilepaths,Arena* temp){
   Tokenizer tokenizer = Tokenizer(fileContent, "()`\\\",+-/*",{"`include","`define","`timescale","`ifdef","`else","`elsif","`endif","`ifndef"});
   Tokenizer* tok = &tokenizer;

   while(!tok->Done()){
      Token peek = tok->PeekToken();
      if(CompareToken(peek,"`include")){ // Assuming defines only happen outside module (Not correct but follows common usage, never seen include between parameters or port definitions)
         tok->AdvancePeek(peek);
         tok->AssertNextToken("\"");

         Token fileName = tok->NextFindUntil("\"");
         tok->AssertNextToken("\"");

         // Open include file
         std::string filename(UNPACK_SS_REVERSE(fileName));
         FILE* file = nullptr;
         for(String str : *includeFilepaths){
            std::string string(str.data,str.size);

            std::string filepath;
            if(string.back() == '/'){
               filepath = string + filename;
            } else {
               filepath = string + '/' + filename;
            }

            file = fopen(filepath.c_str(),"r");

            if(file){
               break;
            }
         }

         if(!file){
            printf("Couldn't find file: %.*s\n",UNPACK_SS(fileName));
            printf("Looked on the following folders:\n");

            printf("  %s\n",GetCurrentDirectory());
            for(String str : *includeFilepaths){
               printf("  %.*s\n",UNPACK_SS(str));
            }

            DEBUG_BREAK();
            exit(0);
         }

         size_t fileSize = GetFileSize(file);

         Byte* mem = PushBytes(temp,fileSize + 1);
         fread(mem,sizeof(char),fileSize,file);
         mem[fileSize] = '\0';

         PreprocessVerilogFile_(output,STRING((const char*) mem,fileSize),macros,includeFilepaths,temp);

         fclose(file);
      } else if(CompareToken(peek,"`define")){ // Same for defines
         tok->AdvancePeek(peek);
         Token defineName = tok->NextToken();

         Token emptySpace = tok->PeekWhitespace();
         if(emptySpace.size == 0){ // Function macro
            Token arguments = tok->PeekFindIncluding(")");
            defineName = ExtendToken(defineName,arguments);
            tok->AdvancePeek(arguments);
         }

         FindFirstResult search = tok->FindFirst({"\n","//","\\"});
         Token first = search.foundFirst;
         Token body = {};

         if(CompareString(first,"//")){ // If single comment newline or slash, the macro does not contain the comment
            body = tok->PeekFindUntil("//");
         } else {
            Token line = tok->PeekFindUntil("\n");
            body = line;
            while(!tok->Done()){
               if(line.size == -1){
                  line = tok->Finish();
                  body = ExtendToken(body,line);
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
                  tok->AdvancePeek(line);
                  body = ExtendToken(body,line);
                  line = tok->PeekFindUntil("\n");
               } else {
                  tok->AdvancePeek(line);
                  body = ExtendToken(body,line);
                  break;
               }
            }
         }

         macros[defineName] = body;
      } else if(CompareToken(peek,"`timescale")){
         Token line = tok->PeekFindIncluding("\n");

         tok->AdvancePeek(line);
      } else if(CompareToken(peek,"`")){
         tok->AdvancePeek(peek);
         Token name = tok->NextToken();

         PerformDefineSubstitution(output,macros,name);
      } else if(CompareToken(peek,"`ifdef") || CompareToken(peek,"`ifndef")){
         DoIfStatement(output,tok,macros,includeFilepaths,temp);
      } else if(CompareToken(peek,"`else")){
         NOT_POSSIBLE;
      } else if(CompareToken(peek,"`endif")){
         NOT_POSSIBLE;
      } else if(CompareToken(peek,"`elsif")){
         NOT_POSSIBLE;
      } else {
         tok->AdvancePeek(peek);

         PushString(output,peek);
      }

      PushString(output,tok->PeekWhitespace());
   }
}

String PreprocessVerilogFile(Arena* output, String fileContent,std::vector<String>* includeFilepaths,Arena* arena){
   MacroMap macros;

   String res = {};
   res.data = (const char*) &output->mem[output->used];
   Byte* mark = MarkArena(output);

   PreprocessVerilogFile_(output,fileContent,macros,includeFilepaths,arena);

   PushString(output,STRING("\0"));
   res.size = MarkArena(output) - mark;

   return res;
}

static Expression* VerilogParseAtom(Tokenizer* tok,Arena* out){
   Expression* expr = PushStruct<Expression>(out);
   *expr = {};

   Token peek = tok->PeekToken();

   if(IsNum(peek[0])){
      int res = ParseInt(peek);
      tok->AdvancePeek(peek);

      expr->val = MakeValue(res);
      expr->type = Expression::LITERAL;

      return expr;
   } else if(peek[0] == '"'){
      tok->AdvancePeek(peek);
      Token str = tok->PeekFindUntil("\"");
      tok->AdvancePeek(str);
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

static Expression* VerilogParseFactor(Tokenizer* tok,Arena* arena){
   Token peek = tok->PeekToken();

   if(CompareString(peek,"(")){
      tok->AdvancePeek(peek);
      Expression* expr = VerilogParseExpression(tok,arena);
      tok->AssertNextToken(")");
      return expr;
   } else {
      Expression* expr = VerilogParseAtom(tok,arena);
      return expr;
   }
}

static Value Eval(Expression* expr,ValueMap& map){
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
         NOT_IMPLEMENTED;
      }break;
      }
   }break;

   case Expression::IDENTIFIER:{
      return map[expr->id];
   }break;
   case Expression::LITERAL:{
      return expr->val;
   }break;
   default:{
      NOT_POSSIBLE;
   }break;
   }

   NOT_POSSIBLE;
   return MakeValue();
}

static Array<ParameterExpression> ParseParameters(Tokenizer* tok,ValueMap& map,Arena* arena){
   //TODO: Add type and range to parsing
   /*
   Range currentRange;
   ParameterType type;
   */

   PushPtr<ParameterExpression> params(arena,99); // TODO: Calculate correct amount

   //Array<DefaultParameters> params =
   //ValueMap parameters;

   while(1){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"parameter")){
         tok->AdvancePeek(peek);
         // Parse optional type info and range
         continue;
      } else if(CompareToken(peek,")")){
         break;
      } else if(CompareToken(peek,";")){ // To parse inside module parameters, technically wrong but harmless
         tok->AdvancePeek(peek);
         break;
      } else if(CompareToken(peek,",")){
         tok->AdvancePeek(peek);
         continue;
      } else { // Must be a parameter assignment
         Token paramName = tok->NextToken();

         tok->AssertNextToken("=");

         Expression* expr = VerilogParseAtom(tok,arena);
         Value val = Eval(expr,map);

         map[paramName] = val;

         ParameterExpression* p = params.Push(1);
         p->name = paramName;
         p->expr = expr;
         //parameters[paramName] = val;
      }
   }

   return params.AsArray();
}

static Expression* VerilogParseExpression(Tokenizer* tok,Arena* out){
   Expression* res = ParseOperationType(tok,{{"+","-"},{"*","/"}},VerilogParseFactor,out);

   return res;
}

static RangeAndExpr ParseRange(Tokenizer* tok,ValueMap& map,Arena* out){
   Token peek = tok->PeekToken();

   if(!CompareString(peek,"[")){
      RangeAndExpr range = {};

      return range;
   }

   tok->AssertNextToken("[");

   RangeAndExpr res = {};
   res.top = VerilogParseExpression(tok,out);
   res.range.high = Eval(res.top,map).number;

   tok->AssertNextToken(":");

   res.bottom = VerilogParseExpression(tok,out);
   res.range.low = Eval(res.bottom,map).number;
   tok->AssertNextToken("]");

   return res;
}

#define VERSAT_LATENCY STRING("versat_latency")
#define VERSAT_STATIC  STRING("versat_static")

static String possibleAttributesData[] = {VERSAT_LATENCY,VERSAT_STATIC};
static Array<String> possibleAttributes = C_ARRAY_TO_ARRAY(possibleAttributesData);

static Module ParseModule(Tokenizer* tok,Arena* arena){
   Module module = {};
   ValueMap values;

   tok->AssertNextToken("module");

   module.name = tok->NextToken();

   Token peek = tok->PeekToken();
   if(CompareToken(peek,"#(")){
      tok->AdvancePeek(peek);
      module.parameters = ParseParameters(tok,values,arena);
      tok->AssertNextToken(")");
   }

   tok->AssertNextToken("(");

   // Parse ports
   while(!tok->Done()){
      peek = tok->PeekToken();

      PortDeclaration port;

      if(CompareToken(peek,"(*")){
         tok->AdvancePeek(peek);
         while(1){
            Token attributeName = tok->NextToken();

            if(!Contains(possibleAttributes,attributeName)){
               printf("ERROR: Do not know attribute named: %.*s\n",UNPACK_SS(attributeName));
               exit(-1);
            }

            peek = tok->PeekToken();
            if(CompareString(peek,"=")){
               tok->AdvancePeek(peek);
               Expression* expr = VerilogParseExpression(tok,arena);
               Value value = Eval(expr,values);

               peek = tok->PeekToken();

               port.attributes[attributeName] = value;
            } else {
               port.attributes[attributeName] = MakeValue();
            }

            if(CompareString(peek,",")){
               tok->AdvancePeek(peek);
               continue;
            }
            if(CompareString(peek,"*)")){
               tok->AdvancePeek(peek);
               break;
            }
         }
      }

      Token portType = tok->NextToken();
      if(CompareString(portType,"input")){
         port.type = PortDeclaration::INPUT;
      } else if(CompareString(portType,"output")){
         port.type = PortDeclaration::OUTPUT;
      } else if(CompareString(portType,"inout")){
         port.type = PortDeclaration::INOUT;
      } else {
         UNHANDLED_ERROR;
      }

      // TODO: Add a new function to parser to "ignore" the following list of tokens (loop every time until it doesn't find one from the list), and replace this function here with reg and all the different types it can be
      while(1){
         peek = tok->PeekToken();
         if(CompareString("reg",peek)){
            tok->AdvancePeek(peek);
            continue;
         }
         if(CompareString("signed",peek)){
            tok->AdvancePeek(peek);
            continue;
         }
         break;
      }
      
      RangeAndExpr res = ParseRange(tok,values,arena);
      port.range = res.range;
      port.top = res.top;
      port.bottom = res.bottom;
      port.name = tok->NextToken();

      module.ports.push_back(port);

      peek = tok->PeekToken();
      if(CompareToken(peek,")")){
         tok->AdvancePeek(peek);
         break;
      }

      tok->AssertNextToken(",");
   }

   // Any inside module parameters
   #if 0
   while(1){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"parameter")){
         ParseParameters(tok,values);
      } else if(CompareToken(peek,"endmodule")){
         tok->AdvancePeek(peek);
         break;
      } else {
         tok->AdvancePeek(peek);
      }
   }
   #endif

   Token skip = tok->PeekFindIncluding("endmodule");
   tok->AdvancePeek(skip);

   return module;
}

std::vector<Module> ParseVerilogFile(String fileContent, std::vector<String>* includeFilepaths, Arena* arena){
   Tokenizer tokenizer = Tokenizer(fileContent,":,()[]{}\"+-/*=",{"#(","+:","-:","(*","*)"});
   Tokenizer* tok = &tokenizer;

   std::vector<Module> modules;

   bool isSource = false;
   while(!tok->Done()){
      Token peek = tok->PeekToken();

      if(CompareToken(peek,"(*")){
         tok->AdvancePeek(peek);

         Token attribute = tok->NextToken();
         if(CompareToken(attribute,"source")){
            isSource = true;
         } else {
            NOT_IMPLEMENTED; // Unknown attribute, error for now
         }

         tok->AssertNextToken("*)");

         continue;
      }

      if(CompareToken(peek,"module")){
         Module module = ParseModule(tok,arena);

         module.isSource = isSource;
         modules.push_back(module);

         isSource = false;
         break; // For now, only parse the first module found
      }

      tok->AdvancePeek(peek);
   }

   return modules;
}

ModuleInfo ExtractModuleInfo(Module& module,Arena* permanent,Arena* tempArena){
   BLOCK_REGION(tempArena);

   ModuleInfo info = {};

   info.defaultParameters = module.parameters;

   PushPtr<int> inputDelay(permanent,100);
   PushPtr<int> outputLatency(permanent,100);
   PushPtr<WireExpression> configs(permanent,1000);
   PushPtr<WireExpression> states(permanent,1000);

   info.name = module.name;
   info.isSource = module.isSource;

   Hashmap<ExternalMemoryID,ExternalMemoryInfo>* external = PushHashmap<ExternalMemoryID,ExternalMemoryInfo>(tempArena,100);

   for(PortDeclaration decl : module.ports){
      Tokenizer port(decl.name,"",{"in","out","delay","done","rst","clk","run","running","databus"});

      if(CompareString("signal_loop",decl.name)){
         info.signalLoop = true;
      } else if(CheckFormat("ext_dp_%s_%d_port_%d",decl.name)){
         Array<Value> values = ExtractValues("ext_dp_%s_%d_port_%d",decl.name,tempArena);

         ExternalMemoryID id = {};
         id.interface = values[1].number;
         id.type = ExternalMemoryType::DP;

         String wire = values[0].str;
         int port = values[2].number;

         Assert(port < 2);

         ExternalMemoryInfo* info = external->GetOrInsert(id,{});
         if(CompareString(wire,"addr")){
            info->ports[port].addrSize = decl.range.high - decl.range.low + 1;
         } else if(CompareString(wire,"out")){
            info->ports[port].dataOutSize = decl.range.high - decl.range.low + 1;
         } else if(CompareString(wire,"in")){
            info->ports[port].dataInSize = decl.range.high - decl.range.low + 1;
         } else if(CompareString(wire,"write")){
            info->ports[port].write = true;
         } else if(CompareString(wire,"enable")){
            info->ports[port].enable = true;
         }
      } else if(CheckFormat("ext_2p_%s",decl.name)){
         ExternalMemoryID id = {};
         id.type = ExternalMemoryType::TWO_P;

         String wire = {};
         if(CheckFormat("ext_2p_%s_%s_%d",decl.name)){
            Array<Value> values = ExtractValues("ext_2p_%s_%s_%d",decl.name,tempArena);

            wire = values[0].str;
            id.interface = values[2].number;
         } else if(CheckFormat("ext_2p_%s_%d",decl.name)){
            Array<Value> values = ExtractValues("ext_2p_%s_%d",decl.name,tempArena);

            wire = values[0].str;
            id.interface = values[1].number;
         } else {
            UNHANDLED_ERROR;
         }

         ExternalMemoryInfo* info = external->GetOrInsert(id,{});

         if(CompareString(wire,"addr")){
            info->ports[0].addrSize = decl.range.high - decl.range.low + 1;
         } else if(CompareString(wire,"data")){
            info->ports[0].dataInSize = decl.range.high - decl.range.low + 1;
            info->ports[0].dataOutSize = decl.range.high - decl.range.low + 1;
         } else if(CompareString(wire,"write")){
            info->ports[0].write = true;
         } else if(CompareString(wire,"read")){
            info->ports[0].write = true;
         } else {
            UNHANDLED_ERROR;
         }
      } else if(CheckFormat("in%d",decl.name)){
         port.AssertNextToken("in");
         /* int input = */ ParseInt(port.NextToken());
         int delay = decl.attributes[VERSAT_LATENCY].number;

         *inputDelay.Push(1) = delay;
      } else if(CheckFormat("out%d",decl.name)){
         port.AssertNextToken("out");
         /* int output = */ ParseInt(port.NextToken());
         int latency = decl.attributes[VERSAT_LATENCY].number;

         *outputLatency.Push(1) = latency;
      } else if(CheckFormat("delay%d",decl.name)){
         port.AssertNextToken("delay");
         int delay = ParseInt(port.NextToken());

         info.nDelays = std::max(info.nDelays,delay + 1);
      } else if(  CheckFormat("databus_ready_%d",decl.name)
               || CheckFormat("databus_valid_%d",decl.name)
               || CheckFormat("databus_addr_%d",decl.name)
               || CheckFormat("databus_rdata_%d",decl.name)
               || CheckFormat("databus_wdata_%d",decl.name)
               || CheckFormat("databus_wstrb_%d",decl.name)
               || CheckFormat("databus_len_%d",decl.name)
               || CheckFormat("databus_last_%d",decl.name)){
         Array<Value> val = ExtractValues("databus_%s_%d",decl.name,tempArena);

         if(CheckFormat("databus_addr_%d",decl.name)){
            info.databusAddrBottom = decl.top;
            info.databusAddrTop = decl.bottom;
         }

         #if 0
         info.addrTop = decl.top;
         info.addrBottom = decl.bottom;

         if(info.addrTop) {
            PrintExpression(info.addrTop);
         }
         #endif

         info.nIO = val[1].number;
         info.doesIO = true;
      } else if(  CheckFormat("ready",decl.name)
               || CheckFormat("valid",decl.name)
               || CheckFormat("addr",decl.name)
               || CheckFormat("rdata",decl.name)
               || CheckFormat("wdata",decl.name)
               || CheckFormat("wstrb",decl.name)){
         info.memoryMapped = true;

         if(CheckFormat("addr",decl.name)){
            info.memoryMappedBits = (decl.range.high - decl.range.low + 1);
         }
      } else if(CheckFormat("clk",decl.name)){
         info.hasClk = true;
      } else if(CheckFormat("rst",decl.name)){
         info.hasReset = true;
      } else if(CheckFormat("run",decl.name)){
         info.hasRun = true;
      } else if(CheckFormat("running",decl.name)){
         info.hasRunning = true;
      } else if(CheckFormat("done",decl.name)){
         info.hasDone = true;
      } else if(decl.type == PortDeclaration::INPUT){ // Config
         WireExpression* wire = configs.Push(1);

         //wire->bitsize = decl.range.high - decl.range.low + 1;
         wire->top = decl.top;
         wire->bottom = decl.bottom;
         wire->name = decl.name;
         wire->isStatic = (decl.attributes.find(VERSAT_STATIC) != decl.attributes.end());
      } else if(decl.type == PortDeclaration::OUTPUT){ // State
         WireExpression* wire = states.Push(1);

         wire->top = decl.top;
         wire->bottom = decl.bottom;
         wire->name = decl.name;
      } else {
         NOT_IMPLEMENTED;
      }
   }

   info.inputDelays = inputDelay.AsArray();
   info.outputLatencies = outputLatency.AsArray();
   info.configs = configs.AsArray();
   info.states = states.AsArray();

   if(info.doesIO){
      info.nIO += 1;
   }

   Array<ExternalMemoryInterface> interfaces = PushArray<ExternalMemoryInterface>(permanent,external->nodesUsed);
   int index = 0;
   for(Pair<ExternalMemoryID,ExternalMemoryInfo> pair : external){
      ExternalMemoryInterface& inter = interfaces[index++];

      inter.interface = pair.first.interface;
      inter.type = pair.first.type;
      inter.bitsize = pair.second.ports[0].addrSize;
      inter.datasize = pair.second.ports[0].dataInSize;
   }
   info.externalInterfaces = interfaces;

   return info;
}

void OutputModuleInfos(FILE* output,ModuleInfoInstance info,String nameSpace,CompiledTemplate* unitVerilogData,Arena* temp,Array<Wire> configsHeaderSide,Array<String> statesHeaderSide){
   ClearTemplateEngine();
   TemplateSetCustom("module",&info,"ModuleInfoInstance");
   TemplateSetString("namespace",nameSpace);
   TemplateSetArray("configsHeader","Wire",configsHeaderSide.data,configsHeaderSide.size);
   TemplateSetArray("statesHeader","String",statesHeaderSide.data,statesHeaderSide.size);

   ProcessTemplate(output,unitVerilogData,temp);
}

void GetAllIdentifiers_(Expression* expr,PushPtr<String>& ptr){
   if(expr->type == Expression::IDENTIFIER){
      String id = expr->id;

      Array<String> arr = ptr.AsArray();
      if(Contains(arr,id)){
         return;
      } else {
         ptr.PushValue(id);
      }
   }

   for(Expression* child : expr->expressions){
      GetAllIdentifiers_(child,ptr);
   }
}

Array<String> GetAllIdentifiers(Expression* expr,Arena* arena){
   PushPtr<String> ptr(arena,99);

   GetAllIdentifiers_(expr,ptr);

   PopPushPtr(arena,ptr);

   return ptr.AsArray();
}

Value Eval(Expression* expr,Array<ParameterExpression> parameters){
   Value ret = {};
   switch(expr->type){
   case Expression::LITERAL: {
      ret = expr->val;
   } break;
   case Expression::OPERATION: {
      // TODO: Not first place where we have default operation behaviour. Refactor into a single place
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
      default: {
         printf("%s\n",expr->op);
         NOT_IMPLEMENTED;
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
   default: {
      printf("%d\n",(int) expr->type);
      NOT_IMPLEMENTED;
   } break;
   }

   return ret;
}



