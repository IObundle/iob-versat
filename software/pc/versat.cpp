#include "versat.hpp"

inline void versat_init(int base_addr)
{

    //init versat stages
    int i;
    base_addr = 0;
    base = base_addr;
    for (i = 0; i < nSTAGE; i++)
        stage[i] = CStage(base_addr + i);

    //prepare sel variables
    int p_offset = (1 << (N_W - 1));
    int s_cnt = 0;

    //Memories
    for (i = 0; i < nMEM; i = i + 1)
    {
        sMEMA[i] = s_cnt + 2 * i;
        sMEMB[i] = sMEMA[i] + 1;
        sMEMA_p[i] = sMEMA[i] + p_offset;
        sMEMB_p[i] = sMEMB[i] + p_offset;
    }
    s_cnt += 2 * nMEM;

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

void run_sim()
{
    run_done = 1;
    int i = 0;
    //put simulation here

    //set run start for all FUs
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i].start_all_FUs();
    }

    //main run loop
    while (run_done)
    {
        //calculate new outputs
        for (i = 0; i < nSTAGE; i++)
        {
            stage[i].output_all_FUs();
        }

        //update output buffers and datapath
        for (i = 0; i < nSTAGE; i++)
        {
            stage[i].update_all_FUs();
        }

        //TO DO: check for run finish
        //set run_done to 0
    }
}

inline void run()
{
    //MEMSET(base, (RUN_DONE), 1);
    int i = 0;

    run_done = 0;

    //update shadow register with current configuration
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i].update_shadow_reg();
    }

    std::thread ti(run_sim);
}

inline int done()
{
    return run_done;
}

inline void globalClearConf()
{
    memset(&conf, 0, sizeof(conf));
    for (int i = 0; i < nSTAGE; i++)
    {
        conf[i] = CStage(i);
    }
}