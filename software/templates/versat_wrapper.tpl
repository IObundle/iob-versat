// File generated by template engine for the versat_wrapper.tpl template
// Do not make changes, versat will overwrite

#{include "versat_common.tpl"}
#include <new>

#include <cassert>
#define Assert(x) assert(x)

#include "versat_accel.h" // TODO: Is this needed? We technically have all the data that we need to not depend on this header and removing this dependency could simplify the build process. Take a look later

//#include "versat.hpp"
//#include "utils.hpp"
//#include "acceleratorStats.hpp"

#define ALIGN_UP(val,size) (((val) + (size - 1)) & ~(size - 1))
#define ALIGN_DOWN(val,size) (val & (~(size - 1)))

#include "verilated.h"

#{if trace}       
#{if opts.generateFSTFormat}
#include "verilated_fst_c.h"
static VerilatedFstC* tfp = NULL;
#{else}
#include "verilated_vcd_c.h"
static VerilatedVcdC* tfp = NULL;
#{end}

VerilatedContext* contextp = new VerilatedContext;
#{end}

#include "V@{type.name}.h"
static V@{type.name}* dut = NULL;

extern bool CreateVCD;
extern bool SimulateDatabus;

// TODO: Maybe this should not exist. There should exist one update function and every other function that needs to update stuff should just call that function. UPDATE as it stands should only be called inside the update function. Instead of having macros that call UPDATE, we should just call the update function directly inside the RESET macro and the START_RUN macro and so one. Everything depends on the update function and the UPDATE macro is removed and replaced inside the function. The only problem is that I do not know if the RESET macro can call the update function without something. Nonetheless, every "update" should go trough the update function and none should go through this macro.
#define UPDATE(unit) \
   unit->clk = 0; \
   unit->eval(); \
#{if trace}
   if(CreateVCD) tfp->dump(contextp->time()); \
   contextp->timeInc(2); \
#{end}
   unit->clk = 1; \
   unit->eval();

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

struct DatabusAccess{
   int counter;
   int latencyCounter;
};

static const int INITIAL_MEMORY_LATENCY = 5;
static const int MEMORY_LATENCY = 0;

typedef char Byte;

// Everything is statically allocated
static constexpr int totalExternalMemory = @{totalExternalMemory};
static Byte externalMemory[totalExternalMemory]; 
static AcceleratorConfig configBuffer = {};
static AcceleratorState stateBuffer = {};
static AcceleratorStatic staticBuffer = {};
static DatabusAccess databusBuffer[@{type.nIOs}] = {}; 

extern "C" void InitializeVerilator(){
#{if trace}       
  Verilated::traceEverOn(true);
#{end}
}

extern "C" AcceleratorConfig* GetStartOfConfig(){
  return &configBuffer;
}

extern "C" AcceleratorState* GetStartOfState(){
  return &stateBuffer;
}

extern "C" AcceleratorStatic* GetStartOfStatic(){
  return &staticBuffer;
}

static void CloseWaveform(){
#{if trace}
   if(CreateVCD && tfp){
      tfp->close();
   }
#{end}
}

extern "C" void VersatAcceleratorCreate(){
#{if trace}
   if(CreateVCD){
   #{if opts.generateFSTFormat}
      tfp = new VerilatedFstC;
   #{else}
      tfp = new VerilatedVcdC;
   #{end}
   }
#{end}

   V@{type.name}* self = new V@{type.name}();

   if(dut){
      printf("Initialize function is being called multiple times\n");
      exit(-1);
   }

   dut = self;

#{if trace}       
   if(CreateVCD){
      self->trace(tfp, 99);
      
      #{if opts.generateFSTFormat}
      tfp->open("system.fst");
      #{else}
      tfp->open("system.vcd");
      #{end}

      atexit(CloseWaveform);
}
#{end}

   self->run = 0;
   self->clk = 0;
   self->rst = 0;
   self->running = 0;

#{for i type.baseConfig.inputDelays.size}
   self->in@{i} = 0;
#{end}

   self->rst = 1;
   UPDATE(self);
   self->rst = 0;
#{if trace}
   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);
   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(1);
#{end}
}

static int cyclesDone = 0;

static void InternalUpdateAccelerator(){
   int baseAddress = 0;

   cyclesDone += 1;

   V@{type.name}* self = dut;

   // Databus must be updated before memories because databus could drive memories but memories "cannot" drive databus (in the sense that databus acts like a master if connected directly to memories but memories do not act like a master when connected to a databus. The unit logic is the one that acts like a master)
#{for i type.nIOs}
if(SimulateDatabus){
   self->databus_ready_@{i} = 0;
   self->databus_last_@{i} = 0;

   DatabusAccess* access = &databusBuffer[@{i}];

   if(self->databus_valid_@{i}){
      if(access->latencyCounter > 0){
         access->latencyCounter -= 1;
      } else {
         #{set dataType #{call IntName opts.dataSize}}
         @{dataType}* ptr = (@{dataType}*) (self->databus_addr_@{i});

         if(self->databus_wstrb_@{i} == 0){
            if(ptr == nullptr){
            #{if opts.dataSize > 64}
            for(int i = 0; i < (@{opts.dataSize} / sizeof(int)); i++){
               self->databus_rdata_@{i}[i] = 0xfeeffeef;
            }
            #{else}
               self->databus_rdata_@{i} = 0xfeeffeef; // Feed bad data if not set (in pc-emul is needed otherwise segfault)
            #{end}
            } else {
            #{if opts.dataSize > 64}
               for(int i = 0; i < (@{opts.dataSize} / sizeof(int)); i++){
                   self->databus_rdata_@{i}[i] = ptr[access->counter].i[i];
               }
            #{else}
               self->databus_rdata_@{i} = ptr[access->counter];
            #{end}
            }
         } else { // self->databus_wstrb_@{i} != 0
            if(ptr != nullptr){
            #{if opts.dataSize > 64}
               for(int i = 0; i < (@{opts.dataSize} / sizeof(int)); i++){
                  ptr[access->counter].i[i] = self->databus_wdata_@{i}[i];
               }
            #{else}
               ptr[access->counter] = self->databus_wdata_@{i};
            #{end}
            }
         }
         self->databus_ready_@{i} = 1;

       int transferLength = self->databus_len_@{i};
       int sizeOfData = sizeof(@{dataType});
       int countersLength = ALIGN_UP(transferLength,sizeOfData) / sizeOfData;

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
   
   baseAddress = 0;
#{for external type.externalMemory}
   #{set id external.interface}
   #{if external.type}
   // DP
   #{for dp external.dp}
   #{set dataType #{call IntName dp.dataSizeIn}}

   int saved_dp_enable_@{id}_port_@{index} = self->ext_dp_enable_@{id}_port_@{index};
   int saved_dp_write_@{id}_port_@{index} = self->ext_dp_write_@{id}_port_@{index};
   int saved_dp_addr_@{id}_port_@{index} = self->ext_dp_addr_@{id}_port_@{index};
   @{dataType} saved_dp_data_@{id}_port_@{index};
   memcpy(&saved_dp_data_@{id}_port_@{index},&self->ext_dp_out_@{id}_port_@{index},sizeof(@{dataType}));
   #{end}

   {
     int memSize = @{external |> MemorySize}; // Template add something about id but do not know if needed
     baseAddress += memSize;
   }
   #{else}
   // 2P
   #{set dataType #{call IntName external.tp.dataSizeOut}}

   @{dataType} saved_2p_r_data_@{id};
   {
     int memSize = @{external |> MemorySize};
   // 2P
      if(self->ext_2p_read_@{id}){
         int readOffset = self->ext_2p_addr_in_@{id};

         Assert(readOffset < memSize);
         int address = baseAddress + readOffset; // * sizeof(@{dataType});
         
         address = ALIGN_DOWN(address,sizeof(@{dataType}));

         Assert(address + sizeof(@{dataType}) <= totalExternalMemory);

         @{dataType}* ptr = (@{dataType}*) &externalMemory[address];
         memcpy(&saved_2p_r_data_@{id},ptr,sizeof(@{dataType}));
      }
     baseAddress += memSize;
   }

#if 1
   int saved_2p_r_enable_@{id} = self->ext_2p_read_@{id};
   int saved_2p_r_addr_@{id} = self->ext_2p_addr_in_@{id}; // Instead of saving address, should access memory and save data. Would simulate better what is actually happening
#endif
   
   int saved_2p_w_enable_@{id} = self->ext_2p_write_@{id};
   int saved_2p_w_addr_@{id} = self->ext_2p_addr_out_@{id};
   @{dataType} saved_2p_w_data_@{id};
   memcpy(&saved_2p_w_data_@{id},&self->ext_2p_data_out_@{id},sizeof(@{dataType}));

   #{end}
   self->eval();
#{end}

   UPDATE(self); // This line causes posedge clk events to activate
   
   // Memory Read
{
   baseAddress = 0;
#{for external type.externalMemory}
   #{set id external.interface}
   #{if external.type}
   // DP
   {

   int memSize = @{external |> MemorySize};

   #{for dp external.dp}
   #{set dataType #{call IntName dp.dataSizeIn}}
      if(saved_dp_enable_@{id}_port_@{index} && !saved_dp_write_@{id}_port_@{index}){
       int readOffset = saved_dp_addr_@{id}_port_@{index}; // Byte space

       Assert(readOffset < memSize);
       int address = baseAddress + readOffset;

       address = ALIGN_DOWN(address,sizeof(@{dataType}));
       Assert(address + sizeof(@{dataType}) <= totalExternalMemory);

       @{dataType}* ptr = (@{dataType}*) &externalMemory[address];
       memcpy(&self->ext_dp_in_@{id}_port_@{index},ptr,sizeof(@{dataType}));
      }
   #{end}
   baseAddress += memSize;
   }
   #{else}
   #{set dataType #{call IntName external.tp.dataSizeIn}}
   {
     int memSize = @{external |> MemorySize};
   // 2P
      if(saved_2p_r_enable_@{id}){
       int readOffset = saved_2p_r_addr_@{id};

       Assert(readOffset < memSize);
       int address = baseAddress + readOffset; // * sizeof(@{dataType});

       address = ALIGN_DOWN(address,sizeof(@{dataType}));
       Assert(address + sizeof(@{dataType}) <= totalExternalMemory);

       @{dataType}* ptr = (@{dataType}*) &externalMemory[address];
       memcpy(&self->ext_2p_data_in_@{id},ptr,sizeof(@{dataType}));
      }
     baseAddress += memSize;
     }
     #{end}
   self->eval();
#{end}
}

// Memory write
{
   baseAddress = 0;
#{for external type.externalMemory}
   #{set id external.interface}
   #{if external.type}
   {
      // DP
      int memSize = @{external |> MemorySize};
   #{for dp external.dp}
   #{set dataType #{call IntName dp.dataSizeIn}}
      if(saved_dp_enable_@{id}_port_@{index} && saved_dp_write_@{id}_port_@{index}){
       int writeOffset = saved_dp_addr_@{id}_port_@{index};

       Assert(writeOffset < memSize);
       int address = baseAddress + writeOffset; // * sizeof(@{dataType});

       address = ALIGN_DOWN(address,sizeof(@{dataType}));
       Assert(address + sizeof(@{dataType}) <= totalExternalMemory);

       @{dataType}* ptr = (@{dataType}*) &externalMemory[address];
       memcpy(ptr,&saved_dp_data_@{id}_port_@{index},sizeof(@{dataType}));
   }
   #{end}
   baseAddress += memSize;
   }
   #{else}
   #{set dataType #{call IntName external.tp.dataSizeOut}}
     {
     int memSize = @{external |> MemorySize};
     // 2P
     if(saved_2p_w_enable_@{id}){
       int writeOffset = saved_2p_w_addr_@{id};

       Assert(writeOffset < memSize);
       int address = baseAddress + writeOffset; // * sizeof(@{dataType});

       address = ALIGN_DOWN(address,sizeof(@{dataType}));
       Assert(address + sizeof(@{dataType}) <= totalExternalMemory);
       
       @{dataType}* ptr = (@{dataType}*) &externalMemory[address];
       memcpy(ptr,&saved_2p_w_data_@{id},sizeof(@{dataType}));
     }
     baseAddress += memSize;
     }
   #{end}
   self->eval();
#{end}
}

#{if trace}
   if(CreateVCD) tfp->dump(contextp->time());
   contextp->timeInc(2);
#{end}

// TODO: Technically only need to do this at the end of an accelerator run, do not need to do this every single update
#{if type.baseConfig.states}
AcceleratorState* state = &stateBuffer;
#{for i type.baseConfig.states.size}
#{set wire type.baseConfig.states[i]}
   state->@{statesHeader[i]} = self->@{wire.name};
#{end}
#{end}
}

static bool IsDone(){
  V@{type.name}* self = dut;

#{if type.implementsDone}
bool done = self->done;
#{else}
bool done = true;
#{end}
return done;
}

struct Once{};
struct _OnceTag{};
template<typename F>
Once operator+(_OnceTag t,F&& f){
  f();
  return Once{};
}

#define TEMP__once(LINE) TEMPonce_ ## LINE
#define TEMP_once(LINE) TEMP__once( LINE )
#define once static Once TEMP_once(__LINE__) = _OnceTag() + [&]()

static void InternalStartAccelerator(){
   V@{type.name}* self = dut;

#{if structuredConfigs.size}
AcceleratorConfig* config = (AcceleratorConfig*) &configBuffer;
AcceleratorStatic* statics = (AcceleratorStatic*) &staticBuffer;

#{for wire allConfigsVerilatorSide}
 #{if configsHeader[index].bitSize != 64}

#if 0
  once{
     if((long long int) config->TOP_@{wire.name} >= ((long long int) 1 << @{configsHeader[index].bitSize})){
      printf("[Once] Warning, configuration value contains more bits\n");
      printf("set than the hardware unit is capable of handling\n");
      printf("Name: %s, BitSize: %d, Value: 0x%lx\n","@{configsHeader[index].name}",@{configsHeader[index].bitSize},config->@{configsHeader[index].name});
    }
  };
#endif
#{end}
  self->@{wire.name} = config->TOP_@{wire.name};
#{end}
#{end}

#{for wire allStaticsVerilatorSide}
  self->@{wire.name} = statics->@{wire.name};
#{end}

#{if type.baseConfig.delayOffsets.max}
#{for i type.baseConfig.delayOffsets.max}
  //self->delay@{i} = accelDelay.TOP_Delay@{i};
#{end}
#{end}

#{for i type.nIOs}
{
   databusBuffer[@{i}].latencyCounter = INITIAL_MEMORY_LATENCY;
}
#{end}

  self->run = 1;
  UPDATE(self);
  self->running = 1;
  self->run = 0;
#{if trace}
  if(CreateVCD) tfp->dump(contextp->time());
  contextp->timeInc(1);
  if(CreateVCD) tfp->dump(contextp->time());
  contextp->timeInc(1);
#{end}
}

static void InternalEndAccelerator(){
  V@{type.name}* self = dut;

  self->running = 0;

  // TODO: Is this update call needed?
  InternalUpdateAccelerator();

  // TODO: We could put the copy of state variables here
}

extern "C" int VersatAcceleratorCyclesElapsed(){
  return cyclesDone;
}

extern "C" void VersatAcceleratorSimulate(){
  InternalStartAccelerator();

  for(int i = 0; !IsDone() ; i++){
    InternalUpdateAccelerator();

    if(i >= 10000){
      printf("Accelerator simulation has surpassed 10000 cycles for a single run\n");
      printf("Assuming that the accelerator is stuck in a never ending loop\n");
      printf("Terminating simulation\n");
      fflush(stdout);
      exit(-1);
    }   
  }

  InternalEndAccelerator();
}

extern "C" int MemoryAccess(int address,int value,int write){
  #{if type.memoryMapBits}
   
  V@{type.name}* self = dut;

  if(write){
    self->valid = 1;
    self->wstrb = 0xf;

    #{if type.memoryMapBits != 0}
      self->addr = address;
    #{end}
      self->wdata = value;

      //while(!self->ready){ For now we assume all writes have no delay 
          InternalUpdateAccelerator();
      //}

      self->valid = 0;
      self->wstrb = 0x00;
      #{if type.memoryMapBits != 0}
      self->addr = 0x00000000;
      #{end}
      
      self->wdata = 0x00000000;

      InternalUpdateAccelerator();
                
      return 0;
   } else {
      self->valid = 1;
      self->wstrb = 0x0;
    #{if type.memoryMapBits != 0}
      self->addr = address;
    #{end}

      self->eval();

      while(!self->rvalid){
          InternalUpdateAccelerator();
      }

      int res = self->rdata;

      self->valid = 0;
    #{if type.memoryMapBits != 0}
      self->addr = 0;
    #{end}

      self->eval();
      while(self->rvalid){
          InternalUpdateAccelerator();
      }

      return res;
   }
   #{end}
   return 0;
}

extern "C" void VersatSignalLoop(){
#{if type.signalLoop}
   V@{type.name}* self = dut;

   self->signal_loop = 1;
   InternalUpdateAccelerator();
   self->signal_loop = 0;
   self->eval();
#{end}
}

extern "C" void VersatLoadDelay(const unsigned int* delayBuffer){
  #{if type.baseConfig.delayOffsets.max}
  V@{type.name}* self = dut;
    #{for i type.baseConfig.delayOffsets.max}
  self->delay@{i} = delayBuffer[@{i}];
    #{end}
   self->eval();
  #{end}
}

#undef UPDATE
