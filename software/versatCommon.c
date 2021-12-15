#include "versat.h"

int UnitDelays(FUInstance* inst){
   if((inst->declaration->type & VERSAT_TYPE_IMPLEMENTS_DELAY)){
      if(inst->declaration->type & VERSAT_TYPE_SOURCE_DELAY){
         return mini(inst->declaration->nOutputs,2);
      } else {
         return 1;
      }
   } else {
      return 0;
   }
}

