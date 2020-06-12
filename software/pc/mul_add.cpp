#include "versat.hpp"
#if nMULADD > 0
CMulAdd::CMulAdd()
{
}

CMulAdd::CMulAdd(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->muladd_base = i;
}

//set MulAdd configuration to shadow register
void CMulAdd::update_shadow_reg_MulAdd()
{
    shadow_reg[versat_base].muladd[muladd_base].sela = conf[versat_base].muladd[muladd_base].sela;
    shadow_reg[versat_base].muladd[muladd_base].selb = conf[versat_base].muladd[muladd_base].selb;
    shadow_reg[versat_base].muladd[muladd_base].fns = conf[versat_base].muladd[muladd_base].fns;
    shadow_reg[versat_base].muladd[muladd_base].iter = conf[versat_base].muladd[muladd_base].iter;
    shadow_reg[versat_base].muladd[muladd_base].per = conf[versat_base].muladd[muladd_base].per;
    shadow_reg[versat_base].muladd[muladd_base].delay = conf[versat_base].muladd[muladd_base].delay;
    shadow_reg[versat_base].muladd[muladd_base].shift = conf[versat_base].muladd[muladd_base].shift;
}

//start run
void CMulAdd::start_run()
{
    //set run_delay
    run_delay = shadow_reg[versat_base].muladd[muladd_base].delay;

    //set addrgen counter variables
    cnt_iter = 0;
    cnt_per = 0;
    cnt_addr = 0;
}

//update output buffer, write results to databus
void CMulAdd::update()
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
        stage[versat_base].databus[sMULADD[muladd_base]] = output_buff[MULADD_LAT - 1];
        //special case for stage 0
        if (versat_base == 0)
        {
            //2nd copy at the end of global databus
            global_databus[nSTAGE * N + sMULADD[muladd_base]] = output_buff[MULADD_LAT - 1];
        }

        //trickle down all outputs in buffer
        for (i = 1; i < MULADD_LAT; i++)
        {
            output_buff[i] = output_buff[i - 1];
        }
        //insert new output
        output_buff[0] = out;
    }
}

versat_t CMulAdd::output() //implemented as PIPELINED MULADD
{
    //wait for delay to end
    if (run_delay > 0)
    {
        return 0;
    }

    //select inputs
    opa = stage[versat_base].databus[sela];
    opb = stage[versat_base].databus[selb];

    //select acc_w value
    acc_w = (cnt_addr == 0) ? 0 : acc;

    //perform MAC operation
    mul_t result_mult = opa * opb;
    if (fns == MULADD_MACC)
    {
        acc = acc_w + result_mult;
    }
    else
    {
        acc = acc_w - result_mult;
    }
    out = (versat_t)(acc >> shift);
    //if (opa != 0 && opb != 0)
    //  printf("Core=%d,A=%hi,B=%hi,Mul_t=%d,Acc=%d,Out=%hi\n", versat_base, opa, opb, result_mult, acc, out);

    //update addrgen counter - 1 iteration of nested for loop
    if (cnt_iter < iter)
    {
        if (cnt_per < per)
        {
            cnt_addr++;
            cnt_per++;
        }
        else
        {
            cnt_per = 0;
            cnt_addr += -per;
            cnt_iter++;
        }
    }

    return out;
}

void CMulAdd::writeConf()
{
    conf[versat_base].muladd[muladd_base].sela = sela;
    conf[versat_base].muladd[muladd_base].selb = selb;
    conf[versat_base].muladd[muladd_base].fns = fns;
    conf[versat_base].muladd[muladd_base].iter = iter;
    conf[versat_base].muladd[muladd_base].per = per;
    conf[versat_base].muladd[muladd_base].delay = delay;
    conf[versat_base].muladd[muladd_base].shift = shift;
}
void CMulAdd::setSelA(int sela)
{
    conf[versat_base].muladd[muladd_base].sela = sela;
    this->sela = sela;
}
void CMulAdd::setSelB(int selb)
{
    conf[versat_base].muladd[muladd_base].selb = selb;
    this->selb = selb;
}
void CMulAdd::setFNS(int fns)
{
    conf[versat_base].muladd[muladd_base].fns = fns;
    this->fns = fns;
}
void CMulAdd::setIter(int iter)
{
    conf[versat_base].muladd[muladd_base].iter = iter;
    this->iter = iter;
}
void CMulAdd::setPer(int per)
{
    conf[versat_base].muladd[muladd_base].per = per;
    this->per = per;
}
void CMulAdd::setDelay(int delay)
{
    conf[versat_base].muladd[muladd_base].delay = delay;
    this->delay = delay;
}
void CMulAdd::setShift(int shift)
{
    conf[versat_base].muladd[muladd_base].shift = shift;
    this->shift = shift;
}
string CMulAdd::info()
{
    string ver = "mul_add[" + to_string(muladd_base) + "]\n";
    ver += "SelA=     " + to_string(sela) + "\n";
    ver += "SelB=     " + to_string(selb) + "\n";
    ver += "FNS =     " + to_string(fns) + "\n";
    ver += "Iter=     " + to_string(iter) + "\n";
    ver += "Per =     " + to_string(per) + "\n";
    ver += "Delay=    " + to_string(delay) + "\n";
    ver += "Shift=    " + to_string(shift) + "\n";
    ver += "\n";
    return ver;
}
#endif