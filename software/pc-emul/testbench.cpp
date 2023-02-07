#include "versatPrivate.hpp"

#include "debug.hpp"
#include "templateEngine.hpp"

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

void VersatSideVerilatorCall(){
   pid_t pid = fork();

   if(pid < 0){
      printf("Error calling fork\n");
   } else if(pid == 0){
      int res = execlp("verilator",(char*) NULL);
      printf("Error calling execlp: %s\n",strerror(errno));
   } else {
      int status;
      pid_t res = wait(&status);
   }
}

void TestVersatSide(Versat* versat){
   #if 0
   FuzzVersatSpecification(versat);
   #endif // 0

   #if 0
   VersatSideVerilatorCall();
   #endif // 0
}
