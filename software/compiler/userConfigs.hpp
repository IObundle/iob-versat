#pragma once

#include "addressGen.hpp"
#include "hierName.hpp"

// TODO: Remove this.
#include "versatSpecificationParser.hpp"

struct FUDeclaration;
struct AddressAccess;

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
  ConfigStatementType_EQUALITY,
  ConfigStatementType_FUNCTION_CALL
};

inline bool IsLeaf(ConfigStatementType type){ return (type == ConfigStatementType_EQUALITY || type == ConfigStatementType_FUNCTION_CALL);}

struct ConfigStatement{
  ConfigStatementType type;

  // Why have a ConfigIdentifier and a SpecExpression?

  // TODO: Union
  ConfigIdentifier* lhs;
  SpecExpression* rhs;
  AddressGenForDef def;
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

  SYM_Expr size;
  String variable;
  HIER_Name name;
};

enum ConfigStuffType{
  ConfigStuffType_MEMORY_TRANSFER,
  ConfigStuffType_ADDRESS_GEN,
  ConfigStuffType_ASSIGNMENT
};

// TODO: We probably want to remove this and just pass everything into ConfigStuff.
struct ConfigAssignment{
  String lhs;
  SYM_Expr rhs;
  String rhsId;
  
  String special;
};

// nocheckin: 
inline Array<String> Add(Array<String> in,String toAdd,Arena* out){
  Array<String> res = PushArray<String>(out,in.size + 1);
  for(int i = 0; i < in.size; i++){
    res[i] = in[i];
  }
  res[in.size] = toAdd;
  return res;
}

inline Array<String> Add(String toAdd,Array<String> in,Arena* out){
  Array<String> res = PushArray<String>(out,in.size + 1);
  res[0] = toAdd;
  for(int i = 0; i < in.size; i++){
    res[i+1] = in[i];
  }
  return res;
}

struct ConfigStuff{
  ConfigStuffType type;

  // TODO: This lhs is only for access. Need to join stuff with assign and access if we eventually cleanup the code.
  HIER_Name lhs;

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

  String individualName;
  String fullName;
  FUDeclaration* decl; // Every config function is associated to one declaration only.

  Array<ConfigStuff> stuff;
  Array<ConfigVariable> variables;
  String structToReturnName;
  String stateStructContent;

  bool supportsSizeCalc;

  // TODO: This is only worth it if we can generate extra stuff that might help figure out the problems. Otherwise just use gdb.
  // NOTE: Some stuff that we might want is to profile (gather statistics data) and stuff like that that we can then report at the end of a run.
  bool debug;
};
 
// Can fail (parsed data is validated in here)
// TODO: Instead of passing the content, it would be easier if the function was capable of reporting the errors without having to accesss the text, just by storing the relevant tokens and the upper parts of the code is responsible for reporting it. The language is not that complicated meaning that we are free to just define all the possible errors in a large enum and having a simple structure that stores all the relevant data. 
ConfigFunction* InstantiateConfigFunction(Env* env,ConfigFunctionDef* def,FUDeclaration* declaration,String content,Arena* out);

