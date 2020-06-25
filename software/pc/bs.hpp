#include "type.hpp"

#if nBS > 0
class CBS
{
private:
    versat_t in = 0, out = 0;
    versat_t output_buff[BS_LAT]; //output FIFO
public:
    int versat_base, bs_base;
    int data = 0, shift = 0, fns = 0;

    //Default constructor
    CBS();
    CBS(int versat_base, int i);

    //set BS configuration to shadow register
    void update_shadow_reg_BS();

    //start run
    void start_run();

    //update output buffer, write results to databus
    void update();

    versat_t output();

    void writeConf();
    void setData(int data);
    void setShift(int shift);
    void setFNS(int fns);

    string info();
}; //end class CBS
#endif