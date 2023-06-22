#ifndef INCLUDED_VERSAT_ACCELERATOR
#define INCLUDED_VERSAT_ACCELERATOR

#include "utils.hpp"
#include "memory.hpp"

struct ComplexFUInstance;
struct InstanceNode;
struct OrderedInstance;
struct Accelerator;

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

   String GetFullName(Arena* out);
   int    GetFullLevel();

   AcceleratorIterator LevelBelowIterator(); // Not taking an arena means that the returned iterator uses current iterator memory. Returned iterator must be iterated fully before the current iterator can be used, otherwise memory conflicts will arise as both iterators are sharing the same stack
};

#endif // INCLUDED_VERSAT_ACCELERATOR
