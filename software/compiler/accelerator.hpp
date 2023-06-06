#ifndef INCLUDED_VERSAT_ACCELERATOR
#define INCLUDED_VERSAT_ACCELERATOR

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
#include "declaration.hpp"

struct ComplexFUInstance;
struct InstanceNode;
struct OrderedInstance;
struct FUDeclaration;
struct Edge;
struct PortEdge;
struct Versat;
struct FUInstance;

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;
typedef Hashmap<InstanceNode*,InstanceNode*> InstanceNodeMap;

struct Accelerator{ // Graph + data storage
   Versat* versat;
   FUDeclaration* subtype; // Set if subaccelerator (accelerator associated to a FUDeclaration). A "free" accelerator has this set to nullptr

   Edge* edges; // TODO: Should be removed, edge info is all contained inside the instance nodes and desync leads to bugs since some code still uses this

   OrderedInstance* ordered;
   InstanceNode* allocated;
   InstanceNode* lastAllocated;
   Pool<ComplexFUInstance> instances;
   Pool<FUInstance> subInstances; // Essentially a "wrapper" so that user code does not have to deal with reallocations when adding units

   Allocation<iptr> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<iptr> staticAlloc;
   Allocation<Byte> extraDataAlloc;
   Allocation<int> externalMemoryAlloc;
   //Allocation<UnitDebugData> debugDataAlloc;

   Allocation<int> outputAlloc;
   Allocation<int> storedOutputAlloc;

   DynamicArena* accelMemory;

   std::unordered_map<StaticId,StaticData> staticUnits;

	void* configuration;
	int configurationSize;

	int cyclesPerRun;

	int created;
	int entityId;
	bool init;
};

class AcceleratorIterator{
public:
   union Type{
      InstanceNode* node;
      OrderedInstance* ordered;
   };

   Array<Type> stack;
   Accelerator* topLevel;
   int level;
   int upperLevels;
   bool calledStart;
   bool populate;
   bool ordered;
   bool levelBelow;

   InstanceNode* GetInstance(int level);

   // Must call first
   InstanceNode* Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* temp,bool populate = false);
   InstanceNode* Start(Accelerator* topLevel,Arena* temp,bool populate = false);

   InstanceNode* StartOrdered(Accelerator* topLevel,Arena* temp,bool populate = false);

   InstanceNode* ParentInstance();
   String GetParentInstanceFullName(Arena* out);

   InstanceNode* Descend(); // Current() must be a composite instance, otherwise this will fail

   InstanceNode* Next(); // Iterates over subunits
   InstanceNode* Skip(); // Next unit on the same level

   InstanceNode* Current(); // Returns nullptr to indicate end of iteration
   InstanceNode* CurrentAcceleratorInstance(); // Returns the accelerator instance for the Current() instance or nullptr if currently at top level

   String GetFullName(Arena* out,const char* sep);
   int    GetFullLevel();

   AcceleratorIterator LevelBelowIterator(); // Not taking an arena means that the returned iterator uses current iterator memory. Returned iterator must be iterated fully before the current iterator can be used, otherwise memory conflicts will arise as both iterators are sharing the same stack
};

// Accelerator
Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
Accelerator* CopyFlatAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName,InstanceNode* previous);
ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName);
InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName);
InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name);
void InitializeFUInstances(Accelerator* accel,bool force);
int CountNonOperationChilds(Accelerator* accel);

bool IsCombinatorial(Accelerator* accel);

void ReorganizeAccelerator(Accelerator* graph,Arena* temp);
void ReorganizeIterative(Accelerator* accel,Arena* temp);

#endif // INCLUDED_VERSAT_ACCELERATOR
