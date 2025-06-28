#pragma once

#include "utils.hpp"
#include "memory.hpp"
#include "logger.hpp"

#include "globals.hpp"
#include "debugVersat.hpp"

// TODO: This file can probably go. Just rename and move things around to files that make more sense.

// Forward declarations
struct Accelerator;
struct FUDeclaration;
struct IterativeUnitDeclaration;
struct FUInstance;

enum VersatDebugFlags{
  OUTPUT_GRAPH_DOT,
  GRAPH_DOT_FORMAT,
  OUTPUT_ACCELERATORS_CODE,
  OUTPUT_VERSAT_CODE,
  USE_FIXED_BUFFERS
};

typedef uint GraphDotFormat;
enum GraphDotFormat_{
  GRAPH_DOT_FORMAT_NAME = 0x1,
  GRAPH_DOT_FORMAT_TYPE = 0x2,
  GRAPH_DOT_FORMAT_ID = 0x4,
  GRAPH_DOT_FORMAT_DELAY = 0x8,
  GRAPH_DOT_FORMAT_EXPLICIT = 0x10, // indicates which field is which when outputting name
  GRAPH_DOT_FORMAT_PORT = 0x20, // Outputs port information for edges and port instance
  GRAPH_DOT_FORMAT_LATENCY = 0x40 // Outputs latency information for edges and port instances which know their latency information
};

struct WireInformation{
  Wire wire;
  int addr;
  int configBitStart;
  bool isStatic;
  SymbolicExpression* bitExpr;
};

struct DelayToAdd{
  Edge edge;
  String bufferName;
  String bufferParameters;
  int bufferAmount;
};

// Temp
bool EqualPortMapping(PortInstance p1,PortInstance p2);

// General info
bool IsUnitCombinatorial(FUInstance* inst);

// Graph fixes
Array<DelayToAdd> GenerateFixDelays(Accelerator* accel,EdgeDelay* edgeDelays,Arena* out);
void FixDelays(Accelerator* accel,Hashmap<Edge,DelayInfo>* edgeDelays);

// Accelerator merging
DAGOrderNodes CalculateDAGOrder(Pool<FUInstance>* instances,Arena* out);

// Global parameters are verilog parameters that Versat assumes that exist and that it uses through the entire accelerator.
// Units are not required to implement them but if they do, their values comes from Versat and user cannot change them.
bool IsGlobalParameter(String name);

// Returns the parameters including default parameters if the unit does not define them (defaults from the declaration)
Hashmap<String,SymbolicExpression*>* GetParametersOfUnit(FUInstance* inst,Arena* out);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
bool IsTypeHierarchical(FUDeclaration* decl);
FUInstance* GetInputInstance(Pool<FUInstance>* nodes,int inputIndex);
FUInstance* GetOutputInstance(Pool<FUInstance>* nodes);

FUDeclaration* RegisterModuleInfo(ModuleInfo* info,Arena* out);

// Accelerator functions
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);
Pair<Accelerator*,SubMap*> Flatten(Accelerator* accel,int times);

Array<WireInformation> CalculateWireInformation(Pool<FUInstance> instances,Hashmap<StaticId,StaticData>* staticUnits,int addrOffset,Arena* out);

void FillDeclarationWithAcceleratorValues(FUDeclaration* decl,Accelerator* accel,Arena* out);
void FillDeclarationWithDelayType(FUDeclaration* decl);

// Static and configuration sharing
void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* inst);

enum SubUnitOptions{
  SubUnitOptions_BAREBONES,
  SubUnitOptions_FULL
};

// Declaration functions
FUDeclaration* RegisterFU(FUDeclaration declaration);
FUDeclaration* GetTypeByName(String str);
FUDeclaration* RegisterIterativeUnit(Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Accelerator* circuit,SubUnitOptions options = SubUnitOptions_FULL);

// Helper functions, useful to implement custom units
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);
FUInstance* CreateOrGetOutput(Accelerator* accel);

// Helper functions to create sub accelerators
int GetInputPortNumber(FUInstance* inputInstance);

template<typename T> class std::hash<Array<T>>{
public:
   std::size_t operator()(Array<T> const& s) const noexcept{
   std::size_t res = 0;

   std::size_t prime = 5;
   for(int i = 0; i < s.size; i++){
      res += (std::size_t) s[i] * prime;
      res <<= 4;
      prime += 6; // Some not prime, but will find most of them
   }

   return res;
   }
};

template<> class std::hash<Edge>{
public:
   std::size_t operator()(Edge const& s) const noexcept{
      std::size_t res = std::hash<PortInstance>()(s.units[0]);
      res += std::hash<PortInstance>()(s.units[1]);
      res += s.delay;
      return (std::size_t) res;
   }
};

template<typename First,typename Second> class std::hash<Pair<First,Second>>{
public:
   std::size_t operator()(Pair<First,Second> const& s) const noexcept{
      std::size_t res = std::hash<First>()(s.first);
      res += std::hash<Second>()(s.second);
      return (std::size_t) res;
   }
};

// Try to use this function when not using any std functions, if possible
// I think the hash implementation for pointers is kinda bad, at this implementation, and not possible to specialize as the compiler complains.
#include <type_traits>
template<typename T>
inline int Hash(T const& t){
   int res;
   if(std::is_pointer<T>::value){
      union{
         T val;
         int integer;
      } conv;

      conv.val = t;
      res = (conv.integer >> 2); // TODO: Should be architecture aware
   } else {
      res = std::hash<T>()(t);
   }
   return res;
}

inline bool operator<(const String& lhs,const String& rhs){
   for(int i = 0; i < std::min(lhs.size,rhs.size); i++){
      if(lhs[i] < rhs[i]){
         return true;
      }
      if(lhs[i] > rhs[i]){
         return false;
      }
   }

   if(lhs.size < rhs.size){
      return true;
   }

   return false;
}

inline bool operator==(const String& lhs,const String& rhs){
   bool res = CompareString(lhs,rhs);
   return res;
}

inline bool operator!=(const String& lhs,const String& rhs){
   bool res = !CompareString(lhs,rhs);

   return res;
}

inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}
