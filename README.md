# What is Versat

At its core, Versat is a compiler that transforms a high-level specification into a custom Coarse-Grained Reconfigurable Array (CGRA) hardware accelerator. Versat is written in the C++ language.

## Table of Contents

- [Dependencies](#dependencies)
- [Compilation](#compilation)
- [Integration in IOb-SoC](#integration-in-iob-soc)
- [Versat Tutorial](#versat-tutorial)
  - [Dataflow Paradigm](#dataflow-paradigm)
  - [Basic Units](#basic-units)
  - [Versat Specification](#versat-specification)
  - [Generated Hardware and Software](#generated-hardware-and-software)
  - [Running a Simple Example](#running-a-simple-example)
  - [PC Emulation and IOb-SoC Integration](#pc-emulation-and-iob-soc-integration)
  - [Custom Units](#custom-units)
  - [Data Validity and Delays](#data-validity-and-delays)
  - [Databus Interfaces](#databus-interfaces)
  - [Memory Mapping](#memory-mapping)
  - [Configuration Shadow Register](#configuration-shadow-register)
  - [Configuration Sharing](#configuration-sharing)
  - [Merging Units](#merging-units)
    - [Advanced Specification Syntax](#advanced-specification-syntax)
  - [Address Generation](#address-generation)
- [Publications](#publications)
- [License](#license)
- [Acknowledgement](#acknowledgement)
    
    

  
# Dependencies

- GNU Bash >=5.1.16
- GNU Make >=4.3
- GCC with C++17 support
- Verilator >= 5

Build systems that use nix-shell can add versat as a package, and versat will be built when needed.

# Compilation

To compile Versat, run the following commands:

```bash
make clean
make -j versat
```


# Integration in IOb-SoC

Versat currently implements the Python setup approach IOb-SoC used to integrate with the rest of the SoC build system.

Since Versat generates the accelerator, the top-level Python setup script must create the Python class representing it. This class is produced by the CreateVersatClass function, which returns a class specific to the generated accelerator and acts like any other peripheral class. The [iob-soc-versat](https://github.com/IObundle/iob-soc-versat) repository provides an example of integrating Versat with IOb-SoC.

Versat also contains a nix package definition, which allows nix-shell users to enter an environment with all dependencies and versat compiled automatically. The [iob-soc-versat](https://github.com/IObundle/iob-soc-versat) repository provides an example of using this package.

# Versat Tutorial

Versat inputs a file that specifies its top-level name and multiple accelerators as dataflow graphs and generates a CGRA. Versat outputs hardware source files, written in Verilog, and software source and header files that allow the software to configure and run the generated accelerator.

The specification is written in the language described in section [Versat specification](https://github.com/iobundle/iob-versat#Versat-specification). The following commands can be used to get command line help:

```bash
./versat -h
# or
./versat --help
```

## Dataflow Paradigm

Versat is based on the dataflow paradigm of computation. In this paradigm, computation is described as moving data across units that perform operations on the data as it moves through them. In Versat, some units are data sources; some units only perform computation, like additions and multiplications, which only produce data from data they receive as input; and some units are sinks of data, only storing results later accessed by software.

The programmer who intends to use Versat to generate the hardware needs to describe the dataflow graph of the algorithms to implement. Describing a dataflow graph is as simple as telling the units (nodes of the graph) and the connections between units, which are the edges. 

The dataflow graph can contain global loops, which include a loop that englobes the whole graph, but internal loops are not allowed. Versat will give an error when the graph specified is not supported.

## Basic Units

The connection of basic units of operation defines dataflow programming. Units contain inputs and outputs and usually include some internal logic to produce the output from the inputs.

All basic units supported by Versat are either simple Verilog operations, like addition and shifts, or are implemented as Verilog modules. Versat implements all the operations supported by Verilog and already contains a set of basic units that can be found in hardware/src/units.

Basic units can contain an internal state, have any number of inputs and outputs, and use various interfaces to obtain data from the system. Versat only supports units that take a fixed amount of time to process data. The inputs and outputs have data streams manipulated by the units. Units that only contain outputs are called sources, and they produce a stream of data. Units containing inputs and outputs are called compute units, and they create data streams from input streams. Finally, sink units, which only contain inputs, take in streams as inputs, do not output anything, and usually store data in memory. Stream processing helps us understand how Versat handles data validity across the accelerator, as further explained in the section [Data validity and delays](https://github.com/iobundle/iob-versat#Data-validity-and-delays).

The programmer uses Versat to design more complex units made of instantiating and connecting units with themselves. Basic units are the building blocks from which higher-level units are created. Any unit that can be made by instantiating and interconnecting basic units should be designed using that approach so that Versat can automatically handle all the complexities of integrating the unit with the accelerator. However, the programmer can define a set of basic units for Versat if needed. These custom units must implement specific interfaces to integrate with the accelerator generated by Versat. These interfaces and their format are explained in the section [Custom units](https://github.com/iobundle/iob-versat#Custom-units).

## Versat Specification

Complex units are designed in the Versat specification language. Its syntax is still evolving as Versat improves and may change. Inspired by Verilog and C, the language describes higher-level units using a hierarchical approach. Complex units are represented by instantiating and connecting simpler units. Units can only instantiate previously defined units (prevents recursive instantiation, which is not permitted in dataflow designs - a unit cannot instantiate itself). 

Complex units can be of different types. Currently, Versat supports three kinds of complex units: module, merge, and iterative. 

A Module unit type is a simple grouping of units and their connections with no other special meaning. A module definition is separated into two parts: The first part instantiates the module subunits, and the second part describes their connections. Binary operations (like additions and shifts) can be defined directly in the second portion without declaring them in the first portion using a syntax similar to assignments in languages like C. 

```verilog
// C style comments are allowed, including multiline comments using /* */

module SimpleExample(){ // Module definition with zero inputs
  // Use this portion to instantiate units
  // In this example, we are instantiating two Const units and one Reg unit.

  Const a; // Const is a simple unit that outputs a constant configurable value.
  Const b;
  Reg result; // Reg is similar to a hardware register, but more complex since it is an actual Versat unit. 

# // This separates instance declaration from interconnection statements
    
  addition = a + b; //This assignment statement is one of the two types used when performing basic logic and arithmetic operations.

  addition -> result; // The other type of statement is the "connection". We are connecting the output of addition the the input of result.
}
```

The SimpleExample module defines two Const units and one Reg unit. The Const units constantly output a single value, which software can configure, as we will see later. The Reg unit implements a hardware register: internally, it stores a value, outputs it constantly, and contains an input to store a new value. 

Unlike regular registers, however, the register does not store values every cycle. The CPU must instruct the accelerator to start working; at this point, the accelerator will perform a "run." For this simple example, a run would take one cycle. At the end of the run, the Reg "result" would contain the sum of the values of 'a' and 'b.' 

Like all modules defined using this language, this module can be used as the Top module for hardware generation; the Versat program must be called with the file that contains this module passed as a command line, and the name of this module, 'SimpleExample,' must be set as the top-level module.

## Generated Hardware and Software

Versat generates header files that define the interface to interact with the accelerator. The header defines the entire API for that particular accelerator. For the SimpleExample, the generated header file looks something like this:

```cpp
// The interface for each unit used is generated
// The config interface allows software to set configuration data for each unit
typedef struct {
  int constant;
} ConstConfig;

// The header preserves the original hierarchy when generating the headers
typedef struct {
  ConstConfig a;
  ConstConfig b;
} SimpleExampleConfig;

// The state interface allows the software to read data the unit exposes.
typedef struct {
  int value;
} RegState;

typedef struct {
  RegState result;
} SimpleExampleState;

extern volatile SimpleExampleConfig* accelConfig;
extern volatile SimpleExampleState*  accelState;

void versat_init(int baseAddress);
void RunAccelerator();
```

To make the configuration process as simple and efficient as possible, we minimize function calls when possible and instead allow the software to write directly to the accelerator's memory-mapped registers. The SimpleExampleConfig and SimpleExampleState structs generated for the SimpleExample match directly with the interface of the generated accelerator. 

Writing to members of the accelConfig pointer and reading from members of the accelState pointer will write and read from the accelerator memory-mapped interface.

Versat tries to maintain the hierarchy defined in the specification file in generating these structures. The software can build abstractions over the generated API by making functions that take pointers to these structures. For an example of this, look at the file [unitConfiguration.h](https://github.com/IObundle/iob-soc-versat/blob/main/software/src/Tests/unitConfiguration.hpp) in the iob-soc-versat repository.

```cpp
int main(){
    versat_init(VERSAT_BASE); // Like other peripherals, Versat needs to be initialized using the versat_init function. This function must be the first function called before anything else. The base address of the accelerator needs to be passed.

    // Set values for the a and b constant units
    accelConfig->a.constant = 10;
    accelConfig->b.constant = 5;

    RunAccelerator(1); // After configuring everything, run the accelerator once.

    printf("Result: %d\n",accelState->result.currentValue); // Outputs "Result: 15".
    return 0;
}
```

The above is an example of software exercising the accelerator generated from the SimpleExample module. versat_init needs to be called first before anything else.

For this simple example, we configure the constant units with two simple values, run the accelerator once by calling the RunAccelerator function, and read the computation result by reading the state of the Reg unit.

RunAccelerator is a higher-level function that starts the accelerator and then loops, waiting for the accelerator to finish before returning. More advanced usages of the accelerator allow the programmer to run the accelerator in the background while the software executes. This usage is further explained in section [Configuration shadow register](https://github.com/iobundle/iob-versat#Configuration-shadow-register).

As shown in the example, Config interfaces are used to write data into the units, and State interfaces are used to read data from units. Attempting to read data from a Config interface or write data to a State interface causes undefined behavior and can potentially lead to a system deadlock. It is the programmer's responsibility to prevent this from occurring.

## Running a Simple Example

While Versat is a standalone program, it is also integrated into the IOb-SoC framework. As a "peripheral" of IOb-SoC, the accelerators generated by Versat can be simulated, PC-emulated, and compiled to run on FPGA.

The simple example mentioned previously can be tested by cloning the [iob-soc-versat](https://github.com/IObundle/iob-soc-versat) and performing these steps inside the repository:

```bash
make clean pc-emul-run TEST=SimpleExample
make clean sim-run TEST=SimpleExample
make clean fpga-run TEST=SimpleExample
```

Before installing, check the README inside the iob-soc-versat repository to learn how to clone it correctly and which dependencies are required.

## PC Emulation and IOb-SoC Integration

PC emulation is similar to simulation since Versat uses Verilator to compile the accelerator design into a C++ simulable model. This fact makes PC emulation faster than simulation while preserving much RTL simulation accuracy. The only difference between PC emulation and simulation is the interface between the accelerator and the system, which must be emulated by a custom wrapper that Versat generates. It is possible that the incorrect usage of the accelerator can produce bugs that are not detected in PC emulation and only appear in simulation.

To integrate Versat pc-emulation with IOb-SoC, we need to make some changes to the build system:

```
Insert "-include VerilatorMake.mk" inside the software Makefile
Add libaccel.a as a prerequisite of the generated executable
Link '-lm libaccel.a -lstdc++' (in this order) when compiling the executable 
```

These changes are in the [iob-soc-versat](https://github.com/IObundle/iob-soc-versat) repository. 

## Custom Units

As mentioned previously, Versat is capable of integrating custom-made units. These units are considered essential and can be instantiated like any other unit. Versat currently only accepts Verilog designs, and the units must implement specific interfaces to integrate correctly with the generated accelerator.

The interfaces, wire formats expected, and their usages are as follows:

```verilog

module ExampleCustomUnitWithAllInterfaces #(
    // Some of the parameters that the interface might need to support
    // The following parameters are used by Versat and have special meaning.
    parameter DATA_W, - Granularity of the coarse grained accelerator. Units are not required to support variable sized inputs and outputs (they can have fixed sizes) but supporting allows Versat to use that unit for different granularity values.
    parameter AXI_ADDR_W, - Address width of the databus connection (which connects to an external SRAM memory). 
    parameter AXI_DATA_W, - Data width of the databus connection (which connects to an external SRAM memory). 
    parameter DELAY_W, - Maximum size of the delay value. Parameter of the delay{i} input. Versat automatically calculates this.
    parameter LEN_W, - Maximum transfer length. Parameter of the databus_len input. User must set this value depending on the maximum transfer length that it expects the unit to perform.

    // This parameter is unit specific. In this case, is used to change the maximum size of the memory mapped interface. Versat allows parameters to be defined inside the specification file, meaning that it is possible for the user to define parameter values when instantiating the unit. 
    parameter ADDR_W
  )

  // Standard wire types (optional)
  input clk,
  input rst,
  input run, // Only asserted for one cycle.
  input running, // Asserted while the accelerator is running. The unit can "turn off" its logic while this signal is de-asserted to save energy and speed up simulation.

  // Inputs and Outputs
  input [DATA_W-1:0] in0, // Input.
  (* versat_latency = T *) output [DATA_W-1:0] out0, // Output. Because Versat needs to calculate statically the amount of time data needs to traverse the dataflow graph, the user must indicate the amount of time it takes for the unit to produce valid data from the moment it receives valid data as input. This latency is passed through Versat using a Verilog attribute.

  output done; // This wire indicates to the accelerator that the unit has finished performing all the operations. Only units that take a variable amount of time to finish must implement this interface.

  // Delay: delays are used by Versat to indicate how many cycles the unit must wait before the input contains valid data.
  input [DELAY_W-1:0] delay0, 

  // Memory mapped - Simple memory mapped interface. Follows the specifications of the IOb native interface
  //                 Note that this interface is usually used when the accelerator is not running.
  input                 valid,
  output                rvalid,
  input  [DATA_W/8-1:0] wstrb,
  input  [  ADDR_W-1:0] addr,
  input  [  DATA_W-1:0] wdata,
  output [  DATA_W-1:0] rdata,

  // Databus interface - Interface that connects to RAM. Units can implement more than one such interface.
  //  The databus interface is similar to a master AXI lite, except we must know the transfer length beforehand.
  //   The last signal still exists, meaning the unit does not need to track how many transfers occurred.
  //   For more information, check the specifications of the AXI lite interface
  input                     databus_ready_0,
  output                    databus_valid_0,
  output [  AXI_ADDR_W-1:0] databus_addr_0,
  input  [  AXI_DATA_W-1:0] databus_rdata_0,
  output [  AXI_DATA_W-1:0] databus_wdata_0,
  output [AXI_DATA_W/8-1:0] databus_wstrb_0,
  output [       LEN_W-1:0] databus_len_0,
  input                     databus_last_0,

  // External memory 2P - Implements a two-port memory interface to a memory that Versat instantiates.
  output [ANY_SIZE:0] ext_2p_addr_out_0,
  output [ANY_SIZE:0] ext_2p_data_out_0,
  output              ext_2p_write_0,
  output [ANY_SIZE:0] ext_2p_addr_in_0,
  input  [ANY_SIZE:0] ext_2p_data_in_0,
  output              ext_2p_read_0,

  // External memory DP - Implements a true dual-port interface to a memory that Versat instantiates.
  //                      All these wires belong to the same interface. 
  output [ANY_SIZE:0] ext_dp_addr_0_port_0,
  output [ANY_SIZE:0] ext_dp_out_0_port_0,
  input  [ANY_SIZE:0] ext_dp_in_0_port_0,
  output              ext_dp_enable_0_port_0,
  output              ext_dp_write_0_port_0,
  output [ANY_SIZE:0] ext_dp_addr_0_port_1,
  output [ANY_SIZE:0] ext_dp_out_0_port_1,
  input  [ANY_SIZE:0] ext_dp_in_0_port_1,
  output              ext_dp_enable_0_port_1,
  output              ext_dp_write_0_port_1,

  // Config - Any input wire whose name does not match any other interface is treated as part of the Config interface
  input [ANY_SIZE:0] anyOtherName,

  // State - Same logic for State interfaces, except that the wire must be an output.
  output [ANY_SIZE:0] anyOtherName,

```

Any interface that contains a number on the end can be implemented multiple times by a single unit. The number must start at zero and increment by one for every extra interface. The only exception is the dp external memory interfaces, where the number that identifies the interface appears first; the last number is used to specify the port.

The wires' sizes must match. Units must also declare the parameters if they implement any interface that uses them. 

For the external memories, the size used for address and data can differ between ports, but the total amount of memory allocated must match. Versat automatically handles the instantiation of memories with different sizes, as in the following example:

```verilog
// Example where the address and data size match between ports. This memory can store 128 bits and contains ports that look at the memory as 16 x 8.
output [3:0] ext_2p_addr_out_0,
output [7:0] ext_2p_data_out_0,

output [3:0] ext_2p_addr_in_0,
input  [7:0] ext_2p_data_in_0,

// Example with mismatched ports. This memory can also store 128 bits in total, but with the difference that the write port sees it as 8 x 16 bits while the read port sees it as 16 x 8 bits.
output [2:0] ext_2p_addr_out_0,
output [15:0] ext_2p_data_out_0,

output [3:0] ext_2p_addr_in_0,
input  [7:0] ext_2p_data_in_0,
```

## Data Validity and Delays

To handle data validity problems, Versat precalculates how many cycles it takes for data to flow from unit to unit and delays faster paths to match slower paths if needed. Each unit that requires the ability to differentiate between valid data and garbage can do so by implementing the delay interface. Versat handles the entire process of calculating the delays and automatically ensures that the accelerator is correctly configured. The end user should not need to know these details. 

This form of handling data validity guarantees that every unit simultaneously receives valid data on all its inputs. Units do not need to delay or wait for other units as the accelerator is built such that all the valid data arrives in the same cycle. 

```verilog
module ValidityExample(){
  Mem slow;
  Mem fast;

  PipelineRegister pipe; // PipelineRegister is a simple unit that takes one cycle to output whatever value it contains on its input. It effectively delays data by one cycle.

  Reg result;
#
  slow -> pipe; // Pipe delays slow by one cycle.

  sum = pipe + fast; // Pipe is one cycle delayed compared to fast. Versat automatically delays the 'fast' path by one cycle to ensure that both inputs arrive simultaneously.

  sum ->result; // Versat guarantees that the value stored by "result" is equal to the addition of the first value outputted by fast and the first value outputted by slow
}
```

The ValidityExample showcases how data validity is handled by default. Regardless of how much time it takes to compute the value of a path, Versat guarantees that when multiple paths converge on a single unit, the unit will have all its inputs carrying valid data on the same cycle. 

This approach is ideal for circuits where computations only concern current values. However, if operations with past values are required, manual delays can change when we expect the values.

```verilog
module DelayExample(){
    Mem mem;
# 
    // An expression of the form [unit]{N} is used to delay the arrival of the data by N cycles.
    // Remember, in the dataflow paradigm, data is continuously streamed at every cycle. 
    // For example, the first valid value from mem{3} is the fourth valid value outputted by mem. Next cycle, the value will be the fifth valid value from mem.

    sum = mem{0} + mem{1} + mem{2} + mem{3}; // Sums four adjacent values from memory every cycle starting at the first value outputted by mem.

    sum -> out;
}
```

In this example, we have a simple memory unit outputting a stream of values. To sum a portion of these values together, we add delays by using the {N} operation, where the delay is given by N. The delay is equivalent to shifting the stream N units when considering data streams. For example, if we assume that the unit 'mem' was configured to produce a stream of natural numbers starting from zero, then the accelerator computes the following values:

| Cycle | mem{0} | mem{1} | mem{2} | mem{3} | sum |
| ----- | ------ | ------ | ------ | ------ | --- |
|     0 |      0 |      1 |      2 |      3 |   6 |
|     1 |      1 |      2 |      3 |      4 |  10 |
|     2 |      2 |      3 |      4 |      5 |  14 |
|     3 |      3 |      4 |      5 |      6 |  18 |

## Databus Interfaces

Units that implement databus interfaces can access external RAM. This interface only works while the accelerator is running. Units that use databus interfaces cannot expect to complete a transfer in a fixed amount of time and must implement the "done" wire.

Since the databus interface does not work in a fixed-time setting, it is not possible to obtain data that is fed directly into the circuit using it. Generally, any unit that uses the database interface needs to perform transfers to or from memory. 

The VRead and VWrite units, which come by default with Versat, are examples of using a databus interface. The VRead units read from RAM and output data to the circuit, while the VWrite unit stores data from the circuit and writes to RAM.

Since these units cannot perform the transfers simultaneously interacting with the circuit, they employ internal memory to act as a buffer. While one portion of the memory is being written with data from RAM, the other portion is being read to output data to the circuit. In the next run, the write and read portions are flipped so that data is read from where the previous run wrote to and vice versa.

## Memory Mapping

Memory mapping is an interface that allows software to interact with units as if writing to memory, unlike the Config interface, where the software stores some values in a register connected to the unit, the memory-mapped interface allows direct communication between the CPU and the unit itself.

Memory mapping usually handles infrequent large data transfers, like initializing a lookup table or a memory unit. A databus interface would be overkill since it is heavier and more complicated. Databus interfaces mostly fill units containing memories but do not implement databus interfaces (like LookupTables).

In order to support PC emulation, we cannot access memory-mapped units in the same way we access configurations. All memory-mapped accesses must use specific functions defined in versat_accel.h.

Furthermore, large memory transfers should be performed using the VersatMemoryCopy function, which can use an internal DMA to speed up the transfer. The DMA only supports transfers between memory and the accelerator; it does not support memory-to-memory or accelerator-to-accelerator transfers.

## Configuration Shadow Register

The Versat architecture stores all the units' configurations in a large register inside the accelerator. This register is later connected to the units, so they do not need to store their configurations internally.

By default, Versat generates the accelerator with an additional configuration "shadow register," which allows the configuration register to be safely modified while the accelerator is running without affecting the current run results. Any change to the configuration register is only visible to the units at the beginning of the next run. 

The software is expected to configure a run while the previous run is still executing to maximize performance. For this purpose, the software must call the StartAccelerator function to start the accelerator and immediately return without waiting for it to stop. Afterward, the software can change configurations without affecting the current run. 

The function EndAccelerator waits for the accelerator to stop. It must be called before the software accesses any data that depends on the accelerator's completion, including the State and memory-mapped interfaces and any memory the accelerator can write to. 

## Configuration Sharing

We implement two ways of sharing the config interface between units to facilitate the design of SIMD portions of code. Note that units with shared configs still work with their inputs, outputs and internal states. Sharing configuration does not imply sharing any other interface. Only Config sharing is currently supported.

Of the two ways, the simplest is to define a set of units inside a module with shared configs. We can only share configurations between units of the same type, and as such, the shared construct is defined by using the shared keyword, specifying the type, and then using a block to determine all the shared instances.

Shared units constructed this way have their configurations shared directly inside the generated structures using a union instead of a struct. In general, when using shared units, the format of the generated struct is a good way of figuring out how the configuration is being shared inside the accelerator.

The second way is using the keyword "static" before an instance declaration. Mimicking the concept found in object-oriented languages, a static unit is bound to its parent module, and every time the module is instantiated, that unit shares its configuration with every instance that instantiates the parent module. Every instance of the module will instantiate the static unit containing the same configuration as every other instance of the same unit, regardless of the position on the hierarchy. 

Unlike configuration sharing, static units contain the structure used to configure them. Static units help implement registers that share constants instead of using extra inputs and outputs to pass the data into lower hierarchy units.

Versat spec:
```verilog
module StaticExample(){
    static Const const;
#
    constant -> out:0;   
}

module ShareExample(){
    share config Const {
        shared_0;
        shared_1;
    }
    StaticExample static_0;
    StaticExample static_1;
#
    {unit[0..1],static[0..1]} -> out:0..3;
}
```

Header generated:
```cpp
// The shared units create a structure that uses unions for the shared units' configs
// The structure mimics the generated configuration register. Changing the configuration of the unit[0] is the same as changing the configuration of the unit[1]. 
typedef struct {
  union{
    ConstConfig unit_0;
    ConstConfig unit_1;
  };
} ShareExampleConfig;

// Static, however, is extracted into a separate structure.
// There exists only one value; any instance of StaticExample will share the configuration of const with all other instances of StaticExample
typedef struct {
  int StaticExample_const_constant;
} ShareExampleStatic;
```

## Merging Units

While implementing an algorithm using dataflow paradigms, it is helpful to divide the algorithm into multiple parts processed using its accelerator. When using Versat, this would be the equivalent of defining various modules, one for each algorithm step, and then grouping all these modules into a top-level unit, which would later be used to generate the final accelerator.

This method works, and it is possible to solve problems using this approach, but it has flaws. Since each module is associated with a given step of an algorithm, at any given point in the algorithm's execution, only one of the modules is being used. The other modules are not being used while still occupying resources. 

While the programmer could design the modules to offer a way of sharing units, we also designed Versat to perform this work automatically. We call this operation merge, and like modules, merge defines new unit types that can be used just like modules.

```verilog
module Child1(){
  Const a;
  Const b;
# 
  ...  
}

module Child2(){
  Const x;  
#
  ... 
}

merge Merged = Child1 | Child2;
```

The merge operation is simple: We only need to describe which types we want to merge. In this example, Merged is just like any other type. It can be instantiated inside modules, part of another merge, or used as the top-level module for code generation.

```cpp
typedef struct {
  ConstConfig a;
  ConstConfig b;
} Child1Config;

typedef struct {
  ConstConfig x; // Since x is in the first slot of the structure, it means that Versat merged Child1.a with Child2.x
  int unused;    // Since the configuration is shared, some structures will have "unused" to pad data to fit their actual places.
} Child2Config;

/*
Note that Versat chooses to merge Child1.a with Child2.x.
Since child1 contains two Consts, it would also be possible for Versat to merge Child1.b with Child2.x;
Which would produce the following structure:

typedef struct {
  int unused; 
  ConstConfig x;
} Child2Config;
*/

typedef struct {
  union { // It is a union. It matches the actual accelerator configuration register, which is shared across units depending on which units were merged
    Child1Config Child1;
    Child1Config Child2;
  };
} MergedConfig;

// The only extra step required for software writers it to inform the accelerator which unit is currently activated.
// The ActivateMergedAccelerator function needs to be called so that Versat can change the datapath so that the accelerator performs the operations associated with the given merged unit.

typedef enum{
    MergeType_Child1 = 0, MergeType_Child2 = 1  
} MergeType;

void ActivateMergedAccelerator(MergeType type);
```

To merge units, Versat must flatten the dataflow graphs into the most basic units. The hierarchy information becomes, therefore, unavailable. However, Versat still tries to keep the same hierarchical structure as much as possible when generating the software. For this simple example, the generated structs for Child1Config and Child2Config are generated not to match their original configurations but to match the configurations from the point of view of the merged unit, explaining why Child2Config contains an "unused" extra member and its necessity is found by looking at what would happen if Versat merged Child1.b with Child2.x: Since unit 'b' of Child1 is the second configuration value after the merge with Child2.x, Child2.x configuration must match the position of Child1.b, which is only possible if we add some padding.

### Advanced Specification Syntax

These specifications are not shown in previous examples, but units can contain multiple inputs and outputs, meaning that connections must encode the ports and the units. By default, if only the unit is specified, the port used is assumed to be port 0. Otherwise, the designer can specify the ports as follows:

```verilog
module PortExample(x,y){ // This module contains two inputs.
    Mem mem; // Mem units contain two input and two output ports.
#
    // This
    x -> mem;   // Connects input named x to port 0 of mem.
    y -> mem:1; // Connects input named y to port 1 of mem.

    // is equivalent to this
    x -> mem:0; // Connects input named x to port 0 of mem.
    y -> mem:1; // Connects input named y to port 1 of mem.

    // which is equivalent to this
    {x,y} -> mem:0..1; // To simplify connections, we can group multiple expressions and use ranges to represent multiple ports.

    mem:0..1 -> out:0..1; // The out instance is a specially named unit representing the module output. It can have any amount of sequential ports.
}

module ModuleUsage(){
  PortExample example; // PortExample contains two inputs and two outputs.

  Const constants[2]; // We can instantiate multiple units by using an array expression like in C
  Reg result[2];
#
  constants[0..1] -> example:0..1; // We can use ranges inside array expressions, inside delay expressions, and port expressions

  example:0..1 -> result[0..1]; 
}
```

## Address Generation

To simplify the usage of VUnits (VRead and VWrite) we allow the user to describe complex address generation expressions using for loops and mathematical expressions.

A couple of example of such usage can be found in [iob-soc-versat](https://github.com/IObundle/iob-soc-versat). A simple address gen is as follows:

```verilog
addressGen Read Simple(a){
   for x 0..a
   addr = 2 * x;
}

module AddressGenUserExample(){
  using(Simple) VRead v;
  using(Simple) Mem  m;
#
  v -> m;
}
```

This address generator construct will generate a C function that is capable of configuring both the VRead and the Mem to generate the addresses defined by the expression 2 * x. Note that the x variable is the variable of a simple for loop construct and that this for loop depends on an input. This input is provided at runtime. The C code generated takes an integer as an input and therefore the user code running on the firmware is capable of programming both the VRead and the Mem unit to read different amount of values depending on the value provided. 

```C
  int* input = ...;
  Simple_VRead(&accelConfig->v,input,2);
  Simple_Mem(&accelConfig->m,2);
```

This C code example demonstrates how to use the generated functions. The first argument is a pointer to the configuration struct of the unit. The VRead function also needs and aditional argument since it is gonna read from memory. The last argument is the input that is defined by adddressGen as input 'a'. 

Note that the for loops include the start and exclude the end. That means that if we called the functions with the value 'a = 0', the units would perform no transfer.

# Publications

Although the current status of the project has evolved, this project originated several publications as listed below. If you use this repository, please consider citing our publications on the Versat project.

@inproceedings{lopes2016versat,
  title={Versat, a Minimal Coarse-Grain Reconfigurable Array},
  author={Lopes, Joao D and de Sousa, Jos{\'e} T},
  booktitle={12th International Meeting on High-Performance Computing for Computational Science (VECPAR 2016)},
  year={2016}
}

@inproceedings{lopes2016versat,
  title={Versat, a Runtime Partially Reconfigurable Coarse-Grain Reconfigurable Array using a Programmable Controller},
  author={Lopes, Jo{\~a}o D. and Santiago, Rui and Sousa, Jos{\'e} T.},
  booktitle={Jornadas Sarteco},
  pages={561--569},
  year={2016},
  organization={Ediciones Universidad Salamanca}
}

@inproceedings{santiago2017compiler,
  title={Compiler for the Versat Reconfigurable Architecture},
  author={Santiago, Rui and Lopes, Jo{\~a}o D. and de Sousa, Jos{\'e} T.},
  booktitle={Jornadas sobre Sistemas Reconfigur{\'a}veis (REC 2017)},
  pages={41--48},
  year={2017}
}

@inproceedings{d2017fast,
  title={Fast Fourier Transform on the Versat CGRA},
  author={D. Lopes, Joao and T. de Sousa, Jose},
  booktitle={Jornadas Sarteco},
  year={2017}
}

@inproceedings{lopes2017k,
  title={K-means clustering on CGRA},
  author={Lopes, Joao D and de Sousa, Jos{\'e} T and Neto, Hor{\'a}cio and V{\'e}stias, M{\'a}rio},
  booktitle={27th Int. Conference on Field Programmable Logic and Applications},
  year={2017}
}

@inproceedings{fiolhais2019low,
  title={Low energy heterogeneous computing with multiple RISC-V and CGRA cores},
  author={Fiolhais, Lu{\'\i}s and Gon{\c{c}}alves, Fernando and Duarte, Rui P and V{\'e}stias, M{\'a}rio and de Sousa, Jos{\'e} T},
  booktitle={2019 IEEE International Symposium on Circuits and Systems (ISCAS)},
  pages={1--5},
  year={2019},
  organization={IEEE}
}

@inproceedings{mario2020implementing,
  title={Implementing cnns using a linear array of full mesh cgras},
  author={M{\'a}rio, Valter and Lopes, Jo{\~a}o D and V{\'e}stias, M{\'a}rio and de Sousa, Jos{\'e} T},
  booktitle={Applied Reconfigurable Computing. Architectures, Tools, and Applications: 16th International Symposium, ARC 2020, Toledo, Spain, April 1--3, 2020, Proceedings 16},
  pages={288--297},
  year={2020},
  organization={Springer International Publishing}
}

@article{lopes2021coarse,
  title={Coarse-Grained Reconfigurable Computing with the Versat Architecture},
  author={Lopes, Jo{\~a}o D. and V{\'e}stias, M{\'a}rio P. and Duarte, Rui Policarpo and C. Neto, Hor{\'a}cio and de Sousa, Jos{\'e} T.},
  journal={Electronics},
  volume={10},
  number={6},
  year={2021},
  publisher={MDPI}
}

# License

IOb-SoC-Versat is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.



# Acknowledgement

The [OpenCryptoLinux](https://nlnet.nl/project/OpenCryptoLinux/) project was funded through the NGI Assure Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement No 957073.

<table>
    <tr>
        <td align="center" width="50%"><img src="https://nlnet.nl/logo/banner.svg" alt="NLnet foundation logo" style="width:90%"></td>
        <td align="center"><img src="https://nlnet.nl/image/logos/NGIAssure_tag.svg" alt="NGI Assure logo" style="width:90%"></td>
    </tr>
</table>

