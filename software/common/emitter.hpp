#pragma once

#include "utils.hpp"

// TODO: We are probably gonna go the route of also creating a Verilog emitter. Templates seem fine at the start but honestly they are more trouble than they are worth.

// A AST format for C code, does not need to be a full implementation of the C ast, only enough so we can do simple generation and printing.

// TODO: If using this C emitter makes things much easier at generating C code, then it would probably be best to start using it in the embedData tool. Just need to make sure that there is no conflict in build time or some form of dependency hell that arises from this. We want tools like embedData to compile and work correctly even if Versat does not compile.
enum CASTType{
  CASTType_TOP_LEVEL,
  CASTType_IF,
  CASTType_FUNCTION,
  CASTType_COMMENT,
  CASTType_STRUCT_DEF,
  CASTType_MEMBER_DECL,
  CASTType_VAR_DECL,
  CASTType_VAR_DECL_STMT,
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
    String statement;
    
    // Top level
    struct{
      String pad1;
      String pad2;
      ArenaList<CAST*>* declarations_pad0;
    };

    // If 
    struct{
      ArenaList<CASTIf>* ifExpressions;
      ArenaList<CAST*>* elseStatements; // If nullptr then no else clause
    };

    // Struct Def
    struct{
      String name;
      String pad0;
      ArenaList<CAST*>* declarations;
    };
    
    // Function decl
    struct{
      String returnType;
      String functionName;
      ArenaList<CAST*>* arguments;
      ArenaList<CAST*>* statements; // Blocks
    };

    // Assignment
    struct {
      String lhs;
      String rhs;
    }; 
    
    // Var decl
    struct {
      String typeName;
      String varName;
      String defaultValue;
    };
  };
};

CAST* PushCAST(CASTType type,Arena* out);

struct CEmitter{
  Arena* arena;
  CAST* topLevel;
  Array<CAST*> buffer;
  int top;  

  // Helper functions, not intended for outside usage
  void PushLevel(CAST* level);
  void PopLevel();
  CAST* FindFirstCASTType(CASTType type,bool errorIfNotFound = true);
  void InsertStatement(CAST* statementCAST);
  void InsertDeclaration(CAST* declarationCAST);
  
  void Struct(String structName);
  void Member(String type,String memberName);
  
  void Function(String returnType,String functionName);
  void Argument(String type,String name);
  void VarDeclare(String type,String name,String initialValue = {});

  void If(String expression);
  void ElseIf(String expression);
  void Else();
  void EndIf();
  
  void EndBlock(); // Any expression that starts a block is terminated by this function, (Function, If, Else, ...)
  
  void Comment(String expression);
  void Statement(String statement); // Generic statement, mostly to bypass any other logic and insert stuff directly.
  void Assignment(String lhs,String rhs);
  void Return(String varToReturn = {});
};

CEmitter* StartCCode(Arena* out);
CAST* EndCCode(CEmitter* m);

void Repr(CAST* top,StringBuilder* b,int level = 0);
