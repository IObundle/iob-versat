#include "debugVersat.hpp"

#include <cstdio>
#include <cstdarg>
#include <set>

#include <algorithm>

#include "filesystem.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"

#include "textualRepresentation.hpp"
#include "versat.hpp"
#include "accelerator.hpp"

// Make sure that it matches the value of the corresponding enum
static String graphPrintingColorTable[] = {
  STRING("dark"),
  STRING("darkblue"),
  STRING("darkred"),
  STRING("darkgreen"),
  STRING("darkyellow")
};

Color DefaultNodeColor(FUInstance* inst){
  Color color = Color_BLUE;
    
  if(inst->type == NodeType_SOURCE || inst->type == NodeType_SOURCE_AND_SINK){
    color = Color_GREEN;
  } else if(inst->type == NodeType_SINK){
    color = Color_BLACK;
  }

  return color;
}

GraphInfo DefaultNodeContent(FUInstance* inst,Arena* out){
  String str = PushString(out,"%.*s_%d",UNPACK_SS(inst->name),inst->id);
  return {str,DefaultNodeColor(inst)};
}

GraphInfo DefaultEdgeContent(Edge* edge,Arena* out){
  int inPort = edge->in.port;
  int outPort = edge->out.port;

  String content = PushString(out,"%d -> %d:%d",outPort,inPort,edge->delay);

  return {content,Color_BLACK};
}

NodeContent defaultNodeContent = DefaultNodeContent;
EdgeContent defaultEdgeContent = DefaultEdgeContent;

String UniqueInstanceName(FUInstance* inst,Arena* out){
 String name = PushString(out,"%p",inst);
  
  return name;
}

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out){
  TEMP_REGION(temp,out);

  Array<Edge> edges = GetAllEdges(accel,temp);

  ArenaList<GraphPrintingNodeInfo>* nodeInfo = PushArenaList<GraphPrintingNodeInfo>(temp);
  for(FUInstance* inst : accel->allocated){
    GraphInfo content = nodeFunction(inst,out);
    String name = UniqueInstanceName(inst,out);

    *nodeInfo->PushElem() = {.name = name,.content = content.content,.color = content.color};
  }
  Array<GraphPrintingNodeInfo> nodes = PushArrayFromList(out,nodeInfo);

  ArenaList<GraphPrintingEdgeInfo>* edgeInfo = PushArenaList<GraphPrintingEdgeInfo>(temp);
  for(Edge edge : edges){
    GraphInfo content = edgeFunction(&edge,out);
    String first = UniqueInstanceName(edge.out.inst,out);
    String second = UniqueInstanceName(edge.in.inst,out); 

    *edgeInfo->PushElem() = {
      .content = content.content,
      .color = content.color,
      .firstNode = first,
      .secondNode = second
    };
  }
  Array<GraphPrintingEdgeInfo> edgeResult = PushArrayFromList(out,edgeInfo);
  
  GraphPrintingContent content = {};
  content.nodes = nodes;
  content.edges = edgeResult;

  return content;
}

GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out){
  GraphPrintingContent content = GeneratePrintingContent(accel,defaultNodeContent,defaultEdgeContent,out);

  content.graphLabel = PushString(out,"%d",accel->id);
  
  return content;
}

String GenerateDotGraph(GraphPrintingContent content,Arena* out){
  TEMP_REGION(temp,out);
  auto result = StartString(temp);

  result->PushString("digraph view {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");

  if(content.graphLabel.size > 0){
    result->PushString("label=\"%.*s\";\n",UNPACK_SS(content.graphLabel));
  }
  
  for(GraphPrintingNodeInfo& node : content.nodes){
    String color = graphPrintingColorTable[(int) node.color];
    String name = node.name;
    String label = node.content;
    result->PushString("\t\"%.*s\" [color=%.*s label=\"%.*s\"];\n",UNPACK_SS(name),UNPACK_SS(color),UNPACK_SS(label));
  }

  for(GraphPrintingEdgeInfo info : content.edges){
    result->PushString("\t\"%.*s\" -> ",UNPACK_SS(info.firstNode));
    result->PushString("\"%.*s\"",UNPACK_SS(info.secondNode));

    String color = graphPrintingColorTable[(int) info.color];
    String label = info.content;
      
    result->PushString("[color=%.*s label=\"%.*s\"];\n",UNPACK_SS(color),UNPACK_SS(label));
  }

  result->PushString("}\n");
    
  return EndString(out,result);
}

void OutputDebugDotGraph(Accelerator* accel,String folder,String fileName){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  String trueFileName = PushString(temp,fileName); // Make it safe to use StaticFormat outside this function
  String filePath = PushDebugPath(temp,folder,trueFileName);
      
  GraphPrintingContent content = GenerateDefaultPrintingContent(accel,temp);
  String result = GenerateDotGraph(content,temp);
  OutputContentToFile(filePath,result);
}

void OutputDebugDotGraph(Accelerator* accel,String fileName){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  String trueFileName = PushString(temp,fileName); // Make it safe to use StaticFormat outside this function
  
  String filePath = PushDebugPath(temp,accel->name,trueFileName);
      
  GraphPrintingContent content = GenerateDefaultPrintingContent(accel,temp);
  String result = GenerateDotGraph(content,temp);
  OutputContentToFile(filePath,result);
}

void OutputDebugDotGraph(Accelerator* accel,String fileName,FUInstance* highlight){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  String trueFileName = PushString(temp,fileName); // Make it safe to use StaticFormat outside this function
  
  String filePath = PushDebugPath(temp,accel->name,trueFileName);

  auto nodeFunction = [highlight](FUInstance* inst,Arena* out) -> GraphInfo {
    GraphInfo def = DefaultNodeContent(inst,out);
    if(inst == highlight){
      Assert(def.color != Color_GREEN);
      return {def.content,Color_GREEN};
    } else {
      return def;
    }
  };
  
  GraphPrintingContent content = GeneratePrintingContent(accel,nodeFunction,defaultEdgeContent,temp);
  String result = GenerateDotGraph(content,temp);
  OutputContentToFile(filePath,result);
}

void OutputDebugDotGraph(Accelerator* accel,String fileName,Set<FUInstance*>* highlight){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  String trueFileName = PushString(temp,fileName); // Make it safe to use StaticFormat outside this function
  
  String filePath = PushDebugPath(temp,accel->name,trueFileName);

  auto nodeFunction = [highlight](FUInstance* inst,Arena* out) -> GraphInfo {
    GraphInfo def = DefaultNodeContent(inst,out);
    if(highlight->Exists(inst)){
      Assert(def.color != Color_RED);
      return {def.content,Color_RED};
    } else {
      return def;
    }
  };
  
  GraphPrintingContent content = GeneratePrintingContent(accel,nodeFunction,defaultEdgeContent,temp);
  String result = GenerateDotGraph(content,temp);
  OutputContentToFile(filePath,result);
}

void OutputContentToFile(String filepath,String content){
  FILE* file = OpenFile(filepath,"w",FilePurpose_DEBUG_INFO);
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    printf("[OutputContentToFile] Error opening file: %.*s\n",UNPACK_SS(filepath));
    return;
  }

  int res = fwrite(content.data,sizeof(char),content.size,file);
  Assert(res == content.size);
}

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,CalculateDelayResult delays,String filename){
  TEMP_REGION(temp,nullptr);
  if(!globalOptions.debug){
    return;
  }

  FILE* outputFile = OpenFileAndCreateDirectories(filename,"w",FilePurpose_DEBUG_INFO);
  DEFER_CLOSE_FILE(outputFile);

  fprintf(outputFile,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  for(FUInstance* ptr : accel->allocated){
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
    PortInstance start = MakePortOut(out,outPort);
    PortInstance end = MakePortIn(in,inPort);
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
