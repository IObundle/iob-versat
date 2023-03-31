#ifndef INCLUDED_VERSAT_CONFIGURATIONS_HPP
#define INCLUDED_VERSAT_CONFIGURATIONS_HPP

#include "versatPrivate.hpp"

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out);
CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out);
CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out);

int GetConfigurationSize(FUDeclaration* decl,MemType type);

// These functions extract directly from the configuration pointer. No change is made, not even a check to see if unit contains any configuration at all
CalculatedOffsets ExtractConfig(Accelerator* accel,Arena* out); // Extracts offsets from a non FUDeclaration accelerator. Static configs are of the form >= N, where N is the size returned in CalculatedOffsets
CalculatedOffsets ExtractState(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractDelay(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractMem(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractOutputs(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractExtraData(Accelerator* accel,Arena* out);

#endif // INCLUDED_VERSAT_CONFIGURATIONS_HPP
