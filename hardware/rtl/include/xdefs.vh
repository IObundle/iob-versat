//
// VERSAT DEFINES
//

`define nVERSAT 5

// EDIT: please assign to log2(nVERSAT)
`define nVERSAT_W 3

// VERSAT DATA WIDTH
`define DATA_W 32 // bits

// VERSAT ADDRESS WIDTH
`define ADDR_W 18

// VERSAT MEMORY TYPE

// Uncomment this line to use ASIC memories
//`define MEM_TYPE ASIC

// Uncomment this line to use Xilinx memories
//`define MEM_TYPE XILINX

// Uncomment this line to use Altera memories
//`define MEM_TYPE ALTERA

// Uncomment this line to use hard ROM
//`define ROM

//
// VERSAT MEMORY MAP
//
`define DMA_BASE `ADDR_W'h0020  //32-47
`define DIV_BASE `ADDR_W'h0030  //48-63
`define PRT_BASE `ADDR_W'h0040  //64-79
`define DUMMY_REG `ADDR_W'h1000 //4096-8191
`define CONF_BASE `ADDR_W'h2000 //8192-8319
`define ENG_BASE `ADDR_W'h20000  //131072-196607
