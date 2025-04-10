#pragma once

#include "utils.hpp"
#include "embeddedData.hpp"

// TODO: We are probably gonna go the route of also creating a Verilog emitter. Templates seem fine at the start but honestly they are more trouble than they are worth.

// A AST format for C code, does not need to be a full implementation of the C ast, only enough so we can do simple generation and printing.

// TODO: If using this C emitter makes things much easier at generating C code, then it would probably be best to start using it in the embedData tool. Just need to make sure that there is no conflict in build time or some form of dependency hell that arises from this. We want tools like embedData to compile and work correctly even if Versat does not compile.
enum CASTType{
  CASTType_TOP_LEVEL,
  CASTType_IF,
  CASTType_FUNCTION,
  CASTType_COMMENT,
  CASTType_VAR_DECL,
  CASTType_VAR_DECL_STMT,
  CASTType_ASSIGNMENT
};

String Repr(CASTType type);
bool IsBlockType(CASTType type);

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
    
    // Top level
    struct{
      ArenaList<CAST*>* functions;
    };

    // If 
    struct{
      ArenaList<CASTIf>* ifExpressions;
      ArenaList<CAST*>* elseStatements; // If nullptr then no else clause
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

  void Function(String returnType,String functionName);
  void Argument(String type,String name);
  void Declare(String type,String name,String initialValue = {});

  void Comment(String expression);
  
  void If(String expression);
  void ElseIf(String expression);
  void Else();
  void EndIf();
  
  void EndBlock(); // Any expression that starts a block is terminated by this function, (Function, If, Else, ...)
  
  void Assignment(String lhs,String rhs);
};

CEmitter* StartCCode(Arena* out);
CAST* EndCCode(CEmitter* m);

void Repr(CAST* top,StringBuilder* b,int level = 0);
