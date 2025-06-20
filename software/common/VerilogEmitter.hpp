#pragma once

#include "utils.hpp"

// TODO: Because the way verilog tools work and how certain linters expect code to follow a certain format, try to abstract the rules on this level and inside the Repr function. This means that, if, for example, linter demands that a signal is cleared in a certain way, then implement a Clear function that makes sure that we always clear the signal that way, instead of pushing the logic outwards. Try to consolidate as much of verilog logic on this Emitter as much as possible and abstract the actual Verilog generation from the rest of the code.

enum VASTType{
  VASTType_TOP_LEVEL,
  VASTType_MODULE_DECL,
  VASTType_PORT_DECL,
  VASTType_PORT_GROUP,
  VASTType_VAR_DECL,
  VASTType_WIRE_ASSIGN_BLOCK,
  VASTType_ASSIGN_DECL,
  VASTType_PORT_CONNECT,
  VASTType_IF,
  VASTType_SET,
  VASTType_EXPR,
  VASTType_ALWAYS_DECL,
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

    struct{
      String name;
      String arrayDim;
      String bitSize;
      bool isWire;
    } varDecl; // Wires and regs 

    struct {
      String content;
    } expr;

    struct {
      Array<String> sensitivity;
      ArenaList<VAST*>* declarations;
    } alwaysBlock;

    struct {
      String portName;
      String connectExpr;
    } portConnect;
    
    struct {
      String name;
      String expr;
    } assignOrSet; 

    struct {
      ArenaList<VASTIf>* ifExpressions;
      ArenaList<VAST*>* elseStatements; // If nullptr then no else clause
    } ifExpr; 
    
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
  
  // Helper functions, do not use unless needed
  void PushLevel(VAST* level);
  void PopLevel();
  void InsertDeclaration(VAST* decl);
  void InsertPortDeclaration(VAST* decl);
  void InsertPortConnect(VAST* decl);
  VAST* FindFirstVASTType(VASTType type,bool errorIfNotFound = true);

  void Timescale(const char* timeUnit,const char* timePrecision);
  void Include(const char* name);
  
  // Module definition
  void Module(String name);
  void DeclParam(const char* name,int value); // A global param of a module
  void EndModule();

  void StartPortGroup();
  void EndPortGroup();
  
  void Input(const char* name,int bitsize = 1);
  void Input(String name,int bitsize = 1);
  void Input(const char* name,const char* expr);
  void InputIndexed(const char* format,int index,int bitsize = 1);
  void InputIndexed(const char* format,int index,const char* expression);

  void Output(const char* name,int bitsize = 1);
  void Output(const char* name,const char* expr);
  void Output(String name,int bitsize = 1);
  void OutputIndexed(const char* format,int index,int bitsize = 1);
  void OutputIndexed(const char* format,int index,const char* expression);

  // Module declarations
  void Wire(const char* name,int bitsize = 1);
  void Wire(const char* name,const char* bitsizeExpr);
  void WireArray(const char* name,int count,int bitsize = 1);
  void WireArray(const char* name,int count,const char* bitsizeExpr);
  void WireAndAssignJoinBlock(const char* name,const char* joinElem,int bitsize = 1);
  void Reg(const char* name,int bitsize = 1);
  void Assign(const char* name,const char* expr);
  void Assign(String name,String expr);
  
  void Blank();
  void Expression(const char* expr);

  // Kinda hardcoded for now, we mostly only care about clk and rst signals.
  void AlwaysBlock(const char* posedge1,const char* posedge2);

  void CombBlock();
  void Set(const char* identifier,int val);
  void Set(const char* identifier,const char* expr);
  void Set(String identifier,String expr);

  void If(const char* expr);
  void ElseIf(const char* expr);
  void Else();
  void EndIf();
  
  void EndBlock();
  
  // Instantiating
  void StartInstance(const char* moduleName,const char* instanceName);
  void StartInstance(String moduleName,const char* instanceName);
  void InstanceParam(const char* paramName,int paramValue);
  void InstanceParam(const char* paramName,const char* paramValue);
  void PortConnect(const char* portName,const char* connectionExpr);
  void PortConnect(String portName,String connectionExpr);
  void PortConnectIndexed(const char* portFormat,int index,const char* connectionExpr);
  void PortConnectIndexed(const char* portFormat,int index,const char* connectFormat,int connectIndex);
  void PortConnectIndexed(const char* portFormat,int index,String connectionExpr);
  void EndInstance();
};

VAST* PushVAST(VASTType type,Arena* out);

VEmitter* StartVCode(Arena* out);
VAST* EndVCode(VEmitter* m);

void Repr(VAST* top,StringBuilder* b);
