#include "versat.hpp"
#include <pthread.h>


versat_t global_databus[(nSTAGE + 1) * (1 << (N_W - 1))];
versat_t* FPGA_mem;
#if nMEM > 0
int sMEMA[nMEM], sMEMA_p[nMEM], sMEMB[nMEM], sMEMB_p[nMEM];
#endif
#if nVI > 0
int sVI[nVI], sVI_p[nVI];
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

void CVersat::versat_init(int base_addr)
{

    //init versat stages
    int i;
    base_addr = 0;
    base = base_addr;
    for (i = 0; i < nSTAGE; i++)
    {
        stage[i] = CStage(base_addr + i);
        shadow_reg[i] = CStage(base_addr + i);
        buffer[i] = CStage(base_addr + i);
    }
    //prepare sel variables
    int p_offset = (1 << (N_W - 1));
    int s_cnt = 0;
#if nVI > 0
    //Vread
    for (i = 0; i < nVI; i = i + 1)
    {
        sVI[i] = s_cnt + i;
        sVI_p[i] = sVI[i] + p_offset;
    }
    s_cnt += nVI;
    //create mem
    FPGA_mem = new versat_t[1073741824 / (DATAPATH_W / 8)]; //1GB
    for (i = 0; i < 1073741824 / (DATAPATH_W / 8); i++)
    {
        FPGA_mem[i] = 0;
    }
#endif
#if nVO > 0
    int j, k;
    for (i = 0; i < nSTAGE; i++)
    {
        for (j = 0; j < nVO; j++)
        {
            for (k = 0; k < VERSAT_CONFIG_BUFFER_SIZE; k++)
                write_buffer[i][j][k] = CWrite(i, j);
        }
    }
#endif
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
void* CVersat::run_sim(void* ie)
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
        if (versat_debug == 1)
        {
            print_versat_iter(*this);
        }
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
#if nVO > 0
void CVersat::write_buffer_transfer()
{
    if (VERSAT_CONFIG_BUFFER_SIZE > 1)
    {
        for (int i = 0; i < nSTAGE; i++)
            for (int j = 0; j < nVO; j++)
            {
                shadow_reg[i].vo[j].copy_ext(write_buffer[i][j][1]);
                write_buffer[i][j][1].copy_ext(write_buffer[i][j][0]);
                write_buffer[i][j][0].copy_ext(stage[i].vo[j]);
            }
    }
    else
    {
        for (int i = 0; i < nSTAGE; i++)
            for (int j = 0; j < nVO; j++)
            {
                shadow_reg[i].vo[j].copy_ext(write_buffer[i][j][0]);
                write_buffer[i][j][0].copy_ext(stage[i].vo[j]);
            }
    }
}
#endif
#if nVI > 0
void CVersat::FU_buffer_transfer()
{
    for (int i = 0; i < nSTAGE; i++)
    {
        stage[i].reset();
        shadow_reg[i].copy(buffer[i]);
        for (int j = 0; j < nVI; j++)
        {
            shadow_reg[i].vi[j].copy_ext(stage[i].vi[j]);
        }
        buffer[i].copy(stage[i]);
    }
}
#endif

pthread_t t;
void CVersat::run()
{
    //MEMSET(base, (RUN_DONE), 1);
    run_done = 0;
    versat_iter = 0;

//update shadow register with current configuration
#if nVO > 0
    write_buffer_transfer();
#endif
#if nVI > 0
    FU_buffer_transfer();
#else
    int i = 0;

    for (i = 0; i < nSTAGE; i++)
    {
        stage[i].reset();
        shadow_reg[i].copy(stage[i]);
    }
#endif

    pthread_create(&t, NULL, run_simulator,(void*)this);
	 /*auto run_simulator= [](CVersat obj){
		 obj.run_sim();
	 };
	 thread simulator(run_simulator,*this);*/
}

void* run_simulator(void* in)
{
	CVersat* versat = (CVersat*)in;
	versat->run_sim(NULL);
	return NULL;
}

int CVersat::done()
{
    return run_done;
}

void CVersat::globalClearConf()
{
    for (int i = 0; i < nSTAGE; i++)
    {
        stage[i] = CStage(i);
    }
}
