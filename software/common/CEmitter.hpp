#pragma once

#include "utils.hpp"

// TODO: Need to decide, based on how the code flows better, whether the End* functions should be able to pop multiple levels until we pop the specific thing that we want to pop or whether we want to assert and make an error if we do not follow what is expected. Mostly depends on how the outside code is structured better, so far have not found a good example of this.

// A AST format for C code, does not need to be a full implementation of the C ast, only enough so we can do simple generation and printing.

enum CASTType{
  CASTType_TOP_LEVEL,
  CASTType_IF,
  CASTType_FUNCTION,
  CASTType_COMMENT,
  CASTType_STRUCT_DEF,
  CASTType_UNION_DEF,
  CASTType_MEMBER_DECL,
  CASTType_VAR_DECL, // No special ending, mostly for functions arguments right now, I think
  CASTType_VAR_DECL_STMT, // Ends in a ';'
  CASTType_VAR_BLOCK,
  CASTType_VAR_DECL_BLOCK,
  CASTType_FOREACH_BLOCK,
  CASTType_RAW_STATEMENT,
  CASTType_ENUM_DEF,
  CASTType_SWITCH_BLOCK,
  CASTType_CASE_BLOCK,
  CASTType_ELEM,
  CASTType_STATEMENT,
  CASTType_ASSIGNMENT
};

String Repr(CASTType type);

struct CAST;

struct CASTIf{
  // TODO: Probably should be an expression but we are not doing expressions right now.
  String ifExpression;
  ArenaList<CAST*>* statements;
};

struct CAST{
  CASTType type;

  union{
    String comment;
    String simpleStatement;
    String rawData;
    String content;
    
    struct{
      ArenaList<CAST*>* declarations;
      bool oncePragma;
    } top;

    struct{
      ArenaList<CASTIf>* ifExpressions;
      ArenaList<CAST*>* elseStatements; // If nullptr then no else clause
    } ifDecl;

    struct{
      String name;
      ArenaList<CAST*>* declarations;
    } structDef; // Also unions

    struct{
      String type;
      String iterName;
      String data;
      ArenaList<CAST*>* statements;
    } foreachDecl;
    
    struct{
      String name;
      ArenaList<Pair<String,String>>* nameAndValue;
    } enumDef;
    
    struct{
      String returnType;
      String functionName;
      ArenaList<CAST*>* arguments;
      ArenaList<CAST*>* statements; // Blocks, if nullptr then it is a function decl only.
    } funcDecl;

    struct{
      String type;
      String name;
      ArenaList<CAST*>* arrayElems;
      bool isStatic;
      bool isArray;
    } varDeclBlock; 

    struct{
      String expr;
      ArenaList<CAST*>* cases;
    } switchBlock;

    struct{
      String caseExpr;
      ArenaList<CAST*>* statements;
    } caseBlock;
    
    struct{
      ArenaList<CAST*>* varElems;
    } varBlock;
    
    // Assignment
    struct {
      String lhs;
      String rhs;
    } assign; 
    
    // Var decl
    struct {
      String typeName;
      String varName;
      String defaultValue;
      int arraySize;
      bool isExtern;
    } varDecl;
  };
};

enum CExprType{
  CExprType_VAR,
  CExprType_LITERAL,
  CExprType_OP
};

enum CExprOp{
  CExprOp_AND,
  CExprOp_GREATER,
  CExprOp_EQUALITY
};

struct CExpr{
  CExprType type;
  
  union{
    int lit;
    String var;

    struct{
      CExprOp op;
      Array<CExpr*> children; // Only set at the very end. We do not build the expression tree until the very end.
    };
  };
};

CAST* PushCAST(CASTType type,Arena* out);

struct CEmitter{
  Arena* arena;
  CAST* topLevel;
  Array<CAST*> buffer;
  int top;  
  ArenaList<CExpr>* expressionList;
  
  // Helper functions, not intended for outside usage
  void PushLevel(CAST* level);
  void PopLevel();
  CAST* FindFirstCASTType(CASTType type,bool errorIfNotFound = true);
  void InsertStatement(CAST* statementCAST);
  String EndExpressionAsString(Arena* out);
  
  void Once(); // Insert preprocessor directive so that file is only processed once
  void Include(String filename);
  void RawLine(String content);

  void Define(String name);
  void Define(String name,String content);

  void Line(); // Empty line
  
  void Struct(String structName = {});
  void Union(String unionName = {});
  void Member(String type,String memberName,int arraySize = 0);
  void EndStruct();

  void Enum(String name);
  void EnumMember(String name,String value = {});
  void EndEnum();
  
  void Extern(String typeName,String name);

  // Only declares a function, no body
  void FunctionDeclOnlyBlock(String returnType,String functionName);
  
  void FunctionBlock(String returnType,String functionName);
  void Argument(String type,String name);
  
  // TODO: Var block + Var element + EndBlock
  void VarDeclare(String type,String name,String initialValue = {});
  
  void ArrayDeclareBlock(String type,String name,bool isStatic = false);
  void VarDeclareBlock(String type,String name,bool isStatic = false);

  // Declaration of variable blocks (multiple elements inside a '{}');
  void VarBlock();
  void Elem(String value);
  void StringElem(String value);

  void ForEachBlock(String type,String iterName,String data);

  void If(String expression);
  void ElseIf(String expression);
  void Else();
  void EndIf();
  
  // To help build an if/else if chain, call this function only. This outputs an 'if' if no if is immediatly found or an 'else if' if the previous block is an if. Care when using this function to make an if else chain inside another if.
  // TODO: We could add a new type where we can have a bunch of conditions and statements associated to the conditions but when printing we would print an if/elseif chain.
  
  void IfOrElseIf(String expression);
  
  void IfFromExpression();
  void ElseIfFromExpression();
  void IfOrElseIfFromExpression();
  
  void SwitchBlock(String switchExpr);
  void CaseBlock(String caseExpr);
  
  void EndBlock(); // Any expression that starts a block is terminated by this function, (Function, If, Else, ...)

  // Kinda hacky way of building expressions. Give the expression as if writing it directly and end it by calling any function that ends with FromExpression;
  void StartExpression();
  void Var(String name);
  void Literal(int val);

  // Binary operations for expressions.
  void IsEqual();
  void And();
  void GreaterThan();
  
  void Comment(String expression);
  void Statement(String statement); // Generic statement, mostly to bypass any other logic and insert stuff directly.
  void Assignment(String lhs,String rhs);
  void Return(String varToReturn = {});
};

CEmitter* StartCCode(Arena* freeArena);
CAST* EndCCode(CEmitter* m);

String PushASTRepr(CEmitter* e,Arena* out,bool cppStyle = false,int startLevel = 0);

// TODO: cppStyle is just one styling choice, if we end up having more make a struct that is easier to pass around.
void Repr(CAST* top,StringBuilder* b,bool cppStyle = false,int level = 0);

