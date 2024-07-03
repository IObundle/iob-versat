#include "dotGraphPrinting.hpp"

#include "accelerator.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"

// Make sure that it matches the value of the corresponding enum
static String graphPrintingColorTable[] = {
  STRING("dark"),
  STRING("darkblue"),
  STRING("darkred"),
  STRING("darkgreen"),
  STRING("darkyellow")
};

GraphPrintingColor DefaultNodeColor(FUInstance* node){
  GraphPrintingColor color = GraphPrintingColor_BLUE;
    
  if(node->type == NodeType_SOURCE || node->type == NodeType_SOURCE_AND_SINK){
    color = GraphPrintingColor_GREEN;
  } else if(node->type == NodeType_SINK){
    color = GraphPrintingColor_BLACK;
  }

  return color;
}

Pair<String,GraphPrintingColor> DefaultNodeContent(FUInstance* node,Arena* out){
  return {node->name,DefaultNodeColor(node)};
}

Pair<String,GraphPrintingColor> DefaultEdgeContent(Edge* edge,Arena* out){
  int inPort = edge->in.port;
  int outPort = edge->out.port;

  String content = PushString(out,"%d -> %d",outPort,inPort);

  return {content,GraphPrintingColor_BLACK};
}

NodeContent defaultNodeContent = DefaultNodeContent;
EdgeContent defaultEdgeContent = DefaultEdgeContent;

String UniqueInstanceName(FUInstance* inst,Arena* out){
 String name = PushString(out,"%p",inst);
  
  return name;
}

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Array<Edge> edges = GetAllEdges(accel,temp);

  ArenaList<GraphPrintingNodeInfo>* nodeInfo = PushArenaList<GraphPrintingNodeInfo>(temp);
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
    Pair<String,GraphPrintingColor> content = nodeFunction(ptr,out);
    String name = UniqueInstanceName(inst,out);

    *nodeInfo->PushElem() = {.name = name,.content = content.first,.color = content.second};
  }
  Array<GraphPrintingNodeInfo> nodes = PushArrayFromList(out,nodeInfo);

  ArenaList<GraphPrintingEdgeInfo>* edgeInfo = PushArenaList<GraphPrintingEdgeInfo>(temp);
  for(Edge edge : edges){
    Pair<String,GraphPrintingColor> content = edgeFunction(&edge,out);
    String first = UniqueInstanceName(edge.out.inst,out);
    String second = UniqueInstanceName(edge.in.inst,out); 

    *edgeInfo->PushElem() = {
      .content = content.first,
      .color = content.second,
      .first = first,
      .second = second
    };
  }
  Array<GraphPrintingEdgeInfo> edgeResult = PushArrayFromList(out,edgeInfo);
  
  GraphPrintingContent content = {};
  content.nodes = nodes;
  content.edges = edgeResult;

  return content;
}

GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out,Arena* temp){
  return GeneratePrintingContent(accel,defaultNodeContent,defaultEdgeContent,out,temp);
}

String GenerateDotGraph(Accelerator* accel,GraphPrintingContent content,Arena* out,Arena* temp){
  auto result = StartString(out);

  PushString(out,"digraph view {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");

  if(content.graphLabel.size > 0){
    PushString(out,"label=\"%.*s\";\n",UNPACK_SS(content.graphLabel));
  }
  
  for(GraphPrintingNodeInfo& node : content.nodes){
    String color = graphPrintingColorTable[(int) node.color];
    String name = node.name;
    String label = node.content;
    PushString(out,"\t\"%.*s\" [color=%.*s label=\"%.*s\"];\n",UNPACK_SS(name),UNPACK_SS(color),UNPACK_SS(label));
  }

  for(GraphPrintingEdgeInfo info : content.edges){
    PushString(out,"\t\"%.*s\" -> ",UNPACK_SS(info.first));
    PushString(out,"\"%.*s\"",UNPACK_SS(info.second));

    String color = graphPrintingColorTable[(int) info.color];
    String label = info.content;
      
    PushString(out,"[color=%.*s label=\"%.*s\"];\n",UNPACK_SS(color),UNPACK_SS(label));
  }

  PushString(out,"}\n");
    
  return EndString(result);
}
