#include <unordered_map>
#include <sys/wait.h>
#include <ftw.h>

#include "debugVersat.hpp"
#include "declaration.hpp"
#include "globals.hpp"
#include "memory.hpp"
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

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

Opt<String> GetFileFormatFromPath(String filename){
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

  return Opt<String>();
}

#if 0
// TODO: There is no reason to use small arguments. -d should just be -DMA. -D should just be -Databus and so on.
// TODO: Rewrite to either use argparse or some lib parsing, or just parse ourselves but simplify adding more options and report errors.
Options* ParseCommandLineOptions(int argc,char* argv[],Arena* out,Arena* temp){
  Options* opts = PushStruct<Options>(out);
  opts->dataSize = 32; // By default.
  opts->addrSize = 32;

  opts->debugPath = PushString(out,"%s/debug",GetCurrentDirectory()); // By default
  PushNullByte(out);
  
  ArenaList<String>* verilogFiles = PushArenaList<String>(temp);
  ArenaList<String>* extraSources = PushArenaList<String>(temp);
  ArenaList<String>* includePaths = PushArenaList<String>(temp);
  ArenaList<String>* unitPaths = PushArenaList<String>(temp);
  
  for(int i = 1; i < argc; i++){
    String str = STRING(argv[i]);

    Opt<String> formatOpt = GetFileFormatFromPath(str);

    // TODO: Verilator does not actually need the source files for non top units. It only needs a include path to the folder that contains the sources and the verilog file of the top module. This could be removed. But also need to test further.
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
      opts->topName = STRING(argv[i + 1]);
      i += 1;
      continue;
    }

    if(str.size >= 2 && str[0] == '-' && str[1] == 's'){
      opts->addInputAndOutputsToTop = true;
      continue;
    }

#if 0
    if(str.size >= 2 && str[0] == '-' && str[1] == 'g'){
      opts->debug = true;
      continue;
    }
#endif
    
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

    if(str.size >= 2 && str[0] == '-' && str[1] == 'A'){
      if(i + 1 >= argc){
        printf("Missing argument\n");
        exit(-1);
      }

      opts->debug = true;

      String debugPath = GetAbsolutePath(argv[i+1],out);
      PushNullByte(out);
     
      opts->debugPath = debugPath;
      PushNullByte(out);

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
        if(opts->specificationFilepath.size){
          printf("Error, multiple specification files specified, not supported for now\n");
          exit(-1);
        }

        opts->specificationFilepath = STRING(argv[i]);
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
#endif

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

     if(status != 0){
       printf("There was a problem calling verilator\n");
       printf("Versat might not produce correct results\n");
     }
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

    auto mark = StartString(out);
    size_t maximumAllowed = SpaceAvailable(out);
    int res = read(pipeRead[0],mark.mark,maximumAllowed);

    PushBytes(out,res);
    String result = EndString(mark);
    return result;
  }

  return {};
}

String GetVerilatorRoot(Arena* out,Arena* temp){
  // We currently get verilator root by running verilator over a simple example and extracting the VERILATOR_ROOT value from the makefile generated by it. It's the way I find to be the more consistent one, since the location of the verilator executable might not be inside the bin folder of VERILATOR_ROOT (happened once with an installation from apt-get for version 4.X).
  BLOCK_REGION(temp);

  int res = 0;

  // TODO: This way of getting a temporary folder is not the most portable. See "mkdtemp"
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
    Opt<Token> possible = tok.PeekFindIncluding("VERILATOR_ROOT");

    if(!possible.has_value()){
      printf("Couldn't find the location of verilator root\n");
      exit(-1);
    }

    tok.AdvancePeek(possible.value());

    Token peek = tok.PeekToken();
    if(CompareString(peek,"=") || CompareString(peek,":=")){
      tok.AdvancePeek(peek);
      String token = tok.NextToken();

      root = PushString(out,"%.*s",UNPACK_SS(token));
      break;
    }
  }
  
  RemoveDirectory(tempDirC);
  
  return root;
}

// TODO: Small fix for common template. Works for now 
void SetIncludeHeader(CompiledTemplate* tpl,String name);

#include "parser.hpp"

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
  Assert(val.type == GetType(STRING("FUInstance*")));
  
  FUInstance** instPtr = (FUInstance**) val.custom;
  FUInstance* inst = *instPtr;

  String ret = PushString(out,"%.*s_%d",UNPACK_SS(inst->name),inst->id);
  
  return MakeValue(ret);
}

#include "templateData.hpp"

void LoadTemplates(Arena* perm,Arena* temp){
  CompiledTemplate* commonTpl = CompileTemplate(versat_common_template,"common",perm,temp);
  SetIncludeHeader(commonTpl,STRING("common"));

  BasicTemplates::acceleratorTemplate = CompileTemplate(versat_accelerator_template,"accel",perm,temp);
  BasicTemplates::topAcceleratorTemplate = CompileTemplate(versat_top_instance_template,"top",perm,temp);
  BasicTemplates::acceleratorHeaderTemplate = CompileTemplate(versat_header_template,"header",perm,temp);
  BasicTemplates::externalInternalPortmapTemplate = CompileTemplate(external_memory_internal_portmap_template,"ext_internal_port",perm,temp);
  BasicTemplates::externalPortTemplate = CompileTemplate(external_memory_port_template,"ext_port",perm,temp);
  BasicTemplates::externalInstTemplate = CompileTemplate(external_memory_inst_template,"ext_inst",perm,temp);
  BasicTemplates::iterativeTemplate = CompileTemplate(versat_iterative_template,"iter",perm,temp);
  BasicTemplates::internalWiresTemplate = CompileTemplate(versat_internal_memory_wires_template,"internal wires",perm,temp);

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

#include <argp.h>


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

  switch (key)
    {
    case 'S': *PushListElement(opts->extraSources) = STRING(arg); break;
    case 'I': *PushListElement(opts->includePaths) = STRING(arg); break;
    case 'u': *PushListElement(opts->unitPaths) = STRING(arg); break;

    case 'b': opts->options->dataSize = ParseInt(STRING(arg)); break;
    case 'x': opts->options->addrSize = ParseInt(STRING(arg)); break;

    case 'd': opts->options->useDMA = true; break;
    case 'D': opts->options->architectureHasDatabus = true; break;
    case 's': opts->options->addInputAndOutputsToTop = true; break;

    case 'g': opts->options->debugPath = STRING(arg); break;
    case 't': opts->options->topName = STRING(arg); break;
    case 'o': opts->options->hardwareOutputFilepath = STRING(arg); break;
    case 'O': opts->options->softwareOutputFilepath = STRING(arg); break;
      
    case ARGP_KEY_ARG: opts->options->specificationFilepath = STRING(arg); break;
    case ARGP_KEY_END: break;
    }
  return 0;
}

int main(int argc,char* argv[]){
  InitDebug();

  *globalPermanent = InitArena(Megabyte(128));

  Arena* perm = globalPermanent;
  
  Arena tempInst = InitArena(Megabyte(128));
  Arena* temp = &tempInst;
  Arena temp2Inst = InitArena(Megabyte(128));
  Arena* temp2 = &temp2Inst;

  // Better error handling
  struct argp_option options[] =
    {
      { 0, 'S',"File", 0, "Extra sources"},
      { 0, 'b',"Size", 0, "Databus size (8,16,default:32,64)"},
      { 0, 'x',"Size", 0, "Address size (default:32,64)"},
      { 0, 'd', 0,     0, "Use DMA"},
      { 0, 'D', 0,     0, "Architecture has databus"},
      { 0, 'I',"Path", 0, "Include paths"},
      { 0, 't',"Name", 0, "Top unit name"},
      { 0, 's', 0,     0, "Insert consts and regs if Top unit contains inputs and outputs"},
      { 0, 'u',"Path", 0, "Units path"},
      { 0, 'g',"Path", OPTION_HIDDEN, "Debug path"},
      { 0, 'o',"Path", 0, "Hardware output path"},
      { 0, 'O',"Path", 0, "Software output path"},
      { 0 }
    };
  argp argp = { options, parse_opt, "SpecFile", "Dataflow to accelerator compiler. Check tutorial in https://github.com/IObundle/iob-versat to learn how to write a specification file"};

  OptionsGather gather = {};

  gather.verilogFiles = PushArenaList<String>(temp);
  gather.extraSources = PushArenaList<String>(temp);
  gather.includePaths = PushArenaList<String>(temp);
  gather.unitPaths = PushArenaList<String>(temp);
  gather.options = &globalOptions;

  if(argp_parse(&argp, argc, argv, 0, 0, &gather) != 0){
    printf("Error parsing arguments. Call -h help to print usage and argument help\n");
    return -1;
  }

  globalOptions.verilogFiles = PushArrayFromList(perm,gather.verilogFiles);
  globalOptions.extraSources = PushArrayFromList(perm,gather.extraSources);
  globalOptions.includePaths = PushArrayFromList(perm,gather.includePaths);
  globalOptions.unitPaths = PushArrayFromList(perm,gather.unitPaths);

  // Check options 
  if(globalOptions.topName.size == 0){
    printf("Need to specific top unit with -t\n");
    exit(-1);
  }
  
  RegisterTypes();
  
  InitializeTemplateEngine(perm);
  LoadTemplates(perm,temp);
  InitializeSimpleDeclarations();
  
  MakeDirectory(globalOptions.debugPath.data);
  
  // TODO: Variable buffers are currently broken. Due to removing statics saved configurations.
  //       When reimplementing this (if we eventually do) separate buffers configs from static buffers.
  //       There was never any point in having both together.
  globalOptions.useFixedBuffers = true;
  globalOptions.shadowRegister = true; 
  globalOptions.disableDelayPropagation = true;

  globalDebug.outputAccelerator = true;
  globalDebug.outputVersat = true;

  globalDebug.outputConsolidationGraphs = false;

#if 0
  globalDebug.dotFormat = GRAPH_DOT_FORMAT_NAME;
#if 1
  globalDebug.outputGraphs = true;
  globalDebug.outputAcceleratorInfo = true;
#endif
  globalDebug.outputVCD = true;
#endif
  
#ifdef USE_FST_FORMAT
  globalOptions.generateFSTFormat = 1;
#endif

  globalOptions.verilatorRoot = GetVerilatorRoot(perm,temp);
  if(globalOptions.verilatorRoot.size == 0){
    fprintf(stderr,"Versat could not find verilator. Check that it is installed\n");
    return -1;
  }

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
  globalOptions.verilogFiles = allVerilogFiles; // TODO: Kind of an hack. We lose information about file origin
  
  // Parse verilog files and register as simple units
  for(String file : globalOptions.verilogFiles){
    String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(file)));

    if(content.size == 0){
      printf("Failed to open file %.*s\n. Exiting\n",UNPACK_SS(file));
      exit(-1);
    }

    String processed = PreprocessVerilogFile(content,globalOptions.includePaths,temp,temp2);
    Array<Module> modules = ParseVerilogFile(processed,globalOptions.includePaths,temp,temp2);
    
    for(Module& mod : modules){
      ModuleInfo info = ExtractModuleInfo(mod,perm,temp);
      RegisterModuleInfo(&info,temp);
    }
  }
  
  // We need to do this after parsing the modules because the majority of these special types come from verilog files.
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

  if(specFilepath.size){
    ParseVersatSpecificationFromFilepath(specFilepath,temp,temp2);
  }

  FUDeclaration* type = GetTypeByName(topLevelTypeStr);
  if(!type){
    printf("Did not find the top level type: %.*s\n",UNPACK_SS(topLevelTypeStr));
    return -1;
  }

  Accelerator* accel = nullptr;
  FUInstance* TOP = nullptr;

  type->signalLoop = true;

  if(globalOptions.addInputAndOutputsToTop && !(type->NumberInputs() == 0 && type->NumberOutputs() == 0)){
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

    FUInstance* node = TOP;
    
    type = RegisterSubUnit(accel,temp,temp2);
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

  // Disable debug for now
  if(globalOptions.debug){
    String path = PushDebugPath(temp,{},STRING("allDeclarations.txt"));
    FILE* allDeclarations = fopen(StaticFormat("%.*s",UNPACK_SS(path)),"w");
    DEFER_CLOSE_FILE(allDeclarations);
    for(FUDeclaration* decl : globalDeclarations){
      BLOCK_REGION(temp);
      
      if(globalDebug.outputAcceleratorInfo && decl->fixedDelayCircuit){
        BLOCK_REGION(temp2);
        
        String path = PushDebugPath(temp,decl->name,STRING("stats.txt"));

        FILE* stats = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(path)),"w");
        DEFER_CLOSE_FILE(stats);

        Accelerator* test = CreateAccelerator(STRING("TEST"),AcceleratorPurpose_TEMP);
        CreateFUInstance(test,decl,STRING("TOP"));
        AccelInfo info = CalculateAcceleratorInfo(test,true,temp,temp2);
        
        PrintRepr(stats,MakeValue(&info),temp,temp2);
      }
    }
  }
  
  TOP->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.LEN_W(LEN_W))");
  OutputVersatSource(accel,
                     globalOptions.hardwareOutputFilepath.data,
                     globalOptions.softwareOutputFilepath.data,
                     globalOptions.addInputAndOutputsToTop,temp,perm);

  OutputVerilatorWrapper(type,accel,globalOptions.softwareOutputFilepath,temp,perm);

  String versatDir = STRING(STRINGIFY(VERSAT_DIR));
  OutputVerilatorMake(accel->name,versatDir,temp,temp2);

  for(FUDeclaration* decl : globalDeclarations){
    BLOCK_REGION(temp);

    if(decl->type == FUDeclarationType_COMPOSITE ||
       decl->type == FUDeclarationType_ITERATIVE ||
       decl->type == FUDeclarationType_MERGED){

      if(globalOptions.debug){
        GraphPrintingContent content = GenerateDefaultPrintingContent(decl->fixedDelayCircuit,temp,temp2);
        String repr = GenerateDotGraph(decl->fixedDelayCircuit,content,temp,temp2);
        String debugPath = PushDebugPath(temp,decl->name,STRING("NormalGraph.dot"));

        FILE* file = fopen(StaticFormat("%.*s",UNPACK_SS(debugPath)),"w");
        DEFER_CLOSE_FILE(file);
        if(!file){
          printf("Error opening file for debug outputting: %.*s\n",UNPACK_SS(debugPath));
        } else {
          fwrite(repr.data,sizeof(char),repr.size,file);
        }
      }
      
      String path = PushString(temp,"%.*s/modules/%.*s.v",UNPACK_SS(globalOptions.hardwareOutputFilepath),UNPACK_SS(decl->name));
      PushNullByte(temp);

      FILE* sourceCode = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(path)),"w");
      DEFER_CLOSE_FILE(sourceCode);

      if(decl->type == FUDeclarationType_COMPOSITE || decl->type == FUDeclarationType_MERGED){
        OutputCircuitSource(decl,sourceCode,temp,perm);
      } else if(decl->type == FUDeclarationType_ITERATIVE){
        OutputIterativeSource(decl,sourceCode,temp,perm);
      }
    }
  }

  return 0;
}

/*

TODO: Need to check the isSource member of ModuleInfo and so on. 

TODO: Because of the delay implementation, there is a possibility of allowing inter-hierarchy optimizations:
      Think a VRead connected a reg file that uses delays to store a value per reg.
      The normal implementation would insert a bunch of delays with an incremental value which is wasteful because we could just change reg delays.
      This requires to keep track for each module which one allows this to happen and then take that into account in the delay calculation functions. Note: Input delay does not work. We are talking about delay info propagating downards accross the hiearchy while input delay propagates upwards. We can still resolve this during the register FUDeclaration by calcuting the delays of lower units inside the register of the upper unit.

 */



















