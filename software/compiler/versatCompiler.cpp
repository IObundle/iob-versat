#include "versat.hpp"
#include "versatPrivate.hpp"
#include "utils.hpp"
#include "verilogParsing.hpp"
#include "type.hpp"
#include "templateEngine.hpp"
#include "unitVerilation.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "templateData.hpp"

#if 0
struct OptionFormat{
   String name;

};

OptionFormat optionsAvailable[] = {STRING(}
#endif

struct Options{
   std::vector<String> verilogFiles;
   std::vector<const char*> includePaths;
   const char* specificationFilepath;
   const char* topName;
   const char* outputFilepath;
   bool addInputAndOutputsToTop;
};

Optional<String> GetFormat(String filename){
   int size = filename.size;
   for(int i = 0; i < filename.size; i++){
      if(filename[size - i - 1] == '.'){
         String res = (String){&filename[size - i],i};
         return res;
      }
   }

   return Optional<String>();
}

// Before continuing, find a more generic approach, check getopt or related functions
// Try to follow the gcc conventions
Options* ParseCommandLineOptions(int argc,const char* argv[],Arena* perm,Arena* temp){
   Options* opts = PushStruct<Options>(perm);

   for(int i = 1; i < argc; i++){
      String str = STRING(argv[i]);

      Optional<String> formatOpt = GetFormat(str);

      if(formatOpt){
         String format = formatOpt.value();
         if(CompareString(format,"v")){
            fs::path relative = argv[i];
            fs::path absolute = fs::canonical(fs::absolute(relative));
            opts->verilogFiles.push_back(PushString(perm,"%s",absolute.c_str()));
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
      }

      if(str.size >= 2 && str[0] == '-' && str[1] == 'o'){
         if(i + 1 >= argc){
            printf("Missing argument\n");
            exit(-1);
         }
         opts->outputFilepath = argv[i + 1];
         i += 1;
      }
   }

   return opts;
}

int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Need specifications and a top level type\n");
      return -1;
   }

   Versat* versat = InitVersat(0,1);

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

   Options* opts = ParseCommandLineOptions(argc,argv,perm,temp);

   if(!opts->specificationFilepath || !opts->topName){
      printf("Missing specification filepath or accelerator type\n");
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

   //String wrapper = PushString(temp,"%s/wrapper.inc",opts->outputFilepath);
   //PushNullByte(temp);

   //FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");
   //CompiledTemplate* comp = CompileTemplate(unit_verilog_data,temp);

   //OutputModuleInfos(output,false,(Array<ModuleInfo>){allModules.data(),(int) allModules.size()},STRING("Verilog"),comp,temp);

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

   ParseVersatSpecification(versat,specFilepath);

   FUDeclaration* type = GetTypeByName(versat,topLevelTypeStr);
   Accelerator* accel = CreateAccelerator(versat);
   FUInstance* TOP = nullptr;

   if(opts->addInputAndOutputsToTop){
      TOP = CreateFUInstance(accel,type,STRING("simple"));

      FUDeclaration* constType = GetTypeByName(versat,STRING("Const"));
      FUDeclaration* regType = GetTypeByName(versat,STRING("Reg"));
      for(int i = 0; i < type->inputDelays.size; i++){
         String name = PushString(perm,"input_%d",i);
         FUInstance* inst = CreateFUInstance(accel,constType,name);
         ConnectUnits(inst,0,TOP,i);
      }
      for(int i = 0; i < type->outputLatencies.size; i++){
         String name = PushString(perm,"output_%d",i);
         FUInstance* inst = CreateFUInstance(accel,regType,name);
         ConnectUnits(TOP,i,inst,0);
      }
      topLevelTypeStr = PushString(perm,"%.*s_Simple",UNPACK_SS(topLevelTypeStr));
      type = RegisterSubUnit(versat,topLevelTypeStr,accel);
      accel = CreateAccelerator(versat);
   }

   TOP = CreateFUInstance(accel,type,STRING("TOP"));

   if(!opts->outputFilepath){
      opts->outputFilepath = ".";
   }

   OutputVersatSource(versat,accel,opts->outputFilepath,topLevelTypeStr,opts->addInputAndOutputsToTop);

   // Need to create an accelerator wrapper of module given
   /*
   String generatedVerilogPath = PushString(temp,"%s/%.*s.v",opts->outputFilepath,UNPACK_SS(topLevelTypeStr));
   PushNullByte(temp);
   CheckOrCompileUnit(generatedVerilogPath,temp);

   String content = PushFile(temp,generatedVerilogPath.data);
   String processed = PreprocessVerilogFile(preprocess,content,&opts->includePaths,temp);
   std::vector<Module> modules = ParseVerilogFile(processed,&opts->includePaths,temp);

   Assert(modules.size() == 1);
   */

   //CheckOrCompileUnit(generatedVerilogPath,temp);
   #if 0
   region(temp){
   String wrapper = PushString(temp,"%s/%.*s.v",opts->outputFilepath,UNPACK_SS(topLevelTypeStr));
   PushNullByte(temp);
   FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");

   OutputCircuitSource(versat,type,type->fixedDelayCircuit,output);
   fclose(output);
   }
   #endif

   ModuleInfo info = {};
   info.name = type->name;
   info.inputDelays = type->inputDelays;
   info.outputLatencies = type->outputLatencies;
   info.configs = type->configs;
   info.states = type->states;
   info.externalInterfaces = type->externalMemory;
   info.nDelays = type->delayOffsets.max;
   info.nIO = type->nIOs;
   info.memoryMappedBits = type->memoryMapBits;
   info.doesIO = type->nIOs > 0;
   info.memoryMapped = type->memoryMapBits >= 0;
   info.hasDone = type->implementsDone;
   info.hasClk = true;
   info.hasReset = true;
   info.hasRun = true;
   info.hasRunning = true;
   info.isSource = false; // Hack but maybe not a problem doing it this way

   info.configs.size = type->configOffsets.max;
   info.states.size = type->delayOffsets.max;

   std::vector<ModuleInfo> finalModule;
   //ModuleInfo info = ExtractModuleInfo(modules[0],perm,temp);
   finalModule.push_back(info);

   region(temp){
      String wrapper = PushString(temp,"%s/wrapper.inc",opts->outputFilepath);
      PushNullByte(temp);

      FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");
      CompiledTemplate* comp = CompileTemplate(unit_verilog_data_template,temp);
      OutputModuleInfos(output,false,(Array<ModuleInfo>){finalModule.data(),(int) finalModule.size()},STRING("Verilog"),comp,temp);
      fclose(output);
   }

   region(temp){
      String name = PushString(temp,"%s/Makefile",opts->outputFilepath);
      PushNullByte(temp);

      FILE* output = OpenFileAndCreateDirectories(name.data,"w");
      CompiledTemplate* comp = CompileTemplate(versat_makefile_template,temp);

      TemplateSetArray("verilogFiles","String",opts->verilogFiles.data(),opts->verilogFiles.size());
      TemplateSetString("typename",topLevelTypeStr);
      TemplateSetString("hack",STRING("#"));
      TemplateSetNumber("verilatorVersion",GetVerilatorMajorVersion(&versat->temp));
      ProcessTemplate(output,comp,temp);
   }

   //RegisterModuleInfo(versat,&info);

   //UnitFunctions functions =

   //String wrapper = PushString(temp,"%s/wrapper.inc",opts->outputFilepath);
   //PushNullByte(temp);

   //FILE* output = OpenFileAndCreateDirectories(wrapper.data,"w");
   //CompiledTemplate* comp = CompileTemplate(unit_verilog_data,temp);

   //OutputModuleInfos(output,false,(Array<ModuleInfo>){allModules.data(),(int) allModules.size()},STRING("Verilog"),comp,temp);

   return 0;
}






