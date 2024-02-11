#include <unordered_map>
#include <sys/wait.h>
#include <ftw.h>

#include "debugVersat.hpp"
#include "graph.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "utils.hpp"
#include "verilogParsing.hpp"
#include "type.hpp"
#include "templateEngine.hpp"
#include "acceleratorStats.hpp"
#include "textualRepresentation.hpp"
#include "codeGeneration.hpp"

#include "templateData.hpp"

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

Optional<String> GetFileFormatFromPath(String filename){
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

Options* ParseCommandLineOptions(int argc,const char* argv[],Arena* out,Arena* temp){
  Options* opts = PushStruct<Options>(out);
  opts->dataSize = 32; // By default.
  opts->addrSize = 32;

  ArenaList<String>* verilogFiles = PushArenaList<String>(temp);
  ArenaList<String>* extraSources = PushArenaList<String>(temp);
  ArenaList<String>* includePaths = PushArenaList<String>(temp);
  ArenaList<String>* unitPaths = PushArenaList<String>(temp);
  
  for(int i = 1; i < argc; i++){
    String str = STRING(argv[i]);

    Optional<String> formatOpt = GetFileFormatFromPath(str);

    // TODO: Verilator does not actually need the source files, it only needs a include path to the folder that contains the sources. This could be removed.
    if(str.size >= 2 && str[0] == '-' && str[1] == 'S'){
      if(str.size == 2){
        if(i + 1 >= argc){
          printf("Missing argument\n");
          exit(-1);
        }
        *PushListElement(extraSources) = GetAbsolutePath(argv[i + 1],out);
        i += 1;
      } else {
        *PushListElement(extraSources) = GetAbsolutePath(&str.data[2],out);
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

      opts->addrSize = 32;
      continue;
    }

    if(str.size >= 4 && str[0] == '-' && str[1] == 'x' && str[2] == '6' && str[3] == '4'){
      Assert(str.size == 4);

      opts->addrSize = 64;
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
      opts->architectureHasDatabus = true;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'I'){
      if(str.size == 2){
        if(i + 1 >= argc){
          printf("Missing argument\n");
          exit(-1);
        }
        *PushListElement(includePaths) = GetAbsolutePath(argv[i + 1],out);
        i += 1;
      } else {
        *PushListElement(includePaths) = GetAbsolutePath(&str.data[2],out);
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

      String unitPath = GetAbsolutePath(argv[i+1],out);
      PushNullByte(out);

      *PushListElement(unitPaths) = unitPath;
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'o'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }
      opts->hardwareOutputFilepath = GetAbsolutePath(argv[i+1],out);
      PushNullByte(out);
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 'H'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }
      opts->softwareOutputFilepath = GetAbsolutePath(argv[i+1],out);
      PushNullByte(out);
      i += 1;
      continue;
    }
    
    if(formatOpt){
      String format = formatOpt.value();
      if(CompareString(format,"v")){
        *PushListElement(verilogFiles) = GetAbsolutePath(argv[i],out);
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

  opts->verilogFiles = PushArrayFromList(out,verilogFiles);
  opts->extraSources = PushArrayFromList(out,extraSources);
  opts->includePaths = PushArrayFromList(out,includePaths);
  opts->unitPaths = PushArrayFromList(out,unitPaths);
  
  return opts;
}

// These Call* functions are a bit hardcoded in functionality. I do not expect to call other programs often so no need to abstract for now.
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

  // TODO: These errors should be cleaned up. Good for debugging but user should not care about internals of the compiler
   if(pid < 0){
     fprintf(stderr,"Error calling fork\n");
     exit(-1);
   } else if(pid == 0){
     execvp("verilator",verilatorArgs);
     printf("Error calling execvp for verilator: %s\n",strerror(errno));
   } else {
     int status;
     wait(&status);
   }  
}

String CallMakefile(const char* makePath,const char* action,Arena* out){
  int pipeRead[2];
  int res = pipe(pipeRead);

  if(res == -1){
    fprintf(stderr,"Problem creating pipe\n");
    exit(-1);
  }
  
  pid_t pid = fork();
  
  char* const makeArgs[] = {
    (char*) "make",
    (char*) "--no-print-directory",
    (char*) "-C",
    (char*) makePath,
    (char*) action,
    nullptr
  };

  if(pid < 0){
    printf("Error calling fork\n");
  } else if(pid == 0){
    dup2(pipeRead[1],STDOUT_FILENO);

    execvp("make",makeArgs);
    printf("Error calling execvp for verilator: %s\n",strerror(errno));
  } else {
    int status;
    wait(&status);

    Byte* mark = MarkArena(out);
    size_t maximumAllowed = SpaceAvailable(out);
    int res = read(pipeRead[0],mark,maximumAllowed);

    PushBytes(out,res);
    String result = PointArena(out,mark);
    return result;
  }

  return {};
}

String GetVerilatorRoot(Arena* out,Arena* temp){
  // We currently get verilator root by running verilator over a simple example and extracting the VERILATOR_ROOT value from the makefile generated by it. It's the way I find to be the more consistent one, since the location of the verilator executable might not be inside the bin folder of VERILATOR_ROOT (happened once with an installation from apt-get for version 4.X).
  BLOCK_REGION(temp);

  int res = 0;

  String tempDir = PushString(temp,"/tmp/versatTmp_%d",getpid());
  PushNullByte(temp);
  const char* tempDirC = tempDir.data;
  
  res = mkdir(tempDirC,0770);
  if(res == -1 && errno != EEXIST){
    printf("Problem making tmp directory errno: %d\n",errno);
    exit(-1);
  }

  String testFile = PushString(temp,"`timescale 1ns / 1ps\nmodule Test(); endmodule");
  PushNullByte(temp);

  FILE* f = fopen(StaticFormat("%.*s/Test.v",UNPACK_SS(tempDir)),"w");
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

  CallVerilator(StaticFormat("%.*s/Test.v",UNPACK_SS(tempDir)),tempDirC);

  String fileContent = PushFile(temp,StaticFormat("%.*s/VTest.mk",UNPACK_SS(tempDir)));

  if(fileContent.size <= 0){
    printf("Problem opening verilator generated file\n");
    exit(-1);
  }

  Tokenizer tok(fileContent,"=",{":="});

  String root = {};
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
      String token = tok.NextToken();

      root = PushString(out,"%.*s",UNPACK_SS(token));
      break;
    }
  }
  
  //RemoveDirectory(tempDirC);
  
  return root;
}

int main(int argc,const char* argv[]){
  // TODO: Need to actually parse and give an error, instead of just checking for less than 3
  if(argc < 3){
    printf("Need specifications and a top level type\n");
    return -1;
  }
  
  InitDebug();

  Versat* versat = InitVersat();
  
  Arena permInst = InitArena(Megabyte(256));
  Arena* perm = &permInst;
  Arena tempInst = InitArena(Megabyte(256));
  Arena* temp = &tempInst;

  InitializeTemplateEngine(perm);
  InitializeSimpleDeclarations(versat);
  
  Options* opts = ParseCommandLineOptions(argc,argv,perm,temp);
  versat->opts = opts;
  versat->opts->shadowRegister = true; 
  versat->opts->noDelayPropagation = true;

  // TODO: Variable buffers are currently broken. Due to removing statics saved configurations.
  //       When reimplementing this (if we eventually do) separate buffers configs from static buffers.
  //       There was never any point in having both together.
  versat->opts->useFixedBuffers = true;

  versat->debug.outputGraphs = false;
  versat->debug.outputAcceleratorInfo = false;
  
#ifdef USE_FST_FORMAT
  versat->opts.generateFSTFormat = 1;
#endif

  // Check options
  if(!opts->topName){
    printf("Specify accelerator type using -T <type>\n");
    exit(-1);
  }
  
  opts->verilatorRoot = GetVerilatorRoot(&versat->permanent,&versat->temp);
  if(opts->verilatorRoot.size == 0){
    fprintf(stderr,"Versat could not find verilator. Check that it is installed\n");
    return -1;
  }

  // Collect all verilog source files.
  Array<String> allVerilogFiles = {};
  region(temp){
    Set<String>* allVerilogFilesSet = PushSet<String>(temp,999);

    for(String str : opts->verilogFiles){
      allVerilogFilesSet->Insert(str);
    }
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
            allVerilogFilesSet->Insert(fullPath);
          }
        }
      }
    }

    allVerilogFiles = PushArrayFromSet(perm,allVerilogFilesSet);
  }
  opts->verilogFiles = allVerilogFiles; // TODO: Kind of an hack. We lose information about file origin
  
  // Parse verilog files and register as simple units
  for(String file : opts->verilogFiles){
    String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(file)));

    if(content.size == 0){
      printf("Failed to open file %.*s\n. Exiting\n",UNPACK_SS(file));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(content,opts->includePaths,temp,perm);
    Array<Module> modules = ParseVerilogFile(processed,opts->includePaths,temp,perm);

    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm,temp);
      RegisterModuleInfo(versat,&info);
    }
  }
  
// We need to do this afer parsing the modules because the majority of these come from verilog files.
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
  
  FUDeclaration* type = GetTypeByName(versat,topLevelTypeStr);
  Accelerator* accel = CreateAccelerator(versat,STRING("TOP"));
  FUInstance* TOP = nullptr;

  bool isSimple = false;
  if(opts->addInputAndOutputsToTop && !(type->inputDelays.size == 0 && type->outputLatencies.size == 0)){
    int input = type->inputDelays.size;
    int output = type->outputLatencies.size;

    Array<FUInstance*> inputs = PushArray<FUInstance*>(perm,input);
    Array<FUInstance*> outputs = PushArray<FUInstance*>(perm,output);

    FUDeclaration* constType = GetTypeByName(versat,STRING("TestConst"));
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

    InstanceNode* node = GetInstanceNode(accel,TOP);
    node->type = InstanceNode::TAG_COMPUTE; // MARK: Temporary handle source_and_delay problems for simple units.
    
    topLevelTypeStr = PushString(perm,"%.*s_Simple",UNPACK_SS(topLevelTypeStr));
    type = RegisterSubUnit(versat,topLevelTypeStr,accel);

    accel = CreateAccelerator(versat,STRING("TOP"));
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
  OutputVersatSource(versat,accel,opts->hardwareOutputFilepath.data,opts->softwareOutputFilepath.data,topLevelTypeStr,opts->addInputAndOutputsToTop,temp,perm);

  //TODO: Repeated code because we must use a modified accelerator to outputVersatSource but use a normal accelerator for simulation. We could just reorder, so that outputVersatSource is done afterwards, and the wrapper is done before.
  if(isSimple){
    accel = CreateAccelerator(versat,STRING("TOP"));
    TOP = CreateFUInstance(accel,type,STRING("TOP"));
  }
  
  OutputVerilatorWrapper(versat,type,accel,opts->softwareOutputFilepath,temp,perm);

  String versatDir = STRING(STRINGIFY(VERSAT_DIR));
  OutputVerilatorMake(versat,topLevelTypeStr,versatDir,opts,temp,perm);

  for(FUDeclaration* decl : versat->declarations){
    BLOCK_REGION(temp);
    if(decl->type == FUDeclaration::COMPOSITE && decl->mergeInfo.size != 0){
      char buffer[256];
      sprintf(buffer,"%.*s/modules/%.*s.v",UNPACK_SS(versat->opts->hardwareOutputFilepath),UNPACK_SS(decl->name));
      FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
      OutputCircuitSource(versat,decl,decl->fixedDelayCircuit,sourceCode,temp,perm);
      fclose(sourceCode);
    }
    if(decl->type == FUDeclaration::COMPOSITE && decl->mergeInfo.size == 0){
      char buffer[256];
      sprintf(buffer,"%.*s/modules/%.*s.v",UNPACK_SS(versat->opts->hardwareOutputFilepath),UNPACK_SS(decl->name));
      FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
      OutputCircuitSource(versat,decl,decl->fixedDelayCircuit,sourceCode,temp,perm);
      fclose(sourceCode);
    }
    if(decl->type == FUDeclaration::ITERATIVE){
      char buffer[256];
      sprintf(buffer,"%.*s/modules/%.*s.v",UNPACK_SS(versat->opts->hardwareOutputFilepath),UNPACK_SS(decl->name));
      FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
      OutputIterativeSource(versat,decl,decl->fixedDelayCircuit,sourceCode,temp,perm);
      fclose(sourceCode);
    }

    if(versat->debug.outputAcceleratorInfo && decl->fixedDelayCircuit){
      char buffer[256];
      sprintf(buffer,"debug/%.*s/stats.txt",UNPACK_SS(decl->name));
      FILE* stats = OpenFileAndCreateDirectories(buffer,"w");
 
      PrintDeclaration(stats,decl,perm,temp);
    }
  }
  
  return 0;
}

/*

Variety1 Problem:

- The source of the problem comes from the fact that the Variety1 unit is being tagged as a SOURCE_AND_SINK unit which prevents the delay values from propagating. This comes from the Reg connection.
- The actual solution would be to completely change delays to actually track the input and output ports instead of the units themselves. Furthermore, if we can describe the relationship between input and output port (wether output X depends on input Y) then we can further improve the delay algorithm to produce lower values and decouple dependencies that exist from hierarchical graphs and from units that are 1 but behave like multiple (for example, memories, that contain 2 ports and the delay should act individually.).
  - Another thing. SOURCE_AND_DELAY should be used for delays that can be used for input or for output depending on how the unit is connected.

How to handle merged subunits.
- One problem current implementation has is that we are calculating delays for the worst accelerator case, while we could calculate for each case and store the information in versat_accel.h
- How would this look like with a simple example (like SHA_AES_Simple)?.
  - The thing about this is that the SHA_AES_Simple declaration would need to store delay information for each case.
  - Meaning that the SHA_AES_Simple declaration would also need to be able to store different information depending on the type being used by the merged unit.
  - This means that merging info propragates accross units, so that parent units must also contain N amount of data for each N accelerators that are merged.
  - This also means that if we have two merged units of N and M accelerators merged, then the parent unit would need to store N * M data.
  - This makes sense and techically not only works for delay but also for latencies.
  - If parent unit contains a merged unit that merged 2 accelerators, one with a latency of 10 and another with a latency of 50, then the latency of the parent unit
    will also depend on which accelerator is active at the moment, the one with a latency of 10 or the one with latency of 50.
  - We could then think of the parent declaration has having multiple declarations depending on the merged accelerator active. Default declaration, declaration with 0 active, declaration with 1 active and so on.
  - Each active accelerator would change the delays of the unit, which would then change the delays of the parent declaration and so on.
  - Other than delays, I cannot think of anything that also behaves this way. Even though configs change between accelerators, this only matters when generating the structs, the parent declaration does not need special treatment 
  - when generating structs because 

 */

