#pragma once

#include "utils.hpp"

struct Versat;
struct Accelerator;
struct FUDeclaration;
struct Options;

// Type can differ because of Merge.
struct SingleTypeStructElement{
  String type;
  String name;
};

struct TypeStructInfoElement{
  Array<SingleTypeStructElement> typeAndNames; // Because of config sharing, we can have multiple elements occupying the same position.
};

struct TypeStructInfo{
  String name;
  Array<TypeStructInfoElement> entries;
};

Array<FUDeclaration*> SortTypesByConfigDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp);

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2);
void OutputIterativeSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2);
void OutputVerilatorWrapper(Versat* versat,FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2);
void OutputVerilatorMake(Versat* versat,String topLevelName,String versatDir,Options* opts,Arena* temp,Arena* temp2);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2);
