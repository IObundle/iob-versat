#include "versat.hpp"

CMul::CMul()
{
}

CMul::CMul(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->mul_base = i;
}

void CMul::update_shadow_reg_Mul()
{
    shadow_reg[versat_base].mul[mul_base].sela = conf[versat_base].mul[mul_base].sela;
    shadow_reg[versat_base].mul[mul_base].selb = conf[versat_base].mul[mul_base].selb;
    shadow_reg[versat_base].mul[mul_base].fns = conf[versat_base].mul[mul_base].fns;
}

void CMul::start_run()
{
    //CMul has no delay
}

void CMul::update()
{
    int i = 0;

    //update databus
    stage[versat_base].databus[sMUL[mul_base]] = output_buff[MUL_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sMUL[mul_base]] = output_buff[MUL_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < MUL_LAT; i++)
    {
        output_buff[i] = output_buff[i - 1];
    }
    //insert new output
    output_buff[0] = out;
}

versat_t CMul::output()
{
    //select inputs
    opa = stage[versat_base].databus[shadow_reg[versat_base].mul[mul_base].sela];
    opb = stage[versat_base].databus[shadow_reg[versat_base].mul[mul_base].selb];

    mul_t result_mult = opa * opb;
    if (shadow_reg[versat_base].mul[mul_base].fns == MUL_HI) //big brain time: to avoid left/right shifts, using a MASK of size mul_t and versat_t
    {
        result_mult = result_mult << 1;
        out = (versat_t)(result_mult >> (sizeof(versat_t) * 8));
    }
    else if (shadow_reg[versat_base].mul[mul_base].fns == MUL_DIV2_HI)
    {
        out = (versat_t)(result_mult >> (sizeof(versat_t) * 8));
    }
    else // MUL_LO
    {
        out = (versat_t)result_mult;
    }

    return out;
}

void CMul::setConf(int sela, int selb, int fns)
{
    this->sela = sela;
    this->selb = selb;
    this->fns = fns;
}

void CMul::writeConf()
{
    conf[versat_base].mul[mul_base].sela = sela;
    conf[versat_base].mul[mul_base].selb = selb;
    conf[versat_base].mul[mul_base].fns = fns;
}
void CMul::setSelA(int sela)
{
    conf[versat_base].mul[mul_base].sela = sela;
    this->sela = sela;
}
void CMul::setSelB(int selb)
{
    conf[versat_base].mul[mul_base].selb = selb;
    this->selb = selb;
}
void CMul::setFNS(int fns)
{
    conf[versat_base].mul[mul_base].fns = fns;
    this->fns = fns;
}