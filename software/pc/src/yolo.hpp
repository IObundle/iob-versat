#include "type.hpp"
#define XYOLO_LAT 7
#define XYOLO_LAT_BYPASS 1


class CYolo
{
private:
    int iter = 0, per = 0, shift = 0, leaky = 0, maxpool = 0, bypass = 0;
    int loop1 = 0, loop2 = 0, pos = 0, aux = 0, sigmoid = 0;
    versat_t out = 0, opa = 0, opb = 0;
    versat_t output_buff[XYOLO_LAT];
    int run_delay = 0;
    versat_t output_buff_bypass[XYOLO_LAT_BYPASS];
    versat_t *databus = NULL;
    versat_t acc_w, acc;
    uint32_t cnt_addr;
    mul_t bias = 0;
    versat_t* databus_bias=NULL;
    versat_t* databus_kernel=NULL;
    versat_t* databus_pixel=NULL;
    versat_t* databus_out=NULL;
public:
    int base;
    int done = 0;

    //Default constructor
    CYolo();
    //Constructor with an associated base
    CYolo(int base,versat_t *bias,versat_t*databus_kernel,versat_t *databus_pixels,versat_t *databus_write);

    //Methods to set config parameters
    void setIter(int iter);
    void setPer(int per);
    void setShift(int shift);
    void setBias(int bias);
    void setLeaky(int leaky);
    void setMaxpool(int maxpool);
    void setSigmoid(int sigmoid);
    void setBypass(int bypass);

    //start run
    void start_run();

    //update output buffer, write results to databus
    void update();
    versat_t output();
    uint32_t acumulator();
    void copy(CYolo that);
    

    string info();
    string info_iter();
}; //end class CYOLO
class CYolo_vec
{
    private:
    CYolo mac[nYOLOvect];
    versat_t* databus_read=NULL;
    versat_t* databus_bias=NULL;
    versat_t* databus_read_yolo=NULL;
    versat_t* databus_yolo_write=NULL;
    public:
    int versat_base;
    //Default constructor
    CYolo_vec();
    //Constructor with an associated base
    CYolo_vec(int versat_base,versat_t* databus_bias, versat_t*databus_read,versat_t* databus_read_yolo,versat_t* databus_yolo_write);

    // Q6.10 para os pesos Q8.8 para as ativações
    // SAIDA do YOLO Q2.14

    //Methods to set config parameters
    void setIter(int iter);
    void setPer(int per);
    void setShift(int shift);
    void setBias(int bias);
    void setLeaky(int leaky);
    void setSigmoid(int sigmoid);
    void setMaxpool(int maxpool);
    void setBypass(int bypass);
    bool done();
    //start run
    void start_run();

    //update output buffer, write results to databus
    void update();
    versat_t output();
    void copy(CYolo_vec that);

    string info();
    string info_iter();
};
