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

// NOTE: A kinda hacky way of having to avoid doing expensive functions on graphs if they are not gonna be needed. (No point doing a flatten operation if we do not have a merge operation that makes use of it). We probably wanna replace this approach with something better, though. Very hacky currently.
enum SubUnitOptions{
  SubUnitOptions_BAREBONES,
  SubUnitOptions_FULL
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

// Declaration functions
FUDeclaration* RegisterIterativeUnit(Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Accelerator* circuit,SubUnitOptions options = SubUnitOptions_FULL);

// Helper functions, useful to implement custom units
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);
FUInstance* CreateOrGetOutput(Accelerator* accel);

// Helper functions to create sub accelerators
int GetInputPortNumber(FUInstance* inputInstance);
