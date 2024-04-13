#pragma once

#include "utils.hpp"

struct Versat;
struct Accelerator;
struct FUDeclaration;
struct Options;

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2);
void OutputIterativeSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2);
void OutputVerilatorWrapper(Versat* versat,FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2);
void OutputVerilatorMake(Versat* versat,String topLevelName,String versatDir,Options* opts,Arena* temp,Arena* temp2);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2);
