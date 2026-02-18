#pragma once

#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "utils.hpp"
#include "accelerator.hpp"
#include "declaration.hpp"

/*
  NOTE: Very important. We do not want to have conditions affect the generated code that interfaces with the user. 
        We want to generate the same "stuff" regardless of what the value is.
        Trying to be clever and avoiding generating stuff because the value is not "needed"
        might cause problems because some symbol might be missing in the generated code 
        which causes the compiler to give out errors and that is not good.
        And forcing the user to disable certain sections or changing its code is not good.
        There is no problem in generating functions that are empty, structs that you know will not be used
        or enums that you know will not be needed. 
        Anything that belongs to the user interface must be generated, even if empty.

        For internal usage then it is different. At this point we do want to make sure that we only generate what
        we actually use in order to avoid compiler warnings, improve compilation speed and such.
*/

struct Accelerator;
struct InstanceInfo;
struct FUDeclaration;
struct VEmitter;

// Type can differ because of Merge.
struct SingleTypeStructElement{
  String type;
  String name;
  int arraySize; // Zero or one represent same thing: no array.
};

struct TypeStructInfoElement{
  Array<SingleTypeStructElement> typeAndNames; // Because of config sharing, we can have multiple elements occupying the same position. Type can also differ because of merge.
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

struct IndexInfo{
  bool isPadding;
  int pos;
  int amountOfEntries;
};

struct MuxInfo{
  int configIndex;
  int val;
  String name;
  String fullName;
};

struct SameMuxEntities{
  int configPos;
  InstanceInfo* info;
};

struct StructInfo;

struct StructElement{
  StructInfo* childStruct; // If nulltpr, then leaf

  // TODO: We could put a Type* here, so that later we can handle different sizes (char,short,int,etc).
  //       Instead of just having childStruct point to a StructInfo("iptr");

  String name;
  int localPos; // Inside the struct info that contains this element
  int size;
  bool isMergeMultiplexer;
  bool doesNotBelong; // Leads to padding being added. From merge 
};

inline u64 Hash(StructInfo* info);
inline bool operator==(StructInfo& l,StructInfo& r);

inline u64 Hash(StructElement x){
  u64 res = Hash(x.name) + 
            x.size + 
            x.localPos + 
            (x.isMergeMultiplexer ? 1 : 0) + 
            Hash(x.childStruct);
  
  return res;
}

inline bool operator==(StructElement lhs,StructElement rhs){
  bool res = *lhs.childStruct == *rhs.childStruct &&
             lhs.name == rhs.name &&
             lhs.size == rhs.size &&
             lhs.localPos == rhs.localPos &&
             lhs.isMergeMultiplexer == rhs.isMergeMultiplexer;

  return res;
}

struct StructInfo{
  String name;
  String originalName; // NOTE: kinda wathever, mainly used to generate generic address gens.

  // As such, it appears that the easiest way of doing stuff is to store struct info inside the InstanceInfo and not the other way around.
  
  ArenaDoubleList<StructElement>* memberList;
};

inline u64 Hash(StructInfo s){
  u64 res = 0;
  for(DoubleLink<StructElement>* ptr = s.memberList ? s.memberList->head : nullptr; ptr; ptr = ptr->next){
    res += Hash(ptr->elem);
  }
  res += Hash(s.name);
  return res;
}

inline u64 Hash(StructInfo* info){
  if(!info) { return 0;};
  return Hash(*info);
}

inline bool operator==(StructInfo& l,StructInfo& r){
  if(!(l.name == r.name)){
    return false;
  }
  
  DoubleLink<StructElement>* lPtr = l.memberList ? l.memberList->head : nullptr;
  DoubleLink<StructElement>* rPtr = r.memberList ? r.memberList->head : nullptr;
  for(; lPtr && rPtr; lPtr = lPtr->next,rPtr = rPtr->next){
    if(!(lPtr->elem == rPtr->elem)){
      return false;
    }
  }

  return true;
}

int GetIndex(VersatComputedValues val,VersatRegister reg);
Opt<int> GetOptIndex(VersatComputedValues val,VersatRegister reg);

// TODO: Maybe move this to a better place. Probably merge.hpp
struct AccelInfoIterator;
Array<Array<MuxInfo>> CalculateMuxInformation(AccelInfoIterator* iter,Arena* out);

String GlobalStaticWireName(StaticId id,Wire w,Arena* out);

void OutputCircuitSource(FUDeclaration* decl,FILE* file);
void OutputIterativeSource(FUDeclaration* decl,FILE* file);

// Versat_instance, all external memory files, makefile for pc-emul, basically everything that is only generated once.
// For the top instance and support files.
void OutputTopLevelFiles(Accelerator* accel,FUDeclaration* topLevelDecl,String hardwarePath,String softwarePath,VersatComputedValues val);

void OutputTestbench(FUDeclaration* decl,FILE* file);
