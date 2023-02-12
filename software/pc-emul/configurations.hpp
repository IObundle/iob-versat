#ifndef INCLUDED_VERSAT_CONFIGURATIONS_HPP
#define INCLUDED_VERSAT_CONFIGURATIONS_HPP

#include "versatPrivate.hpp"

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out);
CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out);
CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out);

int GetConfigurationSize(FUDeclaration* decl,MemType type);

#endif // INCLUDED_VERSAT_CONFIGURATIONS_HPP
