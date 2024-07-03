#pragma once

#include "utils.hpp"

struct Accelerator;
struct FUDeclaration;

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

struct Difference{
  int index;
  int newValue;
};

struct DifferenceArray{
  int oldIndex;
  int newIndex;

  Array<Difference> differences;
};

Array<FUDeclaration*> SortTypesByConfigDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp);

void OutputCircuitSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2);
void OutputIterativeSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2);
void OutputVerilatorWrapper(FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2);
void OutputVerilatorMake(String topLevelName,String versatDir,Arena* temp,Arena* temp2);
void OutputVersatSource(Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2);
