#include "versat.hpp"

CStage::CStage()
{
}
//Default Constructor
CStage::CStage(int versat_base)
{

    //Define control and databus base address
    this->versat_base = versat_base;

    //set databus pointer
    this->databus = &(global_databus[N * (nSTAGE - versat_base)]);
    //special case for first stage
    if (versat_base == 0)
    {
        this->databus = global_databus;
    }

    //Init functional units
    int i;
#if nMEM > 0
    for (i = 0; i < nMEM; i++)
        memA[i] = CMemPort(versat_base, i, 0);
    for (i = 0; i < nMEM; i++)
        memB[i] = CMemPort(versat_base, i, 1);
#endif
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i] = CALU(versat_base, i);
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i] = CALULite(versat_base, i);
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i] = CBS(versat_base, i);
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i] = CMul(versat_base, i);
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i] = CMulAdd(versat_base, i);
#endif
#if nMULADDLITE > 0
    for (i = 0; i < nMULADDLITE; i++)
        muladdlite[i] = CMulAddLite(versat_base, i);
#endif
}

//clear Versat config
void CStage::clearConf()
{
    int i = versat_base;
    conf[i] = CStage(i);
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

    for (i = 0; i < nMEM; i++)
        memA[i].update_shadow_reg_MEM();
    for (i = 0; i < nMEM; i++)
        memB[i].update_shadow_reg_MEM();
#if nALU > 0
    for (i = 0; i < nALU; i++)
        alu[i].update_shadow_reg_ALU();
#endif
#if nALULITE > 0
    for (i = 0; i < nALULITE; i++)
        alulite[i].update_shadow_reg_ALULite();
#endif
#if nBS > 0
    for (i = 0; i < nBS; i++)
        bs[i].update_shadow_reg_BS();
#endif
#if nMUL > 0
    for (i = 0; i < nMUL; i++)
        mul[i].update_shadow_reg_Mul();
#endif
#if nMULADD > 0
    for (i = 0; i < nMULADD; i++)
        muladd[i].update_shadow_reg_MulAdd();
#endif
#if nMULADDLITE > 0
    for (i = 0; i < nMULADDLITE; i++)
        muladdlite[i].update_shadow_reg_MulAddLite();
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
#if nMULADDLITE > 0
    for (i = 0; i < nMULADDLITE; i++)
        muladdlite[i].start_run();
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
#if nMULADDLITE > 0
    for (i = 0; i < nMULADDLITE; i++)
        muladdlite[i].update();
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
#if nMULADDLITE > 0
    for (i = 0; i < nMULADDLITE; i++)
        muladdlite[i].output();
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
        //string ver2 = ver + memA[i].info();
        //string ver = ver2 + memB[i].info();
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
#if nMULADDLITE > 0
    for (i = 0; i < nYOLO; i++)
    {
        ver += muladdlite[i].info();
    }
#endif
    return ver;
}