#include <argp.h>
#include <sys/wait.h>
#include <ftw.h>

// TODO: Eventually need to find a way of detecting superfluous includes or something to the same effect. Maybe possible change to a unity build although the main problem to solve is organization.
#include "accelerator.hpp"
#include "configurations.hpp"
#include "debugVersat.hpp"
#include "embeddedData.hpp"
#include "filesystem.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "parser.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versatSpecificationParser.hpp"
#include "declaration.hpp"
#include "templateEngine.hpp"
#include "codeGeneration.hpp"
#include "addressGen.hpp"

#include <filesystem>
namespace fs = std::filesystem;

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

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
  ConstructDef definition;

  bool calculateDelayFixedGraph;
  bool flattenWithMapping;
};

void Print(Work* work){
  printf("Name: %.*s\n",UN(work->definition.base.name));
  printf("calculateDelayFixedGraph: %d\n",work->calculateDelayFixedGraph ? 1 : 0);
  printf("flattenWithMapping: %d\n",work->flattenWithMapping ? 1 : 0);
}

void GetSubWorkRequirement(Hashmap<String,Work>* typeToWork,ConstructDef type){
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

int CopyFileGroup(Array<FileContent> fileGroup,String filepathBase,bool flattenedDirs,FilePurpose purpose){
  TEMP_REGION(temp,nullptr);
    
  String common = fileGroup[0].originalRelativePath;
  for(FileContent content : fileGroup){
    common = GetCommonPath(common,content.originalRelativePath,temp);
  }
  
  for(FileContent content : fileGroup){
    String pathWithoutCommon = Offset(content.originalRelativePath,common.size);

    String pathWithExpectedCommon = {};
    if(flattenedDirs){
      // Nothing
    } else if(Empty(content.commonFolder)){
      pathWithExpectedCommon = PushString(temp,".%.*s",UN(pathWithoutCommon));
      // Nothing
    } else {
      pathWithExpectedCommon = PushString(temp,"%.*s/%.*s",UN(content.commonFolder),UN(pathWithoutCommon));
    }

    String fullPath = PushString(temp,"%.*s/%.*s",UN(filepathBase),UN(pathWithExpectedCommon));
    fullPath = OS_NormalizePath(fullPath,temp);
    fullPath = PushString(temp,"%.*s/%.*s",UN(fullPath),UN(content.fileName));

    FILE* file = OpenFileAndCreateDirectories(fullPath,"w",purpose);
    
    if(!file){
      printf("Error opening file for output: %.*s\n",UN(fullPath));
      return -1;
      // TODO(Error)
    }
    
    String data = content.content;
    fwrite(data.data,sizeof(char),data.size,file);
    fclose(file);
  }

  return 0;
};

struct OptionsGather{
  ArenaList<String>* verilogFiles; // Individual files, for cases where we want specific files inside a folder.
  ArenaList<String>* unitFolderPaths;
  ArenaList<String>* extraSources;
  ArenaList<String>* includePaths;

  Options* options;
};

static int
parse_opt (int key, char *arg,
           argp_state *state){
  OptionsGather* opts = (OptionsGather*) state->input;

  // TODO: Better error handling
  switch (key)
    {
    case 'S': *opts->extraSources->PushElem() = arg; break;
    case 'I': *opts->includePaths->PushElem() = arg; break;

    // TODO: All the filepaths should be inserted into verilogFiles while this only takes in folders.
    case 'u': *opts->unitFolderPaths->PushElem() = arg; break;

    case 'b': opts->options->databusDataSize = ParseInt(arg); break;

    case 'd': opts->options->useDMA = true; break;
    case 'D': opts->options->architectureHasDatabus = true; break;

    case 'L': opts->options->generetaSourceListName = arg; break;

    case 'p': opts->options->prefixIObPort = arg; break;

    case 'T': opts->options->opMode = VersatOperationMode_GENERATE_TESTBENCH; break;

    case 'E': {
      opts->options->extraIOb = true;
      opts->options->useSymbolAddress = true;
    } break;

    case 128: {
      opts->options->insertDebugRegisters = true;
    } break;

    case 129: {
      opts->options->insertProfilingRegisters = true;
    } break;
      
    case 'g': opts->options->debugPath = arg; opts->options->debug = true; break;
    case 't': opts->options->topName = arg; break;
    case 'o': opts->options->hardwareOutputFilepath = arg; break;
    case 'O': opts->options->softwareOutputFilepath = arg; break;
      
    case ARGP_KEY_ARG: opts->options->specificationFilepath = arg; break;
    case ARGP_KEY_END: break;
    }
  return 0;
}

// TODO: Better error handling
struct argp_option options[] =
  {
    { "debug", 128 ,0, 0, "Insert debug registers on the generated accelerator"},
    { "profile", 129 ,0, 0, "Insert profiling registers on the generated accelerator"},
    { 0, 'b',"Size",   0, "Databus size connected to external RAM (8,16,default:32,64,128,256)"},
    { 0, 'd', 0,       0, "Use DMA"},
    { 0, 'D', 0,       0, "Architecture has databus"},
    { 0, 'E', 0,       0, "Enables a set of hardcoded options that is mostly some hacks for other projects (which in the future I hope to properly implement as proper configurable parameters)"},
    { 0, 'g',"Path",   OPTION_HIDDEN, "Debug path"},
    { 0, 'S',"File",   0, "Extra sources"},
    { 0, 'I',"Path",   0, "Include paths"},
    { 0, 'L',"Name",   0, "Writes to a file a list of all files generated by Versat"},
    { 0, 'o',"Path",   0, "Hardware output path"},
    { 0, 'O',"Path",   0, "Software output path"},
    { 0, 'p',"Prefix", 0, "Prefix the IOb input port with user provided content"},
    { 0, 't',"Name",   0, "Top unit name"},
    { 0, 'T',0,        0, "Generate a testbench for the given unit that exercises it as if being used by an accelerator."},
    { 0, 'u',"Path",   0, "Units path"},
    { 0 }
  };

void ReportFileCreation(bool allFiles = false){
  TEMP_REGION(temp,nullptr);
  for(FileInfo f : CollectAllFilesInfo(temp)){
    if(allFiles || (f.purpose != FilePurpose_DEBUG_INFO && f.mode == FileOpenMode_WRITE)){
      String type = FilePurpose_Name(f.purpose);
      printf("Filename: %.*s Type: %.*s\n",UN(f.filepath),UN(type));
    }
  }
}

// TODO: Remove this
void InitializeUserConfigs();

int main(int argc,char* argv[]){
#ifdef VERSAT_DEBUG
  printf("Running in debug mode\n");
#endif
  
  Arena globalPermanentInst = InitArena(Megabyte(128));
  globalPermanent = &globalPermanentInst;
  Arena tempInst = InitArena(Megabyte(128));
  Arena temp2Inst = InitArena(Megabyte(128));

  contextArenas[0] = &tempInst;
  contextArenas[1] = &temp2Inst;
  
  InitDebug(argv[0]);

  for(int i = 0; i < 8; i++){
    singleUseCasesArenas[i] = InitArena(Megabyte(1));
  }
  
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  
  Arena* perm = globalPermanent;
  
  InitializeDefaultData(perm);
  TE_Init();
  InitializeSimpleDeclarations();
  InitializeUserConfigs();
  InitParser(perm);
  
  argp argp = { options, parse_opt, "SpecFile\n-T UnitName", "Dataflow to accelerator compiler. Check tutorial in https://github.com/IObundle/iob-versat to learn how to write a specification file"};

  OptionsGather gather = {};
  gather.verilogFiles = PushList<String>(temp);
  gather.extraSources = PushList<String>(temp);
  gather.includePaths = PushList<String>(temp);
  gather.unitFolderPaths = PushList<String>(temp);

  globalOptions = DefaultOptions(perm);
  gather.options = &globalOptions;

  if(argp_parse(&argp, argc, argv, 0, 0, &gather) != 0){
    printf("Error parsing arguments. Call -h help to print usage and argument help\n");
    return -1;
  }
  
  globalOptions.verilogFiles = PushArray(perm,gather.verilogFiles);
  globalOptions.extraSources = PushArray(perm,gather.extraSources);
  globalOptions.includePaths = PushArray(perm,gather.includePaths);
  globalOptions.unitFolderPaths = PushArray(perm,gather.unitFolderPaths);
  
  if(globalOptions.opMode == VersatOperationMode_GENERATE_TESTBENCH){
    globalOptions.topName = globalOptions.specificationFilepath;
  }

  globalOptions.hardwareOutputFilepath = OS_NormalizePath(globalOptions.hardwareOutputFilepath,temp);
  globalOptions.softwareOutputFilepath = OS_NormalizePath(globalOptions.softwareOutputFilepath,temp);

  globalDebug.outputGraphs = true;
  globalDebug.outputConsolidationGraphs = true;
  globalDebug.outputVCD = true;
  
  if(Empty(globalOptions.topName)){
    char name[] = "versat";
    argp_help(&argp,stdout,ARGP_HELP_STD_HELP,name);
    printf("\nNeed to specify top unit with -t\n");
    exit(-1);
  }

  FREE_ARENA(moduleAccum);
  TrieMap<String,ModuleInfo>* allModules = PushTrieMap<String,ModuleInfo>(moduleAccum);

  bool anyError = false;
  for(FileContent file : defaultVerilogUnits){
    String content = file.content;
    
    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm);
      info.moduleSource = ModuleSource_DEFAULT_UNIT;

      allModules->Insert(info.name,info);
    }
  }

  bool error = false;
  Array<String> allVerilogFiles = {};
  region(temp){
    TrieSet<String>* allVerilogFilesSet = PushTrieSet<String>(temp);

    for(String str : globalOptions.verilogFiles){
      allVerilogFilesSet->Insert(str);
    }
    
    for(String& path : globalOptions.unitFolderPaths){
      String dirPaths = path;
      Tokenizer pathSplitter(dirPaths,"",{});

      while(!pathSplitter.Done()){
        Token path = pathSplitter.NextToken();

        Opt<Array<String>> res = GetAllFilesInsideDirectory(path,temp);
        if(!res){
          error = true;
          printf("\n\nCannot open dir: %.*s\n\n",UN(path));
        } else {
          for(String& str : res.value()){
            String fullPath = PushString(perm,"%.*s/%.*s",UN(path),UN(str));
            allVerilogFilesSet->Insert(fullPath);
          }
        }
      }
    }

    allVerilogFiles = PushArray(perm,allVerilogFilesSet);
  }
  globalOptions.verilogFiles = allVerilogFiles;

  for(String filepath : allVerilogFiles){
    String content = PushFile(temp,filepath);
    
    if(Empty(content)){
      printf("Failed to open file %.*s\n. Exiting\n",UN(filepath));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm);
      info.moduleSource = ModuleSource_USER_UNIT;

      auto res = allModules->GetOrAllocate(info.name);

      if(!error && res.alreadyExisted){
        printf("[WARNING] Unit %.*s is a default unit from Versat. Assuming that user wants to override it\n",UN(info.name));
        printf("Otherwise rename the unit in file: %.*s\n",UN(filepath));
      }

      *res.data = info;
    }
  }
  if(error){
    return -1;
  }

  for(Pair<String,ModuleInfo> p : allModules){
    RegisterModuleInfo(&p.second,perm);
  }
  
  // We need to do this after parsing the modules because the majority of these special types come from verilog files
  // NOTE: This should never fail since the verilog files are embedded into the exe. A fail in here means that we failed to embed the necessary files at build time
  BasicDeclaration::variableBuffer = GetTypeByNameOrFail("Buffer");
  BasicDeclaration::fixedBuffer = GetTypeByNameOrFail("FixedBuffer");
  BasicDeclaration::pipelineRegister = GetTypeByNameOrFail("PipelineRegister");
  BasicDeclaration::multiplexer = GetTypeByNameOrFail("Mux2");
  BasicDeclaration::combMultiplexer = GetTypeByNameOrFail("CombMux2");
  BasicDeclaration::stridedMerge = GetTypeByNameOrFail("StridedMerge");
  BasicDeclaration::timedMultiplexer = GetTypeByNameOrFail("TimedMux");
  BasicDeclaration::input = GetTypeByNameOrFail("CircuitInput");
  BasicDeclaration::output = GetTypeByNameOrFail("CircuitOutput");

  // TODO: The testbench logic is kinda addhoc right now. Need to join together the other logic and them create a proper switch between these two.
  if(globalOptions.opMode == VersatOperationMode_GENERATE_TESTBENCH){
    String topLevelUnit = globalOptions.topName;

    FUDeclaration* decl = GetTypeByName(topLevelUnit);

    if(!decl){
      printf("Unit of name '%.*s' does not exist\n",UN(topLevelUnit));
      return -1;
    }

    String path = PushString(temp,"%.*s/../%.*s_tb.v",UN(globalOptions.hardwareOutputFilepath),UN(decl->name));
    FILE* testbenchLocation = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(testbenchLocation);

    OutputTestbench(decl,testbenchLocation);

    int res = CopyFileGroup(defaultVerilogFiles,globalOptions.hardwareOutputFilepath,true,FilePurpose_VERILOG_COMMON_CODE);
    if(res){
      return res;
    }
    
    ReportFileCreation(true);
    return 0;
  }
  
  String specFilepath = globalOptions.specificationFilepath;
  String topLevelTypeStr = globalOptions.topName;

  // TODO: Simplify this part. 
  FUDeclaration* simpleType = GetTypeByName(topLevelTypeStr);

  if(!simpleType && specFilepath.size && !CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    String content = PushFile(temp,StaticFormat("%.*s",UN(specFilepath)));
    
    Array<ConstructDef> types = ParseVersatSpecification(content,temp);
    
    auto moduleLike = PushList<ConstructDef>(temp);
    for(ConstructDef def : types){
      if(IsModuleLike(def)){
        *moduleLike->PushElem() = def;
      }
    }
    auto modules = PushArray(temp,moduleLike);
    
    int size = modules.size;
    
    Hashmap<String,int>* typeToId = PushHashmap<String,int>(temp,size);
    for(int i = 0; i < size; i++){
      typeToId->Insert(modules[i].base.name,i);
    }
    
    if(!typeToId->Exists(topLevelTypeStr)){
      printf("Did not find the top level type: %.*s\n",UN(topLevelTypeStr));
      return -1;
    }
    
    auto arr = StartArray<Pair<int,int>>(temp2);
    for(int i = 0; i < size; i++){
      Array<Token> subTypesUsed = TypesUsed(modules[i],temp);

      for(String str : subTypesUsed){
        int* index = typeToId->Get(str);
        if(index){
          *arr.PushElem() = {i,*index};
        }
      }
    }
    Array<Pair<int,int>> edges = EndArray(arr);
    
    // Basically using a simple DAG approach to detect the modules that we only care about. We do not process modules that are not needed
    Array<int> order = CalculateDAG(size,edges,typeToId->GetOrFail(topLevelTypeStr),temp);

    // Represents all the work that we need to do.
    Hashmap<String,Work>* typeToWork = PushHashmap<String,Work>(temp,order.size);

    for(int i : order){
      Work work = {};
      Token name = modules[i].base.name;
      work.definition = modules[i];
      
      typeToWork->Insert(name,work);
    }
    
    for(int i : order){
      ConstructDef type = modules[i];
      GetSubWorkRequirement(typeToWork,type);
    }

    // We first validity check merge and if the types they are merging actually exist.
    bool anyError = false;
    for(auto p : typeToWork){
      Work work = *p.second;

      if(work.definition.type == ConstructType_MERGE){
        MergeDef merge = work.definition.merge;

        for(TypeAndInstance tp : merge.declarations){
          bool found = false;
          for(auto p : typeToWork){
            if(CompareString(p.first,tp.typeName)){
              found = true;
              break;
            }
          }

          if(!found){
            ReportError(content,tp.typeName,"Did not find type");
            anyError = true;
          }
        }
      }
    }

    if(anyError){
      return -1;
    }

    // TODO: We could push more, we can technically parse the modules even if we have address gen errors.
    if(anyError){
      return -1;
    }
    
    // For the TOP unit, currently we do everything:
    Work* topWork = &typeToWork->GetOrFail(topLevelTypeStr);
    topWork->calculateDelayFixedGraph = true;
    topWork->flattenWithMapping = true;
      
    for(auto p : typeToWork){
      Work work = *p.second;

      FUDeclaration* decl = InstantiateSpecifications(content,p.second->definition);
      decl->singleInterfaces |= SingleInterfaces_SIGNAL_LOOP;
      
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

#if 0
      if(work.definition.type == ConstructType_MODULE && work.definition.module.name == "ModuleWithExtra"){
        auto p = FlattenWithMerge(decl->baseCircuit,0);

        DebugRegionOutputDotGraph(p.accel,"FlattenReconAttemp0");
      }
#endif
      
      // Flatten with mapping seems to be specific to modules.
      // Merge circuits are already flatten by the way the merge is performed.
      if(work.definition.type != ConstructType_MERGE && work.flattenWithMapping){
        Pair<Accelerator*,SubMap*> p = Flatten(decl->baseCircuit,99);
        
        decl->flattenedBaseCircuit = p.first;
        decl->flattenMapping = p.second;
      }
    }
  }

  FUDeclaration* type = GetTypeByName(topLevelTypeStr);
  if(!type && !CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    printf("Did not find the top level type: %.*s\n",UN(topLevelTypeStr));
    return -1;
  }

  Accelerator* accel = nullptr;
  //FUInstance* TOP = nullptr;

  // NOTE: Was used to help with linting, since we want to produce an accelerator that utilizes everything.
  if(CompareString(topLevelTypeStr,"VERSAT_RESERVED_ALL_UNITS")){
    accel = CreateAccelerator("allVersatUnits",AcceleratorPurpose_MODULE);
    
    FUDeclaration* constType = GetTypeByName("Const");
    FUDeclaration* regType = GetTypeByName("Reg");

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
    type->singleInterfaces |= SingleInterfaces_SIGNAL_LOOP;

    accel = CreateAccelerator(topLevelTypeStr,AcceleratorPurpose_MODULE);
    CreateFUInstance(accel,type,"TOP");
  } else {
    // nocheckin: We might want to remove this. Check what happens if we do.
#if 0
    accel = type->fixedDelayCircuit;
#else
    accel = CreateAccelerator(topLevelTypeStr,AcceleratorPurpose_MODULE);

    FUInstance* TOP = CreateFUInstance(accel,type,"TOP");

    for(int i = 0; i < type->NumberInputs(); i++){
      String name = PushString(perm,"input%d",i);
      FUInstance* input = CreateOrGetInput(accel,name,i);
      ConnectUnits(input,0,TOP,i);
    }
    if(type->NumberOutputs() > 0){
      FUInstance* output = CreateOrGetOutput(accel);
      for(int i = 0; i < type->NumberOutputs(); i++){
        ConnectUnits(TOP,i,output,i);
      }
    }
#endif
  }


  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,true);
  FillStaticInfo(&info);
  
  VersatComputedValues val = ComputeVersatValues(accel,&info,temp);
  Array<ExternalMemoryInterface> external = val.externalMemoryInterfaces;
  
  OutputTopLevelFiles(accel,type,
                      globalOptions.hardwareOutputFilepath,
                      globalOptions.softwareOutputFilepath,
                      val);

  // NOTE: This data is printed so it can be captured by the IOB python setup.
  // TODO: Probably want a more robust way of doing this. Eventually want to printout some stats so we can
  //       actually visualize what we are producing in terms of resources/performance.
  printf("Some stats\n");
  printf("CONFIG_BITS: %d\n",val.configurationBits);
  printf("STATE_BITS: %d\n",val.stateBits);

  printf("MEM_USED: ");
  String content = ReprMemorySize(val.totalExternalMemory,temp);
  printf("%.*s",UN(content));
  printf("\n");

  printf("UNITS: %d\n",val.nUnits);
  printf("ADDR_W:%d\n",val.memoryConfigDecisionBit + 1);
  if(val.nUnitsIO){
    printf("HAS_AXI:True\n");
  }
  
  if(globalOptions.exportInternalMemories){
    int index = 0;
    for(ExternalMemoryInterface inter : external){
      switch(inter.type){
      case ExternalMemoryType::ExternalMemoryType_DP:{
        printf("DP - %d",index++);
        for(int i = 0; i < 2; i++){
          printf(",%d",inter.dp[i].bitSize);
          printf(",%d",inter.dp[i].dataSizeOut);
          printf(",%d",inter.dp[i].dataSizeIn);
        }
        printf("\n");
      }break;
      case ExternalMemoryType::ExternalMemoryType_2P:{
        printf("2P - %d",index++);
        printf(",%d",inter.tp.bitSizeOut);
        printf(",%d",inter.tp.bitSizeIn);
        printf(",%d",inter.tp.dataSizeOut);
        printf(",%d",inter.tp.dataSizeIn);
        printf("\n");
      }break;
      }
    }
  }
  
  // TODO: We do not want to output all the files. Only the ones that are needed.
  //       Potentially also control this with a CLI option just in case we want to change later.
  // NOTE: This would speed up testing because we would only test things that actually changed.
  //       Otherwise any hardware change will cause a full test reset.
  for(FUDeclaration* decl : globalDeclarations){
    BLOCK_REGION(temp);

    if(decl->type == FUDeclarationType_COMPOSITE ||
       decl->type == FUDeclarationType_ITERATIVE ||
       decl->type == FUDeclarationType_MERGED){

      String path = PushString(temp,"%.*s/%.*s.v",UN(globalOptions.hardwareOutputFilepath),UN(decl->name));
      
      FILE* sourceCode = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_CODE);
      DEFER_CLOSE_FILE(sourceCode);

      if(decl->type == FUDeclarationType_COMPOSITE || decl->type == FUDeclarationType_MERGED){
        OutputCircuitSource(decl,sourceCode);
      } else if(decl->type == FUDeclarationType_ITERATIVE){
        OutputIterativeSource(decl,sourceCode);
      }
    }
  }

  int res = CopyFileGroup(defaultVerilogFiles,globalOptions.hardwareOutputFilepath,true,FilePurpose_VERILOG_COMMON_CODE);
  if(res){
    return res;
  }
  
  // TODO: We probably need to move this to a better place
  fs::path hardwareDestinationPath(StaticFormat("%.*s",UN(globalOptions.hardwareOutputFilepath)));
  auto options = fs::copy_options::update_existing;
  for(String filepath : globalOptions.verilogFiles){
    fs::path path(StaticFormat("%.*s",UN(filepath)));
    
    fs::copy(path,hardwareDestinationPath,options);
  }
  
  // This should be the last thing that we do, no further file creation can occur after this point
  ReportFileCreation();

  return 0;
}

/*

GRAPH WEIRDNESS.

I think I found the problem that we keep getting with the graphs that make them more difficult to use than normal.
It is the same problem that we are having with the parser.

We should have a GraphBuilder Environment (or maybe using the spec environment) so that everytime we end up in an Assert situation we just Report error.

Now if I remember, the major problem that we actually have is than in situations like Merge and so on we are building a graph and also need to access it in order to compute stuff related to delays and such (which can cause more units to be added like buffers and so on).

*/

#if 0
LEFT HERE - 

Was in the process of cleaning up header stuff and the likes. Realize that this is kinda more demanding than simply going one by one removing a header, compilation seeing if we broke stuff and then putting it back if so.

I think what I really need to do is to take a look at the way things are organized and do a proper cleanup.
Why does versat.hpp exist? What is the difference between configurations.hpp, accelerator.hpp, declarations.hpp, versat.hpp? Why are they different files? 
How does stuff depend on each other? 

What I need to do is:
-- Cleanup the main trio of configurations/accelerator/declarations.hpp. If there is a proper reason to separate stuff than do it otherwise just join into two files or even one. 
-- Potentially create a defs file where a bunch of only structures and enums reside. Things like Direction, Wire and the likes are kinda "universal" and no point spending a lot of time on this. If used by everything might as well put into a everything def.
-- General cleanup, function moving, function grouping, some comments explaining stuff and so on.
-- We potentially want to reduce as much dependency on std as possible. Compile times are low so no need to go crazy but the thing that is buggying me is the fact that we have std stuff scathered around and kinda losing track on this.

-- Need to make the worflow to add a new member to instance info really simple. Members on instance info are either: given directly from data stored on FUInstance. Given directly from data that comes from Merge. Fetched from submodule. Calculated from other instance info data. We also have to handle the fact that some of the data is global while another data is local. Need to make these 4 ways of adding an InstanceInfo very simple to see so that we can make this simpler in the future.

-- Potentially remove the Pool from Accelerator. Make the accelerator a proper layer and everything is just stored on that side.

-- Potentially simplify Symbolic expresssion by allocating stuff on that side using an hashed based approach.

After this "organizational" cleanup, finish cleaning up the code, mainly the parser part. I want to remove the old parser completely. The new parser is the way to go.

This should be enough for a good workday.
#endif

/*

TODO:
NOTE:

- Need to add a proper test to API_Mem.

Parameters are still mildly broken. The AccelInfo is not properly taking parameters into account, meaning that the accelInfo of modules is misleading. The only reason that stuff works fine right is because we are instantiating parameters when computing the configs and state wires of the modules. The actual data inside the AccelInfo is still "bad" and not properly responding to the parameters that we are putting.

In fact, maybe the problem is that the FUDeclaration for modules is not using data from AccelInfo directly and instead it is doing computations over other data. This is "bad". We want AccelInfo to be the sole source of truth for all the Accelerator related data. If AccelInfo is properly calculated, then the rest of the code should work fine since all the data that is needed is already provided.

Of course some data cannot come from AccelInfo since simple modules do not contain one. 

TODO: Are we even using the IsSimple stuff anymore? Need to take another look at it.

TODO: Potentially remove the growable array and replace it with a list.
      If we end up not removing it, at least make it so that the EndFunction takes an arena and that we copied the final array into the provided arena. Otherwise the growable aspect of the array might cause some error in the future. It is the same interface as an ArenaList.

TODO: Remove the tokenizerTemplate. Replace it with a function pointer that performs the tokenization.

The main problem is the FUInstance.
A FUInstance is just a node in a graph that contains all the data that we can associate to an unit. However, after we process the graph, we should never have to access a FUInstance again. A FUInstance entire purpose is to hold all the information that we have parsed. Afterwards we use it to generate the AccelInfo, which is something that we are free to manipulate as we see fit, allowing us to change data inplace in order to implement stuff like instantiation and the likes.

So, what we need to do first is:

Second, do we also want FUDeclaration inside AccelInfo? It is probably best that we also remove any references to as well. 

Basically, this makes the AccelInfo structure completely isolated from the FUDeclaration/FUInstance stuff. It should also make it easier to change any data that we want as we please.

We also want to remove the usage of hashmaps all over the code when we could just use indexes 

Some notes:

AccelInfo contains all the information that can be extracted from an accelerator.
What does FUDeclaration need to contain that cannot be stored inside AccelInfo?
- The name of the module. Accelerators techically have names but they do not need to.
- Parameters. AccelInfo does not define parameters.
- Data that is unique for simple modules:
-- The operation that they define. For '+','-' and those type of operations.
-- Supported address gen.

All the info that is calculated from the accelerator should pass to AccelInfo.

*/

/*

Parameters handling:

- Right now the way we handle parameters is kinda adhoc. Need to figure out exactly what we need and maybe improve the approach that we are taking.

- What I absolutely need:
-- Units like VRead and VWrite might have parameterizable address gen wires. Because I want the firmware to be able to abstract the size of the address gen stuff, I need to have the value of the parameter at software compile time.
---- This means that either I find a way of having the parameter be free at the verilog level and find someway of extracting it from the verilog code into the software code during the build process.
---- OR (simpler) I force Versat to instantiate the needed value at compile time, force the user to not be able to change that parameter without passing it through Versat (no more verilog only changes, only Versat changes).

-- This also applies to stuff like memory size and the likes. We solved this problem at the pc-emul level by having a script that extracts the wire sizes from the Verilator generated code before compiling the wrapper, but I do not want to do the same for the 

-- Ultimately, If I cannot find a way of allowing the user to change parameters easily, we might as well force them to change stuff at the Versat level and recompiling stuff again. 

---- That means that I need to have Versat instantiate the proper values for things. I cannot just let the parameters flow through.

---- For the second option, I do not need to force everything to pass through Versat. For example, stuff like 

-- What parameters can we just let through?
---- ADDR_W is fine to just let through. We are probably never gonna care about ADDR_W at pc-emul levels.
---- DATA_W is probably fine? If we can have the software use the proper type stuff (iptr and the likes) then we can probably let it pass through.
---- AXI_ADDR_W is also fine to pass through. 

---- The problem is the following. What If I have a memory or an address gen that depends on that stuff? If address gen wires depend on ADDR_W then we cannot let ADDR_W be a proper parameter since then 

---- What do we lose from forcing instantiation of things? The generated hardware code cannot be parameterizable in Verilog. That's it. 

---- Of course, the best way of progressing is making the code in such a way that we do not have to chose. If we can make the code depend on SymbolicExpressions and then we implement param instantiation by symbolic instantiation and have the code generation depend on symbolic expressions, then later on we can always let the params go through by not performing the symbolic instantiation.

-- Params are obtained by calling the GetParametersOfUnit function.
--- This function either returns the default value or the value of the parameter of the unit (instance node).
--- We probably want to start putting this stuff in the acceleratorInfo struct. 

*/

/*
  Proper merge user configs:

  Right now we assume that we can just use the same name for the user configs after performing merge but things change if we ever end up merging the same module. In this case, the things become more complicated.
-- We can always force the user in this cases to define some user configs on the merge level that "resolve" the naming conflict.

*/

/* ============================================================================
// Major todo:

1. Need to change memories to keep a symbolic expression. Remove the
uglyness that is present in the memories, the memory template and the
different memory structs for int and range and whatnot. Just use
symbolic expressions. We might still want to have the expression range
in the parser, but we convert to a symbolic expression when
registering the module.

2. The merge stuff is causing troubles and we need to cleanup that part of the code. Also need to make the code robust and clean enough so that we can implement some optimizations, like not using a different multiplexer port for every merge type (reusing merge ports if possible) and maybe some other optimizations related to the insertion of buffers and the likes.

2.1 - I remember that we never found a good proper way of doing the recon stuff. Since we have more tests and a fast way of testing stuff out, we might be able to change this stuff very fast.

*/

/*

// ============================================================================
// Bugs

BUG: There was a bug in versat_ai that forced me to put a ResetAccelerator call inside the BatchNormalization layer. The problem only appear if we called Conv before Batch. Otherwise everything was fine. Did not test with the other operations. What I suppose is hapenning is that the Conv is loading some configuration that makes units produce some data that Batch should not depend on but it actually does. Maybe garbage data from merged units is influencing the accel somewhat. Regardless, I did not have the tools needed to debug such a problem at that point and opted to use the quick and dirty way of fixing stuff.

BUG: Need more tests for modules that contain units but are not connected to anything. Also test merge for these cases as well.

BUG: Debug folder gets created inside the iob-soc-versat repo when running ./test.py tests. Something about the python3 script -> makefile is causing the path to not be the same as if just calling makefile directly.

BUG: localOrder appears to be broken. 

BUG: Since the name of the units are copied directly to the header file, it is possible to have conflict with C reserved keywords, like const, static, and stuff like that. 

In fact, need to start checking every keyword used by C,C++ and Verilog and make sure that the names that are passed in the versat spec file never conflict with any special keyword.

BUG: Partial share is missing in the spec parser for multiple units with different names. Only working for arrays.

There is probably a lot of cleanup left (overall and inside Merge).

Need to take a look at State and Mem struct interfaces. State is not taking into account merge types and neither is Mem. Also need to see if we can generate the Mem as an actual structure where the programmer must use pointer arithmetic directly.

BUG:

module Test(){
Mem m0;
Mem m1;
#
m0 -> m1;
m1 -> m0;
}

produces a design that cannot work directly. Both units are given a delay of 0 but each unit takes 3 cycles to produce data which means that the design is fundamentally broken.

Fixing this bug would also give me an opportunity to booster up the delay and graph side of the code. It is probably more complex than if I would write it today.

BUG:

An empty AddressGen is not working correctly. The address gen parser is programmed to return an error if the address gen expression does not contain anything, but it is technically good syntax. Either we allow the program to have empty address gens or we put an error that detects such case and reports this info to the user.

*/

/*

Code generation for customizable features:
- There exists a bunch of code that gets generated even though it is not needed. Stuff like profile functions are declared and even defined in wrapper and firmware. While this is not a problem, since the stuff that is extra does not affect the correctness or causes any error, one thing that is does is slow down the testing, since now every change affects all the tests even though it should only change the tests that actually use the features.

-- Basically, it is not enough to control generation of functions, we also should control stuff like function declarations and the likes in order to make it so that tests that do not use the features are not forced to be recompiled and retested for a change that does not affect them.

Code generation lint friendly:

- If a module is composed of units that do not contain certain signals, like clk, rst, run and the likes, then the module should not have those signals as well. The only thing missing to implement this is changing the wrapper to support the verilated unit not containing these signals.

More user error checking:

- Verilog parsed content does not check direction of ports. We should encode all the interfaces that we expect as data and perform the checks and report errors.

Error handling:

- Versat is currently incapable of handling even the slighest deviation from good data. Very poor handling of error conditions.
- Some stuff that we want:
-- Parser errors and related stuff. Any problem with the input data that is related to the spec file needs to report the error with a message that identifies in the spec file the source of the problem.
-- Problems with the graph most be reported with a good message. Missing connections might be warnings, extra connections need a location error on the spec file. Problems with delay calculations need an identification of the path that is causing the problem and stuff like that.

Usability:

"Bugs" that arise from forgetting to activate the merged accelerator are starting to become problematic.
Need to find a way, at least in PC-Emul, of detecting that the proper merged accelerator was not activated.
There are two things that we can do.
First, start the accelerator in an empty state and give an error if no merged was set when doing a run. Very likely an error. The user cannot rely on the fact that the accelerator starts on merged 0.
Second, we can make it so that changing an accelerator merge state clears the config structure and we can figure out which config values are associated to each merged state.
What this means is that we can potently figure out which merged state the user is likely configuring the accelerator regardless of the actual state the accelerator is set to and we can warn the user if
it is very likely that the user is missing some configuration or it appears to miss configure the current merged accelerator.

Hardware

- Memories are not being properly registered. Also tired of keeping the iob_ stuff around. We probably want to spend one day just cleaning up all this stuff and maybe find a way of checking for places where registering could improve performance. Maybe yosis could be used for this, need to check.

Address Gen:

Address gen logic is very strict. Any deviation leads to program error and stacktraces start popping up.
- Empty address gens fail and give errors.
- Address gen with only address fail and give errors.

We spend a good deal of time writing address gens that access matrix like structures.
- Maybe it would be a good idea to push some of this logic to the address gens.
-- Imagine being able to declare that the data follows a certain matrix like structure and then being able to provide addresses by accessing it.

Something like:

addressGen test(width,height){
  format[height][width];
  for y 0..height
  for x 0..width
  addr = format[y][x];
}

- Currently stride is kinda "faked". We basically just look at the expression and if it is a division, we pick the divisor and make the duty logic from there.
-- Before we could think about the addresses generated and figure out how the unit would work based on the address used, but with this approach to stride it is no different to having a separate input to the address gen construct where we could put the duty stuff.

-- In fact, if we continue down this road, we might as well make it something like:

address Test(a,b){
  for x 0..a
  addr = a;
  stride = stride;
  }

Usability:

- Need to check parameters and sizes and report stuff at Versat compile time.
-- An example is the xunitF which requires input to be 32 bits, meaning that we cannot just change DATA_W and expect this to work.

- Is there a reason to have versat_emul.c ?
-- Couldn't we just put everything into versat_wrapper and be done with it? That way, the user would only need to add the lib to the compilation process and everything should work fine.

Code Generation:

- Header file generating code that should depend on Metadata.

Optimizations:

-- We could collapse certain loops, assuming that we have the info needed (or we can push it into runtime).
-- Leftover loops can be used to reduce the preassure on the lower loops (less bits needed and stuff like that, we currently pay for the upper loops bits wether we use them or not)

- If we eventually have to handle non zero start loops, need to figure out how we do it. We can shift loops around so they start at zero, but do not know if we should do this when converting an addressGenDef into an AddressAccess, or if we convert first and then have a function that does this shift, etc... Mostly depends on how easy this transformation is when using SymbolicExpressions vs using LinearLoopSum. 

- There is an optimization that we can make in order to reduce memory usage. For loops where the innermost term is not a '1' we are fetching more memory than we care about and that is fine. The problem is that we are storing those unused values inside the internal memory when we do not have to. If the innermost term was a 4, we could make it so that the internal memory only stores every 4 values read. For double loop address gens, we make sure that in hardware we reset every time we have a loop (so that the address gen lines up correctly). That way, for a loop that contains a constant term N, we reduce the amount of internal memory used by a factor of N.

-- The same concept applies to writes, but instead of storing memory, we must control the transfer process to not write over memory by completely disabling the strobe for the values that we do not want to write over.

--- How would we handle bigger transfer sizes, like AXI_DATA_W = 256? For both reads and writes it become more difficult to save memory. 


Testability:

- While we test if versat runs and produces valid code, we do not test how does versat handle errors.

-- How does Versat handle not being able to parse a Module Verilog?
-- How does Versat handle not being able to parse the specs file?
-- How does it handle graphs that contain loops, misuse in constructs like share, repeated variable names and the likes?
-- Etc. Need to develop tests that only need to exercise this conditions and need to make sure that at least Versat produces some valid output and does not simply crash without giving any useful information.

Misc:

Hierarchical Merge:

- Hierarchical merge produces a lot of structures in the header files which are not actually needed.
-- Need to collapse same structure format into a single format.
-- Same thing for merge types, merge names and stuff like that. We want to collapse everything that is equal into a single form, reduce complexity and 
--- IMPORTANT: The delays also need to be collapsed. There is no point in storing 

Generated code does not take into account parameters when it should.
- Some wires are given fixed sizes when they depend on verilog parameters. 

Debugability:

- We kinda lost the .dot files generated in order to help debug graph algorithms. Its not a big deal now but eventually would like to have them back, as the .dot files are the only proper way we currently have of debugging graph operations (which I do not think we will be doing much soon, but eventually need to address this).

Blindness:

- We currently do not keep track of memory usage. Which functions use the most memory, which file uses the most memory, etc. This can easily be accomplished by adding code to the ARENA macros used.

Features:

- We could implement a shared delay as well as config. To make it even more powerful, we could make it so we could share the delay of some units and the config of others and have it overlap somewhat. How I would represent that in syntax I do not know.

- When using address gen and when having to deal with sharing, what we actually want is to share the address gen variables.
  - If I have an address gen of X(a,b) what I actually want is to tell the tool to share a or share b or both or none.
    - Otherwise we are forcing the user to always keep in mind how the actual config wires our how the address gen code maps to the unit itself.

Wrapper:

- The databus does not take into consideration the strobe of the databus.

- Remove the while from the simulate function.
  -  Also put in a comment. We cannot have any "while" inside the wrapper. Every function must be guarantee to end, even if it just a for loop of 1 million iterations that then ends by reporting an error to the user.

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
