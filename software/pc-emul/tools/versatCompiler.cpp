#include "versatPrivate.hpp"



int main(int argc,const char* argv){
   if(argc < 3){
      printf("Need specifications and a top level type\n");
      return -1;
   }

   //String versatBaseStr = STRING(argv[1]);
   const char* specFilepath = argv[1];
   String topLevelTypeStr = STRING(argv[2]);

   Versat* versat = InitVersat(0);

   ParseVersatSpecification(versat,specFilepath);



}
