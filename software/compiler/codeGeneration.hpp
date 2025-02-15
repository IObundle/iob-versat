
#pragma once

#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "utils.hpp"

static const int DELAY_SIZE = 7;

struct Accelerator;
struct InstanceInfo;
struct FUDeclaration;

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
  InstanceInfo* info;
};

struct WireInformation{
  Wire wire;
  int addr;
  int configBitStart;
  bool isStatic;
};

struct SameMuxEntities{
  int configPos;
  InstanceInfo* info;
};

template<> struct std::hash<SameMuxEntities>{
   std::size_t operator()(SameMuxEntities const& s) const noexcept{
     std::size_t res = s.configPos;
     return res;
   }
};

static bool operator==(const SameMuxEntities i0,const SameMuxEntities i1){
  bool res = (i0.configPos == i1.configPos);
  return res;
}

struct AddressGenDef;
extern Pool<AddressGenDef> savedAddressGen;

// In order to get the names, I need to associate a given member to a merge index.
// Since multiplexers can be shared accross multiple merge indexes, I need some map of some sorts.
// One multiplexer can be used by dozens of merge indexes.
struct StructInfo;

struct StructElement{
  StructInfo* childStruct; // If nulltpr, then leaf (whose type is type)
  // TODO: We could put a Type* here, so that later we can handle different sizes (char,short,int,etc).
  //       Instead of just having childStruct point to a StructInfo("iptr");
  String name;
  int pos; // This is local pos (inside the struct).
  int size;
  bool isMergeMultiplexer;
  //bool isPadding;
};

struct StructInfo{
  String name;
  FUDeclaration* type; // TODO: isSimpleType is intended to replace this. We treat simple units the same way, where each element points to a StructElement. The most basic struct element is gonna be the iptr, which is the leaf, although we still do not have a proper way of representing this in the codebase. Check TODO inside StructElement.
  StructInfo* parent;
  bool isSimpleType;
  
  ArenaDoubleList<StructElement>* list;
};

size_t HashStructInfo(StructInfo* info);

template<> struct std::hash<StructElement>{
  std::size_t operator()(StructElement const& s) const noexcept{
    std::size_t res = std::hash<String>()(s.name) + s.size + (s.isMergeMultiplexer ? 1 : 0);
    if(s.childStruct == nullptr){
      res += s.pos;
    } else{
      res += HashStructInfo(s.childStruct);
    }
     return res;
   }
};

template<> struct std::hash<StructInfo>{
   std::size_t operator()(StructInfo const& s) const noexcept{
     std::size_t res = 0;
     res += std::hash<void*>()(s.type);
     for(DoubleLink<StructElement>* ptr = s.list ? s.list->head : nullptr; ptr; ptr = ptr->next){
       res += std::hash<StructElement>()(ptr->elem);
     }
     res += std::hash<String>()(s.name);
     return res;
   }
};

static bool operator==(StructInfo& l,StructInfo& r);

static bool operator==(StructElement& l,StructElement& r){
  bool res = (*l.childStruct == *r.childStruct &&
              l.name == r.name &&
              l.size == r.size &&
              l.isMergeMultiplexer == r.isMergeMultiplexer);
    
  return res;
}

static bool operator==(StructInfo& l,StructInfo& r){
  if(l.type != r.type){
    return false;
  }

  if(l.isSimpleType != r.isSimpleType){
    return false;
  }

  bool simpleType = l.isSimpleType;
  
  DoubleLink<StructElement>* lPtr = l.list ? l.list->head : nullptr;
  DoubleLink<StructElement>* rPtr = r.list ? r.list->head : nullptr;
  for(; lPtr && rPtr; lPtr = lPtr->next,rPtr = rPtr->next){
    if(!(lPtr->elem == rPtr->elem)){
      return false;
    }
    if(simpleType && lPtr->elem.pos != rPtr->elem.pos){
      return false;
    }
  }

  if(!(lPtr) != !(rPtr)){
    return false;
  }
  
  return (l.name == r.name);
}

Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out);

void OutputCircuitSource(FUDeclaration* decl,FILE* file);
void OutputIterativeSource(FUDeclaration* decl,FILE* file);
void OutputTopLevelFiles(Accelerator* accel,FUDeclaration* topLevelDecl,const char* hardwarePath,const char* softwarePath,bool isSimple);
