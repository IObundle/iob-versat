//versat-io
#define IO_ADDR_W 32
#define IO_SIZE_W 11
#define nIO (nVI+nVO)
#define VI_CONF_EXT_ADDR 0
#define VI_CONF_INT_ADDR 1
#define VI_CONF_SIZE 2
#define VI_CONF_ITER_A 3
#define VI_CONF_PER_A 4
#define VI_CONF_DUTY_A 5
#define VI_CONF_SHIFT_A 6
#define VI_CONF_INCR_A 7
#define VI_CONF_ITER_B 8
#define VI_CONF_PER_B 9
#define VI_CONF_DUTY_B 10
#define VI_CONF_START_B 11
#define VI_CONF_SHIFT_B 12
#define VI_CONF_INCR_B 13
#define VI_CONF_DELAY_B 14
#define VI_CONF_RVRS_B 15
#define VI_CONF_EXT_B 16
#define VI_CONF_ITER2_B 17
#define VI_CONF_PER2_B 18
#define VI_CONF_SHIFT2_B 19
#define VI_CONF_INCR2_B 20
#define VI_CONF_OFFSET 21
#define VO_CONF_EXT_ADDR 0
#define VO_CONF_INT_ADDR 1
#define VO_CONF_SIZE 2
#define VO_CONF_ITER_A 3
#define VO_CONF_PER_A 4
#define VO_CONF_DUTY_A 5
#define VO_CONF_SHIFT_A 6
#define VO_CONF_INCR_A 7
#define VO_CONF_ITER_B 8
#define VO_CONF_PER_B 9
#define VO_CONF_DUTY_B 10
#define VO_CONF_SEL_B 11
#define VO_CONF_START_B 12
#define VO_CONF_SHIFT_B 13
#define VO_CONF_INCR_B 14
#define VO_CONF_DELAY_B 15
#define VO_CONF_RVRS_B 16
#define VO_CONF_EXT_B 17
#define VO_CONF_ITER2_B 18
#define VO_CONF_PER2_B 19
#define VO_CONF_SHIFT2_B 20
#define VO_CONF_INCR2_B 21
#define VO_CONF_OFFSET 22

//xalulitedefs
#define ALULITE_ADD 0
#define ALULITE_SUB 1
#define ALULITE_CMP_SIG 2
#define ALULITE_MUX 3
#define ALULITE_MAX 4
#define ALULITE_MIN 5
#define ALULITE_OR 6
#define ALULITE_AND 7
#define ALULITE_CONF_SELA 0
#define ALULITE_CONF_SELB 1
#define ALULITE_CONF_FNS 2
#define ALULITE_CONF_OFFSET 3
#define ALULITE_LAT 2

//xconfdefs
#define CONF_MEM0A 0
#define CONF_VI0 (CONF_MEM0A + 2*nMEM*MEMP_CONF_OFFSET)
#define CONF_VO0 (CONF_VI0 + nVI*VI_CONF_OFFSET)
#define CONF_ALU0 (CONF_VO0 + nVO*VO_CONF_OFFSET)
#define CONF_ALULITE0 (CONF_ALU0 + nALU*ALU_CONF_OFFSET)
#define CONF_MUL0 (CONF_ALULITE0 + nALULITE*ALULITE_CONF_OFFSET)
#define CONF_MULADD0 (CONF_MUL0 + nMUL*MUL_CONF_OFFSET)
#define CONF_BS0 (CONF_MULADD0 + nMULADD*MULADD_CONF_OFFSET)
#define CONF_REG_ADDR_W (clog2(CONF_BS0 + nBS*BS_CONF_OFFSET))
#define CONF_CLEAR (1<<CONF_REG_ADDR_W)
#define GLOBAL_CONF_CLEAR (CONF_CLEAR+1)
#define CONF_MEM (CONF_CLEAR + (1<<(CONF_REG_ADDR_W-1)))

//xaludefs
#define ALU_ADD 0
#define ALU_SUB 1
#define ALU_CMP_SIG 2
#define ALU_MUX 3
#define ALU_MAX 4
#define ALU_MIN 5
#define ALU_OR 6
#define ALU_AND 7
#define ALU_CMP_UNS 8
#define ALU_XOR 9
#define ALU_SEXT8 10
#define ALU_SEXT16 11
#define ALU_SHIFTR_ARTH 12
#define ALU_SHIFTR_LOG 13
#define ALU_CLZ 14
#define ALU_ABS 15
#define ALU_CONF_SELA 0
#define ALU_CONF_SELB 1
#define ALU_CONF_FNS 2
#define ALU_CONF_OFFSET 3
#define ALU_LAT 2

//xmuladddefs
#define MULADD_MACC 0
#define MULADD_MSUB 1
#define MULADD_CONF_SELA 0
#define MULADD_CONF_SELB 1
#define MULADD_CONF_FNS 2
#define MULADD_CONF_ITER 3
#define MULADD_CONF_PER 4
#define MULADD_CONF_DELAY 5
#define MULADD_CONF_SHIFT 6
#define MULADD_CONF_OFFSET 7
#ifdef MULADD_COMB
#define MULADD_LAT 1
#else
#define MULADD_LAT 4
#endif

//xbsdefs
#define BS_SHR_A 0
#define BS_SHR_L 1
#define BS_SHL 2
#define BS_CONF_SELD 0
#define BS_CONF_SELS 1
#define BS_CONF_FNS 2
#define BS_CONF_OFFSET 3
#define BS_LAT 2

//xmemdefs
#define MEMP_CONF_ITER 0
#define MEMP_CONF_PER 1
#define MEMP_CONF_DUTY 2
#define MEMP_CONF_SEL 3
#define MEMP_CONF_START 4
#define MEMP_CONF_SHIFT 5
#define MEMP_CONF_INCR 6
#define MEMP_CONF_DELAY 7
#define MEMP_CONF_RVRS 8
#define MEMP_CONF_EXT 9
#define MEMP_CONF_IN_WR 10
#define MEMP_CONF_ITER2 11
#define MEMP_CONF_PER2 12
#define MEMP_CONF_SHIFT2 13
#define MEMP_CONF_INCR2 14
#define MEMP_CONF_OFFSET 15
#define MEMP_LAT 3

//xmuldefs
#define MUL_HI 1
#define MUL_DIV2_LO 2
#define MUL_DIV2_HI 3
#define MUL_CONF_SELA 0
#define MUL_CONF_SELB 1
#define MUL_CONF_FNS 2
#define MUL_CONF_OFFSET 3
#define MUL_LAT 3

int32_t AddUpdateFunction(FUInstance* instance){
    int32_t a = instance->inputs[0]->output;
	int32_t b = instance->inputs[1]->output;

	return a + b;
}

struct MemGen4{
    int iter, per, duty, sel, start, shift, incr, delay, in_wr;
    int rvrs, ext, iter2, per2, shift2, incr2;
    bool done;
    int run_delay;
    int enable;
    int loop1, loop2, loop3, loop4;
    uint32_t pos;
    uint32_t pos2;
    int duty_cnt;
    int16_t* mem;
    bool debug;
};

int32_t MemStartFunction(FUInstance* instance){
	MemGen4* s = (MemGen4*) instance->extraData;

    s->done = 0;
    s->pos = s->start;
    s->pos2 = s->start;
    if (s->duty == 0)
        s->duty = s->per;
    s->run_delay = s->delay;

    return 0;
}

int32_t MemUpdateFunction(FUInstance* instance){
	MemGen4* s = (MemGen4*) instance->extraData;

	if(s->run_delay){
		s->run_delay--;
		return instance->output;
	}

	if(s->done){
		return instance->output;
	}

	uint32_t aux;
	if (s->iter2 == 0 && s->per2 == 0)
    {
        if (s->loop2 < s->iter)
        {
            if (s->loop1 < s->per)
            {
                s->loop1++;
                s->enable = 0;
                if (s->duty_cnt < s->duty)
                {
                    s->enable = 1;
                    aux = s->pos;
                    s->duty_cnt++;
                    s->pos += s->incr;
                }
            }
            if (s->loop1 == s->per)
            {
                s->loop1 = 0;
                s->duty_cnt = 0;
                s->loop2++;
                s->pos += s->shift;
            }
        }
        if (s->loop2 == s->iter)
        {
            s->loop2 = 0;
            s->done = 1;
        }
    }
    else
    {
        if (s->loop4 < s->iter2)
        {
            if (s->loop3 < s->per2)
            {
                if (s->loop2 < s->iter)
                {
                    if (s->loop1 < s->per)
                    {
                        s->loop1++;
                        s->enable = 0;
                        if (s->duty_cnt < s->duty)
                        {
                            s->enable = 1;
                            aux = s->pos;
                            s->duty_cnt++;
                            s->pos += s->incr;
                        }
                    }
                    if (s->loop1 == s->per)
                    {
                        s->loop1 = 0;
                        s->loop2++;
                        s->duty_cnt = 0;
                        s->pos += s->shift;
                    }
                }
                if (s->loop2 == s->iter)
                {
                    s->loop2 = 0;
                    s->pos2 += s->incr2;
                    s->pos = s->pos2;
                    s->loop3++;
                }
            }
            if (s->loop3 == s->per2)
            {
                s->pos2 += s->shift2;
                s->pos = s->pos2;
                s->loop3 = 0;
                s->loop4++;
            }
        }
        if (s->loop4 == s->iter2)
        {
            s->done = 1;
        }
    }
    
    int32_t out;
    if(s->in_wr){
        if(s->enable){
            s->mem[aux] = instance->inputs[0]->output;
        }
    } else {
        out = s->mem[aux];    
    }

    if(s->debug){
        printf("[mem] %d:%d\n",aux,out);
    }

    return out;
}

struct MulAddGen2{
    int32_t acc, acc_w;
    int done, duty;
    int duty_cnt, enable;
    int shift_addr, incr, aux, pos;
    int loop2, loop1, cnt_addr;
    int run_delay;
    int sela, selb, fns, iter, per, delay, shift;
    bool debug;
};

int32_t MulAddStartFunction(FUInstance* instance){
    MulAddGen2* s = (MulAddGen2*) instance->extraData;

    s->run_delay = s->delay;

    //set addrgen counter variables
    s->incr = 1;
    s->loop1 = 0;
    s->duty = s->per;
    s->loop2 = 0;
    s->pos = 0;
    s->shift_addr = -s->per;
    s->duty_cnt = 0;
    s->cnt_addr = 0;
    s->done = 0;

    return 0;
}

int32_t MulAddUpdateFunction(FUInstance* instance){
    int32_t opa = instance->inputs[0]->output;
    int32_t opb = instance->inputs[1]->output;

    MulAddGen2* s = (MulAddGen2*) instance->extraData;

    //check for delay
    if (s->run_delay > 0)
    {
        s->run_delay--;
        return instance->output;
    }

    if (s->loop2 < s->iter)
    {
        if (s->loop1 < s->per)
        {
            s->loop1++;
            s->enable = 0;
            if (s->duty_cnt < s->duty)
            {
                s->enable = 1;
                s->aux = s->pos;
                s->duty_cnt++;
                s->pos += s->incr;
            }
        }
        if (s->loop1 == s->per)
        {
            s->loop1 = 0;
            s->duty_cnt = 0;
            s->loop2++;
            s->pos += s->shift_addr;
        }
    }
    if (s->loop2 == s->iter)
    {
        s->loop2 = 0;
        s->done = 1;
    }
    s->cnt_addr = s->aux;

    //select acc_w value
    s->acc_w = (s->cnt_addr == 0) ? 0 : s->acc;

    //perform MAC operation
    int32_t result_mult = opa * opb;
    if (s->fns == MULADD_MACC)
    {
        s->acc = s->acc_w + result_mult;
    }
    else
    {
        s->acc = s->acc_w - result_mult;
    }
    int32_t out = (int32_t)(s->acc >> s->shift);

    if(s->debug)
        printf("[muladd] %d %d %d: %d\n",s->cnt_addr,opa,opb,out);

    return out;
}