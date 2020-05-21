#ifndef VERSAT_SIM // include guard
#define VERSAT_SIM
#include <iostream>
#include <thread>
#include "versat.h"
#include <string.h>

#define nFU nALU+nALULITE+nMEM+nMUL+nMULADD+nBS

#ifndef DATAPATH_W
#define DATAPATH_W 16
#endif

#if DATAPATH_W == 16
typedef int16_t versat_t;
typedef int32_t mul_t;
typedef uint32_t shift;
#elif DATAPATH_W == 8
typedef int8_t versat_t;
typedef int16_t mul_t;
typedef uint16_t shift;

#else
typedef int32_t versat_t;
typedef int64_t mul_t;
typedef uint64_t shift;

#endif

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
    int iter, per, duty, sel, start, shift, incr, delay, in_wr;
    int rvrs = 0, ext = 0, iter2 = 0, per2 = 0, shift2 = 0, incr2 = 0;

    //Default constructor
    CMemPort() {
    }
    
    versat_t output(){
      return 0;
    }
    //Constructor with an associated base
    CMemPort (int versat_base, int i, int offset) {
      this->versat_base = versat_base;
      this->mem_base = i;
      this->data_base = offset;
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

    void setConf(int iter2, int per2, int shift2, int incr2) {
      this->iter2 = iter2;
      this->per2 = per2;
      this->shift2 = shift2;
      this->incr2 = incr2;
    }

    void writeConf() {
      if(data_base==1) {
            conf[versat_base].memB[mem_base].iter=iter;
            conf[versat_base].memB[mem_base].start=start;
            conf[versat_base].memB[mem_base].per=per;
            conf[versat_base].memB[mem_base].duty=duty;
            conf[versat_base].memB[mem_base].sel=sel;
            conf[versat_base].memB[mem_base].shift=shift;
            conf[versat_base].memB[mem_base].incr=incr;
            conf[versat_base].memB[mem_base].delay=delay;
            conf[versat_base].memB[mem_base].ext=ext;
            conf[versat_base].memB[mem_base].in_wr=in_wr;
            conf[versat_base].memB[mem_base].rvrs=rvrs;
            conf[versat_base].memB[mem_base].iter2=iter2;
            conf[versat_base].memB[mem_base].per2=per2;
            conf[versat_base].memB[mem_base].shift2=shift2;
            conf[versat_base].memB[mem_base].incr2=incr2;
      }            
      else
      {
            conf[versat_base].memA[mem_base].iter=iter;
            conf[versat_base].memA[mem_base].start=start;
            conf[versat_base].memA[mem_base].per=per;
            conf[versat_base].memA[mem_base].duty=duty;
            conf[versat_base].memA[mem_base].sel=sel;
            conf[versat_base].memA[mem_base].shift=shift;
            conf[versat_base].memA[mem_base].incr=incr;
            conf[versat_base].memA[mem_base].delay=delay;
            conf[versat_base].memA[mem_base].ext=ext;
            conf[versat_base].memA[mem_base].in_wr=in_wr;
            conf[versat_base].memA[mem_base].rvrs=rvrs;
            conf[versat_base].memA[mem_base].iter2=iter2;
            conf[versat_base].memA[mem_base].per2=per2;
            conf[versat_base].memA[mem_base].shift2=shift2;
            conf[versat_base].memA[mem_base].incr2=incr2;
      }
    }
    void setIter(int iter) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].iter=iter;
      else
      {
            conf[versat_base].memA[mem_base].iter=iter;
      }
      this->iter = iter;
    } 
    void setPer(int per) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].per=per;
      else
      {
            conf[versat_base].memA[mem_base].per=per;
      }
      this->per = per;
    } 
    void setDuty(int duty) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].duty=duty;
      else
      {
            conf[versat_base].memA[mem_base].duty=duty;
      }
      this->duty = duty;
    } 
    void setSel(int sel) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].sel=sel;
      else
      {
            conf[versat_base].memA[mem_base].sel=sel;
      }
      this->sel = sel;
    } 
    void setStart(int start) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].start=start;
      else
      {
            conf[versat_base].memA[mem_base].start=start;
      }
      this->start = start;
    } 
    void setIncr(int incr) {
      if(data_base==1)
      conf[versat_base].memB[mem_base].incr=incr;
      else
      {
      conf[versat_base].memA[mem_base].incr=incr;
      }
    this->incr = incr;
    } 
    void setShift(int shift) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].shift=shift;
      else
      {
            conf[versat_base].memA[mem_base].shift=shift;
      }
      this->shift = shift;
    } 
    void setDelay(int delay) {
      if(data_base==1)
      conf[versat_base].memB[mem_base].delay=delay;
      else
      {
            conf[versat_base].memA[mem_base].delay=delay;
      }
      this->delay = delay;
    } 
    void setExt(int ext) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].ext=ext;
      else
      {
            conf[versat_base].memA[mem_base].ext=ext;
      }
      this->ext = ext;
    } 
    void setRvrs(int rvrs) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].rvrs=rvrs;
      else
      {
            conf[versat_base].memA[mem_base].rvrs=rvrs;
      }
      this->rvrs = rvrs;
    } 
    void setInWr(int in_wr) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].in_wr=in_wr;
      else
      {
            conf[versat_base].memA[mem_base].in_wr=in_wr;
      }
      this->in_wr = in_wr;
    } 
    void setIter2(int iter2) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].iter2=iter2;
      else
      {
            conf[versat_base].memA[mem_base].iter2=iter2;
      }
      this->iter2 = iter2;
    } 
    void setPer2(int per2) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].per2=per2;
      else
      {
            conf[versat_base].memA[mem_base].per2=per2;
      }
      this->per2 = per2;
    } 
    void setIncr2(int incr) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].incr2=incr2;
      else
      {
            conf[versat_base].memA[mem_base].incr2=incr2;
      }
      this->incr2 = incr2;
    } 
    void setShift2(int shift2) {
      if(data_base==1)
            conf[versat_base].memB[mem_base].shift2=shift2;
      else
      {
            conf[versat_base].memA[mem_base].shift2=shift2;
      }
      this->shift2 = shift2;
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
    versat_t output(){
      return 0;
    }
    CALU(int versat_base, int i) {
      this->versat_base = versat_base;
      this->alu_base = i;
    }
    
    void setConf(int opa, int opb, int fns) {
      this->opa = opa;
      this->opb = opb;
      this->fns = fns;
    }

    void writeConf() {
      conf[versat_base].alu[alu_base].opa=opa;
      conf[versat_base].alu[alu_base].opb=opb;
      conf[versat_base].alu[alu_base].fns=fns;
    }
    void setOpA(int opa) {
      conf[versat_base].alu[alu_base].opa=opa;
      this->opa = opa; 
    }
    void setOpB(int opb) {
      conf[versat_base].alu[alu_base].opb=opb;
      this->opb = opb; 
    }
    void setFNS(int fns) {
      conf[versat_base].alu[alu_base].fns=fns;
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

    versat_t output(){
      return 0;
    }

    CALULite(int versat_base, int i) {
      this->versat_base = versat_base;
      this->alulite_base =i;   
    }
    
    void setConf(int opa, int opb, int fns) {
      this->opa = opa;
      this->opb = opb;
      this->fns = fns;
    }

    void writeConf() {
      conf[versat_base].alulite[alulite_base].opa=opa;
      conf[versat_base].alulite[alulite_base].opb=opb;
      conf[versat_base].alulite[alulite_base].fns=fns;
    }
    void setOpA(int opa) {
      conf[versat_base].alulite[alulite_base].opa=opa;
      this->opa = opa; 
    }
    void setOpB(int opb) {
      conf[versat_base].alulite[alulite_base].opb=opb;
      this->opb = opb; 
    }
    void setFNS(int fns) {
      conf[versat_base].alulite[alulite_base].fns=fns;
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

    versat_t output()
    {
      return 0;
    }

    CBS(int versat_base, int i) {
      this->versat_base = versat_base;
      this->bs_base =i;
    }

    void setConf(int data, int shift, int fns) {
      this->data = data;
      this->shift = shift;
      this->fns = fns;
    }

    void writeConf() {
      conf[versat_base].bs[bs_base].data=data;
      conf[versat_base].bs[bs_base].shift=shift;
      conf[versat_base].bs[bs_base].fns=fns;
    }
    void setData(int data) {
      conf[versat_base].bs[bs_base].data=data;
      this->data = data; 
    }
    void setShift(int shift) {
      conf[versat_base].bs[bs_base].shift=shift;
      this->shift = shift; 
    }
    void setFNS(int fns) {
      conf[versat_base].bs[bs_base].fns=fns;
      this->fns = fns; 
    }
};//end class CBS
#endif

#if nMUL>0
class CMul {
  private:
  versat_t opa,opb;
  public:
    int versat_base, mul_base;
    int sela, selb, fns;
    versat_t in_a[nFU-1],in_b[nFU-1];
    //Default constructor
    CMul() {
    }

    CMul(int versat_base, int i) {
      this->versat_base = versat_base;
      this->mul_base =i;
    }

    versat_t output() {
      for(int i=0;i<nFU-1;i++)
      {
        if (sela==i)
          opa=in_a[i];
        if (selb==i)
          opb=in_b[i];
      }
      mul_t result_mult=opa*opb;
      versat_t out;
      if(fns==MUL_HI) //big brain time: to avoid left/right shifts, using a MASK of size mul_t and versat_t
      {
        result_mult=result_mult<<1; 
        out=(versat_t)(result_mult>>(sizeof(versat_t)*8));
      }
      else if(fns==MUL_DIV2_HI)
      {
      out=(versat_t)(result_mult>>(sizeof(versat_t)*8));
      }
      else // MUL_LO
      { 
        out=(versat_t)result_mult;
      }
    }
    
    void setConf(int sela, int selb, int fns) {
      this->sela=sela;
      this->selb=selb;
      this->fns=fns;
    }

    void writeConf() {
      conf[versat_base].mul[mul_base].sela=sela;
      conf[versat_base].mul[mul_base].selb=selb;
      conf[versat_base].mul[mul_base].fns=fns;
    }
    void setSelA(int sela) {
      conf[versat_base].mul[mul_base].sela=sela;
      this->sela = sela; 
    }
    void setSelB(int selb) {
      conf[versat_base].mul[mul_base].selb=selb;
      this->selb = selb; 
    }
    void setFNS(int fns) {
      conf[versat_base].mul[mul_base].fns=fns;
      this->fns = fns; 
    }
};//end class CMUL
#endif

#if nMULADD>0
class CMulAdd {
  private:   
    //SIM VARIABLES
    versat_t in_a[nFU-1], in_b[nFU-1]; // number of inputs for MUX
    versat_t opa,opb,out,acc_w;
    mul_t acc;
  public:
    int versat_base, muladd_base;
    int sela, selb, fns, iter, per, delay, shift;
    
    //Default constructor
    CMulAdd() {
    }

    CMulAdd(int versat_base, int i) {
      this->versat_base = versat_base;
      this->muladd_base = i;
    }

    versat_t output() //lacks shift,iter and delay implementation aka state machine
    {
      for(int i=0;i<nFU-1;i++)
      {
        if (sela==i)
          opa=in_a[i];
        if (selb==i)
          opb=in_b[i];
      }
      mul_t result_mult=opa*opb;
      if(fns==MULADD_MACC)
      {
        acc=acc_w+result_mult;
      }
      else
      {
        acc=acc_w-result_mult;
      }
      acc_w=(versat_t)(acc>>(sizeof(versat_t)*4));

      return acc_w;
    }

    void setConf(int sela, int selb, int fns, int iter, int per, int delay, int shift) {
      this->sela = sela;
      this->selb = selb;
      this->fns = fns;
      this->iter = iter;
      this->per = per;
      this->delay = delay;
      this->shift = shift;
    }

    void writeConf() {
      conf[versat_base].muladd[muladd_base].sela=sela;
      conf[versat_base].muladd[muladd_base].selb=selb;
      conf[versat_base].muladd[muladd_base].fns=fns;
      conf[versat_base].muladd[muladd_base].iter=iter;
      conf[versat_base].muladd[muladd_base].per=per;
      conf[versat_base].muladd[muladd_base].delay=delay;
      conf[versat_base].muladd[muladd_base].shift=shift;
    }
    void setSelA(int sela) {
      conf[versat_base].muladd[muladd_base].sela=sela;
      this->sela = sela; 
    }
    void setSelB(int selb) {
      conf[versat_base].muladd[muladd_base].selb=selb;
      this->selb = selb; 
    }
    void setFNS(int fns) {
      conf[versat_base].muladd[muladd_base].fns=fns;
      this->fns = fns; 
    }
    void setIter(int iter) {
      conf[versat_base].muladd[muladd_base].iter=iter;
      this->iter = iter; 
    }
    void setPer(int per) {
      conf[versat_base].muladd[muladd_base].per=per;
      this->per = per; 
    }
    void setDelay(int delay) {
      conf[versat_base].muladd[muladd_base].delay=delay;
      this->delay = delay;
    }
    void setShift(int shift) {
      conf[versat_base].muladd[muladd_base].shift=shift;
      this->shift = shift;
    }
};//end class CMULADD
#endif

class CStage {

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
    CStage() {
    }
    
    //Default Constructor
    CStage(int versat_base) {

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
      int i=versat_base;
      memset(&conf[versat_base],0,sizeof(conf[versat_base]));
      conf[i]=CStage(i);
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
};//end class CStage

//
//VERSAT global variables
//
#ifndef VERSAT_cpp // include guard
#define VERSAT_cpp
static int base;
CStage stage[nSTAGE];
CStage conf[nSTAGE];
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
  base_addr=0;
  base = base_addr;
  for(i = 0; i < nSTAGE; i++) stage[i] = CStage(base_addr +i);

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

int run_done=0;

void run_sim()
{
run_done=1;
//put simulation here
}

inline void run() {
  //MEMSET(base, (RUN_DONE), 1);
  run_done=0;
  std::thread ti(run_sim);
}

inline int done() {
  return run_done;
}

inline void globalClearConf() {
 memset(&conf,0,sizeof(conf));
 for(int i=0;i<nSTAGE;i++)
 {
   conf[i]=CStage(i);
 }
}
#endif

#endif

