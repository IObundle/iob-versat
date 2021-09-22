#ifndef VERSAT_SIM
#define VERSAT_SIM
#include <iostream>
#include <thread>
#include "versat.h"
#include <string.h>
#include <bitset>
#include "stage.hpp"

//
// VERSAT CLASSES
//
class CMem;
class CMemPort;
class CALU;
class CALULite;
class CMul;
class CMulAdd;
class CStage;


//
//VERSAT global variables
//
#ifndef VERSAT_cpp // include guard
#define VERSAT_cpp

class CVersat{
public:
void versat_init(int base_addr);
void* run_sim(void* ie);
void run();
int done();
void globalClearConf();
void write_buffer_transfer();
void FU_buffer_transfer();
void free_mem();

int base;
int versat_iter = 0;
int versat_run = 0;
int versat_debug = 0;
int run_done = 0;

CStage stage[nSTAGE];
#if nVI > 0 || nVO > 0
#if nVI > 0 && nVO > 0
#define VERSAT_CONFIG_BUFFER_SIZE 2
CWrite write_buffer[nSTAGE][nVO][VERSAT_CONFIG_BUFFER_SIZE];
CStage buffer[nSTAGE];
#else
#define VERSAT_CONFIG_BUFFER_SIZE 1
#if nVI > 0
CStage buffer[nSTAGE];
#else
CWrite write_buffer[nSTAGE][nVO][VERSAT_CONFIG_BUFFER_SIZE];
#endif
#endif
#endif
CStage shadow_reg[nSTAGE];
#if nMEM > 0
CMem versat_mem[nSTAGE][nMEM];
#endif
};

class Acumulator {
public:
int iter,per,shift,incr,iter2,per2,shift2,incr2,iter3,per3,shift3,incr3;
uint8_t nloops=0;
int delay,duty,start;
int extAddr,intAddr;
Acumulator()
{
	iter=per=shift=incr=iter2=per2=shift2=incr2=iter3=per3=shift3=incr3=nloops=delay=duty=start=extAddr=intAddr=0;
}
void add_loop(int per,int incr = 1 )
{
	switch(nloops)
	{
		case 5:	this->iter3=this->per3;
				this->shift3=this->incr3;
		case 4: this->per3=iter2;
				this->incr3=shift2;
				if(this->iter3==0)
					this->iter3=1;
		case 3:	this->iter2=this->per2;
				this->shift2=this->incr2;
		case 2:	this->per2=iter;
				this->incr2=shift;
				if(this->iter2==0)
					this->iter2=1;
		case 1: this->iter=this->per;
				this->shift=this->incr;
		case 0: this->per=per;
				this->incr=incr;
				if(this->iter==0)
					this->iter=1;
		default: nloops++;
	}
}

void loop_settings(int start = 0, int duty = 0, int delay = 0,int extAddr = 0, int intAddr = 0)
{
	this->duty=duty;
	this->start=start;
	this->delay=delay;
	this->extAddr=extAddr;
	this->intAddr=intAddr;
}
};

int convolutional_layer_xyz(CStage* versat,int input_addr,int channels,int height, int width, int kernel_size, int stride, int pad,int weights_addr,int output_addr,int nkernels);
int matrix_mult(CStage* versat,int matrix_a,int matrix_b,int result_matrix,int r_a,int c_a,int r_b,int c_b,bool store);
int dot_product(CStage* versat,int vector_a,int vector_b,int result,int length,bool store); 
int set_IntMem_Write(CStage* obj,int index,Acumulator loop,int sel);
int set_ExtMem_Write(CStage* obj,int index,Acumulator ext_loop);
int set_IntMem_Read(CStage* obj,int index,Acumulator loop);
int set_ExtMem_Read(CStage* obj,int index,Acumulator ext_loop);
int bs_operation(CStage* obj, int op_a, int shift, int index, int operation);
int muladd_operation(CStage* obj, int op_a, int op_b, int index, int operation,int iter,int per, int delay, int shift);
int mul_operation(CStage* obj, int op_a, int op_b, int index, int operation);
int alu_operation(CStage* obj,int op_a,int op_b,int index, int alu_index,int operation); // alu_index 0 for ALU, 1 for ALU_LITE
void* run_simulator(void* in);

#endif

#endif




#define INFO 1

#if INFO == 1

void print_versat_mems(CVersat versat);
void print_versat_info(CVersat versat);
void print_versat_iter(CVersat versat);
void print_versat_config(CVersat versat);


#endif
