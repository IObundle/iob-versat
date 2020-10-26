#include "yolo.hpp"

CYolo_vec::CYolo_vec() {}

CYolo_vec::CYolo_vec(int versat_base,versat_t* databus_bias, versat_t*databus_read_kernel,versat_t* databus_read_yolo,versat_t* databus_yolo_write) {
    this->versat_base =versat_base;
    this->databus_bias=databus_bias;
    this->databus_read = databus_read_kernel;
    this->databus_read_yolo = databus_read_yolo;
    this->databus_yolo_write = databus_yolo_write;
    for(int i=0;i<nYOLOvect; i++)
        mac[i]=CYolo(i,databus_bias,&databus_read[i],databus_read_yolo,&databus_yolo_write[i]);
}

void CYolo_vec::setIter(int iter)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setIter(iter);
}
void CYolo_vec::setPer(int per)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setPer(per);
}
void CYolo_vec::setShift(int shift)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setShift(shift);
}
void CYolo_vec::setBias(int bias)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setBias(bias);
}
void CYolo_vec::setLeaky(int leaky)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setLeaky(leaky);
}
void CYolo_vec::setMaxpool(int maxpool)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setMaxpool(maxpool);
}
void CYolo_vec::setBypass(int bypass)
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].setBypass(bypass);
}

void CYolo_vec::start_run()
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].start_run();
}

void CYolo_vec::update()
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].update();
}

versat_t CYolo_vec::output()
{
    for (int i = 0; i < nYOLOvect; i++)
        mac[i].output();
    return 0;
}

bool CYolo_vec::done()
{
    bool a = mac[0].done;
    for (int i = 1; i < nYOLOvect; i++)
        a = a && mac[i].done;
    return a;
}
CYolo::CYolo() {}

CYolo:: CYolo(int base,versat_t *bias,versat_t*databus_kernel,versat_t *databus_pixels,versat_t *databus_write){
    this->base =base;
    this->databus_bias=bias;
    this->databus_kernel=databus_kernel;
    this->databus_pixel=databus_pixels;
    this->databus_out=databus_write;
}


void CYolo::setIter(int iter)
{
    this->iter = iter;
}
void CYolo::setPer(int per)
{
    this->per = per;
}
void CYolo::setShift(int shift)
{
    this->shift = shift;
}
void CYolo::setBias(int bias)
{
    this->bias = bias;
}
void CYolo::setLeaky(int leaky)
{
    this->leaky = leaky;
}
void CYolo::setMaxpool(int maxpool)
{
    this->maxpool = maxpool;
}
void CYolo::setBypass(int bypass)
{
    this->bypass = bypass;
}

void CYolo::start_run()
{
}

void CYolo::update()
{
    //trickle down all outputs in buffer
    versat_t aux_output = output_buff[0];
    versat_t aux_output2 = 0;
    //trickle down all outputs in buffer
    for (int i = 1; i < MULADD_LAT; i++)
    {
        aux_output2 = output_buff[i];
        output_buff[i] = aux_output;
        aux_output = aux_output2;
    }
}

versat_t CYolo::output()
{
    //wait for delay to end
    if (run_delay > 0)
    {
        return 0;
    }

    //select inputs
    opa = databus[0];
    opb = databus[1];
    bias = databus[2] << (sizeof(mul_t) - sizeof(versat_t) / 2);

    cnt_addr = acumulator();

    //select acc_w value
    acc_w = (cnt_addr == 0) ? bias : acc;

    //perform MAC operation
    mul_t result_mult = opa * opb;
    mul_t result_add = result_mult + acc_w;
    out = (versat_t)(acc >> shift);

    out = bypass ? opa : out;
    /* do maxpool 
    if (maxpool)
    {
        if (out > max)
            max = out;
    }
    */

    /* do ACTIVATION FUNCS
    if (leaky == 1)
        out = out >> leaky_shift;
    else if (sigmoid == 1)
        out = out =
    else
        out=out;
    */
    return out;
}

uint32_t CYolo::acumulator()
{
    if (loop2 < iter)
    {
        if (loop1 < per)
        {
            loop1++;
            aux = pos;
        }
        if (loop1 == per)
        {
            loop1 = 0;
            loop2++;
            pos += shift;
        }
    }
    if (loop2 == iter)
    {
        loop2 = 0;
        done = 1;
    }
    return aux;
}


