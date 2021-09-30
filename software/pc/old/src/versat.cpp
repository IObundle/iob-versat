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

#if nBS > 0
    //BARREL SHIFTERS
    for (i = 0; i < nBS; i = i + 1)
    {
        sBS[i] = s_cnt + i;
        sBS_p[i] = sBS[i] + p_offset;
    }
#endif
}

int versat_iter = 0;
void *run_sim(void *ie)
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
        versat_iter++;
    }
    run_done = 1;
    return NULL;
}

pthread_t t;
void run()
{
    //MEMSET(base, (RUN_DONE), 1);
    int i = 0;
    run_done = 0;
    versat_iter = 0;

    //update shadow register with current configuration
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i].reset();
        shadow_reg[i].copy(stage[i]);
    }

    pthread_create(&t, NULL, run_sim, NULL);
}

int done()
{
    return run_done;
}

void globalClearConf()
{
    for (int i = 0; i < nSTAGE; i++)
    {
        shadow_reg[i] = CStage(i);
        stage[i] = CStage(i);
    }
}

versat_t global_databus[(nSTAGE + 1) * (1 << (N_W - 1))];
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

#if nBS > 0
int sBS[nBS], sBS_p[nBS];
#endif

int base;
CStage stage[nSTAGE];
CStage shadow_reg[nSTAGE];
CMem versat_mem[nSTAGE][nMEM];
int run_done = 0;