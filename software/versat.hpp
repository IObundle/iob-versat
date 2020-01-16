#include "versat_defs.h"

#define RAM_SET32(base, location, value) *((volatile int*) (base + (sizeof(int)) * location)) = value
#define RAM_GET32(base, location)        *((volatile int*) (base + (sizeof(int)) * location))

#define VERSAT_TOP_BASE 0x11000000

//BUG: When using defines from versat_defs.h on RAM_SET32 use parentheses!!!
//+2 is because RV ignores the 2 lsb since it isn't byte addressable, but 32bit addressable 
#define VERSAT_1 (VERSAT_TOP_BASE | (0<<(ADDR_W+2)))
#define VERSAT_2 (VERSAT_TOP_BASE | (1<<(ADDR_W+2)))
#define VERSAT_3 (VERSAT_TOP_BASE | (2<<(ADDR_W+2)))
#define VERSAT_4 (VERSAT_TOP_BASE | (3<<(ADDR_W+2)))
#define VERSAT_5 (VERSAT_TOP_BASE | (4<<(ADDR_W+2)))

//We don't define DMA_BASE since we are assuming that it will be at 0x12000000

#define VERSAT_DUMMY 4096 //0x10
#define VERSAT_CONF 8192 //0x2000

//VERSAT global variables                                                                             
int i, ENG_MEM[nMEM], CONF_MEMA[nMEM], CONF_MEMB[nMEM], CONF_ALU[nALU], CONF_ALULITE[nALULITE], CONF_MUL[nMUL], CONF_MULADD[nMULADD], CONF_BS[nBS], sMEMA[nMEM], sMEMA_p[nMEM], sMEMB[nMEM], sMEMB_p[nMEM], sALU[nALU], sALU_p[nALU], sALULITE[nALULITE], sALULITE_p[nALULITE], sMUL[nMUL], sMUL_p[nMUL], sMULADD[nMULADD], sMULADD_p[nMULADD], sBS[nBS], sBS_p[nBS];

//VERSAT FUNCTIONS
inline void defs() {                                                         
  //Memories                                                          
  for(i=0; i<nMEM; i=i+1){                  
    ENG_MEM[i] = ENG_BASE+i*(1<<DADDR_W);                             
    CONF_MEMA[i] = 2*i*MEMP_CONF_OFFSET+CONF_BASE+CONF_MEM0A;         
    CONF_MEMB[i] = (2*i+1)*MEMP_CONF_OFFSET+CONF_BASE+CONF_MEM0A;     
    sMEMA[i] = sMEM0A+2*i;                                            
    sMEMA_p[i] = sMEMA[i] | (1<<(N_W-1)); //This is to select previous MEMories                                            
    sMEMB[i] = sMEM0A+2*i+1;                                          
  }                                                                    
                                                                      
  //ALUs                                                              
  for (i=0; i<nALU; i=i+1){                                            
    CONF_ALU[i] = i*ALU_CONF_OFFSET + CONF_BASE+CONF_ALU0;            
    sALU[i] = sALU0+i;
    sALU_p[i] = sALU[i] | (1<<(N_W-1)); //This is to select previous ALU                                               
  }                                          
                                                                      
  //ALULITEs                                                          
  for (i=0; i<nALULITE; i=i+1){                                        
    CONF_ALULITE[i] = i*ALULITE_CONF_OFFSET + CONF_BASE+CONF_ALULITE0;
    sALULITE[i] = sALULITE0+i;                                        
    sALULITE_p[i] = sALULITE[i] | (1<<(N_W-1));                                        
  }                                                                    
                                                                      
  //MULTIPLIERS                                                       
  for (i=0; i<nMUL; i=i+1){                                            
    CONF_MUL[i] = i*MUL_CONF_OFFSET + CONF_BASE+CONF_MUL0;            
    sMUL[i] = sMUL0+i;                                                
    sMUL_p[i] = sMUL[i] | (1<<(N_W-1)); 
  }                                                                    
                                                                      
  //MULADDS                                                           
  for (i=0; i<nMULADD; i=i+1){                                        
    CONF_MULADD[i] = i*MULADD_CONF_OFFSET + CONF_BASE+CONF_MULADD0;   
    sMULADD[i] = sMULADD0+i;                                          
    sMULADD_p[i] = sMULADD[i] | (1<<(N_W-1));
  }                                        
                                                                      
  //BARREL SHIFTERS                                                   
  for (i=0; i<nBS; i=i+1){                                             
    CONF_BS[i] = i*BS_CONF_OFFSET + CONF_BASE+CONF_BS0;               
    sBS[i] = sBS0+i;                                                  
    sBS_p[i] = sBS[i] | (1<<(N_W-1));                                                  
  }                                            
}                                                                     

class CMemPort
{
    public:
    //Versat Address Base
    int versat_base;
    //Memory Address Base
    int mem_base;
    //Conf Regs
    int iter;
    int per=1;
    int duty=per;
    int sel;
    int start;
    int shift=0;
    int incr;
    int delay;
    int ext=0;
    
    //Default Constructor
    CMemPort () {
    }

    //Constructor with an associated base
    CMemPort (int versat_base) {
        this->versat_base = versat_base;
    }

    //Full Configuration (Needed to configure nested loops)
    void setConf (int mem_base, int start, int iter, int incr, int delay, int per,
          int duty, int sel, int shift, int ext)
    {
        this->mem_base = mem_base;
        this->iter = iter;
        this->per = per;
        this->duty = duty;
        this->sel = sel;
        this->start = start;
        this->shift = shift;
        this->incr = incr;
        this->delay = delay;
        this->ext = ext;
    }

    //Minimum Configuration (Can be used to configure just a single Loop)
    void setConf (int mem_base, int start, int iter, int incr, int delay, int per,
          int duty, int sel)
    {
        this->mem_base = mem_base;
        this->iter = iter;
        this->per = per;
        this->duty = duty;
        this->sel = sel;
        this->start = start;
        this->incr = incr;
        this->delay = delay;
    }

    void writeConf() {
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_ITER), iter);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_START), start);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_PER), per);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_DUTY), duty);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_SEL), sel);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_SHIFT), shift);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_INCR), incr);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_DELAY), delay);
        RAM_SET32(versat_base, (mem_base  + MEMP_CONF_EXT), ext);
    }
    void setIter(int iter) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_ITER), iter);
        this->iter = iter;
    } 
    void setPer(int per) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_PER), per);
        this->per = per;
    } 
    void setDuty(int duty) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_DUTY), duty);
        this->duty = duty;
    } 
    void setSel(int sel) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_SEL), sel);
        this->sel = sel;
    } 
    void setStart(int start) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_START), start);
        this->start = start;
    } 
    void setIncr(int incr) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_START), incr);
        this->incr = incr;
    } 
    void setShift(int shift) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_SHIFT), shift);
        this->shift = shift;
    } 
    void setDelay(int delay) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_DELAY), delay);
        this->delay = delay;
    } 
    void setExt(int ext) {
        RAM_SET32(versat_base, (this->mem_base  + MEMP_CONF_EXT), ext);
        this->ext = ext;
    } 
};//end class CMEM

class CALU {
    public:

    int versat_base;
    int alu_base;
    int opa;
    int opb;
    int fns;
    
    CALU (){
    }

    CALU (int versat_base){
        this->versat_base = versat_base;
    }
    
    void setConf (int alu_base, int opa, int opb, int fns) {
        this->alu_base = alu_base;
        this->opa = opa;
        this->opb = opb;
        this->fns = fns;
    }

    void writeConf() {
        RAM_SET32(versat_base, (alu_base  + ALU_CONF_SELA), opa);
        RAM_SET32(versat_base, (alu_base  + ALU_CONF_SELB), opb);
        RAM_SET32(versat_base, (alu_base  + ALU_CONF_FNS), fns);
    }
    void setOpA(int opa) {
        RAM_SET32(versat_base, (this->alu_base  + ALU_CONF_SELA), opa);
        this->opa = opa; 
    }
    void setOpB(int opb) {
        RAM_SET32(versat_base, (this->alu_base  + ALU_CONF_SELB), opb);
        this->opb = opb; 
    }
    void setFNS(int fns) {
        RAM_SET32(versat_base, (this->alu_base  + ALU_CONF_FNS), fns);
        this->fns = fns; 
    }
}; //end class CALI

//Probably we can delete it, since the ALUs are configured in the same way
//we just need to send the right base independent of being ALU or ALULite
class CALULite {
    public:
    int versat_base;
    int alulite_base;
    int opa;
    int opb;
    int fns;
    
    //Default Constructor
    CALULite (){
    }

    CALULite (int versat_base){
        this->versat_base = versat_base;
    }
    
    void setConf (int alulite_base, int opa, int opb, int fns) {
        this->alulite_base = alulite_base;
        this->opa = opa;
        this->opb = opb;
        this->fns = fns;
    }

    void writeConf() {
        RAM_SET32(versat_base, (alulite_base  + ALULITE_CONF_SELA), opa);
        RAM_SET32(versat_base, (alulite_base  + ALULITE_CONF_SELB), opb);
        RAM_SET32(versat_base, (alulite_base  + ALULITE_CONF_FNS), fns);
    }
    void setOpA(int opa) {
        RAM_SET32(versat_base, (this->alulite_base  + ALULITE_CONF_SELA), opa);
        this->opa = opa; 
    }
    void setOpB(int opb) {
        RAM_SET32(versat_base, (this->alulite_base  + ALULITE_CONF_SELB), opb);
        this->opb = opb; 
    }
    void setFNS(int fns) {
        RAM_SET32(versat_base, (this->alulite_base  + ALULITE_CONF_FNS), fns);
        this->fns = fns; 
    }
}; //end class CALUALITE

class CBS {
    public:
    
    int versat_base;
    int bs_base;
    int data;
    int shift;
    int fns;
    
    CBS (){
    }

    CBS (int versat_base){
        this->versat_base = versat_base;
    }
    void setConf (int bs_base, int data, int shift, int fns) {
        this->bs_base = bs_base;
        this->data = data;
        this->shift = shift;
        this->fns = fns;
    }

    void writeConf() {
        RAM_SET32(versat_base, (bs_base  + BS_CONF_SELD), data);
        RAM_SET32(versat_base, (bs_base  + BS_CONF_SELS), shift);
        RAM_SET32(versat_base, (bs_base  + BS_CONF_FNS), fns);
    }
    void setData(int data) {
        RAM_SET32(versat_base, (this->bs_base  + BS_CONF_SELD), data);
        this->data = data; 
    }
    void setShift(int shift) {
        RAM_SET32(versat_base, (this->bs_base  + BS_CONF_SELS), shift);
        this->shift = shift; 
    }
    void setFNS(int fns) {
        RAM_SET32(versat_base, (this->bs_base  + BS_CONF_FNS), fns);
        this->fns = fns; 
    }
};//end class CBS

class CMul
{
    public:
    int versat_base;
    //Address Base
    int mul_base;
    //Conf Regs
    int sela;
    int selb;
    int fns;

    //Default Constructor
    CMul () {
    }

    CMul (int versat_base) {
        this->versat_base = versat_base;
    }
    
    void setConf (int mul_base, int sela, int selb, int fns)
    {
        this->mul_base = mul_base;
        this->sela=sela;
        this->selb=selb;
        this->fns=fns;
    }

    void writeConf() {
        RAM_SET32(versat_base, (mul_base  + MUL_CONF_SELA), sela);
        RAM_SET32(versat_base, (mul_base  + MUL_CONF_SELB), selb);
        RAM_SET32(versat_base, (mul_base  + MUL_CONF_FNS), fns);
    }
    void setSelA(int sela) {
        RAM_SET32(versat_base, (this->mul_base  + MUL_CONF_SELA), sela);
        this->sela = sela; 
    }
    void setSelB(int selb) {
        RAM_SET32(versat_base, (this->mul_base  + MUL_CONF_SELB), selb);
        this->selb = selb; 
    }
    void setFNS(int fns) {
        RAM_SET32(versat_base, (this->mul_base  + MUL_CONF_FNS), fns);
        this->fns = fns; 
    }
};//end class CMUL

class CMulAdd
{
    public:
    int versat_base;
    //Address Base
    int muladd_base;
    //Conf Regs
    int sela;
    int selb;
    int selo;
    int fns;

    //Default Constructor
    CMulAdd () {
    }
    
    CMulAdd(int versat_base) {
        this->versat_base = versat_base;
    }

    void setConf(int muladd_base, int sela, int selo, int selb, int fns)
    {
        this->muladd_base = muladd_base;
        this->sela=sela;
        this->selb=selb;
        this->selo=selo;
        this->fns=fns;
    }

    void writeConf() {
        RAM_SET32(versat_base, (muladd_base  + MULADD_CONF_SELA), sela);
        RAM_SET32(versat_base, (muladd_base  + MULADD_CONF_SELB), selb);
        RAM_SET32(versat_base, (muladd_base  + MULADD_CONF_SELO), selo);
        RAM_SET32(versat_base, (muladd_base  + MULADD_CONF_FNS), fns);
    }
    void setSelA(int sela) {
        RAM_SET32(versat_base, (this->muladd_base  + MULADD_CONF_SELA), sela);
        this->sela = sela; 
    }
    void setSelB(int selb) {
        RAM_SET32(versat_base, (this->muladd_base  + MULADD_CONF_SELB), selb);
        this->selb = selb; 
    }
    void setSelO(int selo) {
        RAM_SET32(versat_base, (this->muladd_base  + MULADD_CONF_SELO), selo);
        this->selb = selb; 
    }
    void setFNS(int fns) {
        RAM_SET32(versat_base, (this->muladd_base  + MULADD_CONF_FNS), fns);
        this->fns = fns; 
    }
};//end class CMULADD

class CVersat
{
    public:
    int versat_base;
    int dma_base;
    //Versat Function Units
    CMemPort memPort[2*nMEM];
    CALU alu[nALU];
    CALULite alulite[nALULITE];
    CBS bs[nBS];
    CMul mul[nMUL];
    CMulAdd muladd[nMULADD];

    //Default Constructor
    CVersat (int versat_base) {

	//Define control and databus base address
        this->versat_base = versat_base;
        this->dma_base = versat_base & ~(1<<24) | (1<<25);

	//Init functional units
	for (i=0; i<2*nMEM; i++)
          memPort[i] = CMemPort (versat_base);
        for (i=0; i<nALU; i++)
          alu[i] = CALU (versat_base);
        for (i=0; i<nALULITE; i++)
          alulite[i] = CALULite (versat_base);
        for (i=0; i<nBS; i++)
          bs[i] = CBS (versat_base);
        for (i=0; i<nMUL; i++)
          mul[i] = CMul (versat_base);
        for (i=0; i<nMULADD; i++)
          muladd[i] = CMulAdd (versat_base);
    }
    
    //Return 0 (false) if it doesn't init with success, 0 otherwise
    int Init() {
        //write dummy register                     
        RAM_SET32(versat_base, VERSAT_DUMMY, 0xDEADBEEF);
        // read and check result                                   
        if (RAM_GET32(versat_base, VERSAT_DUMMY) != 0xDEADBEEF)
          return 0;
        else
          return 1;
    }
    
    void clearConf() {
        //clear Versat config                       
        RAM_SET32(versat_base, (CONF_CLEAR), 0);
    }
    
    void Run() {
        //Wait for ready                                       
        while (RAM_GET32(versat_base, (ENG_RDY_REG)) == 0);
        //Run data engine                                      
        RAM_SET32(versat_base, (ENG_RUN_REG), 1);
    }
    
    void DMAwrite(int mem, int addr, int val) {
        //write to databus
        RAM_SET32(dma_base, (mem  + addr), val);
    }
    
    int DMAread(int mem, int addr) {                                             
        // read from databus                        
        return (RAM_GET32(dma_base, (mem  + addr)));
    }                                               
};//end class CVersat
