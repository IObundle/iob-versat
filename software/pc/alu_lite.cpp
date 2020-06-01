#include "versat.hpp"

CALULite::CALULite()
{
}

CALULite::CALULite(int versat_base, int i)
{
    this->versat_base = versat_base;
    this->alulite_base = i;
}

void CALULite::update_shadow_reg_ALULite()
{
    shadow_reg[versat_base].alulite[alulite_base].opa = conf[versat_base].alulite[alulite_base].opa;
    shadow_reg[versat_base].alulite[alulite_base].opb = conf[versat_base].alulite[alulite_base].opb;
    shadow_reg[versat_base].alulite[alulite_base].fns = conf[versat_base].alulite[alulite_base].fns;
}

void CALULite::start_run()
{
    //CALULite has no delay
}

void CALULite::update()
{
    int i = 0;

    //update databus
    stage[versat_base].databus[sALULITE[alulite_base]] = output_buff[ALULITE_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sALULITE[alulite_base]] = output_buff[ALULITE_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < ALULITE_LAT; i++)
    {
        output_buff[i] = output_buff[i - 1];
    }
    //insert new output
    output_buff[0] = out;
}

versat_t CALULite::output()
{
    bitset<DATAPATH_W + 1> ai;
    bitset<DATAPATH_W + 1> bz;
    SET_BITS(ai, 0, DATAPATH_W + 1);
    SET_BITS(bz, 1, DATAPATH_W + 1);

    //cast to int cin.to_ulong();
    versat_t op_a_reg = stage[versat_base].databus[shadow_reg[versat_base].alulite[alulite_base].opa];
    versat_t self_loop = shadow_reg[versat_base].alulite[alulite_base].fns < 0 ? 1 : 0;
    versat_t op_b_reg = stage[versat_base].databus[shadow_reg[versat_base].alulite[alulite_base].opb];
    versat_t op_a_int = self_loop ? out : op_a_reg;

    switch (shadow_reg[versat_base].alulite[alulite_base].fns)
    {
    case ALULITE_OR:
        out = op_a_int | op_b_reg;
        break;
    case ALULITE_AND:
        out = op_a_int & op_b_reg;
        break;
    case ALULITE_CMP_SIG:
        bitset<DATAPATH_W> aux_cmp;
        aux_cmp.set(DATAPATH_W - 1, op_a_int > op_b_reg ? 1 : 0);
        out = (versat_t)aux_cmp.to_ulong();
        break;
    case ALULITE_MUX:
        out = op_a_reg < 0 ? op_b_reg : self_loop == 1 ? out : 0;
        break;
    case ALULITE_SUB:
        out = op_a_reg < 0 ? op_b_reg : op_a_int - op_b_reg;
        break;
    case ALULITE_ADD:
        out = op_a_reg < 0 ? op_b_reg : op_a_int + op_b_reg;
        break;
    case ALULITE_MAX:
        out = op_a_reg < 0 ? out : max(op_a_int, op_b_reg);
        break;
    case ALULITE_MIN:
        out = op_a_reg < 0 ? out : min(op_a_int, op_b_reg);
        break;
    default:
        break;
    }

    return out;
}

void CALULite::writeConf()
{
    conf[versat_base].alulite[alulite_base].opa = opa;
    conf[versat_base].alulite[alulite_base].opb = opb;
    conf[versat_base].alulite[alulite_base].fns = fns;
}

void CALULite::setOpA(int opa)
{
    conf[versat_base].alulite[alulite_base].opa = opa;
    this->opa = opa;
}

void CALULite::setOpB(int opb)
{
    conf[versat_base].alulite[alulite_base].opb = opb;
    this->opb = opb;
}

void CALULite::setFNS(int fns)
{
    conf[versat_base].alulite[alulite_base].fns = fns;
    this->fns = fns;
}
