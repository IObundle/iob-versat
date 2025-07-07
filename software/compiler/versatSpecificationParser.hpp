#pragma once

#include "symbolic.hpp"
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

enum InstanceDeclarationType{
  InstanceDeclarationType_NONE,
  InstanceDeclarationType_STATIC,
  InstanceDeclarationType_SHARE_CONFIG
};

// TODO: It's kinda bad to group stuff this way. Just because the spec parser groups everything, does not mean that we need to preserve the grouping. We could just parse once and create N different instance declarations for each instance (basically flattening the declarations) instead of preserving this grouping and probably simplifying stuff a bit.
//       The problem is that we would have to put some sharing logic inside the parser. Idk. Push through and see later if any change is needed.
struct InstanceDeclaration{
  InstanceDeclarationType modifier;
  Token typeName;
  Array<VarDeclaration> declarations; // share(config) groups can have multiple different declarations. TODO: It is kinda weird that inside the syntax, the share allows groups of instances to be declared while this does not happen elsewhere. Not enought to warrant a look for now, but keep in mind for later.
  Array<Pair<String,String>> parameters;
  Array<Token> addressGenUsed;
  Array<Token> shareNames;
  bool negateShareNames;
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

struct MergeDef : public DefBase{
  Array<TypeAndInstance> declarations;
  Array<SpecificMergeNode> specifics;
  Array<Token> mergeModifiers;
};

enum ConstructType{
  ConstructType_MODULE,
  ConstructType_MERGE,
  ConstructType_ITERATIVE,
  ConstructType_ADDRESSGEN
};

struct AddressGenForDef{
  Token loopVariable;
  SymbolicExpression* start;
  SymbolicExpression* end;
};

struct AddressGenDef{
  AddressGenType type;
  String name;
  Array<Token> inputs;
  Array<AddressGenForDef> loops;
  SymbolicExpression* symbolic;
};

struct ConstructDef{
  ConstructType type;
  union {
    DefBase base;
    ModuleDef module;
    MergeDef merge;
    AddressGenDef addressGen;
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

struct SymbolicExpression;

typedef Pair<HierarchicalName,HierarchicalName> SpecNode;

Array<Token> TypesUsed(ConstructDef def,Arena* out);
Array<ConstructDef> ParseVersatSpecification(String content,Arena* out);

FUDeclaration* InstantiateBarebonesSpecifications(String content,ConstructDef def);
FUDeclaration* InstantiateSpecifications(String content,ConstructDef def);

Opt<AddressGenDef> ParseAddressGen(Tokenizer* tok,Arena* out);
