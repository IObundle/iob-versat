#include <argp.h>
#include <unordered_map>
#include <sys/wait.h>
#include <ftw.h>

// TODO: Eventually need to find a way of detecting superfluous includes or something to the same effect. Maybe possible change to a unity build althoug the main problem to solve is organization.

#include "debugVersat.hpp"
#include "declaration.hpp"
#include "filesystem.hpp"
#include "globals.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "structParsing.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "utils.hpp"
#include "verilogParsing.hpp"
#include "type.hpp"
#include "templateEngine.hpp"
#include "textualRepresentation.hpp"
#include "codeGeneration.hpp"
#include "accelerator.hpp"
#include "dotGraphPrinting.hpp"
#include "versatSpecificationParser.hpp"
#include "symbolic.hpp"
#include "parser.hpp"

#include <filesystem>
namespace fs = std::filesystem;

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

// TODO: Small fix for common template. Works for now 
void SetIncludeHeader(CompiledTemplate* tpl,String name);

static Value HexValue(Value in,Arena* out){
  static char buffer[128];

  int number = ConvertValue(in,ValueType::NUMBER,nullptr).number;

  int size = sprintf(buffer,"0x%x",number);

  Value res = {};
  res.type = ValueType::STRING;
  res.str = PushString(out,String{buffer,size});

  return res;
}

static Value Identify(Value val,Arena* out){
  //Assert(val.type == GetType(STRING("FUInstance*")));

  FUInstance* inst;
  if(val.type == GetType(STRING("FUInstance*"))){
    FUInstance** instPtr = (FUInstance**) val.custom;
    inst = *instPtr;
  } else if(val.type == GetType(STRING("FUInstance"))){
    inst = (FUInstance*) val.custom;
  } else {
    Assert(false);
  }
  
  String ret = PushString(out,"%.*s_%d",UNPACK_SS(inst->name),inst->id);
  
  return MakeValue(ret);
}

#include "templateData.hpp"

void LoadTemplates(Arena* perm){
  TEMP_REGION(temp,perm);
  CompiledTemplate* commonTpl = CompileTemplate(versat_common_template,"common",perm);
  SetIncludeHeader(commonTpl,STRING("common"));

  BasicTemplates::acceleratorTemplate = CompileTemplate(versat_accelerator_template,"accel",perm);
  BasicTemplates::topAcceleratorTemplate = CompileTemplate(versat_top_instance_template,"top",perm);
  BasicTemplates::topConfigurationsTemplate = CompileTemplate(versat_configurations_template,"top_configurations",perm);
  BasicTemplates::acceleratorHeaderTemplate = CompileTemplate(versat_header_template,"header",perm);
  BasicTemplates::externalInternalPortmapTemplate = CompileTemplate(external_memory_internal_portmap_template,"ext_internal_port",perm);
  BasicTemplates::externalPortTemplate = CompileTemplate(external_memory_port_template,"ext_port",perm);
  BasicTemplates::externalInstTemplate = CompileTemplate(external_memory_inst_template,"ext_inst",perm);
  BasicTemplates::iterativeTemplate = CompileTemplate(versat_iterative_template,"iter",perm);
  BasicTemplates::internalWiresTemplate = CompileTemplate(versat_internal_memory_wires_template,"internal wires",perm);
  
  RegisterPipeOperation(STRING("MemorySize"),[](Value val,Arena* out){
    ExternalMemoryInterface* inter = (ExternalMemoryInterface*) val.custom;
    int byteSize = ExternalMemoryByteSize(inter);
    return MakeValue(byteSize);
  });
  RegisterPipeOperation(STRING("Hex"),HexValue);
  RegisterPipeOperation(STRING("Identify"),Identify);
  RegisterPipeOperation(STRING("Type"),[](Value val,Arena* out){
    Type* type = val.type;

    return MakeValue(type->name);
  });
}


// Only for graphs that we know for sure are DAG
Array<int> CalculateDAG(int maxNode,Array<Pair<int,int>> edges,int start,Arena* out){
  TEMP_REGION(temp,out);

  int NOT_SEEN = 0; 
  int WAIT_CHILDREN = 1;
  int PERMANENT = 2;

  int size = maxNode;
  Stack<int>* toSee = PushQueue<int>(temp,size * 2);
  Array<int> marked = PushArray<int>(temp,size);
  Memset(marked,NOT_SEEN);
  
  toSee->Push(start);

  auto arr = StartArray<int>(out);
  while(toSee->Size()){
    int head = toSee->Pop();

    if(marked[head] == WAIT_CHILDREN){
      marked[head] = PERMANENT;
      *arr.PushElem() = head;
      continue;
    }
    
    if(marked[head] != NOT_SEEN){
      continue;
    }

    bool firstSon = true;
    for(auto p : edges){
      if(p.first == head){
        if(!marked[p.second]){
          if(firstSon){
            firstSon = false;
            marked[head] = WAIT_CHILDREN;
            // Basically using the stack to accomplish two things. Store the children and to store another visit to the parent node, which is now marked with WAIT_CHILDREN and we only arrive at this after we have seen all the children.
            // The parent node can only be visit after all the children have been visited, since we are working on graphs that we know are DAG. 
            toSee->Push(head); // So we are sure to reach the WAIT_CHILDREN check
          }
          toSee->Push(p.second);
        }
      }
    }

    // No sons
    if(firstSon == true){
      marked[head] = PERMANENT;
      *arr.PushElem() = head;
    }
  }
  return EndArray(arr);
}

// This structure needs to represent the entire work that is required to perform 
struct Work{
  TypeDefinition definition;

  bool calculateDelayFixedGraph;
  bool flattenWithMapping;
};

void Print(Work* work){
  printf("Name: %.*s\n",UNPACK_SS(work->definition.base.name));
  printf("calculateDelayFixedGraph: %d\n",work->calculateDelayFixedGraph ? 1 : 0);
  printf("flattenWithMapping: %d\n",work->flattenWithMapping ? 1 : 0);
}

void GetSubWorkRequirement(Hashmap<String,Work>* typeToWork,TypeDefinition type){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  Array<Token> subTypesUsed = TypesUsed(type,temp);
  
  for(Token tok : subTypesUsed){
    Work* work = typeToWork->Get(tok);
    if(!work){
      continue;
    }

    // For now, assume that we want everything
    work->calculateDelayFixedGraph = true;
    work->flattenWithMapping = true;
  }
}

struct OptionsGather{
  ArenaList<String>* verilogFiles;
  ArenaList<String>* extraSources;
  ArenaList<String>* includePaths;
  ArenaList<String>* unitPaths;

  Options* options;
};

static int
parse_opt (int key, char *arg,
           argp_state *state)
{
  OptionsGather* opts = (OptionsGather*) state->input;

  // TODO: Better error handling
  switch (key)
    {
    case 'S': *opts->extraSources->PushElem() = STRING(arg); break;
    case 'I': *opts->includePaths->PushElem() = STRING(arg); break;
    case 'u': *opts->unitPaths->PushElem() = STRING(arg); break;

    case 'b': opts->options->databusDataSize = ParseInt(STRING(arg)); break;
    case 'x': opts->options->databusAddrSize = ParseInt(STRING(arg)); break;

    case 'd': opts->options->useDMA = true; break;
    case 'D': opts->options->architectureHasDatabus = true; break;
    case 's': opts->options->addInputAndOutputsToTop = true; break;

    case 'p': opts->options->copyUnitsConvenience = true; break;
    case 'L': opts->options->generetaSourceListName = STRING(arg); break;

    case 'g': opts->options->debugPath = STRING(arg); opts->options->debug = true; break;
    case 't': opts->options->topName = STRING(arg); break;
    case 'o': opts->options->hardwareOutputFilepath = STRING(arg); break;
    case 'O': opts->options->softwareOutputFilepath = STRING(arg); break;
      
    case ARGP_KEY_ARG: opts->options->specificationFilepath = STRING(arg); break;
    case ARGP_KEY_END: break;
    }
  return 0;
}

// TODO: Better error handling
struct argp_option options[] =
  {
    { 0, 'S',"File", 0, "Extra sources"},
    { 0, 'b',"Size", 0, "Databus size connected to external memory (8,16,default:32,64,128,256)"},
    { 0, 'x',"Size", 0, "Address size (default:32,64)"},
    { 0, 'd', 0,     0, "Use DMA"},
    { 0, 'D', 0,     0, "Architecture has databus"},
    { 0, 'I',"Path", 0, "Include paths"},
    { 0, 't',"Name", 0, "Top unit name"},
    { 0, 'p', 0,     0, "Versat also copies the source files of the units, including custom ones"},
    { 0, 'L',"Name", 0, "Writes to a file a list of all files generated by Versat"},
    { 0, 's', 0,     0, "Insert consts and regs if Top unit contains inputs and outputs"},
    { 0, 'u',"Path", 0, "Units path"},
    { 0, 'g',"Path", OPTION_HIDDEN, "Debug path"},
    { 0, 'o',"Path", 0, "Hardware output path"},
    { 0, 'O',"Path", 0, "Software output path"},
    { 0 }
  };

int main(int argc,char* argv[]){
  InitDebug();

  void SetTemplateNameToContent(Array<Pair<String,String>> val); // Kinda of an hack.
  SetTemplateNameToContent(generated_templateNameToContent);
  
  Arena globalPermanentInst = InitArena(Megabyte(128));
  globalPermanent = &globalPermanentInst;
  Arena tempInst = InitArena(Megabyte(128));
  Arena temp2Inst = InitArena(Megabyte(128));
  
  contextArenas[0] = &tempInst;
  contextArenas[1] = &temp2Inst;

  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  Arena* perm = globalPermanent;
  
  argp argp = { options, parse_opt, "SpecFile", "Dataflow to accelerator compiler. Check tutorial in https://github.com/IObundle/iob-versat to learn how to write a specification file"};

  OptionsGather gather = {};
  gather.verilogFiles = PushArenaList<String>(temp);
  gather.extraSources = PushArenaList<String>(temp);
  gather.includePaths = PushArenaList<String>(temp);
  gather.unitPaths = PushArenaList<String>(temp);

  globalOptions = DefaultOptions(perm);
  gather.options = &globalOptions;

  if(argp_parse(&argp, argc, argv, 0, 0, &gather) != 0){
    printf("Error parsing arguments. Call -h help to print usage and argument help\n");
    return -1;
  }

  if(Empty(globalOptions.topName)){
    printf("Need to specify top unit with -t\n");
    exit(-1);
  }

  globalOptions.verilogFiles = PushArrayFromList(perm,gather.verilogFiles);
  globalOptions.extraSources = PushArrayFromList(perm,gather.extraSources);
  globalOptions.includePaths = PushArrayFromList(perm,gather.includePaths);
  globalOptions.unitPaths = PushArrayFromList(perm,gather.unitPaths);
  
  RegisterTypes();

  void RegisterParsedTypes();
  RegisterParsedTypes();

  void AfterRegisteringParsedTypes();
  AfterRegisteringParsedTypes();
  
  InitializeTemplateEngine(perm);
  LoadTemplates(perm);
  InitializeSimpleDeclarations();

  globalDebug.outputAccelerator = true;
  globalDebug.outputAcceleratorInfo = true;
  globalDebug.outputVersat = true;
  globalDebug.outputGraphs = true;
  globalDebug.outputConsolidationGraphs = true;
  globalDebug.outputVCD = true;

  // Collect all verilog source files.
  Array<String> allVerilogFiles = {};
  region(temp){
    Set<String>* allVerilogFilesSet = PushSet<String>(temp,999);

    for(String str : globalOptions.verilogFiles){
      allVerilogFilesSet->Insert(str);
    }
    for(String& path : globalOptions.unitPaths){
      String dirPaths = path;
      Tokenizer pathSplitter(dirPaths,"",{});

      while(!pathSplitter.Done()){
        Token path = pathSplitter.NextToken();

        Opt<Array<String>> res = GetAllFilesInsideDirectory(path,temp);
        if(!res){
          printf("\n\nCannot open dir: %.*s\n\n",UNPACK_SS(path));
        } else {
          for(String& str : res.value()){
            String fullPath = PushString(perm,"%.*s/%.*s",UNPACK_SS(path),UNPACK_SS(str));
            allVerilogFilesSet->Insert(fullPath);
          }
        }
      }
    }

    allVerilogFiles = PushArrayFromSet(perm,allVerilogFilesSet);
  }
  globalOptions.verilogFiles = allVerilogFiles; // TODO: Kind of an hack. We lose information about file origin (from user vs from us)
  
  // Parse verilog files and register all the simple units.
  for(String filepath : globalOptions.verilogFiles){
    String content = PushFile(temp,filepath);

    if(Empty(content)){
      printf("Failed to open file %.*s\n. Exiting\n",UNPACK_SS(filepath));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm);
      RegisterModuleInfo(&info);
    }
  }

  // We need to do this after parsing the modules because the majority of these special types come from verilog files
  // More robust impl would embed this files at build time to simplify setup. Only need the exe to run Versat at that point. 
  BasicDeclaration::buffer = GetTypeByName(STRING("Buffer"));
  BasicDeclaration::fixedBuffer = GetTypeByName(STRING("FixedBuffer"));
  BasicDeclaration::pipelineRegister = GetTypeByName(STRING("PipelineRegister"));
  BasicDeclaration::multiplexer = GetTypeByName(STRING("Mux2"));
  BasicDeclaration::combMultiplexer = GetTypeByName(STRING("CombMux2"));
  BasicDeclaration::stridedMerge = GetTypeByName(STRING("StridedMerge"));
  BasicDeclaration::timedMultiplexer = GetTypeByName(STRING("TimedMux"));
  BasicDeclaration::input = GetTypeByName(STRING("CircuitInput"));
  BasicDeclaration::output = GetTypeByName(STRING("CircuitOutput"));

  String specFilepath = globalOptions.specificationFilepath;
  String topLevelTypeStr = globalOptions.topName;

  // Basically using a simple DAG approach to detect the modules that we only care about. We do not process modules that are not needed

  // TODO: Simplify this part. 
  FUDeclaration* simpleType = GetTypeByName(topLevelTypeStr);
  
  if(!simpleType && specFilepath.size && !CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(specFilepath)));
    
    Array<TypeDefinition> types = ParseVersatSpecification(content,temp);
    int size = types.size;
    
    Hashmap<String,int>* typeToId = PushHashmap<String,int>(temp,size);
    for(int i = 0; i < size; i++){
      typeToId->Insert(types[i].base.name,i);
    }
    
    if(!typeToId->Exists(topLevelTypeStr)){
      printf("Did not find the top level type: %.*s\n",UNPACK_SS(topLevelTypeStr));
      return -1;
    }
    
    auto arr = StartArray<Pair<int,int>>(temp2);
    for(int i = 0; i < size; i++){
      Array<Token> subTypesUsed = TypesUsed(types[i],temp);

      for(String str : subTypesUsed){
        int* index = typeToId->Get(str);
        if(index){
          *arr.PushElem() = {i,*index};
        }
      }
    }
    Array<Pair<int,int>> edges = EndArray(arr);
    
    Array<int> order = CalculateDAG(size,edges,typeToId->GetOrFail(topLevelTypeStr),temp);

    // Represents all the work that we need to do.
    Hashmap<String,Work>* typeToWork = PushHashmap<String,Work>(temp,order.size);

    for(int i : order){
      Work work = {};
      String name = types[i].base.name;
      work.definition = types[i];
      
      typeToWork->Insert(name,work);
    }
    
    for(int i : order){
      TypeDefinition type = types[i];
      GetSubWorkRequirement(typeToWork,type);
    }

    // For the TOP unit, currently we do everything:
    Work* topWork = &typeToWork->GetOrFail(topLevelTypeStr);
    topWork->calculateDelayFixedGraph = true;
    topWork->flattenWithMapping = true;

    for(auto p : typeToWork){
      Work work = *p.second;

      FUDeclaration* decl = nullptr;
      
      if(work.definition.type == DefinitionType_MODULE){
        decl = InstantiateBarebonesSpecifications(content,p.second->definition);
      } else if(work.definition.type == DefinitionType_MERGE){
        decl = InstantiateSpecifications(content,p.second->definition);
      }
      decl->signalLoop = true;
      
#if 0
      if(work.calculateDelayFixedGraph){
        Accelerator* copy = CopyAccelerator(decl->baseCircuit,AcceleratorPurpose_FIXED_DELAY,true,nullptr);

        DAGOrderNodes order = CalculateDAGOrder(&copy->allocated,temp);
        CalculateDelayResult delays = CalculateDelay(copy,order,temp);

        decl->baseConfig.calculatedDelays = PushArray<int>(perm,delays.nodeDelay->nodesUsed);
        Memset(decl->baseConfig.calculatedDelays,0);
        int index = 0;
        for(Pair<FUInstance*,DelayInfo*> p : delays.nodeDelay){
          if(p.first->declaration->baseConfig.delayOffsets.max > 0){
            decl->baseConfig.calculatedDelays[index] = p.second->value;
            index += 1;
          }
        }

        region(temp){
          FixDelays(copy,delays.edgesDelay,temp);
        }

        decl->fixedDelayCircuit = copy;
        decl->fixedDelayCircuit->name = decl->name;

        FillDeclarationWithDelayType(decl);
      }
#endif

      // Flatten with mapping seems to be specific to modules.
      // Merge circuits are already flatten by the way the merge is performed.
      if(work.definition.type != DefinitionType_MERGE && work.flattenWithMapping){
        Pair<Accelerator*,SubMap*> p = Flatten(decl->baseCircuit,99);
  
        decl->flattenedBaseCircuit = p.first;
        decl->flattenMapping = p.second;
      }

      //Print(p.second);
    }
  }

  FUDeclaration* type = GetTypeByName(topLevelTypeStr);
  if(!type && !CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    printf("Did not find the top level type: %.*s\n",UNPACK_SS(topLevelTypeStr));
    return -1;
  }

  Accelerator* accel = nullptr;
  FUInstance* TOP = nullptr;

  bool isSimple = false;
  if(CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    accel = CreateAccelerator(STRING("allVersatUnits"),AcceleratorPurpose_MODULE);
    
    FUDeclaration* constType = GetTypeByName(STRING("Const"));
    FUDeclaration* regType = GetTypeByName(STRING("Reg"));

    int constsAdded = 0;
    int regsAdded = 0;
    for(FUDeclaration* decl : globalDeclarations){
      if(decl->type == FUDeclarationType_SINGLE){
        int input = decl->NumberInputs();
        int output = decl->NumberOutputs();

        FUInstance* testUnit = CreateFUInstance(accel,decl,decl->name);
        
        for(int i = 0; i < input; i++){
          String name = PushString(perm,"const%d",constsAdded++);
          FUInstance* inUnit = CreateFUInstance(accel,constType,name);

          ConnectUnits(inUnit,0,testUnit,i);
        }

        for(int i = 0; i < output; i++){
          String name = PushString(perm,"reg%d",regsAdded++);
          FUInstance* outUnit = CreateFUInstance(accel,regType,name);

          ConnectUnits(testUnit,i,outUnit,0);
        }
      }
    }
    
    type = RegisterSubUnit(accel);
    type->signalLoop = true;

    accel = CreateAccelerator(topLevelTypeStr,AcceleratorPurpose_MODULE);
    TOP = CreateFUInstance(accel,type,STRING("TOP"));
  } else if(globalOptions.addInputAndOutputsToTop && !(type->NumberInputs() == 0 && type->NumberOutputs() == 0)){
    // TODO: This process needs to be simplified and more integrated with the approach used by versat spec.
    //       Instead of doing everything manually.
    isSimple = true;

    const char* name = StaticFormat("%.*s_Simple",UNPACK_SS(topLevelTypeStr));
    accel = CreateAccelerator(STRING(name),AcceleratorPurpose_MODULE);
    
    int input = type->NumberInputs();
    int output = type->NumberOutputs();

    Array<FUInstance*> inputs = PushArray<FUInstance*>(perm,input);
    Array<FUInstance*> outputs = PushArray<FUInstance*>(perm,output);

    FUDeclaration* constType = GetTypeByName(STRING("TestConst"));
    FUDeclaration* regType = GetTypeByName(STRING("Reg"));
    
    // We need to create input and outputs first before instance
    // to guarantee that the configs and states are at the beginning of the accelerator structs
    for(int i = 0; i < input; i++){
      String name = PushString(perm,"input_%d",i);
      inputs[i] = CreateFUInstance(accel,constType,name);
    }
    for(int i = 0; i < output; i++){
      String name = PushString(perm,"output_%d",i);
      outputs[i] = CreateFUInstance(accel,regType,name);
    }

    TOP = CreateFUInstance(accel,type,STRING("simple"));

    for(int i = 0; i < input; i++){
      ConnectUnits(inputs[i],0,TOP,i);
    }
    for(int i = 0; i < output; i++){
      ConnectUnits(TOP,i,outputs[i],0);
    }
    
    type = RegisterSubUnit(accel);
    type->definitionArrays = PushArray<Pair<String,int>>(perm,2);
    type->definitionArrays[0] = (Pair<String,int>){STRING("input"),input};
    type->definitionArrays[1] = (Pair<String,int>){STRING("output"),output};

    type->signalLoop = true;
    
    name = StaticFormat("%.*s_Simple",UNPACK_SS(topLevelTypeStr));
    accel = CreateAccelerator(STRING(name),AcceleratorPurpose_MODULE);
    TOP = CreateFUInstance(accel,type,STRING("TOP"));
  } else {
    accel = CreateAccelerator(topLevelTypeStr,AcceleratorPurpose_MODULE);

    TOP = CreateFUInstance(accel,type,STRING("TOP"));

    for(int i = 0; i < type->NumberInputs(); i++){
      String name = PushString(perm,"input%d",i);
      FUInstance* input = CreateOrGetInput(accel,name,i);
      ConnectUnits(input,0,TOP,i);
    }
    FUInstance* output = CreateOrGetOutput(accel);
    for(int i = 0; i < type->NumberOutputs(); i++){
      ConnectUnits(TOP,i,output,i);
    }
  }

  if(!SetParameter(TOP,STRING("AXI_ADDR_W"),STRING("AXI_ADDR_W"))){
    printf("Error\n");
  }
  SetParameter(TOP,STRING("LEN_W"),STRING("LEN_W"));

  OutputTopLevelFiles(accel,type,
                     globalOptions.hardwareOutputFilepath.data,
                     globalOptions.softwareOutputFilepath.data,
                     isSimple);

  for(FUDeclaration* decl : globalDeclarations){
    BLOCK_REGION(temp);

    if(decl->type == FUDeclarationType_COMPOSITE ||
       decl->type == FUDeclarationType_ITERATIVE ||
       decl->type == FUDeclarationType_MERGED){

      if(globalOptions.debug){
        GraphPrintingContent content = GenerateDefaultPrintingContent(decl->fixedDelayCircuit,temp);
        String repr = GenerateDotGraph(content,temp);
        String debugPath = PushDebugPath(temp,decl->name,STRING("NormalGraph.dot"));

        FILE* file = OpenFile(debugPath,"w",FilePurpose_DEBUG_INFO);
        DEFER_CLOSE_FILE(file);
        if(!file){
          printf("Error opening file for debug outputting: %.*s\n",UNPACK_SS(debugPath));
        } else {
          fwrite(repr.data,sizeof(char),repr.size,file);
        }
      }

      String path = PushString(temp,"%.*s/modules/%.*s.v",UNPACK_SS(globalOptions.hardwareOutputFilepath),UNPACK_SS(decl->name));
      
      FILE* sourceCode = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_CODE);
      DEFER_CLOSE_FILE(sourceCode);

      if(decl->type == FUDeclarationType_COMPOSITE || decl->type == FUDeclarationType_MERGED){
        OutputCircuitSource(decl,sourceCode);
      } else if(decl->type == FUDeclarationType_ITERATIVE){
        OutputIterativeSource(decl,sourceCode);
      }
    }
  }

  // TODO: We probably need to move this to a better place since we also need to copy include files in order to separate build from setup. We need a lot of file copying and it's probably best to put all in one place.
  fs::path hardwareDestinationPath(StaticFormat("%.*s",UNPACK_SS(globalOptions.hardwareOutputFilepath)));
  auto options = fs::copy_options::update_existing;
  for(String filepath : globalOptions.verilogFiles){
    fs::path path(StaticFormat("%.*s",UNPACK_SS(filepath)));
    
    fs::copy(path,hardwareDestinationPath,options);
  }
  
  // This should be the last thing that we do, no further file creation can occur after this point
  for(FileInfo f : CollectAllFilesInfo(temp)){
    if(f.purpose != FilePurpose_DEBUG_INFO && f.mode == FileOpenMode_WRITE){
      const char* type = FilePurpose_Name(f.purpose);
      printf("Filename: %.*s Type: %s\n",UNPACK_SS(f.filepath),type);
    }
  }

  return 0;
}

/*

BUG: Since the name of the units are copied directly to the header file, it is possible to have conflict with C reserved keywords, like const, static, and stuff like that. 

BUG: An empty accelerator is crashing versat. We do not need to do anything special, generate an empty accelerator or just give an error (an empty accelerator would just be the dma at that point, right?).
     - Regardless, generating an empty accelerator might be a good way of testing whether the generated code is capable of handling empty structs (empty config, empty state, empty static, empty mem, etc.) because other accelerators might have one of these interfaces empty and in theory the empty accelerator would have all of them empty and as such we could test everything here.

BUG: Partial share is missing in the spec parser for multiple units with different names. Only working for arrays.

There is probably a lot of cleanup left (overall and inside Merge).

Need to take a look at State and Mem struct interfaces. State is not taking into account merge types and neither is Mem. Also need to see if we can generate the Mem as an actual structure where the programmer must use pointer arithmetic directly.

*/

/*

Generated code does not take into account parameters when it should.
- Some wires are given fixed sizes when they depend on verilog parameters. 

Debugability:

- We kinda lost the .dot files generated in order to help debug graph algorithms. Its not a big deal now but eventually would like to have them back, as the .dot files are the only proper way we currently have of debugging graph operations (which I do not think we will be doing much soon, but eventually need to address this).

Features:

- The template engine could be updated to compile at build time, offer compile time checks instead of runtime and simplify data management since we could pass and use C code to access and iterate the data. This means that passing large structures is preferebly instead of doing what we do now where we want to minimize data passing this way. Read the NOTEs that are contained inside the OutputTopLevelFiles.

- The Address gen are kinda adhoc in the generated code. Need to find a way of binding them to the accelerator that uses them (some declaration inside the module/merge that indicates to Versat that the address gen is intended to be used for that unit.). As a consequence of this, we might need to generate different address gens for different structs because of merge. I think I have a good enough merge backbone to implement this in a couple of days, but I also would probably like to finalize the static portion before progressing to tackle this problem.

- We could implement a shared delay as well as config. To make it even more powerful, we could make it so we could share the delay of some units and the config of others and have it overlap somewhat. How I would represent that in syntax I do not know.

Wrapper:

- The databus does not take into consideration the strobe of the databus.

Parser:

- I should be able to use write: "share config Type unit[N];", instead of having to write: "share config Type{ unit[N]; }"

*/

/*

TODO?

DelayType and NodeType need a revision.

The hardware is full of sloppy code.
   Start standardizing and pulling up modules that abstract common functionality.
   Maybe look into generating a verilog testbench for individual units that performs a number of tests depending on the interfaces that the unit provides.
      Should be easy, just one more template.

There are a dozens of TODOs scathered all over the codebase. Start by fixing some of them.

Generated code does not take into account parameters when it should.
   Some wires are given fixed sizes when they depend on verilog parameters. 

*/

/*


TODO: Need to check the isSource member of ModuleInfo and so on. 

TODO: Because of the delay implementation, there is a possibility of allowing inter-hierarchy optimizations:
      Think a reg file module that is instantiated inside another module. Each reg is forced to have the same delay because we only calculate the module as a single unit, set the module delay, and when instantiating it, we take the module delay as unchangeable.
      This requires to keep track for each module which one allows this to happen and then take that into account in the delay calculation functions. Note: Input delay does not work. We are talking about delay info propagating downards accross the hiearchy while input delay propagates upwards. We can still resolve this during the register FUDeclaration by calcuting the delays of lower units inside the register of the upper unit.
      For this part, we would basically have to implement greater info on module connectivity (how each output interacts with each input and stuff like that).

 */
