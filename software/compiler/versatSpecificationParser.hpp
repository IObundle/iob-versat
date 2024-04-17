#pragma once

#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "parser.hpp"
#include "debug.hpp"
#include "merge.hpp"

// TODO: Remove Connection extra. Store every range inside Var.
struct ConnectionExtra{
  Range<int> port;
  Range<int> delay;
};

struct Var{
  Token name;
  ConnectionExtra extra;
  Range<int> index;
  bool isArrayAccess;
};

struct VarDeclaration{
  Token name;
  int arraySize;
  bool isArray;
};

typedef Hashmap<String,FUInstance*> InstanceTable;
typedef Set<String> InstanceName;
typedef Array<Var> VarGroup;

struct PortExpression{
  FUInstance* inst;
  ConnectionExtra extra;
};

struct InstanceDeclaration{
  enum {NONE,STATIC,SHARE_CONFIG} modifier;
  Token typeName;
  Array<VarDeclaration> declarations;
  String parameters;
};

struct ConnectionDef{
  Range<Cursor> loc;

  VarGroup output;
  enum {EQUALITY,CONNECTION} type;

  union{
    VarGroup input;
    Expression* expression;
  };
};

struct ModuleDef{
  Token name;
  Array<VarDeclaration> inputs;
  Array<InstanceDeclaration> declarations;
  Array<ConnectionDef> connections;
};
