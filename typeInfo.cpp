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
  RegisterOpaqueType(STRING("uint8_t"),Type::UNKNOWN,sizeof(uint8_t),alignof(uint8_t));
  RegisterOpaqueType(STRING("int8_t"),Type::UNKNOWN,sizeof(int8_t),alignof(int8_t));
  RegisterOpaqueType(STRING("uint16_t"),Type::UNKNOWN,sizeof(uint16_t),alignof(uint16_t));
  RegisterOpaqueType(STRING("int16_t"),Type::UNKNOWN,sizeof(int16_t),alignof(int16_t));
  RegisterOpaqueType(STRING("uint32_t"),Type::UNKNOWN,sizeof(uint32_t),alignof(uint32_t));
  RegisterOpaqueType(STRING("int32_t"),Type::UNKNOWN,sizeof(int32_t),alignof(int32_t));
  RegisterOpaqueType(STRING("uint64_t"),Type::UNKNOWN,sizeof(uint64_t),alignof(uint64_t));
  RegisterOpaqueType(STRING("int64_t"),Type::UNKNOWN,sizeof(int64_t),alignof(int64_t));
  RegisterOpaqueType(STRING("intptr_t"),Type::UNKNOWN,sizeof(intptr_t),alignof(intptr_t));
  RegisterOpaqueType(STRING("uintptr_t"),Type::UNKNOWN,sizeof(uintptr_t),alignof(uintptr_t));
  RegisterOpaqueType(STRING("int"),Type::UNKNOWN,sizeof(int),alignof(int));
  RegisterOpaqueType(STRING("uint"),Type::UNKNOWN,sizeof(uint),alignof(uint));
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
  RegisterOpaqueType(STRING("CalculateDelayResult"),Type::STRUCT,sizeof(CalculateDelayResult),alignof(CalculateDelayResult));
  RegisterOpaqueType(STRING("FUInstance"),Type::STRUCT,sizeof(FUInstance),alignof(FUInstance));
  RegisterOpaqueType(STRING("DelayToAdd"),Type::STRUCT,sizeof(DelayToAdd),alignof(DelayToAdd));
  RegisterOpaqueType(STRING("SingleTypeStructElement"),Type::STRUCT,sizeof(SingleTypeStructElement),alignof(SingleTypeStructElement));
  RegisterOpaqueType(STRING("TypeStructInfoElement"),Type::STRUCT,sizeof(TypeStructInfoElement),alignof(TypeStructInfoElement));
  RegisterOpaqueType(STRING("TypeStructInfo"),Type::STRUCT,sizeof(TypeStructInfo),alignof(TypeStructInfo));
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
  RegisterOpaqueType(STRING("InstanceTable"),Type::TYPEDEF,sizeof(InstanceTable),alignof(InstanceTable));
  RegisterOpaqueType(STRING("InstanceName"),Type::TYPEDEF,sizeof(InstanceName),alignof(InstanceName));
  RegisterOpaqueType(STRING("SpecNode"),Type::TYPEDEF,sizeof(SpecNode),alignof(SpecNode));
  RegisterOpaqueType(STRING("String"),Type::TYPEDEF,sizeof(String),alignof(String));
  RegisterOpaqueType(STRING("Array<const char>"),Type::TEMPLATED_INSTANCE,sizeof(Array<const char>),alignof(Array<const char>));
  RegisterOpaqueType(STRING("Range<Expression *>"),Type::TEMPLATED_INSTANCE,sizeof(Range<Expression *>),alignof(Range<Expression *>));
  RegisterOpaqueType(STRING("WireTemplate<int>"),Type::TEMPLATED_INSTANCE,sizeof(WireTemplate<int>),alignof(WireTemplate<int>));
  RegisterOpaqueType(STRING("WireTemplate<ExpressionRange>"),Type::TEMPLATED_INSTANCE,sizeof(WireTemplate<ExpressionRange>),alignof(WireTemplate<ExpressionRange>));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPortsTemplate<int>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryTwoPortsTemplate<int>),alignof(ExternalMemoryTwoPortsTemplate<int>));
  RegisterOpaqueType(STRING("ExternalMemoryTwoPortsTemplate<ExpressionRange>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryTwoPortsTemplate<ExpressionRange>),alignof(ExternalMemoryTwoPortsTemplate<ExpressionRange>));
  RegisterOpaqueType(STRING("ExternalMemoryDualPortTemplate<int>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryDualPortTemplate<int>),alignof(ExternalMemoryDualPortTemplate<int>));
  RegisterOpaqueType(STRING("ExternalMemoryDualPortTemplate<ExpressionRange>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryDualPortTemplate<ExpressionRange>),alignof(ExternalMemoryDualPortTemplate<ExpressionRange>));
  RegisterOpaqueType(STRING("ExternalMemoryTemplate<int>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryTemplate<int>),alignof(ExternalMemoryTemplate<int>));
  RegisterOpaqueType(STRING("ExternalMemoryTemplate<ExpressionRange>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryTemplate<ExpressionRange>),alignof(ExternalMemoryTemplate<ExpressionRange>));
  RegisterOpaqueType(STRING("ExternalMemoryInterfaceTemplate<int>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryInterfaceTemplate<int>),alignof(ExternalMemoryInterfaceTemplate<int>));
  RegisterOpaqueType(STRING("ExternalMemoryInterfaceTemplate<ExpressionRange>"),Type::TEMPLATED_INSTANCE,sizeof(ExternalMemoryInterfaceTemplate<ExpressionRange>),alignof(ExternalMemoryInterfaceTemplate<ExpressionRange>));
  RegisterOpaqueType(STRING("Hashmap<FUInstance *, FUInstance *>"),Type::TEMPLATED_INSTANCE,sizeof(Hashmap<FUInstance *, FUInstance *>),alignof(Hashmap<FUInstance *, FUInstance *>));
  RegisterOpaqueType(STRING("Hashmap<PortEdge, PortEdge>"),Type::TEMPLATED_INSTANCE,sizeof(Hashmap<PortEdge, PortEdge>),alignof(Hashmap<PortEdge, PortEdge>));
  RegisterOpaqueType(STRING("Hashmap<Edge *, Edge *>"),Type::TEMPLATED_INSTANCE,sizeof(Hashmap<Edge *, Edge *>),alignof(Hashmap<Edge *, Edge *>));
  RegisterOpaqueType(STRING("Hashmap<InstanceNode *, InstanceNode *>"),Type::TEMPLATED_INSTANCE,sizeof(Hashmap<InstanceNode *, InstanceNode *>),alignof(Hashmap<InstanceNode *, InstanceNode *>));
  RegisterOpaqueType(STRING("Hashmap<String, FUInstance *>"),Type::TEMPLATED_INSTANCE,sizeof(Hashmap<String, FUInstance *>),alignof(Hashmap<String, FUInstance *>));
  RegisterOpaqueType(STRING("Set<String>"),Type::TEMPLATED_INSTANCE,sizeof(Set<String>),alignof(Set<String>));
  RegisterOpaqueType(STRING("Pair<HierarchicalName, HierarchicalName>"),Type::TEMPLATED_INSTANCE,sizeof(Pair<HierarchicalName, HierarchicalName>),alignof(Pair<HierarchicalName, HierarchicalName>));

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

static Pair<String,int> OrderTypeData[] = {{STRING("OrderType_SINK"),(int) OrderType::OrderType_SINK}};

RegisterEnum(STRING("OrderType"),C_ARRAY_TO_ARRAY(OrderTypeData));

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

  static String templateArgs[] = { STRING("F") /* 0 */,
    STRING("T") /* 1 */,
    STRING("T") /* 2 */,
    STRING("T") /* 3 */,
    STRING("First") /* 4 */,
    STRING("Second") /* 5 */,
    STRING("T") /* 6 */,
    STRING("T") /* 7 */,
    STRING("Key") /* 8 */,
    STRING("Data") /* 9 */,
    STRING("Data") /* 10 */,
    STRING("Key") /* 11 */,
    STRING("Data") /* 12 */,
    STRING("Data") /* 13 */,
    STRING("Data") /* 14 */,
    STRING("Key") /* 15 */,
    STRING("Data") /* 16 */,
    STRING("Key") /* 17 */,
    STRING("Data") /* 18 */,
    STRING("Key") /* 19 */,
    STRING("Data") /* 20 */,
    STRING("Data") /* 21 */,
    STRING("Data") /* 22 */,
    STRING("T") /* 23 */,
    STRING("T") /* 24 */,
    STRING("Value") /* 25 */,
    STRING("Error") /* 26 */,
    STRING("T") /* 27 */,
    STRING("T") /* 28 */,
    STRING("T") /* 29 */,
    STRING("T") /* 30 */,
    STRING("T") /* 31 */,
    STRING("T") /* 32 */,
    STRING("T") /* 33 */,
    STRING("T") /* 34 */,
    STRING("T") /* 35 */,
    STRING("T") /* 36 */
  };

  RegisterTemplate(STRING("_Defer"),(Array<String>){&templateArgs[0],1});
  RegisterTemplate(STRING("ArrayIterator"),(Array<String>){&templateArgs[1],1});
  RegisterTemplate(STRING("Array"),(Array<String>){&templateArgs[2],1});
  RegisterTemplate(STRING("Range"),(Array<String>){&templateArgs[3],1});
  RegisterTemplate(STRING("Pair"),(Array<String>){&templateArgs[4],2});
  RegisterTemplate(STRING("DynamicArray"),(Array<String>){&templateArgs[6],1});
  RegisterTemplate(STRING("PushPtr"),(Array<String>){&templateArgs[7],1});
  RegisterTemplate(STRING("HashmapIterator"),(Array<String>){&templateArgs[8],2});
  RegisterTemplate(STRING("GetOrAllocateResult"),(Array<String>){&templateArgs[10],1});
  RegisterTemplate(STRING("Hashmap"),(Array<String>){&templateArgs[11],2});
  RegisterTemplate(STRING("Set"),(Array<String>){&templateArgs[13],1});
  RegisterTemplate(STRING("SetIterator"),(Array<String>){&templateArgs[14],1});
  RegisterTemplate(STRING("TrieMapNode"),(Array<String>){&templateArgs[15],2});
  RegisterTemplate(STRING("TrieMapIterator"),(Array<String>){&templateArgs[17],2});
  RegisterTemplate(STRING("TrieMap"),(Array<String>){&templateArgs[19],2});
  RegisterTemplate(STRING("TrieSetIterator"),(Array<String>){&templateArgs[21],1});
  RegisterTemplate(STRING("TrieSet"),(Array<String>){&templateArgs[22],1});
  RegisterTemplate(STRING("PoolIterator"),(Array<String>){&templateArgs[23],1});
  RegisterTemplate(STRING("Pool"),(Array<String>){&templateArgs[24],1});
  RegisterTemplate(STRING("Result"),(Array<String>){&templateArgs[25],2});
  RegisterTemplate(STRING("IndexedStruct"),(Array<String>){&templateArgs[27],1});
  RegisterTemplate(STRING("ListedStruct"),(Array<String>){&templateArgs[28],1});
  RegisterTemplate(STRING("ArenaList"),(Array<String>){&templateArgs[29],1});
  RegisterTemplate(STRING("Stack"),(Array<String>){&templateArgs[30],1});
  RegisterTemplate(STRING("WireTemplate"),(Array<String>){&templateArgs[31],1});
  RegisterTemplate(STRING("ExternalMemoryTwoPortsTemplate"),(Array<String>){&templateArgs[32],1});
  RegisterTemplate(STRING("ExternalMemoryDualPortTemplate"),(Array<String>){&templateArgs[33],1});
  RegisterTemplate(STRING("ExternalMemoryTemplate"),(Array<String>){&templateArgs[34],1});
  RegisterTemplate(STRING("ExternalMemoryInterfaceTemplate"),(Array<String>){&templateArgs[35],1});
  RegisterTemplate(STRING("std::vector"),(Array<String>){&templateArgs[36],1});

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
  RegisterTypedef(STRING("int"),STRING("ValueMap"));
  RegisterTypedef(STRING("int"),STRING("MacroMap"));
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
  RegisterTypedef(STRING("uint"),STRING("GraphDotFormat"));
  RegisterTypedef(STRING("Hashmap<String, FUInstance *>"),STRING("InstanceTable"));
  RegisterTypedef(STRING("Set<String>"),STRING("InstanceName"));
  RegisterTypedef(STRING("Pair<HierarchicalName, HierarchicalName>"),STRING("SpecNode"));

  static TemplatedMember templateMembers[] = { (TemplatedMember){STRING("F"),STRING("f"),0} /* 0 */,
    (TemplatedMember){STRING("T *"),STRING("ptr"),0} /* 1 */,
    (TemplatedMember){STRING("T *"),STRING("data"),0} /* 2 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 3 */,
    (TemplatedMember){STRING("T"),STRING("high"),0} /* 4 */,
    (TemplatedMember){STRING("T"),STRING("start"),0} /* 5 */,
    (TemplatedMember){STRING("T"),STRING("top"),0} /* 6 */,
    (TemplatedMember){STRING("T"),STRING("low"),1} /* 7 */,
    (TemplatedMember){STRING("T"),STRING("end"),1} /* 8 */,
    (TemplatedMember){STRING("T"),STRING("bottom"),1} /* 9 */,
    (TemplatedMember){STRING("First"),STRING("key"),0} /* 10 */,
    (TemplatedMember){STRING("First"),STRING("first"),0} /* 11 */,
    (TemplatedMember){STRING("Second"),STRING("data"),1} /* 12 */,
    (TemplatedMember){STRING("Second"),STRING("second"),1} /* 13 */,
    (TemplatedMember){STRING("ArenaMark"),STRING("mark"),0} /* 14 */,
    (TemplatedMember){STRING("T *"),STRING("ptr"),0} /* 15 */,
    (TemplatedMember){STRING("int"),STRING("maximumTimes"),1} /* 16 */,
    (TemplatedMember){STRING("int"),STRING("timesPushed"),2} /* 17 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("pairs"),0} /* 18 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 19 */,
    (TemplatedMember){STRING("Data *"),STRING("data"),0} /* 20 */,
    (TemplatedMember){STRING("bool"),STRING("alreadyExisted"),1} /* 21 */,
    (TemplatedMember){STRING("int"),STRING("nodesAllocated"),0} /* 22 */,
    (TemplatedMember){STRING("int"),STRING("nodesUsed"),1} /* 23 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("buckets"),2} /* 24 */,
    (TemplatedMember){STRING("Pair<Key, Data> *"),STRING("data"),3} /* 25 */,
    (TemplatedMember){STRING("Pair<Key, Data> **"),STRING("next"),4} /* 26 */,
    (TemplatedMember){STRING("Hashmap<Data, int> *"),STRING("map"),0} /* 27 */,
    (TemplatedMember){STRING("HashmapIterator<Data, int>"),STRING("innerIter"),0} /* 28 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 29 */,
    (TemplatedMember){STRING("Pair<Key, Data>"),STRING("pair"),1} /* 30 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("next"),2} /* 31 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("ptr"),0} /* 32 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *[4]"),STRING("childs"),0} /* 33 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),1} /* 34 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("head"),2} /* 35 */,
    (TemplatedMember){STRING("TrieMapNode<Key, Data> *"),STRING("tail"),3} /* 36 */,
    (TemplatedMember){STRING("int"),STRING("inserted"),4} /* 37 */,
    (TemplatedMember){STRING("TrieMapIterator<Data, int>"),STRING("innerIter"),0} /* 38 */,
    (TemplatedMember){STRING("TrieMap<Data, int> *"),STRING("map"),0} /* 39 */,
    (TemplatedMember){STRING("Pool<T> *"),STRING("pool"),0} /* 40 */,
    (TemplatedMember){STRING("PageInfo"),STRING("pageInfo"),1} /* 41 */,
    (TemplatedMember){STRING("int"),STRING("fullIndex"),2} /* 42 */,
    (TemplatedMember){STRING("int"),STRING("bit"),3} /* 43 */,
    (TemplatedMember){STRING("int"),STRING("index"),4} /* 44 */,
    (TemplatedMember){STRING("Byte *"),STRING("page"),5} /* 45 */,
    (TemplatedMember){STRING("T *"),STRING("lastVal"),6} /* 46 */,
    (TemplatedMember){STRING("Byte *"),STRING("mem"),0} /* 47 */,
    (TemplatedMember){STRING("PoolInfo"),STRING("info"),1} /* 48 */,
    (TemplatedMember){STRING("Value"),STRING("value"),0} /* 49 */,
    (TemplatedMember){STRING("Error"),STRING("error"),0} /* 50 */,
    (TemplatedMember){STRING("bool"),STRING("isError"),1} /* 51 */,
    (TemplatedMember){STRING("int"),STRING("index"),0} /* 52 */,
    (TemplatedMember){STRING("T"),STRING("elem"),0} /* 53 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("next"),1} /* 54 */,
    (TemplatedMember){STRING("Arena *"),STRING("arena"),0} /* 55 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("head"),1} /* 56 */,
    (TemplatedMember){STRING("ListedStruct<T> *"),STRING("tail"),2} /* 57 */,
    (TemplatedMember){STRING("Array<T>"),STRING("mem"),0} /* 58 */,
    (TemplatedMember){STRING("int"),STRING("index"),1} /* 59 */,
    (TemplatedMember){STRING("String"),STRING("name"),0} /* 60 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),1} /* 61 */,
    (TemplatedMember){STRING("bool"),STRING("isStatic"),2} /* 62 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeIn"),0} /* 63 */,
    (TemplatedMember){STRING("T"),STRING("bitSizeOut"),1} /* 64 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),2} /* 65 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),3} /* 66 */,
    (TemplatedMember){STRING("T"),STRING("bitSize"),0} /* 67 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeIn"),1} /* 68 */,
    (TemplatedMember){STRING("T"),STRING("dataSizeOut"),2} /* 69 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 70 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 71 */,
    (TemplatedMember){STRING("ExternalMemoryTwoPortsTemplate<T>"),STRING("tp"),0} /* 72 */,
    (TemplatedMember){STRING("ExternalMemoryDualPortTemplate<T>[2]"),STRING("dp"),0} /* 73 */,
    (TemplatedMember){STRING("ExternalMemoryType"),STRING("type"),1} /* 74 */,
    (TemplatedMember){STRING("int"),STRING("interface"),2} /* 75 */,
    (TemplatedMember){STRING("T*"),STRING("mem"),0} /* 76 */,
    (TemplatedMember){STRING("int"),STRING("size"),1} /* 77 */,
    (TemplatedMember){STRING("int"),STRING("allocated"),2} /* 78 */
  };

  RegisterTemplateMembers(STRING("_Defer"),(Array<TemplatedMember>){&templateMembers[0],1});
  RegisterTemplateMembers(STRING("ArrayIterator"),(Array<TemplatedMember>){&templateMembers[1],1});
  RegisterTemplateMembers(STRING("Array"),(Array<TemplatedMember>){&templateMembers[2],2});
  RegisterTemplateMembers(STRING("Range"),(Array<TemplatedMember>){&templateMembers[4],6});
  RegisterTemplateMembers(STRING("Pair"),(Array<TemplatedMember>){&templateMembers[10],4});
  RegisterTemplateMembers(STRING("DynamicArray"),(Array<TemplatedMember>){&templateMembers[14],1});
  RegisterTemplateMembers(STRING("PushPtr"),(Array<TemplatedMember>){&templateMembers[15],3});
  RegisterTemplateMembers(STRING("HashmapIterator"),(Array<TemplatedMember>){&templateMembers[18],2});
  RegisterTemplateMembers(STRING("GetOrAllocateResult"),(Array<TemplatedMember>){&templateMembers[20],2});
  RegisterTemplateMembers(STRING("Hashmap"),(Array<TemplatedMember>){&templateMembers[22],5});
  RegisterTemplateMembers(STRING("Set"),(Array<TemplatedMember>){&templateMembers[27],1});
  RegisterTemplateMembers(STRING("SetIterator"),(Array<TemplatedMember>){&templateMembers[28],1});
  RegisterTemplateMembers(STRING("TrieMapNode"),(Array<TemplatedMember>){&templateMembers[29],3});
  RegisterTemplateMembers(STRING("TrieMapIterator"),(Array<TemplatedMember>){&templateMembers[32],1});
  RegisterTemplateMembers(STRING("TrieMap"),(Array<TemplatedMember>){&templateMembers[33],5});
  RegisterTemplateMembers(STRING("TrieSetIterator"),(Array<TemplatedMember>){&templateMembers[38],1});
  RegisterTemplateMembers(STRING("TrieSet"),(Array<TemplatedMember>){&templateMembers[39],1});
  RegisterTemplateMembers(STRING("PoolIterator"),(Array<TemplatedMember>){&templateMembers[40],7});
  RegisterTemplateMembers(STRING("Pool"),(Array<TemplatedMember>){&templateMembers[47],2});
  RegisterTemplateMembers(STRING("Result"),(Array<TemplatedMember>){&templateMembers[49],3});
  RegisterTemplateMembers(STRING("IndexedStruct"),(Array<TemplatedMember>){&templateMembers[52],1});
  RegisterTemplateMembers(STRING("ListedStruct"),(Array<TemplatedMember>){&templateMembers[53],2});
  RegisterTemplateMembers(STRING("ArenaList"),(Array<TemplatedMember>){&templateMembers[55],3});
  RegisterTemplateMembers(STRING("Stack"),(Array<TemplatedMember>){&templateMembers[58],2});
  RegisterTemplateMembers(STRING("WireTemplate"),(Array<TemplatedMember>){&templateMembers[60],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTwoPortsTemplate"),(Array<TemplatedMember>){&templateMembers[63],4});
  RegisterTemplateMembers(STRING("ExternalMemoryDualPortTemplate"),(Array<TemplatedMember>){&templateMembers[67],3});
  RegisterTemplateMembers(STRING("ExternalMemoryTemplate"),(Array<TemplatedMember>){&templateMembers[70],2});
  RegisterTemplateMembers(STRING("ExternalMemoryInterfaceTemplate"),(Array<TemplatedMember>){&templateMembers[72],4});
  RegisterTemplateMembers(STRING("std::vector"),(Array<TemplatedMember>){&templateMembers[76],3});

  static Member members[] = {(Member){GetType(STRING("u64")),STRING("microSeconds"),offsetof(Time,microSeconds)} /* 0 */,
    (Member){GetType(STRING("u64")),STRING("seconds"),offsetof(Time,seconds)} /* 1 */,
    (Member){GetType(STRING("float")),STRING("f"),offsetof(Conversion,f)} /* 2 */,
    (Member){GetType(STRING("int")),STRING("i"),offsetof(Conversion,i)} /* 3 */,
    (Member){GetType(STRING("uint")),STRING("ui"),offsetof(Conversion,ui)} /* 4 */,
    (Member){GetType(STRING("Byte *")),STRING("mem"),offsetof(Arena,mem)} /* 5 */,
    (Member){GetType(STRING("int")),STRING("used"),offsetof(Arena,used)} /* 6 */,
    (Member){GetType(STRING("int")),STRING("totalAllocated"),offsetof(Arena,totalAllocated)} /* 7 */,
    (Member){GetType(STRING("int")),STRING("maximum"),offsetof(Arena,maximum)} /* 8 */,
    (Member){GetType(STRING("bool")),STRING("locked"),offsetof(Arena,locked)} /* 9 */,
    (Member){GetType(STRING("Arena *")),STRING("arena"),offsetof(ArenaMark,arena)} /* 10 */,
    (Member){GetType(STRING("Byte *")),STRING("mark"),offsetof(ArenaMark,mark)} /* 11 */,
    (Member){GetType(STRING("DynamicArena *")),STRING("next"),offsetof(DynamicArena,next)} /* 12 */,
    (Member){GetType(STRING("Byte *")),STRING("mem"),offsetof(DynamicArena,mem)} /* 13 */,
    (Member){GetType(STRING("int")),STRING("used"),offsetof(DynamicArena,used)} /* 14 */,
    (Member){GetType(STRING("int")),STRING("pagesAllocated"),offsetof(DynamicArena,pagesAllocated)} /* 15 */,
    (Member){GetType(STRING("Arena *")),STRING("arena"),offsetof(DynamicString,arena)} /* 16 */,
    (Member){GetType(STRING("Byte *")),STRING("mark"),offsetof(DynamicString,mark)} /* 17 */,
    (Member){GetType(STRING("Byte *")),STRING("memory"),offsetof(BitArray,memory)} /* 18 */,
    (Member){GetType(STRING("int")),STRING("bitSize"),offsetof(BitArray,bitSize)} /* 19 */,
    (Member){GetType(STRING("int")),STRING("byteSize"),offsetof(BitArray,byteSize)} /* 20 */,
    (Member){GetType(STRING("Byte *")),STRING("nextPage"),offsetof(PoolHeader,nextPage)} /* 21 */,
    (Member){GetType(STRING("int")),STRING("allocatedUnits"),offsetof(PoolHeader,allocatedUnits)} /* 22 */,
    (Member){GetType(STRING("int")),STRING("unitsPerFullPage"),offsetof(PoolInfo,unitsPerFullPage)} /* 23 */,
    (Member){GetType(STRING("int")),STRING("bitmapSize"),offsetof(PoolInfo,bitmapSize)} /* 24 */,
    (Member){GetType(STRING("int")),STRING("unitsPerPage"),offsetof(PoolInfo,unitsPerPage)} /* 25 */,
    (Member){GetType(STRING("int")),STRING("pageGranuality"),offsetof(PoolInfo,pageGranuality)} /* 26 */,
    (Member){GetType(STRING("PoolHeader *")),STRING("header"),offsetof(PageInfo,header)} /* 27 */,
    (Member){GetType(STRING("Byte *")),STRING("bitmap"),offsetof(PageInfo,bitmap)} /* 28 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(EnumMember,name)} /* 29 */,
    (Member){GetType(STRING("String")),STRING("value"),offsetof(EnumMember,value)} /* 30 */,
    (Member){GetType(STRING("EnumMember *")),STRING("next"),offsetof(EnumMember,next)} /* 31 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TemplateArg,name)} /* 32 */,
    (Member){GetType(STRING("TemplateArg *")),STRING("next"),offsetof(TemplateArg,next)} /* 33 */,
    (Member){GetType(STRING("String")),STRING("typeName"),offsetof(TemplatedMember,typeName)} /* 34 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TemplatedMember,name)} /* 35 */,
    (Member){GetType(STRING("int")),STRING("memberOffset"),offsetof(TemplatedMember,memberOffset)} /* 36 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Type,name)} /* 37 */,
    (Member){GetType(STRING("Type *")),STRING("baseType"),offsetof(Type,baseType)} /* 38 */,
    (Member){GetType(STRING("Type *")),STRING("pointerType"),offsetof(Type,pointerType)} /* 39 */,
    (Member){GetType(STRING("Type *")),STRING("arrayType"),offsetof(Type,arrayType)} /* 40 */,
    (Member){GetType(STRING("Type *")),STRING("typedefType"),offsetof(Type,typedefType)} /* 41 */,
    (Member){GetType(STRING("Array<Pair<String, int>>")),STRING("enumMembers"),offsetof(Type,enumMembers)} /* 42 */,
    (Member){GetType(STRING("Array<TemplatedMember>")),STRING("templateMembers"),offsetof(Type,templateMembers)} /* 43 */,
    (Member){GetType(STRING("Array<String>")),STRING("templateArgs"),offsetof(Type,templateArgs)} /* 44 */,
    (Member){GetType(STRING("Array<Member>")),STRING("members"),offsetof(Type,members)} /* 45 */,
    (Member){GetType(STRING("Type *")),STRING("templateBase"),offsetof(Type,templateBase)} /* 46 */,
    (Member){GetType(STRING("Array<Type *>")),STRING("templateArgTypes"),offsetof(Type,templateArgTypes)} /* 47 */,
    (Member){GetType(STRING("int")),STRING("size"),offsetof(Type,size)} /* 48 */,
    (Member){GetType(STRING("int")),STRING("align"),offsetof(Type,align)} /* 49 */,
    (Member){GetType(STRING("enum Subtype")),STRING("type"),offsetof(Type,type)} /* 50 */,
    (Member){GetType(STRING("Type *")),STRING("type"),offsetof(Member,type)} /* 51 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Member,name)} /* 52 */,
    (Member){GetType(STRING("int")),STRING("offset"),offsetof(Member,offset)} /* 53 */,
    (Member){GetType(STRING("Type *")),STRING("structType"),offsetof(Member,structType)} /* 54 */,
    (Member){GetType(STRING("String")),STRING("arrayExpression"),offsetof(Member,arrayExpression)} /* 55 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(Member,index)} /* 56 */,
    (Member){GetType(STRING("PoolIterator<Type>")),STRING("iter"),offsetof(TypeIterator,iter)} /* 57 */,
    (Member){GetType(STRING("PoolIterator<Type>")),STRING("end"),offsetof(TypeIterator,end)} /* 58 */,
    (Member){GetType(STRING("bool")),STRING("boolean"),offsetof(Value,boolean)} /* 59 */,
    (Member){GetType(STRING("char")),STRING("ch"),offsetof(Value,ch)} /* 60 */,
    (Member){GetType(STRING("i64")),STRING("number"),offsetof(Value,number)} /* 61 */,
    (Member){GetType(STRING("String")),STRING("str"),offsetof(Value,str)} /* 62 */,
    (Member){GetType(STRING("bool")),STRING("literal"),offsetof(Value,literal)} /* 63 */,
    (Member){GetType(STRING("TemplateFunction *")),STRING("templateFunction"),offsetof(Value,templateFunction)} /* 64 */,
    (Member){GetType(STRING("void *")),STRING("custom"),offsetof(Value,custom)} /* 65 */,
    (Member){GetType(STRING("Type *")),STRING("type"),offsetof(Value,type)} /* 66 */,
    (Member){GetType(STRING("bool")),STRING("isTemp"),offsetof(Value,isTemp)} /* 67 */,
    (Member){GetType(STRING("int")),STRING("currentNumber"),offsetof(Iterator,currentNumber)} /* 68 */,
    (Member){GetType(STRING("GenericPoolIterator")),STRING("poolIterator"),offsetof(Iterator,poolIterator)} /* 69 */,
    (Member){GetType(STRING("Type *")),STRING("hashmapType"),offsetof(Iterator,hashmapType)} /* 70 */,
    (Member){GetType(STRING("Value")),STRING("iterating"),offsetof(Iterator,iterating)} /* 71 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(HashmapUnpackedIndex,index)} /* 72 */,
    (Member){GetType(STRING("bool")),STRING("data"),offsetof(HashmapUnpackedIndex,data)} /* 73 */,
    (Member){GetType(STRING("char *")),STRING("op"),offsetof(Expression,op)} /* 74 */,
    (Member){GetType(STRING("String")),STRING("id"),offsetof(Expression,id)} /* 75 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Expression,expressions)} /* 76 */,
    (Member){GetType(STRING("Command *")),STRING("command"),offsetof(Expression,command)} /* 77 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(Expression,val)} /* 78 */,
    (Member){GetType(STRING("String")),STRING("text"),offsetof(Expression,text)} /* 79 */,
    (Member){GetType(STRING("int")),STRING("approximateLine"),offsetof(Expression,approximateLine)} /* 80 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/common/parser.hpp:22:3)")),STRING("type"),offsetof(Expression,type)} /* 81 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Cursor,line)} /* 82 */,
    (Member){GetType(STRING("int")),STRING("column"),offsetof(Cursor,column)} /* 83 */,
    (Member){GetType(STRING("Range<Cursor>")),STRING("loc"),offsetof(Token,loc)} /* 84 */,
    (Member){GetType(STRING("String")),STRING("foundFirst"),offsetof(FindFirstResult,foundFirst)} /* 85 */,
    (Member){GetType(STRING("Token")),STRING("peekFindNotIncluded"),offsetof(FindFirstResult,peekFindNotIncluded)} /* 86 */,
    (Member){GetType(STRING("u16[128]")),STRING("array"),offsetof(Trie,array)} /* 87 */,
    (Member){GetType(STRING("Array<Trie>")),STRING("subTries"),offsetof(TokenizerTemplate,subTries)} /* 88 */,
    (Member){GetType(STRING("char *")),STRING("ptr"),offsetof(TokenizerMark,ptr)} /* 89 */,
    (Member){GetType(STRING("Cursor")),STRING("pos"),offsetof(TokenizerMark,pos)} /* 90 */,
    (Member){GetType(STRING("char *")),STRING("start"),offsetof(Tokenizer,start)} /* 91 */,
    (Member){GetType(STRING("char *")),STRING("ptr"),offsetof(Tokenizer,ptr)} /* 92 */,
    (Member){GetType(STRING("char *")),STRING("end"),offsetof(Tokenizer,end)} /* 93 */,
    (Member){GetType(STRING("TokenizerTemplate *")),STRING("tmpl"),offsetof(Tokenizer,tmpl)} /* 94 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Tokenizer,line)} /* 95 */,
    (Member){GetType(STRING("int")),STRING("column"),offsetof(Tokenizer,column)} /* 96 */,
    (Member){GetType(STRING("bool")),STRING("keepWhitespaces"),offsetof(Tokenizer,keepWhitespaces)} /* 97 */,
    (Member){GetType(STRING("bool")),STRING("keepComments"),offsetof(Tokenizer,keepComments)} /* 98 */,
    (Member){GetType(STRING("char **")),STRING("op"),offsetof(OperationList,op)} /* 99 */,
    (Member){GetType(STRING("int")),STRING("nOperations"),offsetof(OperationList,nOperations)} /* 100 */,
    (Member){GetType(STRING("OperationList *")),STRING("next"),offsetof(OperationList,next)} /* 101 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(CommandDefinition,name)} /* 102 */,
    (Member){GetType(STRING("int")),STRING("numberExpressions"),offsetof(CommandDefinition,numberExpressions)} /* 103 */,
    (Member){GetType(STRING("CommandType")),STRING("type"),offsetof(CommandDefinition,type)} /* 104 */,
    (Member){GetType(STRING("bool")),STRING("isBlockType"),offsetof(CommandDefinition,isBlockType)} /* 105 */,
    (Member){GetType(STRING("CommandDefinition *")),STRING("definition"),offsetof(Command,definition)} /* 106 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("expressions"),offsetof(Command,expressions)} /* 107 */,
    (Member){GetType(STRING("String")),STRING("fullText"),offsetof(Command,fullText)} /* 108 */,
    (Member){GetType(STRING("Token")),STRING("fullText"),offsetof(Block,fullText)} /* 109 */,
    (Member){GetType(STRING("String")),STRING("textBlock"),offsetof(Block,textBlock)} /* 110 */,
    (Member){GetType(STRING("Command *")),STRING("command"),offsetof(Block,command)} /* 111 */,
    (Member){GetType(STRING("Expression *")),STRING("expression"),offsetof(Block,expression)} /* 112 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("innerBlocks"),offsetof(Block,innerBlocks)} /* 113 */,
    (Member){GetType(STRING("BlockType")),STRING("type"),offsetof(Block,type)} /* 114 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(Block,line)} /* 115 */,
    (Member){GetType(STRING("Array<Expression *>")),STRING("arguments"),offsetof(TemplateFunction,arguments)} /* 116 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("blocks"),offsetof(TemplateFunction,blocks)} /* 117 */,
    (Member){GetType(STRING("TemplateRecordType")),STRING("type"),offsetof(TemplateRecord,type)} /* 118 */,
    (Member){GetType(STRING("Type *")),STRING("identifierType"),offsetof(TemplateRecord,identifierType)} /* 119 */,
    (Member){GetType(STRING("String")),STRING("identifierName"),offsetof(TemplateRecord,identifierName)} /* 120 */,
    (Member){GetType(STRING("Type *")),STRING("structType"),offsetof(TemplateRecord,structType)} /* 121 */,
    (Member){GetType(STRING("String")),STRING("fieldName"),offsetof(TemplateRecord,fieldName)} /* 122 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(ValueAndText,val)} /* 123 */,
    (Member){GetType(STRING("String")),STRING("text"),offsetof(ValueAndText,text)} /* 124 */,
    (Member){GetType(STRING("Hashmap<String, Value> *")),STRING("table"),offsetof(Frame,table)} /* 125 */,
    (Member){GetType(STRING("Frame *")),STRING("previousFrame"),offsetof(Frame,previousFrame)} /* 126 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(IndividualBlock,content)} /* 127 */,
    (Member){GetType(STRING("BlockType")),STRING("type"),offsetof(IndividualBlock,type)} /* 128 */,
    (Member){GetType(STRING("int")),STRING("line"),offsetof(IndividualBlock,line)} /* 129 */,
    (Member){GetType(STRING("int")),STRING("totalMemoryUsed"),offsetof(CompiledTemplate,totalMemoryUsed)} /* 130 */,
    (Member){GetType(STRING("String")),STRING("content"),offsetof(CompiledTemplate,content)} /* 131 */,
    (Member){GetType(STRING("Array<Block *>")),STRING("blocks"),offsetof(CompiledTemplate,blocks)} /* 132 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(CompiledTemplate,name)} /* 133 */,
    (Member){GetType(STRING("Hashmap<String, Value> *")),STRING("attributes"),offsetof(PortDeclaration,attributes)} /* 134 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("range"),offsetof(PortDeclaration,range)} /* 135 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(PortDeclaration,name)} /* 136 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/common/verilogParsing.hpp:21:3)")),STRING("type"),offsetof(PortDeclaration,type)} /* 137 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ParameterExpression,name)} /* 138 */,
    (Member){GetType(STRING("Expression *")),STRING("expr"),offsetof(ParameterExpression,expr)} /* 139 */,
    (Member){GetType(STRING("Array<ParameterExpression>")),STRING("parameters"),offsetof(Module,parameters)} /* 140 */,
    (Member){GetType(STRING("Array<PortDeclaration>")),STRING("ports"),offsetof(Module,ports)} /* 141 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Module,name)} /* 142 */,
    (Member){GetType(STRING("bool")),STRING("isSource"),offsetof(Module,isSource)} /* 143 */,
    (Member){GetType(STRING("int")),STRING("interface"),offsetof(ExternalMemoryID,interface)} /* 144 */,
    (Member){GetType(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryID,type)} /* 145 */,
    (Member){GetType(STRING("bool")),STRING("write"),offsetof(ExternalInfoTwoPorts,write)} /* 146 */,
    (Member){GetType(STRING("bool")),STRING("read"),offsetof(ExternalInfoTwoPorts,read)} /* 147 */,
    (Member){GetType(STRING("bool")),STRING("enable"),offsetof(ExternalInfoDualPort,enable)} /* 148 */,
    (Member){GetType(STRING("bool")),STRING("write"),offsetof(ExternalInfoDualPort,write)} /* 149 */,
    (Member){GetType(STRING("ExternalInfoTwoPorts")),STRING("tp"),offsetof(ExternalMemoryInfo,tp)} /* 150 */,
    (Member){GetType(STRING("ExternalInfoDualPort[2]")),STRING("dp"),offsetof(ExternalMemoryInfo,dp)} /* 151 */,
    (Member){GetType(STRING("ExternalMemoryType")),STRING("type"),offsetof(ExternalMemoryInfo,type)} /* 152 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ModuleInfo,name)} /* 153 */,
    (Member){GetType(STRING("Array<ParameterExpression>")),STRING("defaultParameters"),offsetof(ModuleInfo,defaultParameters)} /* 154 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelays"),offsetof(ModuleInfo,inputDelays)} /* 155 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ModuleInfo,outputLatencies)} /* 156 */,
    (Member){GetType(STRING("Array<WireExpression>")),STRING("configs"),offsetof(ModuleInfo,configs)} /* 157 */,
    (Member){GetType(STRING("Array<WireExpression>")),STRING("states"),offsetof(ModuleInfo,states)} /* 158 */,
    (Member){GetType(STRING("Array<ExternalMemoryInterfaceExpression>")),STRING("externalInterfaces"),offsetof(ModuleInfo,externalInterfaces)} /* 159 */,
    (Member){GetType(STRING("int")),STRING("nDelays"),offsetof(ModuleInfo,nDelays)} /* 160 */,
    (Member){GetType(STRING("int")),STRING("nIO"),offsetof(ModuleInfo,nIO)} /* 161 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("memoryMappedBits"),offsetof(ModuleInfo,memoryMappedBits)} /* 162 */,
    (Member){GetType(STRING("ExpressionRange")),STRING("databusAddrSize"),offsetof(ModuleInfo,databusAddrSize)} /* 163 */,
    (Member){GetType(STRING("bool")),STRING("doesIO"),offsetof(ModuleInfo,doesIO)} /* 164 */,
    (Member){GetType(STRING("bool")),STRING("memoryMapped"),offsetof(ModuleInfo,memoryMapped)} /* 165 */,
    (Member){GetType(STRING("bool")),STRING("hasDone"),offsetof(ModuleInfo,hasDone)} /* 166 */,
    (Member){GetType(STRING("bool")),STRING("hasClk"),offsetof(ModuleInfo,hasClk)} /* 167 */,
    (Member){GetType(STRING("bool")),STRING("hasReset"),offsetof(ModuleInfo,hasReset)} /* 168 */,
    (Member){GetType(STRING("bool")),STRING("hasRun"),offsetof(ModuleInfo,hasRun)} /* 169 */,
    (Member){GetType(STRING("bool")),STRING("hasRunning"),offsetof(ModuleInfo,hasRunning)} /* 170 */,
    (Member){GetType(STRING("bool")),STRING("isSource"),offsetof(ModuleInfo,isSource)} /* 171 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(ModuleInfo,signalLoop)} /* 172 */,
    (Member){GetType(STRING("Array<String>")),STRING("verilogFiles"),offsetof(Options,verilogFiles)} /* 173 */,
    (Member){GetType(STRING("Array<String>")),STRING("extraSources"),offsetof(Options,extraSources)} /* 174 */,
    (Member){GetType(STRING("Array<String>")),STRING("includePaths"),offsetof(Options,includePaths)} /* 175 */,
    (Member){GetType(STRING("Array<String>")),STRING("unitPaths"),offsetof(Options,unitPaths)} /* 176 */,
    (Member){GetType(STRING("String")),STRING("hardwareOutputFilepath"),offsetof(Options,hardwareOutputFilepath)} /* 177 */,
    (Member){GetType(STRING("String")),STRING("softwareOutputFilepath"),offsetof(Options,softwareOutputFilepath)} /* 178 */,
    (Member){GetType(STRING("String")),STRING("verilatorRoot"),offsetof(Options,verilatorRoot)} /* 179 */,
    (Member){GetType(STRING("char *")),STRING("specificationFilepath"),offsetof(Options,specificationFilepath)} /* 180 */,
    (Member){GetType(STRING("char *")),STRING("topName"),offsetof(Options,topName)} /* 181 */,
    (Member){GetType(STRING("int")),STRING("addrSize"),offsetof(Options,addrSize)} /* 182 */,
    (Member){GetType(STRING("int")),STRING("dataSize"),offsetof(Options,dataSize)} /* 183 */,
    (Member){GetType(STRING("bool")),STRING("addInputAndOutputsToTop"),offsetof(Options,addInputAndOutputsToTop)} /* 184 */,
    (Member){GetType(STRING("bool")),STRING("debug"),offsetof(Options,debug)} /* 185 */,
    (Member){GetType(STRING("bool")),STRING("shadowRegister"),offsetof(Options,shadowRegister)} /* 186 */,
    (Member){GetType(STRING("bool")),STRING("architectureHasDatabus"),offsetof(Options,architectureHasDatabus)} /* 187 */,
    (Member){GetType(STRING("bool")),STRING("useFixedBuffers"),offsetof(Options,useFixedBuffers)} /* 188 */,
    (Member){GetType(STRING("bool")),STRING("generateFSTFormat"),offsetof(Options,generateFSTFormat)} /* 189 */,
    (Member){GetType(STRING("bool")),STRING("disableDelayPropagation"),offsetof(Options,disableDelayPropagation)} /* 190 */,
    (Member){GetType(STRING("bool")),STRING("useDMA"),offsetof(Options,useDMA)} /* 191 */,
    (Member){GetType(STRING("bool")),STRING("exportInternalMemories"),offsetof(Options,exportInternalMemories)} /* 192 */,
    (Member){GetType(STRING("uint")),STRING("dotFormat"),offsetof(DebugState,dotFormat)} /* 193 */,
    (Member){GetType(STRING("bool")),STRING("outputGraphs"),offsetof(DebugState,outputGraphs)} /* 194 */,
    (Member){GetType(STRING("bool")),STRING("outputAccelerator"),offsetof(DebugState,outputAccelerator)} /* 195 */,
    (Member){GetType(STRING("bool")),STRING("outputVersat"),offsetof(DebugState,outputVersat)} /* 196 */,
    (Member){GetType(STRING("bool")),STRING("outputVCD"),offsetof(DebugState,outputVCD)} /* 197 */,
    (Member){GetType(STRING("bool")),STRING("outputAcceleratorInfo"),offsetof(DebugState,outputAcceleratorInfo)} /* 198 */,
    (Member){GetType(STRING("bool")),STRING("useFixedBuffers"),offsetof(DebugState,useFixedBuffers)} /* 199 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(StaticId,parent)} /* 200 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(StaticId,name)} /* 201 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(StaticData,configs)} /* 202 */,
    (Member){GetType(STRING("int")),STRING("offset"),offsetof(StaticData,offset)} /* 203 */,
    (Member){GetType(STRING("StaticId")),STRING("id"),offsetof(StaticInfo,id)} /* 204 */,
    (Member){GetType(STRING("StaticData")),STRING("data"),offsetof(StaticInfo,data)} /* 205 */,
    (Member){GetType(STRING("Array<int>")),STRING("offsets"),offsetof(CalculatedOffsets,offsets)} /* 206 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(CalculatedOffsets,max)} /* 207 */,
    (Member){GetType(STRING("int")),STRING("level"),offsetof(InstanceInfo,level)} /* 208 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("decl"),offsetof(InstanceInfo,decl)} /* 209 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(InstanceInfo,name)} /* 210 */,
    (Member){GetType(STRING("String")),STRING("baseName"),offsetof(InstanceInfo,baseName)} /* 211 */,
    (Member){GetType(STRING("int")),STRING("configPos"),offsetof(InstanceInfo,configPos)} /* 212 */,
    (Member){GetType(STRING("int")),STRING("isConfigStatic"),offsetof(InstanceInfo,isConfigStatic)} /* 213 */,
    (Member){GetType(STRING("int")),STRING("configSize"),offsetof(InstanceInfo,configSize)} /* 214 */,
    (Member){GetType(STRING("int")),STRING("statePos"),offsetof(InstanceInfo,statePos)} /* 215 */,
    (Member){GetType(STRING("int")),STRING("stateSize"),offsetof(InstanceInfo,stateSize)} /* 216 */,
    (Member){GetType(STRING("int")),STRING("memMapped"),offsetof(InstanceInfo,memMapped)} /* 217 */,
    (Member){GetType(STRING("int")),STRING("memMappedSize"),offsetof(InstanceInfo,memMappedSize)} /* 218 */,
    (Member){GetType(STRING("int")),STRING("memMappedBitSize"),offsetof(InstanceInfo,memMappedBitSize)} /* 219 */,
    (Member){GetType(STRING("int")),STRING("memMappedMask"),offsetof(InstanceInfo,memMappedMask)} /* 220 */,
    (Member){GetType(STRING("int")),STRING("delayPos"),offsetof(InstanceInfo,delayPos)} /* 221 */,
    (Member){GetType(STRING("Array<int>")),STRING("delay"),offsetof(InstanceInfo,delay)} /* 222 */,
    (Member){GetType(STRING("int")),STRING("baseDelay"),offsetof(InstanceInfo,baseDelay)} /* 223 */,
    (Member){GetType(STRING("int")),STRING("delaySize"),offsetof(InstanceInfo,delaySize)} /* 224 */,
    (Member){GetType(STRING("bool")),STRING("isComposite"),offsetof(InstanceInfo,isComposite)} /* 225 */,
    (Member){GetType(STRING("bool")),STRING("isStatic"),offsetof(InstanceInfo,isStatic)} /* 226 */,
    (Member){GetType(STRING("bool")),STRING("isShared"),offsetof(InstanceInfo,isShared)} /* 227 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceInfo,parent)} /* 228 */,
    (Member){GetType(STRING("String")),STRING("fullName"),offsetof(InstanceInfo,fullName)} /* 229 */,
    (Member){GetType(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(InstanceInfo,isMergeMultiplexer)} /* 230 */,
    (Member){GetType(STRING("bool")),STRING("belongs"),offsetof(InstanceInfo,belongs)} /* 231 */,
    (Member){GetType(STRING("int")),STRING("special"),offsetof(InstanceInfo,special)} /* 232 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(InstanceInfo,order)} /* 233 */,
    (Member){GetType(STRING("NodeType")),STRING("connectionType"),offsetof(InstanceInfo,connectionType)} /* 234 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(AcceleratorInfo,info)} /* 235 */,
    (Member){GetType(STRING("int")),STRING("memSize"),offsetof(AcceleratorInfo,memSize)} /* 236 */,
    (Member){GetType(STRING("int")),STRING("name"),offsetof(AcceleratorInfo,name)} /* 237 */,
    (Member){GetType(STRING("Hashmap<StaticId, int> *")),STRING("staticInfo"),offsetof(InstanceConfigurationOffsets,staticInfo)} /* 238 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("parent"),offsetof(InstanceConfigurationOffsets,parent)} /* 239 */,
    (Member){GetType(STRING("String")),STRING("topName"),offsetof(InstanceConfigurationOffsets,topName)} /* 240 */,
    (Member){GetType(STRING("String")),STRING("baseName"),offsetof(InstanceConfigurationOffsets,baseName)} /* 241 */,
    (Member){GetType(STRING("int")),STRING("configOffset"),offsetof(InstanceConfigurationOffsets,configOffset)} /* 242 */,
    (Member){GetType(STRING("int")),STRING("stateOffset"),offsetof(InstanceConfigurationOffsets,stateOffset)} /* 243 */,
    (Member){GetType(STRING("int")),STRING("delayOffset"),offsetof(InstanceConfigurationOffsets,delayOffset)} /* 244 */,
    (Member){GetType(STRING("int")),STRING("delay"),offsetof(InstanceConfigurationOffsets,delay)} /* 245 */,
    (Member){GetType(STRING("int")),STRING("memOffset"),offsetof(InstanceConfigurationOffsets,memOffset)} /* 246 */,
    (Member){GetType(STRING("int")),STRING("level"),offsetof(InstanceConfigurationOffsets,level)} /* 247 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(InstanceConfigurationOffsets,order)} /* 248 */,
    (Member){GetType(STRING("int *")),STRING("staticConfig"),offsetof(InstanceConfigurationOffsets,staticConfig)} /* 249 */,
    (Member){GetType(STRING("bool")),STRING("belongs"),offsetof(InstanceConfigurationOffsets,belongs)} /* 250 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("info"),offsetof(TestResult,info)} /* 251 */,
    (Member){GetType(STRING("InstanceConfigurationOffsets")),STRING("subOffsets"),offsetof(TestResult,subOffsets)} /* 252 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TestResult,name)} /* 253 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelay"),offsetof(TestResult,inputDelay)} /* 254 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(TestResult,outputLatencies)} /* 255 */,
    (Member){GetType(STRING("Array<Array<InstanceInfo>>")),STRING("infos"),offsetof(AccelInfo,infos)} /* 256 */,
    (Member){GetType(STRING("Array<InstanceInfo>")),STRING("baseInfo"),offsetof(AccelInfo,baseInfo)} /* 257 */,
    (Member){GetType(STRING("Array<String>")),STRING("names"),offsetof(AccelInfo,names)} /* 258 */,
    (Member){GetType(STRING("Array<Array<int>>")),STRING("inputDelays"),offsetof(AccelInfo,inputDelays)} /* 259 */,
    (Member){GetType(STRING("Array<Array<int>>")),STRING("outputDelays"),offsetof(AccelInfo,outputDelays)} /* 260 */,
    (Member){GetType(STRING("int")),STRING("memMappedBitsize"),offsetof(AccelInfo,memMappedBitsize)} /* 261 */,
    (Member){GetType(STRING("int")),STRING("howManyMergedUnits"),offsetof(AccelInfo,howManyMergedUnits)} /* 262 */,
    (Member){GetType(STRING("int")),STRING("inputs"),offsetof(AccelInfo,inputs)} /* 263 */,
    (Member){GetType(STRING("int")),STRING("outputs"),offsetof(AccelInfo,outputs)} /* 264 */,
    (Member){GetType(STRING("int")),STRING("configs"),offsetof(AccelInfo,configs)} /* 265 */,
    (Member){GetType(STRING("int")),STRING("states"),offsetof(AccelInfo,states)} /* 266 */,
    (Member){GetType(STRING("int")),STRING("delays"),offsetof(AccelInfo,delays)} /* 267 */,
    (Member){GetType(STRING("int")),STRING("ios"),offsetof(AccelInfo,ios)} /* 268 */,
    (Member){GetType(STRING("int")),STRING("statics"),offsetof(AccelInfo,statics)} /* 269 */,
    (Member){GetType(STRING("int")),STRING("sharedUnits"),offsetof(AccelInfo,sharedUnits)} /* 270 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(AccelInfo,externalMemoryInterfaces)} /* 271 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryByteSize"),offsetof(AccelInfo,externalMemoryByteSize)} /* 272 */,
    (Member){GetType(STRING("int")),STRING("numberUnits"),offsetof(AccelInfo,numberUnits)} /* 273 */,
    (Member){GetType(STRING("int")),STRING("numberConnections"),offsetof(AccelInfo,numberConnections)} /* 274 */,
    (Member){GetType(STRING("int")),STRING("memoryMappedBits"),offsetof(AccelInfo,memoryMappedBits)} /* 275 */,
    (Member){GetType(STRING("bool")),STRING("isMemoryMapped"),offsetof(AccelInfo,isMemoryMapped)} /* 276 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(AccelInfo,signalLoop)} /* 277 */,
    (Member){GetType(STRING("String")),STRING("type"),offsetof(TypeAndNameOnly,type)} /* 278 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TypeAndNameOnly,name)} /* 279 */,
    (Member){GetType(STRING("int")),STRING("value"),offsetof(Partition,value)} /* 280 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(Partition,max)} /* 281 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(OrderedConfigurations,configs)} /* 282 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("statics"),offsetof(OrderedConfigurations,statics)} /* 283 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("delays"),offsetof(OrderedConfigurations,delays)} /* 284 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(ConfigurationInfo,name)} /* 285 */,
    (Member){GetType(STRING("Array<String>")),STRING("baseName"),offsetof(ConfigurationInfo,baseName)} /* 286 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("baseType"),offsetof(ConfigurationInfo,baseType)} /* 287 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("configs"),offsetof(ConfigurationInfo,configs)} /* 288 */,
    (Member){GetType(STRING("Array<Wire>")),STRING("states"),offsetof(ConfigurationInfo,states)} /* 289 */,
    (Member){GetType(STRING("Array<int>")),STRING("inputDelays"),offsetof(ConfigurationInfo,inputDelays)} /* 290 */,
    (Member){GetType(STRING("Array<int>")),STRING("outputLatencies"),offsetof(ConfigurationInfo,outputLatencies)} /* 291 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("configOffsets"),offsetof(ConfigurationInfo,configOffsets)} /* 292 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("stateOffsets"),offsetof(ConfigurationInfo,stateOffsets)} /* 293 */,
    (Member){GetType(STRING("CalculatedOffsets")),STRING("delayOffsets"),offsetof(ConfigurationInfo,delayOffsets)} /* 294 */,
    (Member){GetType(STRING("Array<int>")),STRING("calculatedDelays"),offsetof(ConfigurationInfo,calculatedDelays)} /* 295 */,
    (Member){GetType(STRING("Array<int>")),STRING("order"),offsetof(ConfigurationInfo,order)} /* 296 */,
    (Member){GetType(STRING("Array<bool>")),STRING("unitBelongs"),offsetof(ConfigurationInfo,unitBelongs)} /* 297 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(FUDeclaration,name)} /* 298 */,
    (Member){GetType(STRING("ConfigurationInfo")),STRING("baseConfig"),offsetof(FUDeclaration,baseConfig)} /* 299 */,
    (Member){GetType(STRING("Array<ConfigurationInfo>")),STRING("configInfo"),offsetof(FUDeclaration,configInfo)} /* 300 */,
    (Member){GetType(STRING("int")),STRING("memoryMapBits"),offsetof(FUDeclaration,memoryMapBits)} /* 301 */,
    (Member){GetType(STRING("int")),STRING("nIOs"),offsetof(FUDeclaration,nIOs)} /* 302 */,
    (Member){GetType(STRING("Array<ExternalMemoryInterface>")),STRING("externalMemory"),offsetof(FUDeclaration,externalMemory)} /* 303 */,
    (Member){GetType(STRING("Accelerator *")),STRING("baseCircuit"),offsetof(FUDeclaration,baseCircuit)} /* 304 */,
    (Member){GetType(STRING("Accelerator *")),STRING("fixedDelayCircuit"),offsetof(FUDeclaration,fixedDelayCircuit)} /* 305 */,
    (Member){GetType(STRING("char *")),STRING("operation"),offsetof(FUDeclaration,operation)} /* 306 */,
    (Member){GetType(STRING("int")),STRING("lat"),offsetof(FUDeclaration,lat)} /* 307 */,
    (Member){GetType(STRING("Hashmap<StaticId, StaticData> *")),STRING("staticUnits"),offsetof(FUDeclaration,staticUnits)} /* 308 */,
    (Member){GetType(STRING("FUDeclarationType")),STRING("type"),offsetof(FUDeclaration,type)} /* 309 */,
    (Member){GetType(STRING("DelayType")),STRING("delayType"),offsetof(FUDeclaration,delayType)} /* 310 */,
    (Member){GetType(STRING("bool")),STRING("isOperation"),offsetof(FUDeclaration,isOperation)} /* 311 */,
    (Member){GetType(STRING("bool")),STRING("implementsDone"),offsetof(FUDeclaration,implementsDone)} /* 312 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(FUDeclaration,signalLoop)} /* 313 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(PortInstance,inst)} /* 314 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(PortInstance,port)} /* 315 */,
    (Member){GetType(STRING("PortInstance[2]")),STRING("units"),offsetof(PortEdge,units)} /* 316 */,
    (Member){GetType(STRING("PortInstance")),STRING("out"),offsetof(Edge,out)} /* 317 */,
    (Member){GetType(STRING("PortInstance")),STRING("in"),offsetof(Edge,in)} /* 318 */,
    (Member){GetType(STRING("PortEdge")),STRING("edge"),offsetof(Edge,edge)} /* 319 */,
    (Member){GetType(STRING("PortInstance[2]")),STRING("units"),offsetof(Edge,units)} /* 320 */,
    (Member){GetType(STRING("int")),STRING("delay"),offsetof(Edge,delay)} /* 321 */,
    (Member){GetType(STRING("Edge *")),STRING("next"),offsetof(Edge,next)} /* 322 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("node"),offsetof(PortNode,node)} /* 323 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(PortNode,port)} /* 324 */,
    (Member){GetType(STRING("PortNode")),STRING("node0"),offsetof(EdgeNode,node0)} /* 325 */,
    (Member){GetType(STRING("PortNode")),STRING("node1"),offsetof(EdgeNode,node1)} /* 326 */,
    (Member){GetType(STRING("PortNode")),STRING("instConnectedTo"),offsetof(ConnectionNode,instConnectedTo)} /* 327 */,
    (Member){GetType(STRING("int")),STRING("port"),offsetof(ConnectionNode,port)} /* 328 */,
    (Member){GetType(STRING("int")),STRING("edgeDelay"),offsetof(ConnectionNode,edgeDelay)} /* 329 */,
    (Member){GetType(STRING("int *")),STRING("delay"),offsetof(ConnectionNode,delay)} /* 330 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("next"),offsetof(ConnectionNode,next)} /* 331 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(InstanceNode,inst)} /* 332 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("next"),offsetof(InstanceNode,next)} /* 333 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("allInputs"),offsetof(InstanceNode,allInputs)} /* 334 */,
    (Member){GetType(STRING("ConnectionNode *")),STRING("allOutputs"),offsetof(InstanceNode,allOutputs)} /* 335 */,
    (Member){GetType(STRING("Array<PortNode>")),STRING("inputs"),offsetof(InstanceNode,inputs)} /* 336 */,
    (Member){GetType(STRING("Array<bool>")),STRING("outputs"),offsetof(InstanceNode,outputs)} /* 337 */,
    (Member){GetType(STRING("bool")),STRING("multipleSamePortInputs"),offsetof(InstanceNode,multipleSamePortInputs)} /* 338 */,
    (Member){GetType(STRING("NodeType")),STRING("type"),offsetof(InstanceNode,type)} /* 339 */,
    (Member){GetType(STRING("Edge *")),STRING("edges"),offsetof(Accelerator,edges)} /* 340 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("allocated"),offsetof(Accelerator,allocated)} /* 341 */,
    (Member){GetType(STRING("InstanceNode *")),STRING("lastAllocated"),offsetof(Accelerator,lastAllocated)} /* 342 */,
    (Member){GetType(STRING("Pool<FUInstance>")),STRING("instances"),offsetof(Accelerator,instances)} /* 343 */,
    (Member){GetType(STRING("DynamicArena *")),STRING("accelMemory"),offsetof(Accelerator,accelMemory)} /* 344 */,
    (Member){GetType(STRING("int")),STRING("staticUnits"),offsetof(Accelerator,staticUnits)} /* 345 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(Accelerator,name)} /* 346 */,
    (Member){GetType(STRING("int")),STRING("memoryMaskSize"),offsetof(MemoryAddressMask,memoryMaskSize)} /* 347 */,
    (Member){GetType(STRING("char[33]")),STRING("memoryMaskBuffer"),offsetof(MemoryAddressMask,memoryMaskBuffer)} /* 348 */,
    (Member){GetType(STRING("char *")),STRING("memoryMask"),offsetof(MemoryAddressMask,memoryMask)} /* 349 */,
    (Member){GetType(STRING("int")),STRING("nConfigs"),offsetof(VersatComputedValues,nConfigs)} /* 350 */,
    (Member){GetType(STRING("int")),STRING("configBits"),offsetof(VersatComputedValues,configBits)} /* 351 */,
    (Member){GetType(STRING("int")),STRING("versatConfigs"),offsetof(VersatComputedValues,versatConfigs)} /* 352 */,
    (Member){GetType(STRING("int")),STRING("versatStates"),offsetof(VersatComputedValues,versatStates)} /* 353 */,
    (Member){GetType(STRING("int")),STRING("nStatics"),offsetof(VersatComputedValues,nStatics)} /* 354 */,
    (Member){GetType(STRING("int")),STRING("staticBits"),offsetof(VersatComputedValues,staticBits)} /* 355 */,
    (Member){GetType(STRING("int")),STRING("staticBitsStart"),offsetof(VersatComputedValues,staticBitsStart)} /* 356 */,
    (Member){GetType(STRING("int")),STRING("nDelays"),offsetof(VersatComputedValues,nDelays)} /* 357 */,
    (Member){GetType(STRING("int")),STRING("delayBits"),offsetof(VersatComputedValues,delayBits)} /* 358 */,
    (Member){GetType(STRING("int")),STRING("delayBitsStart"),offsetof(VersatComputedValues,delayBitsStart)} /* 359 */,
    (Member){GetType(STRING("int")),STRING("nConfigurations"),offsetof(VersatComputedValues,nConfigurations)} /* 360 */,
    (Member){GetType(STRING("int")),STRING("configurationBits"),offsetof(VersatComputedValues,configurationBits)} /* 361 */,
    (Member){GetType(STRING("int")),STRING("configurationAddressBits"),offsetof(VersatComputedValues,configurationAddressBits)} /* 362 */,
    (Member){GetType(STRING("int")),STRING("nStates"),offsetof(VersatComputedValues,nStates)} /* 363 */,
    (Member){GetType(STRING("int")),STRING("stateBits"),offsetof(VersatComputedValues,stateBits)} /* 364 */,
    (Member){GetType(STRING("int")),STRING("stateAddressBits"),offsetof(VersatComputedValues,stateAddressBits)} /* 365 */,
    (Member){GetType(STRING("int")),STRING("unitsMapped"),offsetof(VersatComputedValues,unitsMapped)} /* 366 */,
    (Member){GetType(STRING("int")),STRING("memoryMappedBytes"),offsetof(VersatComputedValues,memoryMappedBytes)} /* 367 */,
    (Member){GetType(STRING("int")),STRING("nUnitsIO"),offsetof(VersatComputedValues,nUnitsIO)} /* 368 */,
    (Member){GetType(STRING("int")),STRING("numberConnections"),offsetof(VersatComputedValues,numberConnections)} /* 369 */,
    (Member){GetType(STRING("int")),STRING("externalMemoryInterfaces"),offsetof(VersatComputedValues,externalMemoryInterfaces)} /* 370 */,
    (Member){GetType(STRING("int")),STRING("stateConfigurationAddressBits"),offsetof(VersatComputedValues,stateConfigurationAddressBits)} /* 371 */,
    (Member){GetType(STRING("int")),STRING("memoryAddressBits"),offsetof(VersatComputedValues,memoryAddressBits)} /* 372 */,
    (Member){GetType(STRING("int")),STRING("memoryMappingAddressBits"),offsetof(VersatComputedValues,memoryMappingAddressBits)} /* 373 */,
    (Member){GetType(STRING("int")),STRING("memoryConfigDecisionBit"),offsetof(VersatComputedValues,memoryConfigDecisionBit)} /* 374 */,
    (Member){GetType(STRING("bool")),STRING("signalLoop"),offsetof(VersatComputedValues,signalLoop)} /* 375 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("sinks"),offsetof(DAGOrderNodes,sinks)} /* 376 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("sources"),offsetof(DAGOrderNodes,sources)} /* 377 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("computeUnits"),offsetof(DAGOrderNodes,computeUnits)} /* 378 */,
    (Member){GetType(STRING("Array<InstanceNode *>")),STRING("instances"),offsetof(DAGOrderNodes,instances)} /* 379 */,
    (Member){GetType(STRING("Array<int>")),STRING("order"),offsetof(DAGOrderNodes,order)} /* 380 */,
    (Member){GetType(STRING("int")),STRING("size"),offsetof(DAGOrderNodes,size)} /* 381 */,
    (Member){GetType(STRING("int")),STRING("maxOrder"),offsetof(DAGOrderNodes,maxOrder)} /* 382 */,
    (Member){GetType(STRING("Hashmap<EdgeNode, int> *")),STRING("edgesDelay"),offsetof(CalculateDelayResult,edgesDelay)} /* 383 */,
    (Member){GetType(STRING("Hashmap<PortNode, int> *")),STRING("portDelay"),offsetof(CalculateDelayResult,portDelay)} /* 384 */,
    (Member){GetType(STRING("Hashmap<InstanceNode *, int> *")),STRING("nodeDelay"),offsetof(CalculateDelayResult,nodeDelay)} /* 385 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(FUInstance,name)} /* 386 */,
    (Member){GetType(STRING("String")),STRING("parameters"),offsetof(FUInstance,parameters)} /* 387 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel"),offsetof(FUInstance,accel)} /* 388 */,
    (Member){GetType(STRING("FUDeclaration *")),STRING("declaration"),offsetof(FUInstance,declaration)} /* 389 */,
    (Member){GetType(STRING("int")),STRING("id"),offsetof(FUInstance,id)} /* 390 */,
    (Member){GetType(STRING("int")),STRING("literal"),offsetof(FUInstance,literal)} /* 391 */,
    (Member){GetType(STRING("int")),STRING("bufferAmount"),offsetof(FUInstance,bufferAmount)} /* 392 */,
    (Member){GetType(STRING("int")),STRING("portIndex"),offsetof(FUInstance,portIndex)} /* 393 */,
    (Member){GetType(STRING("int")),STRING("sharedIndex"),offsetof(FUInstance,sharedIndex)} /* 394 */,
    (Member){GetType(STRING("bool")),STRING("isStatic"),offsetof(FUInstance,isStatic)} /* 395 */,
    (Member){GetType(STRING("bool")),STRING("sharedEnable"),offsetof(FUInstance,sharedEnable)} /* 396 */,
    (Member){GetType(STRING("bool")),STRING("isMergeMultiplexer"),offsetof(FUInstance,isMergeMultiplexer)} /* 397 */,
    (Member){GetType(STRING("EdgeNode")),STRING("edge"),offsetof(DelayToAdd,edge)} /* 398 */,
    (Member){GetType(STRING("String")),STRING("bufferName"),offsetof(DelayToAdd,bufferName)} /* 399 */,
    (Member){GetType(STRING("String")),STRING("bufferParameters"),offsetof(DelayToAdd,bufferParameters)} /* 400 */,
    (Member){GetType(STRING("int")),STRING("bufferAmount"),offsetof(DelayToAdd,bufferAmount)} /* 401 */,
    (Member){GetType(STRING("String")),STRING("type"),offsetof(SingleTypeStructElement,type)} /* 402 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(SingleTypeStructElement,name)} /* 403 */,
    (Member){GetType(STRING("Array<SingleTypeStructElement>")),STRING("typeAndNames"),offsetof(TypeStructInfoElement,typeAndNames)} /* 404 */,
    (Member){GetType(STRING("String")),STRING("name"),offsetof(TypeStructInfo,name)} /* 405 */,
    (Member){GetType(STRING("Array<TypeStructInfoElement>")),STRING("entries"),offsetof(TypeStructInfo,entries)} /* 406 */,
    (Member){GetType(STRING("TaskFunction")),STRING("function"),offsetof(Task,function)} /* 407 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(Task,order)} /* 408 */,
    (Member){GetType(STRING("void *")),STRING("args"),offsetof(Task,args)} /* 409 */,
    (Member){GetType(STRING("TaskFunction")),STRING("function"),offsetof(WorkGroup,function)} /* 410 */,
    (Member){GetType(STRING("Array<Task>")),STRING("tasks"),offsetof(WorkGroup,tasks)} /* 411 */,
    (Member){GetType(STRING("String")),STRING("instA"),offsetof(SpecificMerge,instA)} /* 412 */,
    (Member){GetType(STRING("String")),STRING("instB"),offsetof(SpecificMerge,instB)} /* 413 */,
    (Member){GetType(STRING("int")),STRING("index"),offsetof(IndexRecord,index)} /* 414 */,
    (Member){GetType(STRING("IndexRecord *")),STRING("next"),offsetof(IndexRecord,next)} /* 415 */,
    (Member){GetType(STRING("FUInstance *")),STRING("instA"),offsetof(SpecificMergeNodes,instA)} /* 416 */,
    (Member){GetType(STRING("FUInstance *")),STRING("instB"),offsetof(SpecificMergeNodes,instB)} /* 417 */,
    (Member){GetType(STRING("int")),STRING("firstIndex"),offsetof(SpecificMergeNode,firstIndex)} /* 418 */,
    (Member){GetType(STRING("String")),STRING("firstName"),offsetof(SpecificMergeNode,firstName)} /* 419 */,
    (Member){GetType(STRING("int")),STRING("secondIndex"),offsetof(SpecificMergeNode,secondIndex)} /* 420 */,
    (Member){GetType(STRING("String")),STRING("secondName"),offsetof(SpecificMergeNode,secondName)} /* 421 */,
    (Member){GetType(STRING("FUInstance *[2]")),STRING("instances"),offsetof(MergeEdge,instances)} /* 422 */,
    (Member){GetType(STRING("MergeEdge")),STRING("nodes"),offsetof(MappingNode,nodes)} /* 423 */,
    (Member){GetType(STRING("PortEdge[2]")),STRING("edges"),offsetof(MappingNode,edges)} /* 424 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/compiler/merge.hpp:46:4)")),STRING("type"),offsetof(MappingNode,type)} /* 425 */,
    (Member){GetType(STRING("MappingNode *[2]")),STRING("nodes"),offsetof(MappingEdge,nodes)} /* 426 */,
    (Member){GetType(STRING("Array<SpecificMergeNodes>")),STRING("specifics"),offsetof(ConsolidationGraphOptions,specifics)} /* 427 */,
    (Member){GetType(STRING("int")),STRING("order"),offsetof(ConsolidationGraphOptions,order)} /* 428 */,
    (Member){GetType(STRING("int")),STRING("difference"),offsetof(ConsolidationGraphOptions,difference)} /* 429 */,
    (Member){GetType(STRING("bool")),STRING("mapNodes"),offsetof(ConsolidationGraphOptions,mapNodes)} /* 430 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/compiler/merge.hpp:64:4)")),STRING("type"),offsetof(ConsolidationGraphOptions,type)} /* 431 */,
    (Member){GetType(STRING("Array<MappingNode>")),STRING("nodes"),offsetof(ConsolidationGraph,nodes)} /* 432 */,
    (Member){GetType(STRING("Array<BitArray>")),STRING("edges"),offsetof(ConsolidationGraph,edges)} /* 433 */,
    (Member){GetType(STRING("BitArray")),STRING("validNodes"),offsetof(ConsolidationGraph,validNodes)} /* 434 */,
    (Member){GetType(STRING("ConsolidationGraph")),STRING("graph"),offsetof(ConsolidationResult,graph)} /* 435 */,
    (Member){GetType(STRING("Pool<MappingNode>")),STRING("specificsAdded"),offsetof(ConsolidationResult,specificsAdded)} /* 436 */,
    (Member){GetType(STRING("int")),STRING("upperBound"),offsetof(ConsolidationResult,upperBound)} /* 437 */,
    (Member){GetType(STRING("int")),STRING("max"),offsetof(CliqueState,max)} /* 438 */,
    (Member){GetType(STRING("int")),STRING("upperBound"),offsetof(CliqueState,upperBound)} /* 439 */,
    (Member){GetType(STRING("int")),STRING("startI"),offsetof(CliqueState,startI)} /* 440 */,
    (Member){GetType(STRING("int")),STRING("iterations"),offsetof(CliqueState,iterations)} /* 441 */,
    (Member){GetType(STRING("Array<int>")),STRING("table"),offsetof(CliqueState,table)} /* 442 */,
    (Member){GetType(STRING("ConsolidationGraph")),STRING("clique"),offsetof(CliqueState,clique)} /* 443 */,
    (Member){GetType(STRING("Time")),STRING("start"),offsetof(CliqueState,start)} /* 444 */,
    (Member){GetType(STRING("bool")),STRING("found"),offsetof(CliqueState,found)} /* 445 */,
    (Member){GetType(STRING("bool")),STRING("result"),offsetof(IsCliqueResult,result)} /* 446 */,
    (Member){GetType(STRING("int")),STRING("failedIndex"),offsetof(IsCliqueResult,failedIndex)} /* 447 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel1"),offsetof(MergeGraphResult,accel1)} /* 448 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResult,accel2)} /* 449 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map1"),offsetof(MergeGraphResult,map1)} /* 450 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map2"),offsetof(MergeGraphResult,map2)} /* 451 */,
    (Member){GetType(STRING("Accelerator *")),STRING("newGraph"),offsetof(MergeGraphResult,newGraph)} /* 452 */,
    (Member){GetType(STRING("Accelerator *")),STRING("result"),offsetof(MergeGraphResultExisting,result)} /* 453 */,
    (Member){GetType(STRING("Accelerator *")),STRING("accel2"),offsetof(MergeGraphResultExisting,accel2)} /* 454 */,
    (Member){GetType(STRING("InstanceNodeMap *")),STRING("map2"),offsetof(MergeGraphResultExisting,map2)} /* 455 */,
    (Member){GetType(STRING("InstanceMap *")),STRING("instanceMap"),offsetof(GraphMapping,instanceMap)} /* 456 */,
    (Member){GetType(STRING("InstanceMap *")),STRING("reverseInstanceMap"),offsetof(GraphMapping,reverseInstanceMap)} /* 457 */,
    (Member){GetType(STRING("PortEdgeMap *")),STRING("edgeMap"),offsetof(GraphMapping,edgeMap)} /* 458 */,
    (Member){GetType(STRING("Range<int>")),STRING("port"),offsetof(ConnectionExtra,port)} /* 459 */,
    (Member){GetType(STRING("Range<int>")),STRING("delay"),offsetof(ConnectionExtra,delay)} /* 460 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(Var,name)} /* 461 */,
    (Member){GetType(STRING("ConnectionExtra")),STRING("extra"),offsetof(Var,extra)} /* 462 */,
    (Member){GetType(STRING("Range<int>")),STRING("index"),offsetof(Var,index)} /* 463 */,
    (Member){GetType(STRING("bool")),STRING("isArrayAccess"),offsetof(Var,isArrayAccess)} /* 464 */,
    (Member){GetType(STRING("Array<Var>")),STRING("vars"),offsetof(VarGroup,vars)} /* 465 */,
    (Member){GetType(STRING("Token")),STRING("fullText"),offsetof(VarGroup,fullText)} /* 466 */,
    (Member){GetType(STRING("Array<SpecExpression *>")),STRING("expressions"),offsetof(SpecExpression,expressions)} /* 467 */,
    (Member){GetType(STRING("char *")),STRING("op"),offsetof(SpecExpression,op)} /* 468 */,
    (Member){GetType(STRING("Var")),STRING("var"),offsetof(SpecExpression,var)} /* 469 */,
    (Member){GetType(STRING("Value")),STRING("val"),offsetof(SpecExpression,val)} /* 470 */,
    (Member){GetType(STRING("Token")),STRING("text"),offsetof(SpecExpression,text)} /* 471 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/compiler/versatSpecificationParser.hpp:50:3)")),STRING("type"),offsetof(SpecExpression,type)} /* 472 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(VarDeclaration,name)} /* 473 */,
    (Member){GetType(STRING("int")),STRING("arraySize"),offsetof(VarDeclaration,arraySize)} /* 474 */,
    (Member){GetType(STRING("bool")),STRING("isArray"),offsetof(VarDeclaration,isArray)} /* 475 */,
    (Member){GetType(STRING("VarGroup")),STRING("group"),offsetof(GroupIterator,group)} /* 476 */,
    (Member){GetType(STRING("int")),STRING("groupIndex"),offsetof(GroupIterator,groupIndex)} /* 477 */,
    (Member){GetType(STRING("int")),STRING("varIndex"),offsetof(GroupIterator,varIndex)} /* 478 */,
    (Member){GetType(STRING("FUInstance *")),STRING("inst"),offsetof(PortExpression,inst)} /* 479 */,
    (Member){GetType(STRING("ConnectionExtra")),STRING("extra"),offsetof(PortExpression,extra)} /* 480 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/compiler/versatSpecificationParser.hpp:71:3)")),STRING("modifier"),offsetof(InstanceDeclaration,modifier)} /* 481 */,
    (Member){GetType(STRING("Token")),STRING("typeName"),offsetof(InstanceDeclaration,typeName)} /* 482 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("declarations"),offsetof(InstanceDeclaration,declarations)} /* 483 */,
    (Member){GetType(STRING("String")),STRING("parameters"),offsetof(InstanceDeclaration,parameters)} /* 484 */,
    (Member){GetType(STRING("Range<Cursor>")),STRING("loc"),offsetof(ConnectionDef,loc)} /* 485 */,
    (Member){GetType(STRING("VarGroup")),STRING("output"),offsetof(ConnectionDef,output)} /* 486 */,
    (Member){GetType(STRING("enum (unnamed enum at ../../software/compiler/versatSpecificationParser.hpp:81:3)")),STRING("type"),offsetof(ConnectionDef,type)} /* 487 */,
    (Member){GetType(STRING("Array<Token>")),STRING("transforms"),offsetof(ConnectionDef,transforms)} /* 488 */,
    (Member){GetType(STRING("VarGroup")),STRING("input"),offsetof(ConnectionDef,input)} /* 489 */,
    (Member){GetType(STRING("SpecExpression *")),STRING("expression"),offsetof(ConnectionDef,expression)} /* 490 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(ModuleDef,name)} /* 491 */,
    (Member){GetType(STRING("Token")),STRING("numberOutputs"),offsetof(ModuleDef,numberOutputs)} /* 492 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(ModuleDef,inputs)} /* 493 */,
    (Member){GetType(STRING("Array<InstanceDeclaration>")),STRING("declarations"),offsetof(ModuleDef,declarations)} /* 494 */,
    (Member){GetType(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(ModuleDef,connections)} /* 495 */,
    (Member){GetType(STRING("Token")),STRING("name"),offsetof(TransformDef,name)} /* 496 */,
    (Member){GetType(STRING("Array<VarDeclaration>")),STRING("inputs"),offsetof(TransformDef,inputs)} /* 497 */,
    (Member){GetType(STRING("Array<ConnectionDef>")),STRING("connections"),offsetof(TransformDef,connections)} /* 498 */,
    (Member){GetType(STRING("int")),STRING("inputs"),offsetof(Transformation,inputs)} /* 499 */,
    (Member){GetType(STRING("int")),STRING("outputs"),offsetof(Transformation,outputs)} /* 500 */,
    (Member){GetType(STRING("Array<int>")),STRING("map"),offsetof(Transformation,map)} /* 501 */,
    (Member){GetType(STRING("Token")),STRING("instanceName"),offsetof(HierarchicalName,instanceName)} /* 502 */,
    (Member){GetType(STRING("Var")),STRING("subInstance"),offsetof(HierarchicalName,subInstance)} /* 503 */,
    (Member){GetType(STRING("Token")),STRING("typeName"),offsetof(TypeAndInstance,typeName)} /* 504 */,
    (Member){GetType(STRING("Token")),STRING("instanceName"),offsetof(TypeAndInstance,instanceName)} /* 505 */
  };

  RegisterStructMembers(STRING("Time"),(Array<Member>){&members[0],2});
  RegisterStructMembers(STRING("Conversion"),(Array<Member>){&members[2],3});
  RegisterStructMembers(STRING("Arena"),(Array<Member>){&members[5],5});
  RegisterStructMembers(STRING("ArenaMark"),(Array<Member>){&members[10],2});
  RegisterStructMembers(STRING("DynamicArena"),(Array<Member>){&members[12],4});
  RegisterStructMembers(STRING("DynamicString"),(Array<Member>){&members[16],2});
  RegisterStructMembers(STRING("BitArray"),(Array<Member>){&members[18],3});
  RegisterStructMembers(STRING("PoolHeader"),(Array<Member>){&members[21],2});
  RegisterStructMembers(STRING("PoolInfo"),(Array<Member>){&members[23],4});
  RegisterStructMembers(STRING("PageInfo"),(Array<Member>){&members[27],2});
  RegisterStructMembers(STRING("EnumMember"),(Array<Member>){&members[29],3});
  RegisterStructMembers(STRING("TemplateArg"),(Array<Member>){&members[32],2});
  RegisterStructMembers(STRING("TemplatedMember"),(Array<Member>){&members[34],3});
  RegisterStructMembers(STRING("Type"),(Array<Member>){&members[37],14});
  RegisterStructMembers(STRING("Member"),(Array<Member>){&members[51],6});
  RegisterStructMembers(STRING("TypeIterator"),(Array<Member>){&members[57],2});
  RegisterStructMembers(STRING("Value"),(Array<Member>){&members[59],9});
  RegisterStructMembers(STRING("Iterator"),(Array<Member>){&members[68],4});
  RegisterStructMembers(STRING("HashmapUnpackedIndex"),(Array<Member>){&members[72],2});
  RegisterStructMembers(STRING("Expression"),(Array<Member>){&members[74],8});
  RegisterStructMembers(STRING("Cursor"),(Array<Member>){&members[82],2});
  RegisterStructMembers(STRING("Token"),(Array<Member>){&members[84],1});
  RegisterStructMembers(STRING("FindFirstResult"),(Array<Member>){&members[85],2});
  RegisterStructMembers(STRING("Trie"),(Array<Member>){&members[87],1});
  RegisterStructMembers(STRING("TokenizerTemplate"),(Array<Member>){&members[88],1});
  RegisterStructMembers(STRING("TokenizerMark"),(Array<Member>){&members[89],2});
  RegisterStructMembers(STRING("Tokenizer"),(Array<Member>){&members[91],8});
  RegisterStructMembers(STRING("OperationList"),(Array<Member>){&members[99],3});
  RegisterStructMembers(STRING("CommandDefinition"),(Array<Member>){&members[102],4});
  RegisterStructMembers(STRING("Command"),(Array<Member>){&members[106],3});
  RegisterStructMembers(STRING("Block"),(Array<Member>){&members[109],7});
  RegisterStructMembers(STRING("TemplateFunction"),(Array<Member>){&members[116],2});
  RegisterStructMembers(STRING("TemplateRecord"),(Array<Member>){&members[118],5});
  RegisterStructMembers(STRING("ValueAndText"),(Array<Member>){&members[123],2});
  RegisterStructMembers(STRING("Frame"),(Array<Member>){&members[125],2});
  RegisterStructMembers(STRING("IndividualBlock"),(Array<Member>){&members[127],3});
  RegisterStructMembers(STRING("CompiledTemplate"),(Array<Member>){&members[130],4});
  RegisterStructMembers(STRING("PortDeclaration"),(Array<Member>){&members[134],4});
  RegisterStructMembers(STRING("ParameterExpression"),(Array<Member>){&members[138],2});
  RegisterStructMembers(STRING("Module"),(Array<Member>){&members[140],4});
  RegisterStructMembers(STRING("ExternalMemoryID"),(Array<Member>){&members[144],2});
  RegisterStructMembers(STRING("ExternalInfoTwoPorts"),(Array<Member>){&members[146],2});
  RegisterStructMembers(STRING("ExternalInfoDualPort"),(Array<Member>){&members[148],2});
  RegisterStructMembers(STRING("ExternalMemoryInfo"),(Array<Member>){&members[150],3});
  RegisterStructMembers(STRING("ModuleInfo"),(Array<Member>){&members[153],20});
  RegisterStructMembers(STRING("Options"),(Array<Member>){&members[173],20});
  RegisterStructMembers(STRING("DebugState"),(Array<Member>){&members[193],7});
  RegisterStructMembers(STRING("StaticId"),(Array<Member>){&members[200],2});
  RegisterStructMembers(STRING("StaticData"),(Array<Member>){&members[202],2});
  RegisterStructMembers(STRING("StaticInfo"),(Array<Member>){&members[204],2});
  RegisterStructMembers(STRING("CalculatedOffsets"),(Array<Member>){&members[206],2});
  RegisterStructMembers(STRING("InstanceInfo"),(Array<Member>){&members[208],27});
  RegisterStructMembers(STRING("AcceleratorInfo"),(Array<Member>){&members[235],3});
  RegisterStructMembers(STRING("InstanceConfigurationOffsets"),(Array<Member>){&members[238],13});
  RegisterStructMembers(STRING("TestResult"),(Array<Member>){&members[251],5});
  RegisterStructMembers(STRING("AccelInfo"),(Array<Member>){&members[256],22});
  RegisterStructMembers(STRING("TypeAndNameOnly"),(Array<Member>){&members[278],2});
  RegisterStructMembers(STRING("Partition"),(Array<Member>){&members[280],2});
  RegisterStructMembers(STRING("OrderedConfigurations"),(Array<Member>){&members[282],3});
  RegisterStructMembers(STRING("ConfigurationInfo"),(Array<Member>){&members[285],13});
  RegisterStructMembers(STRING("FUDeclaration"),(Array<Member>){&members[298],16});
  RegisterStructMembers(STRING("PortInstance"),(Array<Member>){&members[314],2});
  RegisterStructMembers(STRING("PortEdge"),(Array<Member>){&members[316],1});
  RegisterStructMembers(STRING("Edge"),(Array<Member>){&members[317],6});
  RegisterStructMembers(STRING("PortNode"),(Array<Member>){&members[323],2});
  RegisterStructMembers(STRING("EdgeNode"),(Array<Member>){&members[325],2});
  RegisterStructMembers(STRING("ConnectionNode"),(Array<Member>){&members[327],5});
  RegisterStructMembers(STRING("InstanceNode"),(Array<Member>){&members[332],8});
  RegisterStructMembers(STRING("Accelerator"),(Array<Member>){&members[340],7});
  RegisterStructMembers(STRING("MemoryAddressMask"),(Array<Member>){&members[347],3});
  RegisterStructMembers(STRING("VersatComputedValues"),(Array<Member>){&members[350],26});
  RegisterStructMembers(STRING("DAGOrderNodes"),(Array<Member>){&members[376],7});
  RegisterStructMembers(STRING("CalculateDelayResult"),(Array<Member>){&members[383],3});
  RegisterStructMembers(STRING("FUInstance"),(Array<Member>){&members[386],12});
  RegisterStructMembers(STRING("DelayToAdd"),(Array<Member>){&members[398],4});
  RegisterStructMembers(STRING("SingleTypeStructElement"),(Array<Member>){&members[402],2});
  RegisterStructMembers(STRING("TypeStructInfoElement"),(Array<Member>){&members[404],1});
  RegisterStructMembers(STRING("TypeStructInfo"),(Array<Member>){&members[405],2});
  RegisterStructMembers(STRING("Task"),(Array<Member>){&members[407],3});
  RegisterStructMembers(STRING("WorkGroup"),(Array<Member>){&members[410],2});
  RegisterStructMembers(STRING("SpecificMerge"),(Array<Member>){&members[412],2});
  RegisterStructMembers(STRING("IndexRecord"),(Array<Member>){&members[414],2});
  RegisterStructMembers(STRING("SpecificMergeNodes"),(Array<Member>){&members[416],2});
  RegisterStructMembers(STRING("SpecificMergeNode"),(Array<Member>){&members[418],4});
  RegisterStructMembers(STRING("MergeEdge"),(Array<Member>){&members[422],1});
  RegisterStructMembers(STRING("MappingNode"),(Array<Member>){&members[423],3});
  RegisterStructMembers(STRING("MappingEdge"),(Array<Member>){&members[426],1});
  RegisterStructMembers(STRING("ConsolidationGraphOptions"),(Array<Member>){&members[427],5});
  RegisterStructMembers(STRING("ConsolidationGraph"),(Array<Member>){&members[432],3});
  RegisterStructMembers(STRING("ConsolidationResult"),(Array<Member>){&members[435],3});
  RegisterStructMembers(STRING("CliqueState"),(Array<Member>){&members[438],8});
  RegisterStructMembers(STRING("IsCliqueResult"),(Array<Member>){&members[446],2});
  RegisterStructMembers(STRING("MergeGraphResult"),(Array<Member>){&members[448],5});
  RegisterStructMembers(STRING("MergeGraphResultExisting"),(Array<Member>){&members[453],3});
  RegisterStructMembers(STRING("GraphMapping"),(Array<Member>){&members[456],3});
  RegisterStructMembers(STRING("ConnectionExtra"),(Array<Member>){&members[459],2});
  RegisterStructMembers(STRING("Var"),(Array<Member>){&members[461],4});
  RegisterStructMembers(STRING("VarGroup"),(Array<Member>){&members[465],2});
  RegisterStructMembers(STRING("SpecExpression"),(Array<Member>){&members[467],6});
  RegisterStructMembers(STRING("VarDeclaration"),(Array<Member>){&members[473],3});
  RegisterStructMembers(STRING("GroupIterator"),(Array<Member>){&members[476],3});
  RegisterStructMembers(STRING("PortExpression"),(Array<Member>){&members[479],2});
  RegisterStructMembers(STRING("InstanceDeclaration"),(Array<Member>){&members[481],4});
  RegisterStructMembers(STRING("ConnectionDef"),(Array<Member>){&members[485],6});
  RegisterStructMembers(STRING("ModuleDef"),(Array<Member>){&members[491],5});
  RegisterStructMembers(STRING("TransformDef"),(Array<Member>){&members[496],3});
  RegisterStructMembers(STRING("Transformation"),(Array<Member>){&members[499],3});
  RegisterStructMembers(STRING("HierarchicalName"),(Array<Member>){&members[502],2});
  RegisterStructMembers(STRING("TypeAndInstance"),(Array<Member>){&members[504],2});

  InstantiateTemplate(STRING("Array<const char>"));
  InstantiateTemplate(STRING("Range<Expression *>"));
  InstantiateTemplate(STRING("WireTemplate<int>"));
  InstantiateTemplate(STRING("WireTemplate<ExpressionRange>"));
  InstantiateTemplate(STRING("ExternalMemoryTwoPortsTemplate<int>"));
  InstantiateTemplate(STRING("ExternalMemoryTwoPortsTemplate<ExpressionRange>"));
  InstantiateTemplate(STRING("ExternalMemoryDualPortTemplate<int>"));
  InstantiateTemplate(STRING("ExternalMemoryDualPortTemplate<ExpressionRange>"));
  InstantiateTemplate(STRING("ExternalMemoryTemplate<int>"));
  InstantiateTemplate(STRING("ExternalMemoryTemplate<ExpressionRange>"));
  InstantiateTemplate(STRING("ExternalMemoryInterfaceTemplate<int>"));
  InstantiateTemplate(STRING("ExternalMemoryInterfaceTemplate<ExpressionRange>"));
  InstantiateTemplate(STRING("Hashmap<FUInstance *, FUInstance *>"));
  InstantiateTemplate(STRING("Hashmap<PortEdge, PortEdge>"));
  InstantiateTemplate(STRING("Hashmap<Edge *, Edge *>"));
  InstantiateTemplate(STRING("Hashmap<InstanceNode *, InstanceNode *>"));
  InstantiateTemplate(STRING("Hashmap<String, FUInstance *>"));
  InstantiateTemplate(STRING("Set<String>"));
  InstantiateTemplate(STRING("Pair<HierarchicalName, HierarchicalName>"));

  RegisterTypedef(STRING("Array<char>"),STRING("String"));
}
