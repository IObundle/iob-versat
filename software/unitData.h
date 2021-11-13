#undef START
#undef END
#undef ENTRY
#undef LAST_ENTRY

#ifdef INSTANTIATE_CLASS
#define START(structName,arrayName) typedef struct structName{
#define END(typeName) } typeName;
#define ENTRY(name,bitsize) int name;
#define LAST_ENTRY(name,bitsize) int name;
#endif

#ifdef INSTANTIATE_ARRAY
#define START(structName,arrayName) static Wire arrayName[] = {
#define END(structName) };
#define ENTRY(name,bitsize) {#name,bitsize},
#define LAST_ENTRY(name,bitsize) {#name,bitsize}
#endif

START(MemConfig_t,memConfigWires)
ENTRY(iterA,10)
ENTRY(perA,10)
ENTRY(dutyA,10)
ENTRY(startA,10)
ENTRY(shiftA,10)
ENTRY(incrA,10)
ENTRY(delayA,10)
ENTRY(reverseA,1)
ENTRY(extA,1)
ENTRY(in0_wr,1)
ENTRY(iter2A,10)
ENTRY(per2A,10)
ENTRY(shift2A,10)
ENTRY(incr2A,10)
ENTRY(iterB,10)
ENTRY(perB,10)
ENTRY(dutyB,10)
ENTRY(startB,10)
ENTRY(shiftB,10)
ENTRY(incrB,10)
ENTRY(delayB,10)
ENTRY(reverseB,1)
ENTRY(extB,1)
ENTRY(in1_wr,1)
ENTRY(iter2B,10)
ENTRY(per2B,10)
ENTRY(shift2B,10)
LAST_ENTRY(incr2B,10)
END(MemConfig)

START(RegConfig_t,regConfigWires)
ENTRY(initialValue,32)
LAST_ENTRY(writeDelay,10)
END(RegConfig)

START(RegState_t,regStateWires)
LAST_ENTRY(currentValue,32)
END(RegState)

START(AluliteConfig_t,aluliteConfigWires)
ENTRY(self_loop,1)
LAST_ENTRY(fns,3)
END(AluliteConfig)

START(MuladdConfig_t,muladdConfigWires)
ENTRY(opcode,1)
ENTRY(iterations,10)
ENTRY(period,10)
ENTRY(delay,10)
LAST_ENTRY(shift,5)
END(MuladdConfig)

START(VReadConfig_t,vreadConfigWires)
ENTRY(ext_addr,32)
ENTRY(int_addr,10)
ENTRY(size,11)
ENTRY(iterA,10)
ENTRY(perA,10)
ENTRY(dutyA,10)
ENTRY(shiftA,10)
ENTRY(incrA,10)
ENTRY(iterB,10)
ENTRY(perB,10)
ENTRY(dutyB,10)
ENTRY(startB,10)
ENTRY(shiftB,10)
ENTRY(incrB,10)
ENTRY(delayB,10)
ENTRY(reverseB,1)
ENTRY(extB,1)
ENTRY(iter2B,10)
ENTRY(per2B,10)
ENTRY(shift2B,10)
LAST_ENTRY(incr2B,10)
END(VReadConfig)

START(VWriteConfig_t,vwriteConfigWires)
ENTRY(ext_addr,32)
ENTRY(int_addr,10)
ENTRY(size,11)
ENTRY(iterA,10)
ENTRY(perA,10)
ENTRY(dutyA,10)
ENTRY(shiftA,10)
ENTRY(incrA,10)
ENTRY(iterB,10)
ENTRY(perB,10)
ENTRY(dutyB,10)
ENTRY(startB,10)
ENTRY(shiftB,10)
ENTRY(incrB,10)
ENTRY(delayB,10)
ENTRY(reverseB,1)
ENTRY(extB,1)
ENTRY(iter2B,10)
ENTRY(per2B,10)
ENTRY(shift2B,10)
LAST_ENTRY(incr2B,10)
END(VWriteConfig)




















