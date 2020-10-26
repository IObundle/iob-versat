#include "vread.hpp"
#include "yolo.hpp"
#include "vwrite.hpp"
#include "type.hpp"
class CRead_vec
{
private:versat_t*databus=NULL;

public:
    CRead rmem[nSTAGE];
    versat_t mem[1 << PIXEL_ADDR_W];
    int versat_base = 0;
    int offset = 0;
    CRead_vec();
    CRead_vec(int versat_base,versat_t*databus);

    void setExtAddr(int extAddr);
    void setOffset(int offset);
    void setIntAddr(int intAddr);
    void setExtIter(int iter);
    void setExtPer(int per);
    void setExtShift(int shift);
    void setExtIncr(int incr);
    void setIntStart(int start);
    void setIntIter(int iter);
    void setIntPer(int per);
    void setIntShift(int shift);
    void setIntIncr(int incr);
    void setIntIter2(int iter);
    void setIntPer2(int per);
    void setIntShift2(int shift);
    void setIntIncr2(int incr);
    void setIntIter3(int iter);
    void setIntPer3(int per);
    void setIntShift3(int shift);
    void setIntIncr3(int incr);

    //SIM
    void copy(CRead_vec that);
    void copy_ext(CRead_vec that);

    //set run start
    void start_run();

    //set update output buffers
    void update();

    //calculate new output
    void output();

    string info();
    string info_iter();

    bool done();
    void reset();
};
class CWrite_vec
{
private:
versat_t*databus=NULL;


public:
    int versat_base;
    int offset = 0;
    CWrite wmem[nSTAGE];

    //Default constructor
    CWrite_vec();

    //Constructor with an associated base
    CWrite_vec(int versat_base,versat_t*databus);

    //Methods to set config parameters
    void setExtAddr(int extAddr);
    void setOffset(int offset);
    void setIntAddr(int intAddr);
    void setExtIter(int iter);
    void setExtPer(int per);
    void setExtShift(int shift);
    void setExtIncr(int incr);
    void setIntStart(int start);
    void setIntDuty(int duty);
    void setIntDelay(int delay);
    void setIntIter(int iter);
    void setIntPer(int per);
    void setIntShift(int shift);
    void setIntIncr(int incr);

    //SIM
    void copy(CWrite_vec that);
    void copy_ext(CWrite_vec that);
    //set run start
    void start_run();

    //set update output buffers
    void update();

    //calculate new output
    void output();

    string info();
    string info_iter();

    bool done();
    void reset();
};
class CYoloWrite
{
private:
versat_t*databus=NULL;
public:
versat_t databus_read_yolo[nSTAGE]={0};
versat_t databus_yolo_write[nSTAGE][nYOLOvect]={0};
    int base;
    CRead_vec read;
    CYolo_vec yolo[nSTAGE];
    CWrite_vec write[nSTAGE];

    //Default constructor
    CYoloWrite();

    //Constructor with an associated base
    CYoloWrite(int base,versat_t*databus);

    //SIM
    void copy(CYoloWrite that);

    //set run start
    void start_run();

    //set update output buffers
    void update();

    //calculate new output
    void output();

    string info();
    string info_iter();

    void copy_write_config(CYoloWrite that);
    void copy_read_config(CYoloWrite that);
    bool done();
    void reset();
}; //end class CYoloWrite
