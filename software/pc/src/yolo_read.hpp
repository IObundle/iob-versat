#include "type.hpp"
#include "vread.hpp"
class CYoloRead
{
private:
versat_t*databus=NULL;
public:
    int base;
versat_t kernel_mem[nYOLOvect][1 << WEIGHT_ADDR_W] = {0};
versat_t bias_mem[1 << BIAS_ADDR_W] = {0};
    CRead kernels[nYOLOvect];
    CRead bias;
    int loop_amount = 1;

    //Default constructor
    CYoloRead();

    //Constructor with an associated base
    CYoloRead(int base,versat_t*databus);

    //Methods to set config parameters
    void setExtAddr(int extAddr);
    void setOffset(int offset);
    void setPingPong(int pp);
    void setLen(int len);
    void setIntAddr(int intAddr);
    void setExtIter(int iter);
    void setExtPer(int per);
    void setExtShift(int shift);
    void setExtIncr(int incr);
    void setIntIter(int iter);
    void setIntPer(int per);
    void setIntStart(int start);
    void setIntShift(int shift);
    void setIntIncr(int incr);
    void setBiasExtAddr(int extAddr);
    void setBiasIntAddr(int intAddr);
    void setBiasIntStart(int start);

    //SIM
    void copy(CYoloRead that);
    void copy_ext(CYoloRead that);

    //set run start
    void start_run();

    //set update output buffers
    void update();

    //calculate new output
    void output();

    string info();
    string info_iter();
    
    void copy_read_config(CYoloRead that);

    bool done();
}; //end class CYoloRead