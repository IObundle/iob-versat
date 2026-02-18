#pragma once

#include "parser.hpp"
#include "addressGen.hpp"

// TODO: Remove this.
#include "versatSpecificationParser.hpp"

struct FUDeclaration;
struct SymbolicExpression;
struct AddressAccess;
struct Parser;

// Move user configuration functions here.

// ============================================================================
// Parsed structs only (not validated and no strings is saved)

enum ConfigExpressionType{
  ConfigExpressionType_IDENTIFIER,
  ConfigExpressionType_NUMBER
};

struct ConfigExpression{
  ConfigExpressionType type;

  Token identifier;
  Token access; // Only single access supported, do not see why would need more than one.

  // TODO: Union
  ConfigExpression* child;
  int number;
};

// TODO: While this exists, we do not want to have different flow if possible. I think that it should be possible to describe the data in such a way that we do not have to make this distinction.
enum UserConfigType{
  UserConfigType_NONE,
  UserConfigType_CONFIG,
  UserConfigType_MEM,
  UserConfigType_STATE
};

enum ConfigRHSType{
  ConfigRHSType_SYMBOLIC_EXPR,
  ConfigRHSType_FUNCTION_CALL,
  ConfigRHSType_IDENTIFIER
};

struct FunctionInvocation{
  String functionName;
  Array<Token> arguments; // TODO: For now we do not allow any expression. Only simple assignments.
};

enum ConfigStatementType{
  ConfigStatementType_FOR_LOOP,
  ConfigStatementType_STATEMENT
};

struct ConfigStatement{
  ConfigStatementType type;

  // TODO: Union
  ConfigIdentifier* lhs; // We currently only support <name>.<wire> assignments. NOTE: But I think that we eventually need to support N names and one wire.
  Array<Token> rhs;

  // TODO: Union
#if 0
  ConfigRHSType rhsType;
  SymbolicExpression* expr;
  ConfigIdentifier* rhsId;
  FunctionInvocation* func;
#endif

  SpecExpression* trueRhs;

  //AddressGenForDef def;
  AddressGenForDef2 def2;
  Array<ConfigStatement*> childs; // Only for loops contains these right now.
};

enum ConfigVarType{
  ConfigVarType_SIMPLE,
  ConfigVarType_ADDRESS,
  ConfigVarType_FIXED,
  ConfigVarType_DYN
};

struct ConfigVarDeclaration{
  Token name;

  // TODO: Not implemented, just parsed currently.
  
  //ConfigVarType type;
  Token type;
  int arraySize;
  bool isArray;
};

struct ConfigFunctionDef{
  UserConfigType type;
  
  Token name;
  Array<ConfigVarDeclaration> variables;
  Array<ConfigStatement*> statements;
  bool debug;
};

bool IsNextTokenConfigFunctionStart(Parser* parser);
bool IsNextTokenConfigFunctionStart(Tokenizer* tok);

// ============================================================================
// Instantiation and manipulation

// TODO: We want to remove this. Stupid to force the user to have to provide a switch when we can just figure out on our side. Still need to take care about the differences between config/mem and state.
enum ConfigFunctionType{
  ConfigFunctionType_CONFIG,
  ConfigFunctionType_STATE,
  ConfigFunctionType_MEM
};

enum TransferDirection{
  TransferDirection_NONE = 0,
  TransferDirection_READ = 1,
  TransferDirection_WRITE = 2
};

struct FunctionMemoryTransfer{
  TransferDirection dir;

  String sizeExpr;
  String variable;
  String entityName;
};

enum ConfigStuffType{
  ConfigStuffType_MEMORY_TRANSFER,
  ConfigStuffType_ADDRESS_GEN,
  ConfigStuffType_ASSIGNMENT
};

// TODO: We probably want to remove this and just pass everything into ConfigStuff.
struct ConfigAssignment{
  String lhs;
  SymbolicExpression* rhs;
  String rhsId;
  
  String special;
};

struct ConfigStuff{
  ConfigStuffType type;

  // TODO: This lhs is only for access. Need to join stuff with assign and access if we eventually cleanup the code.
  String lhs;
  String accessVariableName;

  String nameOfLeftEntity;

  union{
    ConfigAssignment assign;
    FunctionMemoryTransfer transfer;
    AccessAndType access;
  };
};

struct ConfigVariable{
  ConfigVarType type;
  String name;
  bool usedOnLoopExpressions;
};

struct ConfigFunction{
  ConfigFunctionType type;

  FunctionMemoryTransfer transfer;

  String individualName;
  String fullName;
  FUDeclaration* decl; // Every config function is associated to one declaration only.

  Array<ConfigStuff> stuff;
  Array<ConfigVariable> variables;
  Array<String> newStructs;
  String structToReturnName;

  bool supportsSizeCalc;

  // TODO: This is only worth it if we can generate extra stuff that might help figure out the problems. Otherwise just use gdb.
  // NOTE: Some stuff that we might want is to profile (gather statistics data) and stuff like that that we can then report at the end of a run.
  bool debug;
};
 
// Can fail (parsed data is validated in here)
// TODO: Instead of passing the content, it would be easier if the function was capable of reporting the errors without having to accesss the text, just by storing the relevant tokens and the upper parts of the code is responsible for reporting it. The language is not that complicated meaning that we are free to just define all the possible errors in a large enum and having a simple structure that stores all the relevant data. 
ConfigFunction* InstantiateConfigFunction(Env* env,ConfigFunctionDef* def,FUDeclaration* declaration,String content,Arena* out);

