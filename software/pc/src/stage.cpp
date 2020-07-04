#include "stage.hpp"

CStage::CStage()
{
}
//Default Constructor
CStage::CStage(int versat_base)
{

    //Define control and databus base address
    this->versat_base = versat_base;

    //set databus pointer
    this->databus = &(global_databus[(1 << (N_W - 1)) * (nSTAGE - versat_base)]);
    //special case for first stage
    if (versat_base == 0)
    {
        this->databus = global_databus;
    }

    //Init functional units
    int i;
#if nMEM > 0
    for (i = 0; i < nMEM; i++)
        memA[i] = CMemPort(versat_base, i, 0, databus);
    for (i = 0; i < nMEM; i++)
        memB[i] = CMemPort(versat_base, i, 1, databus);
#endif
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i] = CALU(versat_base, i, databus);
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i] = CALULite(versat_base, i, databus);
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i] = CBS(versat_base, i, databus);
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i] = CMul(versat_base, i, databus);
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i] = CMulAdd(versat_base, i, databus);
#endif
}

//clear Versat config
void CStage::clearConf()
{
    int i = versat_base;
    stage[i] = CStage(i);
}

#ifdef CONF_MEM_USE
//write current config in conf_mem
void CStage::confMemWrite(int addr)
{
    if (addr < CONF_MEM_SIZE)
        MEMSET(versat_base, (CONF_BASE + CONF_MEM + addr), 0);
}

//set addressed config in conf_mem as current config
void CStage::confMemRead(int addr)
{
    if (addr < CONF_MEM_SIZE)
        MEMGET(versat_base, (CONF_BASE + CONF_MEM + addr));
}
#endif

//update shadow register with current configuration
void CStage::update_shadow_reg()
{
    int i = 0;
#if nMEM > 0
    for (i = 0; i < nMEM; i++)
    {
        //memA[i].update_shadow_reg_MEM();
        shadow_reg[versat_base].memA[i].iter = stage[versat_base].memA[i].iter;
        shadow_reg[versat_base].memA[i].start = stage[versat_base].memA[i].start;
        shadow_reg[versat_base].memA[i].per = stage[versat_base].memA[i].per;
        shadow_reg[versat_base].memA[i].duty = stage[versat_base].memA[i].duty;
        shadow_reg[versat_base].memA[i].sel = stage[versat_base].memA[i].sel;
        shadow_reg[versat_base].memA[i].shift = stage[versat_base].memA[i].shift;
        shadow_reg[versat_base].memA[i].incr = stage[versat_base].memA[i].incr;
        shadow_reg[versat_base].memA[i].delay = stage[versat_base].memA[i].delay;
        shadow_reg[versat_base].memA[i].ext = stage[versat_base].memA[i].ext;
        shadow_reg[versat_base].memA[i].in_wr = stage[versat_base].memA[i].in_wr;
        shadow_reg[versat_base].memA[i].rvrs = stage[versat_base].memA[i].rvrs;
        shadow_reg[versat_base].memA[i].iter2 = stage[versat_base].memA[i].iter2;
        shadow_reg[versat_base].memA[i].per2 = stage[versat_base].memA[i].per2;
        shadow_reg[versat_base].memA[i].shift2 = stage[versat_base].memA[i].shift2;
        shadow_reg[versat_base].memA[i].incr2 = stage[versat_base].memA[i].incr2;
    }
    for (i = 0; i < nMEM; i++)
    {
        //memB[i].update_shadow_reg_MEM();
        shadow_reg[versat_base].memB[i].iter = stage[versat_base].memB[i].iter;
        shadow_reg[versat_base].memB[i].start = stage[versat_base].memB[i].start;
        shadow_reg[versat_base].memB[i].per = stage[versat_base].memB[i].per;
        shadow_reg[versat_base].memB[i].duty = stage[versat_base].memB[i].duty;
        shadow_reg[versat_base].memB[i].sel = stage[versat_base].memB[i].sel;
        shadow_reg[versat_base].memB[i].shift = stage[versat_base].memB[i].shift;
        shadow_reg[versat_base].memB[i].incr = stage[versat_base].memB[i].incr;
        shadow_reg[versat_base].memB[i].delay = stage[versat_base].memB[i].delay;
        shadow_reg[versat_base].memB[i].ext = stage[versat_base].memB[i].ext;
        shadow_reg[versat_base].memB[i].in_wr = stage[versat_base].memB[i].in_wr;
        shadow_reg[versat_base].memB[i].rvrs = stage[versat_base].memB[i].rvrs;
        shadow_reg[versat_base].memB[i].iter2 = stage[versat_base].memB[i].iter2;
        shadow_reg[versat_base].memB[i].per2 = stage[versat_base].memB[i].per2;
        shadow_reg[versat_base].memB[i].shift2 = stage[versat_base].memB[i].shift2;
        shadow_reg[versat_base].memB[i].incr2 = stage[versat_base].memB[i].incr2;
    }
#endif
#if nALU > 0
    for (i = 0; i < nALU; i++)
    {
        //alu[i].update_shadow_reg_ALU();
        shadow_reg[versat_base].alu[i].opa = stage[versat_base].alu[i].opa;
        shadow_reg[versat_base].alu[i].opb = stage[versat_base].alu[i].opb;
        shadow_reg[versat_base].alu[i].fns = stage[versat_base].alu[i].fns;
    }
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
    {
        //alulite[i].update_shadow_reg_ALULite();
        shadow_reg[versat_base].alulite[i].opa = stage[versat_base].alulite[i].opa;
        shadow_reg[versat_base].alulite[i].opb = stage[versat_base].alulite[i].opb;
        shadow_reg[versat_base].alulite[i].fns = stage[versat_base].alulite[i].fns;
    }
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
    {
        //bs[i].update_shadow_reg_BS();
        shadow_reg[versat_base].bs[i].data = stage[versat_base].bs[i].data;
        shadow_reg[versat_base].bs[i].fns = stage[versat_base].bs[i].fns;
        shadow_reg[versat_base].bs[i].shift = stage[versat_base].bs[i].shift;
    }
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
    {
        //mul[i].update_shadow_reg_Mul();
        shadow_reg[versat_base].mul[i].sela = stage[versat_base].mul[i].sela;
        shadow_reg[versat_base].mul[i].selb = stage[versat_base].mul[i].selb;
        shadow_reg[versat_base].mul[i].fns = stage[versat_base].mul[i].fns;
    }
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
    {
        //muladd[i].update_shadow_reg_MulAdd();
        shadow_reg[versat_base].muladd[i].sela = stage[versat_base].muladd[i].sela;
        shadow_reg[versat_base].muladd[i].selb = stage[versat_base].muladd[i].selb;
        shadow_reg[versat_base].muladd[i].fns = stage[versat_base].muladd[i].fns;
        shadow_reg[versat_base].muladd[i].iter = stage[versat_base].muladd[i].iter;
        shadow_reg[versat_base].muladd[i].per = stage[versat_base].muladd[i].per;
        shadow_reg[versat_base].muladd[i].delay = stage[versat_base].muladd[i].delay;
        shadow_reg[versat_base].muladd[i].shift = stage[versat_base].muladd[i].shift;
    }
#endif
}

//set run start on all FUs
void CStage::start_all_FUs()
{
    int i = 0;
    for (i = 0; i < nMEM; i++)
        memA[i].start_run();
    for (i = 0; i < nMEM; i++)
        memB[i].start_run();
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i].start_run();
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i].start_run();
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i].start_run();
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i].start_run();
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i].start_run();
#endif
}

//set update output buffers on all FUs
void CStage::update_all_FUs()
{
    int i = 0;
    for (i = 0; i < nMEM; i++)
        memA[i].update();
    for (i = 0; i < nMEM; i++)
        memB[i].update();
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i].update();
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i].update();
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i].update();
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i].update();
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i].update();
#endif
}

//calculate new output on all FUs
void CStage::output_all_FUs()
{
    int i = 0;
    for (i = 0; i < nMEM; i++)
        memA[i].output();
    for (i = 0; i < nMEM; i++)
        memB[i].output();
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i].output();
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i].output();
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i].output();
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i].output();
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i].output();
#endif
}

bool CStage::done()
{
    bool auxA, auxB;
    auxA = memA[0].done;
    auxB = memB[0].done;
    for (int i = 1; i < nMEM; i++)
    {
        auxA = auxA && memA[i].done;
        auxB = auxB && memB[i].done;
    }
    return auxA && auxB;
}

void CStage::reset()
{
    for (int i = 0; i < 2 * N; i++)
    {
        databus[i] = 0;
    }
    for (int i = 0; i < nMEM; i++)
    {
        memA[i].done = 0;
        memB[i].done = 0;
    }
}
string CStage::info()
{
    int i = 0;
    string ver = "nStage " + to_string(versat_base) + "\n";
#if nMEM > 0
    for (i = 0; i < nMEM; i++)
    {
        ver += memA[i].info();
        ver += memB[i].info();
    }
#endif
#if nALU > 0
    for (i = 0; i < nALU; i++)
    {
        ver += alu[i].info();
    }
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
    {
        ver += alulite[i].info();
    }
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
    {
        ver += bs[i].info();
    }
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
    {
        ver += mul[i].info();
    }
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
    {
        ver += muladd[i].info();
    }
#endif

    return ver;
}

string CStage::info_iter()
{
    int i = 0;
    string ver = "nStage " + to_string(versat_base) + "\n";
#if nMEM > 0
    for (i = 0; i < nMEM; i++)
    {
        ver += memA[i].info_iter();
        ver += memB[i].info_iter();
    }
#endif
#if nALU > 0
    for (i = 0; i < nALU; i++)
    {
        ver += alu[i].info();
    }
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
    {
        ver += alulite[i].info_iter();
    }
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
    {
        ver += bs[i].info();
    }
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
    {
        ver += mul[i].info();
    }
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
    {
        ver += muladd[i].info_iter();
    }
#endif
    return ver;
}
