#include <new>

#include "versat_accel.h"
#include "versat.hpp"
#include "utils.hpp"
#include "acceleratorStats.hpp"

#include "verilated.h"

#{if arch.generateFSTFormat}
#include "verilated_fst_c.h"
static VerilatedFstC* tfp = NULL;
#{else}
#include "verilated_vcd_c.h"
static VerilatedVcdC* tfp = NULL;
#{end}

VerilatedContext* contextp = new VerilatedContext;

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

#include "V@{module.name}.h"
static V@{module.name}* dut = NULL;

extern bool CreateVCD;
extern bool SimulateDatabus;

#define INIT(unit) \
   unit->run = 0; \
   unit->clk = 0; \
   unit->rst = 0; \
   unit->running = 1;

/*
#define UPDATE(unit) \
   unit->clk = 0; \
   unit->eval(); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1); \
   unit->clk = 1; \
   unit->eval(); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1);
*/

#define UPDATE(unit) \
   unit->clk = 0; \
   unit->eval(); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1); \
   unit->clk = 1; \
   unit->eval();

#define RESET(unit) \
   unit->rst = 1; \
   UPDATE(unit); \
   unit->rst = 0; \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1);


#define START_RUN(unit) \
   unit->run = 1; \
   UPDATE(unit); \
   unit->run = 0; \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1); \
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(1);


#define PREAMBLE(type) \
   type* self = &data->unit; \
   VCDData* vcd = &data->vcd;

static char* Bin(unsigned int value){
   static char buffer[33];
   buffer[32] = '\0';

   unsigned int val = value;
   for(int i = 31; i >= 0; i--){
      buffer[i] = '0' + (val & 0x1);
      val >>= 1;
   }

   return buffer;
}

// static Array<int> zerosArray defined in versat.cpp

template<typename T>
static int32_t MemoryAccessNoAddress(FUInstance* inst,int address,int value,int write){
   T* self = (T*) inst->extraData;

   if(write){
      self->valid = 1;
      self->wstrb = 0xf;
      self->wdata = value;

      self->eval();

      while(!self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      self->valid = 0;
      self->wstrb = 0x00;
      self->wdata = 0x00000000;

      inst->declaration->updateFunction(inst,zerosArray);

      return 0;
   } else {
      self->valid = 1;
      self->wstrb = 0x0;

      self->eval();

      while(!self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      int32_t res = self->rdata;

      self->valid = 0;

      self->eval();
      while(self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      return res;
   }
}

template<typename T>
static int32_t MemoryAccess(FUInstance* inst,int address,int value,int write){
   T* self = (T*) inst->extraData;

   if(write){
      self->valid = 1;
      self->wstrb = 0xf;

      self->addr = address;
      self->wdata = value;

      self->eval();

      while(!self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      self->valid = 0;
      self->wstrb = 0x00;
      self->addr = 0x00000000;
      self->wdata = 0x00000000;

      inst->declaration->updateFunction(inst,zerosArray);

      return 0;
   } else {
      self->valid = 1;
      self->wstrb = 0x0;
      self->addr = address;

      self->eval();

      while(!self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      int32_t res = self->rdata;

      self->valid = 0;
      self->addr = 0;

      self->eval();
      while(self->ready){
         inst->declaration->updateFunction(inst,zerosArray);
      }

      return res;
   }
}

struct DatabusAccess{
   int counter;
   int latencyCounter;
};

static const int INITIAL_MEMORY_LATENCY = 5;
static const int MEMORY_LATENCY = 0;

union Int64{
   int64 val;
   struct{
       int32 i0;
       int32 i1;
   };
};

// Memories are evaluated after databus because most of the time
// databus is used to read data to memories
// while memory to databus usually passes through a register that holds the data.
// and therefore order of evaluation usually favours databus first and memories after

void @{module.name}_VCDFunction(FUInstance* inst,FILE* out,VCDMapping& currentMapping,Array<int>,bool firstTime,bool printDefinitions){
   if(printDefinitions){
   #{for external module.externalInterfaces}
      #{set id external.interface}
      #{if external.type}
      #{for dp external.dp}
      // DP
      fprintf(out,"$var wire @{dp.bitSize} %s ext_dp_addr_@{id}_port_@{index} $end\n",currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"$var wire @{dp.dataSizeOut} %s ext_dp_out_@{id}_port_@{index} $end\n",currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"$var wire @{dp.dataSizeIn} %s ext_dp_in_@{id}_port_@{index} $end\n",currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"$var wire 1 %s ext_dp_enable_@{id}_port_@{index} $end\n",currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"$var wire 1 %s ext_dp_write_@{id}_port_@{index} $end\n",currentMapping.Get());
      currentMapping.Increment();
      #{end}
      #{else}
      // 2P
      {
      }
      #{end}
   #{end}
   } else {
   #{for external module.externalInterfaces}
      #{set id external.interface}
      #{if external.type}
      #{for dp external.dp}
      // DP
      {
      V@{module.name}* self = (V@{module.name}*) inst->extraData;

      fprintf(out,"b%s %s\n",Bin(self->ext_dp_addr_@{id}_port_@{index}),currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"b%s %s\n",Bin(self->ext_dp_out_@{id}_port_@{index}),currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"b%s %s\n",Bin(self->ext_dp_in_@{id}_port_@{index}),currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"%d%s\n",self->ext_dp_enable_@{id}_port_@{index} ? 1 : 0,currentMapping.Get());
      currentMapping.Increment();
      fprintf(out,"%d%s\n",self->ext_dp_write_@{id}_port_@{index} ? 1 : 0,currentMapping.Get());
      currentMapping.Increment();
      }
      #{end}
      #{else}
      // 2P
      {
      }
      #{end}
   #{end}
   }
}

static void CloseWaveform(){
   if(CreateVCD && tfp){
      tfp->close();
   }
}

int32_t* @{module.name}_InitializeFunction(FUInstance* inst){
   if(CreateVCD){
   #{if arch.generateFSTFormat}
      tfp = new VerilatedFstC;
   #{else}
      tfp = new VerilatedVcdC;
   #{end}
   }

   memset(inst->extraData,0,inst->declaration->extraDataOffsets.max);

   V@{module.name}* self = new (inst->extraData) V@{module.name}();

   if(dut){
      printf("Initialize function is being called multiple times\n");
      exit(-1);
   }

   dut = self;

   if(CreateVCD){
      self->trace(tfp, 99);
      
      #{if arch.generateFSTFormat}
      tfp->open("system.fst");
      #{else}
      tfp->open("system.vcd");
      #{end}

      atexit(CloseWaveform);
   }

   INIT(self);

#{for i module.inputDelays.size}
   self->in@{i} = 0;
#{end}

   RESET(self);

   return NULL;
}

int32_t* @{module.name}_StartFunction(FUInstance* inst){
#{if module.outputLatencies}
   static int32_t out[@{module.outputLatencies.size}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

#{if module.configs}
AcceleratorConfig* config = (AcceleratorConfig*) inst->config;

#{for i module.configs.size}
#{set wire module.configs[i]}
   self->@{wire.name} = config->@{configsHeader[i].name};
#{end}
#{end}

#{if module.nDelays}
#{for i module.nDelays}
   self->delay@{i} = config->TOP_Delay@{i};
#{end}
#{end}

#{for i module.nIO}
{
   DatabusAccess* memoryLatency = ((DatabusAccess*) &self[1]) + @{i};

   memoryLatency->counter = 0;
   memoryLatency->latencyCounter = INITIAL_MEMORY_LATENCY;
}
#{end}

   START_RUN(self);

#{if module.hasDone}
   inst->done = self->done;
#{end}

#{if module.outputLatencies}
   #{for i module.outputLatencies.size}
   out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

int32_t* @{module.name}_UpdateFunction(FUInstance* inst,Array<int> inputs){
#{if module.outputLatencies}
   static int32_t out[@{module.outputLatencies.size}];
#{end}

   V@{module.name}* self = (V@{module.name}*) inst->extraData;

#{for i module.inputDelays.size}
   self->in@{i} = inputs[@{i}]; //GetInputValue(node,@{i});
#{end}

   self->eval();

#{for i module.nIO}
if(SimulateDatabus){
   self->databus_ready_@{i} = 0;
   self->databus_last_@{i} = 0;

   DatabusAccess* access = ((DatabusAccess*) &self[1]) + @{i};

   if(self->databus_valid_@{i}){
      if(access->latencyCounter > 0){
         access->latencyCounter -= 1;
      } else {
         #{if arch.dataSize == 64}
         int64* ptr = (int64*) (self->databus_addr_@{i});
         #{else}
         int* ptr = (int*) (self->databus_addr_@{i});
         #{end}

         if(self->databus_wstrb_@{i} == 0){
            if(ptr == nullptr){
               self->databus_rdata_@{i} = 0xfeeffeef; // Feed bad data if not set (in pc-emul is needed otherwise segfault)
            } else {
               self->databus_rdata_@{i} = ptr[access->counter];
            }
         } else { // self->databus_wstrb_@{i} != 0
            if(ptr != nullptr){
               ptr[access->counter] = self->databus_wdata_@{i};
            }
         }
         self->databus_ready_@{i} = 1;

       int transferLength = self->databus_len_@{i};

       #{if arch.dataSize == 64}
       //TODO: Need to be able to generate wires depending on parameter specifications
       //       And probably to make it explicit the difference between a parameterizable and a type instance
       #define COUNTER_LENGTH(VAL) ALIGN_8(VAL) / 8
       #{else}
       #define COUNTER_LENGTH(VAL) ALIGN_4(VAL) / 4
       #{end}

       int countersLength = COUNTER_LENGTH(transferLength);

       #undef COUNTER_LENGTH

         if(access->counter >= countersLength - 1){
            access->counter = 0;
            self->databus_last_@{i} = 1;
         } else {
            access->counter += 1;
         }

         access->latencyCounter = MEMORY_LATENCY;
      }
   }

   self->eval();
}
#{end}

#{for external module.externalInterfaces}
   #{set id external.interface}
   #{if external.type}
   // DP
   #{for dp external.dp}
   #{set dataType "int"}
   #{if dp.dataSizeIn == 64}
   #{set dataType "int64"}
   #{end}

   int saved_dp_enable_@{id}_port_@{index} = self->ext_dp_enable_@{id}_port_@{index};
   int saved_dp_write_@{id}_port_@{index} = self->ext_dp_write_@{id}_port_@{index};
   int saved_dp_addr_@{id}_port_@{index} = self->ext_dp_addr_@{id}_port_@{index};
   @{dataType} saved_dp_data_@{id}_port_@{index} = self->ext_dp_out_@{id}_port_@{index};

   #{end}
   #{else}
   // 2P
   #{set dataType "int"}
   #{if external.tp.dataSizeOut == 64}
   #{set dataType "int64"}
   #{end}

   int saved_2p_r_enable_@{id} = self->ext_2p_read_@{id};
   int saved_2p_r_addr_@{id} = self->ext_2p_addr_in_@{id}; // Instead of saving address, should access memory and save data. Would simulate better what is actually happening
   int saved_2p_w_enable_@{id} = self->ext_2p_write_@{id};
   int saved_2p_w_addr_@{id} = self->ext_2p_addr_out_@{id};
   @{dataType} saved_2p_w_data_@{id} = self->ext_2p_data_out_@{id};

   #{end}
   self->eval();
#{end}

   UPDATE(self); // This line causes posedge clk events to activate

//TODO: There might be a bug with the fact that we do not align the base address.
//      I do not remember how external memory is allocated and distributed, check later and confirm this part
//      At a minimum, we should align to make sure that
{
   int baseAddress = 0;
#{for external module.externalInterfaces}
   #{set id external.interface}
   #{if external.type}
   // DP
   {
   int memSize = ExternalMemoryByteSize(&inst->declaration->externalMemory[@{id}]);

   #{for dp external.dp}
      if(saved_dp_enable_@{id}_port_@{index} && !saved_dp_write_@{id}_port_@{index}){
         int readOffset = saved_dp_addr_@{id}_port_@{index};

       #{if dp.dataSizeIn == 64}
       int address = baseAddress + readOffset * 8;
       #{else}
       int address = baseAddress + readOffset * 4;
       #{end}

       Assert(address < memSize);

       #{if dp.dataSizeIn == 64}
         int64* ptr = (int64*) &inst->externalMemory[address];
       #{else}
         int* ptr = (int*) &inst->externalMemory[address];
       #{end}

       self->ext_dp_in_@{id}_port_@{index} = *ptr;
      }
   #{end}
   baseAddress += memSize;
   }
   #{else}
   {
     int memSize = ExternalMemoryByteSize(&inst->declaration->externalMemory[@{id}]);
   // 2P
      if(saved_2p_r_enable_@{id}){
         int readOffset = saved_2p_r_addr_@{id};

       #{if external.tp.dataSizeIn == 64}
       int address = baseAddress + readOffset * 8;
       #{else}
       int address = baseAddress + readOffset * 4;
       #{end}

       Assert(address < memSize);

       //printf("R %d %d %d\n",memSize,address,baseAddress + memSize);

       #{if external.tp.dataSizeIn == 64}
         int64* ptr = (int64*) &inst->externalMemory[address];
       #{else}
         int* ptr = (int*) &inst->externalMemory[address];
       #{end}

       #{if external.tp.dataSizeOut == 32}
             //printf("R %d %lx\n",address,*ptr);
       #{end}

       self->ext_2p_data_in_@{id} = *ptr;
      }
     baseAddress += memSize;
     }
     #{end}
   self->eval();
#{end}
}

{
   int baseAddress = 0;
#{for external module.externalInterfaces}
   #{set id external.interface}
   #{if external.type}
   {
      // DP
        int memSize = ExternalMemoryByteSize(&inst->declaration->externalMemory[@{id}]);
        #{for dp external.dp}
      if(saved_dp_enable_@{id}_port_@{index} && saved_dp_write_@{id}_port_@{index}){
         int writeOffset = saved_dp_addr_@{id}_port_@{index};

       #{if dp.dataSizeIn == 64}
       int address = baseAddress + writeOffset * 8;
       #{else}
       int address = baseAddress + writeOffset * 4;
       #{end}

       Assert(address < memSize);

       #{if dp.dataSizeIn == 64}
         int64* ptr = (int64*) &inst->externalMemory[address];
       #{else}
         int* ptr = (int*) &inst->externalMemory[address];
       #{end}

       *ptr = saved_dp_data_@{id}_port_@{index};
   }
   #{end}
   baseAddress += memSize;
   }
   #{else}
     {
     int memSize = ExternalMemoryByteSize(&inst->declaration->externalMemory[@{id}]);
     // 2P
     if(saved_2p_w_enable_@{id}){
         int writeOffset = saved_2p_w_addr_@{id};

       #{if external.tp.dataSizeOut == 64}
       int address = baseAddress + writeOffset * 8;
       #{else}
       int address = baseAddress + writeOffset * 4;
       #{end}

       Assert(address < memSize);

       #{if external.tp.dataSizeOut == 64}
         int64* ptr = (int64*) &inst->externalMemory[address];
       #{else}
         int* ptr = (int*) &inst->externalMemory[address];
       #{end}

       #{if external.tp.dataSizeOut == 32}
             //printf("W %d %lx\n",address,saved_2p_w_data_@{id});
       #{end}

       *ptr = saved_2p_w_data_@{id};
      }
     baseAddress += memSize;
     }
   #{end}
   self->eval();
#{end}
}

   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);
   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);

#{if module.states}
AcceleratorState* state = (AcceleratorState*) inst->state;
#{for i module.states.size}
#{set wire module.states[i]}
   state->@{statesHeader[i]} = self->@{wire.name};
#{end}
#{end}

#{if module.hasDone}
   inst->done = self->done;
#{end}

#{if module.outputLatencies}
   #{for i module.outputLatencies.size}
      out[@{i}] = self->out@{i};
   #{end}

   return out;
#{else}
   return NULL;
#{end}
}

int32_t* @{module.name}_DestroyFunction(FUInstance* inst){
   V@{module.name}* self = (V@{module.name}*) inst->extraData;

   self->~V@{module.name}();

   return nullptr;
}

int @{module.name}_ExtraDataSize(){
   int extraSize = sizeof(V@{module.name});

   #{if module.nIO}
   extraSize += sizeof(DatabusAccess) * @{module.nIO};
   #{end}

   return extraSize;
}

#{if module.signalLoop}
void SignalFunction@{module.name}(FUInstance* inst){
   V@{module.name}* self = (V@{module.name}*) inst->extraData;

   self->signal_loop = 1;

   UPDATE(self);

   self->signal_loop = 0;
   self->eval();

   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);
   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);
}
#{end}

FUDeclaration @{module.name}_CreateDeclaration(){
   FUDeclaration decl = {};

   #{if module.inputDelays}
   static int inputDelays[] =  {#{join "," for delay module.inputDelays}@{delay}#{end}};
   decl.inputDelays = Array<int>{inputDelays,@{module.inputDelays.size}};
   #{end}

   #{if module.outputLatencies}
   static int outputLatencies[] = {#{join "," for latency module.outputLatencies}@{latency}#{end}};
   decl.outputLatencies = Array<int>{outputLatencies,@{module.outputLatencies.size}};
   #{end}

   decl.name = STRING("@{module.name}");

   decl.extraDataOffsets.max = @{module.name}_ExtraDataSize();

   #{if module.externalInterfaces.size}
   static ExternalMemoryInterface externalMemory[@{module.externalInterfaces.size}];

   #{for ext module.externalInterfaces}
   #{set loopIndex index}
   externalMemory[@{loopIndex}].interface = @{ext.interface};
      #{if ext.type}
   externalMemory[@{loopIndex}].type = ExternalMemoryType::DP;
   // DP
           #{for dp ext.dp}
   externalMemory[@{loopIndex}].dp[@{index}].bitSize = @{dp.bitSize};
   externalMemory[@{loopIndex}].dp[@{index}].dataSizeIn =  @{dp.dataSizeOut};
   externalMemory[@{loopIndex}].dp[@{index}].dataSizeOut =  @{dp.dataSizeOut};
         #{end}
      #{else}
   externalMemory[@{loopIndex}].type = ExternalMemoryType::TWO_P;
   externalMemory[@{loopIndex}].tp.bitSizeIn = @{ext.tp.bitSizeIn};
   externalMemory[@{loopIndex}].tp.bitSizeOut = @{ext.tp.bitSizeOut};
   externalMemory[@{loopIndex}].tp.dataSizeIn =  @{ext.tp.dataSizeOut};
   externalMemory[@{loopIndex}].tp.dataSizeOut =  @{ext.tp.dataSizeOut};
      #{end}
   decl.externalMemory = C_ARRAY_TO_ARRAY(externalMemory);
   #{end}
   #{end}

   decl.initializeFunction = @{module.name}_InitializeFunction;
   decl.startFunction = @{module.name}_StartFunction;
   decl.updateFunction = @{module.name}_UpdateFunction;
   decl.destroyFunction = @{module.name}_DestroyFunction;
   decl.printVCD = @{module.name}_VCDFunction;

   #{if module.configs}
   static Wire @{module.name}ConfigWires[] = {#{join "," for wire module.configs} #{if !wire.isStatic} {STRING("@{wire.name}"),@{wire.bitSize},@{wire.isStatic}} #{end} #{end}};
   decl.configs = Array<Wire>{@{module.name}ConfigWires,@{module.configs.size}};
   #{end}

   #{if module.states}
   static Wire @{module.name}StateWires[] = {#{join "," for wire module.states} {STRING("@{wire.name}"),@{wire.bitSize}} #{end}};
   decl.states = Array<Wire>{@{module.name}StateWires,@{module.states.size}};
   #{end}

   #{if module.hasDone}
   decl.implementsDone = true;
   #{end}

   #{if module.isSource}
   decl.delayType = decl.delayType | DelayType::DELAY_TYPE_SINK_DELAY;
   #{end}

   #{if module.nIO}
   decl.nIOs = @{module.nIO};
   #{end}

   #{if module.signalLoop}
   decl.signalFunction = SignalFunction@{module.name};
   #{end}

   #{if module.memoryMapped}
   decl.isMemoryMapped = true;
   decl.memoryMapBits = @{module.memoryMappedBits};
   #{if module.memoryMappedBits == 0}
   decl.memAccessFunction = MemoryAccessNoAddress<V@{module.name}>;
   #{else}
   decl.memAccessFunction = MemoryAccess<V@{module.name}>;
   #{end}
   #{end}

   decl.delayOffsets.max = @{module.nDelays};

   return decl;
}

FUDeclaration* @{module.name}_Register(Versat* versat){
   FUDeclaration decl = @{module.name}_CreateDeclaration();

   return RegisterFU(versat,decl);
}

extern "C" void RegisterAllVerilogUnits@{namespace}(Versat* versat){
   @{module.name}_Register(versat);
}

extern "C" void InitializeVerilator(){
   Verilated::traceEverOn(true);
}

#undef INIT
#undef UPDATE
#undef RESET
#undef START_RUN
#undef PREAMBLE
