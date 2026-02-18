#pragma once

#include "symbolic.hpp"

// ============================================================================
// Verilog interface manipulation

enum WireDir{
  WireDir_INPUT,
  WireDir_OUTPUT,
  WireDir_INOUT // We do not actually support this, but still need to represent it so that later we can report this as error
};

static inline WireDir ReverseDir(WireDir dir){
  FULL_SWITCH(dir){
  case WireDir_INPUT: return WireDir_OUTPUT;
  case WireDir_OUTPUT: return WireDir_INPUT;
  case WireDir_INOUT: return WireDir_INOUT;
  }

  NOT_POSSIBLE();
}

enum SpecialPortProperties{
  SpecialPortProperties_None     = 0,
  SpecialPortProperties_IsShared = (1 << 0), // For unpacking, shared wires are replicated accross every interface (think rdata and the like)
  SpecialPortProperties_IsClock  = (1 << 1),
  SpecialPortProperties_IsReset  = (1 << 2)
};

struct VerilogPortSpec{
  String name;
  SymbolicExpression* size;
  WireDir dir;
  SpecialPortProperties props;
};

struct VerilogPortGroup{
  String name;
  Array<VerilogPortSpec> ports;
};

struct VerilogModuleInterface{
  Array<VerilogPortGroup> portGroups;
};

// ======================================
// Verilog interface builder

struct VerilogModuleBuilder{
  Arena* arena;

  String currentPortGroupName;
  
  ArenaList<VerilogPortSpec>* currentPorts;
  ArenaList<VerilogPortGroup>* groups;

  // TODO: Is it worth it to encode the groups as an enum instead of names? 
  void StartGroup(String name);
  void EndGroup(bool preserveEmptyGroup = false);

  // If called without group, creates a single wire group.
  void AddPort(String name,SymbolicExpression* expr,WireDir dir,SpecialPortProperties props = SpecialPortProperties_None);
  void AddPortIndexed(const char* format,int index,SymbolicExpression* expr,WireDir dir,SpecialPortProperties props = SpecialPortProperties_None);

  // Acts as a bunch of AddPort calls
  void AddInterface(Array<VerilogPortSpec> interface,String prefix);
  void AddInterfaceIndexed(Array<VerilogPortSpec> interfaceFormat,int index,String prefix);
};

VerilogModuleBuilder*   StartVerilogModuleInterface(Arena* arena);
VerilogModuleInterface* End(VerilogModuleBuilder*,Arena* out);

// ======================================
// Verilog interface manipulation

Array<VerilogPortSpec> AppendSuffix(Array<VerilogPortSpec> in,String suffix,Arena* out);
Array<VerilogPortSpec> ReverseInterfaceDirection(Array<VerilogPortSpec> in,Arena* out);
Array<VerilogPortSpec> AddDirectionToName(Array<VerilogPortSpec> in,Arena* out);

Array<VerilogPortSpec> ExtractAllPorts(VerilogModuleInterface* interface,Arena* out);
Array<VerilogPortSpec> ObtainGroupByName(VerilogModuleInterface* interface,String name);

// True if group was found and removed
bool RemoveGroupInPlace(VerilogModuleInterface* interface,String name);

Opt<VerilogPortSpec>   GetPortSpecByName(Array<VerilogPortSpec> array,String name);

// TODO: We might eventually change from using strings to using an enum for group type.
bool ContainsGroup(VerilogModuleInterface* interface,String name);

// ============================================================================
// Emitter

// TODO: Because the way verilog tools work and how certain linters
// expect code to follow a certain format, try to abstract the rules
// on this level and inside the Repr function. This means that if, for
// example, linter demands that a signal is cleared in a certain way,
// then implement a Clear function that makes sure that we always
// clear the signal that way, instead of pushing the logic
// outwards. Try to consolidate as much of verilog logic on this
// Emitter as much as possible and abstract the actual Verilog
// generation from the rest of the code. Basically, we do not actually
// care if the "VAST" actually corresponds to a Verilog AST node or if
// it corresponds to a higher order node that does not match a single
// verilog construct.
// NOTE: Of course we do this as the need arises. Need to see in what
// ways the linters/tools complain before figuring out how to do this
// NOTE2: If needed, we can also change this into a two step approach,
// where the first step is in a higher level and the bottom step is
// what we currently have somewhat. The higher level generates the
// lower level AST which then generates verilog.  If we end up wanting
// to support VHDL, we would have the higher level generate the AST
// for VHDL that way.

// TODO: Right now we kinda are doing everything manually but there is
// a couple of ways we can improve the Verilog emission. When
// declaring a variable we could keep track of the size and use it to
// simplify later code.  There are a bunch of high level methods we
// could make, depending on how the Emitter is used and if it would
// simplify the code or would help make it linter friendly.


enum VASTType{
  VASTType_TOP_LEVEL,
  VASTType_MODULE_DECL,
  VASTType_TASK_DECL,
  VASTType_PORT_DECL,
  VASTType_PORT_GROUP,
  VASTType_VAR_DECL,
  VASTType_INTEGER_DECL,
  VASTType_LOCAL_PARAM,
  VASTType_WIRE_ASSIGN_BLOCK,
  VASTType_ASSIGN_DECL,
  VASTType_PORT_CONNECT,
  VASTType_RAW_STATEMENT,
  VASTType_INTERFACE_DECL, // Similar to var decl but with interfaces 
  VASTType_IF,
  VASTType_LOOP,
  VASTType_SET,
  VASTType_SIMPLE_OP,
  VASTType_FORCED_SET,
  VASTType_EXPR,
  VASTType_ALWAYS_BLOCK,
  VASTType_INITIAL_BLOCK,
  VASTType_COMMENT,
  VASTType_INSTANCE,
  VASTType_BLANK
};

String Repr(VASTType type);

struct VAST;

struct VASTIf{
  // TODO: Probably should be an expression but we are not doing expressions right now.
  String ifExpression;
  ArenaList<VAST*>* statements;
};

struct VAST{
  VASTType type;

  union{
    String comment;
    String name;
    String rawStatement;
    
    struct{
      String timescaleExpression;
      ArenaList<String>* includes;
      ArenaList<VAST*>* declarations;
      ArenaList<VAST*>* portConnections;
    } top;

    struct{
      String name;
      String bitSizeExpr;
      bool isInput; // We only support input or output, no inout.
    } portDecl;

    struct {
      String name;
      String defaultValue;
    } localParam;
    
    struct{
      String name;
      String arrayDim;
      String bitSize;
      bool isWire;
    } varDecl; // Wires and regs 

    struct {
      VerilogModuleInterface* interface;
      String prefix;
    } interfaceDecl;
    
    struct {
      String content;
    } expr;

    struct {
      Array<String> sensitivity;
      ArenaList<VAST*>* declarations;
    } alwaysBlock;

    struct {
      String identifier;
      char op;
    } simpleOp;

    struct {
      ArenaList<VAST*>* declarations;
    } initialBlock;

    struct {
      String portName;
      String connectExpr;
    } portConnect;
    
    struct {
      String name;
      String expr;
      bool isForcedCombLike;
    } assignOrSet; 

    struct {
      ArenaList<VASTIf>* ifExpressions;
      ArenaList<VAST*>* elseStatements; // If nullptr then no else clause
    } ifExpr; 

    struct {
      String initExpr;
      String loopExpr;
      String incrExpr;
      ArenaList<VAST*>* statements;
    } loopExpr; 
    
    struct{
      String name;
      String bitSize;
      String joinElem;
      ArenaList<VAST*>* expressions;
    } wireAssignBlock;

    struct {
      ArenaList<VAST*>* portDeclarations;
    } portGroup;

    struct{
      String name;
      ArenaList<Pair<String,String>>* parameters;
      ArenaList<VAST*>* portDeclarations;
      ArenaList<VAST*>* declarations;
    } module;

    struct{
      String name;
      ArenaList<VAST*>* portDeclarations;
      ArenaList<VAST*>* declarations;
    } task;

    struct {
      String moduleName;
      String name;
      ArenaList<Pair<String,String>>* parameters;
      ArenaList<VAST*>* portConnections;
    } instance;
  };
};

struct VEmitter{
  Arena* arena;
  VAST* topLevel;
  Array<VAST*> buffer;
  int top;  
  
  // Helper functions
  void PushLevel(VAST* level);
  void PopLevel();
  void InsertDeclaration(VAST* decl);
  void InsertPortDeclaration(VAST* decl);
  void InsertPortConnect(VAST* decl);
  VAST* FindFirstVASTType(VASTType type,bool errorIfNotFound = true);

  //void Timescale(const char* timeUnit,const char* timePrecision);
  void Timescale(String timeUnit,String timePrecision);
  void Include(String name);
  
  // Module definition
  void Module(String name);
  void ModuleParam(String name,int value); // A global param of a module
  void ModuleParam(String name,String value);
  void EndModule();

  void Task(String name);
  void EndTask();
  
  void StartPortGroup();
  void EndPortGroup();

  // TODO: We probably want to change all the bitsize to accept a SymbolicExpression. A lot of these duplicated functions can be removed and only access sym expressions.
  // TODO: We also want to remove all the const char* and only use String
  void Input(String name,int bitsize);
  void Input(String name,String expr);
  void Input(String name,SymbolicExpression* expr = SYM_one);
  void InputIndexed(const char* format,int index,int bitsize);
  void InputIndexed(const char* format,int index,String expr);
  void InputIndexed(const char* format,int index,SymbolicExpression* expr = SYM_one);

  void Output(String name,int bitsize);
  void Output(String name,String expr);
  void Output(String name,SymbolicExpression* expr = SYM_one);
  void OutputIndexed(const char* format,int index,int bitsize);
  void OutputIndexed(const char* format,int index,String expr);
  void OutputIndexed(const char* format,int index,SymbolicExpression* expr = SYM_one);
  
  // Module declarations
  void Wire(String name,int bitsize);
  void Wire(String name,String bitsizeExpr);
  void Wire(String name,SymbolicExpression* expr = SYM_one);
  void WireArray(String name,int count,int bitsize);
  void WireArray(String name,int count,String bitsizeExpr);
  void WireArray(String name,int count,SymbolicExpression* expr = SYM_one);
  void WireAndAssignJoinBlock(String name,String joinElem,int bitsize = 1);
  void WireAndAssignJoinBlock(String name,String joinElem,String bitsize);
  void Reg(String name,int bitsize);
  void Reg(String name,String expr);
  void Reg(String name,SymbolicExpression* expr = SYM_one);
  void Assign(String name,String expr);
  void Integer(String name);
  void LocalParam(String name,String defaultValue = {});
  
  void DeclareInterface(VerilogModuleInterface* interface,String prefix);
  
  void Blank();
  void Comment(String expr);
  void Expression(String expr);
  void RawStatement(String stmt);
  
  void AlwaysBlock(String posedge1,String posedge2); // We only care about clk and rst so max of two allowed
  void InitialBlock();
  void CombBlock();

  void Set(String identifier,int val);
  void Set(String identifier,String expr);

  void Increment(String identifier);

  void SetForced(String identifier,String expr,bool isCombLike); // isCombLike is '=', otherwise it is '<=" 

  void If(String expr);
  void ElseIf(String expr);
  void Else();
  void EndIf();

  void Loop(String start,String loop,String incr);
  void EndLoop();
  
  void EndBlock();

  // Instantiating
  void StartInstance(String moduleName,String instanceName);
  void InstanceParam(String paramName,int paramValue);
  void InstanceParam(String paramName,String paramValue);
  void InstanceParam(String paramName,SymbolicExpression* sym);
  void PortConnect(String portName,String connectionExpr);
  void PortConnectIndexed(const char* portFormat,int index,String connectionExpr);
  void PortConnectIndexed(const char* portFormat,int index,const char* connectFormat,int connectIndex);

  void PortConnectInterface(VerilogModuleInterface* interface,String interfaceWirePrefix);

  void PortConnect(Array<VerilogPortSpec> specs,String sufix);

  void EndInstance();

};

VAST* PushVAST(VASTType type,Arena* out);

VEmitter* StartVCode(Arena* out);
VAST* EndVCode(VEmitter* m);

void Repr(VAST* top,StringBuilder* b);

String EndVCodeAndPrint(VEmitter* v,Arena* out);
