#include <stdio.h>
#include <stdlib.h>

#include "versat.h"

#include "FU_Defs.h"

#define nSTAGE 5

int32_t MemTerminateFunction(FUInstance* inst){
	MemGen4* s = (MemGen4*) inst->extraData;
	if(s->done)
		return 1;

	return 0;
}

int main(int argc,const char* argv[]){
	int i,j;
	int16_t pixels[25 * nSTAGE], weights[9 * nSTAGE], bias = 0;
	Versat versat = {};

	InitVersat(&versat);

	FU_Type ADDER = RegisterFU(&versat,"add",0,NULL,AddUpdateFunction);
	FU_Type MEM = RegisterFU(&versat,"mem",sizeof(MemGen4),MemStartFunction,MemUpdateFunction);
	FU_Type MULADD = RegisterFU(&versat,"muladd",sizeof(MulAddGen2),MulAddStartFunction,MulAddUpdateFunction);

	Accelerator* accel = CreateAccelerator(&versat);

	for (j = 0; j < nSTAGE; j++)
	{
		//write 5x5 feature map in mem0
		for (i = 0; i < 25; i++)
		{
		  pixels[25 * j + i] = rand() % 50 - 25;
		}

		//write 3x3 kernel and bias in mem1
		for (i = 0; i < 9; i++)
		{
		  weights[9 * j + i] = rand() % 10 - 5;
		}

		//write bias after weights of VERSAT 0
		if (j == 0)
		{
		  bias = rand() % 20 - 10;
		}
	}

	printf("\nExpected result of 3D convolution\n");
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			int res = bias;
			for (int k = 0; k < nSTAGE; k++)
			{
				for (int l = 0; l < 3; l++)
				{
					for (int m = 0; m < 3; m++)
					{
						res += pixels[i * 5 + j + k * 25 + l * 5 + m] * weights[9 * k + l * 3 + m];
					}
				}
			}
			printf("%d\t", res);
		}
		printf("\n");
	}
	printf("\n");

	int delay = 0;
	FUInstance* in_1_alulite = NULL;
	FUInstance* mem0[nSTAGE]; 
	FUInstance* mem2[nSTAGE];
	for(j = 0; j < nSTAGE; j++){
		mem0[j] = CreateFUInstance(accel,MEM);
		FUInstance* mem1 = CreateFUInstance(accel,MEM);
		FUInstance* muladd0 = CreateFUInstance(accel,MULADD);
		FUInstance* add0 = CreateFUInstance(accel,ADDER);
		mem2[j] = CreateFUInstance(accel,MEM);

	    //configure mem0A to read 3x3 block from feature map
		{
			MemGen4* s = (MemGen4*) mem0[j]->extraData;
			s->iter = 3;
			s->incr = 1;
			s->delay = delay;
			s->per = 3;
			s->duty = 3;
			s->shift = 5-3;

			s->mem = (int16_t*) malloc(sizeof(int16_t) * 25);
		    for (i = 0; i < 25; i++)
		    {
		      s->mem[i] = pixels[25 * j + i];
		    }
		}

		//configure mem1A to read kernel
		{
			MemGen4* s = (MemGen4*) mem1->extraData;
			s->iter = 1;
			s->incr = 1;
			s->delay = delay;
			s->per = 10;
			s->duty = 10;

			s->mem = (int16_t*) malloc(sizeof(int16_t) * 10);
		    for (i = 0; i < 9; i++)
		    {
		    	s->mem[i] = weights[9 * j + i];
		    }
			if(j == 0)
		    	s->mem[9] = bias;
		}

		//configure muladd0
		{
			MulAddGen2* s = (MulAddGen2*) muladd0->extraData;
			s->iter = 1;
			s->per = 9;
			s->delay = delay + 1;
		}

		// Connect units
		muladd0->inputs[0] = mem0[j];
		muladd0->inputs[1] = mem1;
		mem2[j]->inputs[0] = add0;
		add0->inputs[0] = muladd0;

		if(j == 0){
			add0->inputs[1] = mem1;
		} else {
			add0->inputs[1] = in_1_alulite;
		}
		in_1_alulite = add0;

		delay += 1;
	}
	
  	//config mem2A to store ALULite output
	{
		MemGen4* s = (MemGen4*) mem2[nSTAGE - 1]->extraData;
		s->iter = 1;
		s->incr = 1;
		s->delay = 9 + 1 + delay;
		s->per = 1;
		s->duty = 1;
		s->in_wr = 1;
		s->mem = (int16_t*) malloc(sizeof(int16_t) * 9);
	}

	printf("Result of accelerator\n");
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			for (int k = 0; k < nSTAGE; k++){
				MemGen4* s = (MemGen4*) mem0[k]->extraData;
				s->start = i * 5 + j;
			}
			MemGen4* s = (MemGen4*) mem2[nSTAGE - 1]->extraData;
			s->start = i * 3 + j;

			AcceleratorRun(&versat,accel,mem2[nSTAGE - 1],MemTerminateFunction);

			printf("%d\t",s->mem[i * 3 + j]);
		}
		printf("\n");
	}

	return 0;
}