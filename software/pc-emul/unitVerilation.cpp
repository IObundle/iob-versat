#include "unitVerilation.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <dlfcn.h>

#include "utils.hpp"
#include "verilogParsing.hpp"

struct FUDeclaration;
struct Versat;

typedef FUDeclaration* (*RegisterFunction)(Versat* versat);

void CallVerilator(const char* unitPath,const char* outputPath){
   pid_t pid = fork();

   CreateDirectories(outputPath);

   char* const verilatorArgs[] = {
      (char*) "verilator",
      (char*) "--cc",
      (char*) "-CFLAGS",
#if x86
      (char*) "-m32 -g -fPIC",
#else
      (char*) "-g -fPIC",
#endif
      (char*) "-I../../submodules/VERSAT/hardware/src",
      (char*) "-I../../../submodules/VERSAT/hardware/src",
      (char*) "-I../../../submodules/VERSAT/hardware/include",
      (char*) "-I../../hardware/src/units",
      (char*) "-I../../submodules/VERSAT/hardware/include",
      (char*) "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/tdp_ram",
      (char*) "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/2p_ram",
      (char*) "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/dp_ram",
      (char*) "-I../../submodules/VERSAT/submodules/MEM/hardware/fifo/sfifo",
      (char*) "-I../../submodules/VERSAT/submodules/MEM/hardware/fifo",
      (char*) "-I../../submodules/VERSAT/submodules/FPU/hardware/include",
      (char*) "-I../../submodules/VERSAT/submodules/FPU/hardware/src",
      (char*) "-I../../submodules/VERSAT/submodules/FPU/submodules/DIV/hardware/src",
      (char*) "-Isrc",
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

void CallMake(const char* path,const char* makefileName){
   char* const makeArgs[] = {
      (char*) "make",
      (char*) "-C",
      (char*) path,
      (char*) "-f",
      (char*) makefileName,
      nullptr
   };

   pid_t pid = fork();

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      execvp("make",makeArgs);
      printf("Error calling execvp for verilator: %s\n",strerror(errno));
   } else {
      int status;
      wait(&status);
   }
}

void CreateWrapper(String unitPath,String wrapperName,Arena* temp){
   BLOCK_REGION(temp);

   String unprocessed = PushFile(temp,unitPath.data);
   Arena output = SubArena(temp,Megabyte(64));
   String processed = PreprocessVerilogFile(&output,unprocessed,{},temp);

   std::vector<Module> modules = ParseVerilogFile(processed,{},temp);

   Assert(modules.size() == 1);

   Arena perm = SubArena(temp,Megabyte(1));

   ModuleInfo info = ExtractModuleInfo(modules[0],&perm,temp);

   FILE* wrapper = fopen(wrapperName.data,"w");
   OutputModuleInfos(wrapper,true,(Array<ModuleInfo>){&info,1},STRING("Top"),BasicTemplates::unitVerilogData,temp);
}

#include <wordexp.h>

void CompileWrapper(String objNames,String wrapperName,String soName,Arena* temp){
   BLOCK_REGION(temp);

   Array<char*> args = PushArray<char*>(temp,1024);

   int i = 0;
   args[i++] = (char*) "g++";
   args[i++] = (char*) "-shared";
   args[i++] = (char*) "-fPIC";
#ifdef x86
   args[i++] = (char*) "-m32";
#endif
   args[i++] = (char*) "-g";
   args[i++] = (char*) "-I./obj_dir";
   args[i++] = (char*) "-I/usr/local/share/verilator/include";
   args[i++] = (char*) "-I../../submodules/VERSAT/software";
   args[i++] = (char*) "-I../../submodules/VERSAT/software/pc-emul";
   args[i++] = (char*) "-I../../../submodules/VERSAT/software";
   args[i++] = (char*) "-I../../../submodules/VERSAT/software/pc-emul";
   args[i++] = (char*) "-o";
   args[i++] = (char*) soName.data;
   args[i++] = (char*) wrapperName.data;
#if 0
   args[i++] = (char*) "build/verilated.o";
   args[i++] = (char*) "build/verilated_vcd_c.o";
#endif
   args[i++] = (char*) "../build/verilated.o";
   args[i++] = (char*) "../build/verilated_vcd_c.o";

   wordexp_t exp;
   wordexp(objNames.data,&exp,0);

   Assert(exp.we_wordc > 0);

   for(int index = 0; index < (int) exp.we_wordc; index++){
      args[i++] = (char*) PushString(temp,"%s",exp.we_wordv[index]).data;
      PushNullByte(temp);
   }
   args[i] = nullptr;

   wordfree(&exp);

   pid_t pid = fork();

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      execvp("g++",args.data);
      printf("Error calling execvp for g++: %s\n",strerror(errno));
   } else {
      int status;
      wait(&status);
   }
}

typedef int (*SizeFunction)();

UnitFunctions ExtractUnitFunctions(void* lib,String unitName,Arena* temp){
   BLOCK_REGION(temp);

   String vcdFuncName = PushString(temp,"%.*s_VCDFunction",UNPACK_SS(unitName));
   PushNullByte(temp);
   String initFuncName = PushString(temp,"%.*s_InitializeFunction",UNPACK_SS(unitName));
   PushNullByte(temp);
   String startFuncName = PushString(temp,"%.*s_StartFunction",UNPACK_SS(unitName));
   PushNullByte(temp);
   String updateFuncName = PushString(temp,"%.*s_UpdateFunction",UNPACK_SS(unitName));
   PushNullByte(temp);
   String destroyFuncName = PushString(temp,"%.*s_DestroyFunction",UNPACK_SS(unitName));
   PushNullByte(temp);
   String extraDataSize = PushString(temp,"%.*s_ExtraDataSize",UNPACK_SS(unitName));
   PushNullByte(temp);
   String createDeclarationName = PushString(temp,"%.*s_CreateDeclaration",UNPACK_SS(unitName));
   PushNullByte(temp);

   #if 0
   String registerFuncName = PushString(temp,"%.*s_Register",UNPACK_SS(unitName));
   PushNullByte(temp);
   #endif

   printf("%s\n",GetCurrentDirectory());

   int error = errno;
   if(lib == nullptr){
      printf("%d\n",error);
      printf("Problem loading lib\n");
   }

   SizeFunction sizeFunc = (SizeFunction) dlsym(lib,extraDataSize.data);

   UnitFunctions res = {};
   res.vcd = (VCDFunction) dlsym(lib,vcdFuncName.data);
   res.init = (FUFunction) dlsym(lib,initFuncName.data);
   res.start = (FUFunction) dlsym(lib,startFuncName.data);
   res.update = (FUUpdateFunction) dlsym(lib,updateFuncName.data);
   res.destroy = (FUFunction) dlsym(lib,destroyFuncName.data);
   res.createDeclaration = (CreateDecl) dlsym(lib,createDeclarationName.data);
   res.size = sizeFunc();

   if(res.update == nullptr){
      printf("Problem loading func\n");
   }

   return res;
}

UnitFunctions CompileUnit(String unitName,String soName,Arena* temp){
   BLOCK_REGION(temp);

   String unitPath = PushString(temp,"src/%.*s.v",UNPACK_SS(unitName));
   PushNullByte(temp);

   String outputPath = PushString(temp,"customVerilated/%.*s",UNPACK_SS(unitName));
   PushNullByte(temp);

   String makefileName = PushString(temp,"V%.*s.mk",UNPACK_SS(unitName));
   PushNullByte(temp);

   String wrapperName = PushString(temp,"customVerilated/%.*s/%.*s_wrapper.cpp",UNPACK_SS(unitName),UNPACK_SS(unitName));
   PushNullByte(temp);

   String objNames = PushString(temp,"customVerilated/%.*s/*.o",UNPACK_SS(unitName));
   PushNullByte(temp);

   CallVerilator(unitPath.data,outputPath.data);
   CallMake(outputPath.data,makefileName.data);

   CreateWrapper(unitPath,wrapperName,temp);

   CompileWrapper(objNames,wrapperName,soName,temp);

   void* lib = dlopen(soName.data,RTLD_LAZY);

   UnitFunctions res = ExtractUnitFunctions(lib,unitName,temp);

   //dlclose(lib); Cannot close library otherwise functions will be unloaded and not work

   return res;
}

UnitFunctions CheckOrCompileUnit(String unitName,Arena* temp){
   BLOCK_REGION(temp);

   CreateDirectories("sharedObjects/");

   String soName = PushString(temp,"sharedObjects/%.*s.so",UNPACK_SS(unitName));
   PushNullByte(temp);

   void* lib = dlopen(soName.data,RTLD_LAZY);
   UnitFunctions res = {};
   if(lib){
      res = ExtractUnitFunctions(lib,unitName,temp);
   } else {
      res = CompileUnit(unitName,soName,temp);
   }

   return res;
}

























