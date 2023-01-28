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

void TestVersatSide(Versat* versat){
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

   exit(0);
}
