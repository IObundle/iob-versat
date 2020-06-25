#include "bs.hpp"
#if nBS > 0

CBS::CBS()
{
}

CBS::CBS(int versat_base, int i, versat_t *databus)
{
    this->versat_base = versat_base;
    this->bs_base = i;
    this->databus = databus;
}

void CBS::start_run()
{
    //CBS has no delay
}

void CBS::update()
{
    int i = 0;

    //update databus
    databus[sBS[bs_base]] = output_buff[BS_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sBS[bs_base]] = output_buff[BS_LAT - 1];
    }

    //trickle down all outputs in buffer
    versat_t aux_output = output_buff[0];
    versat_t aux_output2 = 0;
    //trickle down all outputs in buffer
    for (i = 1; i < BS_LAT; i++)
    {
        aux_output2 = output_buff[i];
        output_buff[i] = aux_output;
        aux_output = aux_output2;
    }
    //insert new output
    output_buff[0] = out;
}

versat_t CBS::output()
{
    in = databus[data];
    if (fns == BS_SHR_A)
    {
        in = in >> shift;
    }
    else if (fns == BS_SHR_L)
    {
        shift_t s = in;
        s = s >> shift;
        in = s;
    }
    else if (fns == BS_SHL)
    {
        in = in << shift;
    }

    out = in;

    return out;
}

void CBS::setData(int data)
{
    this->data = data;
}

void CBS::setShift(int shift)
{
    this->shift = shift;
}

void CBS::setFNS(int fns)
{
    this->fns = fns;
}
string CBS::info()
{
    string ver = "bs[" + to_string(bs_base) + "]\n";
    ver += "Data=       " + to_string(data) + "\n";
    ver += "Shift=      " + to_string(selb) + "\n";
    ver += "FNS =       " + to_string(fns) + "\n";
    ver += "\n";
    return ver;
}
#endif