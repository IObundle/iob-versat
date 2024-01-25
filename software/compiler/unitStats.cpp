#include "versat.hpp"

bool IsTypeHierarchical(FUDeclaration* decl){
   bool res = (decl->fixedDelayCircuit != nullptr);
   return res;
}
