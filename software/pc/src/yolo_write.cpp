#include "yolo_write.hpp"

CRead_vec::CRead_vec()
{
}

//Constructor with an associated base
CRead_vec::CRead_vec(int versat_base, versat_t *databus)
{
    this->versat_base = versat_base;
    this->databus = databus;
    for (int i = 0; i < nSTAGE; i++)
        rmem[i] = CRead(versat_base, i, mem, 1 << PIXEL_ADDR_W, 3, &databus[i]);
}

//Methods to set config parameters
void CRead_vec::setExtAddr(int extAddr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setExtAddr(extAddr);
}
void CRead_vec::setOffset(int offset)
{
    this->offset = offset;
}
void CRead_vec::setIntAddr(int intAddr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntAddr(intAddr);
}
void CRead_vec::setExtIter(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setExtIter(iter);
}
void CRead_vec::setExtPer(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setExtPer(per);
}
void CRead_vec::setExtShift(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setExtShift(shift);
}
void CRead_vec::setExtIncr(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setExtIncr(incr);
}
void CRead_vec::setIntStart(int start)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntStart(start);
}
void CRead_vec::setIntIter(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIter(iter);
}
void CRead_vec::setIntPer(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntPer(per);
}
void CRead_vec::setIntShift(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntShift(shift);
}
void CRead_vec::setIntIncr(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIncr(incr);
}

// 4 loop

void CRead_vec::setIntIter2(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIter2(iter);
}
void CRead_vec::setIntPer2(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntPer2(per);
}
void CRead_vec::setIntShift2(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntShift2(shift);
}
void CRead_vec::setIntIncr2(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIncr2(incr);
}

// 6 loop

void CRead_vec::setIntIter3(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIter3(iter);
}
void CRead_vec::setIntPer3(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntPer3(per);
}
void CRead_vec::setIntShift3(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntShift3(shift);
}
void CRead_vec::setIntIncr3(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].setIntIncr3(incr);
}

void CRead_vec::copy(CRead_vec that)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].copy(that.rmem[i]);
}
void CRead_vec::copy_ext(CRead_vec that)
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].copy_ext(that.rmem[i]);
}

void CRead_vec::output()
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].output();
}
void CRead_vec::update()
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].update();
}
void CRead_vec::start_run()
{
    for (int i = 0; i < nSTAGE; i++)
        rmem[i].start_run();
}

bool CRead_vec::done()
{
    for (int i = 0; i < nSTAGE; i++)
        if (rmem[i].done == false)
            return false;
    return true;
}

CWrite_vec::CWrite_vec()
{
}

//Constructor with an associated base
CWrite_vec::CWrite_vec(int versat_base, versat_t *databus)
{
    this->versat_base = versat_base;
    this->databus = databus;
}

//Methods to set config parameters
void CWrite_vec::setExtAddr(int extAddr)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setExtAddr(extAddr);
}
void CWrite_vec::setOffset(int offset)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setOffset(offset);
}

void CWrite_vec::setIntAddr(int intAddr)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntAddr(intAddr);
}
void CWrite_vec::setExtIter(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setExtIter(iter);
}
void CWrite_vec::setExtPer(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setExtPer(per);
}
void CWrite_vec::setExtShift(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setExtShift(shift);
}
void CWrite_vec::setExtIncr(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setExtIncr(incr);
}
void CWrite_vec::setIntStart(int start)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntStart(start);
}
void CWrite_vec::setIntDuty(int duty)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntDuty(duty);
}
void CWrite_vec::setIntDelay(int delay)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntDelay(delay);
}
void CWrite_vec::setIntIter(int iter)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntIter(iter);
}
void CWrite_vec::setIntPer(int per)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntPer(per);
}
void CWrite_vec::setIntShift(int shift)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntShift(shift);
}
void CWrite_vec::setIntIncr(int incr)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].setIntIncr(incr);
}

void CWrite_vec::copy(CWrite_vec that)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].copy(that.wmem[i]);
}
void CWrite_vec::copy_ext(CWrite_vec that)
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].copy_ext(that.wmem[i]);
}
void CWrite_vec::output()
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].output();
}
void CWrite_vec::update()
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].update();
}
void CWrite_vec::start_run()
{
    for (int i = 0; i < nSTAGE; i++)
        wmem[i].start_run();
}
void CWrite_vec::reset()
{
}
bool CWrite_vec::done()
{
    for (int i = 0; i < nSTAGE; i++)
        if (wmem[i].done == false)
            return false;
    return true;
}


string CYoloWrite::info() { return NULL; }
string CYoloWrite::info_iter() { return NULL; }

CYoloWrite::CYoloWrite() {}
CYoloWrite::CYoloWrite(int base, versat_t *databus)
{
    this->base = base;
    this->read = CRead_vec(base, databus_read_yolo); // databus from read to write
    for (int i = 0; i < nSTAGE; i++)
    {
        this->write[i] = CWrite_vec(base, &databus_yolo_write[i][0]);                                             // databus from yolo to write,
        this->yolo[i] = CYolo_vec(base, databus, &databus[1], &databus_read_yolo[i], &databus_yolo_write[i][0]); // here it must use databus BIAS, internal databus AND databus from yolo_read
    }
    this->databus = databus;
}
void CYoloWrite::output()
{
    read.output();
    for (int i = 0; i < nSTAGE; i++)
    {
        write[i].output();
        yolo[i].output();
    }
}
void CYoloWrite::update()
{
    read.update();
    for (int i = 0; i < nSTAGE; i++)
    {
        write[i].update();
        yolo[i].update();
    }
}
void CYoloWrite::start_run()
{
    read.start_run();
    for (int i = 0; i < nSTAGE; i++)
    {
        write[i].start_run();
        yolo[i].start_run();
    }
}
void CYoloWrite::reset() {
    for(int i = 0; i < nSTAGE; i++)
    {
        databus_read_yolo[i]=0;
        for(int j = 0; j < nYOLOvect; j++)
            databus_yolo_write[i][j]=0;
    }
}
bool CYoloWrite::done()
{
    bool a = read.done();
    for (int i = 0; i < nSTAGE; i++)
    {
        a = a && write[i].done();
        a = a && yolo[i].done();
    }
    return a;
}

void CYoloWrite::copy(CYoloWrite that)
{
    
}



void CYoloWrite::copy_write_config(CYoloWrite that){
    for (int i = 0; i < nSTAGE; i++)
        write[i].copy_ext(that.write[i]);
}
void CYoloWrite::copy_read_config(CYoloWrite that){
    read.copy_ext(that.read);
}
