#pragma once

#include "memory.hpp"
#include "accelerator.hpp"
#include "addressGen.hpp"

struct SimplePortInstance{
  int inst;
  int port;
};

struct SimplePortConnection{
  int otherInst;
  int otherPort;
  int port;
};

struct StructInfo;

// How does this work?

// Data that is carried directly from the units is set inside GenerateInitialInstanceInfo
// Data that is computed from graph / data that depends on other units is calculated inside FillInstanceInfo

// We currently just stuff everything into this struct, so it's easier to visualize all the info that we need for the current accelerator.
// Some of this data is duplicated/unnecessary, but for now we just carry on since this simplifies debugging a lot, being able to see all the info for a given accelerator directly.
// This approach is very slow but easier to debug since everything related to one unit is all in the same place.
// Until I find a better way of debugging (visualizing) SoA, this will stay like this for a while.

struct ParamAndValue{
  String name;
  SymbolicExpression* val;
};

enum SpecialUnitType{
  SpecialUnitType_NONE = 0,
  SpecialUnitType_INPUT = 1,
  SpecialUnitType_OUTPUT = 2,
  SpecialUnitType_FIXED_BUFFER = 3,
  SpecialUnitType_VARIABLE_BUFFER = 4,
  SpecialUnitType_OPERATION = 5
};

// TODO: Put some note explaining the required changes when inserting stuff here.
struct InstanceInfo{
  int level;
  FUDeclaration* decl;
  String typeName;
  String parentTypeName;

  int localIndex;

  int id;
  String name;
  String baseName; // NOTE: If the unit does not belong to the merge partition the baseName will equal name.
  String fullName;

  Array<Wire> configs;
  Array<Wire> states;

  Array<ExternalMemoryInterface> externalMemory;
  SingleInterfaces singleInterfaces;
  
  Opt<int> globalStaticPos; // Separating static from global makes stuff simpler. If mixing together, do not forget that struct generation cares about source of configPos.
  Opt<int> globalConfigPos;
  Opt<int> localConfigPos;

  Array<int> individualWiresGlobalStaticPos;
  Array<int> individualWiresGlobalConfigPos;
  Array<int> individualWiresLocalConfigPos;
  Array<bool> individualWiresShared;
  
  Array<ParamAndValue> params;

  int isConfigStatic; // Static must be handle separately, for the top level accelerator. 
  int configSize;

  bool isStatic;
  bool isGloballyStatic;
  
  bool isShared;
  int sharedIndex;
  Array<bool> isSpecificConfigShared;
  
  Opt<int> statePos;
  int stateSize;
  
  // Some of these could be removed and be computed from the others
  Opt<iptr> memMapped; // Used on Code Gen, to create the addresses for the memories.
  Opt<int> memMapBits;
  Opt<int> memMappedSize;
  Opt<String> memDecisionMask; // This is local to the accelerator

  Opt<int> delayPos;
  Array<int> extraDelay;
  int baseNodeDelay;
  int numberDelays;

  int variableBufferDelay;

  bool isComposite;
  bool isMerge;

  // Sepcific to merge muxs
  bool isMergeMultiplexer;
  int mergePort;
  int muxGroup; // TODO: I think that we can remove muxGroup. We know which units belong or not to a given merge partition and we know their input value so there is no point in keeping the harder to understand and compute muxGroups.

  bool doesNotBelong; // For merge units, if true then this unit does not actually exist for the given partition
  int special;
  int localOrder;
  FUInstance* inst; // Points to the recon instance for merge declarations.
  bool debug;

  NodeType connectionType;
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<int> portDelay;
  int partitionIndex; // TODO: What does this do? Probably a remnant from the old implementation.

  Array<SimplePortConnection> inputs; 
  Array<SimplePortConnection> outputs;

  Array<SimplePortInstance> inputsDirectly;
  Array<bool> outputIsConnected;

  AddressGenInst supportedAddressGen;

  SpecialUnitType specialType;

  StructInfo* structInfo;
};

struct ConfigFunction;

/*

Approach:

Push the parameter stuff to the AcceleratorInfo.
Make a function that iterates over the AcceleratorInfo and instantiates parameters.

From that point on everything that requires information uses it from an instantiated AcceleratorInfo.
The Circuits still output parameters, but the final product, the top level instance instantiates everything that is instantiated. The circuits are parameterized, the versat top level instance is not (unless we can sneak in a few stuff, as long as the software does not depend on it we can still provide it).

The only thing that needs parameters is the 

Change top level code generation function to use the instantiated values inside the AcceleratorInfo.

What problem are we trying to solve?

When an accelerator contains multiple merged units, we view it as a single partition that contains each unit activated to a single type.

If we have Module {A,B} where A = a | b and X = x | y, then we have 4 merge partitions: (a,x),(a,y),(b,x),(b,y).

Each partition contains two activated types at the same time. The first partition (a,x) contains the activated type a for the unit A and the activated type x for the unit B.

If a contains a user function and x contains a user function then this "partition" contains two user functions. One that activates the a side of the A unit and configures it and the other that activates the x side of the B unit and configures it.

Which means that:

  Activating one merge unit cannot change the activation of another merge unit. They need to be separated. 

Info that is tied to an unit is stored inside the InstanceInfo.
Info that is tied to a merge unit is stored inside each MergePartition.

What about info that is tied to a module?
If a module contains two merged units (of size 2) how do I save info that is tied to that unit?

The thing is that I can tie info to the unit by putting it into InstanceInfo.
  The problem is that we now have two ways of tying the UserConfig into units. The InstanceInfo and MergePartition way.

Maybe I can uplift this if we change ConfigFunctions to contain the merge index that is associated to. We can also have the struct contain the name of the unit instead of putting directly to it.

As long as the userFunctions can be instantiated for a given configuration 

For now, lets process to handle Composite units first and then we can tackle the merge stuff.

*/

struct MergePartition{
  String name;
  Array<InstanceInfo> info;

  // TODO: Composite units currently break the meaning of baseType.
  //       Since a composite unit with 2 merged instances would need to have 2 base types.
  FUDeclaration* baseType;
  AcceleratorMapping* baseTypeFlattenToMergedBaseCircuit;
  Set<PortInstance>*  mergeMultiplexers;
  
  // TODO: Weird that this is inside MergePartition but at the same time we need to know which merge partition this function belongs to in order to generate the 
  //       The weirdness is mostly the fact that userFunctions are mostly "Global" in the sense that they cannot repeat but at the same time we need the MergePartition info which means that we might just take this out and have to store the merge partition info in some other way.
  Array<ConfigFunction*> userFunctions;

  Accelerator* recon;
  
  // TODO: All these are useless. We can just store the data in the units themselves.
  Array<int> inputDelays;
  Array<int> outputLatencies;
};

// TODO: A lot of values are repeated between merge partitions and the like. Not a problem for now, maybe tackle it when things become stable. Or maybe leave it be, could be easier in future if we want to implement something more complex.
struct AccelInfo{
  Array<MergePartition> infos;
    
  int inputs;
  int outputs;

  int configs;
  int states;
  int delays;
  int nIOs;
  int statics;
  int staticBits;
  int sharedUnits;
  int externalMemoryInterfaces;
  int numberConnections;
  int nDones;

  Array<Wire> configWires;
  Array<Wire> stateWires;

  Array<Wire> allStaticWires;

  String staticExpr; // nocheckin: Check this 
  String delayStart; // nocheckin: Check this. Why string? Why not an actual expression
  
  // TODO: Should be an SymbolicExpression. We probably want everything to be a symbolic expression at this point.
  Opt<int> memMapBits;
  int unitsMapped;
  bool isMemoryMapped;
  bool signalLoop;
  bool implementsDone;
};

// NOTE: The member 'level' of InstanceInfo needs to be valid in order for this iterator to work. 
//       Do not know how to handle merged. Should we iterate Array<InstanceInfo> and let outside code work, or do we take the accelInfo and then allow the iterator to switch between different merges and stuff?      
struct AccelInfoIterator{
  String accelName; // Usually for debug purposes.
  AccelInfo* info;
  int index;
  int mergeIndex;

  void SetMergeIndex(int index){mergeIndex = index;};
  Array<InstanceInfo>& GetCurrentMerge();
  int MergeSize();

  String GetMergeName();
  
  bool IsValid();
  
  int GetIndex();
  int GetIndex(InstanceInfo* instance);
  
  AccelInfoIterator GetParent();
  InstanceInfo* GetParentUnit();
  InstanceInfo* CurrentUnit();
  InstanceInfo* GetUnit(int index);
  Array<InstanceInfo*> GetAllSubUnits(Arena* out);

  // Next and step mimick gdb like commands. Does not update current, instead returning the advanced iterator
  WARN_UNUSED AccelInfoIterator Next(); // Next unit in current level only.
  WARN_UNUSED AccelInfoIterator Step(); // Next unit in the array. Goes down and up the levels as it progresses.
  WARN_UNUSED AccelInfoIterator StepInsideOnly(); // Returns NonValid if non composite unit

  // Going in reverse might be helpful to simplify code. Sometimes it is capable of removing recursing entirely, although it takes a bit to get used to.
  WARN_UNUSED AccelInfoIterator ReverseStep();
  int CurrentLevelSize();
};

struct Partition{
  int value;
  int max;
  int mergeIndexStart;
  FUDeclaration* decl;
};

AccelInfoIterator StartIteration(AccelInfo* info,int mergeIndex = 0);

void FillAccelInfoFromCalculatedInstanceInfo(AccelInfo* info,Accelerator* accel);

Array<InstanceInfo*> GetAllSameLevelUnits(AccelInfo* info,int level,int mergeIndex,Arena* out);

// TODO: mergeIndex seems to be the wrong approach. Check the correct approach when trying to simplify merge.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Array<Partition> partitions,bool calculateOrder = true);
Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out);

void FillInstanceInfo(AccelInfoIterator initialIter,Arena* out);
void FillStaticInfo(AccelInfo* info);

// This function does not perform calculations that are only relevant to the top accelerator (like static units configs and such).
AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,bool calculateOrder = true);

Array<int> ExtractInputDelays(AccelInfoIterator top,Arena* out);
Array<int> ExtractOutputLatencies(AccelInfoIterator top,Arena* out);

// Array info related
// TODO: This should be built on top of AccelInfo (taking an iterator), instead of just taking the array of instance infos.
Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out);
Array<Pair<String,int>> ExtractMem(Array<InstanceInfo> info,Arena* out);

// TODO: Do we add this to AccelInfo?
//       How much do we calculate at the time vs how much we just put inside accelInfo?
String GetEntityMemName(InstanceInfo* info,Arena* out);

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out);

SymbolicExpression* GetParameterValue(InstanceInfo* info,String name);

bool IsUnitCombinatorialOperation(InstanceInfo* info);

// ======================================
// Static naming conventions

String GetStaticFullName(InstanceInfo* info,Arena* out);
String GetStaticWireFullName(InstanceInfo* info,Wire wire,Arena* out);
