#include "versat.hpp"

#include "debug.hpp"
#include "templateEngine.hpp"
#include "unitVerilation.hpp"

static unsigned seed = 0;
static String fuzzed;

void HandleError(int sig){
   printf("Parse versat specification abruptly terminated\n");
   printf("Seed used:  %d\n",seed);
   printf("String used: %.*s\n",UNPACK_SS(fuzzed));
   fflush(stdout);
}

void FuzzVersatSpecification(Versat* versat){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   String toFuzz = STRING("module toFuzzTest ( x ) { # x -> out : 0 ; }");

   time_t t;
   seed = (unsigned) time(&t);
   #if 01
   seed = 1674883351;
   #endif // 0

   fuzzed = FuzzText(toFuzz,arena,seed);
   printf("%.*s\n",UNPACK_SS(fuzzed));

   SetDebugSignalHandler(HandleError);
   ParseVersatSpecification(versat,fuzzed);
}

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#if 0
void VersatSideVerilatorCall(){
   pid_t pid = fork();

   char* const verilatorArgs[] = {
      "verilator",
      "--cc",
      "-CFLAGS",
      "-m32 -g",
      "-I../../submodules/VERSAT/hardware/src",
      "-I../../submodules/VERSAT/hardware/include",
      "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/tdp_ram",
      "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/2p_ram",
      "-I../../submodules/VERSAT/submodules/MEM/hardware/ram/dp_ram",
      "-I../../submodules/VERSAT/submodules/MEM/hardware/fifo/sfifo",
      "-I../../submodules/VERSAT/submodules/MEM/hardware/fifo",
      "-I../../submodules/VERSAT/submodules/FPU/hardware/include",
      "-I../../submodules/VERSAT/submodules/FPU/hardware/src",
      "-I../../submodules/VERSAT/submodules/FPU/submodules/DIV/hardware/src",
      "-Isrc",
      "src/AESPathExample.v",
      nullptr
   };

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      int res = execvp("verilator",verilatorArgs);
      printf("Error calling execvp for verilator: %s\n",strerror(errno));
   } else {
      int status;
      pid_t res = wait(&status);
   }

   char* const makeArgs[] = {
      "make",
      "-C",
      "obj_dir",
      "-f",
      "VAESPathExample.mk",
      nullptr
   };

   pid = fork();

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      int res = execvp("make",makeArgs);
      printf("Error calling execvp for verilator: %s\n",strerror(errno));
   } else {
      int status;
      pid_t res = wait(&status);
   }
}
#endif

String FindProgramLocation(String name,Arena* out);

void TestVersatSide(Versat* versat){
   //CheckOrCompileUnit(STRING("AESPathExample"),&versat->temp);

   #if 0
   String res = FindProgramLocation(STRING("verilator"),&versat->temp);
   printf("%.*s\n",UNPACK_SS(res));
   #endif // 1

   #if 0
   FuzzVersatSpecification(versat);
   #endif

   #if 0
   VersatSideVerilatorCall();
   #endif
}
