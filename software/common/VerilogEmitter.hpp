#pragma once

#include "utils.hpp"

enum VASTType{
  VASTType_TOP_LEVEL,
  VASTType_MODULE_DECL,
  VASTType_PORT_DECL,
  VASTType_INSTANCE
};

String Repr(VASTType type);

struct VAST{
  VASTType type;

  union{

    // Top level
    struct{
      String timescaleExpression;
      ArenaList<VAST*>* declarations;
    };

    struct{
      String name;
      String bitSizeExpr;
      bool isInput; // We only support input or output, no inout.
    } portDecl;
    
    // Module Def
    struct{
      String name;
      ArenaList<Pair<String,String>>* parameters;
      ArenaList<VAST*>* portDeclarations;
    } module;
    
    // Instance
    struct {
      String moduleName;
      String name;
      ArenaList<Pair<String,String>>* parameters;
      ArenaList<Pair<String,String>>* portConnections;
    } instance;
  };
};

struct VEmitter{
  Arena* arena;
  VAST* topLevel;
  Array<VAST*> buffer;
  int top;  

  // Helper functions, do not use unless needed
  void PushLevel(VAST* level);
  void PopLevel();
  void InsertDeclaration(VAST* decl);
  VAST* FindFirstVASTType(VASTType type,bool errorIfNotFound = true);

  void Timescale(const char* timeUnit,const char* timePrecision);

  // Module definition
  void Module(String name);
  void DeclParam(const char* name,int value); // A global param of a module
  
  void Input(const char* name,int bitsize = 1);
  void Input(String name,int bitsize = 1);
  void IndexedInput(const char* format,int index,int bitsize = 1);
  void IndexedInput(const char* format,int index,const char* expression);

  void Output(const char* name,int bitsize = 1);
  void IndexedOutput(const char* format,int index,int bitsize = 1);
  void IndexedOutput(const char* format,int index,const char* expression);
  
  
  // Instantiating
  void StartInstance(const char* moduleName,const char* instanceName);
  void InstanceParam(const char* paramName,int paramValue);
  void PortConnect(const char* portName,const char* connectionExpr);
  void EndInstance();
};

VAST* PushVAST(VASTType type,Arena* out);

VEmitter* StartVCode(Arena* out);
VAST* EndVCode(VEmitter* m);

void Repr(VAST* top,StringBuilder* b,int level = 0);

