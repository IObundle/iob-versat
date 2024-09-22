#pragma once

#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "parser.hpp"
#include "debug.hpp"
#include "merge.hpp"

typedef Hashmap<String,FUInstance*> InstanceTable;
typedef Set<String> InstanceName;

enum ConnectionType{
  ConnectionType_SINGLE,
  ConnectionType_PORT_RANGE,
  ConnectionType_ARRAY_RANGE,
  ConnectionType_DELAY_RANGE,
  ConnectionType_ERROR
};

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

//typedef Array<Var> VarGroup;

struct VarGroup{
  Array<Var> vars;
  Token fullText;
};

struct SpecExpression{
  Array<SpecExpression*> expressions;
  union{
    const char* op;
    Var var;
    Value val;
  };
  Token text;
  
  enum {UNDEFINED,OPERATION,VAR,LITERAL} type;
};

struct VarDeclaration{
  Token name;
  int arraySize;
  bool isArray;
};

struct GroupIterator{
  VarGroup group;
  int groupIndex;
  int varIndex; // Either port, delay or array unit.
};

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

  Array<Token> transforms;
  
  union{
    VarGroup input;
    SpecExpression* expression;
  };
};

struct TypeAndInstance{
  Token typeName;
  Token instanceName;
};

struct DefBase{
  Token name;
};

struct ModuleDef : public DefBase{
  Token numberOutputs; // TODO: Not being used. Not sure if we gonna actually add this or not.
  Array<VarDeclaration> inputs;
  Array<InstanceDeclaration> declarations;
  Array<ConnectionDef> connections;
};

struct TransformDef : public DefBase{
  Array<VarDeclaration> inputs;
  Array<ConnectionDef> connections;
};

struct MergeDef : public DefBase{
  Array<TypeAndInstance> declarations;
  Array<SpecificMergeNode> specifics;
};

enum DefinitionType{
  DefinitionType_MODULE,
  DefinitionType_MERGE,
  DefinitionType_ITERATIVE
};

struct TypeDefinition{
  DefinitionType type;
  union {
    DefBase base;
    ModuleDef module;
    MergeDef merge;
  };
};

struct Transformation{
  int inputs;
  int outputs;
  Array<int> map;
};

struct HierarchicalName{
  Token instanceName;
  Var subInstance;
};

typedef Pair<HierarchicalName,HierarchicalName> SpecNode;

Array<Token> TypesUsed(TypeDefinition def,Arena* out,Arena* temp);
Array<TypeDefinition> ParseVersatSpecification(String content,Arena* out,Arena* temp);
void InstantiateSpecifications(String content,TypeDefinition def,Arena* temp,Arena* temp2);
