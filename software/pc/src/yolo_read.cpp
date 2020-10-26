#include "yolo_read.hpp"

CYoloRead::CYoloRead() {}

CYoloRead::CYoloRead(int base,versat_t*databus)
{
    this->base=base;
    for (int i = 0; i < nYOLOvect; i++)
    {
        kernels[i]=CRead(base, i, kernel_mem[i], 1 << WEIGHT_ADDR_W,2,&databus[i+1]);
    }
    this->bias = CRead(base, nYOLOvect + 1, bias_mem, 1 << BIAS_ADDR_W,2,&databus[0]);
    this->databus = databus;
}

void CYoloRead::setExtAddr(int extAddr)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setExtAddr(extAddr);
}
#if YOLO_VERSAT == 1
void CYoloRead::setPingPong(int pp)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setPingPong(pp);
}
#endif
void CYoloRead::setIntAddr(int intAddr)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntAddr(intAddr);
}
void CYoloRead::setExtIter(int iter)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setExtIter(iter);
}
void CYoloRead::setExtPer(int per)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setExtPer(per);
}
void CYoloRead::setExtShift(int shift)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setExtShift(shift);
}
void CYoloRead::setExtIncr(int incr)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setExtIncr(incr);
}
void CYoloRead::setIntStart(int start)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntStart(start);
}
void CYoloRead::setIntIter(int iter)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntIter(iter);
}
void CYoloRead::setIntPer(int per)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntPer(per);
}
void CYoloRead::setIntShift(int shift)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntShift(shift);
}
void CYoloRead::setIntIncr(int incr)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].setIntIncr(incr);
}
void CYoloRead::copy(CYoloRead that)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].copy(that.kernels[i]);
    bias.copy(that.bias);
}
void CYoloRead::copy_ext(CYoloRead that)
{
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].copy_ext(that.kernels[i]);
    bias.copy_ext(that.bias);
}

void CYoloRead::output() {
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].output();
    bias.output();
}
void CYoloRead::update() {
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].update();
    bias.update();
}
void CYoloRead::start_run() {
    for (int i = 0; i < nYOLOvect; i++)
        kernels[i].start_run();
    bias.start_run();
}
bool CYoloRead::done() {
    bool a = bias.done;
    for (int i = 0; i < nYOLOvect; i++)
        a = a && kernels[i].done;
    return a;
}

void CYoloRead::copy_read_config(CYoloRead that)
{
    for(int i=0; i< nYOLOvect; i++)
        kernels[i].copy_ext(that.kernels[i]);
    bias.copy_ext(that.bias);
}
