#include "versatPrivate.hpp"

#include <unordered_map>
#include <set>
#include <queue>

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;

ComplexFUInstance* AcceleratorIterator::Start(Accelerator* accel){
   index = 0;
   calledStart = true;

   stack[0] = accel->instances.begin();
   ComplexFUInstance* inst = *stack[index];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::Next(){
   Assert(calledStart);

   ComplexFUInstance* inst = *stack[index];

   if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
      stack[++index] = inst->compositeAccel->instances.begin();
   } else {
      while(1){
         ++stack[index];

         if(!stack[index].HasNext()){
            if(index > 0){
               index -= 1;
               continue;
            }
         }

         break;
      }
   }

   if(!stack[index].HasNext()){
      return nullptr;
   }

   inst = *stack[index];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::CurrentAcceleratorInstance(){
   int i = std::max(0,index - 1);

   ComplexFUInstance* inst = *stack[i];
   return inst;
}

static int Visit(ComplexFUInstance*** ordering,ComplexFUInstance* inst){
   if(inst->tag == TAG_PERMANENT_MARK){
      return 0;
   }
   if(inst->tag == TAG_TEMPORARY_MARK){
      Assert(0);
   }

   if(inst->tempData->nodeType == GraphComputedData::TAG_SINK ||
     (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      return 0;
   }

   inst->tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
      for(int i = 0; i < inst->tempData->numberInputs; i++){
         count += Visit(ordering,inst->tempData->inputs[i].instConnectedTo.inst);
      }
   }

   inst->tag = TAG_PERMANENT_MARK;

   if(inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
      *(*ordering) = inst;
      (*ordering) += 1;
      count += 1;
   }

   return count;
}

static void CalculateDAGOrdering(Accelerator* accel){
   Assert(accel->locked == Accelerator::Locked::GRAPH);

   ZeroOutAlloc(&accel->order.instances,accel->instances.Size());

   accel->order.numberComputeUnits = 0;
   accel->order.numberSinks = 0;
   accel->order.numberSources = 0;
   for(ComplexFUInstance* inst : accel->instances){
      inst->tag = 0;
   }

   ComplexFUInstance** sourceUnits = accel->order.instances.ptr;
   accel->order.sources = sourceUnits;
   // Add source units, guaranteed to come first
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE || (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
         *(sourceUnits++) = inst;
         accel->order.numberSources += 1;
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   // Add compute units
   ComplexFUInstance** computeUnits = sourceUnits;
   accel->order.computeUnits = computeUnits;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         *(computeUnits++) = inst;
         accel->order.numberComputeUnits += 1;
         inst->tag = TAG_PERMANENT_MARK;
      } else if(inst->tag == 0 && inst->tempData->nodeType == GraphComputedData::TAG_COMPUTE){
         accel->order.numberComputeUnits += Visit(&computeUnits,inst);
      }
   }

   // Add sink units
   ComplexFUInstance** sinkUnits = computeUnits;
   accel->order.sinks = sinkUnits;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_SINK || (inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
         *(sinkUnits++) = inst;
         accel->order.numberSinks += 1;
         Assert(inst->tag == 0);
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      Assert(inst->tag == TAG_PERMANENT_MARK);
   }

   Assert(accel->order.numberSources + accel->order.numberComputeUnits + accel->order.numberSinks == accel->instances.Size());
}

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers){
   Accelerator* newAccel = CreateAccelerator(versat);
   newAccel->type = Accelerator::CIRCUIT;

   std::set<ComplexFUInstance*> subgraphUnits;
   std::set<ComplexFUInstance*> tempSubgraphUnits;
   subgraphUnits.insert(instance);
   for(int i = 0; i < layers; i++){
      for(ComplexFUInstance* inst : subgraphUnits){
         for(Edge* edge : accel->edges){
            for(int i = 0; i < 2; i++){
               if(edge->units[i].inst == inst){
                  tempSubgraphUnits.insert(edge->units[1 - i].inst);
                  break;
               }
            }
         }
      }

      subgraphUnits.insert(tempSubgraphUnits.begin(),tempSubgraphUnits.end());
      tempSubgraphUnits.clear();
   }

   InstanceMap map;
   for(ComplexFUInstance* nonMapped : subgraphUnits){
      ComplexFUInstance* mapped = (ComplexFUInstance*) CreateFUInstance(newAccel,nonMapped->declaration,nonMapped->name);
      map.insert({nonMapped,mapped});
   }

   for(Edge* edge : accel->edges){
      auto iter0 = map.find(edge->units[0].inst);
      auto iter1 = map.find(edge->units[1].inst);

      if(iter0 == map.end() || iter1 == map.end()){
         continue;
      }

      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = iter0->second;
      newEdge->units[1].inst = iter1->second;
   }

   SubgraphData data = {};

   data.accel = newAccel;
   data.instanceMapped = map.at(instance);

   return data;
}

void LockAccelerator(Accelerator* accel,Accelerator::Locked level,bool freeMemory){
   if(level == Accelerator::Locked::FREE){
      if(freeMemory){
         Free(&accel->versatData);
         Free(&accel->order.instances);
         Free(&accel->graphData);
      }

      accel->locked = level;
      return;
   }

   if(accel->locked >= level){
      return;
   }

   if(level >= Accelerator::Locked::GRAPH){
      CalculateGraphData(accel);
      accel->locked = Accelerator::Locked::GRAPH;
   }
   if(level >= Accelerator::Locked::ORDERED){
      CalculateDAGOrdering(accel);
      accel->locked = Accelerator::Locked::ORDERED;
   }
   if(level >= Accelerator::Locked::FIXED){
      CalculateVersatData(accel);
      accel->locked = Accelerator::Locked::FIXED;
   }
}

void CalculateGraphData(Accelerator* accel){
   int memoryNeeded = sizeof(GraphComputedData) * accel->instances.Size() + 2 * accel->edges.Size() * sizeof(ConnectionInfo);

   ZeroOutAlloc(&accel->graphData,memoryNeeded);

   GraphComputedData* computedData = (GraphComputedData*) accel->graphData.ptr;
   ConnectionInfo* inputBuffer = (ConnectionInfo*) &computedData[accel->instances.Size()];
   ConnectionInfo* outputBuffer = &inputBuffer[accel->edges.Size()];

   // Associate computed data to each instance
   int i = 0;
   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData = &computedData[i++];
   }

   // Set inputs and outputs
   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData->inputs = inputBuffer;
      inst->tempData->outputs = outputBuffer;

      for(Edge* edge : accel->edges){
         if(edge->units[0].inst == inst){
            outputBuffer->instConnectedTo = edge->units[1];
            outputBuffer->port = edge->units[0].port;
            outputBuffer += 1;

            inst->tempData->outputPortsUsed = maxi(inst->tempData->outputPortsUsed,edge->units[0].port + 1);
            inst->tempData->numberOutputs += 1;
         }
         if(edge->units[1].inst == inst){
            inputBuffer->instConnectedTo = edge->units[0];
            inputBuffer->port = edge->units[1].port;
            inputBuffer->delay = edge->delay;
            inputBuffer += 1;

            Assert(abs(inputBuffer->delay) < 100);

            inst->tempData->inputPortsUsed = maxi(inst->tempData->inputPortsUsed,edge->units[1].port + 1);
            inst->tempData->numberInputs += 1;
         }
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData->nodeType = GraphComputedData::TAG_UNCONNECTED;

      bool hasInput = (inst->tempData->numberInputs > 0);
      bool hasOutput = (inst->tempData->numberOutputs > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(hasInput && hasOutput){
         #if 0
         if(inst->declaration->nDelays){
            inst->tempData->nodeType = GraphComputedData::TAG_SOURCE_AND_SINK;
         } else {
            inst->tempData->nodeType = GraphComputedData::TAG_COMPUTE;
         }
         #else
         if(CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY) || CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY)){
            inst->tempData->nodeType = GraphComputedData::TAG_SOURCE_AND_SINK;
         }  else {
            inst->tempData->nodeType = GraphComputedData::TAG_COMPUTE;
         }
         #endif
      } else if(hasInput){
         inst->tempData->nodeType = GraphComputedData::TAG_SINK;
      } else if(hasOutput){
         inst->tempData->nodeType = GraphComputedData::TAG_SOURCE;
      } else {
         //Assert(0); // Unconnected
      }
   }
}

struct HuffmanBlock{
   int bits;
   ComplexFUInstance* instance; // TODO: Maybe add the instance index (on the list) so we can push to the left instances that appear first and make it easier to see the mapping taking place
   HuffmanBlock* left;
   HuffmanBlock* right;
   enum {LEAF,NODE} type;
};

static void SaveMemoryMappingInfo(char* buffer,int size,HuffmanBlock* block){
   if(block->type == HuffmanBlock::LEAF){
      ComplexFUInstance* inst = block->instance;

      memcpy(inst->versatData->memoryMask,buffer,size);
      inst->versatData->memoryMask[size] = '\0';
      inst->versatData->memoryMaskSize = size;
   } else {
      buffer[size] = '1';
      SaveMemoryMappingInfo(buffer,size + 1,block->left);
      buffer[size] = '0';
      SaveMemoryMappingInfo(buffer,size + 1,block->right);
   }
}

void CalculateVersatData(Accelerator* accel){
   ZeroOutAlloc(&accel->versatData,accel->instances.Size());

   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
         LockAccelerator(inst->compositeAccel,Accelerator::FIXED);
      }
   };

   VersatComputedData* mem = accel->versatData.ptr;

   auto Compare = [](const HuffmanBlock* a, const HuffmanBlock* b) {
      return a->bits > b->bits;
   };

   Pool<HuffmanBlock> blocks = {};
   std::priority_queue<HuffmanBlock*,std::vector<HuffmanBlock*>,decltype(Compare)> nodes(Compare);

   for(ComplexFUInstance* inst : accel->instances){
      inst->versatData = mem++;

      if(inst->declaration->isMemoryMapped){
         HuffmanBlock* block = blocks.Alloc();

         block->bits = inst->declaration->memoryMapBits;
         block->instance = inst;
         block->type = HuffmanBlock::LEAF;

         nodes.push(block);
      }
   }

   if(nodes.size() == 0){
      return;
   }

   while(nodes.size() > 1){
      HuffmanBlock* first = nodes.top();
      nodes.pop();
      HuffmanBlock* second = nodes.top();
      nodes.pop();

      HuffmanBlock* block = blocks.Alloc();
      block->type = HuffmanBlock::NODE;
      block->left = first;
      block->right = second;
      block->bits = maxi(first->bits,second->bits) + 1;

      nodes.push(block);
   }

   char bitMapping[33];
   memset(bitMapping,0,33 * sizeof(char));
   HuffmanBlock* root = nodes.top();

   SaveMemoryMappingInfo(bitMapping,0,root);

   int maxMask = 0;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->isMemoryMapped){
         maxMask = maxi(maxMask,inst->versatData->memoryMaskSize + inst->declaration->memoryMapBits);
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->isMemoryMapped){
         int memoryAddressOffset = 0;
         for(int i = 0; i < inst->versatData->memoryMaskSize; i++){
            int bit = inst->versatData->memoryMask[i] - '0';

            memoryAddressOffset += bit * (1 << (maxMask - i - 1));
         }
         inst->versatData->memoryAddressOffset = memoryAddressOffset;

         // TODO: Not a good way of dealing with calculating memory map to lower units, but should suffice for now
         Accelerator* subAccel = inst->compositeAccel;
         if(inst->declaration->type == FUDeclaration::COMPOSITE && subAccel){
            AcceleratorIterator iter = {};
            for(ComplexFUInstance* inst = iter.Start(subAccel); inst; inst = iter.Next()){
               if(inst->declaration->isMemoryMapped){
                  inst->versatData->memoryAddressOffset += memoryAddressOffset;
               }
            };
         }
      }
   }

   blocks.Clear(true);
}

void FixMultipleInputs(Versat* versat,Accelerator* accel){
   static int multiplexersInstantiated = 0;
   static int graphFixed = 0;
   LockAccelerator(accel,Accelerator::Locked::GRAPH);

   int portUsedCount[99];
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration == versat->multiplexer){ // Not good, but works for now (otherwise newly added muxes would break this part)
         continue;
      }

      memset(portUsedCount,0,sizeof(int) * 99);

      for(int i = 0; i < inst->tempData->numberInputs; i++){
         ConnectionInfo* info = &inst->tempData->inputs[i];

         Assert(info->port < 99 && info->port >= 0);
         portUsedCount[info->port] += 1;
      }

      for(int port = 0; port < 99; port++){
         if(portUsedCount[port] > 1){ // Edge has more that one connection
            SizedString name = PushString(&versat->permanent,"mux%d",multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,versat->multiplexer,name);

            // Connect edges to multiplexer
            int portsConnected = 0;
            for(Edge* edge : accel->edges){
               if(edge->units[1] == PortInstance{inst,port}){
                  edge->units[1].inst = multiplexer;
                  edge->units[1].port = portsConnected;
                  portsConnected += 1;
               }
            }

            // Connect multiplexer to intended input
            ConnectUnits(multiplexer,0,inst,port);

            // Ports connected can be used to parameterize the multiplexer connection size
         }
      }
   }

   LockAccelerator(accel,Accelerator::Locked::FREE);

   if(versat->debug.outputGraphs){
      char buffer[1024];
      sprintf(buffer,"debug/%.*s/",UNPACK_SS(accel->subtype->name));
      MakeDirectory(buffer);
      OutputGraphDotFile(accel,false,"debug/%.*s/mux.dot",UNPACK_SS(accel->subtype->name));
   }
}

void SendLatencyUpwards(Versat* versat,ComplexFUInstance* inst){
   int b = inst->tempData->inputDelay;

   int minimum = 0;
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      ConnectionInfo* info = &inst->tempData->inputs[i];

      int a = inst->declaration->inputDelays[info->port];
      int e = info->delay; //inst->tempData->inputs[info->port].delay; // info->delay;

      ComplexFUInstance* other = inst->tempData->inputs[i].instConnectedTo.inst;
      for(int ii = 0; ii < other->tempData->numberOutputs; ii++){
         ConnectionInfo* otherInfo = &other->tempData->outputs[ii];

         int c = other->declaration->latencies[info->instConnectedTo.port];

         if(info->instConnectedTo.inst == other && info->instConnectedTo.port == otherInfo->port &&
            otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
            otherInfo->delay = b + a + e - c;

            minimum = mini(minimum,otherInfo->delay);

            //Assert(abs(otherInfo->delay) < 100);
         }
      }
   }

   #if 0
   for(int i = 0; i < inst->tempData->numberInputs; i++){
      ComplexFUInstance* other = inst->tempData->inputs[i].instConnectedTo.inst;
      for(int ii = 0; ii < other->tempData->numberOutputs; ii++){
         ConnectionInfo* otherInfo = &other->tempData->outputs[ii];

         otherInfo->delay += abs(minimum);
      }
   }
   #endif
}

// Fixes edges such that unit before connected to after, is reconnected to new unit
void InsertUnit(Accelerator* accel, PortInstance before, PortInstance after, PortInstance newUnit){
   LockAccelerator(accel,Accelerator::Locked::FREE);

   for(Edge* edge : accel->edges){
      if(edge->units[0] == before && edge->units[1] == after){
         ConnectUnits(newUnit,after);

         edge->units[1] = newUnit;

         return;
      }
   }
}

void CalculateDelay(Versat* versat,Accelerator* accel){
   static int graphs = 0;
   LockAccelerator(accel,Accelerator::Locked::FREE,true);
   LockAccelerator(accel,Accelerator::Locked::ORDERED);

   DAGOrder order = accel->order;

   // Clear everything, just in case
   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData->inputDelay = 0;

      for(int i = 0; i < inst->tempData->numberOutputs; i++){
         inst->tempData->outputs[i].delay = 0;
      }
   }

   if(versat->debug.outputGraphs){
      char buffer[1024];
      sprintf(buffer,"debug/%.*s/",UNPACK_SS(accel->subtype->name));
      MakeDirectory(buffer);
   }

   #if 0
   printf("%.*s:\n",UNPACK_SS(accel->subtype->name));
   int maxLatency = 0;
   for(int i = 0; i < order.numberSinks; i++){
      int lat = CalculateLatency(order.sinks[i]);

      printf("  %d\n",lat);
      maxLatency = maxi(maxLatency,lat);
   }
   #endif

   for(int i = 0; i < order.numberSinks; i++){
      ComplexFUInstance* inst = order.sinks[i];

      inst->tempData->inputDelay = -10000; // As long as it is a constant greater than max latency, should work fine
      inst->baseDelay = abs(inst->tempData->inputDelay);

      SendLatencyUpwards(versat,inst);

      if(versat->debug.outputGraphs){
         OutputGraphDotFile(accel,false,"debug/%.*s/out1_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
      }
   }

   for(int i = accel->instances.Size() - order.numberSinks - 1; i >= 0; i--){
      ComplexFUInstance* inst = order.instances.ptr[i];

      if(inst->tempData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         continue;
      }

      int minimum = (1 << 30);
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         minimum = mini(minimum,inst->tempData->outputs[ii].delay);
      }

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         inst->tempData->outputs[ii].delay -= minimum;
      }

      inst->tempData->inputDelay = minimum;
      inst->baseDelay = abs(inst->tempData->inputDelay);

      SendLatencyUpwards(versat,inst);

      if(versat->debug.outputGraphs){
         OutputGraphDotFile(accel,false,"debug/%.*s/out2_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
      }
   }

   int minimum = 0;
   for(ComplexFUInstance* inst : accel->instances){
      minimum = mini(minimum,inst->tempData->inputDelay);
   }

   minimum = abs(minimum);
   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData->inputDelay = minimum - abs(inst->tempData->inputDelay);
   }

   for(ComplexFUInstance* inst : accel->instances){
      if(inst->tempData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
            inst->tempData->outputs[ii].delay = 0;
         }
      }
      inst->baseDelay = inst->tempData->inputDelay;
   }

   #if 1
   accel->locked = Accelerator::Locked::FREE;
   // Insert delay units if needed
   int buffersInserted = 0;
   static int maxDelay = 0;
   for(int i = accel->instances.Size() - 1; i >= 0; i--){
      ComplexFUInstance* inst = order.instances.ptr[i];

      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         ConnectionInfo* info = &inst->tempData->outputs[ii];
         ComplexFUInstance* other = info->instConnectedTo.inst;

         if(other->declaration == versat->buffer){
            other->baseDelay = info->delay;
            //Assert(other->baseDelay >= 0);
         } else if(info->delay != 0){

            Assert(inst->declaration != versat->buffer);
            //Assert(info->instConnectedTo.inst->declaration != versat->delay);

            SizedString bufferName = PushString(&versat->permanent,"buffer%d",buffersInserted++);

            ComplexFUInstance* delay = (ComplexFUInstance*) CreateFUInstance(accel,versat->buffer,bufferName,false,true);

            InsertUnit(accel,PortInstance{inst,info->port},PortInstance{info->instConnectedTo.inst,info->instConnectedTo.port},PortInstance{delay,0});

            delay->baseDelay = info->delay - versat->buffer->latencies[0];

            Assert(delay->baseDelay >= 0);

            maxDelay = maxi(maxDelay,delay->baseDelay);

            if(delay->config){
               delay->config[0] = delay->baseDelay;
            }
            Assert(delay->baseDelay >= 0);
         }
      }
   }

   accel->locked = Accelerator::Locked::ORDERED;

   LockAccelerator(accel,Accelerator::Locked::FREE,true);
   LockAccelerator(accel,Accelerator::Locked::ORDERED);

   for(ComplexFUInstance* inst : accel->instances){
      inst->tempData->inputDelay = inst->baseDelay;

      Assert(inst->baseDelay >= 0);

      if(inst->declaration == versat->buffer && inst->config){
         inst->config[0] = inst->baseDelay;
      }
   }
   #endif

   if(versat->debug.outputGraphs){
      OutputGraphDotFile(accel,false,"debug/%.*s/out3_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
   }
}

void ActivateMergedAcceleratorRecursive(Versat* versat,ComplexFUInstance* inst, int index){
   if(inst->compositeAccel){
      AcceleratorIterator iter = {};

      for(ComplexFUInstance* subInst = iter.Start(inst->compositeAccel); subInst; subInst = iter.Next()){
         if(versat->multiplexer == subInst->declaration){
            subInst->config[0] = index;
         }
      }
   } else {
      if(versat->multiplexer == inst->declaration){
         inst->config[0] = index;
      }
   }
}

void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type){
   AcceleratorIterator iter = {};

   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      for(int i = 0; i < 2; i++){
         if(inst->declaration->mergedType[i] == type){
            ActivateMergedAcceleratorRecursive(versat,inst,i);
         }
      }
   }
}


















