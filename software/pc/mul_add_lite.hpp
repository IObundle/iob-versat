#include "type.hpp"
#if nMULADDLITE > 0
class CMulAddLite
{
private:
    //count delay during a run():
    int run_delay = 0;
    versat_t output_buff[MULADDLITE_LAT]; //output FIFO
public:
    int versat_base, muladdlite_base;
    int sela, selb, iter, per, delay, shift;
    int selc = 0, accIN = 0, accOUT = 0, batch = 0;

    //Default constructor
    CMulAddLite();

    CMulAddLite(int versat_base, int i);

    //start run
    void start_run();

    //update output buffer, write results to databus
    void update();
    versat_t output(); //TO DO: need to implemente MulAddLite functionalities

    void setSelA(int sela);
    void setSelB(int selb);
    void setSelC(int selc);
    void setIter(int iter);
    void setPer(int per);
    void setDelay(int delay);
    void setShift(int shift);
    void setAccIN(int accIN);
    void setAccOUT(int accOUT);
    void setBatch(int batch);

    string info();
}; //end class CMULADDLITE

extern versat_t global_databus[(nSTAGE + 1) * N];
extern int sMULADDLITE[nALULITE];
#endif