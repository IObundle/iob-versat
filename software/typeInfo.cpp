#pragma GCC diagnostic ignored "-Winvalid-offsetof"

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
  RegisterOpaqueType(STRING("Time"),Subtype_STRUCT,sizeof(Time),alignof(Time));
  RegisterOpaqueType(STRING("TimeIt"),Subtype_STRUCT,sizeof(TimeIt),alignof(TimeIt));
  RegisterOpaqueType(STRING("Conversion"),Subtype_STRUCT,sizeof(Conversion),alignof(Conversion));
  RegisterOpaqueType(STRING("Arena"),Subtype_STRUCT,sizeof(Arena),alignof(Arena));
  RegisterOpaqueType(STRING("ArenaMark"),Subtype_STRUCT,sizeof(ArenaMark),alignof(ArenaMark));
  RegisterOpaqueType(STRING("ArenaMarker"),Subtype_STRUCT,sizeof(ArenaMarker),alignof(ArenaMarker));
  RegisterOpaqueType(STRING("DynamicArena"),Subtype_STRUCT,sizeof(DynamicArena),alignof(DynamicArena));
  RegisterOpaqueType(STRING("DynamicString"),Subtype_STRUCT,sizeof(DynamicString),alignof(DynamicString));
  RegisterOpaqueType(STRING("BitIterator"),Subtype_STRUCT,sizeof(BitIterator),alignof(BitIterator));
  RegisterOpaqueType(STRING("BitArray"),Subtype_STRUCT,sizeof(BitArray),alignof(BitArray));
  RegisterOpaqueType(STRING("PoolHeader"),Subtype_STRUCT,sizeof(PoolHeader),alignof(PoolHeader));
  RegisterOpaqueType(STRING("PoolInfo"),Subtype_STRUCT,sizeof(PoolInfo),alignof(PoolInfo));
  RegisterOpaqueType(STRING("PageInfo"),Subtype_STRUCT,sizeof(PageInfo),alignof(PageInfo));
  RegisterOpaqueType(STRING("GenericPoolIterator"),Subtype_STRUCT,sizeof(GenericPoolIterator),alignof(GenericPoolIterator));
  RegisterOpaqueType(STRING("EnumMember"),Subtype_STRUCT,sizeof(EnumMember),alignof(EnumMember));
  RegisterOpaqueType(STRING("TemplateArg"),Subtype_STRUCT,sizeof(TemplateArg),alignof(TemplateArg));
  RegisterOpaqueType(STRING("TemplatedMember"),Subtype_STRUCT,sizeof(TemplatedMember),alignof(TemplatedMember));
  RegisterOpaqueType(STRING("NameAndTemplateArguments"),Subtype_STRUCT,sizeof(NameAndTemplateArguments),alignof(NameAndTemplateArguments));
  RegisterOpaqueType(STRING("ParsedType"),Subtype_STRUCT,sizeof(ParsedType),alignof(ParsedType));
  RegisterOpaqueType(STRING("Type"),Subtype_STRUCT,sizeof(Type),alignof(Type));
  RegisterOpaqueType(STRING("Member"),Subtype_STRUCT,sizeof(Member),alignof(Member));
  RegisterOpaqueType(STRING("Value"),Subtype_STRUCT,sizeof(Value),alignof(Value));
  RegisterOpaqueType(STRING("Iterator"),Subtype_STRUCT,sizeof(Iterator),alignof(Iterator));
  RegisterOpaqueType(STRING("HashmapUnpackedIndex"),Subtype_STRUCT,sizeof(HashmapUnpackedIndex),alignof(HashmapUnpackedIndex));
  RegisterOpaqueType(STRING("TypeIterator"),Subtype_STRUCT,sizeof(TypeIterator),alignof(TypeIterator));
  RegisterOpaqueType(STRING("Expression"),Subtype_STRUCT,sizeof(Expression),alignof(Expression));
  RegisterOpaqueType(STRING("Cursor"),Subtype_STRUCT,sizeof(Cursor),alignof(Cursor));
  RegisterOpaqueType(STRING("Token"),Subtype_STRUCT,sizeof(Token),alignof(Token));
  RegisterOpaqueType(STRING("FindFirstResult"),Subtype_STRUCT,sizeof(FindFirstResult),alignof(FindFirstResult));
  RegisterOpaqueType(STRING("Trie"),Subtype_STRUCT,sizeof(Trie),alignof(Trie));
  RegisterOpaqueType(STRING("TokenizerTemplate"),Subtype_STRUCT,sizeof(TokenizerTemplate),alignof(TokenizerTemplate));
  RegisterOpaqueType(STRING("TokenizerMark"),Subtype_STRUCT,sizeof(TokenizerMark),alignof(TokenizerMark));
  RegisterOpaqueType(STRING("Tokenizer"),Subtype_STRUCT,sizeof(Tokenizer),alignof(Tokenizer));
  RegisterOpaqueType(STRING("OperationList"),Subtype_STRUCT,sizeof(OperationList),alignof(OperationList));
  RegisterOpaqueType(STRING("CommandDefinition"),Subtype_STRUCT,sizeof(CommandDefinition),alignof(CommandDefinition));
  RegisterOpaqueType(STRING("Command"),Subtype_STRUCT,sizeof(Command),alignof(Command));
  RegisterOpaqueType(STRING("Block"),Subtype_STRUCT,sizeof(Block),alignof(Block));
  RegisterOpaqueType(STRING("TemplateFunction"),Subtype_STRUCT,sizeof(TemplateFunction),alignof(TemplateFunction));
  RegisterOpaqueType(STRING("TemplateRecord"),Subtype_STRUCT,sizeof(TemplateRecord),alignof(TemplateRecord));
  RegisterOpaqueType(STRING("ValueAndText"),Subtype_STRUCT,sizeof(ValueAndText),alignof(ValueAndText));
  RegisterOpaqueType(STRING("Frame"),Subtype_STRUCT,sizeof(Frame),alignof(Frame));
  RegisterOpaqueType(STRING("IndividualBlock"),Subtype_STRUCT,sizeof(IndividualBlock),alignof(IndividualBlock));
  RegisterOpaqueType(STRING("CompiledTemplate"),Subtype_STRUCT,sizeof(CompiledTemplate),alignof(CompiledTemplate));
  RegisterOpaqueType(STRING("PortDeclaration"),Subtype_STRUCT,sizeof(PortDeclaration),alignof(PortDeclaration));
  RegisterOpaqueType(STRING("ParameterExpression"),Subtype_STRUCT,sizeof(ParameterExpression),alignof(ParameterExpression));
  RegisterOpaqueType(STRING("Module"),Subtype_STRUCT,sizeof(Module),alignof(Module));
  RegisterOpaqueType(STRING("ExternalMemoryID"),Subtype_STRUCT,sizeof(ExternalMemoryID),alignof(ExternalMemoryID));
  RegisterOpaqueType(STRING("ExternalInfoTwoPorts"),Subtype_STRUCT,sizeof(ExternalInfoTwoPorts),alignof(ExternalInfoTwoPorts));
  RegisterOpaqueType(STRING("ExternalInfoDualPort"),Subtype_STRUCT,sizeof(ExternalInfoDualPort),alignof(ExternalInfoDualPort));
  RegisterOpaqueType(STRING("ExternalMemoryInfo"),Subtype_STRUCT,sizeof(ExternalMemoryInfo),alignof(ExternalMemoryInfo));
  RegisterOpaqueType(STRING("ModuleInfo"),Subtype_STRUCT,sizeof(ModuleInfo),alignof(ModuleInfo));
  RegisterOpaqueType(STRING("Options"),Subtype_STRUCT,sizeof(Options),alignof(Options));
  RegisterOpaqueType(STRING("DebugState"),Subtype_STRUCT,sizeof(DebugState),alignof(DebugState));
  RegisterOpaqueType(STRING("PortInstance"),Subtype_STRUCT,sizeof(PortInstance),alignof(PortInstance));
  RegisterOpaqueType(STRING("Edge"),Subtype_STRUCT,sizeof(Edge),alignof(Edge));
  RegisterOpaqueType(STRING("GenericGraphMapping"),Subtype_STRUCT,sizeof(GenericGraphMapping),alignof(GenericGraphMapping));
  RegisterOpaqueType(STRING("EdgeDelayInfo"),Subtype_STRUCT,sizeof(EdgeDelayInfo),alignof(EdgeDelayInfo));
  RegisterOpaqueType(STRING("DelayInfo"),Subtype_STRUCT,sizeof(DelayInfo),alignof(DelayInfo));
  RegisterOpaqueType(STRING("ConnectionNode"),Subtype_STRUCT,sizeof(ConnectionNode),alignof(ConnectionNode));
  RegisterOpaqueType(STRING("FUInstance"),Subtype_STRUCT,sizeof(FUInstance),alignof(FUInstance));
  RegisterOpaqueType(STRING("Accelerator"),Subtype_STRUCT,sizeof(Accelerator),alignof(Accelerator));
  RegisterOpaqueType(STRING("MemoryAddressMask"),Subtype_STRUCT,sizeof(MemoryAddressMask),alignof(MemoryAddressMask));
  RegisterOpaqueType(STRING("VersatComputedValues"),Subtype_STRUCT,sizeof(VersatComputedValues),alignof(VersatComputedValues));
  RegisterOpaqueType(STRING("DAGOrderNodes"),Subtype_STRUCT,sizeof(DAGOrderNodes),alignof(DAGOrderNodes));
  RegisterOpaqueType(STRING("AcceleratorMapping"),Subtype_STRUCT,sizeof(AcceleratorMapping),alignof(AcceleratorMapping));
  RegisterOpaqueType(STRING("EdgeIterator"),Subtype_STRUCT,sizeof(EdgeIterator),alignof(EdgeIterator));
  RegisterOpaqueType(STRING("StaticId"),Subtype_STRUCT,sizeof(StaticId),alignof(StaticId));
  RegisterOpaqueType(STRING("StaticData"),Subtype_STRUCT,sizeof(StaticData),alignof(StaticData));
  RegisterOpaqueType(STRING("StaticInfo"),Subtype_STRUCT,sizeof(StaticInfo),alignof(StaticInfo));
  RegisterOpaqueType(STRING("CalculatedOffsets"),Subtype_STRUCT,sizeof(CalculatedOffsets),alignof(CalculatedOffsets));
  RegisterOpaqueType(STRING("SubMappingInfo"),Subtype_STRUCT,sizeof(SubMappingInfo),alignof(SubMappingInfo));
  RegisterOpaqueType(STRING("InstanceInfo"),Subtype_STRUCT,sizeof(InstanceInfo),alignof(InstanceInfo));
  RegisterOpaqueType(STRING("AcceleratorInfo"),Subtype_STRUCT,sizeof(AcceleratorInfo),alignof(AcceleratorInfo));
  RegisterOpaqueType(STRING("InstanceConfigurationOffsets"),Subtype_STRUCT,sizeof(InstanceConfigurationOffsets),alignof(InstanceConfigurationOffsets));
  RegisterOpaqueType(STRING("TestResult"),Subtype_STRUCT,sizeof(TestResult),alignof(TestResult));
  RegisterOpaqueType(STRING("AccelInfo"),Subtype_STRUCT,sizeof(AccelInfo),alignof(AccelInfo));
  RegisterOpaqueType(STRING("TypeAndNameOnly"),Subtype_STRUCT,sizeof(TypeAndNameOnly),alignof(TypeAndNameOnly));
  RegisterOpaqueType(STRING("Partition"),Subtype_STRUCT,sizeof(Partition),alignof(Partition));
  RegisterOpaqueType(STRING("OrderedConfigurations"),Subtype_STRUCT,sizeof(OrderedConfigurations),alignof(OrderedConfigurations));
  RegisterOpaqueType(STRING("GraphPrintingNodeInfo"),Subtype_STRUCT,sizeof(GraphPrintingNodeInfo),alignof(GraphPrintingNodeInfo));
  RegisterOpaqueType(STRING("GraphPrintingEdgeInfo"),Subtype_STRUCT,sizeof(GraphPrintingEdgeInfo),alignof(GraphPrintingEdgeInfo));
  RegisterOpaqueType(STRING("GraphPrintingContent"),Subtype_STRUCT,sizeof(GraphPrintingContent),alignof(GraphPrintingContent));
  RegisterOpaqueType(STRING("GraphInfo"),Subtype_STRUCT,sizeof(GraphInfo),alignof(GraphInfo));
  RegisterOpaqueType(STRING("CalculateDelayResult"),Subtype_STRUCT,sizeof(CalculateDelayResult),alignof(CalculateDelayResult));
  RegisterOpaqueType(STRING("DelayToAdd"),Subtype_STRUCT,sizeof(DelayToAdd),alignof(DelayToAdd));
  RegisterOpaqueType(STRING("ConfigurationInfo"),Subtype_STRUCT,sizeof(ConfigurationInfo),alignof(ConfigurationInfo));
  RegisterOpaqueType(STRING("FUDeclaration"),Subtype_STRUCT,sizeof(FUDeclaration),alignof(FUDeclaration));
  RegisterOpaqueType(STRING("SingleTypeStructElement"),Subtype_STRUCT,sizeof(SingleTypeStructElement),alignof(SingleTypeStructElement));
  RegisterOpaqueType(STRING("TypeStructInfoElement"),Subtype_STRUCT,sizeof(TypeStructInfoElement),alignof(TypeStructInfoElement));
  RegisterOpaqueType(STRING("TypeStructInfo"),Subtype_STRUCT,sizeof(TypeStructInfo),alignof(TypeStructInfo));
  RegisterOpaqueType(STRING("Difference"),Subtype_STRUCT,sizeof(Difference),alignof(Difference));
  RegisterOpaqueType(STRING("DifferenceArray"),Subtype_STRUCT,sizeof(DifferenceArray),alignof(DifferenceArray));
  RegisterOpaqueType(STRING("MuxInfo"),Subtype_STRUCT,sizeof(MuxInfo),alignof(MuxInfo));
  RegisterOpaqueType(STRING("Task"),Subtype_STRUCT,sizeof(Task),alignof(Task));
  RegisterOpaqueType(STRING("WorkGroup"),Subtype_STRUCT,sizeof(WorkGroup),alignof(WorkGroup));
  RegisterOpaqueType(STRING("SpecificMerge"),Subtype_STRUCT,sizeof(SpecificMerge),alignof(SpecificMerge));
  RegisterOpaqueType(STRING("IndexRecord"),Subtype_STRUCT,sizeof(IndexRecord),alignof(IndexRecord));
  RegisterOpaqueType(STRING("SpecificMergeNodes"),Subtype_STRUCT,sizeof(SpecificMergeNodes),alignof(SpecificMergeNodes));
  RegisterOpaqueType(STRING("SpecificMergeNode"),Subtype_STRUCT,sizeof(SpecificMergeNode),alignof(SpecificMergeNode));
  RegisterOpaqueType(STRING("MergeEdge"),Subtype_STRUCT,sizeof(MergeEdge),alignof(MergeEdge));
  RegisterOpaqueType(STRING("MappingNode"),Subtype_STRUCT,sizeof(MappingNode),alignof(MappingNode));
  RegisterOpaqueType(STRING("ConsolidationGraphOptions"),Subtype_STRUCT,sizeof(ConsolidationGraphOptions),alignof(ConsolidationGraphOptions));
  RegisterOpaqueType(STRING("ConsolidationGraph"),Subtype_STRUCT,sizeof(ConsolidationGraph),alignof(ConsolidationGraph));
  RegisterOpaqueType(STRING("ConsolidationResult"),Subtype_STRUCT,sizeof(ConsolidationResult),alignof(ConsolidationResult));
  RegisterOpaqueType(STRING("CliqueState"),Subtype_STRUCT,sizeof(CliqueState),alignof(CliqueState));
  RegisterOpaqueType(STRING("IsCliqueResult"),Subtype_STRUCT,sizeof(IsCliqueResult),alignof(IsCliqueResult));
  RegisterOpaqueType(STRING("MergeGraphResult"),Subtype_STRUCT,sizeof(MergeGraphResult),alignof(MergeGraphResult));
  RegisterOpaqueType(STRING("MergeGraphResultExisting"),Subtype_STRUCT,sizeof(MergeGraphResultExisting),alignof(MergeGraphResultExisting));
  RegisterOpaqueType(STRING("GraphMapping"),Subtype_STRUCT,sizeof(GraphMapping),alignof(GraphMapping));
  RegisterOpaqueType(STRING("ConnectionExtra"),Subtype_STRUCT,sizeof(ConnectionExtra),alignof(ConnectionExtra));
  RegisterOpaqueType(STRING("Var"),Subtype_STRUCT,sizeof(Var),alignof(Var));
  RegisterOpaqueType(STRING("VarGroup"),Subtype_STRUCT,sizeof(VarGroup),alignof(VarGroup));
  RegisterOpaqueType(STRING("SpecExpression"),Subtype_STRUCT,sizeof(SpecExpression),alignof(SpecExpression));
  RegisterOpaqueType(STRING("VarDeclaration"),Subtype_STRUCT,sizeof(VarDeclaration),alignof(VarDeclaration));
  RegisterOpaqueType(STRING("GroupIterator"),Subtype_STRUCT,sizeof(GroupIterator),alignof(GroupIterator));
  RegisterOpaqueType(STRING("PortExpression"),Subtype_STRUCT,sizeof(PortExpression),alignof(PortExpression));
  RegisterOpaqueType(STRING("InstanceDeclaration"),Subtype_STRUCT,sizeof(InstanceDeclaration),alignof(InstanceDeclaration));
  RegisterOpaqueType(STRING("ConnectionDef"),Subtype_STRUCT,sizeof(ConnectionDef),alignof(ConnectionDef));
  RegisterOpaqueType(STRING("TypeAndInstance"),Subtype_STRUCT,sizeof(TypeAndInstance),alignof(TypeAndInstance));
  RegisterOpaqueType(STRING("ModuleDef"),Subtype_STRUCT,sizeof(ModuleDef),alignof(ModuleDef));
  RegisterOpaqueType(STRING("TransformDef"),Subtype_STRUCT,sizeof(TransformDef),alignof(TransformDef));
  RegisterOpaqueType(STRING("MergeDef"),Subtype_STRUCT,sizeof(MergeDef),alignof(MergeDef));
  RegisterOpaqueType(STRING("TypeDefinition"),Subtype_STRUCT,sizeof(TypeDefinition),alignof(TypeDefinition));
  RegisterOpaqueType(STRING("Transformation"),Subtype_STRUCT,sizeof(Transformation),alignof(Transformation));
  RegisterOpaqueType(STRING("HierarchicalName"),Subtype_STRUCT,sizeof(HierarchicalName),alignof(HierarchicalName));
  RegisterOpaqueType(STRING("SignalHandler"),Subtype_TYPEDEF,sizeof(SignalHandler),alignof(SignalHandler));
  RegisterOpaqueType(STRING("Byte"),Subtype_TYPEDEF,sizeof(Byte),alignof(Byte));
  RegisterOpaqueType(STRING("u8"),Subtype_TYPEDEF,sizeof(u8),alignof(u8));
  RegisterOpaqueType(STRING("i8"),Subtype_TYPEDEF,sizeof(i8),alignof(i8));
  RegisterOpaqueType(STRING("u16"),Subtype_TYPEDEF,sizeof(u16),alignof(u16));
  RegisterOpaqueType(STRING("i16"),Subtype_TYPEDEF,sizeof(i16),alignof(i16));
  RegisterOpaqueType(STRING("u32"),Subtype_TYPEDEF,sizeof(u32),alignof(u32));
  RegisterOpaqueType(STRING("i32"),Subtype_TYPEDEF,sizeof(i32),alignof(i32));
  RegisterOpaqueType(STRING("u64"),Subtype_TYPEDEF,sizeof(u64),alignof(u64));
  RegisterOpaqueType(STRING("i64"),Subtype_TYPEDEF,sizeof(i64),alignof(i64));
  RegisterOpaqueType(STRING("iptr"),Subtype_TYPEDEF,sizeof(iptr),alignof(iptr));
  RegisterOpaqueType(STRING("uptr"),Subtype_TYPEDEF,sizeof(uptr),alignof(uptr));
  RegisterOpaqueType(STRING("uint"),Subtype_TYPEDEF,sizeof(uint),alignof(uint));
  RegisterOpaqueType(STRING("String"),Subtype_TYPEDEF,sizeof(String),alignof(String));
  RegisterOpaqueType(STRING("CharFunction"),Subtype_TYPEDEF,sizeof(CharFunction),alignof(CharFunction));
  RegisterOpaqueType(STRING("PipeFunction"),Subtype_TYPEDEF,sizeof(PipeFunction),alignof(PipeFunction));
  RegisterOpaqueType(STRING("ValueMap"),Subtype_TYPEDEF,sizeof(ValueMap),alignof(ValueMap));
  RegisterOpaqueType(STRING("MacroMap"),Subtype_TYPEDEF,sizeof(MacroMap),alignof(MacroMap));
  RegisterOpaqueType(STRING("ExpressionRange"),Subtype_TYPEDEF,sizeof(ExpressionRange),alignof(ExpressionRange));
  RegisterOpaqueType(STRING("Wire"),Subtype_TYPEDEF,sizeof(Wire),alignof(Wire));
  RegisterOpaqueType(STRING("WireExpression"),Subtype_TYPEDEF,sizeof(WireExpression),alignof(WireExpression));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPorts"),Subtype_TYPEDEF,sizeof(ExternalMemoryTwoPorts),alignof(ExternalMemoryTwoPorts));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPortsExpression"),Subtype_TYPEDEF,sizeof(ExternalMemoryTwoPortsExpression),alignof(ExternalMemoryTwoPortsExpression));
  RegisterOpaqueType(STRING("ExternalMemoryDualPort"),Subtype_TYPEDEF,sizeof(ExternalMemoryDualPort),alignof(ExternalMemoryDualPort));
  RegisterOpaqueType(STRING("ExternalMemoryDualPortExpression"),Subtype_TYPEDEF,sizeof(ExternalMemoryDualPortExpression),alignof(ExternalMemoryDualPortExpression));
  RegisterOpaqueType(STRING("ExternalMemory"),Subtype_TYPEDEF,sizeof(ExternalMemory),alignof(ExternalMemory));
  RegisterOpaqueType(STRING("ExternalMemoryExpression"),Subtype_TYPEDEF,sizeof(ExternalMemoryExpression),alignof(ExternalMemoryExpression));
  RegisterOpaqueType(STRING("ExternalMemoryInterface"),Subtype_TYPEDEF,sizeof(ExternalMemoryInterface),alignof(ExternalMemoryInterface));
  RegisterOpaqueType(STRING("ExternalMemoryInterfaceExpression"),Subtype_TYPEDEF,sizeof(ExternalMemoryInterfaceExpression),alignof(ExternalMemoryInterfaceExpression));
  RegisterOpaqueType(STRING("InstanceMap"),Subtype_TYPEDEF,sizeof(InstanceMap),alignof(InstanceMap));
  RegisterOpaqueType(STRING("EdgeMap"),Subtype_TYPEDEF,sizeof(EdgeMap),alignof(EdgeMap));
  RegisterOpaqueType(STRING("Path"),Subtype_TYPEDEF,sizeof(Path),alignof(Path));
  RegisterOpaqueType(STRING("PathMap"),Subtype_TYPEDEF,sizeof(PathMap),alignof(PathMap));
  RegisterOpaqueType(STRING("SubMap"),Subtype_TYPEDEF,sizeof(SubMap),alignof(SubMap));
  RegisterOpaqueType(STRING("NodeContent"),Subtype_TYPEDEF,sizeof(NodeContent),alignof(NodeContent));
  RegisterOpaqueType(STRING("EdgeContent"),Subtype_TYPEDEF,sizeof(EdgeContent),alignof(EdgeContent));
  RegisterOpaqueType(STRING("EdgeDelay"),Subtype_TYPEDEF,sizeof(EdgeDelay),alignof(EdgeDelay));
  RegisterOpaqueType(STRING("PortDelay"),Subtype_TYPEDEF,sizeof(PortDelay),alignof(PortDelay));
  RegisterOpaqueType(STRING("NodeDelay"),Subtype_TYPEDEF,sizeof(NodeDelay),alignof(NodeDelay));
  RegisterOpaqueType(STRING("GraphDotFormat"),Subtype_TYPEDEF,sizeof(GraphDotFormat),alignof(GraphDotFormat));
  RegisterOpaqueType(STRING("TaskFunction"),Subtype_TYPEDEF,sizeof(TaskFunction),alignof(TaskFunction));
  RegisterOpaqueType(STRING("InstanceTable"),Subtype_TYPEDEF,sizeof(InstanceTable),alignof(InstanceTable));
  RegisterOpaqueType(STRING("InstanceName"),Subtype_TYPEDEF,sizeof(InstanceName),alignof(InstanceName));
  RegisterOpaqueType(STRING("SpecNode"),Subtype_TYPEDEF,sizeof(SpecNode),alignof(SpecNode));
  RegisterOpaqueType(STRING("String"),Subtype_TYPEDEF,sizeof(String),alignof(String));

static Pair<String,int> LogModuleData[] = {{STRING("MEMORY"),(int) LogModule::MEMORY},
    {STRING("TOP_SYS"),(int) LogModule::TOP_SYS},
    {STRING("ACCELERATOR"),(int) LogModule::ACCELERATOR},
    {STRING("PARSER"),(int) LogModule::PARSER},
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

static Pair<String,int> SubtypeData[] = {{STRING("Subtype_UNKNOWN"),(int) Subtype::Subtype_UNKNOWN},
    {STRING("Subtype_BASE"),(int) Subtype::Subtype_BASE},
    {STRING("Subtype_STRUCT"),(int) Subtype::Subtype_STRUCT},
    {STRING("Subtype_POINTER"),(int) Subtype::Subtype_POINTER},
    {STRING("Subtype_ARRAY"),(int) Subtype::Subtype_ARRAY},
    {STRING("Subtype_TEMPLATED_STRUCT_DEF"),(int) Subtype::Subtype_TEMPLATED_STRUCT_DEF},
    {STRING("Subtype_TEMPLATED_INSTANCE"),(int) Subtype::Subtype_TEMPLATED_INSTANCE},
    {STRING("Subtype_ENUM"),(int) Subtype::Subtype_ENUM},
    {STRING("Subtype_TYPEDEF"),(int) Subtype::Subtype_TYPEDEF}};

RegisterEnum(STRING("Subtype"),C_ARRAY_TO_ARRAY(SubtypeData));

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

static Pair<String,int> NodeTypeData[] = {{STRING("NodeType_UNCONNECTED"),(int) NodeType::NodeType_UNCONNECTED},
    {STRING("NodeType_SOURCE"),(int) NodeType::NodeType_SOURCE},
    {STRING("NodeType_COMPUTE"),(int) NodeType::NodeType_COMPUTE},
    {STRING("NodeType_SINK"),(int) NodeType::NodeType_SINK},
    {STRING("NodeType_SOURCE_AND_SINK"),(int) NodeType::NodeType_SOURCE_AND_SINK}};

RegisterEnum(STRING("NodeType"),C_ARRAY_TO_ARRAY(NodeTypeData));

static Pair<String,int> AcceleratorPurposeData[] = {{STRING("AcceleratorPurpose_TEMP"),(int) AcceleratorPurpose::AcceleratorPurpose_TEMP},
    {STRING("AcceleratorPurpose_BASE"),(int) AcceleratorPurpose::AcceleratorPurpose_BASE},
    {STRING("AcceleratorPurpose_FLATTEN"),(int) AcceleratorPurpose::AcceleratorPurpose_FLATTEN},
    {STRING("AcceleratorPurpose_MODULE"),(int) AcceleratorPurpose::AcceleratorPurpose_MODULE},
    {STRING("AcceleratorPurpose_RECON"),(int) AcceleratorPurpose::AcceleratorPurpose_RECON},
    {STRING("AcceleratorPurpose_MERGE"),(int) AcceleratorPurpose::AcceleratorPurpose_MERGE}};

RegisterEnum(STRING("AcceleratorPurpose"),C_ARRAY_TO_ARRAY(AcceleratorPurposeData));

static Pair<String,int> MemTypeData[] = {{STRING("CONFIG"),(int) MemType::CONFIG},
    {STRING("STATE"),(int) MemType::STATE},
    {STRING("DELAY"),(int) MemType::DELAY},
    {STRING("STATIC"),(int) MemType::STATIC}};

RegisterEnum(STRING("MemType"),C_ARRAY_TO_ARRAY(MemTypeData));

static Pair<String,int> ColorData[] = {{STRING("Color_BLACK"),(int) Color::Color_BLACK},
    {STRING("Color_BLUE"),(int) Color::Color_BLUE},
    {STRING("Color_RED"),(int) Color::Color_RED},
    {STRING("Color_GREEN"),(int) Color::Color_GREEN},
    {STRING("Color_YELLOW"),(int) Color::Color_YELLOW}};

RegisterEnum(STRING("Color"),C_ARRAY_TO_ARRAY(ColorData));

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

static Pair<String,int> DefinitionTypeData[] = {{STRING("DefinitionType_MODULE"),(int) DefinitionType::DefinitionType_MODULE},
    {STRING("DefinitionType_MERGE"),(int) DefinitionType::DefinitionType_MERGE},
    {STRING("DefinitionType_ITERATIVE"),(int) DefinitionType::DefinitionType_ITERATIVE}};

RegisterEnum(STRING("DefinitionType"),C_ARRAY_TO_ARRAY(DefinitionTypeData));

  static String templateArgs[] = { STRING("F") /* 0 */,
    STRING("T") /* 1 */,
    STRING("T") /* 2 */,
    STRING("T") /* 3 */,
    STRING("T") /* 4 */,
    STRING("First") /* 5 */,
    STRING("Second") /* 6 */,
    STRING("T") /* 7 */,
    STRING("T") /* 8 */,
    STRING("T") /* 9 */,
    STRING("Key") /* 10 */,
    STRING("Data") /* 11 */,
    STRING("Data") /* 12 */,
    STRING("Key") /* 13 */,
    STRING("Data") /* 14 */,
    STRING("Data") /* 15 */,
    STRING("Data") /* 16 */,
    STRING("Key") /* 17 */,
    STRING("Data") /* 18 */,
    STRING("Key") /* 19 */,
    STRING("Data") /* 20 */,
    STRING("Key") /* 21 */,
    STRING("Data") /* 22 */,
    STRING("Data") /* 23 */,
    STRING("Data") /* 24 */,
    STRING("T") /* 25 */,
    STRING("T") /* 26 */,
    STRING("Value") /* 27 */,
    STRING("Error") /* 28 */,
    STRING("T") /* 29 */,
    STRING("T") /* 30 */,
    STRING("T") /* 31 */,
    STRING("T") /* 32 */,
    STRING("T") /* 33 */,
    STRING("T") /* 34 */,
    STRING("T") /* 35 */,
    STRING("T") /* 36 */,
    STRING("T") /* 37 */,
    STRING("T") /* 38 */
  };

  RegisterTemplate(STRING("_Defer"),(Array<String>){&templateArgs[0],1});
  RegisterTemplate(STRING("Opt"),(Array<String>){&templateArgs[1],1});
  RegisterTemplate(STRING("ArrayIterator"),(Array<String>){&templateArgs[2],1});
  RegisterTemplate(STRING("Array"),(Array<String>){&templateArgs[3],1});
  RegisterTemplate(STRING("Range"),(Array<String>){&templateArgs[4],1});
  RegisterTemplate(STRING("Pair"),(Array<String>){&templateArgs[5],2});
  RegisterTemplate(STRING("DynamicArray"),(Array<String>){&templateArgs[7],1});
  RegisterTemplate(STRING("PushPtr"),(Array<String>){&templateArgs[8],1});
  RegisterTemplate(STRING("Queue"),(Array<String>){&templateArgs[9],1});
  RegisterTemplate(STRING("HashmapIterator"),(Array<String>){&templateArgs[10],2});
  RegisterTemplate(STRING("GetOrAllocateResult"),(Array<String>){&templateArgs[12],1});
  RegisterTemplate(STRING("Hashmap"),(Array<String>){&templateArgs[13],2});
  RegisterTemplate(STRING("Set"),(Array<String>){&templateArgs[15],1});
  RegisterTemplate(STRING("SetIterator"),(Array<String>){&templateArgs[16],1});
  RegisterTemplate(STRING("TrieMapNode"),(Array<String>){&templateArgs[17],2});
  RegisterTemplate(STRING("TrieMapIterator"),(Array<String>){&templateArgs[19],2});
  RegisterTemplate(STRING("TrieMap"),(Array<String>){&templateArgs[21],2});
  RegisterTemplate(STRING("TrieSetIterator"),(Array<String>){&templateArgs[23],1});
  RegisterTemplate(STRING("TrieSet"),(Array<String>){&templateArgs[24],1});
  RegisterTemplate(STRING("PoolIterator"),(Array<String>){&templateArgs[25],1});
  RegisterTemplate(STRING("Pool"),(Array<String>){&templateArgs[26],1});
  RegisterTemplate(STRING("Result"),(Array<String>){&templateArgs[27],2});
  RegisterTemplate(STRING("IndexedStruct"),(Array<String>){&templateArgs[29],1});
  RegisterTemplate(STRING("ListedStruct"),(Array<String>){&templateArgs[30],1});
  RegisterTemplate(STRING("ArenaList"),(Array<String>){&templateArgs[31],1});
  RegisterTemplate(STRING("Stack"),(Array<String>){&templateArgs[32],1});
  RegisterTemplate(STRING("WireTemplate"),(Array<String>){&templateArgs[33],1});
  RegisterTemplate(STRING("ExternalMemoryTwoPortsTemplate"),(Array<String>){&templateArgs[34],1});
  RegisterTemplate(STRING("ExternalMemoryDualPortTemplate"),(Array<String>){&templateArgs[35],1});
  RegisterTemplate(STRING("ExternalMemoryTemplate"),(Array<String>){&templateArgs[36],1});
  RegisterTemplate(STRING("ExternalMemoryInterfaceTemplate"),(Array<String>){&templateArgs[37],1});
  RegisterTemplate(STRING("std::vector"),(Array<String>){&templateArgs[38],1});

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

  static TemplatedMember templateMembers[] = { (TemplatedMember){STRING("F"),STRING("f"),0} /* 0 */,
    (TemplatedMember){STRING("T"),STRING("val"),0} /* 1 */,
    (TemplatedMember){STRING("bool"),STRING("hasVal"),1} /* 2 */,
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
    (TemplatedMember){STRING("Array<T>"),STRING("array"),0} /* 20 */,
    (TemplatedMember){STRING("int"),STRING("used"),1} /* 21 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("pairs"),0} /* 22 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 23 */,
    (TemplatedMember){STRING("Data *"),STRING("data"),0} /* 24 */,
    (TemplatedMember){STRING("bool"),STRING("alreadyExisted"),1} /* 25 */,
    (TemplatedMember){STRING("int"),STRING("nodesAllocated"),0} /* 26 */,
    (TemplatedMember){STRING("int"),STRING("nodesUsed"),1} /* 27 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("buckets"),2} /* 28 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("data"),3} /* 29 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("next"),4} /* 30 */,
    (TemplatedMember){STRING("Hashmap<Data, int> *"),STRING("map"),0} /* 31 */,
    (TemplatedMember){STRING("HashmapIterator<Data, int>"),STRING("innerIter"),0} /* 32 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 33 */,
    (TemplatedMember){STRING("Pair<Key, Data>"),STRING("pair"),1} /* 34 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("next"),2} /* 35 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("ptr"),0} /* 36 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 37 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),1} /* 38 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("head"),2} /* 39 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("tail"),3} /* 40 */,
    (TemplatedMember){STRING("int"),STRING("inserted"),4} /* 41 */,
    (TemplatedMember){STRING("TrieMapIterator<Data, int>"),STRING("innerIter"),0} /* 42 */,
    (TemplatedMember){STRING("TrieMap<Data, int> *"),STRING("map"),0} /* 43 */,
    (TemplatedMember){STRING("Pool<T> *"),STRING("pool"),0} /* 44 */,
    (TemplatedMember){STRING("PageInfo"),STRING("pageInfo"),1} /* 45 */,
    (TemplatedMember){STRING("int"),STRING("fullIndex"),2} /* 46 */,
    (TemplatedMember){STRING("int"),STRING("bit"),3} /* 47 */,
    (TemplatedMember){STRING("int"),STRING("index"),4} /* 48 */,
    (TemplatedMember){STRING("Byte *"),STRING("page"),5} /* 49 */,
    (TemplatedMember){STRING("T *"),STRING("lastVal"),6} /* 50 */,
    (TemplatedMember){STRING("Byte *"),STRING("mem"),0} /* 51 */,
    (TemplatedMember){STRING("PoolInfo"),STRING("info"),1} /* 52 */,
    (TemplatedMember){STRING("Value"),STRING("value"),0} /* 53 */,
    (TemplatedMember){STRING("Error"),STRING("error"),0} /* 54 */,
    (TemplatedMember){STRING("bool"),STRING("isError"),1} /* 55 */,
    (TemplatedMember){STRING("int"),STRING("index"),0} /* 56 */,
    (TemplatedMember){STRING("T"),STRING("elem"),0} /* 57 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("next"),1} /* 58 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),0} /* 59 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("head"),1} /* 60 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("tail"),2} /* 61 */,
    (TemplatedMember){STRING("Array<T>"),STRING("mem"),0} /* 62 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 63 */,
    (TemplatedMember){STRING("String"),STRING("name"),0} /* 64 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),1} /* 65 */,
    (TemplatedMember){STRING("bool"),STRING("isStatic"),2} /* 66 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeIn"),0} /* 67 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeOut"),1} /* 68 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),2} /* 69 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),3} /* 70 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),0} /* 71 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),1} /* 72 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),2} /* 73 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 74 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 75 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 76 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 77 */,
    (TemplatedMember){STRING("ExternalMemoryType"),STRING("type"),1} /* 78 */,
    (TemplatedMember){STRING("int"),STRING("interface"),2} /* 79 */,
    (TemplatedMember){STRING("T*"),STRING("mem"),0} /* 80 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 81 */,
    (TemplatedMember){STRING("int"),STRING("allocated"),2} /* 82 */
  };

  RegisterTemplateMembers(STRING("_Defer"),(Array<TemplatedMember>){&templateMembers[0],1});
  RegisterTemplateMembers(STRING("Opt"),(Array<TemplatedMember>){&templateMembers[1],2});
  RegisterTemplateMembers(STRING("ArrayIterator"),(Array<TemplatedMember>){&templateMembers[3],1});
  RegisterTemplateMembers(STRING("Array"),(Array<TemplatedMember>){&templateMembers[4],2});
  RegisterTemplateMembers(STRING("Range"),(Array<TemplatedMember>){&templateMembers[6],6});
  RegisterTemplateMembers(STRING("Pair"),(Array<TemplatedMember>){&templateMembers[12],4});
  RegisterTemplateMembers(STRING("DynamicArray"),(Array<TemplatedMember>){&templateMembers[16],1});
  RegisterTemplateMembers(STRING("PushPtr"),(Array<TemplatedMember>){&templateMembers[17],3});
  RegisterTemplateMembers(STRING("Queue"),(Array<TemplatedMember>){&templateMembers[20],2});
  RegisterTemplateMembers(STRING("HashmapIterator"),(Array<TemplatedMember>){&templateMembers[22],2});
  RegisterTemplateMembers(STRING("GetOrAllocateResult"),(Array<TemplatedMember>){&templateMembers[24],2});
  RegisterTemplateMembers(STRING("Hashmap"),(Array<TemplatedMember>){&templateMembers[26],5});
  RegisterTemplateMembers(STRING("Set"),(Array<TemplatedMember>){&templateMembers[31],1});
  RegisterTemplateMembers(STRING("SetIterator"),(Array<TemplatedMember>){&templateMembers[32],1});
  RegisterTemplateMembers(STRING("TrieMapNode"),(Array<TemplatedMember>){&templateMembers[33],3});
  RegisterTemplateMembers(STRING("TrieMapIterator"),(Array<TemplatedMember>){&templateMembers[36],1});
  RegisterTemplateMembers(STRING("TrieMap"),(Array<TemplatedMember>){&templateMembers[37],5});
  RegisterTemplateMembers(STRING("TrieSetIterator"),(Array<TemplatedMember>){&templateMembers[42],1});
  RegisterTemplateMembers(STRING("TrieSet"),(Array<TemplatedMember>){&templateMembers[43],1});
  RegisterTemplateMembers(STRING("PoolIterator"),(Array<TemplatedMember>){&templateMembers[44],7});
  RegisterTemplateMembers(STRING("Pool"),(Array<TemplatedMember>){&templateMembers[51],2});
  RegisterTemplateMembers(STRING("Result"),(Array<TemplatedMember>){&templateMembers[53],3});
  RegisterTemplateMembers(STRING("IndexedStruct"),(Array<TemplatedMember>){&templateMembers[56],1});
  RegisterTemplateMembers(STRING("ListedStruct"),(Array<TemplatedMember>){&templateMembers[57],2});
  RegisterTemplateMembers(STRING("ArenaList"),(Array<TemplatedMember>){&templateMembers[59],3});
  RegisterTemplateMembers(STRING("Stack"),(Array<TemplatedMember>){&templateMembers[62],2});
  RegisterTemplateMembers(STRING("WireTemplate"),(Array<TemplatedMember>){&templateMembers[64],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTwoPortsTemplate"),(Array<TemplatedMember>){&templateMembers[67],4});
  RegisterTemplateMembers(STRING("ExternalMemoryDualPortTemplate"),(Array<TemplatedMember>){&templateMembers[71],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTemplate"),(Array<TemplatedMember>){&templateMembers[74],2});
  RegisterTemplateMembers(STRING("ExternalMemoryInterfaceTemplate"),(Array<TemplatedMember>){&templateMembers[76],4});
  RegisterTemplateMembers(STRING("std::vector"),(Array<TemplatedMember>){&templateMembers[80],3});

  static Member members[] = {(Member){GetTypeOrFail(STRING("u64")),STRING("microSeconds"),offsetof(Time,microSeconds)} /* 0 */,
    (Member){GetTypeOrFail(STRING("u64")),STRING("seconds"),offsetof(Time,seconds)} /* 1 */,
    (Member){GetTypeOrFail(STRING("Time")),STRING("start"),offsetof(TimeIt,start)} /* 2 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("id"),offsetof(TimeIt,id)} /* 3 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("endedEarly"),offsetof(TimeIt,endedEarly)} /* 4 */,
    (Member){GetTypeOrFail(STRING("float")),STRING("f"),offsetof(Conversion,f)} /* 5 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("i"),offsetof(Conversion,i)} /* 6 */,
    (Member){GetTypeOrFail(STRING("uint")),STRING("ui"),offsetof(Conversion,ui)} /* 7 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mem"),offsetof(Arena,mem)} /* 8 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("used"),offsetof(Arena,used)} /* 9 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("totalAllocated"),offsetof(Arena,totalAllocated)} /* 10 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("maximum"),offsetof(Arena,maximum)} /* 11 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("locked"),offsetof(Arena,locked)} /* 12 */,
    (Member){GetTypeOrFail(STRING("Arena *")),STRING("arena"),offsetof(ArenaMark,arena)} /* 13 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mark"),offsetof(ArenaMark,mark)} /* 14 */,
    (Member){GetTypeOrFail(STRING("ArenaMark")),STRING("mark"),offsetof(ArenaMarker,mark)} /* 15 */,
    (Member){GetTypeOrFail(STRING("DynamicArena *")),STRING("next"),offsetof(DynamicArena,next)} /* 16 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mem"),offsetof(DynamicArena,mem)} /* 17 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("used"),offsetof(DynamicArena,used)} /* 18 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("pagesAllocated"),offsetof(DynamicArena,pagesAllocated)} /* 19 */,
    (Member){GetTypeOrFail(STRING("Arena *")),STRING("arena"),offsetof(DynamicString,arena)} /* 20 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mark"),offsetof(DynamicString,mark)} /* 21 */,
    (Member){GetTypeOrFail(STRING("BitArray *")),STRING("array"),offsetof(BitIterator,array)} /* 22 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentByte"),offsetof(BitIterator,currentByte)} /* 23 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentBit"),offsetof(BitIterator,currentBit)} /* 24 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("memory"),offsetof(BitArray,memory)} /* 25 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bitSize"),offsetof(BitArray,bitSize)} /* 26 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("byteSize"),offsetof(BitArray,byteSize)} /* 27 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("nextPage"),offsetof(PoolHeader,nextPage)} /* 28 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("allocatedUnits"),offsetof(PoolHeader,allocatedUnits)} /* 29 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsPerFullPage"),offsetof(PoolInfo,unitsPerFullPage)} /* 30 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bitmapSize"),offsetof(PoolInfo,bitmapSize)} /* 31 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsPerPage"),offsetof(PoolInfo,unitsPerPage)} /* 32 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("pageGranuality"),offsetof(PoolInfo,pageGranuality)} /* 33 */,
    (Member){GetTypeOrFail(STRING("PoolHeader *")),STRING("header"),offsetof(PageInfo,header)} /* 34 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("bitmap"),offsetof(PageInfo,bitmap)} /* 35 */,
    (Member){GetTypeOrFail(STRING("PoolInfo")),STRING("poolInfo"),offsetof(GenericPoolIterator,poolInfo)} /* 36 */,
    (Member){GetTypeOrFail(STRING("PageInfo")),STRING("pageInfo"),offsetof(GenericPoolIterator,pageInfo)} /* 37 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("fullIndex"),offsetof(GenericPoolIterator,fullIndex)} /* 38 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bit"),offsetof(GenericPoolIterator,bit)} /* 39 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(GenericPoolIterator,index)} /* 40 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("elemSize"),offsetof(GenericPoolIterator,elemSize)} /* 41 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("page"),offsetof(GenericPoolIterator,page)} /* 42 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(EnumMember,name)} /* 43 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("value"),offsetof(EnumMember,value)} /* 44 */,
    (Member){GetTypeOrFail(STRING("EnumMember *")),STRING("next"),offsetof(EnumMember,next)} /* 45 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TemplateArg,name)} /* 46 */,
    (Member){GetTypeOrFail(STRING("TemplateArg *")),STRING("next"),offsetof(TemplateArg,next)} /* 47 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("typeName"),offsetof(TemplatedMember,typeName)} /* 48 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TemplatedMember,name)} /* 49 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memberOffset"),offsetof(TemplatedMember,memberOffset)} /* 50 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(NameAndTemplateArguments,baseName)} /* 51 */,
    (Member){GetTypeOrFail(STRING("Array<ParsedType>")),STRING("templateMembers"),offsetof(NameAndTemplateArguments,templateMembers)} /* 52 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(ParsedType,baseName)} /* 53 */,
    (Member){GetTypeOrFail(STRING("Array<ParsedType>")),STRING("templateMembers"),offsetof(ParsedType,templateMembers)} /* 54 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("amountOfPointers"),offsetof(ParsedType,amountOfPointers)} /* 55 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("arrayExpressions"),offsetof(ParsedType,arrayExpressions)} /* 56 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Type,name)} /* 57 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("baseType"),offsetof(Type,baseType)} /* 58 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("pointerType"),offsetof(Type,pointerType)} /* 59 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("arrayType"),offsetof(Type,arrayType)} /* 60 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("typedefType"),offsetof(Type,typedefType)} /* 61 */,
    (Member){GetTypeOrFail(STRING("Array<Pair<String, int>>")),STRING("enumMembers"),offsetof(Type,enumMembers)} /* 62 */,
    (Member){GetTypeOrFail(STRING("Array<TemplatedMember>")),STRING("templateMembers"),offsetof(Type,templateMembers)} /* 63 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("templateArgs"),offsetof(Type,templateArgs)} /* 64 */,
    (Member){GetTypeOrFail(STRING("Array<Member>")),STRING("members"),offsetof(Type,members)} /* 65 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("templateBase"),offsetof(Type,templateBase)} /* 66 */,
    (Member){GetTypeOrFail(STRING("Array<Type *>")),STRING("templateArgTypes"),offsetof(Type,templateArgTypes)} /* 67 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("size"),offsetof(Type,size)} /* 68 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("align"),offsetof(Type,align)} /* 69 */,
    (Member){GetTypeOrFail(STRING("Subtype")),STRING("type"),offsetof(Type,type)} /* 70 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("type"),offsetof(Member,type)} /* 71 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Member,name)} /* 72 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("offset"),offsetof(Member,offset)} /* 73 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("structType"),offsetof(Member,structType)} /* 74 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("arrayExpression"),offsetof(Member,arrayExpression)} /* 75 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(Member,index)} /* 76 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("boolean"),offsetof(Value,boolean)} /* 77 */,
    (Member){GetTypeOrFail(STRING("char")),STRING("ch"),offsetof(Value,ch)} /* 78 */,
    (Member){GetTypeOrFail(STRING("i64")),STRING("number"),offsetof(Value,number)} /* 79 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("str"),offsetof(Value,str)} /* 80 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("literal"),offsetof(Value,literal)} /* 81 */,
    (Member){GetTypeOrFail(STRING("TemplateFunction *")),STRING("templateFunction"),offsetof(Value,templateFunction)} /* 82 */,
    (Member){GetTypeOrFail(STRING("void *")),STRING("custom"),offsetof(Value,custom)} /* 83 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("type"),offsetof(Value,type)} /* 84 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isTemp"),offsetof(Value,isTemp)} /* 85 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentNumber"),offsetof(Iterator,currentNumber)} /* 86 */,
    (Member){GetTypeOrFail(STRING("GenericPoolIterator")),STRING("poolIterator"),offsetof(Iterator,poolIterator)} /* 87 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("hashmapType"),offsetof(Iterator,hashmapType)} /* 88 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("iterating"),offsetof(Iterator,iterating)} /* 89 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(HashmapUnpackedIndex,index)} /* 90 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("data"),offsetof(HashmapUnpackedIndex,data)} /* 91 */,
    (Member){GetTypeOrFail(STRING("PoolIterator<Type>")),STRING("iter"),offsetof(TypeIterator,iter)} /* 92 */,
    (Member){GetTypeOrFail(STRING("PoolIterator<Type>")),STRING("end"),offsetof(TypeIterator,end)} /* 93 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("op"),offsetof(Expression,op)} /* 94 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("id"),offsetof(Expression,id)} /* 95 */,
    (Member){GetTypeOrFail(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Expression,expressions)} /* 96 */,
    (Member){GetTypeOrFail(STRING("Command *")),STRING("command"),offsetof(Expression,command)} /* 97 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("val"),offsetof(Expression,val)} /* 98 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("text"),offsetof(Expression,text)} /* 99 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("approximateLine"),offsetof(Expression,approximateLine)} /* 100 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(Expression,type)} /* 101 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("line"),offsetof(Cursor,line)} /* 102 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("column"),offsetof(Cursor,column)} /* 103 */,
    (Member){GetTypeOrFail(STRING("Range<Cursor>")),STRING("loc"),offsetof(Token,loc)} /* 104 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("foundFirst"),offsetof(FindFirstResult,foundFirst)} /* 105 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("peekFindNotIncluded"),offsetof(FindFirstResult,peekFindNotIncluded)} /* 106 */,
    (Member){GetTypeOrFail(STRING("u16[128]")),STRING("array"),offsetof(Trie,array)} /* 107 */,
    (Member){GetTypeOrFail(STRING("Array<Trie>")),STRING("subTries"),offsetof(TokenizerTemplate,subTries)} /* 108 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("ptr"),offsetof(TokenizerMark,ptr)} /* 109 */,
    (Member){GetTypeOrFail(STRING("Cursor")),STRING("pos"),offsetof(TokenizerMark,pos)} /* 110 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("start"),offsetof(Tokenizer,start)} /* 111 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("ptr"),offsetof(Tokenizer,ptr)} /* 112 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("end"),offsetof(Tokenizer,end)} /* 113 */,
    (Member){GetTypeOrFail(STRING("TokenizerTemplate *")),STRING("tmpl"),offsetof(Tokenizer,tmpl)} /* 114 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("line"),offsetof(Tokenizer,line)} /* 115 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("column"),offsetof(Tokenizer,column)} /* 116 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("keepWhitespaces"),offsetof(Tokenizer,keepWhitespaces)} /* 117 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("keepComments"),offsetof(Tokenizer,keepComments)} /* 118 */,
    (Member){GetTypeOrFail(STRING("char **")),STRING("op"),offsetof(OperationList,op)} /* 119 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nOperations"),offsetof(OperationList,nOperations)} /* 120 */,
    (Member){GetTypeOrFail(STRING("OperationList *")),STRING("next"),offsetof(OperationList,next)} /* 121 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(CommandDefinition,name)} /* 122 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberExpressions"),offsetof(CommandDefinition,numberExpressions)} /* 123 */,
    (Member){GetTypeOrFail(STRING("CommandType")),STRING("type"),offsetof(CommandDefinition,type)} /* 124 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isBlockType"),offsetof(CommandDefinition,isBlockType)} /* 125 */,
    (Member){GetTypeOrFail(STRING("CommandDefinition *")),STRING("definition"),offsetof(Command,definition)} /* 126 */,
    (Member){GetTypeOrFail(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Command,expressions)} /* 127 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("fullText"),offsetof(Command,fullText)} /* 128 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("fullText"),offsetof(Block,fullText)} /* 129 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("textBlock"),offsetof(Block,textBlock)} /* 130 */,
    (Member){GetTypeOrFail(STRING("Command *")),STRING("command"),offsetof(Block,command)} /* 131 */,
    (Member){GetTypeOrFail(STRING("Expression *")),STRING("expression"),offsetof(Block,expression)} /* 132 */,
    (Member){GetTypeOrFail(STRING("Array<Block *>")),STRING("innerBlocks"),offsetof(Block,innerBlocks)} /* 133 */,
    (Member){GetTypeOrFail(STRING("BlockType")),STRING("type"),offsetof(Block,type)} /* 134 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("line"),offsetof(Block,line)} /* 135 */,
    (Member){GetTypeOrFail(STRING("Array<Expression *>")),STRING("arguments"),offsetof(TemplateFunction,arguments)} /* 136 */,
    (Member){GetTypeOrFail(STRING("Array<Block *>")),STRING("blocks"),offsetof(TemplateFunction,blocks)} /* 137 */,
    (Member){GetTypeOrFail(STRING("TemplateRecordType")),STRING("type"),offsetof(TemplateRecord,type)} /* 138 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("identifierType"),offsetof(TemplateRecord,identifierType)} /* 139 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("identifierName"),offsetof(TemplateRecord,identifierName)} /* 140 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("structType"),offsetof(TemplateRecord,structType)} /* 141 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("fieldName"),offsetof(TemplateRecord,fieldName)} /* 142 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("val"),offsetof(ValueAndText,val)} /* 143 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("text"),offsetof(ValueAndText,text)} /* 144 */,
    (Member){GetTypeOrFail(STRING("Hashmap<String, Value> *")),STRING("table"),offsetof(Frame,table)} /* 145 */,
    (Member){GetTypeOrFail(STRING("Frame *")),STRING("previousFrame"),offsetof(Frame,previousFrame)} /* 146 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(IndividualBlock,content)} /* 147 */,
    (Member){GetTypeOrFail(STRING("BlockType")),STRING("type"),offsetof(IndividualBlock,type)} /* 148 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("line"),offsetof(IndividualBlock,line)} /* 149 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("totalMemoryUsed"),offsetof(CompiledTemplate,totalMemoryUsed)} /* 150 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(CompiledTemplate,content)} /* 151 */,
    (Member){GetTypeOrFail(STRING("Array<Block *>")),STRING("blocks"),offsetof(CompiledTemplate,blocks)} /* 152 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(CompiledTemplate,name)} /* 153 */,
    (Member){GetTypeOrFail(STRING("Hashmap<String, Value> *")),STRING("attributes"),offsetof(PortDeclaration,attributes)} /* 154 */,
    (Member){GetTypeOrFail(STRING("ExpressionRange")),STRING("range"),offsetof(PortDeclaration,range)} /* 155 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(PortDeclaration,name)} /* 156 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(PortDeclaration,type)} /* 157 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(ParameterExpression,name)} /* 158 */,
    (Member){GetTypeOrFail(STRING("Expression *")),STRING("expr"),offsetof(ParameterExpression,expr)} /* 159 */,
    (Member){GetTypeOrFail(STRING("Array<ParameterExpression>")),STRING("parameters"),offsetof(Module,parameters)} /* 160 */,
    (Member){GetTypeOrFail(STRING("Array<PortDeclaration>")),STRING("ports"),offsetof(Module,ports)} /* 161 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Module,name)} /* 162 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isSource"),offsetof(Module,isSource)} /* 163 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("interface"),offsetof(ExternalMemoryID,interface)} /* 164 */,
    (Member){GetTypeOrFail(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryID,type)} /* 165 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("write"),offsetof(ExternalInfoTwoPorts,write)} /* 166 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("read"),offsetof(ExternalInfoTwoPorts,read)} /* 167 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("enable"),offsetof(ExternalInfoDualPort,enable)} /* 168 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("write"),offsetof(ExternalInfoDualPort,write)} /* 169 */,
    (Member){GetTypeOrFail(STRING("ExternalInfoTwoPorts")),STRING("tp"),offsetof(ExternalMemoryInfo,tp)} /* 170 */,
    (Member){GetTypeOrFail(STRING("ExternalInfoDualPort[2]")),STRING("dp"),offsetof(ExternalMemoryInfo,dp)} /* 171 */,
    (Member){GetTypeOrFail(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryInfo,type)} /* 172 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(ModuleInfo,name)} /* 173 */,
    (Member){GetTypeOrFail(STRING("Array<ParameterExpression>")),STRING("defaultParameters"),offsetof(ModuleInfo,defaultParameters)} /* 174 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("inputDelays"),offsetof(ModuleInfo,inputDelays)} /* 175 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ModuleInfo,outputLatencies)} /* 176 */,
    (Member){GetTypeOrFail(STRING("Array<WireExpression>")),STRING("configs"),offsetof(ModuleInfo,configs)} /* 177 */,
    (Member){GetTypeOrFail(STRING("Array<WireExpression>")),STRING("states"),offsetof(ModuleInfo,states)} /* 178 */,
    (Member){GetTypeOrFail(STRING("Array<ExternalMemoryInterfaceExpression>")),STRING("externalInterfaces"),offsetof(ModuleInfo,externalInterfaces)} /* 179 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nDelays"),offsetof(ModuleInfo,nDelays)} /* 180 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nIO"),offsetof(ModuleInfo,nIO)} /* 181 */,
    (Member){GetTypeOrFail(STRING("ExpressionRange")),STRING("memoryMappedBits"),offsetof(ModuleInfo,memoryMappedBits)} /* 182 */,
    (Member){GetTypeOrFail(STRING("ExpressionRange")),STRING("databusAddrSize"),offsetof(ModuleInfo,databusAddrSize)} /* 183 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("doesIO"),offsetof(ModuleInfo,doesIO)} /* 184 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("memoryMapped"),offsetof(ModuleInfo,memoryMapped)} /* 185 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("hasDone"),offsetof(ModuleInfo,hasDone)} /* 186 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("hasClk"),offsetof(ModuleInfo,hasClk)} /* 187 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("hasReset"),offsetof(ModuleInfo,hasReset)} /* 188 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("hasRun"),offsetof(ModuleInfo,hasRun)} /* 189 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("hasRunning"),offsetof(ModuleInfo,hasRunning)} /* 190 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isSource"),offsetof(ModuleInfo,isSource)} /* 191 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(ModuleInfo,signalLoop)} /* 192 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("verilogFiles"),offsetof(Options,verilogFiles)} /* 193 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("extraSources"),offsetof(Options,extraSources)} /* 194 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("includePaths"),offsetof(Options,includePaths)} /* 195 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("unitPaths"),offsetof(Options,unitPaths)} /* 196 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("hardwareOutputFilepath"),offsetof(Options,hardwareOutputFilepath)} /* 197 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("softwareOutputFilepath"),offsetof(Options,softwareOutputFilepath)} /* 198 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("verilatorRoot"),offsetof(Options,verilatorRoot)} /* 199 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("debugPath"),offsetof(Options,debugPath)} /* 200 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("specificationFilepath"),offsetof(Options,specificationFilepath)} /* 201 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("topName"),offsetof(Options,topName)} /* 202 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("databusAddrSize"),offsetof(Options,databusAddrSize)} /* 203 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("databusDataSize"),offsetof(Options,databusDataSize)} /* 204 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("addInputAndOutputsToTop"),offsetof(Options,addInputAndOutputsToTop)} /* 205 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("debug"),offsetof(Options,debug)} /* 206 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("shadowRegister"),offsetof(Options,shadowRegister)} /* 207 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("architectureHasDatabus"),offsetof(Options,architectureHasDatabus)} /* 208 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("useFixedBuffers"),offsetof(Options,useFixedBuffers)} /* 209 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("generateFSTFormat"),offsetof(Options,generateFSTFormat)} /* 210 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("disableDelayPropagation"),offsetof(Options,disableDelayPropagation)} /* 211 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("useDMA"),offsetof(Options,useDMA)} /* 212 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("exportInternalMemories"),offsetof(Options,exportInternalMemories)} /* 213 */,
    (Member){GetTypeOrFail(STRING("uint")),STRING("dotFormat"),offsetof(DebugState,dotFormat)} /* 214 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputGraphs"),offsetof(DebugState,outputGraphs)} /* 215 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputConsolidationGraphs"),offsetof(DebugState,outputConsolidationGraphs)} /* 216 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputAccelerator"),offsetof(DebugState,outputAccelerator)} /* 217 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputVersat"),offsetof(DebugState,outputVersat)} /* 218 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputVCD"),offsetof(DebugState,outputVCD)} /* 219 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("outputAcceleratorInfo"),offsetof(DebugState,outputAcceleratorInfo)} /* 220 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("useFixedBuffers"),offsetof(DebugState,useFixedBuffers)} /* 221 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("inst"),offsetof(PortInstance,inst)} /* 222 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("port"),offsetof(PortInstance,port)} /* 223 */,
    (Member){GetTypeOrFail(STRING("PortInstance")),STRING("out"),offsetof(Edge,out)} /* 224 */,
    (Member){GetTypeOrFail(STRING("PortInstance")),STRING("in"),offsetof(Edge,in)} /* 225 */,
    (Member){GetTypeOrFail(STRING("PortInstance[2]")),STRING("units"),offsetof(Edge,units)} /* 226 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delay"),offsetof(Edge,delay)} /* 227 */,
    (Member){GetTypeOrFail(STRING("Edge *")),STRING("next"),offsetof(Edge,next)} /* 228 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("nodeMap"),offsetof(GenericGraphMapping,nodeMap)} /* 229 */,
    (Member){GetTypeOrFail(STRING("PathMap *")),STRING("edgeMap"),offsetof(GenericGraphMapping,edgeMap)} /* 230 */,
    (Member){GetTypeOrFail(STRING("int *")),STRING("value"),offsetof(EdgeDelayInfo,value)} /* 231 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isAny"),offsetof(EdgeDelayInfo,isAny)} /* 232 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("value"),offsetof(DelayInfo,value)} /* 233 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isAny"),offsetof(DelayInfo,isAny)} /* 234 */,
    (Member){GetTypeOrFail(STRING("PortInstance")),STRING("instConnectedTo"),offsetof(ConnectionNode,instConnectedTo)} /* 235 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("port"),offsetof(ConnectionNode,port)} /* 236 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("edgeDelay"),offsetof(ConnectionNode,edgeDelay)} /* 237 */,
    (Member){GetTypeOrFail(STRING("EdgeDelayInfo")),STRING("delay"),offsetof(ConnectionNode,delay)} /* 238 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("next"),offsetof(ConnectionNode,next)} /* 239 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(FUInstance,name)} /* 240 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("parameters"),offsetof(FUInstance,parameters)} /* 241 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel"),offsetof(FUInstance,accel)} /* 242 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("declaration"),offsetof(FUInstance,declaration)} /* 243 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(FUInstance,id)} /* 244 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("literal"),offsetof(FUInstance,literal)} /* 245 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bufferAmount"),offsetof(FUInstance,bufferAmount)} /* 246 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("portIndex"),offsetof(FUInstance,portIndex)} /* 247 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedIndex"),offsetof(FUInstance,sharedIndex)} /* 248 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isStatic"),offsetof(FUInstance,isStatic)} /* 249 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("sharedEnable"),offsetof(FUInstance,sharedEnable)} /* 250 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(FUInstance,isMergeMultiplexer)} /* 251 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("mergeMultiplexerId"),offsetof(FUInstance,mergeMultiplexerId)} /* 252 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("next"),offsetof(FUInstance,next)} /* 253 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("allInputs"),offsetof(FUInstance,allInputs)} /* 254 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("allOutputs"),offsetof(FUInstance,allOutputs)} /* 255 */,
    (Member){GetTypeOrFail(STRING("Array<PortInstance>")),STRING("inputs"),offsetof(FUInstance,inputs)} /* 256 */,
    (Member){GetTypeOrFail(STRING("Array<bool>")),STRING("outputs"),offsetof(FUInstance,outputs)} /* 257 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("multipleSamePortInputs"),offsetof(FUInstance,multipleSamePortInputs)} /* 258 */,
    (Member){GetTypeOrFail(STRING("NodeType")),STRING("type"),offsetof(FUInstance,type)} /* 259 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("allocated"),offsetof(Accelerator,allocated)} /* 260 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("lastAllocated"),offsetof(Accelerator,lastAllocated)} /* 261 */,
    (Member){GetTypeOrFail(STRING("DynamicArena *")),STRING("accelMemory"),offsetof(Accelerator,accelMemory)} /* 262 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Accelerator,name)} /* 263 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(Accelerator,id)} /* 264 */,
    (Member){GetTypeOrFail(STRING("AcceleratorPurpose")),STRING("purpose"),offsetof(Accelerator,purpose)} /* 265 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMaskSize"),offsetof(MemoryAddressMask,memoryMaskSize)} /* 266 */,
    (Member){GetTypeOrFail(STRING("char[33]")),STRING("memoryMaskBuffer"),offsetof(MemoryAddressMask,memoryMaskBuffer)} /* 267 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("memoryMask"),offsetof(MemoryAddressMask,memoryMask)} /* 268 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nConfigs"),offsetof(VersatComputedValues,nConfigs)} /* 269 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configBits"),offsetof(VersatComputedValues,configBits)} /* 270 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("versatConfigs"),offsetof(VersatComputedValues,versatConfigs)} /* 271 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("versatStates"),offsetof(VersatComputedValues,versatStates)} /* 272 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nStatics"),offsetof(VersatComputedValues,nStatics)} /* 273 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBits"),offsetof(VersatComputedValues,staticBits)} /* 274 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBitsStart"),offsetof(VersatComputedValues,staticBitsStart)} /* 275 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nDelays"),offsetof(VersatComputedValues,nDelays)} /* 276 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayBits"),offsetof(VersatComputedValues,delayBits)} /* 277 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayBitsStart"),offsetof(VersatComputedValues,delayBitsStart)} /* 278 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nConfigurations"),offsetof(VersatComputedValues,nConfigurations)} /* 279 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configurationBits"),offsetof(VersatComputedValues,configurationBits)} /* 280 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configurationAddressBits"),offsetof(VersatComputedValues,configurationAddressBits)} /* 281 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nStates"),offsetof(VersatComputedValues,nStates)} /* 282 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateBits"),offsetof(VersatComputedValues,stateBits)} /* 283 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateAddressBits"),offsetof(VersatComputedValues,stateAddressBits)} /* 284 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsMapped"),offsetof(VersatComputedValues,unitsMapped)} /* 285 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappedBytes"),offsetof(VersatComputedValues,memoryMappedBytes)} /* 286 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nUnitsIO"),offsetof(VersatComputedValues,nUnitsIO)} /* 287 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberConnections"),offsetof(VersatComputedValues,numberConnections)} /* 288 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(VersatComputedValues,externalMemoryInterfaces)} /* 289 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateConfigurationAddressBits"),offsetof(VersatComputedValues,stateConfigurationAddressBits)} /* 290 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryAddressBits"),offsetof(VersatComputedValues,memoryAddressBits)} /* 291 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappingAddressBits"),offsetof(VersatComputedValues,memoryMappingAddressBits)} /* 292 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryConfigDecisionBit"),offsetof(VersatComputedValues,memoryConfigDecisionBit)} /* 293 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(VersatComputedValues,signalLoop)} /* 294 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("sinks"),offsetof(DAGOrderNodes,sinks)} /* 295 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("sources"),offsetof(DAGOrderNodes,sources)} /* 296 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("computeUnits"),offsetof(DAGOrderNodes,computeUnits)} /* 297 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("instances"),offsetof(DAGOrderNodes,instances)} /* 298 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("order"),offsetof(DAGOrderNodes,order)} /* 299 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("size"),offsetof(DAGOrderNodes,size)} /* 300 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("maxOrder"),offsetof(DAGOrderNodes,maxOrder)} /* 301 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("firstId"),offsetof(AcceleratorMapping,firstId)} /* 302 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("secondId"),offsetof(AcceleratorMapping,secondId)} /* 303 */,
    (Member){GetTypeOrFail(STRING("TrieMap<PortInstance, PortInstance> *")),STRING("inputMap"),offsetof(AcceleratorMapping,inputMap)} /* 304 */,
    (Member){GetTypeOrFail(STRING("TrieMap<PortInstance, PortInstance> *")),STRING("outputMap"),offsetof(AcceleratorMapping,outputMap)} /* 305 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("currentNode"),offsetof(EdgeIterator,currentNode)} /* 306 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("currentPort"),offsetof(EdgeIterator,currentPort)} /* 307 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(StaticId,parent)} /* 308 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(StaticId,name)} /* 309 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(StaticData,configs)} /* 310 */,
    (Member){GetTypeOrFail(STRING("StaticId")),STRING("id"),offsetof(StaticInfo,id)} /* 311 */,
    (Member){GetTypeOrFail(STRING("StaticData")),STRING("data"),offsetof(StaticInfo,data)} /* 312 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("offsets"),offsetof(CalculatedOffsets,offsets)} /* 313 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(CalculatedOffsets,max)} /* 314 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("subDeclaration"),offsetof(SubMappingInfo,subDeclaration)} /* 315 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("higherName"),offsetof(SubMappingInfo,higherName)} /* 316 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isInput"),offsetof(SubMappingInfo,isInput)} /* 317 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("subPort"),offsetof(SubMappingInfo,subPort)} /* 318 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("level"),offsetof(InstanceInfo,level)} /* 319 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("decl"),offsetof(InstanceInfo,decl)} /* 320 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(InstanceInfo,name)} /* 321 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(InstanceInfo,baseName)} /* 322 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("configPos"),offsetof(InstanceInfo,configPos)} /* 323 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("isConfigStatic"),offsetof(InstanceInfo,isConfigStatic)} /* 324 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configSize"),offsetof(InstanceInfo,configSize)} /* 325 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("statePos"),offsetof(InstanceInfo,statePos)} /* 326 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateSize"),offsetof(InstanceInfo,stateSize)} /* 327 */,
    (Member){GetTypeOrFail(STRING("Opt<iptr>")),STRING("memMapped"),offsetof(InstanceInfo,memMapped)} /* 328 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memMappedSize"),offsetof(InstanceInfo,memMappedSize)} /* 329 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memMappedBitSize"),offsetof(InstanceInfo,memMappedBitSize)} /* 330 */,
    (Member){GetTypeOrFail(STRING("Opt<String>")),STRING("memMappedMask"),offsetof(InstanceInfo,memMappedMask)} /* 331 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("delayPos"),offsetof(InstanceInfo,delayPos)} /* 332 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("delay"),offsetof(InstanceInfo,delay)} /* 333 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("baseDelay"),offsetof(InstanceInfo,baseDelay)} /* 334 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delaySize"),offsetof(InstanceInfo,delaySize)} /* 335 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isComposite"),offsetof(InstanceInfo,isComposite)} /* 336 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isStatic"),offsetof(InstanceInfo,isStatic)} /* 337 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isShared"),offsetof(InstanceInfo,isShared)} /* 338 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedIndex"),offsetof(InstanceInfo,sharedIndex)} /* 339 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceInfo,parent)} /* 340 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("fullName"),offsetof(InstanceInfo,fullName)} /* 341 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(InstanceInfo,isMergeMultiplexer)} /* 342 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("mergeMultiplexerId"),offsetof(InstanceInfo,mergeMultiplexerId)} /* 343 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("belongs"),offsetof(InstanceInfo,belongs)} /* 344 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("special"),offsetof(InstanceInfo,special)} /* 345 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(InstanceInfo,order)} /* 346 */,
    (Member){GetTypeOrFail(STRING("NodeType")),STRING("connectionType"),offsetof(InstanceInfo,connectionType)} /* 347 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(InstanceInfo,id)} /* 348 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(AcceleratorInfo,info)} /* 349 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memSize"),offsetof(AcceleratorInfo,memSize)} /* 350 */,
    (Member){GetTypeOrFail(STRING("Opt<String>")),STRING("name"),offsetof(AcceleratorInfo,name)} /* 351 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("mergeMux"),offsetof(AcceleratorInfo,mergeMux)} /* 352 */,
    (Member){GetTypeOrFail(STRING("Hashmap<StaticId, int> *")),STRING("staticInfo"),offsetof(InstanceConfigurationOffsets,staticInfo)} /* 353 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceConfigurationOffsets,parent)} /* 354 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("topName"),offsetof(InstanceConfigurationOffsets,topName)} /* 355 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(InstanceConfigurationOffsets,baseName)} /* 356 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configOffset"),offsetof(InstanceConfigurationOffsets,configOffset)} /* 357 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateOffset"),offsetof(InstanceConfigurationOffsets,stateOffset)} /* 358 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayOffset"),offsetof(InstanceConfigurationOffsets,delayOffset)} /* 359 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delay"),offsetof(InstanceConfigurationOffsets,delay)} /* 360 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memOffset"),offsetof(InstanceConfigurationOffsets,memOffset)} /* 361 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("level"),offsetof(InstanceConfigurationOffsets,level)} /* 362 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(InstanceConfigurationOffsets,order)} /* 363 */,
    (Member){GetTypeOrFail(STRING("int *")),STRING("staticConfig"),offsetof(InstanceConfigurationOffsets,staticConfig)} /* 364 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("belongs"),offsetof(InstanceConfigurationOffsets,belongs)} /* 365 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(TestResult,info)} /* 366 */,
    (Member){GetTypeOrFail(STRING("InstanceConfigurationOffsets")),STRING("subOffsets"),offsetof(TestResult,subOffsets)} /* 367 */,
    (Member){GetTypeOrFail(STRING("Array<PortInstance>")),STRING("multiplexersPorts"),offsetof(TestResult,multiplexersPorts)} /* 368 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TestResult,name)} /* 369 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("muxConfigs"),offsetof(TestResult,muxConfigs)} /* 370 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("inputDelay"),offsetof(TestResult,inputDelay)} /* 371 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("outputLatencies"),offsetof(TestResult,outputLatencies)} /* 372 */,
    (Member){GetTypeOrFail(STRING("Array<Array<InstanceInfo>>")),STRING("infos"),offsetof(AccelInfo,infos)} /* 373 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("baseInfo"),offsetof(AccelInfo,baseInfo)} /* 374 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("names"),offsetof(AccelInfo,names)} /* 375 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("inputDelays"),offsetof(AccelInfo,inputDelays)} /* 376 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("outputDelays"),offsetof(AccelInfo,outputDelays)} /* 377 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("muxConfigs"),offsetof(AccelInfo,muxConfigs)} /* 378 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memMappedBitsize"),offsetof(AccelInfo,memMappedBitsize)} /* 379 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("howManyMergedUnits"),offsetof(AccelInfo,howManyMergedUnits)} /* 380 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("inputs"),offsetof(AccelInfo,inputs)} /* 381 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("outputs"),offsetof(AccelInfo,outputs)} /* 382 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configs"),offsetof(AccelInfo,configs)} /* 383 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("states"),offsetof(AccelInfo,states)} /* 384 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delays"),offsetof(AccelInfo,delays)} /* 385 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("ios"),offsetof(AccelInfo,ios)} /* 386 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("statics"),offsetof(AccelInfo,statics)} /* 387 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBits"),offsetof(AccelInfo,staticBits)} /* 388 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedUnits"),offsetof(AccelInfo,sharedUnits)} /* 389 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(AccelInfo,externalMemoryInterfaces)} /* 390 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryByteSize"),offsetof(AccelInfo,externalMemoryByteSize)} /* 391 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberUnits"),offsetof(AccelInfo,numberUnits)} /* 392 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberConnections"),offsetof(AccelInfo,numberConnections)} /* 393 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappedBits"),offsetof(AccelInfo,memoryMappedBits)} /* 394 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMemoryMapped"),offsetof(AccelInfo,isMemoryMapped)} /* 395 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(AccelInfo,signalLoop)} /* 396 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("type"),offsetof(TypeAndNameOnly,type)} /* 397 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TypeAndNameOnly,name)} /* 398 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("value"),offsetof(Partition,value)} /* 399 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(Partition,max)} /* 400 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(OrderedConfigurations,configs)} /* 401 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("statics"),offsetof(OrderedConfigurations,statics)} /* 402 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("delays"),offsetof(OrderedConfigurations,delays)} /* 403 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(GraphPrintingNodeInfo,name)} /* 404 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphPrintingNodeInfo,content)} /* 405 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphPrintingNodeInfo,color)} /* 406 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphPrintingEdgeInfo,content)} /* 407 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphPrintingEdgeInfo,color)} /* 408 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("first"),offsetof(GraphPrintingEdgeInfo,first)} /* 409 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("second"),offsetof(GraphPrintingEdgeInfo,second)} /* 410 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("graphLabel"),offsetof(GraphPrintingContent,graphLabel)} /* 411 */,
    (Member){GetTypeOrFail(STRING("Array<GraphPrintingNodeInfo>")),STRING("nodes"),offsetof(GraphPrintingContent,nodes)} /* 412 */,
    (Member){GetTypeOrFail(STRING("Array<GraphPrintingEdgeInfo>")),STRING("edges"),offsetof(GraphPrintingContent,edges)} /* 413 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphInfo,content)} /* 414 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphInfo,color)} /* 415 */,
    (Member){GetTypeOrFail(STRING("EdgeDelay *")),STRING("edgesDelay"),offsetof(CalculateDelayResult,edgesDelay)} /* 416 */,
    (Member){GetTypeOrFail(STRING("PortDelay *")),STRING("portDelay"),offsetof(CalculateDelayResult,portDelay)} /* 417 */,
    (Member){GetTypeOrFail(STRING("NodeDelay *")),STRING("nodeDelay"),offsetof(CalculateDelayResult,nodeDelay)} /* 418 */,
    (Member){GetTypeOrFail(STRING("Edge")),STRING("edge"),offsetof(DelayToAdd,edge)} /* 419 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("bufferName"),offsetof(DelayToAdd,bufferName)} /* 420 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("bufferParameters"),offsetof(DelayToAdd,bufferParameters)} /* 421 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bufferAmount"),offsetof(DelayToAdd,bufferAmount)} /* 422 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(ConfigurationInfo,name)} /* 423 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("baseName"),offsetof(ConfigurationInfo,baseName)} /* 424 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("baseType"),offsetof(ConfigurationInfo,baseType)} /* 425 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(ConfigurationInfo,configs)} /* 426 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("states"),offsetof(ConfigurationInfo,states)} /* 427 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("inputDelays"),offsetof(ConfigurationInfo,inputDelays)} /* 428 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ConfigurationInfo,outputLatencies)} /* 429 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("configOffsets"),offsetof(ConfigurationInfo,configOffsets)} /* 430 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("stateOffsets"),offsetof(ConfigurationInfo,stateOffsets)} /* 431 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("delayOffsets"),offsetof(ConfigurationInfo,delayOffsets)} /* 432 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("calculatedDelays"),offsetof(ConfigurationInfo,calculatedDelays)} /* 433 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("order"),offsetof(ConfigurationInfo,order)} /* 434 */,
    (Member){GetTypeOrFail(STRING("AcceleratorMapping *")),STRING("mapping"),offsetof(ConfigurationInfo,mapping)} /* 435 */,
    (Member){GetTypeOrFail(STRING("Set<PortInstance> *")),STRING("mergeMultiplexers"),offsetof(ConfigurationInfo,mergeMultiplexers)} /* 436 */,
    (Member){GetTypeOrFail(STRING("Array<bool>")),STRING("unitBelongs"),offsetof(ConfigurationInfo,unitBelongs)} /* 437 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("mergeMultiplexerConfigs"),offsetof(ConfigurationInfo,mergeMultiplexerConfigs)} /* 438 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(FUDeclaration,name)} /* 439 */,
    (Member){GetTypeOrFail(STRING("ConfigurationInfo")),STRING("baseConfig"),offsetof(FUDeclaration,baseConfig)} /* 440 */,
    (Member){GetTypeOrFail(STRING("Array<ConfigurationInfo>")),STRING("configInfo"),offsetof(FUDeclaration,configInfo)} /* 441 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memoryMapBits"),offsetof(FUDeclaration,memoryMapBits)} /* 442 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nIOs"),offsetof(FUDeclaration,nIOs)} /* 443 */,
    (Member){GetTypeOrFail(STRING("Array<ExternalMemoryInterface>")),STRING("externalMemory"),offsetof(FUDeclaration,externalMemory)} /* 444 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("baseCircuit"),offsetof(FUDeclaration,baseCircuit)} /* 445 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("fixedDelayCircuit"),offsetof(FUDeclaration,fixedDelayCircuit)} /* 446 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("flattenedBaseCircuit"),offsetof(FUDeclaration,flattenedBaseCircuit)} /* 447 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("operation"),offsetof(FUDeclaration,operation)} /* 448 */,
    (Member){GetTypeOrFail(STRING("SubMap *")),STRING("flattenMapping"),offsetof(FUDeclaration,flattenMapping)} /* 449 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("lat"),offsetof(FUDeclaration,lat)} /* 450 */,
    (Member){GetTypeOrFail(STRING("Hashmap<StaticId, StaticData> *")),STRING("staticUnits"),offsetof(FUDeclaration,staticUnits)} /* 451 */,
    (Member){GetTypeOrFail(STRING("FUDeclarationType")),STRING("type"),offsetof(FUDeclaration,type)} /* 452 */,
    (Member){GetTypeOrFail(STRING("DelayType")),STRING("delayType"),offsetof(FUDeclaration,delayType)} /* 453 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isOperation"),offsetof(FUDeclaration,isOperation)} /* 454 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("implementsDone"),offsetof(FUDeclaration,implementsDone)} /* 455 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(FUDeclaration,signalLoop)} /* 456 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("type"),offsetof(SingleTypeStructElement,type)} /* 457 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(SingleTypeStructElement,name)} /* 458 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("arraySize"),offsetof(SingleTypeStructElement,arraySize)} /* 459 */,
    (Member){GetTypeOrFail(STRING("Array<SingleTypeStructElement>")),STRING("typeAndNames"),offsetof(TypeStructInfoElement,typeAndNames)} /* 460 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TypeStructInfo,name)} /* 461 */,
    (Member){GetTypeOrFail(STRING("Array<TypeStructInfoElement>")),STRING("entries"),offsetof(TypeStructInfo,entries)} /* 462 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(Difference,index)} /* 463 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("newValue"),offsetof(Difference,newValue)} /* 464 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("oldIndex"),offsetof(DifferenceArray,oldIndex)} /* 465 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("newIndex"),offsetof(DifferenceArray,newIndex)} /* 466 */,
    (Member){GetTypeOrFail(STRING("Array<Difference>")),STRING("differences"),offsetof(DifferenceArray,differences)} /* 467 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configIndex"),offsetof(MuxInfo,configIndex)} /* 468 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("val"),offsetof(MuxInfo,val)} /* 469 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(MuxInfo,name)} /* 470 */,
    (Member){GetTypeOrFail(STRING("InstanceInfo *")),STRING("info"),offsetof(MuxInfo,info)} /* 471 */,
    (Member){GetTypeOrFail(STRING("TaskFunction")),STRING("function"),offsetof(Task,function)} /* 472 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(Task,order)} /* 473 */,
    (Member){GetTypeOrFail(STRING("void *")),STRING("args"),offsetof(Task,args)} /* 474 */,
    (Member){GetTypeOrFail(STRING("TaskFunction")),STRING("function"),offsetof(WorkGroup,function)} /* 475 */,
    (Member){GetTypeOrFail(STRING("Array<Task>")),STRING("tasks"),offsetof(WorkGroup,tasks)} /* 476 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("instA"),offsetof(SpecificMerge,instA)} /* 477 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("instB"),offsetof(SpecificMerge,instB)} /* 478 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(IndexRecord,index)} /* 479 */,
    (Member){GetTypeOrFail(STRING("IndexRecord *")),STRING("next"),offsetof(IndexRecord,next)} /* 480 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("instA"),offsetof(SpecificMergeNodes,instA)} /* 481 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("instB"),offsetof(SpecificMergeNodes,instB)} /* 482 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("firstIndex"),offsetof(SpecificMergeNode,firstIndex)} /* 483 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("firstName"),offsetof(SpecificMergeNode,firstName)} /* 484 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("secondIndex"),offsetof(SpecificMergeNode,secondIndex)} /* 485 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("secondName"),offsetof(SpecificMergeNode,secondName)} /* 486 */,
    (Member){GetTypeOrFail(STRING("FUInstance *[2]")),STRING("instances"),offsetof(MergeEdge,instances)} /* 487 */,
    (Member){GetTypeOrFail(STRING("MergeEdge")),STRING("nodes"),offsetof(MappingNode,nodes)} /* 488 */,
    (Member){GetTypeOrFail(STRING("Edge[2]")),STRING("edges"),offsetof(MappingNode,edges)} /* 489 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(MappingNode,type)} /* 490 */,
    (Member){GetTypeOrFail(STRING("Array<SpecificMergeNodes>")),STRING("specifics"),offsetof(ConsolidationGraphOptions,specifics)} /* 491 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(ConsolidationGraphOptions,order)} /* 492 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("difference"),offsetof(ConsolidationGraphOptions,difference)} /* 493 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("mapNodes"),offsetof(ConsolidationGraphOptions,mapNodes)} /* 494 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(ConsolidationGraphOptions,type)} /* 495 */,
    (Member){GetTypeOrFail(STRING("Array<MappingNode>")),STRING("nodes"),offsetof(ConsolidationGraph,nodes)} /* 496 */,
    (Member){GetTypeOrFail(STRING("Array<BitArray>")),STRING("edges"),offsetof(ConsolidationGraph,edges)} /* 497 */,
    (Member){GetTypeOrFail(STRING("BitArray")),STRING("validNodes"),offsetof(ConsolidationGraph,validNodes)} /* 498 */,
    (Member){GetTypeOrFail(STRING("ConsolidationGraph")),STRING("graph"),offsetof(ConsolidationResult,graph)} /* 499 */,
    (Member){GetTypeOrFail(STRING("Pool<MappingNode>")),STRING("specificsAdded"),offsetof(ConsolidationResult,specificsAdded)} /* 500 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("upperBound"),offsetof(ConsolidationResult,upperBound)} /* 501 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(CliqueState,max)} /* 502 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("upperBound"),offsetof(CliqueState,upperBound)} /* 503 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("startI"),offsetof(CliqueState,startI)} /* 504 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("iterations"),offsetof(CliqueState,iterations)} /* 505 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("table"),offsetof(CliqueState,table)} /* 506 */,
    (Member){GetTypeOrFail(STRING("ConsolidationGraph")),STRING("clique"),offsetof(CliqueState,clique)} /* 507 */,
    (Member){GetTypeOrFail(STRING("Time")),STRING("start"),offsetof(CliqueState,start)} /* 508 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("found"),offsetof(CliqueState,found)} /* 509 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("result"),offsetof(IsCliqueResult,result)} /* 510 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("failedIndex"),offsetof(IsCliqueResult,failedIndex)} /* 511 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel1"),offsetof(MergeGraphResult,accel1)} /* 512 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResult,accel2)} /* 513 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("map1"),offsetof(MergeGraphResult,map1)} /* 514 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("map2"),offsetof(MergeGraphResult,map2)} /* 515 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("newGraph"),offsetof(MergeGraphResult,newGraph)} /* 516 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("result"),offsetof(MergeGraphResultExisting,result)} /* 517 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResultExisting,accel2)} /* 518 */,
    (Member){GetTypeOrFail(STRING("AcceleratorMapping *")),STRING("map2"),offsetof(MergeGraphResultExisting,map2)} /* 519 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("instanceMap"),offsetof(GraphMapping,instanceMap)} /* 520 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("reverseInstanceMap"),offsetof(GraphMapping,reverseInstanceMap)} /* 521 */,
    (Member){GetTypeOrFail(STRING("EdgeMap *")),STRING("edgeMap"),offsetof(GraphMapping,edgeMap)} /* 522 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("port"),offsetof(ConnectionExtra,port)} /* 523 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("delay"),offsetof(ConnectionExtra,delay)} /* 524 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(Var,name)} /* 525 */,
    (Member){GetTypeOrFail(STRING("ConnectionExtra")),STRING("extra"),offsetof(Var,extra)} /* 526 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("index"),offsetof(Var,index)} /* 527 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isArrayAccess"),offsetof(Var,isArrayAccess)} /* 528 */,
    (Member){GetTypeOrFail(STRING("Array<Var>")),STRING("vars"),offsetof(VarGroup,vars)} /* 529 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("fullText"),offsetof(VarGroup,fullText)} /* 530 */,
    (Member){GetTypeOrFail(STRING("Array<SpecExpression *>")),STRING("expressions"),offsetof(SpecExpression,expressions)} /* 531 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("op"),offsetof(SpecExpression,op)} /* 532 */,
    (Member){GetTypeOrFail(STRING("Var")),STRING("var"),offsetof(SpecExpression,var)} /* 533 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("val"),offsetof(SpecExpression,val)} /* 534 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("text"),offsetof(SpecExpression,text)} /* 535 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(SpecExpression,type)} /* 536 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(VarDeclaration,name)} /* 537 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("arraySize"),offsetof(VarDeclaration,arraySize)} /* 538 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isArray"),offsetof(VarDeclaration,isArray)} /* 539 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("group"),offsetof(GroupIterator,group)} /* 540 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("groupIndex"),offsetof(GroupIterator,groupIndex)} /* 541 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("varIndex"),offsetof(GroupIterator,varIndex)} /* 542 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("inst"),offsetof(PortExpression,inst)} /* 543 */,
    (Member){GetTypeOrFail(STRING("ConnectionExtra")),STRING("extra"),offsetof(PortExpression,extra)} /* 544 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("modifier"),offsetof(InstanceDeclaration,modifier)} /* 545 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("typeName"),offsetof(InstanceDeclaration,typeName)} /* 546 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("declarations"),offsetof(InstanceDeclaration,declarations)} /* 547 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("parameters"),offsetof(InstanceDeclaration,parameters)} /* 548 */,
    (Member){GetTypeOrFail(STRING("Range<Cursor>")),STRING("loc"),offsetof(ConnectionDef,loc)} /* 549 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("output"),offsetof(ConnectionDef,output)} /* 550 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(ConnectionDef,type)} /* 551 */,
    (Member){GetTypeOrFail(STRING("Array<Token>")),STRING("transforms"),offsetof(ConnectionDef,transforms)} /* 552 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("input"),offsetof(ConnectionDef,input)} /* 553 */,
    (Member){GetTypeOrFail(STRING("SpecExpression *")),STRING("expression"),offsetof(ConnectionDef,expression)} /* 554 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("typeName"),offsetof(TypeAndInstance,typeName)} /* 555 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("instanceName"),offsetof(TypeAndInstance,instanceName)} /* 556 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(ModuleDef,name)} /* 557 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("numberOutputs"),offsetof(ModuleDef,numberOutputs)} /* 558 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(ModuleDef,inputs)} /* 559 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceDeclaration>")),STRING("declarations"),offsetof(ModuleDef,declarations)} /* 560 */,
    (Member){GetTypeOrFail(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(ModuleDef,connections)} /* 561 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(TransformDef,name)} /* 562 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(TransformDef,inputs)} /* 563 */,
    (Member){GetTypeOrFail(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(TransformDef,connections)} /* 564 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(MergeDef,name)} /* 565 */,
    (Member){GetTypeOrFail(STRING("Array<TypeAndInstance>")),STRING("declarations"),offsetof(MergeDef,declarations)} /* 566 */,
    (Member){GetTypeOrFail(STRING("Array<SpecificMergeNode>")),STRING("specifics"),offsetof(MergeDef,specifics)} /* 567 */,
    (Member){GetTypeOrFail(STRING("DefinitionType")),STRING("type"),offsetof(TypeDefinition,type)} /* 568 */,
    (Member){GetTypeOrFail(STRING("ModuleDef")),STRING("module"),offsetof(TypeDefinition,module)} /* 569 */,
    (Member){GetTypeOrFail(STRING("MergeDef")),STRING("merge"),offsetof(TypeDefinition,merge)} /* 570 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("inputs"),offsetof(Transformation,inputs)} /* 571 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("outputs"),offsetof(Transformation,outputs)} /* 572 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("map"),offsetof(Transformation,map)} /* 573 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("instanceName"),offsetof(HierarchicalName,instanceName)} /* 574 */,
    (Member){GetTypeOrFail(STRING("Var")),STRING("subInstance"),offsetof(HierarchicalName,subInstance)} /* 575 */
  };

  RegisterStructMembers(STRING("Time"),(Array<Member>){&members[0],2});
  RegisterStructMembers(STRING("TimeIt"),(Array<Member>){&members[2],3});
  RegisterStructMembers(STRING("Conversion"),(Array<Member>){&members[5],3});
  RegisterStructMembers(STRING("Arena"),(Array<Member>){&members[8],5});
  RegisterStructMembers(STRING("ArenaMark"),(Array<Member>){&members[13],2});
  RegisterStructMembers(STRING("ArenaMarker"),(Array<Member>){&members[15],1});
  RegisterStructMembers(STRING("DynamicArena"),(Array<Member>){&members[16],4});
  RegisterStructMembers(STRING("DynamicString"),(Array<Member>){&members[20],2});
  RegisterStructMembers(STRING("BitIterator"),(Array<Member>){&members[22],3});
  RegisterStructMembers(STRING("BitArray"),(Array<Member>){&members[25],3});
  RegisterStructMembers(STRING("PoolHeader"),(Array<Member>){&members[28],2});
  RegisterStructMembers(STRING("PoolInfo"),(Array<Member>){&members[30],4});
  RegisterStructMembers(STRING("PageInfo"),(Array<Member>){&members[34],2});
  RegisterStructMembers(STRING("GenericPoolIterator"),(Array<Member>){&members[36],7});
  RegisterStructMembers(STRING("EnumMember"),(Array<Member>){&members[43],3});
  RegisterStructMembers(STRING("TemplateArg"),(Array<Member>){&members[46],2});
  RegisterStructMembers(STRING("TemplatedMember"),(Array<Member>){&members[48],3});
  RegisterStructMembers(STRING("NameAndTemplateArguments"),(Array<Member>){&members[51],2});
  RegisterStructMembers(STRING("ParsedType"),(Array<Member>){&members[53],4});
  RegisterStructMembers(STRING("Type"),(Array<Member>){&members[57],14});
  RegisterStructMembers(STRING("Member"),(Array<Member>){&members[71],6});
  RegisterStructMembers(STRING("Value"),(Array<Member>){&members[77],9});
  RegisterStructMembers(STRING("Iterator"),(Array<Member>){&members[86],4});
  RegisterStructMembers(STRING("HashmapUnpackedIndex"),(Array<Member>){&members[90],2});
  RegisterStructMembers(STRING("TypeIterator"),(Array<Member>){&members[92],2});
  RegisterStructMembers(STRING("Expression"),(Array<Member>){&members[94],8});
  RegisterStructMembers(STRING("Cursor"),(Array<Member>){&members[102],2});
  RegisterStructMembers(STRING("Token"),(Array<Member>){&members[104],1});
  RegisterStructMembers(STRING("FindFirstResult"),(Array<Member>){&members[105],2});
  RegisterStructMembers(STRING("Trie"),(Array<Member>){&members[107],1});
  RegisterStructMembers(STRING("TokenizerTemplate"),(Array<Member>){&members[108],1});
  RegisterStructMembers(STRING("TokenizerMark"),(Array<Member>){&members[109],2});
  RegisterStructMembers(STRING("Tokenizer"),(Array<Member>){&members[111],8});
  RegisterStructMembers(STRING("OperationList"),(Array<Member>){&members[119],3});
  RegisterStructMembers(STRING("CommandDefinition"),(Array<Member>){&members[122],4});
  RegisterStructMembers(STRING("Command"),(Array<Member>){&members[126],3});
  RegisterStructMembers(STRING("Block"),(Array<Member>){&members[129],7});
  RegisterStructMembers(STRING("TemplateFunction"),(Array<Member>){&members[136],2});
  RegisterStructMembers(STRING("TemplateRecord"),(Array<Member>){&members[138],5});
  RegisterStructMembers(STRING("ValueAndText"),(Array<Member>){&members[143],2});
  RegisterStructMembers(STRING("Frame"),(Array<Member>){&members[145],2});
  RegisterStructMembers(STRING("IndividualBlock"),(Array<Member>){&members[147],3});
  RegisterStructMembers(STRING("CompiledTemplate"),(Array<Member>){&members[150],4});
  RegisterStructMembers(STRING("PortDeclaration"),(Array<Member>){&members[154],4});
  RegisterStructMembers(STRING("ParameterExpression"),(Array<Member>){&members[158],2});
  RegisterStructMembers(STRING("Module"),(Array<Member>){&members[160],4});
  RegisterStructMembers(STRING("ExternalMemoryID"),(Array<Member>){&members[164],2});
  RegisterStructMembers(STRING("ExternalInfoTwoPorts"),(Array<Member>){&members[166],2});
  RegisterStructMembers(STRING("ExternalInfoDualPort"),(Array<Member>){&members[168],2});
  RegisterStructMembers(STRING("ExternalMemoryInfo"),(Array<Member>){&members[170],3});
  RegisterStructMembers(STRING("ModuleInfo"),(Array<Member>){&members[173],20});
  RegisterStructMembers(STRING("Options"),(Array<Member>){&members[193],21});
  RegisterStructMembers(STRING("DebugState"),(Array<Member>){&members[214],8});
  RegisterStructMembers(STRING("PortInstance"),(Array<Member>){&members[222],2});
  RegisterStructMembers(STRING("Edge"),(Array<Member>){&members[224],5});
  RegisterStructMembers(STRING("GenericGraphMapping"),(Array<Member>){&members[229],2});
  RegisterStructMembers(STRING("EdgeDelayInfo"),(Array<Member>){&members[231],2});
  RegisterStructMembers(STRING("DelayInfo"),(Array<Member>){&members[233],2});
  RegisterStructMembers(STRING("ConnectionNode"),(Array<Member>){&members[235],5});
  RegisterStructMembers(STRING("FUInstance"),(Array<Member>){&members[240],20});
  RegisterStructMembers(STRING("Accelerator"),(Array<Member>){&members[260],6});
  RegisterStructMembers(STRING("MemoryAddressMask"),(Array<Member>){&members[266],3});
  RegisterStructMembers(STRING("VersatComputedValues"),(Array<Member>){&members[269],26});
  RegisterStructMembers(STRING("DAGOrderNodes"),(Array<Member>){&members[295],7});
  RegisterStructMembers(STRING("AcceleratorMapping"),(Array<Member>){&members[302],4});
  RegisterStructMembers(STRING("EdgeIterator"),(Array<Member>){&members[306],2});
  RegisterStructMembers(STRING("StaticId"),(Array<Member>){&members[308],2});
  RegisterStructMembers(STRING("StaticData"),(Array<Member>){&members[310],1});
  RegisterStructMembers(STRING("StaticInfo"),(Array<Member>){&members[311],2});
  RegisterStructMembers(STRING("CalculatedOffsets"),(Array<Member>){&members[313],2});
  RegisterStructMembers(STRING("SubMappingInfo"),(Array<Member>){&members[315],4});
  RegisterStructMembers(STRING("InstanceInfo"),(Array<Member>){&members[319],30});
  RegisterStructMembers(STRING("AcceleratorInfo"),(Array<Member>){&members[349],4});
  RegisterStructMembers(STRING("InstanceConfigurationOffsets"),(Array<Member>){&members[353],13});
  RegisterStructMembers(STRING("TestResult"),(Array<Member>){&members[366],7});
  RegisterStructMembers(STRING("AccelInfo"),(Array<Member>){&members[373],24});
  RegisterStructMembers(STRING("TypeAndNameOnly"),(Array<Member>){&members[397],2});
  RegisterStructMembers(STRING("Partition"),(Array<Member>){&members[399],2});
  RegisterStructMembers(STRING("OrderedConfigurations"),(Array<Member>){&members[401],3});
  RegisterStructMembers(STRING("GraphPrintingNodeInfo"),(Array<Member>){&members[404],3});
  RegisterStructMembers(STRING("GraphPrintingEdgeInfo"),(Array<Member>){&members[407],4});
  RegisterStructMembers(STRING("GraphPrintingContent"),(Array<Member>){&members[411],3});
  RegisterStructMembers(STRING("GraphInfo"),(Array<Member>){&members[414],2});
  RegisterStructMembers(STRING("CalculateDelayResult"),(Array<Member>){&members[416],3});
  RegisterStructMembers(STRING("DelayToAdd"),(Array<Member>){&members[419],4});
  RegisterStructMembers(STRING("ConfigurationInfo"),(Array<Member>){&members[423],16});
  RegisterStructMembers(STRING("FUDeclaration"),(Array<Member>){&members[439],18});
  RegisterStructMembers(STRING("SingleTypeStructElement"),(Array<Member>){&members[457],3});
  RegisterStructMembers(STRING("TypeStructInfoElement"),(Array<Member>){&members[460],1});
  RegisterStructMembers(STRING("TypeStructInfo"),(Array<Member>){&members[461],2});
  RegisterStructMembers(STRING("Difference"),(Array<Member>){&members[463],2});
  RegisterStructMembers(STRING("DifferenceArray"),(Array<Member>){&members[465],3});
  RegisterStructMembers(STRING("MuxInfo"),(Array<Member>){&members[468],4});
  RegisterStructMembers(STRING("Task"),(Array<Member>){&members[472],3});
  RegisterStructMembers(STRING("WorkGroup"),(Array<Member>){&members[475],2});
  RegisterStructMembers(STRING("SpecificMerge"),(Array<Member>){&members[477],2});
  RegisterStructMembers(STRING("IndexRecord"),(Array<Member>){&members[479],2});
  RegisterStructMembers(STRING("SpecificMergeNodes"),(Array<Member>){&members[481],2});
  RegisterStructMembers(STRING("SpecificMergeNode"),(Array<Member>){&members[483],4});
  RegisterStructMembers(STRING("MergeEdge"),(Array<Member>){&members[487],1});
  RegisterStructMembers(STRING("MappingNode"),(Array<Member>){&members[488],3});
  RegisterStructMembers(STRING("ConsolidationGraphOptions"),(Array<Member>){&members[491],5});
  RegisterStructMembers(STRING("ConsolidationGraph"),(Array<Member>){&members[496],3});
  RegisterStructMembers(STRING("ConsolidationResult"),(Array<Member>){&members[499],3});
  RegisterStructMembers(STRING("CliqueState"),(Array<Member>){&members[502],8});
  RegisterStructMembers(STRING("IsCliqueResult"),(Array<Member>){&members[510],2});
  RegisterStructMembers(STRING("MergeGraphResult"),(Array<Member>){&members[512],5});
  RegisterStructMembers(STRING("MergeGraphResultExisting"),(Array<Member>){&members[517],3});
  RegisterStructMembers(STRING("GraphMapping"),(Array<Member>){&members[520],3});
  RegisterStructMembers(STRING("ConnectionExtra"),(Array<Member>){&members[523],2});
  RegisterStructMembers(STRING("Var"),(Array<Member>){&members[525],4});
  RegisterStructMembers(STRING("VarGroup"),(Array<Member>){&members[529],2});
  RegisterStructMembers(STRING("SpecExpression"),(Array<Member>){&members[531],6});
  RegisterStructMembers(STRING("VarDeclaration"),(Array<Member>){&members[537],3});
  RegisterStructMembers(STRING("GroupIterator"),(Array<Member>){&members[540],3});
  RegisterStructMembers(STRING("PortExpression"),(Array<Member>){&members[543],2});
  RegisterStructMembers(STRING("InstanceDeclaration"),(Array<Member>){&members[545],4});
  RegisterStructMembers(STRING("ConnectionDef"),(Array<Member>){&members[549],6});
  RegisterStructMembers(STRING("TypeAndInstance"),(Array<Member>){&members[555],2});
  RegisterStructMembers(STRING("ModuleDef"),(Array<Member>){&members[557],5});
  RegisterStructMembers(STRING("TransformDef"),(Array<Member>){&members[562],3});
  RegisterStructMembers(STRING("MergeDef"),(Array<Member>){&members[565],3});
  RegisterStructMembers(STRING("TypeDefinition"),(Array<Member>){&members[568],3});
  RegisterStructMembers(STRING("Transformation"),(Array<Member>){&members[571],3});
  RegisterStructMembers(STRING("HierarchicalName"),(Array<Member>){&members[574],2});


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
  RegisterTypedef(STRING("Hashmap<Edge, Edge>"),STRING("EdgeMap"));
  RegisterTypedef(STRING("Array<Edge>"),STRING("Path"));
  RegisterTypedef(STRING("Hashmap<Edge, Path>"),STRING("PathMap"));
  RegisterTypedef(STRING("TrieMap<SubMappingInfo, PortInstance>"),STRING("SubMap"));
  RegisterTypedef(STRING("Hashmap<Edge, DelayInfo>"),STRING("EdgeDelay"));
  RegisterTypedef(STRING("Hashmap<PortInstance, DelayInfo>"),STRING("PortDelay"));
  RegisterTypedef(STRING("Hashmap<FUInstance *, DelayInfo>"),STRING("NodeDelay"));
  RegisterTypedef(STRING("Hashmap<String, FUInstance *>"),STRING("InstanceTable"));
  RegisterTypedef(STRING("Set<String>"),STRING("InstanceName"));
  RegisterTypedef(STRING("Pair<HierarchicalName, HierarchicalName>"),STRING("SpecNode"));

}
