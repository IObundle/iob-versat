#pragma once

#include "merge.hpp"

#include "embeddedData.hpp"

struct ConfigFunctionDef;
struct SpecExpression;

typedef TrieMap<String,FUInstance*> InstanceTable;

enum PortRangeType{
  PortRangeType_SINGLE,
  PortRangeType_PORT_RANGE,
  PortRangeType_ARRAY_RANGE,
  PortRangeType_DELAY_RANGE,
  PortRangeType_ERROR
};

struct ConnectionExtra{
  Range<SpecExpression*> port;
  Range<SpecExpression*> delay;
};

struct Var{
  Token name;

  ConnectionExtra extra;
  Range<SpecExpression*> index;

  bool isArrayAccess;
};

struct VarGroup{
  Array<Var> vars;
  Token fullText;
};

enum SpecType{
  SpecType_OPERATION,
  SpecType_VAR,
  SpecType_NAME,
  SpecType_LITERAL,
  SpecType_SINGLE_ACCESS,
  SpecType_ARRAY_ACCESS,
  SpecType_FUNCTION_CALL
};

struct SpecExpression{
  Array<SpecExpression*> expressions;
  union{
    const char* op;
    Var var;
    int val;
    Token name;
  };
  Token text;
  
  // NOTE: If array access, expressions is an array of the expressions in order and var contains the array name.
  SpecType type;
};

//nocheckin - TODO: We probably want to remove this after we move more logic to Env
Array<Token> AccumTokens(SpecExpression* top,Arena* out);

struct VarDeclaration{
  Token name;
  Array<SpecExpression*> arrayDims;
};

struct ParameterDeclaration{
  Token name;
  SpecExpression* defaultValue;
};

struct PortExpression{
  FUInstance* inst;
  ConnectionExtra extra;
};

enum InstanceDeclarationType{
  InstanceDeclarationType_NONE = 0,
  InstanceDeclarationType_STATIC = 1,
  InstanceDeclarationType_SHARE_CONFIG = 2
};

struct InstanceDeclaration{
  InstanceDeclarationType modifier;
  Token typeName;
  Array<VarDeclaration> declarations; // share(config) groups can have multiple different declarations. TODO: It is kinda weird that inside the syntax, the share allows groups of instances to be declared while this does not happen elsewhere. Not enought to warrant a look for now, but keep in mind for later.

  // NOTE: We could create a different expression type 
  Array<Pair<String,SpecExpression*>> parameters;

  Array<Token> addressGenUsed; // NOTE: We do not check if address gen exists at parse time, we check it later.
  Array<Token> shareNames;
  bool negateShareNames;
  bool debug;
  
  // Set later
  int shareIndex;
};

enum ConnectionType{
  ConnectionType_NONE,
  ConnectionType_EQUALITY,
  ConnectionType_CONNECTION
};

struct ConnectionDef{
  ConnectionType type;
  VarGroup output;
  Array<Token> transforms;

  // TODO: Union.
  VarGroup input;
  SpecExpression* expression;
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
  Array<ParameterDeclaration> params;
  Array<VarDeclaration> inputs;
  Array<InstanceDeclaration> declarations;
  Array<ConnectionDef> connections;
  Array<ConfigFunctionDef> configs;
};

struct MergeDef : public DefBase{
  Array<TypeAndInstance> declarations;
  Array<SpecificMergeNode> specifics;
  Array<Token> mergeModifiers;
};

struct ConstructDef{
  ConstructType type;
  union {
    DefBase base;
    ModuleDef module;
    MergeDef merge;
  };
};

struct HierarchicalName{
  Token instanceName;
  Var subInstance;
};

typedef Pair<HierarchicalName,HierarchicalName> SpecNode;

bool IsModuleLike(ConstructDef def);
Array<Token> TypesUsed(ConstructDef def,Arena* out);

Array<ConstructDef> ParseVersatSpecification(String content,Arena* out);

// TODO: Move this function to a better place, no reason to be inside spec parser
FUDeclaration* InstantiateSpecifications(String content,ConstructDef def);


// nocheckin Move to a better place after we probably remove the userConfigs.hpp file.
// TODO: We cannot represent an array access followed by a wireOrPort access.
//       This is probably not the best way to proceed.
enum ConfigAccessType{
  ConfigAccessType_BASE,
  ConfigAccessType_ACCESS,
  ConfigAccessType_ARRAY,
  ConfigAccessType_FUNC_CALL
};

#if 0
A ConfigIdentifier is basically just an "atom" of a SpecExpression
It implements stuff that the SpecExpression atom does not implement, I think.
But realistically it is basically just a special case of an atom.


#endif

// TODO: Probably rename this.
// ConfigIdentifier is parsed in reverse order than expected. Name appears first and access expressions appear after.
struct ConfigIdentifier{
  ConfigAccessType type;

  ConfigIdentifier* parent;

  // TODO: Union
  Token name;
  SpecExpression* arrayExpr;
  Token functionName;
  Array<SpecExpression*> arguments;
};

inline ConfigIdentifier* GetBase(ConfigIdentifier* top){
  return top;
}

inline ConfigIdentifier* GetBeforeBase(ConfigIdentifier* top){
  if(top){
    return top->parent;
  }
  return nullptr;
}

// ======================================
// Hierarchical access (WIP)

// Map from name in hierarchical access (ex: a.b.c[0].d) to an entity. The VARIABLE part is that this mapping is also used inside the parser to map stuff to things like variables or to special names that are specific to a given part of the code.

enum EntityType{
  EntityType_FU,
  EntityType_FU_ARRAY,
  EntityType_NODE,
  EntityType_PARAM,
  EntityType_MEM_PORT, // User can "represent" a memory port by doing something like mem.in0 (input port 0).
  EntityType_CONFIG_WIRE,
  EntityType_STATE_WIRE,
  EntityType_CONFIG_FUNCTION,
  EntityType_VARIABLE_INPUT,
  EntityType_VARIABLE_SPECIAL // For variables that exist "by default"
};

bool IsEntitySubType(EntityType type);

enum VariableType{
  VariableType_VOID_PTR,
  VariableType_INTEGER
};

struct Entity{
  EntityType type;

  // TODO: We might want to also store the token associated to the entity in question.

  // Virtual port
  Direction dir;
  int port;
  Entity* parent;
  
  // TODO: Union
  //union {
  FUInstance* instance;

  bool isInput;

  Wire* wire;

  ConfigFunction* func;

  int arraySize;
  //Array<int> arrayDims;

  union {
    String varName;
    String arrayBaseName;
    String paramName;
  }; 

  VariableType varType;
  //};
};

enum ScopeType{
  ScopeType_CONFIG_FUNCTION,
  ScopeType_FOR_LOOP
};

struct EnvScope{
  ArenaMark mark;

  //TrieMap<String,VariableType>* variableTypes;
  TrieMap<String,Entity>* variable;
};

// Env is more of a parser related thing than it is an accelerator related thing.
// Need to copy this to a better place and start using it in other parts of the code that should use it.
// NOTE: This is more like a FUDeclaration builder than an environment, I think
// NOTE: Make the easy changes first and then see what happens.
struct Env{
  Arena* scopeArena;
  Arena* miscArena;

  ArenaList<String>* errors;
  Accelerator* circuit;

  InstanceTable* table;

  int insertedInputs;

  Array<EnvScope*> scopes;
  int currentScope;

  void ReportError(Token badToken,String msg);

  // By default we are inside a module scope.
  void PushScope();
  void PopScope();

  FUInstance* CreateInstance(FUDeclaration* type,String name);
  FUInstance* CreateFUInstanceWithDeclaration(FUDeclaration* type,String name,InstanceDeclaration decl);

  // TODO: The arrayIndexIfArray does not tell us if we are trying to access an array or not.
  //       We probably need to encode such info so that we can properly error report
  FUInstance* GetFUInstance(Token name,int arrayIndexIfArray);

  FUInstance* GetFUInstance(Var var);
  FUInstance* GetOutputInstance();

  Entity* PushNewEntity(Token name);
  Entity* GetEntity(Token name);
  Entity* GetEntity(String name);

  void CheckIfEntityExists(Token name);
  
  Entity* GetEntity(ConfigIdentifier* id,Arena* out);
  Entity* GetEntity(SpecExpression* id,Arena* out);
  
  Array<int> CalculateArraySize(Array<SpecExpression*> exprs);
  int CalculateConstantExpression(SpecExpression* top);

  void AddInput(VarDeclaration decl);
  void AddInstance(InstanceDeclaration decl,VarDeclaration var);

  void AddConnection(ConnectionDef def);
  void AddEquality(ConnectionDef def);

  void AddParam(Token name);
  void AddVariable(Token name);

  PortExpression InstantiateSpecExpression(SpecExpression* root);

  SYM_Expr SymbolicFromSpecExpression(SpecExpression* spec);
};

Env* StartEnvironment(Arena* freeUse,Arena* freeUse2);

struct FUInstanceIterator{
  Env* env;
  Entity* ent;
  int index;
  int max;

  FUInstanceIterator Next();
  bool IsValid();
  FUInstance* Current();
};

struct GroupIterator{
  Env* env;
  VarGroup group;
  int groupIndex;
  int varIndex; // Either port, delay or array unit.
};

FUInstanceIterator StartIteration(Env* env,Entity* ent);
