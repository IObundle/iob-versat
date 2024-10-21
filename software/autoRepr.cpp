// File auto generated by structParser.py
#include "autoRepr.hpp"

String GetRepr(LogModule* e,Arena* out){
  switch(*e){
  case LogModule::MEMORY: return PushString(out,STRING("MEMORY"));
  case LogModule::TOP_SYS: return PushString(out,STRING("TOP_SYS"));
  case LogModule::ACCELERATOR: return PushString(out,STRING("ACCELERATOR"));
  case LogModule::PARSER: return PushString(out,STRING("PARSER"));
  case LogModule::TYPE: return PushString(out,STRING("TYPE"));
  case LogModule::TEMPLATE: return PushString(out,STRING("TEMPLATE"));
  case LogModule::UTILS: return PushString(out,STRING("UTILS"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(LogLevel* e,Arena* out){
  switch(*e){
  case LogLevel::DEBUG: return PushString(out,STRING("DEBUG"));
  case LogLevel::INFO: return PushString(out,STRING("INFO"));
  case LogLevel::WARN: return PushString(out,STRING("WARN"));
  case LogLevel::ERROR: return PushString(out,STRING("ERROR"));
  case LogLevel::FATAL: return PushString(out,STRING("FATAL"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(Subtype* e,Arena* out){
  switch(*e){
  case Subtype::Subtype_UNKNOWN: return PushString(out,STRING("Subtype_UNKNOWN"));
  case Subtype::Subtype_BASE: return PushString(out,STRING("Subtype_BASE"));
  case Subtype::Subtype_STRUCT: return PushString(out,STRING("Subtype_STRUCT"));
  case Subtype::Subtype_POINTER: return PushString(out,STRING("Subtype_POINTER"));
  case Subtype::Subtype_ARRAY: return PushString(out,STRING("Subtype_ARRAY"));
  case Subtype::Subtype_TEMPLATED_STRUCT_DEF: return PushString(out,STRING("Subtype_TEMPLATED_STRUCT_DEF"));
  case Subtype::Subtype_TEMPLATED_INSTANCE: return PushString(out,STRING("Subtype_TEMPLATED_INSTANCE"));
  case Subtype::Subtype_ENUM: return PushString(out,STRING("Subtype_ENUM"));
  case Subtype::Subtype_TYPEDEF: return PushString(out,STRING("Subtype_TYPEDEF"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(CommandType* e,Arena* out){
  switch(*e){
  case CommandType::CommandType_JOIN: return PushString(out,STRING("CommandType_JOIN"));
  case CommandType::CommandType_FOR: return PushString(out,STRING("CommandType_FOR"));
  case CommandType::CommandType_IF: return PushString(out,STRING("CommandType_IF"));
  case CommandType::CommandType_END: return PushString(out,STRING("CommandType_END"));
  case CommandType::CommandType_SET: return PushString(out,STRING("CommandType_SET"));
  case CommandType::CommandType_LET: return PushString(out,STRING("CommandType_LET"));
  case CommandType::CommandType_INC: return PushString(out,STRING("CommandType_INC"));
  case CommandType::CommandType_ELSE: return PushString(out,STRING("CommandType_ELSE"));
  case CommandType::CommandType_DEBUG: return PushString(out,STRING("CommandType_DEBUG"));
  case CommandType::CommandType_INCLUDE: return PushString(out,STRING("CommandType_INCLUDE"));
  case CommandType::CommandType_DEFINE: return PushString(out,STRING("CommandType_DEFINE"));
  case CommandType::CommandType_CALL: return PushString(out,STRING("CommandType_CALL"));
  case CommandType::CommandType_WHILE: return PushString(out,STRING("CommandType_WHILE"));
  case CommandType::CommandType_RETURN: return PushString(out,STRING("CommandType_RETURN"));
  case CommandType::CommandType_FORMAT: return PushString(out,STRING("CommandType_FORMAT"));
  case CommandType::CommandType_DEBUG_MESSAGE: return PushString(out,STRING("CommandType_DEBUG_MESSAGE"));
  case CommandType::CommandType_DEBUG_BREAK: return PushString(out,STRING("CommandType_DEBUG_BREAK"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(BlockType* e,Arena* out){
  switch(*e){
  case BlockType::BlockType_TEXT: return PushString(out,STRING("BlockType_TEXT"));
  case BlockType::BlockType_COMMAND: return PushString(out,STRING("BlockType_COMMAND"));
  case BlockType::BlockType_EXPRESSION: return PushString(out,STRING("BlockType_EXPRESSION"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(TemplateRecordType* e,Arena* out){
  switch(*e){
  case TemplateRecordType::TemplateRecordType_FIELD: return PushString(out,STRING("TemplateRecordType_FIELD"));
  case TemplateRecordType::TemplateRecordType_IDENTIFER: return PushString(out,STRING("TemplateRecordType_IDENTIFER"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(VersatStage* e,Arena* out){
  switch(*e){
  case VersatStage::VersatStage_COMPUTE: return PushString(out,STRING("VersatStage_COMPUTE"));
  case VersatStage::VersatStage_READ: return PushString(out,STRING("VersatStage_READ"));
  case VersatStage::VersatStage_WRITE: return PushString(out,STRING("VersatStage_WRITE"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(ExternalMemoryType* e,Arena* out){
  switch(*e){
  case ExternalMemoryType::TWO_P: return PushString(out,STRING("TWO_P"));
  case ExternalMemoryType::DP: return PushString(out,STRING("DP"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(NodeType* e,Arena* out){
  switch(*e){
  case NodeType::NodeType_UNCONNECTED: return PushString(out,STRING("NodeType_UNCONNECTED"));
  case NodeType::NodeType_SOURCE: return PushString(out,STRING("NodeType_SOURCE"));
  case NodeType::NodeType_COMPUTE: return PushString(out,STRING("NodeType_COMPUTE"));
  case NodeType::NodeType_SINK: return PushString(out,STRING("NodeType_SINK"));
  case NodeType::NodeType_SOURCE_AND_SINK: return PushString(out,STRING("NodeType_SOURCE_AND_SINK"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(AcceleratorPurpose* e,Arena* out){
  switch(*e){
  case AcceleratorPurpose::AcceleratorPurpose_TEMP: return PushString(out,STRING("AcceleratorPurpose_TEMP"));
  case AcceleratorPurpose::AcceleratorPurpose_BASE: return PushString(out,STRING("AcceleratorPurpose_BASE"));
  case AcceleratorPurpose::AcceleratorPurpose_FIXED_DELAY: return PushString(out,STRING("AcceleratorPurpose_FIXED_DELAY"));
  case AcceleratorPurpose::AcceleratorPurpose_FLATTEN: return PushString(out,STRING("AcceleratorPurpose_FLATTEN"));
  case AcceleratorPurpose::AcceleratorPurpose_MODULE: return PushString(out,STRING("AcceleratorPurpose_MODULE"));
  case AcceleratorPurpose::AcceleratorPurpose_RECON: return PushString(out,STRING("AcceleratorPurpose_RECON"));
  case AcceleratorPurpose::AcceleratorPurpose_MERGE: return PushString(out,STRING("AcceleratorPurpose_MERGE"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(MemType* e,Arena* out){
  switch(*e){
  case MemType::CONFIG: return PushString(out,STRING("CONFIG"));
  case MemType::STATE: return PushString(out,STRING("STATE"));
  case MemType::DELAY: return PushString(out,STRING("DELAY"));
  case MemType::STATIC: return PushString(out,STRING("STATIC"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(Color* e,Arena* out){
  switch(*e){
  case Color::Color_BLACK: return PushString(out,STRING("Color_BLACK"));
  case Color::Color_BLUE: return PushString(out,STRING("Color_BLUE"));
  case Color::Color_RED: return PushString(out,STRING("Color_RED"));
  case Color::Color_GREEN: return PushString(out,STRING("Color_GREEN"));
  case Color::Color_YELLOW: return PushString(out,STRING("Color_YELLOW"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(VersatDebugFlags* e,Arena* out){
  switch(*e){
  case VersatDebugFlags::OUTPUT_GRAPH_DOT: return PushString(out,STRING("OUTPUT_GRAPH_DOT"));
  case VersatDebugFlags::GRAPH_DOT_FORMAT: return PushString(out,STRING("GRAPH_DOT_FORMAT"));
  case VersatDebugFlags::OUTPUT_ACCELERATORS_CODE: return PushString(out,STRING("OUTPUT_ACCELERATORS_CODE"));
  case VersatDebugFlags::OUTPUT_VERSAT_CODE: return PushString(out,STRING("OUTPUT_VERSAT_CODE"));
  case VersatDebugFlags::USE_FIXED_BUFFERS: return PushString(out,STRING("USE_FIXED_BUFFERS"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(GraphDotFormat_* e,Arena* out){
  switch(*e){
  case GraphDotFormat_::GRAPH_DOT_FORMAT_NAME: return PushString(out,STRING("GRAPH_DOT_FORMAT_NAME"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_TYPE: return PushString(out,STRING("GRAPH_DOT_FORMAT_TYPE"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_ID: return PushString(out,STRING("GRAPH_DOT_FORMAT_ID"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_DELAY: return PushString(out,STRING("GRAPH_DOT_FORMAT_DELAY"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_EXPLICIT: return PushString(out,STRING("GRAPH_DOT_FORMAT_EXPLICIT"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_PORT: return PushString(out,STRING("GRAPH_DOT_FORMAT_PORT"));
  case GraphDotFormat_::GRAPH_DOT_FORMAT_LATENCY: return PushString(out,STRING("GRAPH_DOT_FORMAT_LATENCY"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(DelayType* e,Arena* out){
  switch(*e){
  case DelayType::DelayType_BASE: return PushString(out,STRING("DelayType_BASE"));
  case DelayType::DelayType_SINK_DELAY: return PushString(out,STRING("DelayType_SINK_DELAY"));
  case DelayType::DelayType_SOURCE_DELAY: return PushString(out,STRING("DelayType_SOURCE_DELAY"));
  case DelayType::DelayType_COMPUTE_DELAY: return PushString(out,STRING("DelayType_COMPUTE_DELAY"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(FUDeclarationType* e,Arena* out){
  switch(*e){
  case FUDeclarationType::FUDeclarationType_SINGLE: return PushString(out,STRING("FUDeclarationType_SINGLE"));
  case FUDeclarationType::FUDeclarationType_COMPOSITE: return PushString(out,STRING("FUDeclarationType_COMPOSITE"));
  case FUDeclarationType::FUDeclarationType_SPECIAL: return PushString(out,STRING("FUDeclarationType_SPECIAL"));
  case FUDeclarationType::FUDeclarationType_MERGED: return PushString(out,STRING("FUDeclarationType_MERGED"));
  case FUDeclarationType::FUDeclarationType_ITERATIVE: return PushString(out,STRING("FUDeclarationType_ITERATIVE"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(MergingStrategy* e,Arena* out){
  switch(*e){
  case MergingStrategy::SIMPLE_COMBINATION: return PushString(out,STRING("SIMPLE_COMBINATION"));
  case MergingStrategy::CONSOLIDATION_GRAPH: return PushString(out,STRING("CONSOLIDATION_GRAPH"));
  case MergingStrategy::FIRST_FIT: return PushString(out,STRING("FIRST_FIT"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(ConnectionType* e,Arena* out){
  switch(*e){
  case ConnectionType::ConnectionType_SINGLE: return PushString(out,STRING("ConnectionType_SINGLE"));
  case ConnectionType::ConnectionType_PORT_RANGE: return PushString(out,STRING("ConnectionType_PORT_RANGE"));
  case ConnectionType::ConnectionType_ARRAY_RANGE: return PushString(out,STRING("ConnectionType_ARRAY_RANGE"));
  case ConnectionType::ConnectionType_DELAY_RANGE: return PushString(out,STRING("ConnectionType_DELAY_RANGE"));
  case ConnectionType::ConnectionType_ERROR: return PushString(out,STRING("ConnectionType_ERROR"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
String GetRepr(DefinitionType* e,Arena* out){
  switch(*e){
  case DefinitionType::DefinitionType_MODULE: return PushString(out,STRING("DefinitionType_MODULE"));
  case DefinitionType::DefinitionType_MERGE: return PushString(out,STRING("DefinitionType_MERGE"));
  case DefinitionType::DefinitionType_ITERATIVE: return PushString(out,STRING("DefinitionType_ITERATIVE"));
  }
  return STRING("NOT POSSIBLE ENUM VALUE");
}
