#ifndef INCLUDED_VERSAT_UNIT_VERILATION
#define INCLUDED_VERSAT_UNIT_VERILATION

#include "versatPrivate.hpp"

typedef FUDeclaration (*CreateDecl)();

struct UnitFunctions{
   VCDFunction vcd;
   FUFunction init;
   FUFunction start;
   FUUpdateFunction update;
   FUFunction destroy;
   CreateDecl createDeclaration;
   int size;
};

//UnitFunctions CompileUnit(String unitName,String soName,Arena* temp); Only allow the CheckOrCompile function, for now
UnitFunctions CheckOrCompileUnit(String unitName,Arena* temp);
int GetVerilatorMajorVersion(Arena* temp);

#endif // INCLUDED_VERSAT_UNIT_VERILATION
