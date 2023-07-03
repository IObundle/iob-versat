#include "versat.hpp"
#include "utils.hpp"
#include "verilogParsing.hpp"
#include "type.hpp"
#include "templateEngine.hpp"
#include "unitVerilation.hpp"
#include "debugGUI.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "templateData.hpp"

// TODO: For some reason, in the makefile we cannot stringify the arguments that we want to
//       Do not actually want to do these preprocessor things. Fix the makefile so that it passes as a string
#define DO_STRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) DO_STRINGIFY(ARG)

#if 0
struct OptionFormat{
   String name;

};
#endif

struct ArgumentOptions{
   std::vector<String> verilogFiles;
   std::vector<const char*> includePaths;
   std::vector<const char*> extraSources;
   const char* specificationFilepath;
   const char* topName;
   String outputFilepath;
   String verilatorRoot;
   int bitSize;
   bool addInputAndOutputsToTop;
   bool useDMA;
};

String GetAbsolutePath(const char* path,Arena* arena){
   fs::path relative = path;
   fs::path absolute = fs::canonical(fs::absolute(relative));
   String res = PushString(arena,"%s",absolute.c_str());
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

// Before continuing, find a more generic approach, check getopt or related functions
// Try to follow the gcc conventions
ArgumentOptions* ParseCommandLineOptions(int argc,const char* argv[],Arena* perm,Arena* temp){
   ArgumentOptions* opts = PushStruct<ArgumentOptions>(perm);

   opts->bitSize = sizeof(void*) * 8; // TODO: After rewriting, need to take into account default values

   for(int i = 1; i < argc; i++){
      String str = STRING(argv[i]);

      Optional<String> formatOpt = GetFormat(str);

      if(str.size >= 2 && str[0] == '-' && str[1] == 'S'){
         if(str.size == 2){
            if(i + 1 >= argc){
               printf("Missing argument\n");
               exit(-1);
            }
            opts->extraSources.push_back(argv[i + 1]);
            i += 1;
         } else {
            opts->extraSources.push_back(&str.data[2]);
         }

         continue;
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

      if(str.size >= 2 && str[0] == '-' && str[1] == 'I'){
         if(str.size == 2){
            if(i + 1 >= argc){
               printf("Missing argument\n");
               exit(-1);
            }
            opts->includePaths.push_back(argv[i + 1]);
            i += 1;
         } else {
            opts->includePaths.push_back(&str.data[2]);
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

      if(str.size >= 2 && str[0] == '-' && str[1] == 'o'){
         if(i + 1 >= argc){
            printf("Missing argument\n");
            exit(-1);
         }
         opts->outputFilepath = GetAbsolutePath(argv[i+1],perm);
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

//General TODO: There is a lot of code that makes sense for the old one phase versat, but that are just overhead for the two phase versat. We do not have to guarantee that the Accelerator is "valid" at all times, like the previous version did. We can use more immediate mode APIs and rely on correct function calling instead of pre doing work.

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Need specifications and a top level type\n");
      return -1;
   }

   Versat* versat = InitVersat(0,1);

   // TODO: This is not a good idea, after changing versat to 2 phases
   SetDebug(versat,VersatDebugFlags::OUTPUT_ACCELERATORS_CODE,1);
   SetDebug(versat,VersatDebugFlags::OUTPUT_VERSAT_CODE,1);
   SetDebug(versat,VersatDebugFlags::USE_FIXED_BUFFERS,0);
   SetDebug(versat,VersatDebugFlags::OUTPUT_GRAPH_DOT,0);
   SetDebug(versat,VersatDebugFlags::OUTPUT_VCD,0);

   Arena permInst = InitArena(Megabyte(256));
   Arena* perm = &permInst;
   Arena preprocessInst = InitArena(Megabyte(256));
   Arena* preprocess = &preprocessInst;
   Arena tempInst = InitArena(Megabyte(256));
   Arena* temp = &tempInst;

   // TODO: Add options directly to versat
   ArgumentOptions* opts = ParseCommandLineOptions(argc,argv,perm,temp);
   versat->outputLocation = opts->outputFilepath;

   // Check existance of Verilator. We cannot proceed without Verilator
   if(opts->verilatorRoot.size == 0){
      bool lackOfVerilator = false;
      String vr = {};
#ifdef VERILATOR_ROOT
      vr = TrimWhitespaces(STRING(STRINGIFY(VERILATOR_ROOT)));
      //TODO: Could have some extra checks here, like make sure that it's a valid path
      if(vr.size == 0){
         lackOfVerilator = true;
      }
      printf("VERILATOR_ROOT: %s\n",STRINGIFY(VERILATOR_ROOT));
#else
      lackOfVerilator = true;
#endif

      if(lackOfVerilator){
         char* possible = getenv(STRINGIFY(VERILATOR_ROOT));
         if(possible){
            vr = STRING(possible);
            lackOfVerilator = false;
         }
      }

      if(lackOfVerilator){
         printf("===\n\n");
         printf("Verilator root is not defined. Make sure that verilator is correctly installed or\n");
         printf("set VERILATOR_ROOT to the top folder of verilator\n");
         printf("Folders $(VERILATOR_ROOT)/bin and $(VERILATOR_ROOT)/include\n");
         printf("\n\n===\n");
         exit(-1);
      }
      opts->verilatorRoot = vr;
   }

   if(!opts->topName){
      printf("Specify accelerator type using -T <type>\n");
      exit(-1);
   }

   #if 1
   std::vector<ModuleInfo> allModules;
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
         allModules.push_back(info);

         RegisterModuleInfo(versat,&info);
      }
   }
   #endif

   // Compile all templates before hand
   #if 1
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
   #endif

   const char* specFilepath = opts->specificationFilepath;
   String topLevelTypeStr = STRING(opts->topName);

   if(specFilepath){
      ParseVersatSpecification(versat,specFilepath);
   }

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

   if(!opts->outputFilepath.data){
      opts->outputFilepath = GetAbsolutePath(".",perm);
   }

   TemplateSetNumber("bitWidth",sizeof(void*) * 8); // TODO: Move to correct location

   TOP->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W))");
   InitializeAccelerator(versat,accel,&versat->temp);
   OutputVersatSource(versat,accel,opts->outputFilepath.data,topLevelTypeStr,opts->addInputAndOutputsToTop);

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
   Array<Wire> allConfigsVerilatorSide = PushArray<Wire>(temp,99); // TODO: Correct size
   #if 1
   {
      int index = 0;
      for(Wire& config : type->configs){
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
   #endif

   // Extract states with the expected TOP level name (not module name)
   Hashmap<String,SizedConfig>* namedStates = ExtractNamedSingleStates(accel,&versat->permanent);
   Array<String> statesHeaderSize = PushArray<String>(temp,namedStates->nodesUsed);
   int index = 0;
   for(Pair<String,SizedConfig> pair : namedStates){
      statesHeaderSize[index++] = pair.first;
   }

   // Module info for top level
   ModuleInfo info = {};
   info.name = type->name;
   info.inputDelays = type->inputDelays;
   info.outputLatencies = type->outputLatencies;
   info.configs = allConfigsVerilatorSide; // Bundled config + state (delay is not needed, because we know how many delays we have
   info.states = type->states; 
   info.externalInterfaces = type->externalMemory;
   info.nDelays = type->delayOffsets.max;
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
   
   std::vector<ModuleInfo> finalModule;
   finalModule.push_back(info);

   // Wrapper
   region(temp){
      String wrapper = PushString(temp,"%s/wrapper.cpp",opts->outputFilepath.data);
      PushNullByte(temp);

      TemplateSetString("versatDir",STRINGIFY(VERSAT_DIR));
      TemplateSetString("verilatorRoot",opts->verilatorRoot);
      TemplateSetNumber("bitWidth",opts->bitSize);
      FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");
      CompiledTemplate* comp = CompileTemplate(versat_wrapper_template,"wrapper",temp);
      OutputModuleInfos(output,false,(Array<ModuleInfo>){finalModule.data(),(int) finalModule.size()},STRING("Verilog"),comp,temp,allConfigsHeaderSide,statesHeaderSize);
      fclose(output);
   }

   // Makefile
   region(temp){
      String name = PushString(temp,"%s/Makefile",opts->outputFilepath.data);
      PushNullByte(temp);

      FILE* output = OpenFileAndCreateDirectories(name.data,"w");
      CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp);

      fs::path outputPath = opts->outputFilepath.data;
      fs::path srcLocation = fs::current_path();
      fs::path fixedPath = fs::weakly_canonical(outputPath / srcLocation);

      String srcDir = PushString(perm,"%.*s/src",UNPACK_SS(opts->outputFilepath));

      TemplateSetString("srcDir",srcDir);
      TemplateSetString("versatDir",STRINGIFY(VERSAT_DIR));
      TemplateSetString("verilatorRoot",opts->verilatorRoot);
      TemplateSetNumber("bitWidth",opts->bitSize);
      TemplateSetArray("verilogFiles","String",opts->verilogFiles.data(),opts->verilogFiles.size());
      TemplateSetArray("extraSources","char*",opts->extraSources.data(),opts->extraSources.size());
      TemplateSetArray("includePaths","char*",opts->includePaths.data(),opts->includePaths.size());
      TemplateSetString("typename",topLevelTypeStr);
      TemplateSetString("hack",STRING("#"));
      TemplateSetString("rootPath",STRING(fixedPath.c_str()));
      ProcessTemplate(output,comp,temp);
   }

   return 0;
}









