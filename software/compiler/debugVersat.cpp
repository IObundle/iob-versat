#include "debugVersat.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

#include <algorithm>

#include "memory.hpp"
#include "type.hpp"
#include "utilsCore.hpp"

#include "textualRepresentation.hpp"
#include "versat.hpp"
#include "accelerator.hpp"

struct MemoryRange{
  int start;
  int delta;
  FUDeclaration* decl;
  String name;
  int level;
  bool isComposite;
};

void SortRanges(Array<Range<int>> ranges){
  qsort(ranges.data,ranges.size,sizeof(Range<int>),[](const void* v1,const void* v2){
    Range<int>* r1 = (Range<int>*) v1;
    Range<int>* r2 = (Range<int>*) v2;
    return r1->start - r2->start;
  });
}

static bool IsSorted(Array<Range<int>> ranges){
  for(int i = 0; i < ranges.size - 1; i++){
    if(ranges[i + 1].start < ranges[i].start){
      DEBUG_BREAK_IF_DEBUGGING();
      return false;
    }
  }
  return true;
}

CheckRangeResult CheckNoOverlap(Array<Range<int>> ranges){
  CheckRangeResult res = {};

  Assert(IsSorted(ranges));

  for(int i = 0; i < ranges.size - 1; i++){
    Range cur = ranges[i];
    Range next = ranges[i+1];

    if(cur.end > next.start){
      res.result = false;
      res.problemIndex = i;

      return res;
    }
  }

  res.result = true;
  return res;
}

CheckRangeResult CheckNoGaps(Array<Range<int>> ranges){
  CheckRangeResult res = {};

  Assert(IsSorted(ranges));

  for(int i = 0; i < ranges.size - 1; i++){
    Range cur = ranges[i];
    Range next = ranges[i+1];

    if(cur.end != next.start){
      res.result = false;
      res.problemIndex = i;

      return res;
    }
  }

  res.result = true;
  return res;
}

bool CheckInputAndOutputNumber(FUDeclaration* type,int inputs,int outputs){
  if(inputs > type->inputDelays.size){
    return false;
  } else if(outputs > type->outputLatencies.size){
    return false;
  }

  return true;
}

static void OutputGraphDotFile_(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlighInstance,FILE* outputFile,Arena* temp){
  BLOCK_REGION(temp);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    String id = UniqueRepr(inst,temp);
    String name = Repr(inst,versat->debug.dotFormat,temp);

    String color = STRING("darkblue");
    int delay = 0;
    
    if(ptr->type == InstanceNode::TAG_SOURCE || ptr->type == InstanceNode::TAG_SOURCE_AND_SINK){
      color = STRING("darkgreen");
      delay = inst->baseDelay;
    } else if(ptr->type == InstanceNode::TAG_SINK){
      color = STRING("dark");
    }

    bool doHighligh = (highlighInstance ? highlighInstance->Exists(inst) : false);

    if(doHighligh){
      fprintf(outputFile,"\t\"%.*s\" [color=darkred,label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(name),delay);
    } else {
      fprintf(outputFile,"\t\"%.*s\" [color=%.*s label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(color),UNPACK_SS(name),delay);
    }
  }

  int size = Size(accel->edges);
  Hashmap<Pair<InstanceNode*,InstanceNode*>,int>* seen = PushHashmap<Pair<InstanceNode*,InstanceNode*>,int>(temp,size);

  // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* out = ptr->inst;

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      FUInstance* in = con->instConnectedTo.node->inst;

      if(collapseSameEdges){
        Pair<InstanceNode*,InstanceNode*> nodeEdge = {};
        nodeEdge.first = ptr;
        nodeEdge.second = con->instConnectedTo.node;

        GetOrAllocateResult<int> res = seen->GetOrAllocate(nodeEdge);
        if(res.alreadyExisted){
          continue;
        }
      }

      int inPort = con->instConnectedTo.port;
      int outPort = con->port;

      String first = UniqueRepr(out,temp);
      String second = UniqueRepr(in,temp);
      PortInstance start = {out,outPort};
      PortInstance end = {in,inPort};
      String label = Repr(&start,&end,versat->debug.dotFormat,temp);
      int calculatedDelay = con->delay ? *con->delay : 0;

      bool highlighStart = (highlighInstance ? highlighInstance->Exists(start.inst) : false);
      bool highlighEnd = (highlighInstance ? highlighInstance->Exists(end.inst) : false);

      bool highlight = highlighStart && highlighEnd;

      fprintf(outputFile,"\t\"%.*s\" -> ",UNPACK_SS(first));
      fprintf(outputFile,"\"%.*s\"",UNPACK_SS(second));

      if(highlight){
        fprintf(outputFile,"[color=darkred,label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),con->edgeDelay,calculatedDelay);
      } else {
        fprintf(outputFile,"[label=\"%.*s\\n[%d:%d]\"];\n",UNPACK_SS(label),con->edgeDelay,calculatedDelay);
      }
    }
  }
  fprintf(outputFile,"}\n");
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,String filename,Arena* temp){
  if(!versat->debug.outputGraphs){
    return;
  }

  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  OutputGraphDotFile_(versat,accel,collapseSameEdges,nullptr,file,temp);
  fclose(file);
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,String filename,Arena* temp){
  if(!versat->debug.outputGraphs){
    return;
  }

  BLOCK_REGION(temp);
  Set<FUInstance*>* highlight = PushSet<FUInstance*>(temp,1);
  highlight->Insert(highlighInstance);

  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  OutputGraphDotFile_(versat,accel,collapseSameEdges,highlight,file,temp);
  fclose(file);
}

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlight,String filename,Arena* temp){
  if(!versat->debug.outputGraphs){
    return;
  }

  BLOCK_REGION(temp);
  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  OutputGraphDotFile_(versat,accel,collapseSameEdges,highlight,file,temp);
  fclose(file);
}

String PushMemoryHex(Arena* arena,void* memory,int size){
  Byte* mark = MarkArena(arena);

  unsigned char* view = (unsigned char*) memory;

  for(int i = 0; i < size; i++){
    int low = view[i] % 16;
    int high = view[i] / 16;

    PushString(arena,"%c%c ",GetHex(high),GetHex(low));
  }

  return PointArena(arena,mark);
}

void OutputMemoryHex(void* memory,int size){
  unsigned char* view = (unsigned char*) memory;

  for(int i = 0; i < size; i++){
    if(i % 16 == 0 && i != 0){
      printf("\n");
    }

    int low = view[i] % 16;
    int high = view[i] / 16;

    printf("%c%c ",GetHex(high),GetHex(low));

  }

  printf("\n");
}