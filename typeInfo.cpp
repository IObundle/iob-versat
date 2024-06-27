#pragma GCC diagnostic ignored "-Winvalid-offsetof"

#include "test.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "memory.hpp"
#include "templateEngine.hpp"
#include "parser.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "declaration.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"
#include "codeGeneration.hpp"
#include "versatSpecificationParser.hpp"

void RegisterParsedTypes(){
  RegisterOpaqueType(STRING("Test"),Type::STRUCT,sizeof(Test),alignof(Test));
  RegisterOpaqueType(STRING("Test3"),Type::STRUCT,sizeof(Test3),alignof(Test3));
  RegisterOpaqueType(STRING("Time"),Type::STRUCT,sizeof(Time),alignof(Time));
  RegisterOpaqueType(STRING("Conversion"),Type::STRUCT,sizeof(Conversion),alignof(Conversion));
  RegisterOpaqueType(STRING("Arena"),Type::STRUCT,sizeof(Arena),alignof(Arena));
  RegisterOpaqueType(STRING("ArenaMark"),Type::STRUCT,sizeof(ArenaMark),alignof(ArenaMark));
  RegisterOpaqueType(STRING("DynamicArena"),Type::STRUCT,sizeof(DynamicArena),alignof(DynamicArena));
  RegisterOpaqueType(STRING("DynamicString"),Type::STRUCT,sizeof(DynamicString),alignof(DynamicString));
  RegisterOpaqueType(STRING("BitArray"),Type::STRUCT,sizeof(BitArray),alignof(BitArray));
  RegisterOpaqueType(STRING("PoolHeader"),Type::STRUCT,sizeof(PoolHeader),alignof(PoolHeader));
  RegisterOpaqueType(STRING("PoolInfo"),Type::STRUCT,sizeof(PoolInfo),alignof(PoolInfo));
  RegisterOpaqueType(STRING("PageInfo"),Type::STRUCT,sizeof(PageInfo),alignof(PageInfo));
  RegisterOpaqueType(STRING("EnumMember"),Type::STRUCT,sizeof(EnumMember),alignof(EnumMember));
  RegisterOpaqueType(STRING("TemplateArg"),Type::STRUCT,sizeof(TemplateArg),alignof(TemplateArg));
  RegisterOpaqueType(STRING("TemplatedMember"),Type::STRUCT,sizeof(TemplatedMember),alignof(TemplatedMember));
  RegisterOpaqueType(STRING("Type"),Type::STRUCT,sizeof(Type),alignof(Type));
  RegisterOpaqueType(STRING("Member"),Type::STRUCT,sizeof(Member),alignof(Member));
  RegisterOpaqueType(STRING("TypeIterator"),Type::STRUCT,sizeof(TypeIterator),alignof(TypeIterator));
  RegisterOpaqueType(STRING("Value"),Type::STRUCT,sizeof(Value),alignof(Value));
  RegisterOpaqueType(STRING("Iterator"),Type::STRUCT,sizeof(Iterator),alignof(Iterator));
  RegisterOpaqueType(STRING("HashmapUnpackedIndex"),Type::STRUCT,sizeof(HashmapUnpackedIndex),alignof(HashmapUnpackedIndex));
  RegisterOpaqueType(STRING("Expression"),Type::STRUCT,sizeof(Expression),alignof(Expression));
  RegisterOpaqueType(STRING("Cursor"),Type::STRUCT,sizeof(Cursor),alignof(Cursor));
  RegisterOpaqueType(STRING("Token"),Type::STRUCT,sizeof(Token),alignof(Token));
  RegisterOpaqueType(STRING("FindFirstResult"),Type::STRUCT,sizeof(FindFirstResult),alignof(FindFirstResult));
  RegisterOpaqueType(STRING("Trie"),Type::STRUCT,sizeof(Trie),alignof(Trie));
  RegisterOpaqueType(STRING("TokenizerTemplate"),Type::STRUCT,sizeof(TokenizerTemplate),alignof(TokenizerTemplate));
  RegisterOpaqueType(STRING("TokenizerMark"),Type::STRUCT,sizeof(TokenizerMark),alignof(TokenizerMark));
  RegisterOpaqueType(STRING("Tokenizer"),Type::STRUCT,sizeof(Tokenizer),alignof(Tokenizer));
  RegisterOpaqueType(STRING("OperationList"),Type::STRUCT,sizeof(OperationList),alignof(OperationList));
  RegisterOpaqueType(STRING("CommandDefinition"),Type::STRUCT,sizeof(CommandDefinition),alignof(CommandDefinition));
  RegisterOpaqueType(STRING("Command"),Type::STRUCT,sizeof(Command),alignof(Command));
  RegisterOpaqueType(STRING("Block"),Type::STRUCT,sizeof(Block),alignof(Block));
  RegisterOpaqueType(STRING("TemplateFunction"),Type::STRUCT,sizeof(TemplateFunction),alignof(TemplateFunction));
  RegisterOpaqueType(STRING("TemplateRecord"),Type::STRUCT,sizeof(TemplateRecord),alignof(TemplateRecord));
  RegisterOpaqueType(STRING("ValueAndText"),Type::STRUCT,sizeof(ValueAndText),alignof(ValueAndText));
  RegisterOpaqueType(STRING("Frame"),Type::STRUCT,sizeof(Frame),alignof(Frame));
  RegisterOpaqueType(STRING("IndividualBlock"),Type::STRUCT,sizeof(IndividualBlock),alignof(IndividualBlock));
  RegisterOpaqueType(STRING("CompiledTemplate"),Type::STRUCT,sizeof(CompiledTemplate),alignof(CompiledTemplate));
  RegisterOpaqueType(STRING("PortDeclaration"),Type::STRUCT,sizeof(PortDeclaration),alignof(PortDeclaration));
  RegisterOpaqueType(STRING("ParameterExpression"),Type::STRUCT,sizeof(ParameterExpression),alignof(ParameterExpression));
  RegisterOpaqueType(STRING("Module"),Type::STRUCT,sizeof(Module),alignof(Module));
  RegisterOpaqueType(STRING("ExternalMemoryID"),Type::STRUCT,sizeof(ExternalMemoryID),alignof(ExternalMemoryID));
  RegisterOpaqueType(STRING("ExternalInfoTwoPorts"),Type::STRUCT,sizeof(ExternalInfoTwoPorts),alignof(ExternalInfoTwoPorts));
  RegisterOpaqueType(STRING("ExternalInfoDualPort"),Type::STRUCT,sizeof(ExternalInfoDualPort),alignof(ExternalInfoDualPort));
  RegisterOpaqueType(STRING("ExternalMemoryInfo"),Type::STRUCT,sizeof(ExternalMemoryInfo),alignof(ExternalMemoryInfo));
  RegisterOpaqueType(STRING("ModuleInfo"),Type::STRUCT,sizeof(ModuleInfo),alignof(ModuleInfo));
  RegisterOpaqueType(STRING("Options"),Type::STRUCT,sizeof(Options),alignof(Options));
  RegisterOpaqueType(STRING("DebugState"),Type::STRUCT,sizeof(DebugState),alignof(DebugState));
  RegisterOpaqueType(STRING("StaticId"),Type::STRUCT,sizeof(StaticId),alignof(StaticId));
  RegisterOpaqueType(STRING("StaticData"),Type::STRUCT,sizeof(StaticData),alignof(StaticData));
  RegisterOpaqueType(STRING("StaticInfo"),Type::STRUCT,sizeof(StaticInfo),alignof(StaticInfo));
  RegisterOpaqueType(STRING("CalculatedOffsets"),Type::STRUCT,sizeof(CalculatedOffsets),alignof(CalculatedOffsets));
  RegisterOpaqueType(STRING("InstanceInfo"),Type::STRUCT,sizeof(InstanceInfo),alignof(InstanceInfo));
  RegisterOpaqueType(STRING("AcceleratorInfo"),Type::STRUCT,sizeof(AcceleratorInfo),alignof(AcceleratorInfo));
  RegisterOpaqueType(STRING("InstanceConfigurationOffsets"),Type::STRUCT,sizeof(InstanceConfigurationOffsets),alignof(InstanceConfigurationOffsets));
  RegisterOpaqueType(STRING("TestResult"),Type::STRUCT,sizeof(TestResult),alignof(TestResult));
  RegisterOpaqueType(STRING("AccelInfo"),Type::STRUCT,sizeof(AccelInfo),alignof(AccelInfo));
  RegisterOpaqueType(STRING("TypeAndNameOnly"),Type::STRUCT,sizeof(TypeAndNameOnly),alignof(TypeAndNameOnly));
  RegisterOpaqueType(STRING("Partition"),Type::STRUCT,sizeof(Partition),alignof(Partition));
  RegisterOpaqueType(STRING("OrderedConfigurations"),Type::STRUCT,sizeof(OrderedConfigurations),alignof(OrderedConfigurations));
  RegisterOpaqueType(STRING("ConfigurationInfo"),Type::STRUCT,sizeof(ConfigurationInfo),alignof(ConfigurationInfo));
  RegisterOpaqueType(STRING("FUDeclaration"),Type::STRUCT,sizeof(FUDeclaration),alignof(FUDeclaration));
  RegisterOpaqueType(STRING("PortInstance"),Type::STRUCT,sizeof(PortInstance),alignof(PortInstance));
  RegisterOpaqueType(STRING("PortEdge"),Type::STRUCT,sizeof(PortEdge),alignof(PortEdge));
  RegisterOpaqueType(STRING("Edge"),Type::STRUCT,sizeof(Edge),alignof(Edge));
  RegisterOpaqueType(STRING("PortNode"),Type::STRUCT,sizeof(PortNode),alignof(PortNode));
  RegisterOpaqueType(STRING("EdgeNode"),Type::STRUCT,sizeof(EdgeNode),alignof(EdgeNode));
  RegisterOpaqueType(STRING("ConnectionNode"),Type::STRUCT,sizeof(ConnectionNode),alignof(ConnectionNode));
  RegisterOpaqueType(STRING("InstanceNode"),Type::STRUCT,sizeof(InstanceNode),alignof(InstanceNode));
  RegisterOpaqueType(STRING("Accelerator"),Type::STRUCT,sizeof(Accelerator),alignof(Accelerator));
  RegisterOpaqueType(STRING("MemoryAddressMask"),Type::STRUCT,sizeof(MemoryAddressMask),alignof(MemoryAddressMask));
  RegisterOpaqueType(STRING("VersatComputedValues"),Type::STRUCT,sizeof(VersatComputedValues),alignof(VersatComputedValues));
  RegisterOpaqueType(STRING("DAGOrderNodes"),Type::STRUCT,sizeof(DAGOrderNodes),alignof(DAGOrderNodes));
  RegisterOpaqueType(STRING("GraphPrintingNodeInfo"),Type::STRUCT,sizeof(GraphPrintingNodeInfo),alignof(GraphPrintingNodeInfo));
  RegisterOpaqueType(STRING("GraphPrintingEdgeInfo"),Type::STRUCT,sizeof(GraphPrintingEdgeInfo),alignof(GraphPrintingEdgeInfo));
  RegisterOpaqueType(STRING("GraphPrintingContent"),Type::STRUCT,sizeof(GraphPrintingContent),alignof(GraphPrintingContent));
  RegisterOpaqueType(STRING("CalculateDelayResult"),Type::STRUCT,sizeof(CalculateDelayResult),alignof(CalculateDelayResult));
  RegisterOpaqueType(STRING("FUInstance"),Type::STRUCT,sizeof(FUInstance),alignof(FUInstance));
  RegisterOpaqueType(STRING("DelayToAdd"),Type::STRUCT,sizeof(DelayToAdd),alignof(DelayToAdd));
  RegisterOpaqueType(STRING("SingleTypeStructElement"),Type::STRUCT,sizeof(SingleTypeStructElement),alignof(SingleTypeStructElement));
  RegisterOpaqueType(STRING("TypeStructInfoElement"),Type::STRUCT,sizeof(TypeStructInfoElement),alignof(TypeStructInfoElement));
  RegisterOpaqueType(STRING("TypeStructInfo"),Type::STRUCT,sizeof(TypeStructInfo),alignof(TypeStructInfo));
  RegisterOpaqueType(STRING("Difference"),Type::STRUCT,sizeof(Difference),alignof(Difference));
  RegisterOpaqueType(STRING("DifferenceArray"),Type::STRUCT,sizeof(DifferenceArray),alignof(DifferenceArray));
  RegisterOpaqueType(STRING("Task"),Type::STRUCT,sizeof(Task),alignof(Task));
  RegisterOpaqueType(STRING("WorkGroup"),Type::STRUCT,sizeof(WorkGroup),alignof(WorkGroup));
  RegisterOpaqueType(STRING("SpecificMerge"),Type::STRUCT,sizeof(SpecificMerge),alignof(SpecificMerge));
  RegisterOpaqueType(STRING("IndexRecord"),Type::STRUCT,sizeof(IndexRecord),alignof(IndexRecord));
  RegisterOpaqueType(STRING("SpecificMergeNodes"),Type::STRUCT,sizeof(SpecificMergeNodes),alignof(SpecificMergeNodes));
  RegisterOpaqueType(STRING("SpecificMergeNode"),Type::STRUCT,sizeof(SpecificMergeNode),alignof(SpecificMergeNode));
  RegisterOpaqueType(STRING("MergeEdge"),Type::STRUCT,sizeof(MergeEdge),alignof(MergeEdge));
  RegisterOpaqueType(STRING("MappingNode"),Type::STRUCT,sizeof(MappingNode),alignof(MappingNode));
  RegisterOpaqueType(STRING("MappingEdge"),Type::STRUCT,sizeof(MappingEdge),alignof(MappingEdge));
  RegisterOpaqueType(STRING("ConsolidationGraphOptions"),Type::STRUCT,sizeof(ConsolidationGraphOptions),alignof(ConsolidationGraphOptions));
  RegisterOpaqueType(STRING("ConsolidationGraph"),Type::STRUCT,sizeof(ConsolidationGraph),alignof(ConsolidationGraph));
  RegisterOpaqueType(STRING("ConsolidationResult"),Type::STRUCT,sizeof(ConsolidationResult),alignof(ConsolidationResult));
  RegisterOpaqueType(STRING("CliqueState"),Type::STRUCT,sizeof(CliqueState),alignof(CliqueState));
  RegisterOpaqueType(STRING("IsCliqueResult"),Type::STRUCT,sizeof(IsCliqueResult),alignof(IsCliqueResult));
  RegisterOpaqueType(STRING("MergeGraphResult"),Type::STRUCT,sizeof(MergeGraphResult),alignof(MergeGraphResult));
  RegisterOpaqueType(STRING("MergeGraphResultExisting"),Type::STRUCT,sizeof(MergeGraphResultExisting),alignof(MergeGraphResultExisting));
  RegisterOpaqueType(STRING("GraphMapping"),Type::STRUCT,sizeof(GraphMapping),alignof(GraphMapping));
  RegisterOpaqueType(STRING("ConnectionExtra"),Type::STRUCT,sizeof(ConnectionExtra),alignof(ConnectionExtra));
  RegisterOpaqueType(STRING("Var"),Type::STRUCT,sizeof(Var),alignof(Var));
  RegisterOpaqueType(STRING("VarGroup"),Type::STRUCT,sizeof(VarGroup),alignof(VarGroup));
  RegisterOpaqueType(STRING("SpecExpression"),Type::STRUCT,sizeof(SpecExpression),alignof(SpecExpression));
  RegisterOpaqueType(STRING("VarDeclaration"),Type::STRUCT,sizeof(VarDeclaration),alignof(VarDeclaration));
  RegisterOpaqueType(STRING("GroupIterator"),Type::STRUCT,sizeof(GroupIterator),alignof(GroupIterator));
  RegisterOpaqueType(STRING("PortExpression"),Type::STRUCT,sizeof(PortExpression),alignof(PortExpression));
  RegisterOpaqueType(STRING("InstanceDeclaration"),Type::STRUCT,sizeof(InstanceDeclaration),alignof(InstanceDeclaration));
  RegisterOpaqueType(STRING("ConnectionDef"),Type::STRUCT,sizeof(ConnectionDef),alignof(ConnectionDef));
  RegisterOpaqueType(STRING("ModuleDef"),Type::STRUCT,sizeof(ModuleDef),alignof(ModuleDef));
  RegisterOpaqueType(STRING("TransformDef"),Type::STRUCT,sizeof(TransformDef),alignof(TransformDef));
  RegisterOpaqueType(STRING("Transformation"),Type::STRUCT,sizeof(Transformation),alignof(Transformation));
  RegisterOpaqueType(STRING("HierarchicalName"),Type::STRUCT,sizeof(HierarchicalName),alignof(HierarchicalName));
  RegisterOpaqueType(STRING("TypeAndInstance"),Type::STRUCT,sizeof(TypeAndInstance),alignof(TypeAndInstance));
  RegisterOpaqueType(STRING("SignalHandler"),Type::TYPEDEF,sizeof(SignalHandler),alignof(SignalHandler));
  RegisterOpaqueType(STRING("Byte"),Type::TYPEDEF,sizeof(Byte),alignof(Byte));
  RegisterOpaqueType(STRING("u8"),Type::TYPEDEF,sizeof(u8),alignof(u8));
  RegisterOpaqueType(STRING("i8"),Type::TYPEDEF,sizeof(i8),alignof(i8));
  RegisterOpaqueType(STRING("u16"),Type::TYPEDEF,sizeof(u16),alignof(u16));
  RegisterOpaqueType(STRING("i16"),Type::TYPEDEF,sizeof(i16),alignof(i16));
  RegisterOpaqueType(STRING("u32"),Type::TYPEDEF,sizeof(u32),alignof(u32));
  RegisterOpaqueType(STRING("i32"),Type::TYPEDEF,sizeof(i32),alignof(i32));
  RegisterOpaqueType(STRING("u64"),Type::TYPEDEF,sizeof(u64),alignof(u64));
  RegisterOpaqueType(STRING("i64"),Type::TYPEDEF,sizeof(i64),alignof(i64));
  RegisterOpaqueType(STRING("iptr"),Type::TYPEDEF,sizeof(iptr),alignof(iptr));
  RegisterOpaqueType(STRING("uptr"),Type::TYPEDEF,sizeof(uptr),alignof(uptr));
  RegisterOpaqueType(STRING("uint"),Type::TYPEDEF,sizeof(uint),alignof(uint));
  RegisterOpaqueType(STRING("String"),Type::TYPEDEF,sizeof(String),alignof(String));
  RegisterOpaqueType(STRING("CharFunction"),Type::TYPEDEF,sizeof(CharFunction),alignof(CharFunction));
  RegisterOpaqueType(STRING("PipeFunction"),Type::TYPEDEF,sizeof(PipeFunction),alignof(PipeFunction));
  RegisterOpaqueType(STRING("ValueMap"),Type::TYPEDEF,sizeof(ValueMap),alignof(ValueMap));
  RegisterOpaqueType(STRING("MacroMap"),Type::TYPEDEF,sizeof(MacroMap),alignof(MacroMap));
  RegisterOpaqueType(STRING("ExpressionRange"),Type::TYPEDEF,sizeof(ExpressionRange),alignof(ExpressionRange));
  RegisterOpaqueType(STRING("Wire"),Type::TYPEDEF,sizeof(Wire),alignof(Wire));
  RegisterOpaqueType(STRING("WireExpression"),Type::TYPEDEF,sizeof(WireExpression),alignof(WireExpression));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPorts"),Type::TYPEDEF,sizeof(ExternalMemoryTwoPorts),alignof(ExternalMemoryTwoPorts));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPortsExpression"),Type::TYPEDEF,sizeof(ExternalMemoryTwoPortsExpression),alignof(ExternalMemoryTwoPortsExpression));
  RegisterOpaqueType(STRING("ExternalMemoryDualPort"),Type::TYPEDEF,sizeof(ExternalMemoryDualPort),alignof(ExternalMemoryDualPort));
  RegisterOpaqueType(STRING("ExternalMemoryDualPortExpression"),Type::TYPEDEF,sizeof(ExternalMemoryDualPortExpression),alignof(ExternalMemoryDualPortExpression));
  RegisterOpaqueType(STRING("ExternalMemory"),Type::TYPEDEF,sizeof(ExternalMemory),alignof(ExternalMemory));
  RegisterOpaqueType(STRING("ExternalMemoryExpression"),Type::TYPEDEF,sizeof(ExternalMemoryExpression),alignof(ExternalMemoryExpression));
  RegisterOpaqueType(STRING("ExternalMemoryInterface"),Type::TYPEDEF,sizeof(ExternalMemoryInterface),alignof(ExternalMemoryInterface));
  RegisterOpaqueType(STRING("ExternalMemoryInterfaceExpression"),Type::TYPEDEF,sizeof(ExternalMemoryInterfaceExpression),alignof(ExternalMemoryInterfaceExpression));
  RegisterOpaqueType(STRING("InstanceMap"),Type::TYPEDEF,sizeof(InstanceMap),alignof(InstanceMap));
  RegisterOpaqueType(STRING("PortEdgeMap"),Type::TYPEDEF,sizeof(PortEdgeMap),alignof(PortEdgeMap));
  RegisterOpaqueType(STRING("EdgeMap"),Type::TYPEDEF,sizeof(EdgeMap),alignof(EdgeMap));
  RegisterOpaqueType(STRING("InstanceNodeMap"),Type::TYPEDEF,sizeof(InstanceNodeMap),alignof(InstanceNodeMap));
  RegisterOpaqueType(STRING("GraphDotFormat"),Type::TYPEDEF,sizeof(GraphDotFormat),alignof(GraphDotFormat));
  RegisterOpaqueType(STRING("TaskFunction"),Type::TYPEDEF,sizeof(TaskFunction),alignof(TaskFunction));
  RegisterOpaqueType(STRING("InstanceTable"),Type::TYPEDEF,sizeof(InstanceTable),alignof(InstanceTable));
  RegisterOpaqueType(STRING("InstanceName"),Type::TYPEDEF,sizeof(InstanceName),alignof(InstanceName));
  RegisterOpaqueType(STRING("SpecNode"),Type::TYPEDEF,sizeof(SpecNode),alignof(SpecNode));
  RegisterOpaqueType(STRING("String"),Type::TYPEDEF,sizeof(String),alignof(String));

static Pair<String,int> LogModuleData[] = {{STRING("MEMORY"),(int) LogModule::MEMORY},
    {STRING("TOP_SYS"),(int) LogModule::TOP_SYS},
    {STRING("ACCELERATOR"),(int) LogModule::ACCELERATOR},
    {STRING("PARSER"),(int) LogModule::PARSER},
    {STRING("DEBUG_SYS"),(int) LogModule::DEBUG_SYS},
    {STRING("TYPE"),(int) LogModule::TYPE},
    {STRING("TEMPLATE"),(int) LogModule::TEMPLATE},
    {STRING("UTILS"),(int) LogModule::UTILS}};

RegisterEnum(STRING("LogModule"),C_ARRAY_TO_ARRAY(LogModuleData));

static Pair<String,int> LogLevelData[] = {{STRING("DEBUG"),(int) LogLevel::DEBUG},
    {STRING("INFO"),(int) LogLevel::INFO},
    {STRING("WARN"),(int) LogLevel::WARN},
    {STRING("ERROR"),(int) LogLevel::ERROR},
    {STRING("FATAL"),(int) LogLevel::FATAL}};

RegisterEnum(STRING("LogLevel"),C_ARRAY_TO_ARRAY(LogLevelData));

static Pair<String,int> CommandTypeData[] = {{STRING("CommandType_JOIN"),(int) CommandType::CommandType_JOIN},
    {STRING("CommandType_FOR"),(int) CommandType::CommandType_FOR},
    {STRING("CommandType_IF"),(int) CommandType::CommandType_IF},
    {STRING("CommandType_END"),(int) CommandType::CommandType_END},
    {STRING("CommandType_SET"),(int) CommandType::CommandType_SET},
    {STRING("CommandType_LET"),(int) CommandType::CommandType_LET},
    {STRING("CommandType_INC"),(int) CommandType::CommandType_INC},
    {STRING("CommandType_ELSE"),(int) CommandType::CommandType_ELSE},
    {STRING("CommandType_DEBUG"),(int) CommandType::CommandType_DEBUG},
    {STRING("CommandType_INCLUDE"),(int) CommandType::CommandType_INCLUDE},
    {STRING("CommandType_DEFINE"),(int) CommandType::CommandType_DEFINE},
    {STRING("CommandType_CALL"),(int) CommandType::CommandType_CALL},
    {STRING("CommandType_WHILE"),(int) CommandType::CommandType_WHILE},
    {STRING("CommandType_RETURN"),(int) CommandType::CommandType_RETURN},
    {STRING("CommandType_FORMAT"),(int) CommandType::CommandType_FORMAT},
    {STRING("CommandType_DEBUG_MESSAGE"),(int) CommandType::CommandType_DEBUG_MESSAGE},
    {STRING("CommandType_DEBUG_BREAK"),(int) CommandType::CommandType_DEBUG_BREAK}};

RegisterEnum(STRING("CommandType"),C_ARRAY_TO_ARRAY(CommandTypeData));

static Pair<String,int> BlockTypeData[] = {{STRING("BlockType_TEXT"),(int) BlockType::BlockType_TEXT},
    {STRING("BlockType_COMMAND"),(int) BlockType::BlockType_COMMAND},
    {STRING("BlockType_EXPRESSION"),(int) BlockType::BlockType_EXPRESSION}};

RegisterEnum(STRING("BlockType"),C_ARRAY_TO_ARRAY(BlockTypeData));

static Pair<String,int> TemplateRecordTypeData[] = {{STRING("TemplateRecordType_FIELD"),(int) TemplateRecordType::TemplateRecordType_FIELD},
    {STRING("TemplateRecordType_IDENTIFER"),(int) TemplateRecordType::TemplateRecordType_IDENTIFER}};

RegisterEnum(STRING("TemplateRecordType"),C_ARRAY_TO_ARRAY(TemplateRecordTypeData));

static Pair<String,int> ExternalMemoryTypeData[] = {{STRING("TWO_P"),(int) ExternalMemoryType::TWO_P},
    {STRING("DP"),(int) ExternalMemoryType::DP}};

RegisterEnum(STRING("ExternalMemoryType"),C_ARRAY_TO_ARRAY(ExternalMemoryTypeData));

static Pair<String,int> MemTypeData[] = {{STRING("CONFIG"),(int) MemType::CONFIG},
    {STRING("STATE"),(int) MemType::STATE},
    {STRING("DELAY"),(int) MemType::DELAY},
    {STRING("STATIC"),(int) MemType::STATIC}};

RegisterEnum(STRING("MemType"),C_ARRAY_TO_ARRAY(MemTypeData));

static Pair<String,int> NodeTypeData[] = {{STRING("NodeType_UNCONNECTED"),(int) NodeType::NodeType_UNCONNECTED},
    {STRING("NodeType_SOURCE"),(int) NodeType::NodeType_SOURCE},
    {STRING("NodeType_COMPUTE"),(int) NodeType::NodeType_COMPUTE},
    {STRING("NodeType_SINK"),(int) NodeType::NodeType_SINK},
    {STRING("NodeType_SOURCE_AND_SINK"),(int) NodeType::NodeType_SOURCE_AND_SINK}};

RegisterEnum(STRING("NodeType"),C_ARRAY_TO_ARRAY(NodeTypeData));

static Pair<String,int> DelayTypeData[] = {{STRING("DelayType_BASE"),(int) DelayType::DelayType_BASE},
    {STRING("DelayType_SINK_DELAY"),(int) DelayType::DelayType_SINK_DELAY},
    {STRING("DelayType_SOURCE_DELAY"),(int) DelayType::DelayType_SOURCE_DELAY},
    {STRING("DelayType_COMPUTE_DELAY"),(int) DelayType::DelayType_COMPUTE_DELAY}};

RegisterEnum(STRING("DelayType"),C_ARRAY_TO_ARRAY(DelayTypeData));

static Pair<String,int> FUDeclarationTypeData[] = {{STRING("FUDeclarationType_SINGLE"),(int) FUDeclarationType::FUDeclarationType_SINGLE},
    {STRING("FUDeclarationType_COMPOSITE"),(int) FUDeclarationType::FUDeclarationType_COMPOSITE},
    {STRING("FUDeclarationType_SPECIAL"),(int) FUDeclarationType::FUDeclarationType_SPECIAL},
    {STRING("FUDeclarationType_MERGED"),(int) FUDeclarationType::FUDeclarationType_MERGED},
    {STRING("FUDeclarationType_ITERATIVE"),(int) FUDeclarationType::FUDeclarationType_ITERATIVE}};

RegisterEnum(STRING("FUDeclarationType"),C_ARRAY_TO_ARRAY(FUDeclarationTypeData));

static Pair<String,int> GraphPrintingColorData[] = {{STRING("GraphPrintingColor_BLACK"),(int) GraphPrintingColor::GraphPrintingColor_BLACK},
    {STRING("GraphPrintingColor_BLUE"),(int) GraphPrintingColor::GraphPrintingColor_BLUE},
    {STRING("GraphPrintingColor_RED"),(int) GraphPrintingColor::GraphPrintingColor_RED},
    {STRING("GraphPrintingColor_GREEN"),(int) GraphPrintingColor::GraphPrintingColor_GREEN}};

RegisterEnum(STRING("GraphPrintingColor"),C_ARRAY_TO_ARRAY(GraphPrintingColorData));

static Pair<String,int> VersatDebugFlagsData[] = {{STRING("OUTPUT_GRAPH_DOT"),(int) VersatDebugFlags::OUTPUT_GRAPH_DOT},
    {STRING("GRAPH_DOT_FORMAT"),(int) VersatDebugFlags::GRAPH_DOT_FORMAT},
    {STRING("OUTPUT_ACCELERATORS_CODE"),(int) VersatDebugFlags::OUTPUT_ACCELERATORS_CODE},
    {STRING("OUTPUT_VERSAT_CODE"),(int) VersatDebugFlags::OUTPUT_VERSAT_CODE},
    {STRING("USE_FIXED_BUFFERS"),(int) VersatDebugFlags::USE_FIXED_BUFFERS}};

RegisterEnum(STRING("VersatDebugFlags"),C_ARRAY_TO_ARRAY(VersatDebugFlagsData));

static Pair<String,int> GraphDotFormat_Data[] = {{STRING("GRAPH_DOT_FORMAT_NAME"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_NAME},
    {STRING("GRAPH_DOT_FORMAT_TYPE"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_TYPE},
    {STRING("GRAPH_DOT_FORMAT_ID"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_ID},
    {STRING("GRAPH_DOT_FORMAT_DELAY"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_DELAY},
    {STRING("GRAPH_DOT_FORMAT_EXPLICIT"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_EXPLICIT},
    {STRING("GRAPH_DOT_FORMAT_PORT"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_PORT},
    {STRING("GRAPH_DOT_FORMAT_LATENCY"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_LATENCY}};

RegisterEnum(STRING("GraphDotFormat_"),C_ARRAY_TO_ARRAY(GraphDotFormat_Data));

static Pair<String,int> MergingStrategyData[] = {{STRING("SIMPLE_COMBINATION"),(int) MergingStrategy::SIMPLE_COMBINATION},
    {STRING("CONSOLIDATION_GRAPH"),(int) MergingStrategy::CONSOLIDATION_GRAPH},
    {STRING("FIRST_FIT"),(int) MergingStrategy::FIRST_FIT}};

RegisterEnum(STRING("MergingStrategy"),C_ARRAY_TO_ARRAY(MergingStrategyData));

static Pair<String,int> ConnectionTypeData[] = {{STRING("ConnectionType_SINGLE"),(int) ConnectionType::ConnectionType_SINGLE},
    {STRING("ConnectionType_PORT_RANGE"),(int) ConnectionType::ConnectionType_PORT_RANGE},
    {STRING("ConnectionType_ARRAY_RANGE"),(int) ConnectionType::ConnectionType_ARRAY_RANGE},
    {STRING("ConnectionType_DELAY_RANGE"),(int) ConnectionType::ConnectionType_DELAY_RANGE},
    {STRING("ConnectionType_ERROR"),(int) ConnectionType::ConnectionType_ERROR}};

RegisterEnum(STRING("ConnectionType"),C_ARRAY_TO_ARRAY(ConnectionTypeData));

  static String templateArgs[] = { STRING("T") /* 0 */,
    STRING("F") /* 1 */,
    STRING("T") /* 2 */,
    STRING("T") /* 3 */,
    STRING("T") /* 4 */,
    STRING("First") /* 5 */,
    STRING("Second") /* 6 */,
    STRING("T") /* 7 */,
    STRING("T") /* 8 */,
    STRING("Key") /* 9 */,
    STRING("Data") /* 10 */,
    STRING("Data") /* 11 */,
    STRING("Key") /* 12 */,
    STRING("Data") /* 13 */,
    STRING("Data") /* 14 */,
    STRING("Data") /* 15 */,
    STRING("Key") /* 16 */,
    STRING("Data") /* 17 */,
    STRING("Key") /* 18 */,
    STRING("Data") /* 19 */,
    STRING("Key") /* 20 */,
    STRING("Data") /* 21 */,
    STRING("Data") /* 22 */,
    STRING("Data") /* 23 */,
    STRING("T") /* 24 */,
    STRING("T") /* 25 */,
    STRING("Value") /* 26 */,
    STRING("Error") /* 27 */,
    STRING("T") /* 28 */,
    STRING("T") /* 29 */,
    STRING("T") /* 30 */,
    STRING("T") /* 31 */,
    STRING("T") /* 32 */,
    STRING("T") /* 33 */,
    STRING("T") /* 34 */,
    STRING("T") /* 35 */,
    STRING("T") /* 36 */,
    STRING("T") /* 37 */
  };

  RegisterTemplate(STRING("Test2"),(Array<String>){&templateArgs[0],1});
  RegisterTemplate(STRING("_Defer"),(Array<String>){&templateArgs[1],1});
  RegisterTemplate(STRING("ArrayIterator"),(Array<String>){&templateArgs[2],1});
  RegisterTemplate(STRING("Array"),(Array<String>){&templateArgs[3],1});
  RegisterTemplate(STRING("Range"),(Array<String>){&templateArgs[4],1});
  RegisterTemplate(STRING("Pair"),(Array<String>){&templateArgs[5],2});
  RegisterTemplate(STRING("DynamicArray"),(Array<String>){&templateArgs[7],1});
  RegisterTemplate(STRING("PushPtr"),(Array<String>){&templateArgs[8],1});
  RegisterTemplate(STRING("HashmapIterator"),(Array<String>){&templateArgs[9],2});
  RegisterTemplate(STRING("GetOrAllocateResult"),(Array<String>){&templateArgs[11],1});
  RegisterTemplate(STRING("Hashmap"),(Array<String>){&templateArgs[12],2});
  RegisterTemplate(STRING("Set"),(Array<String>){&templateArgs[14],1});
  RegisterTemplate(STRING("SetIterator"),(Array<String>){&templateArgs[15],1});
  RegisterTemplate(STRING("TrieMapNode"),(Array<String>){&templateArgs[16],2});
  RegisterTemplate(STRING("TrieMapIterator"),(Array<String>){&templateArgs[18],2});
  RegisterTemplate(STRING("TrieMap"),(Array<String>){&templateArgs[20],2});
  RegisterTemplate(STRING("TrieSetIterator"),(Array<String>){&templateArgs[22],1});
  RegisterTemplate(STRING("TrieSet"),(Array<String>){&templateArgs[23],1});
  RegisterTemplate(STRING("PoolIterator"),(Array<String>){&templateArgs[24],1});
  RegisterTemplate(STRING("Pool"),(Array<String>){&templateArgs[25],1});
  RegisterTemplate(STRING("Result"),(Array<String>){&templateArgs[26],2});
  RegisterTemplate(STRING("IndexedStruct"),(Array<String>){&templateArgs[28],1});
  RegisterTemplate(STRING("ListedStruct"),(Array<String>){&templateArgs[29],1});
  RegisterTemplate(STRING("ArenaList"),(Array<String>){&templateArgs[30],1});
  RegisterTemplate(STRING("Stack"),(Array<String>){&templateArgs[31],1});
  RegisterTemplate(STRING("WireTemplate"),(Array<String>){&templateArgs[32],1});
  RegisterTemplate(STRING("ExternalMemoryTwoPortsTemplate"),(Array<String>){&templateArgs[33],1});
  RegisterTemplate(STRING("ExternalMemoryDualPortTemplate"),(Array<String>){&templateArgs[34],1});
  RegisterTemplate(STRING("ExternalMemoryTemplate"),(Array<String>){&templateArgs[35],1});
  RegisterTemplate(STRING("ExternalMemoryInterfaceTemplate"),(Array<String>){&templateArgs[36],1});
  RegisterTemplate(STRING("std::vector"),(Array<String>){&templateArgs[37],1});

  RegisterTypedef(STRING("uint8_t"),STRING("Byte"));
  RegisterTypedef(STRING("uint8_t"),STRING("u8"));
  RegisterTypedef(STRING("int8_t"),STRING("i8"));
  RegisterTypedef(STRING("uint16_t"),STRING("u16"));
  RegisterTypedef(STRING("int16_t"),STRING("i16"));
  RegisterTypedef(STRING("uint32_t"),STRING("u32"));
  RegisterTypedef(STRING("int32_t"),STRING("i32"));
  RegisterTypedef(STRING("uint64_t"),STRING("u64"));
  RegisterTypedef(STRING("int64_t"),STRING("i64"));
  RegisterTypedef(STRING("intptr_t"),STRING("iptr"));
  RegisterTypedef(STRING("uintptr_t"),STRING("uptr"));
  RegisterTypedef(STRING("unsigned int"),STRING("uint"));
  RegisterTypedef(STRING("int"),STRING("ValueMap"));
  RegisterTypedef(STRING("int"),STRING("MacroMap"));
  RegisterTypedef(STRING("uint"),STRING("GraphDotFormat"));

  static TemplatedMember templateMembers[] = { (TemplatedMember){STRING("T"),STRING("a"),0} /* 0 */,
    (TemplatedMember){STRING("T"),STRING("b"),1} /* 1 */,
    (TemplatedMember){STRING("F"),STRING("f"),0} /* 2 */,
    (TemplatedMember){STRING("T *"),STRING("ptr"),0} /* 3 */,
    (TemplatedMember){STRING("T *"),STRING("data"),0} /* 4 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 5 */,
    (TemplatedMember){STRING("T"),STRING("high"),0} /* 6 */,
    (TemplatedMember){STRING("T"),STRING("start"),0} /* 7 */,
    (TemplatedMember){STRING("T"),STRING("top"),0} /* 8 */,
    (TemplatedMember){STRING("T"),STRING("low"),1} /* 9 */,
    (TemplatedMember){STRING("T"),STRING("end"),1} /* 10 */,
    (TemplatedMember){STRING("T"),STRING("bottom"),1} /* 11 */,
    (TemplatedMember){STRING("First"),STRING("key"),0} /* 12 */,
    (TemplatedMember){STRING("First"),STRING("first"),0} /* 13 */,
    (TemplatedMember){STRING("Second"),STRING("data"),1} /* 14 */,
    (TemplatedMember){STRING("Second"),STRING("second"),1} /* 15 */,
    (TemplatedMember){STRING("ArenaMark"),STRING("mark"),0} /* 16 */,
    (TemplatedMember){STRING("T *"),STRING("ptr"),0} /* 17 */,
    (TemplatedMember){STRING("int"),STRING("maximumTimes"),1} /* 18 */,
    (TemplatedMember){STRING("int"),STRING("timesPushed"),2} /* 19 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("pairs"),0} /* 20 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 21 */,
    (TemplatedMember){STRING("Data *"),STRING("data"),0} /* 22 */,
    (TemplatedMember){STRING("bool"),STRING("alreadyExisted"),1} /* 23 */,
    (TemplatedMember){STRING("int"),STRING("nodesAllocated"),0} /* 24 */,
    (TemplatedMember){STRING("int"),STRING("nodesUsed"),1} /* 25 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("buckets"),2} /* 26 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("data"),3} /* 27 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("next"),4} /* 28 */,
    (TemplatedMember){STRING("Hashmap<Data, int> *"),STRING("map"),0} /* 29 */,
    (TemplatedMember){STRING("HashmapIterator<Data, int>"),STRING("innerIter"),0} /* 30 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 31 */,
    (TemplatedMember){STRING("Pair<Key, Data>"),STRING("pair"),1} /* 32 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("next"),2} /* 33 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("ptr"),0} /* 34 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 35 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),1} /* 36 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("head"),2} /* 37 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("tail"),3} /* 38 */,
    (TemplatedMember){STRING("int"),STRING("inserted"),4} /* 39 */,
    (TemplatedMember){STRING("TrieMapIterator<Data, int>"),STRING("innerIter"),0} /* 40 */,
    (TemplatedMember){STRING("TrieMap<Data, int> *"),STRING("map"),0} /* 41 */,
    (TemplatedMember){STRING("Pool<T> *"),STRING("pool"),0} /* 42 */,
    (TemplatedMember){STRING("PageInfo"),STRING("pageInfo"),1} /* 43 */,
    (TemplatedMember){STRING("int"),STRING("fullIndex"),2} /* 44 */,
    (TemplatedMember){STRING("int"),STRING("bit"),3} /* 45 */,
    (TemplatedMember){STRING("int"),STRING("index"),4} /* 46 */,
    (TemplatedMember){STRING("Byte *"),STRING("page"),5} /* 47 */,
    (TemplatedMember){STRING("T *"),STRING("lastVal"),6} /* 48 */,
    (TemplatedMember){STRING("Byte *"),STRING("mem"),0} /* 49 */,
    (TemplatedMember){STRING("PoolInfo"),STRING("info"),1} /* 50 */,
    (TemplatedMember){STRING("Value"),STRING("value"),0} /* 51 */,
    (TemplatedMember){STRING("Error"),STRING("error"),0} /* 52 */,
    (TemplatedMember){STRING("bool"),STRING("isError"),1} /* 53 */,
    (TemplatedMember){STRING("int"),STRING("index"),0} /* 54 */,
    (TemplatedMember){STRING("T"),STRING("elem"),0} /* 55 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("next"),1} /* 56 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),0} /* 57 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("head"),1} /* 58 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("tail"),2} /* 59 */,
    (TemplatedMember){STRING("Array<T>"),STRING("mem"),0} /* 60 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 61 */,
    (TemplatedMember){STRING("String"),STRING("name"),0} /* 62 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),1} /* 63 */,
    (TemplatedMember){STRING("bool"),STRING("isStatic"),2} /* 64 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeIn"),0} /* 65 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeOut"),1} /* 66 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),2} /* 67 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),3} /* 68 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),0} /* 69 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),1} /* 70 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),2} /* 71 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 72 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 73 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 74 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 75 */,
    (TemplatedMember){STRING("ExternalMemoryType"),STRING("type"),1} /* 76 */,
    (TemplatedMember){STRING("int"),STRING("interface"),2} /* 77 */,
    (TemplatedMember){STRING("T*"),STRING("mem"),0} /* 78 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 79 */,
    (TemplatedMember){STRING("int"),STRING("allocated"),2} /* 80 */
  };

  RegisterTemplateMembers(STRING("Test2"),(Array<TemplatedMember>){&templateMembers[0],2});
  RegisterTemplateMembers(STRING("_Defer"),(Array<TemplatedMember>){&templateMembers[2],1});
  RegisterTemplateMembers(STRING("ArrayIterator"),(Array<TemplatedMember>){&templateMembers[3],1});
  RegisterTemplateMembers(STRING("Array"),(Array<TemplatedMember>){&templateMembers[4],2});
  RegisterTemplateMembers(STRING("Range"),(Array<TemplatedMember>){&templateMembers[6],6});
  RegisterTemplateMembers(STRING("Pair"),(Array<TemplatedMember>){&templateMembers[12],4});
  RegisterTemplateMembers(STRING("DynamicArray"),(Array<TemplatedMember>){&templateMembers[16],1});
  RegisterTemplateMembers(STRING("PushPtr"),(Array<TemplatedMember>){&templateMembers[17],3});
  RegisterTemplateMembers(STRING("HashmapIterator"),(Array<TemplatedMember>){&templateMembers[20],2});
  RegisterTemplateMembers(STRING("GetOrAllocateResult"),(Array<TemplatedMember>){&templateMembers[22],2});
  RegisterTemplateMembers(STRING("Hashmap"),(Array<TemplatedMember>){&templateMembers[24],5});
  RegisterTemplateMembers(STRING("Set"),(Array<TemplatedMember>){&templateMembers[29],1});
  RegisterTemplateMembers(STRING("SetIterator"),(Array<TemplatedMember>){&templateMembers[30],1});
  RegisterTemplateMembers(STRING("TrieMapNode"),(Array<TemplatedMember>){&templateMembers[31],3});
  RegisterTemplateMembers(STRING("TrieMapIterator"),(Array<TemplatedMember>){&templateMembers[34],1});
  RegisterTemplateMembers(STRING("TrieMap"),(Array<TemplatedMember>){&templateMembers[35],5});
  RegisterTemplateMembers(STRING("TrieSetIterator"),(Array<TemplatedMember>){&templateMembers[40],1});
  RegisterTemplateMembers(STRING("TrieSet"),(Array<TemplatedMember>){&templateMembers[41],1});
  RegisterTemplateMembers(STRING("PoolIterator"),(Array<TemplatedMember>){&templateMembers[42],7});
  RegisterTemplateMembers(STRING("Pool"),(Array<TemplatedMember>){&templateMembers[49],2});
  RegisterTemplateMembers(STRING("Result"),(Array<TemplatedMember>){&templateMembers[51],3});
  RegisterTemplateMembers(STRING("IndexedStruct"),(Array<TemplatedMember>){&templateMembers[54],1});
  RegisterTemplateMembers(STRING("ListedStruct"),(Array<TemplatedMember>){&templateMembers[55],2});
  RegisterTemplateMembers(STRING("ArenaList"),(Array<TemplatedMember>){&templateMembers[57],3});
  RegisterTemplateMembers(STRING("Stack"),(Array<TemplatedMember>){&templateMembers[60],2});
  RegisterTemplateMembers(STRING("WireTemplate"),(Array<TemplatedMember>){&templateMembers[62],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTwoPortsTemplate"),(Array<TemplatedMember>){&templateMembers[65],4});
  RegisterTemplateMembers(STRING("ExternalMemoryDualPortTemplate"),(Array<TemplatedMember>){&templateMembers[69],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTemplate"),(Array<TemplatedMember>){&templateMembers[72],2});
  RegisterTemplateMembers(STRING("ExternalMemoryInterfaceTemplate"),(Array<TemplatedMember>){&templateMembers[74],4});
  RegisterTemplateMembers(STRING("std::vector"),(Array<TemplatedMember>){&templateMembers[78],3});

  static Member members[] = {(Member){GetType(STRING("int")),STRING("a"),offsetof(Test,a)} /* 0 */,
    (Member){GetType(STRING("int")),STRING("b"),offsetof(Test,b)} /* 1 */,
    (Member){GetType(STRING("Test")),STRING("t"),offsetof(Test3,t)} /* 2 */,
    (Member){GetType(STRING("u64")),STRING("microSeconds"),offsetof(Time,microSeconds)} /* 3 */,
    (Member){GetType(STRING("u64")),STRING("seconds"),offsetof(Time,seconds)} /* 4 */,
    (Member){GetType(STRING("float")),STRING("f"),offsetof(Conversion,f)} /* 5 */,
    (Member){GetType(STRING("int")),STRING("i"),offsetof(Conversion,i)} /* 6 */,
    (Member){GetType(STRING("uint")),STRING("ui"),offsetof(Conversion,ui)} /* 7 */,
    (Member){GetType(STRING("Byte *")),STRING("mem"),offsetof(Arena,mem)} /* 8 */,
    (Member){GetType(STRING("int")),STRING("used"),offsetof(Arena,used)} /* 9 */,
    (Member){GetType(STRING("int")),STRING("totalAllocated"),offsetof(Arena,totalAllocated)} /* 10 */,
    (Member){GetType(STRING("int")),STRING("maximum"),offsetof(Arena,maximum)} /* 11 */,
    (Member){GetType(STRING("bool")),STRING("locked"),offsetof(Arena,locked)} /* 12 */,
    (Member){GetType(STRING("Arena *")),STRING("arena"),offsetof(ArenaMark,arena)} /* 13 */,
    (Member){GetType(STRING("Byte *")),STRING("mark"),offsetof(ArenaMark,mark)} /* 14 */,
    (Member){GetType(STRING("DynamicArena *")),STRING("next"),offsetof(DynamicArena,next)} /* 15 */,
    (Member){GetType(STRING("Byte *")),STRING("mem"),offsetof(DynamicArena,mem)} /* 16 */,
    (Member){GetType(STRING("int")),STRING("used"),offsetof(DynamicArena,used)} /* 17 */,
    (Member){GetType(STRING("int")),STRING("pagesAllocated"),offsetof(DynamicArena,pagesAllocated)} /* 18 */,
    (Member){GetType(STRING("Arena *")),STRING("arena"),offsetof(DynamicString,arena)} /* 19 */,
    (Member){GetType(STRING("Byte *")),STRING("mark"),offsetof(DynamicString,mark)} /* 20 */,
    (Member){GetType(STRING("Byte *")),STRING("memory"),offsetof(BitArray,memory)} /* 21 */,
    (Member){GetType(STRING("int")),STRING("bitSize"),offsetof(BitArray,bitSize)} /* 22 */,
    (Member){GetType(STRING("int")),STRING("byteSize"),offsetof(BitArray,byteSize)} /* 23 */,
    (Member){GetType(STRING("Byte *")),STRING("nextPage"),offsetof(PoolHeader,nextPage)} /* 24 */,
    (Member){GetType(STRING("int")),STRING("allocatedUnits"),offsetof(PoolHeader,allocatedUnits)} /* 25 */,
    (Member){GetType(STRING("int")),STRING("unitsPerFullPage"),offsetof(PoolInfo,unitsPerFullPage)} /* 26 */,
    (Member){GetType(STRING("int")),STRING("bitmapSize"),offsetof(PoolInfo,bitmapSize)} /* 27 */,
    (Member){GetType(STRING("int")),STRING("unitsPerPage"),offsetof(PoolInfo,unitsPerPage)} /* 28 */,
    (Member){GetType(STRING("int")),STRING("pageGranuality"),offsetof(PoolInfo,pageGranuality)} /* 29 */,
    (Member){GetType(STRING("PoolHeader *")),STRING("header"),offsetof(PageInfo,header)} /* 30 */,
    (Member){GetType(STRING("Byte *")),STRING("bitmap"),offsetof(PageInfo,bitmap)} /* 31 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(EnumMember,name)} /* 32 */,
    (Member){GetType(STRING("String")),STRING("value"),offsetof(EnumMember,value)} /* 33 */,
    (Member){GetType(STRING("EnumMember *")),STRING("next"),offsetof(EnumMember,next)} /* 34 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TemplateArg,name)} /* 35 */,
    (Member){GetType(STRING("TemplateArg *")),STRING("next"),offsetof(TemplateArg,next)} /* 36 */,
    (Member){GetType(STRING("String")),STRING("typeName"),offsetof(TemplatedMember,typeName)} /* 37 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TemplatedMember,name)} /* 38 */,
    (Member){GetType(STRING("int")),STRING("memberOffset"),offsetof(TemplatedMember,memberOffset)} /* 39 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Type,name)} /* 40 */,
    (Member){GetType(STRING("Type *")),STRING("baseType"),offsetof(Type,baseType)} /* 41 */,
    (Member){GetType(STRING("Type *")),STRING("pointerType"),offsetof(Type,pointerType)} /* 42 */,
    (Member){GetType(STRING("Type *")),STRING("arrayType"),offsetof(Type,arrayType)} /* 43 */,
    (Member){GetType(STRING("Type *")),STRING("typedefType"),offsetof(Type,typedefType)} /* 44 */,
    (Member){GetType(STRING("Array<Pair<String, int>>")),STRING("enumMembers"),offsetof(Type,enumMembers)} /* 45 */,
    (Member){GetType(STRING("Array<TemplatedMember>")),STRING("templateMembers"),offsetof(Type,templateMembers)} /* 46 */,
    (Member){GetType(STRING("Array<String>")),STRING("templateArgs"),offsetof(Type,templateArgs)} /* 47 */,
    (Member){GetType(STRING("Array<Member>")),STRING("members"),offsetof(Type,members)} /* 48 */,
    (Member){GetType(STRING("Type *")),STRING("templateBase"),offsetof(Type,templateBase)} /* 49 */,
    (Member){GetType(STRING("Array<Type *>")),STRING("templateArgTypes"),offsetof(Type,templateArgTypes)} /* 50 */,
    (Member){GetType(STRING("int")),STRING("size"),offsetof(Type,size)} /* 51 */,
    (Member){GetType(STRING("int")),STRING("align"),offsetof(Type,align)} /* 52 */,
    (Member){GetType(STRING("enum Subtype")),STRING("type"),offsetof(Type,type)} /* 53 */,
    (Member){GetType(STRING("Type *")),STRING("type"),offsetof(Member,type)} /* 54 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Member,name)} /* 55 */,
    (Member){GetType(STRING("int")),STRING("offset"),offsetof(Member,offset)} /* 56 */,
    (Member){GetType(STRING("Type *")),STRING("structType"),offsetof(Member,structType)} /* 57 */,
    (Member){GetType(STRING("String")),STRING("arrayExpression"),offsetof(Member,arrayExpression)} /* 58 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(Member,index)} /* 59 */,
    (Member){GetType(STRING("PoolIterator<Type>")),STRING("iter"),offsetof(TypeIterator,iter)} /* 60 */,
    (Member){GetType(STRING("PoolIterator<Type>")),STRING("end"),offsetof(TypeIterator,end)} /* 61 */,
    (Member){GetType(STRING("bool")),STRING("boolean"),offsetof(Value,boolean)} /* 62 */,
    (Member){GetType(STRING("char")),STRING("ch"),offsetof(Value,ch)} /* 63 */,
    (Member){GetType(STRING("i64")),STRING("number"),offsetof(Value,number)} /* 64 */,
    (Member){GetType(STRING("String")),STRING("str"),offsetof(Value,str)} /* 65 */,
    (Member){GetType(STRING("bool")),STRING("literal"),offsetof(Value,literal)} /* 66 */,
    (Member){GetType(STRING("TemplateFunction *")),STRING("templateFunction"),offsetof(Value,templateFunction)} /* 67 */,
    (Member){GetType(STRING("void *")),STRING("custom"),offsetof(Value,custom)} /* 68 */,
    (Member){GetType(STRING("Type *")),STRING("type"),offsetof(Value,type)} /* 69 */,
    (Member){GetType(STRING("bool")),STRING("isTemp"),offsetof(Value,isTemp)} /* 70 */,
    (Member){GetType(STRING("int")),STRING("currentNumber"),offsetof(Iterator,currentNumber)} /* 71 */,
    (Member){GetType(STRING("GenericPoolIterator")),STRING("poolIterator"),offsetof(Iterator,poolIterator)} /* 72 */,
    (Member){GetType(STRING("Type *")),STRING("hashmapType"),offsetof(Iterator,hashmapType)} /* 73 */,
    (Member){GetType(STRING("Value")),STRING("iterating"),offsetof(Iterator,iterating)} /* 74 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(HashmapUnpackedIndex,index)} /* 75 */,
    (Member){GetType(STRING("bool")),STRING("data"),offsetof(HashmapUnpackedIndex,data)} /* 76 */,
    (Member){GetType(STRING("char *")),STRING("op"),offsetof(Expression,op)} /* 77 */,
    (Member){GetType(STRING("String")),STRING("id"),offsetof(Expression,id)} /* 78 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Expression,expressions)} /* 79 */,
    (Member){GetType(STRING("Command *")),STRING("command"),offsetof(Expression,command)} /* 80 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(Expression,val)} /* 81 */,
    (Member){GetType(STRING("String")),STRING("text"),offsetof(Expression,text)} /* 82 */,
    (Member){GetType(STRING("int")),STRING("approximateLine"),offsetof(Expression,approximateLine)} /* 83 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/common/parser.hpp:22:3)")),STRING("type"),offsetof(Expression,type)} /* 84 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Cursor,line)} /* 85 */,
    (Member){GetType(STRING("int")),STRING("column"),offsetof(Cursor,column)} /* 86 */,
    (Member){GetType(STRING("Range<Cursor>")),STRING("loc"),offsetof(Token,loc)} /* 87 */,
    (Member){GetType(STRING("String")),STRING("foundFirst"),offsetof(FindFirstResult,foundFirst)} /* 88 */,
    (Member){GetType(STRING("Token")),STRING("peekFindNotIncluded"),offsetof(FindFirstResult,peekFindNotIncluded)} /* 89 */,
    (Member){GetType(STRING("u16[128]")),STRING("array"),offsetof(Trie,array)} /* 90 */,
    (Member){GetType(STRING("Array<Trie>")),STRING("subTries"),offsetof(TokenizerTemplate,subTries)} /* 91 */,
    (Member){GetType(STRING("char *")),STRING("ptr"),offsetof(TokenizerMark,ptr)} /* 92 */,
    (Member){GetType(STRING("Cursor")),STRING("pos"),offsetof(TokenizerMark,pos)} /* 93 */,
    (Member){GetType(STRING("char *")),STRING("start"),offsetof(Tokenizer,start)} /* 94 */,
    (Member){GetType(STRING("char *")),STRING("ptr"),offsetof(Tokenizer,ptr)} /* 95 */,
    (Member){GetType(STRING("char *")),STRING("end"),offsetof(Tokenizer,end)} /* 96 */,
    (Member){GetType(STRING("TokenizerTemplate *")),STRING("tmpl"),offsetof(Tokenizer,tmpl)} /* 97 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Tokenizer,line)} /* 98 */,
    (Member){GetType(STRING("int")),STRING("column"),offsetof(Tokenizer,column)} /* 99 */,
    (Member){GetType(STRING("bool")),STRING("keepWhitespaces"),offsetof(Tokenizer,keepWhitespaces)} /* 100 */,
    (Member){GetType(STRING("bool")),STRING("keepComments"),offsetof(Tokenizer,keepComments)} /* 101 */,
    (Member){GetType(STRING("char **")),STRING("op"),offsetof(OperationList,op)} /* 102 */,
    (Member){GetType(STRING("int")),STRING("nOperations"),offsetof(OperationList,nOperations)} /* 103 */,
    (Member){GetType(STRING("OperationList *")),STRING("next"),offsetof(OperationList,next)} /* 104 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(CommandDefinition,name)} /* 105 */,
    (Member){GetType(STRING("int")),STRING("numberExpressions"),offsetof(CommandDefinition,numberExpressions)} /* 106 */,
    (Member){GetType(STRING("CommandType")),STRING("type"),offsetof(CommandDefinition,type)} /* 107 */,
    (Member){GetType(STRING("bool")),STRING("isBlockType"),offsetof(CommandDefinition,isBlockType)} /* 108 */,
    (Member){GetType(STRING("CommandDefinition *")),STRING("definition"),offsetof(Command,definition)} /* 109 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Command,expressions)} /* 110 */,
    (Member){GetType(STRING("String")),STRING("fullText"),offsetof(Command,fullText)} /* 111 */,
    (Member){GetType(STRING("Token")),STRING("fullText"),offsetof(Block,fullText)} /* 112 */,
    (Member){GetType(STRING("String")),STRING("textBlock"),offsetof(Block,textBlock)} /* 113 */,
    (Member){GetType(STRING("Command *")),STRING("command"),offsetof(Block,command)} /* 114 */,
    (Member){GetType(STRING("Expression *")),STRING("expression"),offsetof(Block,expression)} /* 115 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("innerBlocks"),offsetof(Block,innerBlocks)} /* 116 */,
    (Member){GetType(STRING("BlockType")),STRING("type"),offsetof(Block,type)} /* 117 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Block,line)} /* 118 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("arguments"),offsetof(TemplateFunction,arguments)} /* 119 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("blocks"),offsetof(TemplateFunction,blocks)} /* 120 */,
    (Member){GetType(STRING("TemplateRecordType")),STRING("type"),offsetof(TemplateRecord,type)} /* 121 */,
    (Member){GetType(STRING("Type *")),STRING("identifierType"),offsetof(TemplateRecord,identifierType)} /* 122 */,
    (Member){GetType(STRING("String")),STRING("identifierName"),offsetof(TemplateRecord,identifierName)} /* 123 */,
    (Member){GetType(STRING("Type *")),STRING("structType"),offsetof(TemplateRecord,structType)} /* 124 */,
    (Member){GetType(STRING("String")),STRING("fieldName"),offsetof(TemplateRecord,fieldName)} /* 125 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(ValueAndText,val)} /* 126 */,
    (Member){GetType(STRING("String")),STRING("text"),offsetof(ValueAndText,text)} /* 127 */,
    (Member){GetType(STRING("Hashmap<String, Value> *")),STRING("table"),offsetof(Frame,table)} /* 128 */,
    (Member){GetType(STRING("Frame *")),STRING("previousFrame"),offsetof(Frame,previousFrame)} /* 129 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(IndividualBlock,content)} /* 130 */,
    (Member){GetType(STRING("BlockType")),STRING("type"),offsetof(IndividualBlock,type)} /* 131 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(IndividualBlock,line)} /* 132 */,
    (Member){GetType(STRING("int")),STRING("totalMemoryUsed"),offsetof(CompiledTemplate,totalMemoryUsed)} /* 133 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(CompiledTemplate,content)} /* 134 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("blocks"),offsetof(CompiledTemplate,blocks)} /* 135 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(CompiledTemplate,name)} /* 136 */,
    (Member){GetType(STRING("Hashmap<String, Value> *")),STRING("attributes"),offsetof(PortDeclaration,attributes)} /* 137 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("range"),offsetof(PortDeclaration,range)} /* 138 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(PortDeclaration,name)} /* 139 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/common/verilogParsing.hpp:21:3)")),STRING("type"),offsetof(PortDeclaration,type)} /* 140 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ParameterExpression,name)} /* 141 */,
    (Member){GetType(STRING("Expression *")),STRING("expr"),offsetof(ParameterExpression,expr)} /* 142 */,
    (Member){GetType(STRING("Array<ParameterExpression>")),STRING("parameters"),offsetof(Module,parameters)} /* 143 */,
    (Member){GetType(STRING("Array<PortDeclaration>")),STRING("ports"),offsetof(Module,ports)} /* 144 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Module,name)} /* 145 */,
    (Member){GetType(STRING("bool")),STRING("isSource"),offsetof(Module,isSource)} /* 146 */,
    (Member){GetType(STRING("int")),STRING("interface"),offsetof(ExternalMemoryID,interface)} /* 147 */,
    (Member){GetType(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryID,type)} /* 148 */,
    (Member){GetType(STRING("bool")),STRING("write"),offsetof(ExternalInfoTwoPorts,write)} /* 149 */,
    (Member){GetType(STRING("bool")),STRING("read"),offsetof(ExternalInfoTwoPorts,read)} /* 150 */,
    (Member){GetType(STRING("bool")),STRING("enable"),offsetof(ExternalInfoDualPort,enable)} /* 151 */,
    (Member){GetType(STRING("bool")),STRING("write"),offsetof(ExternalInfoDualPort,write)} /* 152 */,
    (Member){GetType(STRING("ExternalInfoTwoPorts")),STRING("tp"),offsetof(ExternalMemoryInfo,tp)} /* 153 */,
    (Member){GetType(STRING("ExternalInfoDualPort[2]")),STRING("dp"),offsetof(ExternalMemoryInfo,dp)} /* 154 */,
    (Member){GetType(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryInfo,type)} /* 155 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ModuleInfo,name)} /* 156 */,
    (Member){GetType(STRING("Array<ParameterExpression>")),STRING("defaultParameters"),offsetof(ModuleInfo,defaultParameters)} /* 157 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelays"),offsetof(ModuleInfo,inputDelays)} /* 158 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ModuleInfo,outputLatencies)} /* 159 */,
    (Member){GetType(STRING("Array<WireExpression>")),STRING("configs"),offsetof(ModuleInfo,configs)} /* 160 */,
    (Member){GetType(STRING("Array<WireExpression>")),STRING("states"),offsetof(ModuleInfo,states)} /* 161 */,
    (Member){GetType(STRING("Array<ExternalMemoryInterfaceExpression>")),STRING("externalInterfaces"),offsetof(ModuleInfo,externalInterfaces)} /* 162 */,
    (Member){GetType(STRING("int")),STRING("nDelays"),offsetof(ModuleInfo,nDelays)} /* 163 */,
    (Member){GetType(STRING("int")),STRING("nIO"),offsetof(ModuleInfo,nIO)} /* 164 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("memoryMappedBits"),offsetof(ModuleInfo,memoryMappedBits)} /* 165 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("databusAddrSize"),offsetof(ModuleInfo,databusAddrSize)} /* 166 */,
    (Member){GetType(STRING("bool")),STRING("doesIO"),offsetof(ModuleInfo,doesIO)} /* 167 */,
    (Member){GetType(STRING("bool")),STRING("memoryMapped"),offsetof(ModuleInfo,memoryMapped)} /* 168 */,
    (Member){GetType(STRING("bool")),STRING("hasDone"),offsetof(ModuleInfo,hasDone)} /* 169 */,
    (Member){GetType(STRING("bool")),STRING("hasClk"),offsetof(ModuleInfo,hasClk)} /* 170 */,
    (Member){GetType(STRING("bool")),STRING("hasReset"),offsetof(ModuleInfo,hasReset)} /* 171 */,
    (Member){GetType(STRING("bool")),STRING("hasRun"),offsetof(ModuleInfo,hasRun)} /* 172 */,
    (Member){GetType(STRING("bool")),STRING("hasRunning"),offsetof(ModuleInfo,hasRunning)} /* 173 */,
    (Member){GetType(STRING("bool")),STRING("isSource"),offsetof(ModuleInfo,isSource)} /* 174 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(ModuleInfo,signalLoop)} /* 175 */,
    (Member){GetType(STRING("Array<String>")),STRING("verilogFiles"),offsetof(Options,verilogFiles)} /* 176 */,
    (Member){GetType(STRING("Array<String>")),STRING("extraSources"),offsetof(Options,extraSources)} /* 177 */,
    (Member){GetType(STRING("Array<String>")),STRING("includePaths"),offsetof(Options,includePaths)} /* 178 */,
    (Member){GetType(STRING("Array<String>")),STRING("unitPaths"),offsetof(Options,unitPaths)} /* 179 */,
    (Member){GetType(STRING("String")),STRING("hardwareOutputFilepath"),offsetof(Options,hardwareOutputFilepath)} /* 180 */,
    (Member){GetType(STRING("String")),STRING("softwareOutputFilepath"),offsetof(Options,softwareOutputFilepath)} /* 181 */,
    (Member){GetType(STRING("String")),STRING("verilatorRoot"),offsetof(Options,verilatorRoot)} /* 182 */,
    (Member){GetType(STRING("String")),STRING("debugPath"),offsetof(Options,debugPath)} /* 183 */,
    (Member){GetType(STRING("char *")),STRING("specificationFilepath"),offsetof(Options,specificationFilepath)} /* 184 */,
    (Member){GetType(STRING("char *")),STRING("topName"),offsetof(Options,topName)} /* 185 */,
    (Member){GetType(STRING("int")),STRING("addrSize"),offsetof(Options,addrSize)} /* 186 */,
    (Member){GetType(STRING("int")),STRING("dataSize"),offsetof(Options,dataSize)} /* 187 */,
    (Member){GetType(STRING("bool")),STRING("addInputAndOutputsToTop"),offsetof(Options,addInputAndOutputsToTop)} /* 188 */,
    (Member){GetType(STRING("bool")),STRING("debug"),offsetof(Options,debug)} /* 189 */,
    (Member){GetType(STRING("bool")),STRING("shadowRegister"),offsetof(Options,shadowRegister)} /* 190 */,
    (Member){GetType(STRING("bool")),STRING("architectureHasDatabus"),offsetof(Options,architectureHasDatabus)} /* 191 */,
    (Member){GetType(STRING("bool")),STRING("useFixedBuffers"),offsetof(Options,useFixedBuffers)} /* 192 */,
    (Member){GetType(STRING("bool")),STRING("generateFSTFormat"),offsetof(Options,generateFSTFormat)} /* 193 */,
    (Member){GetType(STRING("bool")),STRING("disableDelayPropagation"),offsetof(Options,disableDelayPropagation)} /* 194 */,
    (Member){GetType(STRING("bool")),STRING("useDMA"),offsetof(Options,useDMA)} /* 195 */,
    (Member){GetType(STRING("bool")),STRING("exportInternalMemories"),offsetof(Options,exportInternalMemories)} /* 196 */,
    (Member){GetType(STRING("uint")),STRING("dotFormat"),offsetof(DebugState,dotFormat)} /* 197 */,
    (Member){GetType(STRING("bool")),STRING("outputGraphs"),offsetof(DebugState,outputGraphs)} /* 198 */,
    (Member){GetType(STRING("bool")),STRING("outputAccelerator"),offsetof(DebugState,outputAccelerator)} /* 199 */,
    (Member){GetType(STRING("bool")),STRING("outputVersat"),offsetof(DebugState,outputVersat)} /* 200 */,
    (Member){GetType(STRING("bool")),STRING("outputVCD"),offsetof(DebugState,outputVCD)} /* 201 */,
    (Member){GetType(STRING("bool")),STRING("outputAcceleratorInfo"),offsetof(DebugState,outputAcceleratorInfo)} /* 202 */,
    (Member){GetType(STRING("bool")),STRING("useFixedBuffers"),offsetof(DebugState,useFixedBuffers)} /* 203 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(StaticId,parent)} /* 204 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(StaticId,name)} /* 205 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(StaticData,configs)} /* 206 */,
    (Member){GetType(STRING("int")),STRING("offset"),offsetof(StaticData,offset)} /* 207 */,
    (Member){GetType(STRING("StaticId")),STRING("id"),offsetof(StaticInfo,id)} /* 208 */,
    (Member){GetType(STRING("StaticData")),STRING("data"),offsetof(StaticInfo,data)} /* 209 */,
    (Member){GetType(STRING("Array<int>")),STRING("offsets"),offsetof(CalculatedOffsets,offsets)} /* 210 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(CalculatedOffsets,max)} /* 211 */,
    (Member){GetType(STRING("int")),STRING("level"),offsetof(InstanceInfo,level)} /* 212 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("decl"),offsetof(InstanceInfo,decl)} /* 213 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(InstanceInfo,name)} /* 214 */,
    (Member){GetType(STRING("String")),STRING("baseName"),offsetof(InstanceInfo,baseName)} /* 215 */,
    (Member){GetType(STRING("int")),STRING("configPos"),offsetof(InstanceInfo,configPos)} /* 216 */,
    (Member){GetType(STRING("int")),STRING("isConfigStatic"),offsetof(InstanceInfo,isConfigStatic)} /* 217 */,
    (Member){GetType(STRING("int")),STRING("configSize"),offsetof(InstanceInfo,configSize)} /* 218 */,
    (Member){GetType(STRING("int")),STRING("statePos"),offsetof(InstanceInfo,statePos)} /* 219 */,
    (Member){GetType(STRING("int")),STRING("stateSize"),offsetof(InstanceInfo,stateSize)} /* 220 */,
    (Member){GetType(STRING("int")),STRING("memMapped"),offsetof(InstanceInfo,memMapped)} /* 221 */,
    (Member){GetType(STRING("int")),STRING("memMappedSize"),offsetof(InstanceInfo,memMappedSize)} /* 222 */,
    (Member){GetType(STRING("int")),STRING("memMappedBitSize"),offsetof(InstanceInfo,memMappedBitSize)} /* 223 */,
    (Member){GetType(STRING("int")),STRING("memMappedMask"),offsetof(InstanceInfo,memMappedMask)} /* 224 */,
    (Member){GetType(STRING("int")),STRING("delayPos"),offsetof(InstanceInfo,delayPos)} /* 225 */,
    (Member){GetType(STRING("Array<int>")),STRING("delay"),offsetof(InstanceInfo,delay)} /* 226 */,
    (Member){GetType(STRING("int")),STRING("baseDelay"),offsetof(InstanceInfo,baseDelay)} /* 227 */,
    (Member){GetType(STRING("int")),STRING("delaySize"),offsetof(InstanceInfo,delaySize)} /* 228 */,
    (Member){GetType(STRING("bool")),STRING("isComposite"),offsetof(InstanceInfo,isComposite)} /* 229 */,
    (Member){GetType(STRING("bool")),STRING("isStatic"),offsetof(InstanceInfo,isStatic)} /* 230 */,
    (Member){GetType(STRING("bool")),STRING("isShared"),offsetof(InstanceInfo,isShared)} /* 231 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceInfo,parent)} /* 232 */,
    (Member){GetType(STRING("String")),STRING("fullName"),offsetof(InstanceInfo,fullName)} /* 233 */,
    (Member){GetType(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(InstanceInfo,isMergeMultiplexer)} /* 234 */,
    (Member){GetType(STRING("bool")),STRING("belongs"),offsetof(InstanceInfo,belongs)} /* 235 */,
    (Member){GetType(STRING("int")),STRING("special"),offsetof(InstanceInfo,special)} /* 236 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(InstanceInfo,order)} /* 237 */,
    (Member){GetType(STRING("NodeType")),STRING("connectionType"),offsetof(InstanceInfo,connectionType)} /* 238 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(AcceleratorInfo,info)} /* 239 */,
    (Member){GetType(STRING("int")),STRING("memSize"),offsetof(AcceleratorInfo,memSize)} /* 240 */,
    (Member){GetType(STRING("int")),STRING("name"),offsetof(AcceleratorInfo,name)} /* 241 */,
    (Member){GetType(STRING("Hashmap<StaticId, int> *")),STRING("staticInfo"),offsetof(InstanceConfigurationOffsets,staticInfo)} /* 242 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceConfigurationOffsets,parent)} /* 243 */,
    (Member){GetType(STRING("String")),STRING("topName"),offsetof(InstanceConfigurationOffsets,topName)} /* 244 */,
    (Member){GetType(STRING("String")),STRING("baseName"),offsetof(InstanceConfigurationOffsets,baseName)} /* 245 */,
    (Member){GetType(STRING("int")),STRING("configOffset"),offsetof(InstanceConfigurationOffsets,configOffset)} /* 246 */,
    (Member){GetType(STRING("int")),STRING("stateOffset"),offsetof(InstanceConfigurationOffsets,stateOffset)} /* 247 */,
    (Member){GetType(STRING("int")),STRING("delayOffset"),offsetof(InstanceConfigurationOffsets,delayOffset)} /* 248 */,
    (Member){GetType(STRING("int")),STRING("delay"),offsetof(InstanceConfigurationOffsets,delay)} /* 249 */,
    (Member){GetType(STRING("int")),STRING("memOffset"),offsetof(InstanceConfigurationOffsets,memOffset)} /* 250 */,
    (Member){GetType(STRING("int")),STRING("level"),offsetof(InstanceConfigurationOffsets,level)} /* 251 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(InstanceConfigurationOffsets,order)} /* 252 */,
    (Member){GetType(STRING("int *")),STRING("staticConfig"),offsetof(InstanceConfigurationOffsets,staticConfig)} /* 253 */,
    (Member){GetType(STRING("bool")),STRING("belongs"),offsetof(InstanceConfigurationOffsets,belongs)} /* 254 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(TestResult,info)} /* 255 */,
    (Member){GetType(STRING("InstanceConfigurationOffsets")),STRING("subOffsets"),offsetof(TestResult,subOffsets)} /* 256 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TestResult,name)} /* 257 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelay"),offsetof(TestResult,inputDelay)} /* 258 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(TestResult,outputLatencies)} /* 259 */,
    (Member){GetType(STRING("Array<Array<InstanceInfo>>")),STRING("infos"),offsetof(AccelInfo,infos)} /* 260 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("baseInfo"),offsetof(AccelInfo,baseInfo)} /* 261 */,
    (Member){GetType(STRING("Array<String>")),STRING("names"),offsetof(AccelInfo,names)} /* 262 */,
    (Member){GetType(STRING("Array<Array<int>>")),STRING("inputDelays"),offsetof(AccelInfo,inputDelays)} /* 263 */,
    (Member){GetType(STRING("Array<Array<int>>")),STRING("outputDelays"),offsetof(AccelInfo,outputDelays)} /* 264 */,
    (Member){GetType(STRING("int")),STRING("memMappedBitsize"),offsetof(AccelInfo,memMappedBitsize)} /* 265 */,
    (Member){GetType(STRING("int")),STRING("howManyMergedUnits"),offsetof(AccelInfo,howManyMergedUnits)} /* 266 */,
    (Member){GetType(STRING("int")),STRING("inputs"),offsetof(AccelInfo,inputs)} /* 267 */,
    (Member){GetType(STRING("int")),STRING("outputs"),offsetof(AccelInfo,outputs)} /* 268 */,
    (Member){GetType(STRING("int")),STRING("configs"),offsetof(AccelInfo,configs)} /* 269 */,
    (Member){GetType(STRING("int")),STRING("states"),offsetof(AccelInfo,states)} /* 270 */,
    (Member){GetType(STRING("int")),STRING("delays"),offsetof(AccelInfo,delays)} /* 271 */,
    (Member){GetType(STRING("int")),STRING("ios"),offsetof(AccelInfo,ios)} /* 272 */,
    (Member){GetType(STRING("int")),STRING("statics"),offsetof(AccelInfo,statics)} /* 273 */,
    (Member){GetType(STRING("int")),STRING("sharedUnits"),offsetof(AccelInfo,sharedUnits)} /* 274 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(AccelInfo,externalMemoryInterfaces)} /* 275 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryByteSize"),offsetof(AccelInfo,externalMemoryByteSize)} /* 276 */,
    (Member){GetType(STRING("int")),STRING("numberUnits"),offsetof(AccelInfo,numberUnits)} /* 277 */,
    (Member){GetType(STRING("int")),STRING("numberConnections"),offsetof(AccelInfo,numberConnections)} /* 278 */,
    (Member){GetType(STRING("int")),STRING("memoryMappedBits"),offsetof(AccelInfo,memoryMappedBits)} /* 279 */,
    (Member){GetType(STRING("bool")),STRING("isMemoryMapped"),offsetof(AccelInfo,isMemoryMapped)} /* 280 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(AccelInfo,signalLoop)} /* 281 */,
    (Member){GetType(STRING("String")),STRING("type"),offsetof(TypeAndNameOnly,type)} /* 282 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TypeAndNameOnly,name)} /* 283 */,
    (Member){GetType(STRING("int")),STRING("value"),offsetof(Partition,value)} /* 284 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(Partition,max)} /* 285 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(OrderedConfigurations,configs)} /* 286 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("statics"),offsetof(OrderedConfigurations,statics)} /* 287 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("delays"),offsetof(OrderedConfigurations,delays)} /* 288 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ConfigurationInfo,name)} /* 289 */,
    (Member){GetType(STRING("Array<String>")),STRING("baseName"),offsetof(ConfigurationInfo,baseName)} /* 290 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("baseType"),offsetof(ConfigurationInfo,baseType)} /* 291 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(ConfigurationInfo,configs)} /* 292 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("states"),offsetof(ConfigurationInfo,states)} /* 293 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelays"),offsetof(ConfigurationInfo,inputDelays)} /* 294 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ConfigurationInfo,outputLatencies)} /* 295 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("configOffsets"),offsetof(ConfigurationInfo,configOffsets)} /* 296 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("stateOffsets"),offsetof(ConfigurationInfo,stateOffsets)} /* 297 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("delayOffsets"),offsetof(ConfigurationInfo,delayOffsets)} /* 298 */,
    (Member){GetType(STRING("Array<int>")),STRING("calculatedDelays"),offsetof(ConfigurationInfo,calculatedDelays)} /* 299 */,
    (Member){GetType(STRING("Array<int>")),STRING("order"),offsetof(ConfigurationInfo,order)} /* 300 */,
    (Member){GetType(STRING("Array<bool>")),STRING("unitBelongs"),offsetof(ConfigurationInfo,unitBelongs)} /* 301 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(FUDeclaration,name)} /* 302 */,
    (Member){GetType(STRING("ConfigurationInfo")),STRING("baseConfig"),offsetof(FUDeclaration,baseConfig)} /* 303 */,
    (Member){GetType(STRING("Array<ConfigurationInfo>")),STRING("configInfo"),offsetof(FUDeclaration,configInfo)} /* 304 */,
    (Member){GetType(STRING("int")),STRING("memoryMapBits"),offsetof(FUDeclaration,memoryMapBits)} /* 305 */,
    (Member){GetType(STRING("int")),STRING("nIOs"),offsetof(FUDeclaration,nIOs)} /* 306 */,
    (Member){GetType(STRING("Array<ExternalMemoryInterface>")),STRING("externalMemory"),offsetof(FUDeclaration,externalMemory)} /* 307 */,
    (Member){GetType(STRING("Accelerator *")),STRING("baseCircuit"),offsetof(FUDeclaration,baseCircuit)} /* 308 */,
    (Member){GetType(STRING("Accelerator *")),STRING("fixedDelayCircuit"),offsetof(FUDeclaration,fixedDelayCircuit)} /* 309 */,
    (Member){GetType(STRING("char *")),STRING("operation"),offsetof(FUDeclaration,operation)} /* 310 */,
    (Member){GetType(STRING("int")),STRING("lat"),offsetof(FUDeclaration,lat)} /* 311 */,
    (Member){GetType(STRING("Hashmap<StaticId, StaticData> *")),STRING("staticUnits"),offsetof(FUDeclaration,staticUnits)} /* 312 */,
    (Member){GetType(STRING("FUDeclarationType")),STRING("type"),offsetof(FUDeclaration,type)} /* 313 */,
    (Member){GetType(STRING("DelayType")),STRING("delayType"),offsetof(FUDeclaration,delayType)} /* 314 */,
    (Member){GetType(STRING("bool")),STRING("isOperation"),offsetof(FUDeclaration,isOperation)} /* 315 */,
    (Member){GetType(STRING("bool")),STRING("implementsDone"),offsetof(FUDeclaration,implementsDone)} /* 316 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(FUDeclaration,signalLoop)} /* 317 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(PortInstance,inst)} /* 318 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(PortInstance,port)} /* 319 */,
    (Member){GetType(STRING("PortInstance[2]")),STRING("units"),offsetof(PortEdge,units)} /* 320 */,
    (Member){GetType(STRING("PortInstance")),STRING("out"),offsetof(Edge,out)} /* 321 */,
    (Member){GetType(STRING("PortInstance")),STRING("in"),offsetof(Edge,in)} /* 322 */,
    (Member){GetType(STRING("PortEdge")),STRING("edge"),offsetof(Edge,edge)} /* 323 */,
    (Member){GetType(STRING("PortInstance[2]")),STRING("units"),offsetof(Edge,units)} /* 324 */,
    (Member){GetType(STRING("int")),STRING("delay"),offsetof(Edge,delay)} /* 325 */,
    (Member){GetType(STRING("Edge *")),STRING("next"),offsetof(Edge,next)} /* 326 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("node"),offsetof(PortNode,node)} /* 327 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(PortNode,port)} /* 328 */,
    (Member){GetType(STRING("PortNode")),STRING("node0"),offsetof(EdgeNode,node0)} /* 329 */,
    (Member){GetType(STRING("PortNode")),STRING("node1"),offsetof(EdgeNode,node1)} /* 330 */,
    (Member){GetType(STRING("int")),STRING("delay"),offsetof(EdgeNode,delay)} /* 331 */,
    (Member){GetType(STRING("PortNode")),STRING("instConnectedTo"),offsetof(ConnectionNode,instConnectedTo)} /* 332 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(ConnectionNode,port)} /* 333 */,
    (Member){GetType(STRING("int")),STRING("edgeDelay"),offsetof(ConnectionNode,edgeDelay)} /* 334 */,
    (Member){GetType(STRING("int *")),STRING("delay"),offsetof(ConnectionNode,delay)} /* 335 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("next"),offsetof(ConnectionNode,next)} /* 336 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(InstanceNode,inst)} /* 337 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("next"),offsetof(InstanceNode,next)} /* 338 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("allInputs"),offsetof(InstanceNode,allInputs)} /* 339 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("allOutputs"),offsetof(InstanceNode,allOutputs)} /* 340 */,
    (Member){GetType(STRING("Array<PortNode>")),STRING("inputs"),offsetof(InstanceNode,inputs)} /* 341 */,
    (Member){GetType(STRING("Array<bool>")),STRING("outputs"),offsetof(InstanceNode,outputs)} /* 342 */,
    (Member){GetType(STRING("bool")),STRING("multipleSamePortInputs"),offsetof(InstanceNode,multipleSamePortInputs)} /* 343 */,
    (Member){GetType(STRING("NodeType")),STRING("type"),offsetof(InstanceNode,type)} /* 344 */,
    (Member){GetType(STRING("Edge *")),STRING("edges"),offsetof(Accelerator,edges)} /* 345 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("allocated"),offsetof(Accelerator,allocated)} /* 346 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("lastAllocated"),offsetof(Accelerator,lastAllocated)} /* 347 */,
    (Member){GetType(STRING("Pool<FUInstance>")),STRING("instances"),offsetof(Accelerator,instances)} /* 348 */,
    (Member){GetType(STRING("DynamicArena *")),STRING("accelMemory"),offsetof(Accelerator,accelMemory)} /* 349 */,
    (Member){GetType(STRING("int")),STRING("staticUnits"),offsetof(Accelerator,staticUnits)} /* 350 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Accelerator,name)} /* 351 */,
    (Member){GetType(STRING("int")),STRING("memoryMaskSize"),offsetof(MemoryAddressMask,memoryMaskSize)} /* 352 */,
    (Member){GetType(STRING("char[33]")),STRING("memoryMaskBuffer"),offsetof(MemoryAddressMask,memoryMaskBuffer)} /* 353 */,
    (Member){GetType(STRING("char *")),STRING("memoryMask"),offsetof(MemoryAddressMask,memoryMask)} /* 354 */,
    (Member){GetType(STRING("int")),STRING("nConfigs"),offsetof(VersatComputedValues,nConfigs)} /* 355 */,
    (Member){GetType(STRING("int")),STRING("configBits"),offsetof(VersatComputedValues,configBits)} /* 356 */,
    (Member){GetType(STRING("int")),STRING("versatConfigs"),offsetof(VersatComputedValues,versatConfigs)} /* 357 */,
    (Member){GetType(STRING("int")),STRING("versatStates"),offsetof(VersatComputedValues,versatStates)} /* 358 */,
    (Member){GetType(STRING("int")),STRING("nStatics"),offsetof(VersatComputedValues,nStatics)} /* 359 */,
    (Member){GetType(STRING("int")),STRING("staticBits"),offsetof(VersatComputedValues,staticBits)} /* 360 */,
    (Member){GetType(STRING("int")),STRING("staticBitsStart"),offsetof(VersatComputedValues,staticBitsStart)} /* 361 */,
    (Member){GetType(STRING("int")),STRING("nDelays"),offsetof(VersatComputedValues,nDelays)} /* 362 */,
    (Member){GetType(STRING("int")),STRING("delayBits"),offsetof(VersatComputedValues,delayBits)} /* 363 */,
    (Member){GetType(STRING("int")),STRING("delayBitsStart"),offsetof(VersatComputedValues,delayBitsStart)} /* 364 */,
    (Member){GetType(STRING("int")),STRING("nConfigurations"),offsetof(VersatComputedValues,nConfigurations)} /* 365 */,
    (Member){GetType(STRING("int")),STRING("configurationBits"),offsetof(VersatComputedValues,configurationBits)} /* 366 */,
    (Member){GetType(STRING("int")),STRING("configurationAddressBits"),offsetof(VersatComputedValues,configurationAddressBits)} /* 367 */,
    (Member){GetType(STRING("int")),STRING("nStates"),offsetof(VersatComputedValues,nStates)} /* 368 */,
    (Member){GetType(STRING("int")),STRING("stateBits"),offsetof(VersatComputedValues,stateBits)} /* 369 */,
    (Member){GetType(STRING("int")),STRING("stateAddressBits"),offsetof(VersatComputedValues,stateAddressBits)} /* 370 */,
    (Member){GetType(STRING("int")),STRING("unitsMapped"),offsetof(VersatComputedValues,unitsMapped)} /* 371 */,
    (Member){GetType(STRING("int")),STRING("memoryMappedBytes"),offsetof(VersatComputedValues,memoryMappedBytes)} /* 372 */,
    (Member){GetType(STRING("int")),STRING("nUnitsIO"),offsetof(VersatComputedValues,nUnitsIO)} /* 373 */,
    (Member){GetType(STRING("int")),STRING("numberConnections"),offsetof(VersatComputedValues,numberConnections)} /* 374 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(VersatComputedValues,externalMemoryInterfaces)} /* 375 */,
    (Member){GetType(STRING("int")),STRING("stateConfigurationAddressBits"),offsetof(VersatComputedValues,stateConfigurationAddressBits)} /* 376 */,
    (Member){GetType(STRING("int")),STRING("memoryAddressBits"),offsetof(VersatComputedValues,memoryAddressBits)} /* 377 */,
    (Member){GetType(STRING("int")),STRING("memoryMappingAddressBits"),offsetof(VersatComputedValues,memoryMappingAddressBits)} /* 378 */,
    (Member){GetType(STRING("int")),STRING("memoryConfigDecisionBit"),offsetof(VersatComputedValues,memoryConfigDecisionBit)} /* 379 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(VersatComputedValues,signalLoop)} /* 380 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("sinks"),offsetof(DAGOrderNodes,sinks)} /* 381 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("sources"),offsetof(DAGOrderNodes,sources)} /* 382 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("computeUnits"),offsetof(DAGOrderNodes,computeUnits)} /* 383 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("instances"),offsetof(DAGOrderNodes,instances)} /* 384 */,
    (Member){GetType(STRING("Array<int>")),STRING("order"),offsetof(DAGOrderNodes,order)} /* 385 */,
    (Member){GetType(STRING("int")),STRING("size"),offsetof(DAGOrderNodes,size)} /* 386 */,
    (Member){GetType(STRING("int")),STRING("maxOrder"),offsetof(DAGOrderNodes,maxOrder)} /* 387 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(GraphPrintingNodeInfo,name)} /* 388 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(GraphPrintingNodeInfo,content)} /* 389 */,
    (Member){GetType(STRING("GraphPrintingColor")),STRING("color"),offsetof(GraphPrintingNodeInfo,color)} /* 390 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(GraphPrintingEdgeInfo,content)} /* 391 */,
    (Member){GetType(STRING("GraphPrintingColor")),STRING("color"),offsetof(GraphPrintingEdgeInfo,color)} /* 392 */,
    (Member){GetType(STRING("String")),STRING("first"),offsetof(GraphPrintingEdgeInfo,first)} /* 393 */,
    (Member){GetType(STRING("String")),STRING("second"),offsetof(GraphPrintingEdgeInfo,second)} /* 394 */,
    (Member){GetType(STRING("String")),STRING("graphLabel"),offsetof(GraphPrintingContent,graphLabel)} /* 395 */,
    (Member){GetType(STRING("Array<GraphPrintingNodeInfo>")),STRING("nodes"),offsetof(GraphPrintingContent,nodes)} /* 396 */,
    (Member){GetType(STRING("Array<GraphPrintingEdgeInfo>")),STRING("edges"),offsetof(GraphPrintingContent,edges)} /* 397 */,
    (Member){GetType(STRING("Hashmap<EdgeNode, int> *")),STRING("edgesDelay"),offsetof(CalculateDelayResult,edgesDelay)} /* 398 */,
    (Member){GetType(STRING("Hashmap<PortNode, int> *")),STRING("portDelay"),offsetof(CalculateDelayResult,portDelay)} /* 399 */,
    (Member){GetType(STRING("Hashmap<InstanceNode *, int> *")),STRING("nodeDelay"),offsetof(CalculateDelayResult,nodeDelay)} /* 400 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(FUInstance,name)} /* 401 */,
    (Member){GetType(STRING("String")),STRING("parameters"),offsetof(FUInstance,parameters)} /* 402 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel"),offsetof(FUInstance,accel)} /* 403 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("declaration"),offsetof(FUInstance,declaration)} /* 404 */,
    (Member){GetType(STRING("int")),STRING("id"),offsetof(FUInstance,id)} /* 405 */,
    (Member){GetType(STRING("int")),STRING("literal"),offsetof(FUInstance,literal)} /* 406 */,
    (Member){GetType(STRING("int")),STRING("bufferAmount"),offsetof(FUInstance,bufferAmount)} /* 407 */,
    (Member){GetType(STRING("int")),STRING("portIndex"),offsetof(FUInstance,portIndex)} /* 408 */,
    (Member){GetType(STRING("int")),STRING("sharedIndex"),offsetof(FUInstance,sharedIndex)} /* 409 */,
    (Member){GetType(STRING("bool")),STRING("isStatic"),offsetof(FUInstance,isStatic)} /* 410 */,
    (Member){GetType(STRING("bool")),STRING("sharedEnable"),offsetof(FUInstance,sharedEnable)} /* 411 */,
    (Member){GetType(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(FUInstance,isMergeMultiplexer)} /* 412 */,
    (Member){GetType(STRING("EdgeNode")),STRING("edge"),offsetof(DelayToAdd,edge)} /* 413 */,
    (Member){GetType(STRING("String")),STRING("bufferName"),offsetof(DelayToAdd,bufferName)} /* 414 */,
    (Member){GetType(STRING("String")),STRING("bufferParameters"),offsetof(DelayToAdd,bufferParameters)} /* 415 */,
    (Member){GetType(STRING("int")),STRING("bufferAmount"),offsetof(DelayToAdd,bufferAmount)} /* 416 */,
    (Member){GetType(STRING("String")),STRING("type"),offsetof(SingleTypeStructElement,type)} /* 417 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(SingleTypeStructElement,name)} /* 418 */,
    (Member){GetType(STRING("Array<SingleTypeStructElement>")),STRING("typeAndNames"),offsetof(TypeStructInfoElement,typeAndNames)} /* 419 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TypeStructInfo,name)} /* 420 */,
    (Member){GetType(STRING("Array<TypeStructInfoElement>")),STRING("entries"),offsetof(TypeStructInfo,entries)} /* 421 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(Difference,index)} /* 422 */,
    (Member){GetType(STRING("int")),STRING("newValue"),offsetof(Difference,newValue)} /* 423 */,
    (Member){GetType(STRING("int")),STRING("oldIndex"),offsetof(DifferenceArray,oldIndex)} /* 424 */,
    (Member){GetType(STRING("int")),STRING("newIndex"),offsetof(DifferenceArray,newIndex)} /* 425 */,
    (Member){GetType(STRING("Array<Difference>")),STRING("differences"),offsetof(DifferenceArray,differences)} /* 426 */,
    (Member){GetType(STRING("TaskFunction")),STRING("function"),offsetof(Task,function)} /* 427 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(Task,order)} /* 428 */,
    (Member){GetType(STRING("void *")),STRING("args"),offsetof(Task,args)} /* 429 */,
    (Member){GetType(STRING("TaskFunction")),STRING("function"),offsetof(WorkGroup,function)} /* 430 */,
    (Member){GetType(STRING("Array<Task>")),STRING("tasks"),offsetof(WorkGroup,tasks)} /* 431 */,
    (Member){GetType(STRING("String")),STRING("instA"),offsetof(SpecificMerge,instA)} /* 432 */,
    (Member){GetType(STRING("String")),STRING("instB"),offsetof(SpecificMerge,instB)} /* 433 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(IndexRecord,index)} /* 434 */,
    (Member){GetType(STRING("IndexRecord *")),STRING("next"),offsetof(IndexRecord,next)} /* 435 */,
    (Member){GetType(STRING("FUInstance *")),STRING("instA"),offsetof(SpecificMergeNodes,instA)} /* 436 */,
    (Member){GetType(STRING("FUInstance *")),STRING("instB"),offsetof(SpecificMergeNodes,instB)} /* 437 */,
    (Member){GetType(STRING("int")),STRING("firstIndex"),offsetof(SpecificMergeNode,firstIndex)} /* 438 */,
    (Member){GetType(STRING("String")),STRING("firstName"),offsetof(SpecificMergeNode,firstName)} /* 439 */,
    (Member){GetType(STRING("int")),STRING("secondIndex"),offsetof(SpecificMergeNode,secondIndex)} /* 440 */,
    (Member){GetType(STRING("String")),STRING("secondName"),offsetof(SpecificMergeNode,secondName)} /* 441 */,
    (Member){GetType(STRING("FUInstance *[2]")),STRING("instances"),offsetof(MergeEdge,instances)} /* 442 */,
    (Member){GetType(STRING("MergeEdge")),STRING("nodes"),offsetof(MappingNode,nodes)} /* 443 */,
    (Member){GetType(STRING("PortEdge[2]")),STRING("edges"),offsetof(MappingNode,edges)} /* 444 */,
    (Member){GetType(STRING("enum (unnamed enum at /home/z/AA/Versat/iob-soc-versat/submodules/VERSAT/software/compiler/merge.hpp:46:4)")),STRING("type"),offsetof(MappingNode,type)} /* 445 */,
    (Member){GetType(STRING("MappingNode *[2]")),STRING("nodes"),offsetof(MappingEdge,nodes)} /* 446 */,
    (Member){GetType(STRING("Array<SpecificMergeNodes>")),STRING("specifics"),offsetof(ConsolidationGraphOptions,specifics)} /* 447 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(ConsolidationGraphOptions,order)} /* 448 */,
    (Member){GetType(STRING("int")),STRING("difference"),offsetof(ConsolidationGraphOptions,difference)} /* 449 */,
    (Member){GetType(STRING("bool")),STRING("mapNodes"),offsetof(ConsolidationGraphOptions,mapNodes)} /* 450 */,
    (Member){GetType(STRING("enum (unnamed enum at /home/z/AA/Versat/iob-soc-versat/submodules/VERSAT/software/compiler/merge.hpp:64:4)")),STRING("type"),offsetof(ConsolidationGraphOptions,type)} /* 451 */,
    (Member){GetType(STRING("Array<MappingNode>")),STRING("nodes"),offsetof(ConsolidationGraph,nodes)} /* 452 */,
    (Member){GetType(STRING("Array<BitArray>")),STRING("edges"),offsetof(ConsolidationGraph,edges)} /* 453 */,
    (Member){GetType(STRING("BitArray")),STRING("validNodes"),offsetof(ConsolidationGraph,validNodes)} /* 454 */,
    (Member){GetType(STRING("ConsolidationGraph")),STRING("graph"),offsetof(ConsolidationResult,graph)} /* 455 */,
    (Member){GetType(STRING("Pool<MappingNode>")),STRING("specificsAdded"),offsetof(ConsolidationResult,specificsAdded)} /* 456 */,
    (Member){GetType(STRING("int")),STRING("upperBound"),offsetof(ConsolidationResult,upperBound)} /* 457 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(CliqueState,max)} /* 458 */,
    (Member){GetType(STRING("int")),STRING("upperBound"),offsetof(CliqueState,upperBound)} /* 459 */,
    (Member){GetType(STRING("int")),STRING("startI"),offsetof(CliqueState,startI)} /* 460 */,
    (Member){GetType(STRING("int")),STRING("iterations"),offsetof(CliqueState,iterations)} /* 461 */,
    (Member){GetType(STRING("Array<int>")),STRING("table"),offsetof(CliqueState,table)} /* 462 */,
    (Member){GetType(STRING("ConsolidationGraph")),STRING("clique"),offsetof(CliqueState,clique)} /* 463 */,
    (Member){GetType(STRING("Time")),STRING("start"),offsetof(CliqueState,start)} /* 464 */,
    (Member){GetType(STRING("bool")),STRING("found"),offsetof(CliqueState,found)} /* 465 */,
    (Member){GetType(STRING("bool")),STRING("result"),offsetof(IsCliqueResult,result)} /* 466 */,
    (Member){GetType(STRING("int")),STRING("failedIndex"),offsetof(IsCliqueResult,failedIndex)} /* 467 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel1"),offsetof(MergeGraphResult,accel1)} /* 468 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResult,accel2)} /* 469 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map1"),offsetof(MergeGraphResult,map1)} /* 470 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map2"),offsetof(MergeGraphResult,map2)} /* 471 */,
    (Member){GetType(STRING("Accelerator *")),STRING("newGraph"),offsetof(MergeGraphResult,newGraph)} /* 472 */,
    (Member){GetType(STRING("Accelerator *")),STRING("result"),offsetof(MergeGraphResultExisting,result)} /* 473 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResultExisting,accel2)} /* 474 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map2"),offsetof(MergeGraphResultExisting,map2)} /* 475 */,
    (Member){GetType(STRING("InstanceMap *")),STRING("instanceMap"),offsetof(GraphMapping,instanceMap)} /* 476 */,
    (Member){GetType(STRING("InstanceMap *")),STRING("reverseInstanceMap"),offsetof(GraphMapping,reverseInstanceMap)} /* 477 */,
    (Member){GetType(STRING("PortEdgeMap *")),STRING("edgeMap"),offsetof(GraphMapping,edgeMap)} /* 478 */,
    (Member){GetType(STRING("Range<int>")),STRING("port"),offsetof(ConnectionExtra,port)} /* 479 */,
    (Member){GetType(STRING("Range<int>")),STRING("delay"),offsetof(ConnectionExtra,delay)} /* 480 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(Var,name)} /* 481 */,
    (Member){GetType(STRING("ConnectionExtra")),STRING("extra"),offsetof(Var,extra)} /* 482 */,
    (Member){GetType(STRING("Range<int>")),STRING("index"),offsetof(Var,index)} /* 483 */,
    (Member){GetType(STRING("bool")),STRING("isArrayAccess"),offsetof(Var,isArrayAccess)} /* 484 */,
    (Member){GetType(STRING("Array<Var>")),STRING("vars"),offsetof(VarGroup,vars)} /* 485 */,
    (Member){GetType(STRING("Token")),STRING("fullText"),offsetof(VarGroup,fullText)} /* 486 */,
    (Member){GetType(STRING("Array<SpecExpression *>")),STRING("expressions"),offsetof(SpecExpression,expressions)} /* 487 */,
    (Member){GetType(STRING("char *")),STRING("op"),offsetof(SpecExpression,op)} /* 488 */,
    (Member){GetType(STRING("Var")),STRING("var"),offsetof(SpecExpression,var)} /* 489 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(SpecExpression,val)} /* 490 */,
    (Member){GetType(STRING("Token")),STRING("text"),offsetof(SpecExpression,text)} /* 491 */,
    (Member){GetType(STRING("enum (unnamed enum at /home/z/AA/Versat/iob-soc-versat/submodules/VERSAT/software/compiler/versatSpecificationParser.hpp:50:3)")),STRING("type"),offsetof(SpecExpression,type)} /* 492 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(VarDeclaration,name)} /* 493 */,
    (Member){GetType(STRING("int")),STRING("arraySize"),offsetof(VarDeclaration,arraySize)} /* 494 */,
    (Member){GetType(STRING("bool")),STRING("isArray"),offsetof(VarDeclaration,isArray)} /* 495 */,
    (Member){GetType(STRING("VarGroup")),STRING("group"),offsetof(GroupIterator,group)} /* 496 */,
    (Member){GetType(STRING("int")),STRING("groupIndex"),offsetof(GroupIterator,groupIndex)} /* 497 */,
    (Member){GetType(STRING("int")),STRING("varIndex"),offsetof(GroupIterator,varIndex)} /* 498 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(PortExpression,inst)} /* 499 */,
    (Member){GetType(STRING("ConnectionExtra")),STRING("extra"),offsetof(PortExpression,extra)} /* 500 */,
    (Member){GetType(STRING("enum (unnamed enum at /home/z/AA/Versat/iob-soc-versat/submodules/VERSAT/software/compiler/versatSpecificationParser.hpp:71:3)")),STRING("modifier"),offsetof(InstanceDeclaration,modifier)} /* 501 */,
    (Member){GetType(STRING("Token")),STRING("typeName"),offsetof(InstanceDeclaration,typeName)} /* 502 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("declarations"),offsetof(InstanceDeclaration,declarations)} /* 503 */,
    (Member){GetType(STRING("String")),STRING("parameters"),offsetof(InstanceDeclaration,parameters)} /* 504 */,
    (Member){GetType(STRING("Range<Cursor>")),STRING("loc"),offsetof(ConnectionDef,loc)} /* 505 */,
    (Member){GetType(STRING("VarGroup")),STRING("output"),offsetof(ConnectionDef,output)} /* 506 */,
    (Member){GetType(STRING("enum (unnamed enum at /home/z/AA/Versat/iob-soc-versat/submodules/VERSAT/software/compiler/versatSpecificationParser.hpp:81:3)")),STRING("type"),offsetof(ConnectionDef,type)} /* 507 */,
    (Member){GetType(STRING("Array<Token>")),STRING("transforms"),offsetof(ConnectionDef,transforms)} /* 508 */,
    (Member){GetType(STRING("VarGroup")),STRING("input"),offsetof(ConnectionDef,input)} /* 509 */,
    (Member){GetType(STRING("SpecExpression *")),STRING("expression"),offsetof(ConnectionDef,expression)} /* 510 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(ModuleDef,name)} /* 511 */,
    (Member){GetType(STRING("Token")),STRING("numberOutputs"),offsetof(ModuleDef,numberOutputs)} /* 512 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(ModuleDef,inputs)} /* 513 */,
    (Member){GetType(STRING("Array<InstanceDeclaration>")),STRING("declarations"),offsetof(ModuleDef,declarations)} /* 514 */,
    (Member){GetType(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(ModuleDef,connections)} /* 515 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(TransformDef,name)} /* 516 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(TransformDef,inputs)} /* 517 */,
    (Member){GetType(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(TransformDef,connections)} /* 518 */,
    (Member){GetType(STRING("int")),STRING("inputs"),offsetof(Transformation,inputs)} /* 519 */,
    (Member){GetType(STRING("int")),STRING("outputs"),offsetof(Transformation,outputs)} /* 520 */,
    (Member){GetType(STRING("Array<int>")),STRING("map"),offsetof(Transformation,map)} /* 521 */,
    (Member){GetType(STRING("Token")),STRING("instanceName"),offsetof(HierarchicalName,instanceName)} /* 522 */,
    (Member){GetType(STRING("Var")),STRING("subInstance"),offsetof(HierarchicalName,subInstance)} /* 523 */,
    (Member){GetType(STRING("Token")),STRING("typeName"),offsetof(TypeAndInstance,typeName)} /* 524 */,
    (Member){GetType(STRING("Token")),STRING("instanceName"),offsetof(TypeAndInstance,instanceName)} /* 525 */
  };

  RegisterStructMembers(STRING("Test"),(Array<Member>){&members[0],2});
  RegisterStructMembers(STRING("Test3"),(Array<Member>){&members[2],1});
  RegisterStructMembers(STRING("Time"),(Array<Member>){&members[3],2});
  RegisterStructMembers(STRING("Conversion"),(Array<Member>){&members[5],3});
  RegisterStructMembers(STRING("Arena"),(Array<Member>){&members[8],5});
  RegisterStructMembers(STRING("ArenaMark"),(Array<Member>){&members[13],2});
  RegisterStructMembers(STRING("DynamicArena"),(Array<Member>){&members[15],4});
  RegisterStructMembers(STRING("DynamicString"),(Array<Member>){&members[19],2});
  RegisterStructMembers(STRING("BitArray"),(Array<Member>){&members[21],3});
  RegisterStructMembers(STRING("PoolHeader"),(Array<Member>){&members[24],2});
  RegisterStructMembers(STRING("PoolInfo"),(Array<Member>){&members[26],4});
  RegisterStructMembers(STRING("PageInfo"),(Array<Member>){&members[30],2});
  RegisterStructMembers(STRING("EnumMember"),(Array<Member>){&members[32],3});
  RegisterStructMembers(STRING("TemplateArg"),(Array<Member>){&members[35],2});
  RegisterStructMembers(STRING("TemplatedMember"),(Array<Member>){&members[37],3});
  RegisterStructMembers(STRING("Type"),(Array<Member>){&members[40],14});
  RegisterStructMembers(STRING("Member"),(Array<Member>){&members[54],6});
  RegisterStructMembers(STRING("TypeIterator"),(Array<Member>){&members[60],2});
  RegisterStructMembers(STRING("Value"),(Array<Member>){&members[62],9});
  RegisterStructMembers(STRING("Iterator"),(Array<Member>){&members[71],4});
  RegisterStructMembers(STRING("HashmapUnpackedIndex"),(Array<Member>){&members[75],2});
  RegisterStructMembers(STRING("Expression"),(Array<Member>){&members[77],8});
  RegisterStructMembers(STRING("Cursor"),(Array<Member>){&members[85],2});
  RegisterStructMembers(STRING("Token"),(Array<Member>){&members[87],1});
  RegisterStructMembers(STRING("FindFirstResult"),(Array<Member>){&members[88],2});
  RegisterStructMembers(STRING("Trie"),(Array<Member>){&members[90],1});
  RegisterStructMembers(STRING("TokenizerTemplate"),(Array<Member>){&members[91],1});
  RegisterStructMembers(STRING("TokenizerMark"),(Array<Member>){&members[92],2});
  RegisterStructMembers(STRING("Tokenizer"),(Array<Member>){&members[94],8});
  RegisterStructMembers(STRING("OperationList"),(Array<Member>){&members[102],3});
  RegisterStructMembers(STRING("CommandDefinition"),(Array<Member>){&members[105],4});
  RegisterStructMembers(STRING("Command"),(Array<Member>){&members[109],3});
  RegisterStructMembers(STRING("Block"),(Array<Member>){&members[112],7});
  RegisterStructMembers(STRING("TemplateFunction"),(Array<Member>){&members[119],2});
  RegisterStructMembers(STRING("TemplateRecord"),(Array<Member>){&members[121],5});
  RegisterStructMembers(STRING("ValueAndText"),(Array<Member>){&members[126],2});
  RegisterStructMembers(STRING("Frame"),(Array<Member>){&members[128],2});
  RegisterStructMembers(STRING("IndividualBlock"),(Array<Member>){&members[130],3});
  RegisterStructMembers(STRING("CompiledTemplate"),(Array<Member>){&members[133],4});
  RegisterStructMembers(STRING("PortDeclaration"),(Array<Member>){&members[137],4});
  RegisterStructMembers(STRING("ParameterExpression"),(Array<Member>){&members[141],2});
  RegisterStructMembers(STRING("Module"),(Array<Member>){&members[143],4});
  RegisterStructMembers(STRING("ExternalMemoryID"),(Array<Member>){&members[147],2});
  RegisterStructMembers(STRING("ExternalInfoTwoPorts"),(Array<Member>){&members[149],2});
  RegisterStructMembers(STRING("ExternalInfoDualPort"),(Array<Member>){&members[151],2});
  RegisterStructMembers(STRING("ExternalMemoryInfo"),(Array<Member>){&members[153],3});
  RegisterStructMembers(STRING("ModuleInfo"),(Array<Member>){&members[156],20});
  RegisterStructMembers(STRING("Options"),(Array<Member>){&members[176],21});
  RegisterStructMembers(STRING("DebugState"),(Array<Member>){&members[197],7});
  RegisterStructMembers(STRING("StaticId"),(Array<Member>){&members[204],2});
  RegisterStructMembers(STRING("StaticData"),(Array<Member>){&members[206],2});
  RegisterStructMembers(STRING("StaticInfo"),(Array<Member>){&members[208],2});
  RegisterStructMembers(STRING("CalculatedOffsets"),(Array<Member>){&members[210],2});
  RegisterStructMembers(STRING("InstanceInfo"),(Array<Member>){&members[212],27});
  RegisterStructMembers(STRING("AcceleratorInfo"),(Array<Member>){&members[239],3});
  RegisterStructMembers(STRING("InstanceConfigurationOffsets"),(Array<Member>){&members[242],13});
  RegisterStructMembers(STRING("TestResult"),(Array<Member>){&members[255],5});
  RegisterStructMembers(STRING("AccelInfo"),(Array<Member>){&members[260],22});
  RegisterStructMembers(STRING("TypeAndNameOnly"),(Array<Member>){&members[282],2});
  RegisterStructMembers(STRING("Partition"),(Array<Member>){&members[284],2});
  RegisterStructMembers(STRING("OrderedConfigurations"),(Array<Member>){&members[286],3});
  RegisterStructMembers(STRING("ConfigurationInfo"),(Array<Member>){&members[289],13});
  RegisterStructMembers(STRING("FUDeclaration"),(Array<Member>){&members[302],16});
  RegisterStructMembers(STRING("PortInstance"),(Array<Member>){&members[318],2});
  RegisterStructMembers(STRING("PortEdge"),(Array<Member>){&members[320],1});
  RegisterStructMembers(STRING("Edge"),(Array<Member>){&members[321],6});
  RegisterStructMembers(STRING("PortNode"),(Array<Member>){&members[327],2});
  RegisterStructMembers(STRING("EdgeNode"),(Array<Member>){&members[329],3});
  RegisterStructMembers(STRING("ConnectionNode"),(Array<Member>){&members[332],5});
  RegisterStructMembers(STRING("InstanceNode"),(Array<Member>){&members[337],8});
  RegisterStructMembers(STRING("Accelerator"),(Array<Member>){&members[345],7});
  RegisterStructMembers(STRING("MemoryAddressMask"),(Array<Member>){&members[352],3});
  RegisterStructMembers(STRING("VersatComputedValues"),(Array<Member>){&members[355],26});
  RegisterStructMembers(STRING("DAGOrderNodes"),(Array<Member>){&members[381],7});
  RegisterStructMembers(STRING("GraphPrintingNodeInfo"),(Array<Member>){&members[388],3});
  RegisterStructMembers(STRING("GraphPrintingEdgeInfo"),(Array<Member>){&members[391],4});
  RegisterStructMembers(STRING("GraphPrintingContent"),(Array<Member>){&members[395],3});
  RegisterStructMembers(STRING("CalculateDelayResult"),(Array<Member>){&members[398],3});
  RegisterStructMembers(STRING("FUInstance"),(Array<Member>){&members[401],12});
  RegisterStructMembers(STRING("DelayToAdd"),(Array<Member>){&members[413],4});
  RegisterStructMembers(STRING("SingleTypeStructElement"),(Array<Member>){&members[417],2});
  RegisterStructMembers(STRING("TypeStructInfoElement"),(Array<Member>){&members[419],1});
  RegisterStructMembers(STRING("TypeStructInfo"),(Array<Member>){&members[420],2});
  RegisterStructMembers(STRING("Difference"),(Array<Member>){&members[422],2});
  RegisterStructMembers(STRING("DifferenceArray"),(Array<Member>){&members[424],3});
  RegisterStructMembers(STRING("Task"),(Array<Member>){&members[427],3});
  RegisterStructMembers(STRING("WorkGroup"),(Array<Member>){&members[430],2});
  RegisterStructMembers(STRING("SpecificMerge"),(Array<Member>){&members[432],2});
  RegisterStructMembers(STRING("IndexRecord"),(Array<Member>){&members[434],2});
  RegisterStructMembers(STRING("SpecificMergeNodes"),(Array<Member>){&members[436],2});
  RegisterStructMembers(STRING("SpecificMergeNode"),(Array<Member>){&members[438],4});
  RegisterStructMembers(STRING("MergeEdge"),(Array<Member>){&members[442],1});
  RegisterStructMembers(STRING("MappingNode"),(Array<Member>){&members[443],3});
  RegisterStructMembers(STRING("MappingEdge"),(Array<Member>){&members[446],1});
  RegisterStructMembers(STRING("ConsolidationGraphOptions"),(Array<Member>){&members[447],5});
  RegisterStructMembers(STRING("ConsolidationGraph"),(Array<Member>){&members[452],3});
  RegisterStructMembers(STRING("ConsolidationResult"),(Array<Member>){&members[455],3});
  RegisterStructMembers(STRING("CliqueState"),(Array<Member>){&members[458],8});
  RegisterStructMembers(STRING("IsCliqueResult"),(Array<Member>){&members[466],2});
  RegisterStructMembers(STRING("MergeGraphResult"),(Array<Member>){&members[468],5});
  RegisterStructMembers(STRING("MergeGraphResultExisting"),(Array<Member>){&members[473],3});
  RegisterStructMembers(STRING("GraphMapping"),(Array<Member>){&members[476],3});
  RegisterStructMembers(STRING("ConnectionExtra"),(Array<Member>){&members[479],2});
  RegisterStructMembers(STRING("Var"),(Array<Member>){&members[481],4});
  RegisterStructMembers(STRING("VarGroup"),(Array<Member>){&members[485],2});
  RegisterStructMembers(STRING("SpecExpression"),(Array<Member>){&members[487],6});
  RegisterStructMembers(STRING("VarDeclaration"),(Array<Member>){&members[493],3});
  RegisterStructMembers(STRING("GroupIterator"),(Array<Member>){&members[496],3});
  RegisterStructMembers(STRING("PortExpression"),(Array<Member>){&members[499],2});
  RegisterStructMembers(STRING("InstanceDeclaration"),(Array<Member>){&members[501],4});
  RegisterStructMembers(STRING("ConnectionDef"),(Array<Member>){&members[505],6});
  RegisterStructMembers(STRING("ModuleDef"),(Array<Member>){&members[511],5});
  RegisterStructMembers(STRING("TransformDef"),(Array<Member>){&members[516],3});
  RegisterStructMembers(STRING("Transformation"),(Array<Member>){&members[519],3});
  RegisterStructMembers(STRING("HierarchicalName"),(Array<Member>){&members[522],2});
  RegisterStructMembers(STRING("TypeAndInstance"),(Array<Member>){&members[524],2});


  RegisterTypedef(STRING("Array<const char>"),STRING("String"));
  RegisterTypedef(STRING("Range<Expression *>"),STRING("ExpressionRange"));
  RegisterTypedef(STRING("WireTemplate<int>"),STRING("Wire"));
  RegisterTypedef(STRING("WireTemplate<ExpressionRange>"),STRING("WireExpression"));
  RegisterTypedef(STRING("ExternalMemoryTwoPortsTemplate<int>"),STRING("ExternalMemoryTwoPorts"));
  RegisterTypedef(STRING("ExternalMemoryTwoPortsTemplate<ExpressionRange>"),STRING("ExternalMemoryTwoPortsExpression"));
  RegisterTypedef(STRING("ExternalMemoryDualPortTemplate<int>"),STRING("ExternalMemoryDualPort"));
  RegisterTypedef(STRING("ExternalMemoryDualPortTemplate<ExpressionRange>"),STRING("ExternalMemoryDualPortExpression"));
  RegisterTypedef(STRING("ExternalMemoryTemplate<int>"),STRING("ExternalMemory"));
  RegisterTypedef(STRING("ExternalMemoryTemplate<ExpressionRange>"),STRING("ExternalMemoryExpression"));
  RegisterTypedef(STRING("ExternalMemoryInterfaceTemplate<int>"),STRING("ExternalMemoryInterface"));
  RegisterTypedef(STRING("ExternalMemoryInterfaceTemplate<ExpressionRange>"),STRING("ExternalMemoryInterfaceExpression"));
  RegisterTypedef(STRING("Hashmap<FUInstance *, FUInstance *>"),STRING("InstanceMap"));
  RegisterTypedef(STRING("Hashmap<PortEdge, PortEdge>"),STRING("PortEdgeMap"));
  RegisterTypedef(STRING("Hashmap<Edge *, Edge *>"),STRING("EdgeMap"));
  RegisterTypedef(STRING("Hashmap<InstanceNode *, InstanceNode *>"),STRING("InstanceNodeMap"));
  RegisterTypedef(STRING("Hashmap<String, FUInstance *>"),STRING("InstanceTable"));
  RegisterTypedef(STRING("Set<String>"),STRING("InstanceName"));
  RegisterTypedef(STRING("Pair<HierarchicalName, HierarchicalName>"),STRING("SpecNode"));

}
