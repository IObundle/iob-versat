#include "versat.hpp"
//The point of this file is to store configurations

/*
simd_mult(versat_t* vector_a, int width,versat_t* vector_b)
{

}

simd_add(versat_t* vector_a,int width,versat_t* vector_b)
{
	
}
*/
int alu_operation(CStage* obj,int op_a,int op_b,int index, int alu_index,int operation) // alu_index 0 for ALU, 1 for ALU_LITE
{
	if (alu_index == 0)
	{
		obj->alu[index].setOpA(op_a);
		obj->alu[index].setOpB(op_b);
		obj->alu[index].setFNS(operation);
		return sALU[index];
	}
	else
	{
		obj->alulite[index].setOpA(op_a);
		obj->alulite[index].setOpB(op_b);
		obj->alulite[index].setFNS(operation);
		return sALULITE[index];
	}

	return sALU[index];
}


int mul_operation(CStage* obj, int op_a, int op_b, int index, int operation)
{
		obj->mul[index].setSelA(op_a);
		obj->mul[index].setSelB(op_b);
		obj->mul[index].setFNS(operation);
		return sMUL[index];
}

int muladd_operation(CStage* obj, int op_a, int op_b, int index, int operation,int iter,int per, int delay, int shift)
{
	//  acumulates till end of PER, ITER repeats X times
	obj->muladd[index].setSelA(op_a);
	obj->muladd[index].setSelB(op_b);
	obj->muladd[index].setFNS(operation);
	obj->muladd[index].setIter(iter);
	obj->muladd[index].setPer(per);
	obj->muladd[index].setDelay(delay);
	return sMULADD[index];
}

int bs_operation(CStage* obj, int op_a, int shift, int index, int operation)
{
	obj->bs[index].setData(op_a);
	obj->bs[index].setFNS(operation);
	obj->bs[index].setShift(shift);
	return sBS[index];
}

int set_ExtMem_Read(CStage* obj,int index,Acumulator ext_loop)
{
	obj->vi[index].setExtIter(ext_loop.iter);
	obj->vi[index].setExtPer(ext_loop.per);
	obj->vi[index].setExtIncr(ext_loop.incr);
	obj->vi[index].setExtShift(ext_loop.shift);
	obj->vi[index].setExtAddr(ext_loop.extAddr);
	obj->vi[index].setIntAddr(ext_loop.intAddr);
	return sVI[index];
}

int set_IntMem_Read(CStage* obj,int index,Acumulator loop)
{
	switch(loop.nloops)
	{
		case 6:
		case 5:loop.incr2+=loop.shift*loop.iter+(loop.incr*loop.per)*loop.iter; // 4 + 2*2 = 8
			   loop.incr3+=loop.shift2*loop.iter2+(loop.incr2*loop.per2)*loop.iter2;
			   cout << "\nreal values of incr2 are " << loop.incr2 << " and incr3 is " << loop.incr3 << "\n";
				break;
		case 4:
		case 3:loop.incr2+=loop.shift*loop.iter+(loop.incr*loop.per)*loop.iter;
				cout << "\nreal values of incr2 are " << loop.incr2; 
		default : break;
	}
	obj->vi[index].setIntIter(loop.iter);
	obj->vi[index].setIntPer(loop.per);
	obj->vi[index].setIntIter2(loop.iter2);
	obj->vi[index].setIntPer2(loop.per2);
	obj->vi[index].setIntIter3(loop.iter3);
	obj->vi[index].setIntPer3(loop.per3);
	obj->vi[index].setIntIncr(loop.incr);
	obj->vi[index].setIntIncr2(loop.incr2);
	obj->vi[index].setIntIncr3(loop.incr3);
	obj->vi[index].setIntShift(loop.shift);
	obj->vi[index].setIntShift2(loop.shift2);
	obj->vi[index].setIntShift3(loop.shift3);
	obj->vi[index].setDuty(loop.duty);
	obj->vi[index].setDelay(loop.delay);
	obj->vi[index].setIntStart(loop.start);
	return sVI[index];
}


int set_IntMem_Write(CStage* obj,int index,Acumulator loop,int sel)
{
	//Each pair of loops is an acumulator.
	//The pairs work independently, so when going to use the 3rd loop, the iterator is reset to 0
	//this is not intended on this specific API
	//we want to iterate over data without being reset to loop.Start;
	switch(loop.nloops)
	{
		case 6:
		case 5:loop.incr2+=loop.shift*loop.iter+(loop.incr*loop.per)*loop.iter; // 4 + 2*2 = 8
			   loop.incr3+=loop.shift2*loop.iter2+(loop.incr2*loop.per2)*loop.iter2;
			   cout << "\nreal values of incr2 are " << loop.incr2 << " and incr3 is " << loop.incr3 << "\n";
				break;
		case 4:
		case 3:loop.incr2+=loop.shift*loop.iter+(loop.incr*loop.per)*loop.iter;
				cout << "\nreal values of incr2 are " << loop.incr2; 
		default : break;
	}
	obj->vo[index].setIntIter(loop.iter);
	obj->vo[index].setIntPer(loop.per);
	obj->vo[index].setIntIter2(loop.iter2);
	obj->vo[index].setIntPer2(loop.per2);
	obj->vo[index].setIntIncr(loop.incr);
	obj->vo[index].setIntIncr2(loop.incr2);
	obj->vo[index].setIntShift(loop.shift);
	obj->vo[index].setIntShift2(loop.shift2);
	obj->vo[index].setDuty(loop.duty);
	obj->vo[index].setDelay(loop.delay);
	obj->vo[index].setIntStart(loop.start);
	obj->vo[index].setSel(sel);


	return 0;
}

int set_ExtMem_Write(CStage* obj,int index,Acumulator ext_loop)
{
	obj->vo[index].setExtIter(ext_loop.iter);
	obj->vo[index].setExtPer(ext_loop.per);
	obj->vo[index].setExtIncr(ext_loop.incr);
	obj->vo[index].setExtShift(ext_loop.shift);
	obj->vo[index].setExtAddr(ext_loop.extAddr);
	obj->vo[index].setIntAddr(ext_loop.intAddr);
	return 0;
}




/************************************************************************
 * matrix_mult
 * description:
 * Creates the Configurations for a Stage to perform a Matrix Multiplication between Matrix A and Matrix B.
 * Both Matrixes must have a size lower than half of MEM_SIZE to fit inside each Memory.
 * Parameters:
 *		input	versat	-	CStage Object that contains the stage to implement this matrix_mult.
 *		input	matrix_a	-	Matrix A
 *		input	matrix_b	-	Matrix B
 *		input	r_a,r_b	-	Row Number of Matrix A and B
 * 		input	c_a,c_b	-	Column Number of Matrix A and B	
 * 		input	store	-	True = store Matrix Multiplication in Memory and then
 * returns: Position in the Databus to get Results of Matrix Multiplication
 ************************************************************************/
int matrix_mult(CStage* versat,int matrix_a,int matrix_b,int result_matrix,int r_a,int c_a,int r_b,int c_b,bool store)
{
	Acumulator store_matrix_A= Acumulator();
	Acumulator store_matrix_B= Acumulator();

/*
	if(versat_debug==1)
	{
		if(c_a!=r_b)
			printf("ERROR: MATRIX_MULT NOT POSSIBLE\n");
		if(nMULADD<1 || nVI < 2 || nVO < 1 || nSTAGE < 1)
			printf("ERROR: NOT ENOUGH RESOURCES FOR MATRIX_MULT");
		if(r_a*c_a> MEM_SIZE/2)
			printf("ERROR: READ MEMORY IS TOO SMALL\nNEEDED SIZE:%d\nACTUAL SIZE:%d",r_a*c_a,MEM_SIZE/2);
		if(r_b*c_b> MEM_SIZE/2)
			printf("ERROR: READ MEMORY IS TOO SMALL\nNEEDED SIZE:%d\nACTUAL SIZE:%d",r_b*c_b,MEM_SIZE/2);
	}
*/
	store_matrix_A.add_loop(r_a*c_a);
	store_matrix_A.loop_settings(0,0,0,matrix_a,0);
	store_matrix_B.add_loop(r_b*c_b);
	store_matrix_B.loop_settings(0,0,0,matrix_b,0);

	
	set_ExtMem_Read(versat,0,store_matrix_A);
	set_ExtMem_Read(versat,1,store_matrix_B);

	Acumulator read_matrix_A = Acumulator();
	Acumulator read_matrix_B = Acumulator();

		read_matrix_A.add_loop(r_a,c_a);
			read_matrix_A.add_loop(c_b,-c_a);
				read_matrix_A.add_loop(c_a,1);
					read_matrix_A.loop_settings(0);
					set_IntMem_Read(versat,0,read_matrix_A);

		read_matrix_B.add_loop(r_a,-c_b);
			read_matrix_B.add_loop(c_b,-r_b*c_b+1);
				read_matrix_B.add_loop(r_b,c_b);
					read_matrix_B.loop_settings(0);
					set_IntMem_Read(versat,1,read_matrix_B);


	muladd_operation(versat,sVI[0],sVI[1],0,MULADD_MACC,r_a*c_b,c_a,MEMP_LAT,0);


	if(store==true)
	{
		Acumulator write_matrix = Acumulator();
		write_matrix.add_loop(r_a*c_b,1);
		write_matrix.loop_settings(0,0,MEMP_LAT+MULADD_LAT,result_matrix,0);
		set_ExtMem_Write(versat,0,write_matrix);
			write_matrix.add_loop(c_a,0);
			set_IntMem_Write(versat,0,write_matrix,sMULADD[0]);
	}



	return sMULADD[0];
}

int dot_product(CStage* versat,int vector_a,int vector_b,int result,int length,bool store)
{
	Acumulator store_matrix_A= Acumulator();
	Acumulator store_matrix_B= Acumulator();

	store_matrix_A.add_loop(length);
	store_matrix_A.loop_settings(0,0,0,vector_a,0);
	store_matrix_B.add_loop(length);
	store_matrix_B.loop_settings(0,0,0,vector_b,0);

	Acumulator read_matrix_A = Acumulator();
	Acumulator read_matrix_B = Acumulator();

	read_matrix_A.add_loop(length);
	read_matrix_B.add_loop(length);

	muladd_operation(versat,sVI[0],sVI[1],0,MULADD_MACC,1,length,MEMP_LAT,0);


	if(store==true)
	{
		Acumulator write_matrix = Acumulator();
		write_matrix.add_loop(length,1);
		write_matrix.loop_settings(0,0,MEMP_LAT+MULADD_LAT,result,0);
		set_ExtMem_Write(versat,0,write_matrix);
			write_matrix.add_loop(length,0);
			set_IntMem_Write(versat,0,write_matrix,sMULADD[0]);
	}

	return sMULADD[0];
}
/*	numer of filters
	output dimension
	filter dimension * number of collors,
	weights
	workspace
	output
*/
int load_data(CStage* versat,int index,int addr,int size)
{
	Acumulator load= Acumulator();
	load.add_loop(size);
	load.loop_settings(0,0,0,addr,0);
	set_ExtMem_Read(versat,index,load);
	return index;
}
int write_data(CStage* versat,int index,int DDR_addr,int size,int versat_addr)
{
	Acumulator load= Acumulator();
	load.add_loop(size);
	load.loop_settings(0,0,0,DDR_addr,versat_addr);
	set_ExtMem_Write(versat,index,load);
	return index;
}


void pad_input(int pad,int new_addr,int input_addr,int channels,int height,int width)
{
	int cur=0;
	for(int i=0;i<channels;i++)
		for(int j=0;j<height+2*pad;j++)
			for(int k=0;k<width+2*pad;k++)
			{
				if(k>=width+pad || j>=height+pad || k<pad || j<pad)
					cur=0;
				else
					cur=FPGA_mem[input_addr+(k-pad)+(j-pad)*width+i*height*width];
			FPGA_mem[new_addr+k+j*(width+pad)+i*(height+pad)*(width+pad)] = cur;
			}
}
//makes it easier to compute convolutions
void xyz_to_zxy(int pad,int new_addr,int input_addr,int channels,int height,int width)
{
		for(int j=0;j<height+2*pad;j++)
			for(int k=0;k<width+2*pad;k++)
				for(int i=0;i<channels;i++)
				{
					FPGA_mem[new_addr+i+k*(channels)+j*(channels)*(width+pad)]=FPGA_mem[input_addr+k+j*width+i*height*width];
				}
}
int convolutional_layer_xyz(CStage* versat,int input_addr,int channels,int height, int width, int kernel_size, int stride, int pad,int weights_addr,int output_addr,int nkernels)
{
	Acumulator weights = Acumulator();
	Acumulator input = 	Acumulator();
	int new_addr=input_addr+channels*height*width+1;
	int out_w=(width + 2*pad - kernel_size) / stride + 1;
	int out_h=(height + 2*pad - kernel_size) / stride + 1;
	int in_w=width+pad;
	int in_h=height+pad;
	int rewind_kernel=-kernel_size*kernel_size;
	int	line_plus_one = in_w-kernel_size;
	bool padded=true;
	if(padded==false)
		{
			pad_input(pad,new_addr,input_addr,channels,height,width);
			input_addr=new_addr;
		}

	load_data(versat,0,input_addr,channels*in_h*in_w);
	load_data(versat,1,weights_addr,kernel_size*kernel_size*nkernels);
	//calcuclar quantos outputs cabem na MEM_SIZE/2, Minimo Channels*k_size*k_size, fazer varias runs, para calcular as linhas todas
	input.add_loop(out_h,kernel_size-stride);
		input.add_loop(out_w,-channels*in_w*in_h+stride);
			input.add_loop(channels, in_w*in_h-line_plus_one*kernel_size+rewind_kernel); // Posição final x8 depois dos 2 loops -8+16
				cout << "Position after Input finished Accumulator is " << in_w*in_h-line_plus_one*kernel_size+rewind_kernel;
				input.add_loop(kernel_size,line_plus_one);
					input.add_loop(kernel_size,1);
						input.loop_settings(0);
						set_IntMem_Read(versat,0,input);
						cout << "\nInput has nloops=" << (int)input.nloops << " \n";

	weights.add_loop(out_h,0);
		weights.add_loop(out_w,0);
			weights.add_loop(channels,rewind_kernel);
			cout << "Position after Weight finished Accumulator is " << rewind_kernel;
				weights.add_loop(kernel_size,0);
					weights.add_loop(kernel_size,1);
						weights.loop_settings(0);
					set_IntMem_Read(versat,1,weights);
						cout << "\nWeights has nloops=" << (int)weights.nloops << " \n";


	muladd_operation(versat,sVI[0],sVI[1],0,MULADD_MACC,out_h*out_w,kernel_size*kernel_size*channels,MEMP_LAT,0);

		Acumulator write_matrix = Acumulator();
		write_matrix.add_loop(out_h*out_w,1);
		write_matrix.loop_settings(0,0,MEMP_LAT+MULADD_LAT,output_addr,0);
		set_ExtMem_Write(versat,0,write_matrix);
			write_matrix.add_loop(kernel_size*kernel_size*channels,0);
			set_IntMem_Write(versat,0,write_matrix,sMULADD[0]);
	return 0;
}


