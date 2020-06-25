#include "type.hpp"

#if nBS > 0
class CBS
{
private:
    versat_t in = 0, out = 0;
    versat_t output_buff[BS_LAT]; //output FIFO
    versat_t *databus = NULL;

public:
    int versat_base, bs_base;
    int data = 0, shift = 0, fns = 0;

    //Default constructor
    CBS();
    CBS(int versat_base, int i, versat_t *databus);

    //set BS configuration to shadow register

    //start run
    void start_run();

    //update output buffer, write results to databus
    void update();

    versat_t output();

    void setData(int data);
    void setShift(int shift);
    void setFNS(int fns);

    string info();
}; //end class CBS

extern versat_t global_databus[(nSTAGE + 1) * N];
extern int sBS[nALULITE];
#endif