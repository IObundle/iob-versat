#include "versat.h"

//Macro functions to use cpu interface
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

//constants
#define CONF_BASE (1<<(nMEM_W+MEM_ADDR_W+1))
#define CONF_MEM_SIZE ((int)pow(2,CONF_MEM_ADDR_W))
#define MEM_SIZE ((int)pow(2,MEM_ADDR_W))
#define RUN_DONE (1<<(nMEM_W+MEM_ADDR_W))

//
// VERSAT CLASSES
//
class CMemPort {
  public:
    int versat_base, mem_base, data_base;
    int iter, per, duty, sel, start, shift, incr, delay, in_wr, rvrs = 0, ext = 0;

    //Default constructor
    CMemPort() {
    }
    
    //Constructor with an associated base
    CMemPort (int versat_base, int i, int offset) {
      this->versat_base = versat_base;
      this->mem_base = CONF_BASE + CONF_MEM0A + (2*i+offset)*MEMP_CONF_OFFSET;
      this->data_base = (i<<MEM_ADDR_W);
    }

    //Full Configuration (includes ext and rvrs)
    void setConf(int start, int iter, int incr, int delay, int per, int duty, int sel, int shift, int in_wr, int rvrs, int ext) {
      this->iter = iter;
      this->per = per;
      this->duty = duty;
      this->sel = sel;
      this->start = start;
      this->shift = shift;
      this->incr = incr;
      this->delay = delay;
      this->in_wr = in_wr;
      this->rvrs = rvrs;
      this->ext = ext;
    }

    //Minimum Configuration
    void setConf(int start, int iter, int incr, int delay, int per, int duty, int sel, int shift, int in_wr) {
      this->iter = iter;
      this->per = per;
      this->duty = duty;
      this->sel = sel;
      this->start = start;
      this->shift = shift;
      this->incr = incr;
      this->delay = delay;
      this->in_wr = in_wr;
    }

    void writeConf() {
      MEMSET(versat_base, (mem_base + MEMP_CONF_ITER), iter);
      MEMSET(versat_base, (mem_base + MEMP_CONF_START), start);
      MEMSET(versat_base, (mem_base + MEMP_CONF_PER), per);
      MEMSET(versat_base, (mem_base + MEMP_CONF_DUTY), duty);
      MEMSET(versat_base, (mem_base + MEMP_CONF_SEL), sel);
      MEMSET(versat_base, (mem_base + MEMP_CONF_SHIFT), shift);
      MEMSET(versat_base, (mem_base + MEMP_CONF_INCR), incr);
      MEMSET(versat_base, (mem_base + MEMP_CONF_DELAY), delay);
      MEMSET(versat_base, (mem_base + MEMP_CONF_EXT), ext);
      MEMSET(versat_base, (mem_base + MEMP_CONF_IN_WR), in_wr);
      MEMSET(versat_base, (mem_base + MEMP_CONF_RVRS), rvrs);
    }
    void setIter(int iter) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_ITER), iter);
      this->iter = iter;
    } 
    void setPer(int per) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_PER), per);
      this->per = per;
    } 
    void setDuty(int duty) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_DUTY), duty);
      this->duty = duty;
    } 
    void setSel(int sel) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_SEL), sel);
      this->sel = sel;
    } 
    void setStart(int start) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_START), start);
      this->start = start;
    } 
    void setIncr(int incr) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_START), incr);
      this->incr = incr;
    } 
    void setShift(int shift) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_SHIFT), shift);
      this->shift = shift;
    } 
    void setDelay(int delay) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_DELAY), delay);
      this->delay = delay;
    } 
    void setExt(int ext) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_EXT), ext);
      this->ext = ext;
    } 
    void setRvrs(int rvrs) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_RVRS), rvrs);
      this->rvrs = rvrs;
    } 
    void setInWr(int in_wr) {
      MEMSET(versat_base, (this->mem_base + MEMP_CONF_IN_WR), in_wr);
      this->in_wr = in_wr;
    } 

    void write(int addr, int val) {
      MEMSET(versat_base, (this->data_base + addr), val);
    }
    int read(int addr) {
      return MEMGET(versat_base, (this->data_base + addr));
    }
};//end class CMEM

#if nALU>0
class CALU {
  public:
    int versat_base, alu_base;
    int opa, opb, fns;
    
    //Default constructor
    CALU() {
    }
    
    CALU(int versat_base, int i) {
      this->versat_base = versat_base;
      this->alu_base = CONF_BASE + CONF_ALU0 + i*ALU_CONF_OFFSET;
    }
    
    void setConf(int opa, int opb, int fns) {
      this->opa = opa;
      this->opb = opb;
      this->fns = fns;
    }

    void writeConf() {
      MEMSET(versat_base, (alu_base + ALU_CONF_SELA), opa);
      MEMSET(versat_base, (alu_base + ALU_CONF_SELB), opb);
      MEMSET(versat_base, (alu_base + ALU_CONF_FNS), fns);
    }
    void setOpA(int opa) {
      MEMSET(versat_base, (this->alu_base + ALU_CONF_SELA), opa);
      this->opa = opa; 
    }
    void setOpB(int opb) {
      MEMSET(versat_base, (this->alu_base + ALU_CONF_SELB), opb);
      this->opb = opb; 
    }
    void setFNS(int fns) {
      MEMSET(versat_base, (this->alu_base + ALU_CONF_FNS), fns);
      this->fns = fns; 
    }
}; //end class CALU
#endif

#if nALULITE>0
class CALULite {
  public:
    int versat_base, alulite_base;
    int opa, opb, fns;
    
    //Default constructor
    CALULite() {
    }

    CALULite(int versat_base, int i) {
      this->versat_base = versat_base;
      this->alulite_base = CONF_BASE + CONF_ALULITE0 + i*ALULITE_CONF_OFFSET;   
    }
    
    void setConf(int opa, int opb, int fns) {
      this->opa = opa;
      this->opb = opb;
      this->fns = fns;
    }

    void writeConf() {
      MEMSET(versat_base, (alulite_base + ALULITE_CONF_SELA), opa);
      MEMSET(versat_base, (alulite_base + ALULITE_CONF_SELB), opb);
      MEMSET(versat_base, (alulite_base + ALULITE_CONF_FNS), fns);
    }
    void setOpA(int opa) {
      MEMSET(versat_base, (this->alulite_base + ALULITE_CONF_SELA), opa);
      this->opa = opa; 
    }
    void setOpB(int opb) {
      MEMSET(versat_base, (this->alulite_base + ALULITE_CONF_SELB), opb);
      this->opb = opb; 
    }
    void setFNS(int fns) {
      MEMSET(versat_base, (this->alulite_base + ALULITE_CONF_FNS), fns);
      this->fns = fns; 
    }
}; //end class CALUALITE
#endif

#if nBS>0
class CBS {
  public:
    int versat_base, bs_base;
    int data, shift, fns;
    
    //Default constructor
    CBS() {
    }

    CBS(int versat_base, int i) {
      this->versat_base = versat_base;
      this->bs_base = CONF_BASE + CONF_BS0 + i*BS_CONF_OFFSET;
    }

    void setConf(int data, int shift, int fns) {
      this->data = data;
      this->shift = shift;
      this->fns = fns;
    }

    void writeConf() {
      MEMSET(versat_base, (bs_base + BS_CONF_SELD), data);
      MEMSET(versat_base, (bs_base + BS_CONF_SELS), shift);
      MEMSET(versat_base, (bs_base + BS_CONF_FNS), fns);
    }
    void setData(int data) {
      MEMSET(versat_base, (this->bs_base + BS_CONF_SELD), data);
      this->data = data; 
    }
    void setShift(int shift) {
      MEMSET(versat_base, (this->bs_base + BS_CONF_SELS), shift);
      this->shift = shift; 
    }
    void setFNS(int fns) {
      MEMSET(versat_base, (this->bs_base + BS_CONF_FNS), fns);
      this->fns = fns; 
    }
};//end class CBS
#endif

#if nMUL>0
class CMul {
  public:
    int versat_base, mul_base;
    int sela, selb, fns;

    //Default constructor
    CMul() {
    }

    CMul(int versat_base, int i) {
      this->versat_base = versat_base;
      this->mul_base = CONF_BASE + CONF_MUL0 + i*MUL_CONF_OFFSET;
    }
    
    void setConf(int sela, int selb, int fns) {
      this->sela=sela;
      this->selb=selb;
      this->fns=fns;
    }

    void writeConf() {
      MEMSET(versat_base, (mul_base  + MUL_CONF_SELA), sela);
      MEMSET(versat_base, (mul_base  + MUL_CONF_SELB), selb);
      MEMSET(versat_base, (mul_base  + MUL_CONF_FNS), fns);
    }
    void setSelA(int sela) {
      MEMSET(versat_base, (this->mul_base + MUL_CONF_SELA), sela);
      this->sela = sela; 
    }
    void setSelB(int selb) {
      MEMSET(versat_base, (this->mul_base + MUL_CONF_SELB), selb);
      this->selb = selb; 
    }
    void setFNS(int fns) {
      MEMSET(versat_base, (this->mul_base + MUL_CONF_FNS), fns);
      this->fns = fns; 
    }
};//end class CMUL
#endif

#if nMULADD>0
class CMulAdd {
  public:
    int versat_base, muladd_base;
    int sela, selb, fns, iter, per, delay;

    //Default constructor
    CMulAdd() {
    }

    CMulAdd(int versat_base, int i) {
      this->versat_base = versat_base;
      this->muladd_base = CONF_BASE + CONF_MULADD0 + i*MULADD_CONF_OFFSET;
    }

    void setConf(int sela, int selb, int fns, int iter, int per, int delay) {
      this->sela = sela;
      this->selb = selb;
      this->fns = fns;
      this->iter = iter;
      this->per = per;
      this->delay = delay;
    }

    void writeConf() {
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_SELA), sela);
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_SELB), selb);
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_FNS), fns);
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_ITER), iter);
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_PER), per);
      MEMSET(versat_base, (muladd_base  + MULADD_CONF_DELAY), delay);
    }
    void setSelA(int sela) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_SELA), sela);
      this->sela = sela; 
    }
    void setSelB(int selb) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_SELB), selb);
      this->selb = selb; 
    }
    void setFNS(int fns) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_FNS), fns);
      this->fns = fns; 
    }
    void setIter(int iter) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_ITER), iter);
      this->iter = iter; 
    }
    void setPer(int per) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_PER), per);
      this->per = per; 
    }
    void setDelay(int delay) {
      MEMSET(versat_base, (this->muladd_base + MULADD_CONF_PER), delay);
      this->delay = delay;
    }
};//end class CMULADD
#endif

class CVersat {

  public:
    int versat_base;
    //Versat Function Units
    CMemPort memA[nMEM];
    CMemPort memB[nMEM];
  #if nALU>0
    CALU alu[nALU];
  #endif
  #if nALULITE>0
    CALULite alulite[nALULITE];
  #endif
  #if nBS>0
    CBS bs[nBS];
  #endif
  #if nMUL>0
    CMul mul[nMUL];
  #endif
  #if nMULADD>0
    CMulAdd muladd[nMULADD];
  #endif

    //Default constructor
    CVersat() {
    }
    
    //Default Constructor
    CVersat(int versat_base) {

      //Define control and databus base address
      this->versat_base = versat_base;

      //Init functional units
      int i;
      for (i=0; i<nMEM; i++) memA[i] = CMemPort(versat_base, i, 0);
      for (i=0; i<nMEM; i++) memB[i] = CMemPort(versat_base, i, 1);
    #if nALU>0
      for (i=0; i<nALU; i++) alu[i] = CALU(versat_base, i);
    #endif
    #if nALULITE>0
      for (i=0; i<nALULITE; i++) alulite[i] = CALULite(versat_base, i);
    #endif
    #if nBS>0
      for (i=0; i<nBS; i++) bs[i] = CBS(versat_base, i);
    #endif
    #if nMUL>0
      for (i=0; i<nMUL; i++) mul[i] = CMul(versat_base, i);
    #endif
    #if nMULADD>0
      for (i=0; i<nMULADD; i++) muladd[i] = CMulAdd(versat_base, i);
    #endif
    }
    
    //clear Versat config                       
    void clearConf() {
      MEMSET(versat_base, (CONF_BASE + CONF_CLEAR), 0);
    }
    
#ifdef CONF_MEM_USE
    //write current config in conf_mem
    void confMemWrite(int addr) {
      if(addr < CONF_MEM_SIZE) MEMSET(versat_base, (CONF_BASE + CONF_MEM + addr), 0);
    }

    //set addressed config in conf_mem as current config
    void confMemRead(int addr) {
      if(addr < CONF_MEM_SIZE) MEMGET(versat_base, (CONF_BASE + CONF_MEM + addr));
    }
#endif
};//end class CVersat

//
//VERSAT global variables
//
static int base;
CVersat versat[nSTAGE];
int sMEMA[nMEM], sMEMA_p[nMEM], sMEMB[nMEM], sMEMB_p[nMEM];
#if nALU>0
  int sALU[nALU], sALU_p[nALU];
#endif
#if nALULITE>0
  int sALULITE[nALULITE], sALULITE_p[nALULITE];
#endif
#if nMUL>0
  int sMUL[nMUL], sMUL_p[nMUL];
#endif
#if nMULADD>0
  int sMULADD[nMULADD], sMULADD_p[nMULADD];
#endif
#if nBS>0
  int sBS[nBS], sBS_p[nBS];
#endif

//
//VERSAT FUNCTIONS
//
inline void versat_init(int base_addr) {

  //init versat stages
  int i;
  base = base_addr;
  for(i = 0; i < nSTAGE; i++) versat[i] = CVersat(base_addr + (i<<(CTR_ADDR_W-nSTAGE_W+2))); //+2 as RV32 is not byte addressable

  //prepare sel variables
  int p_offset = (1<<(N_W-1));
  int s_cnt = 0;

  //Memories
  for(i=0; i<nMEM; i=i+1){                  
    sMEMA[i] = s_cnt + 2*i;                                            
    sMEMB[i] = sMEMA[i]+1;                                          
    sMEMA_p[i] = sMEMA[i] + p_offset;
    sMEMB_p[i] = sMEMB[i] + p_offset;
  } s_cnt += 2*nMEM;                                               

#if nALU>0                                                                      
  //ALUs
  for (i=0; i<nALU; i=i+1){                                            
    sALU[i] = s_cnt+i;
    sALU_p[i] = sALU[i] + p_offset;                                               
  } s_cnt += nALU;                                       
#endif                                          
                                                                      
#if nALULITE>0
  //ALULITEs
  for (i=0; i<nALULITE; i=i+1) {                                    
    sALULITE[i] = s_cnt+i;                                        
    sALULITE_p[i] = sALULITE[i] + p_offset;                                        
  } s_cnt += nALULITE;
#endif                                                         
    
#if nMUL>0                                                                  
  //MULTIPLIERS
  for (i=0; i<nMUL; i=i+1) {                                             
    sMUL[i] = s_cnt+i;                                                
    sMUL_p[i] = sMUL[i] + p_offset; 
  } s_cnt += nMUL;                                    
#endif    
    
#if nMULADD>0                                                              
  //MULADDS
  for (i=0; i<nMULADD; i=i+1) {                                        
    sMULADD[i] = s_cnt+i;    
    sMULADD_p[i] = sMULADD[i] + p_offset;
  } s_cnt += nMULADD;                             
#endif
    
#if nBS>0
  //BARREL SHIFTERS
  for (i=0; i<nBS; i=i+1){                                             
    sBS[i] = s_cnt+i;                                                  
    sBS_p[i] = sBS[i] + p_offset;
  }
#endif                                     
}                                                                     

inline void run() {
  MEMSET(base, (RUN_DONE), 1);
}

inline int done() {
  return MEMGET(base, (RUN_DONE));
}

inline void globalClearConf() {
  MEMSET(base, (CONF_BASE + GLOBAL_CONF_CLEAR), 0);
}
