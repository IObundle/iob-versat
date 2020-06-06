#include "versat.hpp"
#if nALU > 0
CALU::CALU()
{
}

CALU::CALU(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->alu_base = i;
}

void CALU::update_shadow_reg_ALU()
{
    shadow_reg[versat_base].alu[alu_base].opa = conf[versat_base].alu[alu_base].opa;
    shadow_reg[versat_base].alu[alu_base].opb = conf[versat_base].alu[alu_base].opb;
    shadow_reg[versat_base].alu[alu_base].fns = conf[versat_base].alu[alu_base].fns;
}

void CALU::start_run()
{
    //CAlu has no delay
}

void CALU::update()
{
    int i = 0;

    //update databus
    stage[versat_base].databus[sALU[alu_base]] = output_buff[ALU_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sALU[alu_base]] = output_buff[ALU_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < ALU_LAT; i++)
    {
        output_buff[i] = output_buff[i - 1];
    }
    //insert new output
    output_buff[0] = out;
}

versat_t CALU::output()
{
    inb = stage[versat_base].databus[shadow_reg[versat_base].alu[alu_base].opa];
    ina = stage[versat_base].databus[shadow_reg[versat_base].alu[alu_base].opb];
    bitset<DATAPATH_W> aux_sext;
    bitset<DATAPATH_W> aux_cmp;
    uint8_t aux_a = ina;
    bool val;
    shift_t aux = ina;
    shift_t ina_uns = ina;
    shift_t inb_uns = inb;

    switch (fns)
    {
    case ALU_OR:
        out = inb | ina;
        break;
    case ALU_AND:
        out = inb & ina;
        break;
    case ALU_XOR:
        out = inb ^ ina;
        break;
    case ALU_SEXT8:
        aux_a = ina;
        val = MSB(aux_a, 8);
        SET_BITS_SEXT8(aux_sext, val, DATAPATH_W);
        out = aux_sext.to_ulong() + aux_a;
        break;
    case ALU_SEXT16:
        val = MSB(aux_a, 16);
        SET_BITS_SEXT16(aux_sext, val, DATAPATH_W);
        out = aux_sext.to_ulong() + aux_a;
        break;
    case ALU_SHIFTR_ARTH:
        out = ina >> 1;
        break;
    case ALU_SHIFTR_LOG:
        aux = aux >> 1;
        out = (versat_t)aux;
        break;
    case ALU_CMP_SIG:
        aux_cmp.set(DATAPATH_W - 1, ina > inb ? 1 : 0);
        out = (versat_t)aux_cmp.to_ulong();
        break;
    case ALU_CMP_UNS:
        ina_uns = ina;
        inb_uns = inb;
        aux_cmp.set(DATAPATH_W - 1, ina_uns > inb_uns ? 1 : 0);
        out = (versat_t)aux_cmp.to_ulong();
        break;
    case ALU_MUX:
        out = ina < 0 ? inb : 0;
        break;
    case ALU_ADD:
        out = ina + inb;
        break;
    case ALU_SUB:
        out = inb - ina;
        break;
    case ALU_MAX:
        out = ina > inb ? ina : inb;
        break;
    case ALU_MIN:
        out = ina < inb ? ina : inb;
        break;
    default:
        break;
    }
    return out;
}

void CALU::writeConf()
{
    conf[versat_base].alu[alu_base].opa = opa;
    conf[versat_base].alu[alu_base].opb = opb;
    conf[versat_base].alu[alu_base].fns = fns;
}

void CALU::setOpA(int opa)
{
    conf[versat_base].alu[alu_base].opa = opa;
    this->opa = opa;
}

void CALU::setOpB(int opb)
{
    conf[versat_base].alu[alu_base].opb = opb;
    this->opb = opb;
}

void CALU::setFNS(int fns)
{
    conf[versat_base].alu[alu_base].fns = fns;
    this->fns = fns;
}
#endif