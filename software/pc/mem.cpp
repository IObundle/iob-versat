#include "versat.hpp"
#if nMEM > 0

versat_t CMem::read(uint32_t addr)
{
    return data[addr];
}
void CMem::write(uint32_t addr, versat_t data_in)
{
    data[addr] = data_in;
}
CMemPort::CMemPort() {}
CMemPort::CMemPort(int versat_base, int i, int offset)
{
    this->versat_base = versat_base;
    this->mem_base = i;
    this->data_base = offset;
    this->my_mem = &versat_mem[versat_base][i];
    this->iter = 0;
    this->per = 0;
    this->duty = 0;
    this->sel = 0;
    this->start = 0;
    this->shift = 0;
    this->incr = 0;
    this->delay = 0;
    this->in_wr = 0;
    this->rvrs = 0;
    this->ext = 0;
    this->iter2 = 0;
    this->per2 = 0;
    this->shift2 = 0;
    this->incr2 = 0;
    this->duty_cnt = 0;
}
void CMemPort::reset()
{
    loop1 = 0;
    loop2 = 0;
    loop3 = 0;
    loop4 = 0;
    duty_cnt = 0;
    enable = 0;
    run_delay = delay;
}
void CMemPort::update_shadow_reg_MEM()
{
    if (data_base == 1)
    {
        shadow_reg[versat_base].memB[mem_base].iter = conf[versat_base].memB[mem_base].iter;
        shadow_reg[versat_base].memB[mem_base].start = conf[versat_base].memB[mem_base].start;
        shadow_reg[versat_base].memB[mem_base].per = conf[versat_base].memB[mem_base].per;
        shadow_reg[versat_base].memB[mem_base].duty = conf[versat_base].memB[mem_base].duty;
        shadow_reg[versat_base].memB[mem_base].sel = conf[versat_base].memB[mem_base].sel;
        shadow_reg[versat_base].memB[mem_base].shift = conf[versat_base].memB[mem_base].shift;
        shadow_reg[versat_base].memB[mem_base].incr = conf[versat_base].memB[mem_base].incr;
        shadow_reg[versat_base].memB[mem_base].delay = conf[versat_base].memB[mem_base].delay;
        shadow_reg[versat_base].memB[mem_base].ext = conf[versat_base].memB[mem_base].ext;
        shadow_reg[versat_base].memB[mem_base].in_wr = conf[versat_base].memB[mem_base].in_wr;
        shadow_reg[versat_base].memB[mem_base].rvrs = conf[versat_base].memB[mem_base].rvrs;
        shadow_reg[versat_base].memB[mem_base].iter2 = conf[versat_base].memB[mem_base].iter2;
        shadow_reg[versat_base].memB[mem_base].per2 = conf[versat_base].memB[mem_base].per2;
        shadow_reg[versat_base].memB[mem_base].shift2 = conf[versat_base].memB[mem_base].shift2;
        shadow_reg[versat_base].memB[mem_base].incr2 = conf[versat_base].memB[mem_base].incr2;
    }
    else
    {
        shadow_reg[versat_base].memA[mem_base].iter = conf[versat_base].memA[mem_base].iter;
        shadow_reg[versat_base].memA[mem_base].start = conf[versat_base].memA[mem_base].start;
        shadow_reg[versat_base].memA[mem_base].per = conf[versat_base].memA[mem_base].per;
        shadow_reg[versat_base].memA[mem_base].duty = conf[versat_base].memA[mem_base].duty;
        shadow_reg[versat_base].memA[mem_base].sel = conf[versat_base].memA[mem_base].sel;
        shadow_reg[versat_base].memA[mem_base].shift = conf[versat_base].memA[mem_base].shift;
        shadow_reg[versat_base].memA[mem_base].incr = conf[versat_base].memA[mem_base].incr;
        shadow_reg[versat_base].memA[mem_base].delay = conf[versat_base].memA[mem_base].delay;
        shadow_reg[versat_base].memA[mem_base].ext = conf[versat_base].memA[mem_base].ext;
        shadow_reg[versat_base].memA[mem_base].in_wr = conf[versat_base].memA[mem_base].in_wr;
        shadow_reg[versat_base].memA[mem_base].rvrs = conf[versat_base].memA[mem_base].rvrs;
        shadow_reg[versat_base].memA[mem_base].iter2 = conf[versat_base].memA[mem_base].iter2;
        shadow_reg[versat_base].memA[mem_base].per2 = conf[versat_base].memA[mem_base].per2;
        shadow_reg[versat_base].memA[mem_base].shift2 = conf[versat_base].memA[mem_base].shift2;
        shadow_reg[versat_base].memA[mem_base].incr2 = conf[versat_base].memA[mem_base].incr2;
    }
}
void CMemPort::start_run()
{

    done = 0;
    pos = start;
    pos2 = start;
    if (duty == 0)
        duty = per;
    //set run_delay
    if (data_base == 1)
    {
        run_delay = shadow_reg[versat_base].memB[mem_base].delay;
    }
    else
    {
        run_delay = shadow_reg[versat_base].memA[mem_base].delay;
    }
}
void CMemPort::update()
{
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
        run_delay--;
    }
    else
    {
        versat_t aux_output = output_port[0];
        versat_t aux_output2 = 0;
        //trickle down all outputs in buffer
        for (i = 1; i < MEMP_LAT; i++)
        {
            aux_output2 = output_port[i];
            output_port[i] = aux_output;
            aux_output = aux_output2;
        }
        //insert new output
        output_port[0] = out; //TO DO: change according to output()

        //update databus
        if (data_base == 0)
        {
            stage[versat_base].databus[sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
            //special case for stage 0
            if (versat_base == 0)
            {
                //2nd copy at the end of global databus
                global_databus[nSTAGE * N + sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
            }
        }
        else
        {
            stage[versat_base].databus[sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
            //special case for stage 0
            if (versat_base == 0)
            {
                //2nd copy at the end of global databus
                global_databus[nSTAGE * N + sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
            }
        }
    }
}

uint32_t CMemPort::AGU()
{
    //pos = pos2 + (incr * l + shift) * k;
    //pos2 = incr2 * j + shift2 * i;
    uint32_t aux_acc = 0;
    if (done == 0)
        aux_acc = acumulator();

    //if (data_base == 0 && versat_base == 0 && mem_base == 0)
    //  printf("Current Core 0 MEMA[0] addr=%d,incr2=%d,shift2=%d\n", aux_acc, incr2, shift2);
    if (done == 1)
    {
        /*
        if (iter != 0 && per != 0)
        {
            printf("last addr was nstage=%d,Mem_%d[%d]=%u\n", versat_base, data_base, mem_base, aux);
            //printf("iter=%d,per=%d,iter2=%d,per2=%d\n", iter, per, iter2, per2);
        }
        */
        pos2 = start;
        pos = start;
    }
    return aux_acc;
}

uint32_t CMemPort::acumulator()
{
    if (iter2 == 0 && per2 == 0)
    {
        if (loop2 < iter)
        {
            if (loop1 < per)
            {
                loop1++;
                enable = 0;
                if (duty_cnt < duty)
                {
                    enable = 1;
                    aux = pos;
                    duty_cnt++;
                    pos += incr;
                }
            }
            if (loop1 == per)
            {
                loop1 = 0;
                duty_cnt = 0;
                loop2++;
                pos += shift;
            }
        }
        if (loop2 == iter)
        {
            loop2 = 0;
            done = 1;
        }
    }
    else
    {
        if (loop4 < iter2)
        {
            if (loop3 < per2)
            {
                if (loop2 < iter)
                {
                    if (loop1 < per)
                    {
                        loop1++;
                        enable = 0;
                        if (duty_cnt < duty)
                        {
                            enable = 1;
                            aux = pos;
                            duty_cnt++;
                            pos += incr;
                        }
                    }
                    if (loop1 == per)
                    {
                        loop1 = 0;
                        loop2++;
                        duty_cnt = 0;
                        pos += shift;
                    }
                }
                if (loop2 == iter)
                {
                    loop2 = 0;
                    pos2 += incr2;
                    pos = pos2;
                    loop3++;
                }
            }
            if (loop3 == per2)
            {
                pos2 += shift2;
                pos = pos2;
                loop3 = 0;
                loop4++;
            }
        }
        if (loop4 == iter2)
        {
            done = 1;
        }
    }
    return aux;
}

versat_t CMemPort::output()
{
    if (run_delay > 0)
        return 0;

    int addr = 0;
    if (done == 0)
    {
        addr = AGU();
    }
    else
    {
        return out;
    }

    versat_t data;
    if (in_wr == 1)
    {

        if (enable == 1)
        {
            if (data_base == 0)
            {
                //  if (versat_base == 4 && mem_base == 2 && data_base == 0)
                //    printf("My addr is %d, value is %d\n", addr, stage[versat_base].databus[sel]);
                write(addr, stage[versat_base].databus[sel]);
                out = stage[versat_base].databus[sel];
            }
            else
            {
                write(addr, stage[versat_base].databus[sel]);
                out = stage[versat_base].databus[sel];
            }
        }
    }
    else
    {
        data = read(addr);
        out = data;
    }
    return 0;
}

void CMemPort::writeConf()
{
    if (data_base == 1)
    {
        conf[versat_base].memB[mem_base].iter = iter;
        conf[versat_base].memB[mem_base].start = start;
        conf[versat_base].memB[mem_base].per = per;
        conf[versat_base].memB[mem_base].duty = duty;
        conf[versat_base].memB[mem_base].sel = sel;
        conf[versat_base].memB[mem_base].shift = shift;
        conf[versat_base].memB[mem_base].incr = incr;
        conf[versat_base].memB[mem_base].delay = delay;
        conf[versat_base].memB[mem_base].ext = ext;
        conf[versat_base].memB[mem_base].in_wr = in_wr;
        conf[versat_base].memB[mem_base].rvrs = rvrs;
        conf[versat_base].memB[mem_base].iter2 = iter2;
        conf[versat_base].memB[mem_base].per2 = per2;
        conf[versat_base].memB[mem_base].shift2 = shift2;
        conf[versat_base].memB[mem_base].incr2 = incr2;
    }
    else
    {
        conf[versat_base].memA[mem_base].iter = iter;
        conf[versat_base].memA[mem_base].start = start;
        conf[versat_base].memA[mem_base].per = per;
        conf[versat_base].memA[mem_base].duty = duty;
        conf[versat_base].memA[mem_base].sel = sel;
        conf[versat_base].memA[mem_base].shift = shift;
        conf[versat_base].memA[mem_base].incr = incr;
        conf[versat_base].memA[mem_base].delay = delay;
        conf[versat_base].memA[mem_base].ext = ext;
        conf[versat_base].memA[mem_base].in_wr = in_wr;
        conf[versat_base].memA[mem_base].rvrs = rvrs;
        conf[versat_base].memA[mem_base].iter2 = iter2;
        conf[versat_base].memA[mem_base].per2 = per2;
        conf[versat_base].memA[mem_base].shift2 = shift2;
        conf[versat_base].memA[mem_base].incr2 = incr2;
    }
}

void CMemPort::setIter(int iter)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].iter = iter;
    else
    {
        conf[versat_base].memA[mem_base].iter = iter;
    }
    this->iter = iter;
}

void CMemPort::setPer(int per)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].per = per;
    else
    {
        conf[versat_base].memA[mem_base].per = per;
    }
    this->per = per;
}

void CMemPort::setDuty(int duty)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].duty = duty;
    else
    {
        conf[versat_base].memA[mem_base].duty = duty;
    }
    this->duty = duty;
}

void CMemPort::setSel(int sel)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].sel = sel;
    else
    {
        conf[versat_base].memA[mem_base].sel = sel;
    }
    this->sel = sel;
}

void CMemPort::setStart(int start)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].start = start;
    else
    {
        conf[versat_base].memA[mem_base].start = start;
    }
    this->start = start;
}
void CMemPort::setIncr(int incr)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].incr = incr;
    else
    {
        conf[versat_base].memA[mem_base].incr = incr;
    }
    this->incr = incr;
}

void CMemPort::setShift(int shift)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].shift = shift;
    else
    {
        conf[versat_base].memA[mem_base].shift = shift;
    }
    this->shift = shift;
}

void CMemPort::setDelay(int delay)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].delay = delay;
    else
    {
        conf[versat_base].memA[mem_base].delay = delay;
    }
    this->delay = delay;
}

void CMemPort::setExt(int ext)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].ext = ext;
    else
    {
        conf[versat_base].memA[mem_base].ext = ext;
    }
    this->ext = ext;
}

void CMemPort::setRvrs(int rvrs)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].rvrs = rvrs;
    else
    {
        conf[versat_base].memA[mem_base].rvrs = rvrs;
    }
    this->rvrs = rvrs;
}

void CMemPort::setInWr(int in_wr)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].in_wr = in_wr;
    else
    {
        conf[versat_base].memA[mem_base].in_wr = in_wr;
    }
    this->in_wr = in_wr;
}

void CMemPort::setIter2(int iter2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].iter2 = iter2;
    else
    {
        conf[versat_base].memA[mem_base].iter2 = iter2;
    }
    this->iter2 = iter2;
}

void CMemPort::setPer2(int per2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].per2 = per2;
    else
    {
        conf[versat_base].memA[mem_base].per2 = per2;
    }
    this->per2 = per2;
}

void CMemPort::setIncr2(int incr2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].incr2 = incr2;
    else
    {
        conf[versat_base].memA[mem_base].incr2 = incr2;
    }
    this->incr2 = incr2;
}

void CMemPort::setShift2(int shift2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].shift2 = shift2;
    else
    {
        conf[versat_base].memA[mem_base].shift2 = shift2;
    }
    this->shift2 = shift2;
}

void CMemPort::write(int addr, int val)
{
    //MEMSET(versat_base, (this->data_base + addr), val);
    if (addr >= MEM_SIZE)
    {
        printf("Invalid WRITE MEM ADDR=%u\n", addr);
        print_versat_info();
        printf("VERSAT EXITING ON nStage_%d MEM_%d[%d]\n", versat_base, data_base, mem_base);
        //exit(0);
    }
    else
        my_mem->write(addr, val);
}

int CMemPort::read(int addr)
{
    //return MEMGET(versat_base, (this->data_base + addr));
    if (addr >= MEM_SIZE)
    {
        printf("Invalid READ MEM ADDR=%u\n", addr);
        print_versat_info();
        printf("VERSAT EXITING ON nStage_%d MEM_%d[%d]\n", versat_base, data_base, mem_base);
        //exit(0);
        return 0;
    }
    else
        return my_mem->read(addr);

    return 0;
}
string CMemPort::info()
{
    string ver = "mem";
    if (data_base == 0)
        ver += "A[" + to_string(mem_base) + "]\n";
    else
    {
        ver += "B[" + to_string(mem_base) + "]\n";
    }
    ver += "Start=    " + to_string(start) + "\n";
    ver += "Iter=     " + to_string(iter) + "\n";
    ver += "Per =     " + to_string(per) + "\n";
    ver += "Incr=     " + to_string(incr) + "\n";
    ver += "Delay=    " + to_string(delay) + "\n";
    ver += "Shift=    " + to_string(shift) + "\n";
    ver += "Duty=     " + to_string(duty) + "\n";
    ver += "Sel=      " + to_string(sel) + "\n";
    ver += "Ext=      " + to_string(ext) + "\n";
    ver += "Rvrs =    " + to_string(rvrs) + "\n";
    ver += "InWr=     " + to_string(in_wr) + "\n";
    ver += "Iter2=    " + to_string(iter2) + "\n";
    ver += "Per2 =    " + to_string(per2) + "\n";
    ver += "Shift2=   " + to_string(shift2) + "\n";
    ver += "Incr2=    " + to_string(incr2) + "\n";
    ver += "Done=     " + to_string(done) + "\n";
    ver += "\n";

    return ver;
}

string CMemPort::info_iter()
{
    string ver = "mem";
    if (data_base == 0)
        ver += "A[" + to_string(mem_base) + "]\n";
    else
    {
        ver += "B[" + to_string(mem_base) + "]\n";
    }
    ver += "Out=    " + to_string(out) + "\n";
    ver += "Addr=    " + to_string(aux) + "\n";
    ver += "OUTPUT_BUFFER (LATENCY SIM)\n";
    for (int z = 0; z < MEMP_LAT; z++)
    {
        ver += "Output[" + to_string(z) + "]=" + to_string(output_port[z]) + "\n";
    }
    ver += "\n";

    return ver;
}
#endif