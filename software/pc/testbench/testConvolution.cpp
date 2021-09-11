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
	versat_t input[16]={0};
	time_t t;
	srand((unsigned) time(&t));
	cout << "Testing Convolution Layer xyz on Deep Versat\n";
	cout << "Randomized Input 4x4\n";
	for(i=0; i<16; i++) {
		if(i%4==0 && i!=0)
			printf("\n");
		input[i]=rand() % 50 - 25;
		printf("%d\t",input[i]);
	}
	versat_t kernel[4]={0};
	cout << "\nRandomized Kernel 2x2\n";
	for(i=0; i<4; i++) {
		if(i%2==0 && i!=0)
			printf("\n");
		kernel[i]=rand() % 10 - 5;
		printf("%d\t",kernel[i]);
	}
    clock_t start, end;
	uint32_t addr_B=0,addr_res=0;
	printf("\nVERSAT TEST \n\n");
	CVersat versat;
	start = clock();
    versat.versat_init(VERSAT);
    end = clock();
	printf("Deep versat initialized in %ld us\n", (end - start));
	printf("\nExpected result of Convolution 3x3\n");
	for (i = 0; i < 16; i++)
	{
		FPGA_mem[i] = input[i];
		addr_B++;
	}
	int addr_exp=addr_B;
	for (i = 0; i < 4; i++)
	{
		FPGA_mem[i+addr_B] = kernel[i];
		addr_exp++;
	}
	addr_res=addr_exp+9;
	int kernel_size=2;
	int channels = 1;
	int nkernels = 1;
	int height = 4;
	int width = 4;
	int stride = 1;
	int pad = 0;
	versat_t acc;

    for (i = 0; i < (height + 2*pad - kernel_size) / stride + 1 ; i++)
    {
        for (j = 0; j < (width + 2*pad - kernel_size) / stride + 1; j++)
        {
			acc=0;
			for(int l=0; l<kernel_size; l++)
				for(int k=0;k<kernel_size;k++)
					acc+=FPGA_mem[(i*width+j)+(k+l*width)]*FPGA_mem[l*kernel_size+k+addr_B];
			FPGA_mem[addr_exp+i*((width + 2*pad - kernel_size) / stride + 1)+j]=acc;
			printf("%d\t",FPGA_mem[addr_exp+i*((width + 2*pad - kernel_size) / stride + 1)+j]);
        }
        printf("\n");
    }
	convolutional_layer_xyz(&versat.stage[0],0,channels,height,width,kernel_size,stride,pad,addr_B*(DATAPATH_W / 8),addr_res*(DATAPATH_W / 8),nkernels);
	//print_versat_config();
	versat.versat_debug=1;

	start = clock();
    versat.run();
    while (versat.done() == 0)
        ;
	int aux_versat_iter=versat.versat_iter;
    //print_versat_info();
    versat.globalClearConf();

    versat.run();
    while (versat.done() == 0)
        ;
    versat.run();
    //print_versat_info();

    while (versat.done() == 0)
        ;
    end = clock();
    //print_versat_info();
    printf("\nMatrix Multiplication done in %ld us\n", (end - start));
    printf("Simulation took %d Versat Clock Cycles\n", aux_versat_iter);

    //display results
    printf("\nActual Matrix result\n");
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
           printf("%d\t",(int16_t)FPGA_mem[addr_res+j+i*3]);
        printf("\n");
    }
	return 0;
}