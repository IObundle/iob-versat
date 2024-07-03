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

static void OutputGraphDotFile_(Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlighInstance,FILE* outputFile,Arena* temp){
  BLOCK_REGION(temp);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    String id = UniqueRepr(inst,temp);
    String name = Repr(inst,globalDebug.dotFormat,temp);

    String color = STRING("darkblue");
    int delay = 0;
    
    if(ptr->type == NodeType_SOURCE || ptr->type == NodeType_SOURCE_AND_SINK){
      color = STRING("darkgreen");
      delay = 0; //TODO: Broken //inst->baseDelay;
    } else if(ptr->type == NodeType_SINK){
      color = STRING("dark");
    }

    bool doHighligh = (highlighInstance ? highlighInstance->Exists(inst) : false);

    if(doHighligh){
      fprintf(outputFile,"\t\"%.*s\" [color=darkred,label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(name),delay);
    } else {
      fprintf(outputFile,"\t\"%.*s\" [color=%.*s label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(color),UNPACK_SS(name),delay);
    }
  }

  int size = 9999; // Size(accel->edges);
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
      String label = Repr(&start,&end,globalDebug.dotFormat,temp);
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

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,String filename,Arena* temp){
  if(!globalDebug.outputGraphs){
    return;
  }

  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(file);
  OutputGraphDotFile_(accel,collapseSameEdges,nullptr,file,temp);
}

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,String filename,Arena* temp){
  if(!globalDebug.outputGraphs){
    return;
  }

  BLOCK_REGION(temp);
  Set<FUInstance*>* highlight = PushSet<FUInstance*>(temp,1);
  highlight->Insert(highlighInstance);

  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(file);
  OutputGraphDotFile_(accel,collapseSameEdges,highlight,file,temp);
}

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlight,String filename,Arena* temp){
  if(!globalDebug.outputGraphs){
    return;
  }

  BLOCK_REGION(temp);
  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(file);
  OutputGraphDotFile_(accel,collapseSameEdges,highlight,file,temp);
}

void OutputContentToFile(String filepath,String content){
  FILE* file = fopen(StaticFormat("%.*s",UNPACK_SS(filepath)),"w");
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    printf("[OutputContentToFile] Error opening file: %.*s\n",UNPACK_SS(filepath));
    DEBUG_BREAK();
  }

  int res = fwrite(content.data,sizeof(char),content.size,file);
  Assert(res == content.size);
}

String PushMemoryHex(Arena* arena,void* memory,int size){
  auto mark = StartString(arena);

  unsigned char* view = (unsigned char*) memory;

  for(int i = 0; i < size; i++){
    int low = view[i] % 16;
    int high = view[i] / 16;

    PushString(arena,"%c%c ",GetHex(high),GetHex(low));
  }

  return EndString(mark);
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

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,CalculateDelayResult delays,String filename,Arena* temp){
  if(!globalDebug.outputGraphs){
    return;
  }

  FILE* outputFile = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(outputFile);
  
  BLOCK_REGION(temp);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    String id = UniqueRepr(inst,temp);
    String name = Repr(inst,globalDebug.dotFormat,temp);

    String color = STRING("darkblue");
    int delay = 0;
    
    if(ptr->type == NodeType_SOURCE || ptr->type == NodeType_SOURCE_AND_SINK){
      color = STRING("darkgreen");
      delay = 0; //TODO: Broken inst->baseDelay;
    } else if(ptr->type == NodeType_SINK){
      color = STRING("dark");
    }

    bool doHighligh = highlighInstance ? highlighInstance == inst : false;

    if(doHighligh){
      fprintf(outputFile,"\t\"%.*s\" [color=darkred,label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(name),delay);
    } else {
      fprintf(outputFile,"\t\"%.*s\" [color=%.*s label=\"%.*s-%d\"];\n",UNPACK_SS(id),UNPACK_SS(color),UNPACK_SS(name),delay);
    }
  }

  int size = 9999; // Size(accel->edges);
  Hashmap<Pair<InstanceNode*,InstanceNode*>,int>* seen = PushHashmap<Pair<InstanceNode*,InstanceNode*>,int>(temp,size);
  // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
  // TODO: Change to use Array<Edge> 
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
      String label = Repr(&start,&end,globalDebug.dotFormat,temp);

      PortNode nodeStart = {ptr,con->port};
      PortNode nodeEnd = con->instConnectedTo;

      EdgeNode edge = {nodeStart,nodeEnd};
      int calculatedDelay = delays.edgesDelay->GetOrFail(edge);

      bool highlighStart = (highlighInstance ? start.inst == highlighInstance : false);
      bool highlighEnd = (highlighInstance ? end.inst == highlighInstance : false);

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

String PushDebugPath(Arena* out,String folderName,String fileName){
  String path = {};
  if(folderName.size == 0){
    CreateDirectories(StaticFormat("%.*s",UNPACK_SS(globalOptions.debugPath)));
    path = PushString(out,"%.*s/%.*s",UNPACK_SS(globalOptions.debugPath),UNPACK_SS(fileName));
  } else {
    CreateDirectories(StaticFormat("%.*s/%.*s",UNPACK_SS(globalOptions.debugPath),UNPACK_SS(folderName)));
    path = PushString(out,"%.*s/%.*s/%.*s",UNPACK_SS(globalOptions.debugPath),UNPACK_SS(folderName),UNPACK_SS(fileName));
  }

  return path;
}
