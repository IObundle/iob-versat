#include "debugVersat.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

#include <algorithm>

#include "globals.hpp"
#include "memory.hpp"
#include "type.hpp"
#include "utilsCore.hpp"

#include "textualRepresentation.hpp"
#include "versat.hpp"
#include "accelerator.hpp"

static void OutputGraphDotFile_(Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlighInstance,FILE* outputFile,Arena* temp){
  BLOCK_REGION(temp);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
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
  Hashmap<Pair<FUInstance*,FUInstance*>,int>* seen = PushHashmap<Pair<FUInstance*,FUInstance*>,int>(temp,size);

  // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* out = ptr;

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      FUInstance* in = con->instConnectedTo.inst;

      if(collapseSameEdges){
        Pair<FUInstance*,FUInstance*> nodeEdge = {};
        nodeEdge.first = ptr;
        nodeEdge.second = con->instConnectedTo.inst;

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
      int calculatedDelay = con->delay.value ? *con->delay.value : 0;

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
  if(!globalOptions.debug){
    return;
  }

  FILE* file = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(file);
  OutputGraphDotFile_(accel,collapseSameEdges,nullptr,file,temp);
}

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,String filename,Arena* temp){
  if(!globalOptions.debug){
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
  if(!globalOptions.debug){
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
  if(!globalOptions.debug){
    return;
  }

  FILE* outputFile = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filename)),"w");
  DEFER_CLOSE_FILE(outputFile);
  
  BLOCK_REGION(temp);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
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
  Hashmap<Pair<FUInstance*,FUInstance*>,int>* seen = PushHashmap<Pair<FUInstance*,FUInstance*>,int>(temp,size);
  // TODO: Consider adding a true same edge counter, that collects edges with equal delay and then represents them on the graph as a pair, using [portStart-portEnd]
  // TODO: Change to use Array<Edge> 

  EdgeIterator iter = IterateEdges(accel);
  while(iter.HasNext()){
    Edge edge = iter.Next();
    FUInstance* out = edge.out.inst;

    FUInstance* in = edge.in.inst;
    int inPort = edge.in.port;
    int outPort = edge.out.port;

    if(collapseSameEdges){
      Pair<FUInstance*,FUInstance*> nodeEdge = {};
      nodeEdge.first = out;
      nodeEdge.second = in;

      GetOrAllocateResult<int> res = seen->GetOrAllocate(nodeEdge);
      if(res.alreadyExisted){
        continue;
      }
    }

    String first = UniqueRepr(out,temp);
    String second = UniqueRepr(in,temp);
    PortInstance start = {out,outPort};
    PortInstance end = {in,inPort};
    String label = Repr(&start,&end,globalDebug.dotFormat,temp);

    int calculatedDelay = delays.edgesDelay->GetOrFail(edge).value;

    bool highlighStart = (highlighInstance ? start.inst == highlighInstance : false);
    bool highlighEnd = (highlighInstance ? end.inst == highlighInstance : false);

    bool highlight = highlighStart && highlighEnd;

    fprintf(outputFile,"\t\"%.*s\" -> ",UNPACK_SS(first));
    fprintf(outputFile,"\"%.*s\"",UNPACK_SS(second));

    if(highlight){
      fprintf(outputFile,"[color=darkred,label=\"%.*s\\n[%d]\"];\n",UNPACK_SS(label),calculatedDelay);
    } else {
      fprintf(outputFile,"[label=\"%.*s\\n[:%d]\"];\n",UNPACK_SS(label),calculatedDelay);
    }
  }
  fprintf(outputFile,"}\n");
}

String PushDebugPath(Arena* out,String folderName,String fileName){
  Assert(globalOptions.debug);
  Assert(!Contains(fileName,"/"));
  Assert(fileName.size != 0);
  
  const char* fullFolderPath = nullptr;
  if(folderName.size == 0){
    fullFolderPath = StaticFormat("%.*s",UNPACK_SS(globalOptions.debugPath));
  } else {
    fullFolderPath = StaticFormat("%.*s/%.*s",UNPACK_SS(globalOptions.debugPath),UNPACK_SS(folderName));
  }

  CreateDirectories(fullFolderPath);
  String path = PushString(out,"%s/%.*s",fullFolderPath,UNPACK_SS(fileName));
  
  return path;
}

String PushDebugPath(Arena* out,String folderName,String subFolder,String fileName){
  Assert(globalOptions.debug);
  Assert(!Contains(fileName,"/"));
  Assert(folderName.size != 0);
  Assert(subFolder.size != 0);
  Assert(fileName.size != 0);

  const char* fullFolderPath = StaticFormat("%.*s/%.*s/%.*s",UNPACK_SS(globalOptions.debugPath),UNPACK_SS(folderName),UNPACK_SS(subFolder));

  CreateDirectories(fullFolderPath);
  String path = PushString(out,"%s/%.*s",fullFolderPath,UNPACK_SS(fileName));
  
  return path;
}
