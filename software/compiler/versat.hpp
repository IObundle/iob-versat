#pragma once

#include "utils.hpp"
#include "memory.hpp"

#include "globals.hpp"
#include "debugVersat.hpp"

// TODO: This file can probably go. Just rename and move things around to files that make more sense.

// Forward declarations
struct Accelerator;
struct FUDeclaration;
struct IterativeUnitDeclaration;
struct FUInstance;

//typedef uint GraphDotFormat;
enum GraphDotFormat : int{
  GraphDotFormat_NAME = 0x1,
  GraphDotFormat_TYPE = 0x2,
  GraphDotFormat_ID = 0x4,
  GraphDotFormat_DELAY = 0x8,
  GraphDotFormat_EXPLICIT = 0x10, // indicates which field is which when outputting name
  GraphDotFormat_PORT = 0x20, // Outputs port information for edges and port instance
  GraphDotFormat_LATENCY = 0x40 // Outputs latency information for edges and port instances which know their latency information
};

struct WireInformation{
  Wire wire;
  int addr;
  bool isStatic;
  SymbolicExpression* startBitExpr;
};

// Global parameters are verilog parameters that Versat assumes that exist and that it uses through the entire accelerator.
// Units are not required to implement them but if they do, their values comes from Versat and user cannot change them.
bool IsGlobalParameter(String name);

// Returns the parameters including default parameters if the unit does not define them (defaults from the declaration)
Hashmap<String,SymbolicExpression*>* GetParametersOfUnit(FUInstance* inst,Arena* out);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
bool IsTypeHierarchical(FUDeclaration* decl);

Opt<FUDeclaration*> RegisterModuleInfo(ModuleInfo* info,Arena* out);

Array<WireInformation> CalculateWireInformation(Pool<FUInstance> instances,Hashmap<StaticId,StaticData>* staticUnits,int addrOffset,Arena* out);

void FillDeclarationWithAcceleratorValues(FUDeclaration* decl,Accelerator* accel,Arena* out,bool calculateOrder = true);
void FillDeclarationWithDelayType(FUDeclaration* decl);

// NOTE: A kinda hacky way of having to avoid doing expensive functions on graphs if they are not gonna be needed. (No point doing a flatten operation if we do not have a merge operation that makes use of it). We probably wanna replace this approach with something better, though. Very hacky currently.
enum SubUnitOptions{
  SubUnitOptions_BAREBONES,
  SubUnitOptions_FULL
};

// Declaration functions
FUDeclaration* RegisterIterativeUnit(Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Accelerator* circuit,SubUnitOptions options = SubUnitOptions_FULL);

// Helper functions, useful to implement custom units
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);
FUInstance* CreateOrGetOutput(Accelerator* accel);

// Helper functions to create sub accelerators
int GetInputPortNumber(FUInstance* inputInstance);

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

inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}
