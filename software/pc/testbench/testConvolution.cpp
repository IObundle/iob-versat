//import custom libraries

#include "versat.hpp"

//import c libraries
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if nVI > 0 && nVO > 0
// Use Xreads and xwrites instead of nMEM
#define IO_VERSAT 1
extern versat_t *FPGA_mem;
#else
#define IO_VERSAT 0
#endif


#define VERSAT 0
int main(void) {
	int i, j;
	time_t t;
	srand((unsigned) time(&t));
	cout << "Testing Convolution Layer xyz on Deep Versat\n";

	int kernel_size=2;
	int channels = 3;
	int nkernels = 2;
	int height = 4;
	int width = 4;
	//int height = 9;
	//int width = 9;
	int input_size = height*width*channels;
	int stride = 1;
	int pad = 0;
	int out_w=((width + 2*pad - kernel_size) / stride) + 1;
	int out_h=((height + 2*pad - kernel_size) / stride) + 1;
	cout << "Randomized Input " << height << "x" << width << "\n";
	versat_t input[input_size]={0};
	versat_t kernel[kernel_size*kernel_size*nkernels]={0};


	if(width<10)
	{
		for(i=0; i<input_size; i++) {
			if(i%width==0 && i!=0)
				printf("\n");
			if(i%(width*height)==0)
				printf("\n \n");
			input[i]=rand() % 50 - 25;
			printf("%d\t",input[i]);
		}
		for(i=0; i<kernel_size*kernel_size*nkernels; i++) {
			if(i%kernel_size==0 && i!=0)
				printf("\n");
			if(i%(kernel_size*kernel_size)==0)
				printf("\n\n");
			kernel[i]=rand() % 10 - 5;
			printf("%d\t",kernel[i]);
		}
	}
	else
	{
		for(i=0; i<input_size; i++) {
			input[i]=rand() % 50 - 25;
		}
		cout << "\nRandomized Kernel " << kernel_size << "x" << kernel_size << "\n";
		for(i=0; i<kernel_size*kernel_size*nkernels; i++) {
			kernel[i]=rand() % 10 - 5;
		}
	}
    clock_t start, end;
	uint32_t addr_B=0,addr_res=0;
	printf("\nVERSAT TEST \n\n");
	CVersat versat;
	start = clock();
    versat.versat_init(VERSAT);
    end = clock();
	printf("Deep versat initialized in %ld us\n", (end - start));
	for (i = 0; i < input_size; i++)
	{
		FPGA_mem[i] = input[i];
		addr_B++;
	}
	int addr_exp=addr_B;
	for (i = 0; i < kernel_size*kernel_size*nkernels; i++)
	{
		FPGA_mem[i+addr_B] = kernel[i];
		addr_exp++;
	}
	addr_res=addr_exp+out_w*out_h*nkernels;
	cout << "Input ADDR=" << 0 << "\n";
	cout << "Kernel ADDR=" << addr_B<< "\n";
	cout << "Expected Result ADDR=" << addr_exp << "\n";
	cout << "Actual Result ADDR=" << addr_res << "\n";
	versat_t acc;
	for(int z=0; z<nkernels; z++)
	{
		for (i = 0; i < out_h ; i++)
		{
			for (j = 0; j < out_w; j++)
			{
				acc=0;
				for(int n = 0; n < channels; n++)
				{
					for(int l=0; l<kernel_size; l++)
						{
							for(int k=0;k<kernel_size;k++)
								{
									acc+=FPGA_mem[(i*width*stride+j*stride)+(k+l*width)+n*width*height]*FPGA_mem[l*kernel_size+k+addr_B+z*kernel_size*kernel_size];
								}					
						}
				}
				FPGA_mem[addr_exp+i*(out_w)+j+z*out_w*out_h]=acc;
				if(width<10)
				printf("%d\t",FPGA_mem[addr_exp+i*(out_w)+j+z*out_w*out_h]);
			}
			if(width<10)
			printf("\n");
		}
		if(width<10)
		printf("\n\n");
	}

	convolutional_layer_xyz(&versat.stage[0],0,channels,height,width,kernel_size,stride,pad,addr_B*(DATAPATH_W / 8),addr_res*(DATAPATH_W / 8),nkernels);
	//print_versat_config();



	versat.versat_debug=0;

	start = clock();
	cout << "Start Test ---------------------------\n";
    versat.run();
    while (versat.done() == 0)
        ;
	cout << "Data Loaded into Versat---------------------------\n";
    //print_versat_info();
    versat.globalClearConf();

    versat.run();
    while (versat.done() == 0)
        ;
	cout << "Versat finished the run---------------------------\n";
	int aux_versat_iter=versat.versat_iter;
    versat.run();
    //print_versat_info();

    while (versat.done() == 0)
        ;
	cout << "Data Written into Memory---------------------------\n";
    end = clock();
    //print_versat_info();
    printf("\nMatrix Multiplication done in %ld us\n", (end - start));
    printf("Simulation took %d Versat Clock Cycles\n", aux_versat_iter);

    //display results
    printf("\nCheck Result for Errors:\n");
	for(int z=0;z<nkernels;z++)
	{
		for (i = 0; i < out_h; i++)
		{
			for (j = 0; j < out_w; j++)
			printf("%d\t",(int16_t)FPGA_mem[addr_res+j+i*(out_w)+z*out_w*out_h]);
			printf("\n");
		}
		printf("\n\n");
	}
for (int k = 0; k < nkernels; k++)
{
	for(i=0;i<out_h;i++)
	{
		for(j=0;j<out_w;j++)
		{
			if(FPGA_mem[addr_exp+k*out_w*out_h+i*out_w+j]!=FPGA_mem[addr_res+k*out_w*out_h+i*out_w+j])
			{
				cout << "ERROR: Wrong Value at (" << i << "," << j <<","<<k<<")\n";
				cout << "Expected=" << FPGA_mem[addr_exp+k*out_w*out_h+i*out_w+j] << "\n";
				cout << "Actual=" << FPGA_mem[addr_res+k*out_w*out_h+i*out_w+j] << "\n";
			}		
		}
	}
}
	versat.free_mem();
	return 0;
}