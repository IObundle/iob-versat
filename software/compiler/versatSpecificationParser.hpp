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
  Array<Pair<String,String>> parameters;
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

struct AddressGenForDef{
  Token loopVariable;
  Token start; // Can be a number or an input
  Token end;
};

struct IdOrNumber{
  union {
    Token identifier;
    int number;
  };
  bool isId;
};

struct AddressGenFor{
  Token loopVariable;
  IdOrNumber start;
  IdOrNumber end;
};

// A bit hardcoded for the moment but I just want something that works for now, still need more uses until we see how to progress the AddressGen code
enum AddressGenType{
  AddressGenType_MEM,
  AddressGenType_VREAD_LOAD,
  AddressGenType_VREAD_OUTPUT,
  AddressGenType_VWRITE_INPUT,
  AddressGenType_VWRITE_STORE
};

struct SymbolicExpression;

struct AddressGenDef{
  Array<Token> inputs;
  Array<AddressGenForDef> loops;
  SymbolicExpression* symbolic;
  SymbolicExpression* symbolicInternal;
  SymbolicExpression* symbolicExternal;
  Token externalName;
  AddressGenType type;
  String name;
};

struct AddressGenTerm{
  Opt<Token> loopVariable;
  Array<Token> constants;
};

struct AddressGen{
  
};

struct AddressGenLoopSpecificaton{
  String iterationExpression;
  String incrementExpression;
  String dutyExpression; // Non empty if it exists
  String nonPeriodIncrement;
  String nonPeriodEnd;
  String nonPeriodVal;
  bool isPeriodType;
};

struct AddressGenLoopSpecificatonSym{
  String periodExpression;
  String incrementExpression;

  String iterationExpression;
  String shiftExpression;

  String dutyExpression; // Non empty if it exists

  String shiftWithoutRemovingIncrement; // Shift as if period did not change addr. Useful for current implementation of VRead/VWrites
};

typedef Pair<HierarchicalName,HierarchicalName> SpecNode;

Array<Token> TypesUsed(TypeDefinition def,Arena* out);
Array<TypeDefinition> ParseVersatSpecification(String content,Arena* out);

FUDeclaration* InstantiateBarebonesSpecifications(String content,TypeDefinition def);
FUDeclaration* InstantiateSpecifications(String content,TypeDefinition def);

Opt<AddressGenDef> ParseAddressGen(Tokenizer* tok,Arena* out);
String InstantiateAddressGen(AddressGenDef def,Arena* out);
