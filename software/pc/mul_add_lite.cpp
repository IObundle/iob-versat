#include "versat.hpp"
#if nMULADDLITE > 0
CMulAddLite::CMulAddLite()
{
}

CMulAddLite::CMulAddLite(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->muladdlite_base = i;
}

//set MulAdd configuration to shadow register
void CMulAddLite::update_shadow_reg_MulAddLite()
{
    shadow_reg[versat_base].muladdlite[muladdlite_base].sela = conf[versat_base].muladdlite[muladdlite_base].sela;
    shadow_reg[versat_base].muladdlite[muladdlite_base].selb = conf[versat_base].muladdlite[muladdlite_base].selb;
    shadow_reg[versat_base].muladdlite[muladdlite_base].selc = conf[versat_base].muladdlite[muladdlite_base].selc;
    shadow_reg[versat_base].muladdlite[muladdlite_base].iter = conf[versat_base].muladdlite[muladdlite_base].iter;
    shadow_reg[versat_base].muladdlite[muladdlite_base].per = conf[versat_base].muladdlite[muladdlite_base].per;
    shadow_reg[versat_base].muladdlite[muladdlite_base].delay = conf[versat_base].muladdlite[muladdlite_base].delay;
    shadow_reg[versat_base].muladdlite[muladdlite_base].shift = conf[versat_base].muladdlite[muladdlite_base].shift;
    shadow_reg[versat_base].muladdlite[muladdlite_base].accIN = conf[versat_base].muladdlite[muladdlite_base].accIN;
    shadow_reg[versat_base].muladdlite[muladdlite_base].accOUT = conf[versat_base].muladdlite[muladdlite_base].accOUT;
    shadow_reg[versat_base].muladdlite[muladdlite_base].batch = conf[versat_base].muladdlite[muladdlite_base].batch;
}

//start run
void CMulAddLite::start_run()
{
    //set run_delay
    run_delay = shadow_reg[versat_base].muladdlite[muladdlite_base].delay;
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
        stage[versat_base].databus[sMULADDLITE[muladdlite_base]] = output_buff[MULADDLITE_LAT - 1];
        //special case for stage 0
        if (versat_base == 0)
        {
            //2nd copy at the end of global databus
            global_databus[nSTAGE * N + sMULADDLITE[muladdlite_base]] = output_buff[MULADDLITE_LAT - 1];
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
    opa = stage[versat_base].databus[shadow_reg[versat_base].muladdlite[muladdlite_base].sela];
    opb = stage[versat_base].databus[shadow_reg[versat_base].muladdlite[muladdlite_base].selb];

    mul_t result_mult = opa * opb;
    if (shadow_reg[versat_base].muladdlite[muladdlite_base].fns == MULADD_MACC)
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

void CMulAddLite::setConf(int sela, int selb, int iter, int per, int delay, int shift)
{
    this->sela = sela;
    this->selb = selb;
    this->iter = iter;
    this->per = per;
    this->delay = delay;
    this->shift = shift;
}

void CMulAddLite::setConf(int selc, int accIN, int accOUT, int batch)
{
    this->selc = selc;
    this->accIN = accIN;
    this->accOUT = accOUT;
    this->batch = batch;
}

void CMulAddLite::writeConf()
{
    conf[versat_base].muladdlite[muladdlite_base].sela = sela;
    conf[versat_base].muladdlite[muladdlite_base].selb = selb;
    conf[versat_base].muladdlite[muladdlite_base].selc = selc;
    conf[versat_base].muladdlite[muladdlite_base].iter = iter;
    conf[versat_base].muladdlite[muladdlite_base].per = per;
    conf[versat_base].muladdlite[muladdlite_base].delay = delay;
    conf[versat_base].muladdlite[muladdlite_base].shift = shift;
    conf[versat_base].muladdlite[muladdlite_base].accIN = accIN;
    conf[versat_base].muladdlite[muladdlite_base].accOUT = accOUT;
    conf[versat_base].muladdlite[muladdlite_base].batch = batch;
}
void CMulAddLite::setSelA(int sela)
{
    conf[versat_base].muladdlite[muladdlite_base].sela = sela;
    this->sela = sela;
}
void CMulAddLite::setSelB(int selb)
{
    conf[versat_base].muladdlite[muladdlite_base].selb = selb;
    this->selb = selb;
}
void CMulAddLite::setSelC(int selc)
{
    conf[versat_base].muladdlite[muladdlite_base].selc = selc;
    this->selc = selc;
}
void CMulAddLite::setIter(int iter)
{
    conf[versat_base].muladdlite[muladdlite_base].iter = iter;
    this->iter = iter;
}
void CMulAddLite::setPer(int per)
{
    conf[versat_base].muladdlite[muladdlite_base].per = per;
    this->per = per;
}
void CMulAddLite::setDelay(int delay)
{
    conf[versat_base].muladdlite[muladdlite_base].delay = delay;
    this->delay = delay;
}
void CMulAddLite::setShift(int shift)
{
    conf[versat_base].muladdlite[muladdlite_base].shift = shift;
    this->shift = shift;
}
void CMulAddLite::setAccIN(int accIN)
{
    conf[versat_base].muladdlite[muladdlite_base].accIN = accIN;
    this->accIN = accIN;
}
void CMulAddLite::setAccOUT(int accOUT)
{
    conf[versat_base].muladdlite[muladdlite_base].accOUT = accOUT;
    this->accOUT = accOUT;
}
void CMulAddLite::setBatch(int batch)
{
    conf[versat_base].muladdlite[muladdlite_base].batch = batch;
    this->batch = batch;
}

#endif