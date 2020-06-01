#include "versat.hpp"

CBS::CBS()
{
}

CBS::CBS(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->bs_base = i;
}

void CBS::update_shadow_reg_BS()
{
    shadow_reg[versat_base].bs[bs_base].data = conf[versat_base].bs[bs_base].data;
    shadow_reg[versat_base].bs[bs_base].fns = conf[versat_base].bs[bs_base].fns;
    shadow_reg[versat_base].bs[bs_base].shift = conf[versat_base].bs[bs_base].shift;
}

void CBS::start_run()
{
    //CBS has no delay
}

void CBS::update()
{
    int i = 0;

    //update databus
    stage[versat_base].databus[sBS[bs_base]] = output_buff[BS_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sBS[bs_base]] = output_buff[BS_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < BS_LAT; i++)
    {
        output_buff[i] = output_buff[i - 1];
    }
    //insert new output
    output_buff[0] = out;
}

versat_t CBS::output()
{
    in = stage[versat_base].databus[shadow_reg[versat_base].bs[bs_base].data];
    if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHR_A)
    {
        in = in >> shadow_reg[versat_base].bs[bs_base].shift;
    }
    else if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHR_L)
    {
        shift_t s = in;
        s = s >> shadow_reg[versat_base].bs[bs_base].shift;
        in = s;
    }
    else if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHL)
    {
        in = in << shadow_reg[versat_base].bs[bs_base].shift;
    }

    out = in;

    return out;
}

void CBS::writeConf()
{
    conf[versat_base].bs[bs_base].data = data;
    conf[versat_base].bs[bs_base].shift = shift;
    conf[versat_base].bs[bs_base].fns = fns;
}

void CBS::setData(int data)
{
    conf[versat_base].bs[bs_base].data = data;
    this->data = data;
}

void CBS::setShift(int shift)
{
    conf[versat_base].bs[bs_base].shift = shift;
    this->shift = shift;
}

void CBS::setFNS(int fns)
{
    conf[versat_base].bs[bs_base].fns = fns;
    this->fns = fns;
}