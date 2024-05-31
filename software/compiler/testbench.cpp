#include "versat.hpp"

#include "debugVersat.hpp"
#include "templateEngine.hpp"

static unsigned seed = 0;
static String fuzzed;

void HandleError(int sig){
   printf("Parse versat specification abruptly terminated\n");
   printf("Seed used:  %d\n",seed);
   printf("String used: %.*s\n",UNPACK_SS(fuzzed));
   fflush(stdout);
}

#if 0

void FuzzVersatSpecification(Versat* versat){
   Arena* temp = &versat->temp;
   BLOCK_REGION(temp);

   String toFuzz = STRING("module toFuzzTest ( x ) { # x -> out : 0 ; }");

   time_t t;
   seed = (unsigned) time(&t);

   #if 0
   seed = 1674883351;
   #endif // 0

   fuzzed = FuzzText(toFuzz,temp,seed);
   printf("%.*s\n",UNPACK_SS(fuzzed));

   SetDebugSignalHandler(HandleError);
   ParseVersatSpecification(versat,fuzzed);
}

void TestVersatSide(Versat* versat){
  // TODO: If comes a point where we beef up the parsing and error reporting of VersatSpec,
  //       Also implement a fuzzer. Use the versat spec given by the user.
  //       Change the parsing so that we can divide the parsing from the units registration.
  //       In such a way that we can fuzz and check that it does not crash, without registering bad units.
  
  FuzzVersatSpecification(versat);
}
#endif
