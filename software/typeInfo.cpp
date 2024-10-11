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
  RegisterOpaqueType(STRING("ParameterValue"),Subtype_STRUCT,sizeof(ParameterValue),alignof(ParameterValue));
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
  RegisterOpaqueType(STRING("WireInformation"),Subtype_STRUCT,sizeof(WireInformation),alignof(WireInformation));
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
  RegisterOpaqueType(STRING("DefBase"),Subtype_STRUCT,sizeof(DefBase),alignof(DefBase));
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

RegisterEnum(STRING("LogModule"),sizeof(LogModule),alignof(LogModule),C_ARRAY_TO_ARRAY(LogModuleData));

static Pair<String,int> LogLevelData[] = {{STRING("DEBUG"),(int) LogLevel::DEBUG},
    {STRING("INFO"),(int) LogLevel::INFO},
    {STRING("WARN"),(int) LogLevel::WARN},
    {STRING("ERROR"),(int) LogLevel::ERROR},
    {STRING("FATAL"),(int) LogLevel::FATAL}};

RegisterEnum(STRING("LogLevel"),sizeof(LogLevel),alignof(LogLevel),C_ARRAY_TO_ARRAY(LogLevelData));

static Pair<String,int> SubtypeData[] = {{STRING("Subtype_UNKNOWN"),(int) Subtype::Subtype_UNKNOWN},
    {STRING("Subtype_BASE"),(int) Subtype::Subtype_BASE},
    {STRING("Subtype_STRUCT"),(int) Subtype::Subtype_STRUCT},
    {STRING("Subtype_POINTER"),(int) Subtype::Subtype_POINTER},
    {STRING("Subtype_ARRAY"),(int) Subtype::Subtype_ARRAY},
    {STRING("Subtype_TEMPLATED_STRUCT_DEF"),(int) Subtype::Subtype_TEMPLATED_STRUCT_DEF},
    {STRING("Subtype_TEMPLATED_INSTANCE"),(int) Subtype::Subtype_TEMPLATED_INSTANCE},
    {STRING("Subtype_ENUM"),(int) Subtype::Subtype_ENUM},
    {STRING("Subtype_TYPEDEF"),(int) Subtype::Subtype_TYPEDEF}};

RegisterEnum(STRING("Subtype"),sizeof(Subtype),alignof(Subtype),C_ARRAY_TO_ARRAY(SubtypeData));

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

RegisterEnum(STRING("CommandType"),sizeof(CommandType),alignof(CommandType),C_ARRAY_TO_ARRAY(CommandTypeData));

static Pair<String,int> BlockTypeData[] = {{STRING("BlockType_TEXT"),(int) BlockType::BlockType_TEXT},
    {STRING("BlockType_COMMAND"),(int) BlockType::BlockType_COMMAND},
    {STRING("BlockType_EXPRESSION"),(int) BlockType::BlockType_EXPRESSION}};

RegisterEnum(STRING("BlockType"),sizeof(BlockType),alignof(BlockType),C_ARRAY_TO_ARRAY(BlockTypeData));

static Pair<String,int> TemplateRecordTypeData[] = {{STRING("TemplateRecordType_FIELD"),(int) TemplateRecordType::TemplateRecordType_FIELD},
    {STRING("TemplateRecordType_IDENTIFER"),(int) TemplateRecordType::TemplateRecordType_IDENTIFER}};

RegisterEnum(STRING("TemplateRecordType"),sizeof(TemplateRecordType),alignof(TemplateRecordType),C_ARRAY_TO_ARRAY(TemplateRecordTypeData));

static Pair<String,int> VersatStageData[] = {{STRING("VersatStage_COMPUTE"),(int) VersatStage::VersatStage_COMPUTE},
    {STRING("VersatStage_READ"),(int) VersatStage::VersatStage_READ},
    {STRING("VersatStage_WRITE"),(int) VersatStage::VersatStage_WRITE}};

RegisterEnum(STRING("VersatStage"),sizeof(VersatStage),alignof(VersatStage),C_ARRAY_TO_ARRAY(VersatStageData));

static Pair<String,int> ExternalMemoryTypeData[] = {{STRING("TWO_P"),(int) ExternalMemoryType::TWO_P},
    {STRING("DP"),(int) ExternalMemoryType::DP}};

RegisterEnum(STRING("ExternalMemoryType"),sizeof(ExternalMemoryType),alignof(ExternalMemoryType),C_ARRAY_TO_ARRAY(ExternalMemoryTypeData));

static Pair<String,int> NodeTypeData[] = {{STRING("NodeType_UNCONNECTED"),(int) NodeType::NodeType_UNCONNECTED},
    {STRING("NodeType_SOURCE"),(int) NodeType::NodeType_SOURCE},
    {STRING("NodeType_COMPUTE"),(int) NodeType::NodeType_COMPUTE},
    {STRING("NodeType_SINK"),(int) NodeType::NodeType_SINK},
    {STRING("NodeType_SOURCE_AND_SINK"),(int) NodeType::NodeType_SOURCE_AND_SINK}};

RegisterEnum(STRING("NodeType"),sizeof(NodeType),alignof(NodeType),C_ARRAY_TO_ARRAY(NodeTypeData));

static Pair<String,int> AcceleratorPurposeData[] = {{STRING("AcceleratorPurpose_TEMP"),(int) AcceleratorPurpose::AcceleratorPurpose_TEMP},
    {STRING("AcceleratorPurpose_BASE"),(int) AcceleratorPurpose::AcceleratorPurpose_BASE},
    {STRING("AcceleratorPurpose_FLATTEN"),(int) AcceleratorPurpose::AcceleratorPurpose_FLATTEN},
    {STRING("AcceleratorPurpose_MODULE"),(int) AcceleratorPurpose::AcceleratorPurpose_MODULE},
    {STRING("AcceleratorPurpose_RECON"),(int) AcceleratorPurpose::AcceleratorPurpose_RECON},
    {STRING("AcceleratorPurpose_MERGE"),(int) AcceleratorPurpose::AcceleratorPurpose_MERGE}};

RegisterEnum(STRING("AcceleratorPurpose"),sizeof(AcceleratorPurpose),alignof(AcceleratorPurpose),C_ARRAY_TO_ARRAY(AcceleratorPurposeData));

static Pair<String,int> MemTypeData[] = {{STRING("CONFIG"),(int) MemType::CONFIG},
    {STRING("STATE"),(int) MemType::STATE},
    {STRING("DELAY"),(int) MemType::DELAY},
    {STRING("STATIC"),(int) MemType::STATIC}};

RegisterEnum(STRING("MemType"),sizeof(MemType),alignof(MemType),C_ARRAY_TO_ARRAY(MemTypeData));

static Pair<String,int> ColorData[] = {{STRING("Color_BLACK"),(int) Color::Color_BLACK},
    {STRING("Color_BLUE"),(int) Color::Color_BLUE},
    {STRING("Color_RED"),(int) Color::Color_RED},
    {STRING("Color_GREEN"),(int) Color::Color_GREEN},
    {STRING("Color_YELLOW"),(int) Color::Color_YELLOW}};

RegisterEnum(STRING("Color"),sizeof(Color),alignof(Color),C_ARRAY_TO_ARRAY(ColorData));

static Pair<String,int> VersatDebugFlagsData[] = {{STRING("OUTPUT_GRAPH_DOT"),(int) VersatDebugFlags::OUTPUT_GRAPH_DOT},
    {STRING("GRAPH_DOT_FORMAT"),(int) VersatDebugFlags::GRAPH_DOT_FORMAT},
    {STRING("OUTPUT_ACCELERATORS_CODE"),(int) VersatDebugFlags::OUTPUT_ACCELERATORS_CODE},
    {STRING("OUTPUT_VERSAT_CODE"),(int) VersatDebugFlags::OUTPUT_VERSAT_CODE},
    {STRING("USE_FIXED_BUFFERS"),(int) VersatDebugFlags::USE_FIXED_BUFFERS}};

RegisterEnum(STRING("VersatDebugFlags"),sizeof(VersatDebugFlags),alignof(VersatDebugFlags),C_ARRAY_TO_ARRAY(VersatDebugFlagsData));

static Pair<String,int> GraphDotFormat_Data[] = {{STRING("GRAPH_DOT_FORMAT_NAME"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_NAME},
    {STRING("GRAPH_DOT_FORMAT_TYPE"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_TYPE},
    {STRING("GRAPH_DOT_FORMAT_ID"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_ID},
    {STRING("GRAPH_DOT_FORMAT_DELAY"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_DELAY},
    {STRING("GRAPH_DOT_FORMAT_EXPLICIT"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_EXPLICIT},
    {STRING("GRAPH_DOT_FORMAT_PORT"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_PORT},
    {STRING("GRAPH_DOT_FORMAT_LATENCY"),(int) GraphDotFormat_::GRAPH_DOT_FORMAT_LATENCY}};

RegisterEnum(STRING("GraphDotFormat_"),sizeof(GraphDotFormat_),alignof(GraphDotFormat_),C_ARRAY_TO_ARRAY(GraphDotFormat_Data));

static Pair<String,int> DelayTypeData[] = {{STRING("DelayType_BASE"),(int) DelayType::DelayType_BASE},
    {STRING("DelayType_SINK_DELAY"),(int) DelayType::DelayType_SINK_DELAY},
    {STRING("DelayType_SOURCE_DELAY"),(int) DelayType::DelayType_SOURCE_DELAY},
    {STRING("DelayType_COMPUTE_DELAY"),(int) DelayType::DelayType_COMPUTE_DELAY}};

RegisterEnum(STRING("DelayType"),sizeof(DelayType),alignof(DelayType),C_ARRAY_TO_ARRAY(DelayTypeData));

static Pair<String,int> FUDeclarationTypeData[] = {{STRING("FUDeclarationType_SINGLE"),(int) FUDeclarationType::FUDeclarationType_SINGLE},
    {STRING("FUDeclarationType_COMPOSITE"),(int) FUDeclarationType::FUDeclarationType_COMPOSITE},
    {STRING("FUDeclarationType_SPECIAL"),(int) FUDeclarationType::FUDeclarationType_SPECIAL},
    {STRING("FUDeclarationType_MERGED"),(int) FUDeclarationType::FUDeclarationType_MERGED},
    {STRING("FUDeclarationType_ITERATIVE"),(int) FUDeclarationType::FUDeclarationType_ITERATIVE}};

RegisterEnum(STRING("FUDeclarationType"),sizeof(FUDeclarationType),alignof(FUDeclarationType),C_ARRAY_TO_ARRAY(FUDeclarationTypeData));

static Pair<String,int> MergingStrategyData[] = {{STRING("SIMPLE_COMBINATION"),(int) MergingStrategy::SIMPLE_COMBINATION},
    {STRING("CONSOLIDATION_GRAPH"),(int) MergingStrategy::CONSOLIDATION_GRAPH},
    {STRING("FIRST_FIT"),(int) MergingStrategy::FIRST_FIT}};

RegisterEnum(STRING("MergingStrategy"),sizeof(MergingStrategy),alignof(MergingStrategy),C_ARRAY_TO_ARRAY(MergingStrategyData));

static Pair<String,int> ConnectionTypeData[] = {{STRING("ConnectionType_SINGLE"),(int) ConnectionType::ConnectionType_SINGLE},
    {STRING("ConnectionType_PORT_RANGE"),(int) ConnectionType::ConnectionType_PORT_RANGE},
    {STRING("ConnectionType_ARRAY_RANGE"),(int) ConnectionType::ConnectionType_ARRAY_RANGE},
    {STRING("ConnectionType_DELAY_RANGE"),(int) ConnectionType::ConnectionType_DELAY_RANGE},
    {STRING("ConnectionType_ERROR"),(int) ConnectionType::ConnectionType_ERROR}};

RegisterEnum(STRING("ConnectionType"),sizeof(ConnectionType),alignof(ConnectionType),C_ARRAY_TO_ARRAY(ConnectionTypeData));

static Pair<String,int> DefinitionTypeData[] = {{STRING("DefinitionType_MODULE"),(int) DefinitionType::DefinitionType_MODULE},
    {STRING("DefinitionType_MERGE"),(int) DefinitionType::DefinitionType_MERGE},
    {STRING("DefinitionType_ITERATIVE"),(int) DefinitionType::DefinitionType_ITERATIVE}};

RegisterEnum(STRING("DefinitionType"),sizeof(DefinitionType),alignof(DefinitionType),C_ARRAY_TO_ARRAY(DefinitionTypeData));

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
    STRING("T") /* 10 */,
    STRING("Key") /* 11 */,
    STRING("Data") /* 12 */,
    STRING("Data") /* 13 */,
    STRING("Key") /* 14 */,
    STRING("Data") /* 15 */,
    STRING("Data") /* 16 */,
    STRING("Data") /* 17 */,
    STRING("Key") /* 18 */,
    STRING("Data") /* 19 */,
    STRING("Key") /* 20 */,
    STRING("Data") /* 21 */,
    STRING("Key") /* 22 */,
    STRING("Data") /* 23 */,
    STRING("Data") /* 24 */,
    STRING("Data") /* 25 */,
    STRING("T") /* 26 */,
    STRING("T") /* 27 */,
    STRING("Value") /* 28 */,
    STRING("Error") /* 29 */,
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
  RegisterTemplate(STRING("GrowableArray"),(Array<String>){&templateArgs[8],1});
  RegisterTemplate(STRING("PushPtr"),(Array<String>){&templateArgs[9],1});
  RegisterTemplate(STRING("Stack"),(Array<String>){&templateArgs[10],1});
  RegisterTemplate(STRING("HashmapIterator"),(Array<String>){&templateArgs[11],2});
  RegisterTemplate(STRING("GetOrAllocateResult"),(Array<String>){&templateArgs[13],1});
  RegisterTemplate(STRING("Hashmap"),(Array<String>){&templateArgs[14],2});
  RegisterTemplate(STRING("Set"),(Array<String>){&templateArgs[16],1});
  RegisterTemplate(STRING("SetIterator"),(Array<String>){&templateArgs[17],1});
  RegisterTemplate(STRING("TrieMapNode"),(Array<String>){&templateArgs[18],2});
  RegisterTemplate(STRING("TrieMapIterator"),(Array<String>){&templateArgs[20],2});
  RegisterTemplate(STRING("TrieMap"),(Array<String>){&templateArgs[22],2});
  RegisterTemplate(STRING("TrieSetIterator"),(Array<String>){&templateArgs[24],1});
  RegisterTemplate(STRING("TrieSet"),(Array<String>){&templateArgs[25],1});
  RegisterTemplate(STRING("PoolIterator"),(Array<String>){&templateArgs[26],1});
  RegisterTemplate(STRING("Pool"),(Array<String>){&templateArgs[27],1});
  RegisterTemplate(STRING("Result"),(Array<String>){&templateArgs[28],2});
  RegisterTemplate(STRING("IndexedStruct"),(Array<String>){&templateArgs[30],1});
  RegisterTemplate(STRING("ListedStruct"),(Array<String>){&templateArgs[31],1});
  RegisterTemplate(STRING("ArenaList"),(Array<String>){&templateArgs[32],1});
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
    (TemplatedMember){STRING("Byte *"),STRING("nextExpectedAddress"),1} /* 17 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),0} /* 18 */,
    (TemplatedMember){STRING("T *"),STRING("data"),1} /* 19 */,
    (TemplatedMember){STRING("int"),STRING("size"),2} /* 20 */,
    (TemplatedMember){STRING("int"),STRING("capacity"),3} /* 21 */,
    (TemplatedMember){STRING("T *"),STRING("ptr"),0} /* 22 */,
    (TemplatedMember){STRING("int"),STRING("maximumTimes"),1} /* 23 */,
    (TemplatedMember){STRING("int"),STRING("timesPushed"),2} /* 24 */,
    (TemplatedMember){STRING("Array<T>"),STRING("array"),0} /* 25 */,
    (TemplatedMember){STRING("int"),STRING("used"),1} /* 26 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("pairs"),0} /* 27 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 28 */,
    (TemplatedMember){STRING("Data *"),STRING("data"),0} /* 29 */,
    (TemplatedMember){STRING("bool"),STRING("alreadyExisted"),1} /* 30 */,
    (TemplatedMember){STRING("int"),STRING("nodesAllocated"),0} /* 31 */,
    (TemplatedMember){STRING("int"),STRING("nodesUsed"),1} /* 32 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("buckets"),2} /* 33 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("data"),3} /* 34 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("next"),4} /* 35 */,
    (TemplatedMember){STRING("Hashmap<Data, int> *"),STRING("map"),0} /* 36 */,
    (TemplatedMember){STRING("HashmapIterator<Data, int>"),STRING("innerIter"),0} /* 37 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 38 */,
    (TemplatedMember){STRING("Pair<Key, Data>"),STRING("pair"),1} /* 39 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("next"),2} /* 40 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("ptr"),0} /* 41 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 42 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),1} /* 43 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("head"),2} /* 44 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("tail"),3} /* 45 */,
    (TemplatedMember){STRING("int"),STRING("inserted"),4} /* 46 */,
    (TemplatedMember){STRING("TrieMapIterator<Data, int>"),STRING("innerIter"),0} /* 47 */,
    (TemplatedMember){STRING("TrieMap<Data, int> *"),STRING("map"),0} /* 48 */,
    (TemplatedMember){STRING("Pool<T> *"),STRING("pool"),0} /* 49 */,
    (TemplatedMember){STRING("PageInfo"),STRING("pageInfo"),1} /* 50 */,
    (TemplatedMember){STRING("int"),STRING("fullIndex"),2} /* 51 */,
    (TemplatedMember){STRING("int"),STRING("bit"),3} /* 52 */,
    (TemplatedMember){STRING("int"),STRING("index"),4} /* 53 */,
    (TemplatedMember){STRING("Byte *"),STRING("page"),5} /* 54 */,
    (TemplatedMember){STRING("T *"),STRING("lastVal"),6} /* 55 */,
    (TemplatedMember){STRING("Byte *"),STRING("mem"),0} /* 56 */,
    (TemplatedMember){STRING("PoolInfo"),STRING("info"),1} /* 57 */,
    (TemplatedMember){STRING("Value"),STRING("value"),0} /* 58 */,
    (TemplatedMember){STRING("Error"),STRING("error"),0} /* 59 */,
    (TemplatedMember){STRING("bool"),STRING("isError"),1} /* 60 */,
    (TemplatedMember){STRING("int"),STRING("index"),0} /* 61 */,
    (TemplatedMember){STRING("T"),STRING("elem"),0} /* 62 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("next"),1} /* 63 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),0} /* 64 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("head"),1} /* 65 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("tail"),2} /* 66 */,
    (TemplatedMember){STRING("String"),STRING("name"),0} /* 67 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),1} /* 68 */,
    (TemplatedMember){STRING("VersatStage"),STRING("stage"),2} /* 69 */,
    (TemplatedMember){STRING("bool"),STRING("isStatic"),3} /* 70 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeIn"),0} /* 71 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeOut"),1} /* 72 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),2} /* 73 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),3} /* 74 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),0} /* 75 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),1} /* 76 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),2} /* 77 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 78 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 79 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 80 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 81 */,
    (TemplatedMember){STRING("ExternalMemoryType"),STRING("type"),1} /* 82 */,
    (TemplatedMember){STRING("int"),STRING("interface"),2} /* 83 */,
    (TemplatedMember){STRING("T*"),STRING("mem"),0} /* 84 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 85 */,
    (TemplatedMember){STRING("int"),STRING("allocated"),2} /* 86 */
  };

  RegisterTemplateMembers(STRING("_Defer"),(Array<TemplatedMember>){&templateMembers[0],1});
  RegisterTemplateMembers(STRING("Opt"),(Array<TemplatedMember>){&templateMembers[1],2});
  RegisterTemplateMembers(STRING("ArrayIterator"),(Array<TemplatedMember>){&templateMembers[3],1});
  RegisterTemplateMembers(STRING("Array"),(Array<TemplatedMember>){&templateMembers[4],2});
  RegisterTemplateMembers(STRING("Range"),(Array<TemplatedMember>){&templateMembers[6],6});
  RegisterTemplateMembers(STRING("Pair"),(Array<TemplatedMember>){&templateMembers[12],4});
  RegisterTemplateMembers(STRING("DynamicArray"),(Array<TemplatedMember>){&templateMembers[16],2});
  RegisterTemplateMembers(STRING("GrowableArray"),(Array<TemplatedMember>){&templateMembers[18],4});
  RegisterTemplateMembers(STRING("PushPtr"),(Array<TemplatedMember>){&templateMembers[22],3});
  RegisterTemplateMembers(STRING("Stack"),(Array<TemplatedMember>){&templateMembers[25],2});
  RegisterTemplateMembers(STRING("HashmapIterator"),(Array<TemplatedMember>){&templateMembers[27],2});
  RegisterTemplateMembers(STRING("GetOrAllocateResult"),(Array<TemplatedMember>){&templateMembers[29],2});
  RegisterTemplateMembers(STRING("Hashmap"),(Array<TemplatedMember>){&templateMembers[31],5});
  RegisterTemplateMembers(STRING("Set"),(Array<TemplatedMember>){&templateMembers[36],1});
  RegisterTemplateMembers(STRING("SetIterator"),(Array<TemplatedMember>){&templateMembers[37],1});
  RegisterTemplateMembers(STRING("TrieMapNode"),(Array<TemplatedMember>){&templateMembers[38],3});
  RegisterTemplateMembers(STRING("TrieMapIterator"),(Array<TemplatedMember>){&templateMembers[41],1});
  RegisterTemplateMembers(STRING("TrieMap"),(Array<TemplatedMember>){&templateMembers[42],5});
  RegisterTemplateMembers(STRING("TrieSetIterator"),(Array<TemplatedMember>){&templateMembers[47],1});
  RegisterTemplateMembers(STRING("TrieSet"),(Array<TemplatedMember>){&templateMembers[48],1});
  RegisterTemplateMembers(STRING("PoolIterator"),(Array<TemplatedMember>){&templateMembers[49],7});
  RegisterTemplateMembers(STRING("Pool"),(Array<TemplatedMember>){&templateMembers[56],2});
  RegisterTemplateMembers(STRING("Result"),(Array<TemplatedMember>){&templateMembers[58],3});
  RegisterTemplateMembers(STRING("IndexedStruct"),(Array<TemplatedMember>){&templateMembers[61],1});
  RegisterTemplateMembers(STRING("ListedStruct"),(Array<TemplatedMember>){&templateMembers[62],2});
  RegisterTemplateMembers(STRING("ArenaList"),(Array<TemplatedMember>){&templateMembers[64],3});
  RegisterTemplateMembers(STRING("WireTemplate"),(Array<TemplatedMember>){&templateMembers[67],4});
  RegisterTemplateMembers(STRING("ExternalMemoryTwoPortsTemplate"),(Array<TemplatedMember>){&templateMembers[71],4});
  RegisterTemplateMembers(STRING("ExternalMemoryDualPortTemplate"),(Array<TemplatedMember>){&templateMembers[75],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTemplate"),(Array<TemplatedMember>){&templateMembers[78],2});
  RegisterTemplateMembers(STRING("ExternalMemoryInterfaceTemplate"),(Array<TemplatedMember>){&templateMembers[80],4});
  RegisterTemplateMembers(STRING("std::vector"),(Array<TemplatedMember>){&templateMembers[84],3});

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
    (Member){GetTypeOrFail(STRING("Arena *")),STRING("arena"),offsetof(ArenaMark,arena)} /* 12 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mark"),offsetof(ArenaMark,mark)} /* 13 */,
    (Member){GetTypeOrFail(STRING("ArenaMark")),STRING("mark"),offsetof(ArenaMarker,mark)} /* 14 */,
    (Member){GetTypeOrFail(STRING("DynamicArena *")),STRING("next"),offsetof(DynamicArena,next)} /* 15 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mem"),offsetof(DynamicArena,mem)} /* 16 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("used"),offsetof(DynamicArena,used)} /* 17 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("pagesAllocated"),offsetof(DynamicArena,pagesAllocated)} /* 18 */,
    (Member){GetTypeOrFail(STRING("Arena *")),STRING("arena"),offsetof(DynamicString,arena)} /* 19 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("mark"),offsetof(DynamicString,mark)} /* 20 */,
    (Member){GetTypeOrFail(STRING("BitArray *")),STRING("array"),offsetof(BitIterator,array)} /* 21 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentByte"),offsetof(BitIterator,currentByte)} /* 22 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentBit"),offsetof(BitIterator,currentBit)} /* 23 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("memory"),offsetof(BitArray,memory)} /* 24 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bitSize"),offsetof(BitArray,bitSize)} /* 25 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("byteSize"),offsetof(BitArray,byteSize)} /* 26 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("nextPage"),offsetof(PoolHeader,nextPage)} /* 27 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("allocatedUnits"),offsetof(PoolHeader,allocatedUnits)} /* 28 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsPerFullPage"),offsetof(PoolInfo,unitsPerFullPage)} /* 29 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bitmapSize"),offsetof(PoolInfo,bitmapSize)} /* 30 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsPerPage"),offsetof(PoolInfo,unitsPerPage)} /* 31 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("pageGranuality"),offsetof(PoolInfo,pageGranuality)} /* 32 */,
    (Member){GetTypeOrFail(STRING("PoolHeader *")),STRING("header"),offsetof(PageInfo,header)} /* 33 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("bitmap"),offsetof(PageInfo,bitmap)} /* 34 */,
    (Member){GetTypeOrFail(STRING("PoolInfo")),STRING("poolInfo"),offsetof(GenericPoolIterator,poolInfo)} /* 35 */,
    (Member){GetTypeOrFail(STRING("PageInfo")),STRING("pageInfo"),offsetof(GenericPoolIterator,pageInfo)} /* 36 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("fullIndex"),offsetof(GenericPoolIterator,fullIndex)} /* 37 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bit"),offsetof(GenericPoolIterator,bit)} /* 38 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(GenericPoolIterator,index)} /* 39 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("elemSize"),offsetof(GenericPoolIterator,elemSize)} /* 40 */,
    (Member){GetTypeOrFail(STRING("Byte *")),STRING("page"),offsetof(GenericPoolIterator,page)} /* 41 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(EnumMember,name)} /* 42 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("value"),offsetof(EnumMember,value)} /* 43 */,
    (Member){GetTypeOrFail(STRING("EnumMember *")),STRING("next"),offsetof(EnumMember,next)} /* 44 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TemplateArg,name)} /* 45 */,
    (Member){GetTypeOrFail(STRING("TemplateArg *")),STRING("next"),offsetof(TemplateArg,next)} /* 46 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("typeName"),offsetof(TemplatedMember,typeName)} /* 47 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TemplatedMember,name)} /* 48 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memberOffset"),offsetof(TemplatedMember,memberOffset)} /* 49 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(NameAndTemplateArguments,baseName)} /* 50 */,
    (Member){GetTypeOrFail(STRING("Array<ParsedType>")),STRING("templateMembers"),offsetof(NameAndTemplateArguments,templateMembers)} /* 51 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(ParsedType,baseName)} /* 52 */,
    (Member){GetTypeOrFail(STRING("Array<ParsedType>")),STRING("templateMembers"),offsetof(ParsedType,templateMembers)} /* 53 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("amountOfPointers"),offsetof(ParsedType,amountOfPointers)} /* 54 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("arrayExpressions"),offsetof(ParsedType,arrayExpressions)} /* 55 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Type,name)} /* 56 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("baseType"),offsetof(Type,baseType)} /* 57 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("pointerType"),offsetof(Type,pointerType)} /* 58 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("arrayType"),offsetof(Type,arrayType)} /* 59 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("typedefType"),offsetof(Type,typedefType)} /* 60 */,
    (Member){GetTypeOrFail(STRING("Array<Pair<String, int>>")),STRING("enumMembers"),offsetof(Type,enumMembers)} /* 61 */,
    (Member){GetTypeOrFail(STRING("Array<TemplatedMember>")),STRING("templateMembers"),offsetof(Type,templateMembers)} /* 62 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("templateArgs"),offsetof(Type,templateArgs)} /* 63 */,
    (Member){GetTypeOrFail(STRING("Array<Member>")),STRING("members"),offsetof(Type,members)} /* 64 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("templateBase"),offsetof(Type,templateBase)} /* 65 */,
    (Member){GetTypeOrFail(STRING("Array<Type *>")),STRING("templateArgTypes"),offsetof(Type,templateArgTypes)} /* 66 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("size"),offsetof(Type,size)} /* 67 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("align"),offsetof(Type,align)} /* 68 */,
    (Member){GetTypeOrFail(STRING("Subtype")),STRING("type"),offsetof(Type,type)} /* 69 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("type"),offsetof(Member,type)} /* 70 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Member,name)} /* 71 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("offset"),offsetof(Member,offset)} /* 72 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("structType"),offsetof(Member,structType)} /* 73 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("arrayExpression"),offsetof(Member,arrayExpression)} /* 74 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(Member,index)} /* 75 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("boolean"),offsetof(Value,boolean)} /* 76 */,
    (Member){GetTypeOrFail(STRING("char")),STRING("ch"),offsetof(Value,ch)} /* 77 */,
    (Member){GetTypeOrFail(STRING("i64")),STRING("number"),offsetof(Value,number)} /* 78 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("str"),offsetof(Value,str)} /* 79 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("literal"),offsetof(Value,literal)} /* 80 */,
    (Member){GetTypeOrFail(STRING("TemplateFunction *")),STRING("templateFunction"),offsetof(Value,templateFunction)} /* 81 */,
    (Member){GetTypeOrFail(STRING("void *")),STRING("custom"),offsetof(Value,custom)} /* 82 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("type"),offsetof(Value,type)} /* 83 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isTemp"),offsetof(Value,isTemp)} /* 84 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("currentNumber"),offsetof(Iterator,currentNumber)} /* 85 */,
    (Member){GetTypeOrFail(STRING("GenericPoolIterator")),STRING("poolIterator"),offsetof(Iterator,poolIterator)} /* 86 */,
    (Member){GetTypeOrFail(STRING("Type *")),STRING("hashmapType"),offsetof(Iterator,hashmapType)} /* 87 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("iterating"),offsetof(Iterator,iterating)} /* 88 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(HashmapUnpackedIndex,index)} /* 89 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("data"),offsetof(HashmapUnpackedIndex,data)} /* 90 */,
    (Member){GetTypeOrFail(STRING("PoolIterator<Type>")),STRING("iter"),offsetof(TypeIterator,iter)} /* 91 */,
    (Member){GetTypeOrFail(STRING("PoolIterator<Type>")),STRING("end"),offsetof(TypeIterator,end)} /* 92 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("op"),offsetof(Expression,op)} /* 93 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("id"),offsetof(Expression,id)} /* 94 */,
    (Member){GetTypeOrFail(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Expression,expressions)} /* 95 */,
    (Member){GetTypeOrFail(STRING("Command *")),STRING("command"),offsetof(Expression,command)} /* 96 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("val"),offsetof(Expression,val)} /* 97 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("text"),offsetof(Expression,text)} /* 98 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("approximateLine"),offsetof(Expression,approximateLine)} /* 99 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(Expression,type)} /* 100 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("line"),offsetof(Cursor,line)} /* 101 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("column"),offsetof(Cursor,column)} /* 102 */,
    (Member){GetTypeOrFail(STRING("Range<Cursor>")),STRING("loc"),offsetof(Token,loc)} /* 103 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("foundFirst"),offsetof(FindFirstResult,foundFirst)} /* 104 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("peekFindNotIncluded"),offsetof(FindFirstResult,peekFindNotIncluded)} /* 105 */,
    (Member){GetTypeOrFail(STRING("u16[128]")),STRING("array"),offsetof(Trie,array)} /* 106 */,
    (Member){GetTypeOrFail(STRING("Array<Trie>")),STRING("subTries"),offsetof(TokenizerTemplate,subTries)} /* 107 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("ptr"),offsetof(TokenizerMark,ptr)} /* 108 */,
    (Member){GetTypeOrFail(STRING("Cursor")),STRING("pos"),offsetof(TokenizerMark,pos)} /* 109 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("start"),offsetof(Tokenizer,start)} /* 110 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("ptr"),offsetof(Tokenizer,ptr)} /* 111 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("end"),offsetof(Tokenizer,end)} /* 112 */,
    (Member){GetTypeOrFail(STRING("TokenizerTemplate *")),STRING("tmpl"),offsetof(Tokenizer,tmpl)} /* 113 */,
    (Member){GetTypeOrFail(STRING("Arena")),STRING("leaky"),offsetof(Tokenizer,leaky)} /* 114 */,
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
    (Member){GetTypeOrFail(STRING("String")),STRING("val"),offsetof(ParameterValue,val)} /* 240 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(FUInstance,name)} /* 241 */,
    (Member){GetTypeOrFail(STRING("Array<ParameterValue>")),STRING("parameterValues"),offsetof(FUInstance,parameterValues)} /* 242 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel"),offsetof(FUInstance,accel)} /* 243 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("declaration"),offsetof(FUInstance,declaration)} /* 244 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(FUInstance,id)} /* 245 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("literal"),offsetof(FUInstance,literal)} /* 246 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bufferAmount"),offsetof(FUInstance,bufferAmount)} /* 247 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("portIndex"),offsetof(FUInstance,portIndex)} /* 248 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedIndex"),offsetof(FUInstance,sharedIndex)} /* 249 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isStatic"),offsetof(FUInstance,isStatic)} /* 250 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("sharedEnable"),offsetof(FUInstance,sharedEnable)} /* 251 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(FUInstance,isMergeMultiplexer)} /* 252 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("mergeMultiplexerId"),offsetof(FUInstance,mergeMultiplexerId)} /* 253 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("next"),offsetof(FUInstance,next)} /* 254 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("allInputs"),offsetof(FUInstance,allInputs)} /* 255 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("allOutputs"),offsetof(FUInstance,allOutputs)} /* 256 */,
    (Member){GetTypeOrFail(STRING("Array<PortInstance>")),STRING("inputs"),offsetof(FUInstance,inputs)} /* 257 */,
    (Member){GetTypeOrFail(STRING("Array<bool>")),STRING("outputs"),offsetof(FUInstance,outputs)} /* 258 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("multipleSamePortInputs"),offsetof(FUInstance,multipleSamePortInputs)} /* 259 */,
    (Member){GetTypeOrFail(STRING("NodeType")),STRING("type"),offsetof(FUInstance,type)} /* 260 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("allocated"),offsetof(Accelerator,allocated)} /* 261 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("lastAllocated"),offsetof(Accelerator,lastAllocated)} /* 262 */,
    (Member){GetTypeOrFail(STRING("DynamicArena *")),STRING("accelMemory"),offsetof(Accelerator,accelMemory)} /* 263 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(Accelerator,name)} /* 264 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(Accelerator,id)} /* 265 */,
    (Member){GetTypeOrFail(STRING("AcceleratorPurpose")),STRING("purpose"),offsetof(Accelerator,purpose)} /* 266 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMaskSize"),offsetof(MemoryAddressMask,memoryMaskSize)} /* 267 */,
    (Member){GetTypeOrFail(STRING("char[33]")),STRING("memoryMaskBuffer"),offsetof(MemoryAddressMask,memoryMaskBuffer)} /* 268 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("memoryMask"),offsetof(MemoryAddressMask,memoryMask)} /* 269 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nConfigs"),offsetof(VersatComputedValues,nConfigs)} /* 270 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configBits"),offsetof(VersatComputedValues,configBits)} /* 271 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("versatConfigs"),offsetof(VersatComputedValues,versatConfigs)} /* 272 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("versatStates"),offsetof(VersatComputedValues,versatStates)} /* 273 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nStatics"),offsetof(VersatComputedValues,nStatics)} /* 274 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBits"),offsetof(VersatComputedValues,staticBits)} /* 275 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBitsStart"),offsetof(VersatComputedValues,staticBitsStart)} /* 276 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nDelays"),offsetof(VersatComputedValues,nDelays)} /* 277 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayBits"),offsetof(VersatComputedValues,delayBits)} /* 278 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayBitsStart"),offsetof(VersatComputedValues,delayBitsStart)} /* 279 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nConfigurations"),offsetof(VersatComputedValues,nConfigurations)} /* 280 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configurationBits"),offsetof(VersatComputedValues,configurationBits)} /* 281 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configurationAddressBits"),offsetof(VersatComputedValues,configurationAddressBits)} /* 282 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nStates"),offsetof(VersatComputedValues,nStates)} /* 283 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateBits"),offsetof(VersatComputedValues,stateBits)} /* 284 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateAddressBits"),offsetof(VersatComputedValues,stateAddressBits)} /* 285 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("unitsMapped"),offsetof(VersatComputedValues,unitsMapped)} /* 286 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappedBytes"),offsetof(VersatComputedValues,memoryMappedBytes)} /* 287 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nUnitsIO"),offsetof(VersatComputedValues,nUnitsIO)} /* 288 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberConnections"),offsetof(VersatComputedValues,numberConnections)} /* 289 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(VersatComputedValues,externalMemoryInterfaces)} /* 290 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateConfigurationAddressBits"),offsetof(VersatComputedValues,stateConfigurationAddressBits)} /* 291 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryAddressBits"),offsetof(VersatComputedValues,memoryAddressBits)} /* 292 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappingAddressBits"),offsetof(VersatComputedValues,memoryMappingAddressBits)} /* 293 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryConfigDecisionBit"),offsetof(VersatComputedValues,memoryConfigDecisionBit)} /* 294 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(VersatComputedValues,signalLoop)} /* 295 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("sinks"),offsetof(DAGOrderNodes,sinks)} /* 296 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("sources"),offsetof(DAGOrderNodes,sources)} /* 297 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("computeUnits"),offsetof(DAGOrderNodes,computeUnits)} /* 298 */,
    (Member){GetTypeOrFail(STRING("Array<FUInstance *>")),STRING("instances"),offsetof(DAGOrderNodes,instances)} /* 299 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("order"),offsetof(DAGOrderNodes,order)} /* 300 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("size"),offsetof(DAGOrderNodes,size)} /* 301 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("maxOrder"),offsetof(DAGOrderNodes,maxOrder)} /* 302 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("firstId"),offsetof(AcceleratorMapping,firstId)} /* 303 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("secondId"),offsetof(AcceleratorMapping,secondId)} /* 304 */,
    (Member){GetTypeOrFail(STRING("TrieMap<PortInstance, PortInstance> *")),STRING("inputMap"),offsetof(AcceleratorMapping,inputMap)} /* 305 */,
    (Member){GetTypeOrFail(STRING("TrieMap<PortInstance, PortInstance> *")),STRING("outputMap"),offsetof(AcceleratorMapping,outputMap)} /* 306 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("currentNode"),offsetof(EdgeIterator,currentNode)} /* 307 */,
    (Member){GetTypeOrFail(STRING("ConnectionNode *")),STRING("currentPort"),offsetof(EdgeIterator,currentPort)} /* 308 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(StaticId,parent)} /* 309 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(StaticId,name)} /* 310 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(StaticData,configs)} /* 311 */,
    (Member){GetTypeOrFail(STRING("StaticId")),STRING("id"),offsetof(StaticInfo,id)} /* 312 */,
    (Member){GetTypeOrFail(STRING("StaticData")),STRING("data"),offsetof(StaticInfo,data)} /* 313 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("offsets"),offsetof(CalculatedOffsets,offsets)} /* 314 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(CalculatedOffsets,max)} /* 315 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("subDeclaration"),offsetof(SubMappingInfo,subDeclaration)} /* 316 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("higherName"),offsetof(SubMappingInfo,higherName)} /* 317 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isInput"),offsetof(SubMappingInfo,isInput)} /* 318 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("subPort"),offsetof(SubMappingInfo,subPort)} /* 319 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("level"),offsetof(InstanceInfo,level)} /* 320 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("decl"),offsetof(InstanceInfo,decl)} /* 321 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(InstanceInfo,name)} /* 322 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(InstanceInfo,baseName)} /* 323 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("configPos"),offsetof(InstanceInfo,configPos)} /* 324 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("isConfigStatic"),offsetof(InstanceInfo,isConfigStatic)} /* 325 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configSize"),offsetof(InstanceInfo,configSize)} /* 326 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("statePos"),offsetof(InstanceInfo,statePos)} /* 327 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateSize"),offsetof(InstanceInfo,stateSize)} /* 328 */,
    (Member){GetTypeOrFail(STRING("Opt<iptr>")),STRING("memMapped"),offsetof(InstanceInfo,memMapped)} /* 329 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memMappedSize"),offsetof(InstanceInfo,memMappedSize)} /* 330 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memMappedBitSize"),offsetof(InstanceInfo,memMappedBitSize)} /* 331 */,
    (Member){GetTypeOrFail(STRING("Opt<String>")),STRING("memMappedMask"),offsetof(InstanceInfo,memMappedMask)} /* 332 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("delayPos"),offsetof(InstanceInfo,delayPos)} /* 333 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("delay"),offsetof(InstanceInfo,delay)} /* 334 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("baseDelay"),offsetof(InstanceInfo,baseDelay)} /* 335 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delaySize"),offsetof(InstanceInfo,delaySize)} /* 336 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isComposite"),offsetof(InstanceInfo,isComposite)} /* 337 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isStatic"),offsetof(InstanceInfo,isStatic)} /* 338 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isShared"),offsetof(InstanceInfo,isShared)} /* 339 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedIndex"),offsetof(InstanceInfo,sharedIndex)} /* 340 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceInfo,parent)} /* 341 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("fullName"),offsetof(InstanceInfo,fullName)} /* 342 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(InstanceInfo,isMergeMultiplexer)} /* 343 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("mergeMultiplexerId"),offsetof(InstanceInfo,mergeMultiplexerId)} /* 344 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("belongs"),offsetof(InstanceInfo,belongs)} /* 345 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("special"),offsetof(InstanceInfo,special)} /* 346 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(InstanceInfo,order)} /* 347 */,
    (Member){GetTypeOrFail(STRING("NodeType")),STRING("connectionType"),offsetof(InstanceInfo,connectionType)} /* 348 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("id"),offsetof(InstanceInfo,id)} /* 349 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(AcceleratorInfo,info)} /* 350 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memSize"),offsetof(AcceleratorInfo,memSize)} /* 351 */,
    (Member){GetTypeOrFail(STRING("Opt<String>")),STRING("name"),offsetof(AcceleratorInfo,name)} /* 352 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("mergeMux"),offsetof(AcceleratorInfo,mergeMux)} /* 353 */,
    (Member){GetTypeOrFail(STRING("Hashmap<StaticId, int> *")),STRING("staticInfo"),offsetof(InstanceConfigurationOffsets,staticInfo)} /* 354 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceConfigurationOffsets,parent)} /* 355 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("topName"),offsetof(InstanceConfigurationOffsets,topName)} /* 356 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("baseName"),offsetof(InstanceConfigurationOffsets,baseName)} /* 357 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configOffset"),offsetof(InstanceConfigurationOffsets,configOffset)} /* 358 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("stateOffset"),offsetof(InstanceConfigurationOffsets,stateOffset)} /* 359 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delayOffset"),offsetof(InstanceConfigurationOffsets,delayOffset)} /* 360 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delay"),offsetof(InstanceConfigurationOffsets,delay)} /* 361 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memOffset"),offsetof(InstanceConfigurationOffsets,memOffset)} /* 362 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("level"),offsetof(InstanceConfigurationOffsets,level)} /* 363 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(InstanceConfigurationOffsets,order)} /* 364 */,
    (Member){GetTypeOrFail(STRING("int *")),STRING("staticConfig"),offsetof(InstanceConfigurationOffsets,staticConfig)} /* 365 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("belongs"),offsetof(InstanceConfigurationOffsets,belongs)} /* 366 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(TestResult,info)} /* 367 */,
    (Member){GetTypeOrFail(STRING("InstanceConfigurationOffsets")),STRING("subOffsets"),offsetof(TestResult,subOffsets)} /* 368 */,
    (Member){GetTypeOrFail(STRING("Array<PortInstance>")),STRING("multiplexersPorts"),offsetof(TestResult,multiplexersPorts)} /* 369 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TestResult,name)} /* 370 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("muxConfigs"),offsetof(TestResult,muxConfigs)} /* 371 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("inputDelay"),offsetof(TestResult,inputDelay)} /* 372 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("outputLatencies"),offsetof(TestResult,outputLatencies)} /* 373 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceInfo>")),STRING("baseInfo"),offsetof(AccelInfo,baseInfo)} /* 374 */,
    (Member){GetTypeOrFail(STRING("Array<Array<InstanceInfo>>")),STRING("infos"),offsetof(AccelInfo,infos)} /* 375 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("names"),offsetof(AccelInfo,names)} /* 376 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("inputDelays"),offsetof(AccelInfo,inputDelays)} /* 377 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("outputDelays"),offsetof(AccelInfo,outputDelays)} /* 378 */,
    (Member){GetTypeOrFail(STRING("Array<Array<int>>")),STRING("muxConfigs"),offsetof(AccelInfo,muxConfigs)} /* 379 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memMappedBitsize"),offsetof(AccelInfo,memMappedBitsize)} /* 380 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("howManyMergedUnits"),offsetof(AccelInfo,howManyMergedUnits)} /* 381 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("inputs"),offsetof(AccelInfo,inputs)} /* 382 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("outputs"),offsetof(AccelInfo,outputs)} /* 383 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configs"),offsetof(AccelInfo,configs)} /* 384 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("states"),offsetof(AccelInfo,states)} /* 385 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("delays"),offsetof(AccelInfo,delays)} /* 386 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("ios"),offsetof(AccelInfo,ios)} /* 387 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("statics"),offsetof(AccelInfo,statics)} /* 388 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("staticBits"),offsetof(AccelInfo,staticBits)} /* 389 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("sharedUnits"),offsetof(AccelInfo,sharedUnits)} /* 390 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(AccelInfo,externalMemoryInterfaces)} /* 391 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("externalMemoryByteSize"),offsetof(AccelInfo,externalMemoryByteSize)} /* 392 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberUnits"),offsetof(AccelInfo,numberUnits)} /* 393 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("numberConnections"),offsetof(AccelInfo,numberConnections)} /* 394 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("memoryMappedBits"),offsetof(AccelInfo,memoryMappedBits)} /* 395 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isMemoryMapped"),offsetof(AccelInfo,isMemoryMapped)} /* 396 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(AccelInfo,signalLoop)} /* 397 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("type"),offsetof(TypeAndNameOnly,type)} /* 398 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TypeAndNameOnly,name)} /* 399 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("value"),offsetof(Partition,value)} /* 400 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(Partition,max)} /* 401 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(OrderedConfigurations,configs)} /* 402 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("statics"),offsetof(OrderedConfigurations,statics)} /* 403 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("delays"),offsetof(OrderedConfigurations,delays)} /* 404 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(GraphPrintingNodeInfo,name)} /* 405 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphPrintingNodeInfo,content)} /* 406 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphPrintingNodeInfo,color)} /* 407 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphPrintingEdgeInfo,content)} /* 408 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphPrintingEdgeInfo,color)} /* 409 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("first"),offsetof(GraphPrintingEdgeInfo,first)} /* 410 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("second"),offsetof(GraphPrintingEdgeInfo,second)} /* 411 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("graphLabel"),offsetof(GraphPrintingContent,graphLabel)} /* 412 */,
    (Member){GetTypeOrFail(STRING("Array<GraphPrintingNodeInfo>")),STRING("nodes"),offsetof(GraphPrintingContent,nodes)} /* 413 */,
    (Member){GetTypeOrFail(STRING("Array<GraphPrintingEdgeInfo>")),STRING("edges"),offsetof(GraphPrintingContent,edges)} /* 414 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("content"),offsetof(GraphInfo,content)} /* 415 */,
    (Member){GetTypeOrFail(STRING("Color")),STRING("color"),offsetof(GraphInfo,color)} /* 416 */,
    (Member){GetTypeOrFail(STRING("EdgeDelay *")),STRING("edgesDelay"),offsetof(CalculateDelayResult,edgesDelay)} /* 417 */,
    (Member){GetTypeOrFail(STRING("PortDelay *")),STRING("portDelay"),offsetof(CalculateDelayResult,portDelay)} /* 418 */,
    (Member){GetTypeOrFail(STRING("NodeDelay *")),STRING("nodeDelay"),offsetof(CalculateDelayResult,nodeDelay)} /* 419 */,
    (Member){GetTypeOrFail(STRING("Edge")),STRING("edge"),offsetof(DelayToAdd,edge)} /* 420 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("bufferName"),offsetof(DelayToAdd,bufferName)} /* 421 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("bufferParameters"),offsetof(DelayToAdd,bufferParameters)} /* 422 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("bufferAmount"),offsetof(DelayToAdd,bufferAmount)} /* 423 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(ConfigurationInfo,name)} /* 424 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("baseName"),offsetof(ConfigurationInfo,baseName)} /* 425 */,
    (Member){GetTypeOrFail(STRING("FUDeclaration *")),STRING("baseType"),offsetof(ConfigurationInfo,baseType)} /* 426 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("configs"),offsetof(ConfigurationInfo,configs)} /* 427 */,
    (Member){GetTypeOrFail(STRING("Array<Wire>")),STRING("states"),offsetof(ConfigurationInfo,states)} /* 428 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("inputDelays"),offsetof(ConfigurationInfo,inputDelays)} /* 429 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ConfigurationInfo,outputLatencies)} /* 430 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("configOffsets"),offsetof(ConfigurationInfo,configOffsets)} /* 431 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("stateOffsets"),offsetof(ConfigurationInfo,stateOffsets)} /* 432 */,
    (Member){GetTypeOrFail(STRING("CalculatedOffsets")),STRING("delayOffsets"),offsetof(ConfigurationInfo,delayOffsets)} /* 433 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("calculatedDelays"),offsetof(ConfigurationInfo,calculatedDelays)} /* 434 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("order"),offsetof(ConfigurationInfo,order)} /* 435 */,
    (Member){GetTypeOrFail(STRING("AcceleratorMapping *")),STRING("mapping"),offsetof(ConfigurationInfo,mapping)} /* 436 */,
    (Member){GetTypeOrFail(STRING("Set<PortInstance> *")),STRING("mergeMultiplexers"),offsetof(ConfigurationInfo,mergeMultiplexers)} /* 437 */,
    (Member){GetTypeOrFail(STRING("Array<bool>")),STRING("unitBelongs"),offsetof(ConfigurationInfo,unitBelongs)} /* 438 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("mergeMultiplexerConfigs"),offsetof(ConfigurationInfo,mergeMultiplexerConfigs)} /* 439 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(FUDeclaration,name)} /* 440 */,
    (Member){GetTypeOrFail(STRING("ConfigurationInfo")),STRING("baseConfig"),offsetof(FUDeclaration,baseConfig)} /* 441 */,
    (Member){GetTypeOrFail(STRING("Array<ConfigurationInfo>")),STRING("configInfo"),offsetof(FUDeclaration,configInfo)} /* 442 */,
    (Member){GetTypeOrFail(STRING("Array<String>")),STRING("parameters"),offsetof(FUDeclaration,parameters)} /* 443 */,
    (Member){GetTypeOrFail(STRING("Opt<int>")),STRING("memoryMapBits"),offsetof(FUDeclaration,memoryMapBits)} /* 444 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("nIOs"),offsetof(FUDeclaration,nIOs)} /* 445 */,
    (Member){GetTypeOrFail(STRING("Array<ExternalMemoryInterfaceExpression>")),STRING("externalExpressionMemory"),offsetof(FUDeclaration,externalExpressionMemory)} /* 446 */,
    (Member){GetTypeOrFail(STRING("Array<ExternalMemoryInterface>")),STRING("externalMemory"),offsetof(FUDeclaration,externalMemory)} /* 447 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("baseCircuit"),offsetof(FUDeclaration,baseCircuit)} /* 448 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("fixedDelayCircuit"),offsetof(FUDeclaration,fixedDelayCircuit)} /* 449 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("flattenedBaseCircuit"),offsetof(FUDeclaration,flattenedBaseCircuit)} /* 450 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("operation"),offsetof(FUDeclaration,operation)} /* 451 */,
    (Member){GetTypeOrFail(STRING("SubMap *")),STRING("flattenMapping"),offsetof(FUDeclaration,flattenMapping)} /* 452 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("lat"),offsetof(FUDeclaration,lat)} /* 453 */,
    (Member){GetTypeOrFail(STRING("Hashmap<StaticId, StaticData> *")),STRING("staticUnits"),offsetof(FUDeclaration,staticUnits)} /* 454 */,
    (Member){GetTypeOrFail(STRING("FUDeclarationType")),STRING("type"),offsetof(FUDeclaration,type)} /* 455 */,
    (Member){GetTypeOrFail(STRING("DelayType")),STRING("delayType"),offsetof(FUDeclaration,delayType)} /* 456 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isOperation"),offsetof(FUDeclaration,isOperation)} /* 457 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("implementsDone"),offsetof(FUDeclaration,implementsDone)} /* 458 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("signalLoop"),offsetof(FUDeclaration,signalLoop)} /* 459 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("type"),offsetof(SingleTypeStructElement,type)} /* 460 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(SingleTypeStructElement,name)} /* 461 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("arraySize"),offsetof(SingleTypeStructElement,arraySize)} /* 462 */,
    (Member){GetTypeOrFail(STRING("Array<SingleTypeStructElement>")),STRING("typeAndNames"),offsetof(TypeStructInfoElement,typeAndNames)} /* 463 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(TypeStructInfo,name)} /* 464 */,
    (Member){GetTypeOrFail(STRING("Array<TypeStructInfoElement>")),STRING("entries"),offsetof(TypeStructInfo,entries)} /* 465 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(Difference,index)} /* 466 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("newValue"),offsetof(Difference,newValue)} /* 467 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("oldIndex"),offsetof(DifferenceArray,oldIndex)} /* 468 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("newIndex"),offsetof(DifferenceArray,newIndex)} /* 469 */,
    (Member){GetTypeOrFail(STRING("Array<Difference>")),STRING("differences"),offsetof(DifferenceArray,differences)} /* 470 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configIndex"),offsetof(MuxInfo,configIndex)} /* 471 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("val"),offsetof(MuxInfo,val)} /* 472 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("name"),offsetof(MuxInfo,name)} /* 473 */,
    (Member){GetTypeOrFail(STRING("InstanceInfo *")),STRING("info"),offsetof(MuxInfo,info)} /* 474 */,
    (Member){GetTypeOrFail(STRING("Wire")),STRING("wire"),offsetof(WireInformation,wire)} /* 475 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("addr"),offsetof(WireInformation,addr)} /* 476 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("configBitStart"),offsetof(WireInformation,configBitStart)} /* 477 */,
    (Member){GetTypeOrFail(STRING("TaskFunction")),STRING("function"),offsetof(Task,function)} /* 478 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(Task,order)} /* 479 */,
    (Member){GetTypeOrFail(STRING("void *")),STRING("args"),offsetof(Task,args)} /* 480 */,
    (Member){GetTypeOrFail(STRING("TaskFunction")),STRING("function"),offsetof(WorkGroup,function)} /* 481 */,
    (Member){GetTypeOrFail(STRING("Array<Task>")),STRING("tasks"),offsetof(WorkGroup,tasks)} /* 482 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("instA"),offsetof(SpecificMerge,instA)} /* 483 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("instB"),offsetof(SpecificMerge,instB)} /* 484 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("index"),offsetof(IndexRecord,index)} /* 485 */,
    (Member){GetTypeOrFail(STRING("IndexRecord *")),STRING("next"),offsetof(IndexRecord,next)} /* 486 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("instA"),offsetof(SpecificMergeNodes,instA)} /* 487 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("instB"),offsetof(SpecificMergeNodes,instB)} /* 488 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("firstIndex"),offsetof(SpecificMergeNode,firstIndex)} /* 489 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("firstName"),offsetof(SpecificMergeNode,firstName)} /* 490 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("secondIndex"),offsetof(SpecificMergeNode,secondIndex)} /* 491 */,
    (Member){GetTypeOrFail(STRING("String")),STRING("secondName"),offsetof(SpecificMergeNode,secondName)} /* 492 */,
    (Member){GetTypeOrFail(STRING("FUInstance *[2]")),STRING("instances"),offsetof(MergeEdge,instances)} /* 493 */,
    (Member){GetTypeOrFail(STRING("MergeEdge")),STRING("nodes"),offsetof(MappingNode,nodes)} /* 494 */,
    (Member){GetTypeOrFail(STRING("Edge[2]")),STRING("edges"),offsetof(MappingNode,edges)} /* 495 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(MappingNode,type)} /* 496 */,
    (Member){GetTypeOrFail(STRING("Array<SpecificMergeNodes>")),STRING("specifics"),offsetof(ConsolidationGraphOptions,specifics)} /* 497 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("order"),offsetof(ConsolidationGraphOptions,order)} /* 498 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("difference"),offsetof(ConsolidationGraphOptions,difference)} /* 499 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("mapNodes"),offsetof(ConsolidationGraphOptions,mapNodes)} /* 500 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(ConsolidationGraphOptions,type)} /* 501 */,
    (Member){GetTypeOrFail(STRING("Array<MappingNode>")),STRING("nodes"),offsetof(ConsolidationGraph,nodes)} /* 502 */,
    (Member){GetTypeOrFail(STRING("Array<BitArray>")),STRING("edges"),offsetof(ConsolidationGraph,edges)} /* 503 */,
    (Member){GetTypeOrFail(STRING("BitArray")),STRING("validNodes"),offsetof(ConsolidationGraph,validNodes)} /* 504 */,
    (Member){GetTypeOrFail(STRING("ConsolidationGraph")),STRING("graph"),offsetof(ConsolidationResult,graph)} /* 505 */,
    (Member){GetTypeOrFail(STRING("Pool<MappingNode>")),STRING("specificsAdded"),offsetof(ConsolidationResult,specificsAdded)} /* 506 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("upperBound"),offsetof(ConsolidationResult,upperBound)} /* 507 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("max"),offsetof(CliqueState,max)} /* 508 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("upperBound"),offsetof(CliqueState,upperBound)} /* 509 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("startI"),offsetof(CliqueState,startI)} /* 510 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("iterations"),offsetof(CliqueState,iterations)} /* 511 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("table"),offsetof(CliqueState,table)} /* 512 */,
    (Member){GetTypeOrFail(STRING("ConsolidationGraph")),STRING("clique"),offsetof(CliqueState,clique)} /* 513 */,
    (Member){GetTypeOrFail(STRING("Time")),STRING("start"),offsetof(CliqueState,start)} /* 514 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("found"),offsetof(CliqueState,found)} /* 515 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("result"),offsetof(IsCliqueResult,result)} /* 516 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("failedIndex"),offsetof(IsCliqueResult,failedIndex)} /* 517 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel1"),offsetof(MergeGraphResult,accel1)} /* 518 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResult,accel2)} /* 519 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("map1"),offsetof(MergeGraphResult,map1)} /* 520 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("map2"),offsetof(MergeGraphResult,map2)} /* 521 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("newGraph"),offsetof(MergeGraphResult,newGraph)} /* 522 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("result"),offsetof(MergeGraphResultExisting,result)} /* 523 */,
    (Member){GetTypeOrFail(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResultExisting,accel2)} /* 524 */,
    (Member){GetTypeOrFail(STRING("AcceleratorMapping *")),STRING("map2"),offsetof(MergeGraphResultExisting,map2)} /* 525 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("instanceMap"),offsetof(GraphMapping,instanceMap)} /* 526 */,
    (Member){GetTypeOrFail(STRING("InstanceMap *")),STRING("reverseInstanceMap"),offsetof(GraphMapping,reverseInstanceMap)} /* 527 */,
    (Member){GetTypeOrFail(STRING("EdgeMap *")),STRING("edgeMap"),offsetof(GraphMapping,edgeMap)} /* 528 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("port"),offsetof(ConnectionExtra,port)} /* 529 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("delay"),offsetof(ConnectionExtra,delay)} /* 530 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(Var,name)} /* 531 */,
    (Member){GetTypeOrFail(STRING("ConnectionExtra")),STRING("extra"),offsetof(Var,extra)} /* 532 */,
    (Member){GetTypeOrFail(STRING("Range<int>")),STRING("index"),offsetof(Var,index)} /* 533 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isArrayAccess"),offsetof(Var,isArrayAccess)} /* 534 */,
    (Member){GetTypeOrFail(STRING("Array<Var>")),STRING("vars"),offsetof(VarGroup,vars)} /* 535 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("fullText"),offsetof(VarGroup,fullText)} /* 536 */,
    (Member){GetTypeOrFail(STRING("Array<SpecExpression *>")),STRING("expressions"),offsetof(SpecExpression,expressions)} /* 537 */,
    (Member){GetTypeOrFail(STRING("char *")),STRING("op"),offsetof(SpecExpression,op)} /* 538 */,
    (Member){GetTypeOrFail(STRING("Var")),STRING("var"),offsetof(SpecExpression,var)} /* 539 */,
    (Member){GetTypeOrFail(STRING("Value")),STRING("val"),offsetof(SpecExpression,val)} /* 540 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("text"),offsetof(SpecExpression,text)} /* 541 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(SpecExpression,type)} /* 542 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(VarDeclaration,name)} /* 543 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("arraySize"),offsetof(VarDeclaration,arraySize)} /* 544 */,
    (Member){GetTypeOrFail(STRING("bool")),STRING("isArray"),offsetof(VarDeclaration,isArray)} /* 545 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("group"),offsetof(GroupIterator,group)} /* 546 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("groupIndex"),offsetof(GroupIterator,groupIndex)} /* 547 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("varIndex"),offsetof(GroupIterator,varIndex)} /* 548 */,
    (Member){GetTypeOrFail(STRING("FUInstance *")),STRING("inst"),offsetof(PortExpression,inst)} /* 549 */,
    (Member){GetTypeOrFail(STRING("ConnectionExtra")),STRING("extra"),offsetof(PortExpression,extra)} /* 550 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("modifier"),offsetof(InstanceDeclaration,modifier)} /* 551 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("typeName"),offsetof(InstanceDeclaration,typeName)} /* 552 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("declarations"),offsetof(InstanceDeclaration,declarations)} /* 553 */,
    (Member){GetTypeOrFail(STRING("Array<Pair<String, String>>")),STRING("parameters"),offsetof(InstanceDeclaration,parameters)} /* 554 */,
    (Member){GetTypeOrFail(STRING("Range<Cursor>")),STRING("loc"),offsetof(ConnectionDef,loc)} /* 555 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("output"),offsetof(ConnectionDef,output)} /* 556 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("type"),offsetof(ConnectionDef,type)} /* 557 */,
    (Member){GetTypeOrFail(STRING("Array<Token>")),STRING("transforms"),offsetof(ConnectionDef,transforms)} /* 558 */,
    (Member){GetTypeOrFail(STRING("VarGroup")),STRING("input"),offsetof(ConnectionDef,input)} /* 559 */,
    (Member){GetTypeOrFail(STRING("SpecExpression *")),STRING("expression"),offsetof(ConnectionDef,expression)} /* 560 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("typeName"),offsetof(TypeAndInstance,typeName)} /* 561 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("instanceName"),offsetof(TypeAndInstance,instanceName)} /* 562 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(DefBase,name)} /* 563 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(ModuleDef,name)} /* 564 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("numberOutputs"),offsetof(ModuleDef,numberOutputs)} /* 565 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(ModuleDef,inputs)} /* 566 */,
    (Member){GetTypeOrFail(STRING("Array<InstanceDeclaration>")),STRING("declarations"),offsetof(ModuleDef,declarations)} /* 567 */,
    (Member){GetTypeOrFail(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(ModuleDef,connections)} /* 568 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(TransformDef,name)} /* 569 */,
    (Member){GetTypeOrFail(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(TransformDef,inputs)} /* 570 */,
    (Member){GetTypeOrFail(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(TransformDef,connections)} /* 571 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("name"),offsetof(MergeDef,name)} /* 572 */,
    (Member){GetTypeOrFail(STRING("Array<TypeAndInstance>")),STRING("declarations"),offsetof(MergeDef,declarations)} /* 573 */,
    (Member){GetTypeOrFail(STRING("Array<SpecificMergeNode>")),STRING("specifics"),offsetof(MergeDef,specifics)} /* 574 */,
    (Member){GetTypeOrFail(STRING("DefinitionType")),STRING("type"),offsetof(TypeDefinition,type)} /* 575 */,
    (Member){GetTypeOrFail(STRING("DefBase")),STRING("base"),offsetof(TypeDefinition,base)} /* 576 */,
    (Member){GetTypeOrFail(STRING("ModuleDef")),STRING("module"),offsetof(TypeDefinition,module)} /* 577 */,
    (Member){GetTypeOrFail(STRING("MergeDef")),STRING("merge"),offsetof(TypeDefinition,merge)} /* 578 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("inputs"),offsetof(Transformation,inputs)} /* 579 */,
    (Member){GetTypeOrFail(STRING("int")),STRING("outputs"),offsetof(Transformation,outputs)} /* 580 */,
    (Member){GetTypeOrFail(STRING("Array<int>")),STRING("map"),offsetof(Transformation,map)} /* 581 */,
    (Member){GetTypeOrFail(STRING("Token")),STRING("instanceName"),offsetof(HierarchicalName,instanceName)} /* 582 */,
    (Member){GetTypeOrFail(STRING("Var")),STRING("subInstance"),offsetof(HierarchicalName,subInstance)} /* 583 */
  };

  RegisterStructMembers(STRING("Time"),(Array<Member>){&members[0],2});
  RegisterStructMembers(STRING("TimeIt"),(Array<Member>){&members[2],3});
  RegisterStructMembers(STRING("Conversion"),(Array<Member>){&members[5],3});
  RegisterStructMembers(STRING("Arena"),(Array<Member>){&members[8],4});
  RegisterStructMembers(STRING("ArenaMark"),(Array<Member>){&members[12],2});
  RegisterStructMembers(STRING("ArenaMarker"),(Array<Member>){&members[14],1});
  RegisterStructMembers(STRING("DynamicArena"),(Array<Member>){&members[15],4});
  RegisterStructMembers(STRING("DynamicString"),(Array<Member>){&members[19],2});
  RegisterStructMembers(STRING("BitIterator"),(Array<Member>){&members[21],3});
  RegisterStructMembers(STRING("BitArray"),(Array<Member>){&members[24],3});
  RegisterStructMembers(STRING("PoolHeader"),(Array<Member>){&members[27],2});
  RegisterStructMembers(STRING("PoolInfo"),(Array<Member>){&members[29],4});
  RegisterStructMembers(STRING("PageInfo"),(Array<Member>){&members[33],2});
  RegisterStructMembers(STRING("GenericPoolIterator"),(Array<Member>){&members[35],7});
  RegisterStructMembers(STRING("EnumMember"),(Array<Member>){&members[42],3});
  RegisterStructMembers(STRING("TemplateArg"),(Array<Member>){&members[45],2});
  RegisterStructMembers(STRING("TemplatedMember"),(Array<Member>){&members[47],3});
  RegisterStructMembers(STRING("NameAndTemplateArguments"),(Array<Member>){&members[50],2});
  RegisterStructMembers(STRING("ParsedType"),(Array<Member>){&members[52],4});
  RegisterStructMembers(STRING("Type"),(Array<Member>){&members[56],14});
  RegisterStructMembers(STRING("Member"),(Array<Member>){&members[70],6});
  RegisterStructMembers(STRING("Value"),(Array<Member>){&members[76],9});
  RegisterStructMembers(STRING("Iterator"),(Array<Member>){&members[85],4});
  RegisterStructMembers(STRING("HashmapUnpackedIndex"),(Array<Member>){&members[89],2});
  RegisterStructMembers(STRING("TypeIterator"),(Array<Member>){&members[91],2});
  RegisterStructMembers(STRING("Expression"),(Array<Member>){&members[93],8});
  RegisterStructMembers(STRING("Cursor"),(Array<Member>){&members[101],2});
  RegisterStructMembers(STRING("Token"),(Array<Member>){&members[103],1});
  RegisterStructMembers(STRING("FindFirstResult"),(Array<Member>){&members[104],2});
  RegisterStructMembers(STRING("Trie"),(Array<Member>){&members[106],1});
  RegisterStructMembers(STRING("TokenizerTemplate"),(Array<Member>){&members[107],1});
  RegisterStructMembers(STRING("TokenizerMark"),(Array<Member>){&members[108],2});
  RegisterStructMembers(STRING("Tokenizer"),(Array<Member>){&members[110],9});
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
  RegisterStructMembers(STRING("ParameterValue"),(Array<Member>){&members[240],1});
  RegisterStructMembers(STRING("FUInstance"),(Array<Member>){&members[241],20});
  RegisterStructMembers(STRING("Accelerator"),(Array<Member>){&members[261],6});
  RegisterStructMembers(STRING("MemoryAddressMask"),(Array<Member>){&members[267],3});
  RegisterStructMembers(STRING("VersatComputedValues"),(Array<Member>){&members[270],26});
  RegisterStructMembers(STRING("DAGOrderNodes"),(Array<Member>){&members[296],7});
  RegisterStructMembers(STRING("AcceleratorMapping"),(Array<Member>){&members[303],4});
  RegisterStructMembers(STRING("EdgeIterator"),(Array<Member>){&members[307],2});
  RegisterStructMembers(STRING("StaticId"),(Array<Member>){&members[309],2});
  RegisterStructMembers(STRING("StaticData"),(Array<Member>){&members[311],1});
  RegisterStructMembers(STRING("StaticInfo"),(Array<Member>){&members[312],2});
  RegisterStructMembers(STRING("CalculatedOffsets"),(Array<Member>){&members[314],2});
  RegisterStructMembers(STRING("SubMappingInfo"),(Array<Member>){&members[316],4});
  RegisterStructMembers(STRING("InstanceInfo"),(Array<Member>){&members[320],30});
  RegisterStructMembers(STRING("AcceleratorInfo"),(Array<Member>){&members[350],4});
  RegisterStructMembers(STRING("InstanceConfigurationOffsets"),(Array<Member>){&members[354],13});
  RegisterStructMembers(STRING("TestResult"),(Array<Member>){&members[367],7});
  RegisterStructMembers(STRING("AccelInfo"),(Array<Member>){&members[374],24});
  RegisterStructMembers(STRING("TypeAndNameOnly"),(Array<Member>){&members[398],2});
  RegisterStructMembers(STRING("Partition"),(Array<Member>){&members[400],2});
  RegisterStructMembers(STRING("OrderedConfigurations"),(Array<Member>){&members[402],3});
  RegisterStructMembers(STRING("GraphPrintingNodeInfo"),(Array<Member>){&members[405],3});
  RegisterStructMembers(STRING("GraphPrintingEdgeInfo"),(Array<Member>){&members[408],4});
  RegisterStructMembers(STRING("GraphPrintingContent"),(Array<Member>){&members[412],3});
  RegisterStructMembers(STRING("GraphInfo"),(Array<Member>){&members[415],2});
  RegisterStructMembers(STRING("CalculateDelayResult"),(Array<Member>){&members[417],3});
  RegisterStructMembers(STRING("DelayToAdd"),(Array<Member>){&members[420],4});
  RegisterStructMembers(STRING("ConfigurationInfo"),(Array<Member>){&members[424],16});
  RegisterStructMembers(STRING("FUDeclaration"),(Array<Member>){&members[440],20});
  RegisterStructMembers(STRING("SingleTypeStructElement"),(Array<Member>){&members[460],3});
  RegisterStructMembers(STRING("TypeStructInfoElement"),(Array<Member>){&members[463],1});
  RegisterStructMembers(STRING("TypeStructInfo"),(Array<Member>){&members[464],2});
  RegisterStructMembers(STRING("Difference"),(Array<Member>){&members[466],2});
  RegisterStructMembers(STRING("DifferenceArray"),(Array<Member>){&members[468],3});
  RegisterStructMembers(STRING("MuxInfo"),(Array<Member>){&members[471],4});
  RegisterStructMembers(STRING("WireInformation"),(Array<Member>){&members[475],3});
  RegisterStructMembers(STRING("Task"),(Array<Member>){&members[478],3});
  RegisterStructMembers(STRING("WorkGroup"),(Array<Member>){&members[481],2});
  RegisterStructMembers(STRING("SpecificMerge"),(Array<Member>){&members[483],2});
  RegisterStructMembers(STRING("IndexRecord"),(Array<Member>){&members[485],2});
  RegisterStructMembers(STRING("SpecificMergeNodes"),(Array<Member>){&members[487],2});
  RegisterStructMembers(STRING("SpecificMergeNode"),(Array<Member>){&members[489],4});
  RegisterStructMembers(STRING("MergeEdge"),(Array<Member>){&members[493],1});
  RegisterStructMembers(STRING("MappingNode"),(Array<Member>){&members[494],3});
  RegisterStructMembers(STRING("ConsolidationGraphOptions"),(Array<Member>){&members[497],5});
  RegisterStructMembers(STRING("ConsolidationGraph"),(Array<Member>){&members[502],3});
  RegisterStructMembers(STRING("ConsolidationResult"),(Array<Member>){&members[505],3});
  RegisterStructMembers(STRING("CliqueState"),(Array<Member>){&members[508],8});
  RegisterStructMembers(STRING("IsCliqueResult"),(Array<Member>){&members[516],2});
  RegisterStructMembers(STRING("MergeGraphResult"),(Array<Member>){&members[518],5});
  RegisterStructMembers(STRING("MergeGraphResultExisting"),(Array<Member>){&members[523],3});
  RegisterStructMembers(STRING("GraphMapping"),(Array<Member>){&members[526],3});
  RegisterStructMembers(STRING("ConnectionExtra"),(Array<Member>){&members[529],2});
  RegisterStructMembers(STRING("Var"),(Array<Member>){&members[531],4});
  RegisterStructMembers(STRING("VarGroup"),(Array<Member>){&members[535],2});
  RegisterStructMembers(STRING("SpecExpression"),(Array<Member>){&members[537],6});
  RegisterStructMembers(STRING("VarDeclaration"),(Array<Member>){&members[543],3});
  RegisterStructMembers(STRING("GroupIterator"),(Array<Member>){&members[546],3});
  RegisterStructMembers(STRING("PortExpression"),(Array<Member>){&members[549],2});
  RegisterStructMembers(STRING("InstanceDeclaration"),(Array<Member>){&members[551],4});
  RegisterStructMembers(STRING("ConnectionDef"),(Array<Member>){&members[555],6});
  RegisterStructMembers(STRING("TypeAndInstance"),(Array<Member>){&members[561],2});
  RegisterStructMembers(STRING("DefBase"),(Array<Member>){&members[563],1});
  RegisterStructMembers(STRING("ModuleDef"),(Array<Member>){&members[564],5});
  RegisterStructMembers(STRING("TransformDef"),(Array<Member>){&members[569],3});
  RegisterStructMembers(STRING("MergeDef"),(Array<Member>){&members[572],3});
  RegisterStructMembers(STRING("TypeDefinition"),(Array<Member>){&members[575],4});
  RegisterStructMembers(STRING("Transformation"),(Array<Member>){&members[579],3});
  RegisterStructMembers(STRING("HierarchicalName"),(Array<Member>){&members[582],2});


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
