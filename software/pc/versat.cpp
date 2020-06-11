#include "versat.hpp"
#include <pthread.h>
void versat_init(int base_addr)
{

    //init versat stages
    int i;
    base_addr = 0;
    base = base_addr;
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i] = CStage(base_addr + i);
        conf[i] = CStage(base_addr + i);
        shadow_reg[i] = CStage(base_addr + i);
    }
    //prepare sel variables
    int p_offset = (1 << (N_W - 1));
    int s_cnt = 0;
#if nMEM > 0
    //Memories
    for (i = 0; i < nMEM; i = i + 1)
    {
        sMEMA[i] = s_cnt + 2 * i;
        sMEMB[i] = sMEMA[i] + 1;
        sMEMA_p[i] = sMEMA[i] + p_offset;
        sMEMB_p[i] = sMEMB[i] + p_offset;
    }
    s_cnt += 2 * nMEM;
#endif
#if nALU > 0
    //ALUs
    for (i = 0; i < nALU; i = i + 1)
    {
        sALU[i] = s_cnt + i;
        sALU_p[i] = sALU[i] + p_offset;
    }
    s_cnt += nALU;
#endif

#if nALULITE > 0
    //ALULITEs
    for (i = 0; i < nALULITE; i = i + 1)
    {
        sALULITE[i] = s_cnt + i;
        sALULITE_p[i] = sALULITE[i] + p_offset;
    }
    s_cnt += nALULITE;
#endif

#if nMUL > 0
    //MULTIPLIERS
    for (i = 0; i < nMUL; i = i + 1)
    {
        sMUL[i] = s_cnt + i;
        sMUL_p[i] = sMUL[i] + p_offset;
    }
    s_cnt += nMUL;
#endif

#if nMULADD > 0
    //MULADDS
    for (i = 0; i < nMULADD; i = i + 1)
    {
        sMULADD[i] = s_cnt + i;
        sMULADD_p[i] = sMULADD[i] + p_offset;
    }
    s_cnt += nMULADD;
#endif

#if nMULADDLITE > 0
    //MULADDLITES
    for (i = 0; i < nMULADDLITE; i = i + 1)
    {
        sMULADDLITE[i] = s_cnt + i;
        sMULADDLITE_p[i] = sMULADDLITE[i] + p_offset;
    }
    s_cnt += nMULADDLITE;
#endif

#if nBS > 0
    //BARREL SHIFTERS
    for (i = 0; i < nBS; i = i + 1)
    {
        sBS[i] = s_cnt + i;
        sBS_p[i] = sBS[i] + p_offset;
    }
#endif
}

int iter = 0;
void run_sim()
{
    int i = 0;
    //put simulation here
    bool run_mem = 0;
    bool run_mem_stage[nSTAGE] = {0};
    bool aux;
    //set run start for all FUs
    for (i = 0; i < nSTAGE; i++)
    {
        shadow_reg[i].start_all_FUs();
    }

    //main run loop
    while (!run_mem)
    {
        //calculate new outputs
        for (i = 0; i < nSTAGE; i++)
        {
            shadow_reg[i].output_all_FUs();
        }

        //update output buffers and datapath
        for (i = 0; i < nSTAGE; i++)
        {
            shadow_reg[i].update_all_FUs();
        }

        //TO DO: check for run finish
        //set run_done to 0
        for (i = 0; i < nSTAGE; i++)
        {
            run_mem_stage[i] = shadow_reg[i].done();
        }
        aux = run_mem_stage[0];
        for (i = 1; i < nSTAGE; i++)
        {
            aux = aux && run_mem_stage[i];
        }
        run_mem = aux;
        iter++;
    }
    run_done = 1;
}

void run()
{
    //MEMSET(base, (RUN_DONE), 1);
    int i = 0;
    run_done = 0;
    iter = 0;

    //update shadow register with current configuration
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i].reset();
        //shadow_reg[i] = stage[i];
        stage[i].update_shadow_reg();
    }

    thread ti(run_sim);
    ti.join();
    printf("It took %d Versat Clock Cycles\n", iter);
}

int done()
{
    return run_done;
}

void globalClearConf()
{
    memset(&conf, 0, sizeof(conf));
    for (int i = 0; i < nSTAGE; i++)
    {
        conf[i] = CStage(i);
    }
}

int base;
CStage stage[nSTAGE];
CStage conf[nSTAGE];
CStage shadow_reg[nSTAGE];
CMem versat_mem[nSTAGE][nMEM];
int run_done = 0;

/*databus vector
stage 0 is repeated in the start and at the end
stage order in databus
[ 0 | nSTAGE-1 | nSTAGE-2 | ... | 2  | 1 | 0 ]
^                                    ^
|                                    |
stage 0 databus                      stage 1 databus

*/
versat_t global_databus[(nSTAGE + 1) * N];
#if nMEM > 0
int sMEMA[nMEM], sMEMA_p[nMEM], sMEMB[nMEM], sMEMB_p[nMEM];
#endif
#if nALU > 0
int sALU[nALU], sALU_p[nALU];
#endif
#if nALULITE > 0
int sALULITE[nALULITE], sALULITE_p[nALULITE];
#endif
#if nMUL > 0
int sMUL[nMUL], sMUL_p[nMUL];
#endif
#if nMULADD > 0
int sMULADD[nMULADD], sMULADD_p[nMULADD];
#endif
#if nMULADDLITE > 0
int sMULADDLITE[nMULADDLITE], sMULADDLITE_p[nMULADDLITE];
#endif
#if nBS > 0
int sBS[nBS], sBS_p[nBS];
#endif