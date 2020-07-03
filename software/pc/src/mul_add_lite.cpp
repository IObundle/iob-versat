#include "mul_add_lite.hpp"
#if nMULADDLITE > 0
extern CStage stage[nSTAGE];
extern CStage conf[nSTAGE];
extern CStage shadow_reg[nSTAGE];
extern CMem versat_mem[nSTAGE][nMEM];
extern int versat_iter;
extern versat_t global_databus[(nSTAGE + 1) * N];
CMulAddLite::CMulAddLite()
{
}

CMulAddLite::CMulAddLite(int versat_base, int i, versat_t *databus)
{
    this->versat_base = versat_base;
    this->muladdlite_base = i;
    this->databus = databus;
}

//start run
void CMulAddLite::start_run()
{
    //set run_delay
    run_delay = delay;
}

//update output buffer, write results to databus
void CMulAddLite::update()
{
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
        run_delay--;
    }
    else
    {

        //update databus
        databus[sMULADDLITE[muladdlite_base]] = output_buff[MULADDLITE_LAT - 1];
        //special case for stage 0
        if (versat_base == 0)
        {
            //2nd copy at the end of global databus
            global_databus[nSTAGE * (1 << (N_W - 1)) + sMULADDLITE[muladdlite_base]] = output_buff[MULADDLITE_LAT - 1];
        }

        //trickle down all outputs in buffer
        for (i = 1; i < MULADDLITE_LAT; i++)
        {
            output_buff[i] = output_buff[i - 1];
        }
        //insert new output
        output_buff[0] = out;
    }
}

versat_t CMulAddLite::output() //TO DO: need to implemente MulAddLite functionalities
{

    //select inputs
    opa = databus[sela];
    opb = databus[selb];

    mul_t result_mult = opa * opb;
    if (fns == MULADD_MACC)
    {
        acc = acc_w + result_mult;
    }
    else
    {
        acc = acc_w - result_mult;
    }
    acc_w = (versat_t)(acc >> shift);

    return acc_w;
}

void CMulAddLite::setSelA(int sela)
{
    this->sela = sela;
}
void CMulAddLite::setSelB(int selb)
{
    this->selb = selb;
}
void CMulAddLite::setSelC(int selc)
{
    this->selc = selc;
}
void CMulAddLite::setIter(int iter)
{
    this->iter = iter;
}
void CMulAddLite::setPer(int per)
{
    this->per = per;
}
void CMulAddLite::setDelay(int delay)
{
    this->delay = delay;
}
void CMulAddLite::setShift(int shift)
{
    this->shift = shift;
}
void CMulAddLite::setAccIN(int accIN)
{
    this->accIN = accIN;
}
void CMulAddLite::setAccOUT(int accOUT)
{
    this->accOUT = accOUT;
}
void CMulAddLite::setBatch(int batch)
{
    this->batch = batch;
}

string CMulAddLite::info()
{
}

#endif