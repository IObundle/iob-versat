#include "configurations.hpp"
#include "versat.hpp"
#include "utils.hpp"
#include "verilogParsing.hpp"
#include "type.hpp"
#include "templateEngine.hpp"
#include "unitVerilation.hpp"
#include "debugGUI.hpp"
#include "acceleratorStats.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "templateData.hpp"

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

struct ArgumentOptions{
  std::vector<String> verilogFiles;
  std::vector<String> extraSources;
  std::vector<String> includePaths;
  const char* specificationFilepath;
  const char* topName;
  std::vector<String> unitPaths;
  String hardwareOutputFilepath;
  String softwareOutputFilepath;
  String verilatorRoot;
  int bitSize; // AXI_ADDR_W
  int dataSize; // AXI_DATA_W
  bool addInputAndOutputsToTop;
  bool useDMA;
  bool archHasDatabus;
  bool debug;
};

String GetAbsolutePath(const char* path,Arena* arena){
  fs::path canonical = fs::weakly_canonical(path);
  String res = PushString(arena,"%s",canonical.c_str());
  return res;
}

Optional<String> GetFormat(String filename){
  int size = filename.size;
  for(int i = 0; i < filename.size; i++){
    char ch = filename[size - i - 1];
    if(ch == '.'){
      String res = (String){&filename[size - i],i};
      return res;
    }
    if(ch == '/'){
      break;
    }
  }

  return Optional<String>();
}

// Before continuing, find a more generic approach, check getopt, argparse or any other related functions that you can find
// Try to follow the gcc conventions.
// What I want: Default values, must have values, generate help automatically, stuff like that.
//
// TODO: Change things so that we use a ArenaList to store everything while processing and then convert to Array when storing permanently in the permanent Arena
ArgumentOptions* ParseCommandLineOptions(int argc,const char* argv[],Arena* perm,Arena* temp){
  ArgumentOptions* opts = PushStruct<ArgumentOptions>(perm);
  opts->dataSize = 32; // By default.
  opts->bitSize = 32;

  for(int i = 1; i < argc; i++){
    String str = STRING(argv[i]);

    Optional<String> formatOpt = GetFormat(str);

    // TODO: Verilator does not actually need the source files, it only needs a include path to the folder that contains the sources. This could be removed.
    if(str.size >= 2 && str[0] == '-' && str[1] == 'S'){
      if(str.size == 2){
        if(i + 1 >= argc){
          printf("Missing argument\n");
          exit(-1);
        }
        opts->extraSources.push_back(GetAbsolutePath(argv[i + 1],perm));
        i += 1;
      } else {
        opts->extraSources.push_back(GetAbsolutePath(&str.data[2],perm));
      }

      continue;
    }

	if(str.size > 3 && str[0] == '-' && str[1] == 'b' && str[2] == '='){
	  Tokenizer tok(str,"=",{});

	  tok.AssertNextToken("-b");
	  tok.AssertNextToken("=");
	  Token dataSize = tok.NextToken();
	  Assert(tok.Done());

	  int size = ParseInt(dataSize); // TODO: Should check for errors, probably have ParseInt return an optional
	  opts->dataSize = size;
	}

    // TODO: This code indicates that we need an "arquitecture" generic portion of code
    //       Things should be parameterizable
    if(str.size >= 4 && str[0] == '-' && str[1] == 'x' && str[2] == '3' && str[3] == '2'){
      Assert(str.size == 4);

      opts->bitSize = 32;
      continue;
    }

    if(str.size >= 4 && str[0] == '-' && str[1] == 'x' && str[2] == '6' && str[3] == '4'){
      Assert(str.size == 4);

      opts->bitSize = 64;
      continue;
    }

    if(str.size >= 4 && str[0] == '-' && str[1] == '-' && str[2] == 'V' && str[3] == 'R'){
      Assert(str.size == 4);

      opts->verilatorRoot = STRING(argv[i+1]);
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'd'){
      Assert(str.size == 2);
      opts->useDMA = true;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'D'){
      Assert(str.size == 2);
      opts->archHasDatabus = true;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'I'){
      if(str.size == 2){
        if(i + 1 >= argc){
          printf("Missing argument\n");
          exit(-1);
        }
        opts->includePaths.push_back(GetAbsolutePath(argv[i + 1],perm));
        i += 1;
      } else {
        opts->includePaths.push_back(GetAbsolutePath(&str.data[2],perm));
      }

      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'T'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }
      opts->topName = argv[i + 1];
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 's'){
      opts->addInputAndOutputsToTop = true;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'g'){
      opts->debug = true;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'O'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }

      String unitPath = GetAbsolutePath(argv[i+1],perm);
      PushNullByte(perm);

      opts->unitPaths.push_back(unitPath);
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'o'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }
      opts->hardwareOutputFilepath = GetAbsolutePath(argv[i+1],perm);
      PushNullByte(perm);
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'H'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }
      opts->softwareOutputFilepath = GetAbsolutePath(argv[i+1],perm);
      PushNullByte(perm);
      i += 1;
      continue;
    }
    
    if(formatOpt){
      String format = formatOpt.value();
      if(CompareString(format,"v")){
        opts->verilogFiles.push_back(GetAbsolutePath(argv[i],perm));
        continue;
      } else { // Assume to be the specification file, for now
            if(opts->specificationFilepath){
              printf("Error, multiple specification files specified, not supported for now\n");
              exit(-1);
            }

            opts->specificationFilepath = argv[i];
      }
      continue;
    }
  }

  return opts;
}

//General TODO: There is a lot of code that makes sense for the old one phase versat, but that is just overhead for the two phase versat. We do not have to guarantee that the Accelerator is "valid" at all times, like the previous version did. We can use more immediate mode APIs and rely on correct function calling instead of pre doing work.

#include <unordered_map>

#include <execinfo.h>

void print_trace(){
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    printf ("Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);
  }

  free (strings);
}

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <ftw.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int RemoveDirectory(const char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void CallVerilator(const char* unitPath,const char* outputPath){
   pid_t pid = fork();
  
   char* const verilatorArgs[] = {
      (char*) "verilator",
      (char*) "--cc",
      (char*) "-Mdir",
      (char*) outputPath,
      (char*) unitPath,
      nullptr
   };

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      execvp("verilator",verilatorArgs);
      printf("Error calling execvp for verilator: %s\n",strerror(errno));
   } else {
      int status;
      wait(&status);
   }  
}

String GetVerilatorRoot(Arena* arena){
  // We currently get verilator root by running verilator over a simple example and extracting the VERILATOR_ROOT value from the makefile generated by it. It's the way I find to be the more consistent one, since the location of the verilator executable might not be inside the bin folder of VERILATOR_ROOT (happened once with an installation from apt-get for version 4.X).

  Byte* mark = MarkArena(arena);
  PushBytes(arena,1024); // Reserve some space to return the string
  
  BLOCK_REGION(arena);

  int res = 0;
  
  res = mkdir("/tmp/versatTmp",0770);
  if(res == -1 && errno != EEXIST){
    printf("Problem making tmp directory errno: %d\n",errno);
    exit(-1);
  }

  String testFile = PushString(arena,"`timescale 1ns / 1ps\nmodule Test(); endmodule");
  PushNullByte(arena);
  
  FILE* f = fopen("/tmp/versatTmp/Test.v","w");
  if(f == nullptr){
    printf("Error creating file\n");
    exit(-1);
  }
  
  size_t sizeWritten = fwrite(testFile.data,sizeof(char),testFile.size,f);

  if(sizeWritten != testFile.size){
    printf("Did not write as many bytes as expected\n");
    exit(-1);
  }

  fflush(f);
  fclose(f);

  CallVerilator("/tmp/versatTmp/Test.v","/tmp/versatTmp");

  String fileContent = PushFile(arena,"/tmp/versatTmp/VTest.mk");

  if(fileContent.size == 0){
    printf("Problem opening verilator generated file\n");
    exit(-1);
  }

  Tokenizer tok(fileContent,"=",{":="});

  String verilatorRoot = {};
  while(!tok.Done()){
    Token possible = tok.PeekFindIncluding("VERILATOR_ROOT");

    if(possible.size == 0){
      printf("Couldn't find the location of verilator root\n");
      exit(-1);
    }

    tok.AdvancePeek(possible);

    Token peek = tok.PeekToken();
    if(CompareString(peek,"=") || CompareString(peek,":=")){
      tok.AdvancePeek(peek);
      String root = tok.NextToken();

      PopMark(arena,mark);
      verilatorRoot = PushString(arena,"%.*s",UNPACK_SS(root));
      break;
    }
  }

  RemoveDirectory("/tmp/versatTmp");
  
  return verilatorRoot;
}

int main(int argc,const char* argv[]){
#if 1
  if(argc < 3){
    printf("Need specifications and a top level type\n");
    return -1;
  }
#endif
  
  Versat* versat = InitVersat(0,1);
  
  // TODO: This is not a good idea, after changing versat to 2 phases
  SetDebug(versat,VersatDebugFlags::OUTPUT_ACCELERATORS_CODE,1);
  SetDebug(versat,VersatDebugFlags::OUTPUT_VERSAT_CODE,1);
  SetDebug(versat,VersatDebugFlags::USE_FIXED_BUFFERS,0);
  SetDebug(versat,VersatDebugFlags::OUTPUT_GRAPH_DOT,1);
  //SetDebug(versat,VersatDebugFlags::OUTPUT_VCD,0);

  Arena permInst = InitArena(Megabyte(256));
  Arena* perm = &permInst;
  Arena preprocessInst = InitArena(Megabyte(256));
  Arena* preprocess = &preprocessInst;
  Arena tempInst = InitArena(Megabyte(256));
  Arena* temp = &tempInst;

  // TODO: Add options directly to versat instead of having two different structures
  ArgumentOptions* opts = ParseCommandLineOptions(argc,argv,perm,temp);
  versat->outputLocation = opts->hardwareOutputFilepath;
  versat->opts.addrSize = opts->bitSize;
  versat->opts.architectureHasDatabus = opts->archHasDatabus;
  versat->opts.dataSize = opts->dataSize;

#ifdef USE_FST_FORMAT
  versat->opts.generateFSTFormat = 1;
#endif
  
  //printf("%s\n",STRINGIFY(DEFAULT_UNIT_PATHS));

  for(String& path : opts->unitPaths){
    String dirPaths = path;
    Tokenizer pathSplitter(dirPaths,"",{});

    while(!pathSplitter.Done()){
      Token path = pathSplitter.NextToken();

      Optional<Array<String>> res = GetAllFilesInsideDirectory(path,temp);
      if(!res){
        printf("\n\nCannot open dir: %.*s\n\n",UNPACK_SS(path));
      } else {
        for(String& str : res.value()){
          String fullPath = PushString(perm,"%.*s/%.*s",UNPACK_SS(path),UNPACK_SS(str));

          // Crude avoid duplicated files
          bool add = true;
          for(String& insideVec : opts->verilogFiles){
            if(CompareString(insideVec,fullPath)){
              add = false;
              break;
            }
          }
          if(add){
            opts->verilogFiles.push_back(fullPath);
          }
        }
      }
    }
  }
    
  // Check existance of Verilator. We cannot proceed without Verilator
  if(opts->verilatorRoot.size == 0){
    opts->verilatorRoot = GetVerilatorRoot(&versat->permanent);
  }

  if(!opts->topName){
    printf("Specify accelerator type using -T <type>\n");
    exit(-1);
  }

  PushPtr<ModuleInfo> allModulesPush(perm,999); // For debugging purposes
  for(String file : opts->verilogFiles){
    String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(file)));

    if(content.size == 0){
      printf("Failed to open file %.*s\n. Exiting\n",UNPACK_SS(file));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(preprocess,content,&opts->includePaths,temp);
    std::vector<Module> modules = ParseVerilogFile(processed,&opts->includePaths,temp);

    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm,temp);
      allModulesPush.PushValue(info);

      RegisterModuleInfo(versat,&info);
    }
  }
  Array<ModuleInfo> allModules = allModulesPush.AsArray();

  FUDeclaration* decl = nullptr;
  
  BasicDeclaration::buffer = GetTypeByName(versat,STRING("Buffer"));
  BasicDeclaration::fixedBuffer = GetTypeByName(versat,STRING("FixedBuffer"));
  BasicDeclaration::pipelineRegister = GetTypeByName(versat,STRING("PipelineRegister"));
  BasicDeclaration::multiplexer = GetTypeByName(versat,STRING("Mux2"));
  BasicDeclaration::combMultiplexer = GetTypeByName(versat,STRING("CombMux2"));
  BasicDeclaration::stridedMerge = GetTypeByName(versat,STRING("StridedMerge"));
  BasicDeclaration::timedMultiplexer = GetTypeByName(versat,STRING("TimedMux"));
  BasicDeclaration::input = GetTypeByName(versat,STRING("CircuitInput"));
  BasicDeclaration::output = GetTypeByName(versat,STRING("CircuitOutput"));
  BasicDeclaration::data = GetTypeByName(versat,STRING("Data"));

  const char* specFilepath = opts->specificationFilepath;
  String topLevelTypeStr = STRING(opts->topName);

  if(specFilepath){
    ParseVersatSpecification(versat,specFilepath);
  }

  // MARK
  //DebugVersat(versat);
  
  FUDeclaration* type = GetTypeByName(versat,topLevelTypeStr);
  Accelerator* accel = CreateAccelerator(versat);
  FUInstance* TOP = nullptr;

  accel->useDMA = opts->useDMA; // TODO: Maybe should be a versat configuration, instead of accelerator

  bool isSimple = false;
  if(opts->addInputAndOutputsToTop && !(type->inputDelays.size == 0 && type->outputLatencies.size == 0)){
    int input = type->inputDelays.size;
    int output = type->outputLatencies.size;

    Array<FUInstance*> inputs = PushArray<FUInstance*>(perm,input);
    Array<FUInstance*> outputs = PushArray<FUInstance*>(perm,output);

    FUDeclaration* constType = GetTypeByName(versat,STRING("Const"));
    FUDeclaration* regType = GetTypeByName(versat,STRING("Reg"));

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

    topLevelTypeStr = PushString(perm,"%.*s_Simple",UNPACK_SS(topLevelTypeStr));
    type = RegisterSubUnit(versat,topLevelTypeStr,accel);
    accel = CreateAccelerator(versat);
    accel->useDMA = opts->useDMA;
    TOP = CreateFUInstance(accel,type,STRING("TOP"));
  } else {
    bool isSimple = true;
    TOP = CreateFUInstance(accel,type,STRING("TOP"));

    for(int i = 0; i < type->inputDelays.size; i++){
      String name = PushString(&versat->permanent,"input%d",i);
      FUInstance* input = CreateOrGetInput(accel,name,i);
      ConnectUnits(input,0,TOP,i);
    }
    FUInstance* output = CreateOrGetOutput(accel);
    for(int i = 0; i < type->outputLatencies.size; i++){
      ConnectUnits(TOP,i,output,i);
    }
  }

  TOP->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.LEN_W(LEN_W))");
  InitializeAccelerator(versat,accel,&versat->temp);
  OutputVersatSource(versat,accel,opts->hardwareOutputFilepath.data,opts->softwareOutputFilepath.data,topLevelTypeStr,opts->addInputAndOutputsToTop);

  //TODO: Repeated code because we must use a modified accelerator to outputVersatSource but use a normal accelerator for simulation. We could just reorder, so that outputVersatSource is done afterwards, and the wrapper is done before.
  if(isSimple){
    accel = CreateAccelerator(versat);
    TOP = CreateFUInstance(accel,type,STRING("TOP"));
    InitializeAccelerator(versat,accel,&versat->temp);
  }

  // TODO: Both config and states can implement hierarchical composition, instead of being everything single config
  // We need to extract configs and states with the expected TOP level name (not module name)
  OrderedConfigurations configs = ExtractOrderedConfigurationNames(versat,accel,&versat->permanent,&versat->temp);
  Array<Wire> allConfigsHeaderSide = OrderedConfigurationsAsArray(configs,&versat->permanent);

  // We need to bundle config + static (type->config) only contains config, but not static
  Array<Wire> allConfigsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  {
    int index = 0;
    for(Wire& config : type->configInfo.configs){
      allConfigsVerilatorSide[index++] = config;
    }
    for(Pair<StaticId,StaticData> p : type->staticUnits){
      for(Wire& config : p.second.configs){
        allConfigsVerilatorSide[index] = config;
        allConfigsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,&versat->permanent);
        index += 1;
      }
    }
    allConfigsVerilatorSide.size = index;
  }

  // Extract states with the expected TOP level name (not module name)
  Hashmap<String,SizedConfig>* namedStates = ExtractNamedSingleStates(accel,&versat->permanent);
  Array<String> statesHeaderSize = PushArray<String>(temp,namedStates->nodesUsed);
  int index = 0;
  for(Pair<String,SizedConfig> pair : namedStates){
    statesHeaderSize[index++] = pair.first;
  }
  // TODO: There is some bug with the state header not having the correct amount of entries
  //printf("%d\n",namedStates->nodesUsed);

  // Module info for top level
  ModuleInfoInstance info = {};
  info.name = type->name;
  info.inputDelays = type->inputDelays;
  info.outputLatencies = type->outputLatencies;
  info.configs = allConfigsVerilatorSide; // Bundled config + state (delay is not needed, because we know how many delays we have
  info.states = type->configInfo.states;
  info.externalInterfaces = type->externalMemory;
  info.nDelays = type->configInfo.delayOffsets.max;
  info.nIO = type->nIOs;
  info.memoryMappedBits = type->memoryMapBits;
  info.doesIO = type->nIOs > 0;
  info.memoryMapped = type->isMemoryMapped; //type->memoryMapBits >= 0;
  info.hasDone = type->implementsDone;
  info.hasClk = true;
  info.hasReset = true;
  info.hasRun = true;
  info.hasRunning = true;
  info.isSource = false; // Hack but maybe not a problem doing it this way, we only care to generate the wrapper and the instance is only done individually
  info.signalLoop = type->signalLoop;

  std::vector<ModuleInfoInstance> finalModule;
  finalModule.push_back(info);

  // Wrapper
  region(temp){
    String wrapper = PushString(temp,"%.*s/wrapper_emul.cpp",UNPACK_SS(opts->softwareOutputFilepath));
    PushNullByte(temp);

    printf("Wrapper: %.*s\n",UNPACK_SS(wrapper));

    int totalExternalMemory = ExternalMemoryByteSize(info.externalInterfaces);
    
    TemplateSetNumber("totalExternalMemory",totalExternalMemory);
    TemplateSetString("versatDir",STRINGIFY(SAT_DIR));
    //TemplateSetString("verilatorRoot",opts->verilatorRoot);
    TemplateSetNumber("bitWidth",opts->bitSize);
    TemplateSetCustom("arch",&versat->opts,"Options");
    FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");
    CompiledTemplate* comp = CompileTemplate(versat_wrapper_template,"wrapper",temp);
    OutputModuleInfos(output,info,STRING("Verilog"),comp,temp,allConfigsHeaderSide,statesHeaderSize);
    fclose(output);
  }

  // Makefile
  region(temp){
    String name = PushString(temp,"%.*s/VerilatorMake.mk",UNPACK_SS(opts->softwareOutputFilepath));
    PushNullByte(temp);

    printf("Makefile: %.*s\n",UNPACK_SS(name));

    FILE* output = OpenFileAndCreateDirectories(name.data,"w");
    CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp);
    
    fs::path outputPath = opts->softwareOutputFilepath.data;
    fs::path srcLocation = fs::current_path();
    fs::path fixedPath = fs::weakly_canonical(outputPath / srcLocation);

    String srcDir = PushString(perm,"%.*s/src",UNPACK_SS(opts->softwareOutputFilepath));

    TemplateSetCustom("arch",&versat->opts,"Options");
    TemplateSetString("srcDir",srcDir);
    TemplateSetString("versatDir",STRINGIFY(VERSAT_DIR));
    TemplateSetString("verilatorRoot",opts->verilatorRoot);
    TemplateSetNumber("bitWidth",opts->bitSize);
    TemplateSetArray("verilogFiles","String",opts->verilogFiles.data(),opts->verilogFiles.size());
    TemplateSetArray("extraSources","String",opts->extraSources.data(),opts->extraSources.size());
    TemplateSetArray("includePaths","String",opts->includePaths.data(),opts->includePaths.size());
    TemplateSetString("typename",topLevelTypeStr);
    TemplateSetString("hack",STRING("#"));
    TemplateSetString("rootPath",STRING(fixedPath.c_str()));
    ProcessTemplate(output,comp,temp);
  }

//  DebugVersat(versat);
  
  return 0;
}









