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
  Range<MathExpression*> port;
  Range<MathExpression*> delay;
};

struct Var{
  Token name;

  ConnectionExtra extra;
  Array<Range<MathExpression*>> index;

  bool IsArrayAccess(){return index.size > 0;}
  
  // TODO: Is this needed?
  //bool isArrayAccess;
};

struct VarGroup{
  Array<Var> vars;
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
  
  // NOTE: If array access, expressions is an array of the expressions in order and var contains the array name.
  SpecType type;
};

enum MathType{

};

struct MathExpression{
  Array<MathExpression*> expressions;
  union{
    const char* op;
    Var var;
    int val;
    Token name;
  };
  
  // NOTE: If array access, expressions is an array of the expressions in order and var contains the array name.
  SpecType type;
};


//nocheckin - TODO: We probably want to remove this after we move more logic to Env
Array<Token> AccumTokens(MathExpression* top,Arena* out);

struct VarDeclaration{
  Token name;
  Array<MathExpression*> arrayDims;
};

struct ParameterDeclaration{
  Token name;
  MathExpression* defaultValue;
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
  Array<Pair<String,MathExpression*>> parameters;

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
  MathExpression* arrayExpr;
  Token functionName;
  Array<MathExpression*> arguments;
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

  //int arraySize;
  Array<int> arrayDims;

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
  FUInstance* GetFUInstance(Token name,Array<int> arrayIndexIfArray);

  FUInstance* GetFUInstance(Var var);

  Entity* PushReservedEntity(String name);

  Entity* PushNewEntity(Token name);
  Entity* GetEntity(Token name);
  
  // TODO: Need to remove this. We want tokens so that we can do proper error report.
  Entity* GetEntity(String name);

  void CheckIfEntityExists(Token name);

#if 0
  - LEFT HERE - A Var is an "expression" inside a connect expression.
  -             A ConfigIdentifier is an "expression" inside a Config function. Mostly the lhs side of an assignemnt.
  -             A SpecExpression is also an expression and is mostly on the right side.
  -             Var is separate from the rest. It is the ConfigIdentifier and the SpecExpression that can be joined into the same flow.
  -             Anything that the lhs differs from the rhs representation must be handled afterwards.
  -             ConfigIdentifier is basically an "atom" of a SpecExpression that we created back when we where prototyping stuff and when the userConfig was distinct from a SpecExpression.
  -             If we can augment SpecExpression to represent the same data that a ConfigIdentifier can represent then we are golden. We can also do this in a more direct approach by making a ConfigIdentifier be the "atom" of the SpecExpression. Of course we first need to make sure that we can still reprent everything that we could beforehand.
  -             What even is a SpecExpression? 
  -               Connection stuff is handled by Var. 
  -               Equality does use SpecExpression.
  -               Everything else is MathExpression right?
  -             I'm almost positive that SpecExpression is only used for the Equality stuff.

  -             In fact we might even separate SpecExpression into SpecExpression,MathExpr and UserConfigExpr or something similar and simplify the type portion of the code. 
#endif
  
  Entity* GetEntity(ConfigIdentifier* id,Arena* out);
  Entity* GetEntity(MathExpression* id,Arena* out);

  Array<int> ConvertRangeToStart(Array<Range<MathExpression*>> range,Arena* out);
  Array<int> ConvertRangeToEnd(Array<Range<MathExpression*>> range,Arena* out);
  
  Array<int> ConvertRangeToIndex(Array<Range<MathExpression*>> range,Arena* out);

  Array<int> CalculateArraySize(Array<MathExpression*> exprs);
  int CalculateConstantExpression(MathExpression* top);

  void AddInput(VarDeclaration decl);
  void AddInstance(InstanceDeclaration decl,VarDeclaration var);

  void AddConnection(ConnectionDef def);
  void AddEquality(ConnectionDef def);

  void AddParam(Token name);
  void AddVariable(Token name);

  PortExpression InstantiateSpecExpression(SpecExpression* root);

  SYM_Expr SymbolicFromMathExpression(MathExpression* spec);
};

Env* StartEnvironment(Arena* freeUse,Arena* freeUse2);


// NOTE: Even thought the specs use closed intervals, all the values inside the iterators
//       are half intervals. Close on bottom and open on the top.
//       The StartIteration functions perform the fixup to make sure that everything lines up

struct DimIterator{
  Array<int> dim;
  Array<int> startValue;
  Array<int> current;

  int Size();
  void Invalidate();

  void Advance();
  bool IsValid();
  Array<int> Current();
};

DimIterator* StartIteration(Array<int> dims,Array<int> startValues,Arena* out);
DimIterator* StartIteration(int size,Arena* out);

void ArrayIndexIncrementInPlace(Array<int> dims,Array<int> startValue,Array<int> index);
int ArrayIndexToInteger(Array<int> dims,Array<int> index);
Array<int> IntegerToArrayIndex(Array<int> dims,int index,Arena* out);

struct FUInstanceIterator{
  Env* env;
  Entity* ent;

  bool used;
  DimIterator* iter;

  void Advance();
  bool IsValid();
  FUInstance* Current();
};

FUInstanceIterator StartIteration(Env* env,Entity* ent,Arena* out);

struct Connection{
  Token name;

  int port;
  int delay;
  Array<int> arrayIndex;
};

struct VarIterator{
  Token name;
  
  int startPort;
  int currentPort;
  int endPort;

  int startDelay;
  int currentDelay;
  int endDelay;

  DimIterator* arrayIndex;

  int Size();
  void Invalidate();

  void Advance();
  bool IsValid();
  Connection Current();
};

VarIterator* StartIteration(Env* env,Var var,Arena* out);

struct GroupIterator{
  Env* env;
  VarGroup* group;
  Array<VarIterator*> innerIters;
  int currentIter;

  int Size();
  void Invalidate();

  bool IsValid();
  void Advance();
  Connection Current();
};

GroupIterator IterateGroup(Env* env,VarGroup* group,Arena* out);
