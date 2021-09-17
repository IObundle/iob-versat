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
	versat_t input_A[4]={1,2,3,4};
	versat_t input_B[6]={5,6,7,8,9,10};
	versat_t expected[6]={21,24,27,47,54,61};
    clock_t start, end;
	uint32_t addr_B=0,addr_res=0;
	printf("\nVERSAT TEST \n\n");
	CVersat versat;
	start = clock();
    versat.versat_init(VERSAT);
    end = clock();
	printf("Deep versat initialized in %ld us\n", (end - start));
	printf("\nExpected result of Matrix Multiplication\n");
	for (i = 0; i < 4; i++)
	{
		FPGA_mem[i] = input_A[i];
		addr_B++;
	}
	addr_res=addr_B;
	for (i = 0; i < 6; i++)
	{
		FPGA_mem[i+addr_B] = input_B[i];
		addr_res++;
	}
	
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 3; j++)
        {
            printf("%d\t", expected[i*3+j]);
        }
        printf("\n");
    }

	matrix_mult(&versat.stage[0],0,addr_B*(DATAPATH_W / 8),addr_res*(DATAPATH_W / 8),2,2,2,3,true);
	//print_versat_config();
	versat.versat_debug=0;

	start = clock();
    versat.run();
    while (versat.done() == 0)
        ;
    //print_versat_info();
    versat.globalClearConf();

    versat.run();
    while (versat.done() == 0)
        ;
	int aux_versat_iter=versat.versat_iter;
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
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 3; j++)
           printf("%d\t",(int16_t)FPGA_mem[addr_res+j+i*3]);
        printf("\n");
    }
	return 0;
}