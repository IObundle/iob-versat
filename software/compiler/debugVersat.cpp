#include "debugVersat.hpp"
#include "declaration.hpp"
#include "globals.hpp"

// Make sure that it matches the value of the corresponding enum
static String graphPrintingColorTable[] = {
  "dark",
  "darkblue",
  "darkred",
  "darkgreen",
  "darkyellow"
};

Color DefaultNodeColor(FUInstance* inst){
  Color color = Color_BLUE;

  if(inst->declaration == BasicDeclaration::input){
    color = Color_GREEN;
  }
  if(inst->declaration == BasicDeclaration::output){
    color = Color_BLACK;
  }
  
  return color;
}

GraphInfo DefaultNodeContent(FUInstance* inst,Arena* out){
  String str = PushString(out,"%.*s_%d",UN(inst->name),inst->id);
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

  ArenaList<GraphPrintingNodeInfo>* nodeInfo = PushList<GraphPrintingNodeInfo>(temp);
  for(FUInstance* inst : accel->allocated){
    GraphInfo content = nodeFunction(inst,out);
    String name = UniqueInstanceName(inst,out);

    *nodeInfo->PushElem() = {.name = name,.content = content.content,.color = content.color};
  }
  Array<GraphPrintingNodeInfo> nodes = PushArray(out,nodeInfo);

  ArenaList<GraphPrintingEdgeInfo>* edgeInfo = PushList<GraphPrintingEdgeInfo>(temp);
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
  Array<GraphPrintingEdgeInfo> edgeResult = PushArray(out,edgeInfo);
  
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
    result->PushString("label=\"%.*s\";\n",UN(content.graphLabel));
  }
  
  for(GraphPrintingNodeInfo& node : content.nodes){
    String color = graphPrintingColorTable[(int) node.color];
    String name = node.name;
    String label = node.content;
    result->PushString("\t\"%.*s\" [color=%.*s label=\"%.*s\"];\n",UN(name),UN(color),UN(label));
  }

  for(GraphPrintingEdgeInfo info : content.edges){
    result->PushString("\t\"%.*s\" -> ",UN(info.firstNode));
    result->PushString("\"%.*s\"",UN(info.secondNode));

    String color = graphPrintingColorTable[(int) info.color];
    String label = info.content;
      
    result->PushString("[color=%.*s label=\"%.*s\"];\n",UN(color),UN(label));
  }

  result->PushString("}\n");

  return EndString(out,result);
}

void OutputContentToFile(String filepath,String content){
  FILE* file = OpenFileAndCreateDirectories(filepath,"w",FilePurpose_DEBUG_INFO);
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    printf("[OutputContentToFile] Error opening file: %.*s\n",UN(filepath));
    return;
  }

  int res = fwrite(content.data,sizeof(char),content.size,file);
  Assert(res == content.size);
}

String PushDebugPath(Arena* out,String folderName,String fileName){
  Assert(globalOptions.debug);
  Assert(!Contains(fileName,"/"));
  Assert(fileName.size != 0);
  
  const char* fullFolderPath = nullptr;
  if(folderName.size == 0){
    fullFolderPath = StaticFormat("%.*s",UN(globalOptions.debugPath));
  } else {
    fullFolderPath = StaticFormat("%.*s/%.*s",UN(globalOptions.debugPath),UN(folderName));
  }

  CreateDirectories(fullFolderPath);
  String path = PushString(out,"%s/%.*s",fullFolderPath,UN(fileName));
  
  return path;
}

// ============================================================================
// Debug path regions

static Array<String> debugRegionStack;
static int debugRegionIndex;
static Arena debugRegionArenaInst;
static Arena* debugRegionArena;
static bool debugRegionInit;

DebugPathMarker::DebugPathMarker(String name){
  if(!debugRegionInit){
    debugRegionArenaInst = InitArena(Megabyte(16));
    debugRegionArena = &debugRegionArenaInst;
    debugRegionStack = PushArray<String>(debugRegionArena,10);
    
    debugRegionInit = true;
  }
  
  // TODO: This will eventually exhaust the arena since we do not free anything.
  //       Easy to fix by just storing the arena mark.
  debugRegionStack[debugRegionIndex] = PushString(debugRegionArena,name);

  debugRegionIndex += 1;
}

DebugPathMarker::~DebugPathMarker(){
  debugRegionIndex -= 1;
}

String GetDebugRegionFilepath(String filename,Arena* out){
  TEMP_REGION(temp,out);
  
  auto* builder = StartString(temp);
  
  builder->PushString(globalOptions.debugPath);

  if(debugRegionIndex == 0){
    builder->PushString("/TOP");
  } else {
    for(int i = 0; i < debugRegionIndex; i++){
      String component = debugRegionStack[i];
      builder->PushString("/");
      builder->PushString(component);
    }
  }
    
  builder->PushString("/");
  builder->PushString(filename);

  return EndString(out,builder);
}

void DebugRegionOutputDotGraph(Accelerator* accel,String fileName){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  String filePath = GetDebugRegionFilepath(fileName,temp);
  
  GraphPrintingContent content = GenerateDefaultPrintingContent(accel,temp);
  String result = GenerateDotGraph(content,temp);
  OutputContentToFile(filePath,result);
}

void DebugRegionOutputLatencyGraph(Accelerator* accel,NodeDelay* nodeDelay,PortDelay* portDelay,EdgeDelay* edgeToDelay,String fileName){
  if(!globalOptions.debug){
    return;
  }

  TEMP_REGION(temp,nullptr);

  int size = nodeDelay->size;
  Array<GraphPrintingNodeInfo> nodeArray = PushArray<GraphPrintingNodeInfo>(temp,size);
  int index = 0;
  for(Pair<FUInstance*,DelayInfo*> p : nodeDelay){
    FUInstance* node = p.first;
    DelayInfo delay = *p.second;
    
    nodeArray[index].name = PushString(temp,node->name);
    nodeArray[index].content = PushString(temp,"%.*s:%d:%d",UN(node->name),delay.value,node->literal);
    nodeArray[index].color = Color_BLACK;

    index += 1;
  }

  int totalEdges = edgeToDelay->size;

  Array<GraphPrintingEdgeInfo> edgeArray = PushArray<GraphPrintingEdgeInfo>(temp,totalEdges);
  int edgeIndex = 0;
  for(Pair<Edge,DelayInfo*> p : edgeToDelay){
    Edge edge = p.first;
    DelayInfo edgeLatency = *p.second;

    if(edgeLatency.isAny){
      edgeArray[edgeIndex].color = Color_BLUE;
    } else {
      edgeArray[edgeIndex].color = Color_BLACK;
    }
    edgeArray[edgeIndex].content = PushString(temp,"%d",edgeLatency.value);
    edgeArray[edgeIndex].firstNode = edge.units[0].inst->name;
    edgeArray[edgeIndex].secondNode = edge.units[1].inst->name;

    edgeIndex += 1;
  }
    
  GraphPrintingContent printContent = {};
  printContent.edges = edgeArray;
  printContent.nodes = nodeArray;
  printContent.graphLabel = "Nodes and edges contain their global latency (nodes also contain special)";

  String result = GenerateDotGraph(printContent,temp);

  String filePath = GetDebugRegionFilepath(fileName,temp);
  OutputContentToFile(filePath,result);
}
