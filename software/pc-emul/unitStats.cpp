#include "versatPrivate.hpp"

bool IsTypeHierarchical(FUDeclaration* decl){
   bool res = (decl->fixedDelayCircuit != nullptr);
   return res;
}

bool IsTypeSimulatable(FUDeclaration* decl){
   bool res = (decl->updateFunction != nullptr);
   return res;
}
