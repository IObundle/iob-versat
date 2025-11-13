#pragma once

#include "parser.hpp"

struct FUDeclaration;

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

struct ConfigIdentifier{
  Token name;
  Token wire;
};

struct ConfigStatement{
  // TODO: Union
  ConfigIdentifier lhs; // We currently only support <name>.<wire> assignments
  Array<Token> rhs;
};

struct ConfigVarDeclaration{
  Token name;

  // TODO: Not implemented, just parsed currently.
  int arraySize;
  bool isArray;
};

struct ConfigFunctionDef{
  String name;
  Array<ConfigVarDeclaration> variables;
  Array<ConfigStatement*> statements;
};

ConfigFunctionDef* ParseConfigFunction(Tokenizer* tok,Arena* out);
bool IsNextTokenConfigFunctionStart(Tokenizer* tok);

// ============================================================================
// Instantiation and manipulation

struct ConfigFunction{
  FUDeclaration* decl; // Every config function is associated to one declaration.
};

// Can fail (parsed data is validated in here)
ConfigFunction* InstantiateConfigFunction(ConfigFunctionDef* def,FUDeclaration* declaration);
