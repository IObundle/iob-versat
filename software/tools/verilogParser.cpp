#include "verilogParsing.hpp"

#include "type.hpp"
#include "templateEngine.hpp"

#include "templateData.hpp"

int main(int argc,const char* argv[]){
   if(argc < 4){
      printf("Error, need at least 4 arguments: <program> <namespace> <outputFile> <inputFile1> ...");

      return 0;
   }

   RegisterTypes();

   std::vector<const char*> includePaths;
   std::vector<const char*> filePaths;
   // Parse include paths
   for(int i = 3; i < argc; i++){
      if(CompareString("-I",argv[i])){
         includePaths.push_back(argv[i+1]);
         i += 1;
      } else {
         filePaths.push_back(argv[i]);
      }
   }

   Arena tempArenaInst = InitArena(Megabyte(256));
   Arena* tempArena = &tempArenaInst;

   Arena preprocessInst = InitArena(Megabyte(256));
   Arena* preprocess = &preprocessInst;

   Arena permanentInst = InitArena(Megabyte(256));
   Arena* permanent = &permanentInst;

   std::vector<ModuleInfo> allModules;
   for(const char* str : filePaths){
      Byte* mark = MarkArena(tempArena);

      String content = PushFile(tempArena,str);

      if(content.size <= 0){
         printf("Failed to open file: %s\n",str);
         return 0;
      }

      String processed = PreprocessVerilogFile(preprocess,content,&includePaths,tempArena);
      std::vector<Module> modules = ParseVerilogFile(processed,&includePaths,tempArena);

      for(Module& module : modules){
         ModuleInfo info = ExtractModuleInfo(module,permanent,tempArena);
         allModules.push_back(info);
      }
   }

   FILE* output = OpenFileAndCreateDirectories(argv[2],"w");
   CompiledTemplate* comp = CompileTemplate(versat_wrapper_template,tempArena);

   OutputModuleInfos(output,false,(Array<ModuleInfo>){allModules.data(),(int) allModules.size()},STRING(argv[1]),comp,tempArena,(Array<Wire>){},(Array<String>){});

   return 0;
}








