#pragma once

#include "parser.hpp"

struct FUDeclaration;
struct SymbolicExpression;

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
  UserConfigurationType_CONFIG,
  UserConfigurationType_MEM,
  UserConfigurationType_STATE
};

struct ConfigIdentifier{
  Token name;
  Token wire;
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

struct ConfigStatement{
  // TODO: Union
  ConfigIdentifier lhs; // We currently only support <name>.<wire> assignments. NOTE: But I think that we eventually need to support N names and one wire.
  Array<Token> rhs;

  // TODO: Union
  ConfigRHSType type;
  SymbolicExpression* expr;
  ConfigIdentifier rhsId;
  FunctionInvocation* func;
};

struct ConfigVarDeclaration{
  Token name;

  // TODO: Not implemented, just parsed currently.
  int arraySize;
  bool isArray;
};

struct ConfigFunctionDef{
  UserConfigType type;
  
  String name;
  Array<ConfigVarDeclaration> variables;
  Array<ConfigStatement*> statements;
};

ConfigFunctionDef* ParseConfigFunction(Tokenizer* tok,Arena* out);
bool IsNextTokenConfigFunctionStart(Tokenizer* tok);

// ============================================================================
// Instantiation and manipulation

struct ConfigAssignment{
  String lhs;
  SymbolicExpression* rhs;
  String rhsId;
};

struct ConfigFunction{
  String individualName;
  String fullName;
  FUDeclaration* decl; // Every config function is associated to one declaration.
  Array<ConfigAssignment> assignments;
  Array<String> variables;
  Array<String> newStructs;
  String structToReturnName;
};

// Can fail (parsed data is validated in here)
ConfigFunction* InstantiateConfigFunction(ConfigFunctionDef* def,FUDeclaration* declaration);
