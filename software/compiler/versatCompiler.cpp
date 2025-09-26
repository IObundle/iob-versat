#include <argp.h>
#include <sys/wait.h>
#include <ftw.h>

// TODO: Eventually need to find a way of detecting superfluous includes or something to the same effect. Maybe possible change to a unity build although the main problem to solve is organization.
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
    case 's': opts->options->addInputAndOutputsToTop = true; break;

    case 'L': opts->options->generetaSourceListName = arg; break;

    case 'p': opts->options->prefixIObPort = arg; break;

    case 'T': opts->options->opMode = VersatOperationMode_GENERATE_TESTBENCH; break;

    case 'E': {
      opts->options->extraIOb = true;
      opts->options->useSymbolAddress = true;
    } break;

    case 128: {
      opts->options->insertAdditionalDebugRegisters = true;
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
    { "debug", 128 ,0, 0, "Insert debug utilities on the generated accelerator"},
    { 0, 'b',"Size",   0, "Databus size connected to external RAM (8,16,default:32,64,128,256)"},
    { 0, 'd', 0,       0, "Use DMA"},
    { 0, 'D', 0,       0, "Architecture has databus"},
    { 0, 'E', 0,       0, "Enables a set of hardcoded options that is mostly some hacks for other projects (which in the future I hope to properly implement as proper configurable parameters)"},
    { 0, 'g',"Path",   OPTION_HIDDEN, "Debug path"},
    { 0, 's', 0,       0, "Insert consts and regs if Top unit contains inputs and outputs"},
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

int main(int argc,char* argv[]){
#ifdef VERSAT_DEBUG
  printf("Running in debug mode\n");
#endif
  
  InitDebug();
  
  Arena globalPermanentInst = InitArena(Megabyte(128));
  globalPermanent = &globalPermanentInst;
  Arena tempInst = InitArena(Megabyte(128));
  Arena temp2Inst = InitArena(Megabyte(128));

  contextArenas[0] = &tempInst;
  contextArenas[1] = &temp2Inst;
  
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  
  Arena* perm = globalPermanent;
  
  InitializeDefaultData(perm);
  InitializeTemplateEngine(perm);
  InitializeSimpleDeclarations();

#if 0
  TestSymbolic();
  return 0;
#endif
  
  argp argp = { options, parse_opt, "SpecFile\n-T UnitName", "Dataflow to accelerator compiler. Check tutorial in https://github.com/IObundle/iob-versat to learn how to write a specification file"};

  OptionsGather gather = {};
  gather.verilogFiles = PushArenaList<String>(temp);
  gather.extraSources = PushArenaList<String>(temp);
  gather.includePaths = PushArenaList<String>(temp);
  gather.unitFolderPaths = PushArenaList<String>(temp);

  globalOptions = DefaultOptions(perm);
  gather.options = &globalOptions;

  if(argp_parse(&argp, argc, argv, 0, 0, &gather) != 0){
    printf("Error parsing arguments. Call -h help to print usage and argument help\n");
    return -1;
  }
  
  globalOptions.verilogFiles = PushArrayFromList(perm,gather.verilogFiles);
  globalOptions.extraSources = PushArrayFromList(perm,gather.extraSources);
  globalOptions.includePaths = PushArrayFromList(perm,gather.includePaths);
  globalOptions.unitFolderPaths = PushArrayFromList(perm,gather.unitFolderPaths);
  
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
  
  // Register Versat common files. 
  bool anyError = false;
  for(FileContent file : defaultVerilogUnits){
    String content = file.content;
    
    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm);
      Opt<FUDeclaration*> inst = RegisterModuleInfo(&info,perm);
      anyError |= !inst.has_value();
    }
  }

  if(anyError){
    return -1;
  }
  
  // We need to do this after parsing the modules because the majority of these special types come from verilog files
  // NOTE: This should never fail since the verilog files are embedded into the exe. A fail in here means that we failed to embed the necessary files at build time
  BasicDeclaration::buffer = GetTypeByNameOrFail("Buffer");
  BasicDeclaration::fixedBuffer = GetTypeByNameOrFail("FixedBuffer");
  BasicDeclaration::pipelineRegister = GetTypeByNameOrFail("PipelineRegister");
  BasicDeclaration::multiplexer = GetTypeByNameOrFail("Mux2");
  BasicDeclaration::combMultiplexer = GetTypeByNameOrFail("CombMux2");
  BasicDeclaration::stridedMerge = GetTypeByNameOrFail("StridedMerge");
  BasicDeclaration::timedMultiplexer = GetTypeByNameOrFail("TimedMux");
  BasicDeclaration::input = GetTypeByNameOrFail("CircuitInput");
  BasicDeclaration::output = GetTypeByNameOrFail("CircuitOutput");

  // Collect all user verilog source files.
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

    allVerilogFiles = PushArrayFromSet(perm,allVerilogFilesSet);
  }

  if(error){
    return -1;
  }
  
  // NOTE: We process all the folders and just replace verilogFiles with all the filepaths in here.
  globalOptions.verilogFiles = allVerilogFiles;
  
  // Parse verilog files and register all the user supplied units.
  error = false;
  for(String filepath : globalOptions.verilogFiles){
    String content = PushFile(temp,filepath);
    
    if(Empty(content)){
      printf("Failed to open file %.*s\n. Exiting\n",UN(filepath));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm);

      FUDeclaration* decl = GetTypeByName(info.name);
      if(decl){
        once(){
          fprintf(stderr,"Error, naming conflict that must be resolved by user\n");
        };
        fprintf(stderr,"Module '%.*s' already exists\n",UN(info.name));

        error = true;
        continue;
      }
      
      RegisterModuleInfo(&info,perm);
    }
  }
  if(error){
    return -1;
  }

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

    auto GetConstructOrFail = [&types](String name) -> ConstructDef{
      for(ConstructDef def : types){
        if(CompareString(def.base.name,name)){
          return def;
        }
      }

      Assert(false);
      return {};
    };
    
    auto moduleLike = PushArenaList<ConstructDef>(temp);
    for(ConstructDef def : types){
      if(IsModuleLike(def)){
        *moduleLike->PushElem() = def;
      }
    }
    auto modules = PushArrayFromList(temp,moduleLike);

    auto addressGenList = PushArenaList<ConstructDef>(temp);
    for(ConstructDef def : types){
      if(def.type == ConstructType_ADDRESSGEN){
        *addressGenList->PushElem() = def;
      }
    }
    auto addressGen = PushArrayFromList(temp,addressGenList);
    
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

    // After getting all the types that we need to process, we first collect and compile all the address gens that are gonna be needed.
    auto addressGenTokensUsed = PushArenaList<Token>(temp);
    
    for(auto p : typeToWork){
      Work work = *p.second;

      if(IsModuleLike(work.definition)){
        Array<Token> tokens = AddressGenUsed(work.definition,types,temp);

        for(Token t : tokens){
          *addressGenTokensUsed->PushElem() = t;
        }
      }
    }

    Array<Token> allAddressGenUsed = PushArrayFromList(temp,addressGenTokensUsed);

    for(Token t : allAddressGenUsed){
      bool found = false;
      for(ConstructDef def : addressGen){
        if(CompareString(def.base.name,t)){
          found = true;
          break;
        }
      }

      if(!found){
        ReportError(content,t,"Did not find address gen definition");
        anyError = true;
      }
    }
    
    if(anyError){
      return -1;
    }

    // NOTE: From this point on, no missing address gen or missing module problem should exist. Everything has been validity in regards to missing definitions and everything passed. (There can still exist problems inherit to the construct itself).
    
    TrieSet<String>* allAddressGens = PushTrieSet<String>(temp);
    for(Token t : allAddressGenUsed){
      allAddressGens->Insert(t);
    }

    // For now we compile every single address gen that is used but eventually we want to only compile what we need, otherwise the address gen tests will always fail.
    for(String name : allAddressGens){
      ConstructDef def = GetConstructOrFail(name);

      AddressAccess* access = CompileAddressGen(&def.addressGen,content);
      
      if(!access){
        anyError = true;
      }
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

      FUDeclaration* decl = nullptr;
      
      if(work.definition.type == ConstructType_MODULE){
        decl = InstantiateBarebonesSpecifications(content,p.second->definition);
      } else if(work.definition.type == ConstructType_MERGE){
        decl = InstantiateSpecifications(content,p.second->definition);
      }
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
  FUInstance* TOP = nullptr;

  bool isSimple = false;
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
    TOP = CreateFUInstance(accel,type,"TOP");
  } else if(globalOptions.addInputAndOutputsToTop && !(type->NumberInputs() == 0 && type->NumberOutputs() == 0)){
    // TODO: This process needs to be simplified and more integrated with the approach used by versat spec.
    //       Instead of doing everything manually.
    isSimple = true;

    const char* name = StaticFormat("%.*s_Simple",UN(topLevelTypeStr));
    accel = CreateAccelerator(name,AcceleratorPurpose_MODULE);
    
    int input = type->NumberInputs();
    int output = type->NumberOutputs();

    Array<FUInstance*> inputs = PushArray<FUInstance*>(perm,input);
    Array<FUInstance*> outputs = PushArray<FUInstance*>(perm,output);

    FUDeclaration* constType = GetTypeByName("TestConst");
    FUDeclaration* regType = GetTypeByName("Reg");
    
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

    TOP = CreateFUInstance(accel,type,"simple");

    for(int i = 0; i < input; i++){
      ConnectUnits(inputs[i],0,TOP,i);
    }
    for(int i = 0; i < output; i++){
      ConnectUnits(TOP,i,outputs[i],0);
    }
    
    type = RegisterSubUnit(accel);
    type->definitionArrays = PushArray<Pair<String,int>>(perm,2);
    type->definitionArrays[0] = (Pair<String,int>){"input",input};
    type->definitionArrays[1] = (Pair<String,int>){"output",output};

    type->singleInterfaces |= SingleInterfaces_SIGNAL_LOOP;
    
    name = StaticFormat("%.*s_Simple",UN(topLevelTypeStr));
    accel = CreateAccelerator(name,AcceleratorPurpose_MODULE);
    TOP = CreateFUInstance(accel,type,"TOP");
  } else {
    // TODO: I think that the rest of the code is good enough that we do not need to do this extra step.
    //       
    accel = CreateAccelerator(topLevelTypeStr,AcceleratorPurpose_MODULE);

    TOP = CreateFUInstance(accel,type,"TOP");

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
  }

  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,true);
  FillStaticInfo(&info);
  
  VersatComputedValues val = ComputeVersatValues(&info,temp);
  
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(temp,val.externalMemoryInterfaces);
  int externalIndex = 0;
  for(InstanceInfo& in : info.infos[0].info){
    if(!in.isComposite){
      for(ExternalMemoryInterface& inter : in.decl->externalMemory){
        external[externalIndex++] = inter;
      }
    }
  }
  external.size = externalIndex;
  
  OutputTopLevelFiles(accel,type,
                     globalOptions.hardwareOutputFilepath,
                     globalOptions.softwareOutputFilepath,
                      isSimple,info,val,external);

  // NOTE: This data is printed so it can be captured by the IOB python setup.
  // TODO: Probably want a more robust way of doing this. Eventually want to printout some stats so we can
  //       actually visualize what we are producing.
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
  
  for(FUDeclaration* decl : globalDeclarations){
    BLOCK_REGION(temp);

    if(decl->type == FUDeclarationType_COMPOSITE ||
       decl->type == FUDeclarationType_ITERATIVE ||
       decl->type == FUDeclarationType_MERGED){

      // TODO: Disabled since it was putting content on the wrong folder
#if 0
      if(globalOptions.debug && decl->fixedDelayCircuit){
        GraphPrintingContent content = GenerateDefaultPrintingContent(decl->fixedDelayCircuit,temp);
        String repr = GenerateDotGraph(content,temp);
        String debugPath = PushDebugPath(temp,decl->name,"NormalGraph.dot");

        FILE* file = OpenFileAndCreateDirectories(debugPath,"w",FilePurpose_DEBUG_INFO);
        DEFER_CLOSE_FILE(file);
        if(!file){
          printf("Error opening file for debug outputting: %.*s\n",UN(debugPath));
        } else {
          fwrite(repr.data,sizeof(char),repr.size,file);
        }
      }
#endif

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
  res = CopyFileGroup(defaultSoftwareFiles,globalOptions.softwareOutputFilepath,false,FilePurpose_SOFTWARE);
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

/* ============================================================================
// Major changes

Overall of versat spec usability and accelerator programming.

- The idea of separating address gen from the modules that use them seems that it is causing more trouble than it is worth, kinda.

- If we made it so that modules could define a set of variables as inputs and we could set config values inside VersatSpec files, including defining address gens, then we could generate functions that could program the entire module in a single step.

-- The embedded logic would just be Call a function (that would program the entire accelerator) passing the parameters that we define and VersatSpec would define everything related to the configuration.

-- What would the syntax look like?

Maybe we could do something like this:

Module Test(){
  VRead read;
  VWrite write;
  Const const;
#

  a = read + const;
  a -> write;

  config SimpleTransfer(size,constant){
    for x 0..size {
      read = x;
      write = x;
    }
    const.constant = constant;
  }

  // A way for the user to make a simpler function instead of reusing the more complex SimpleTransfer function above.
  config JustChangeConstant(newConstant){
    const.constant = constant;
  }

  // We also need a way of loading memories if we follow this route.
  mem LoadMem(address,size){
    for x 0..size {
      memUnit = address[x];
    }
  }
}

- This also solves one other problem. We can use special modifiers when declaring the variables to indicate
- wether the variables are gonna change frequently, often or if they are mostly constant. Afterwards, we can generate
- code that only changes some variables and only contains logic for those variable changes. The rest just
- stays the same value. This allow us to only generate the functions that we actually need and to generate
- the most efficient functions possible.

-- What this basically amounts to is that firmware literally just becomes a vehicle to call a single function, generate by Versat, that performs the entire configuration of a given accelerator run. Different configurations are given by different functions and if we ask for user for more information in regards to the way the accelerator is gonna be used, we can implement these functions very efficiently.

- Also important, if we follow this route, we need to 

// ============================================================================
// Bugs

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

Code generation lint friendly:

- If a module is composed of units that do not contain certain signals, like clk, rst, run and the likes, then the module should not have those signals as well. The only thing missing to implement this is changing the wrapper to support the verilated unit not containing these signals.

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

Versat Spec:

- The using stuff is kinda weird syntax. What we probably want is to think about types as parameterizable.
-- Insteand of "using(Addr) VRead ...", we probably want "VRead(Addr) ...", where Addr is a parameter for the VRead type.
-- We then end up with two types of parameters, Versat parameters and Verilog parameters, which we probably want to collapse into a single type from the perspective of the user and let Versat handle how to route the parameter itself.

Usability:
x
- Need to check parameters and sizes and report stuff at Versat compile time.
-- An example is the xunitF which requires input to be 32 bits, meaning that we cannot just change DATA_W and expect this to work.

- Is there a reason to have versat_emul.c ?
-- Couldn't we just put everything into versat_wrapper and be done with it? That way, the user would only need to add the lib to the compilation process and everything should work fine.

Code Generation:

- Header file generating code that should depend on Metadata.

- Usability.

- Optimizations:

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

- I'm considering removing the usage of the template engine for the generation of code and instead replace it with an "emitter" based approach. Templates work for a good amount of static and low amount of dynamic data but for a lot of dynamic data they kinda fall apart. Furthermore, the amount of complexity required to implement them is kinda not worth it.

-- I'm considering writting an emitter for Verilog that should be useful for a lot of situations.

- The template engine could be updated to compile at build time, offer compile time checks instead of runtime and simplify data management since we could pass and use C code to access and iterate the data. This means that passing large structures is preferebly instead of doing what we do now where we want to minimize data passing this way. Read the NOTEs that are contained inside the OutputTopLevelFiles.

- The Address gen are kinda adhoc in the generated code. Need to find a way of binding them to the accelerator that uses them (some declaration inside the module/merge that indicates to Versat that the address gen is intended to be used for that unit.). As a consequence of this, we might need to generate different address gens for different structs because of merge. I think I have a good enough merge backbone to implement this in a couple of days, but I also would probably like to finalize the static portion before progressing to tackle this problem.

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
