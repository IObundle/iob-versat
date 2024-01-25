#pragma once

#include "utils.hpp"
#include "versat.hpp"

struct Versat;
struct Accelerator;

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
void OutputIterativeSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
void OutputVerilatorWrapper(Versat* versat,FUDeclaration* type,Accelerator* accel,String outputPath);
void OutputVerilatorMake(Versat* versat,String topLevelName,String versatDir,Options* opts);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,String accelName,bool isSimple);
