#ifndef VERSAT_SIM
#define VERSAT_SIM
#include <iostream>
#include <thread>
#include "versat.h"
#include <string.h>
#include <bitset>

#ifndef DATAPATH_W
#define DATAPATH_W 16
#endif

#if DATAPATH_W == 16
typedef int16_t versat_t;
typedef int32_t mul_t;
typedef uint32_t shift_t;
#elif DATAPATH_W == 8
typedef int8_t versat_t;
typedef int16_t mul_t;
typedef uint16_t shift_t;

#else
typedef int32_t versat_t;
typedef int64_t mul_t;
typedef uint64_t shift_t;

#endif
using namespace std;

#define SET_BITS(var, val, size) \
  for (int i = 0; i < size; i++) \
  {                              \
    var.set(i, val);             \
  }

#define MSB(var, size) var >> (size - 1)

//Macro functions to use cpu interface
#define MEMSET(base, location, value) (*((volatile int *)(base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location) (*((volatile int *)(base + (sizeof(int)) * location)))

//constants
#define CONF_BASE (1 << (nMEM_W + MEM_ADDR_W + 1))
#define CONF_MEM_SIZE ((int)pow(2, CONF_MEM_ADDR_W))
//#define MEM_SIZE ((int)pow(2,MEM_ADDR_W))
#define MEM_SIZE (1 << MEM_ADDR_W)
#define RUN_DONE (1 << (nMEM_W + MEM_ADDR_W))

//
// VERSAT CLASSES
//

class CMem
{
private:
  versat_t mem[MEM_SIZE];
  versat_t read(uint32_t addr)
  {
    if (addr >= MEM_SIZE)
    {
      printf("Invalid MEM ADDR\n");
      return 0;
    }
    return mem[addr];
  }
  void write(int32_t addr, versat_t data_in)
  {
    if (addr >= MEM_SIZE)
    {
      printf("Invalid MEM ADDR\n");
    }
    else
    {
      mem[addr] = data_in;
    }
  }

public:
  friend class CMemPort;
};

class CMemPort // 4 Loop AGU
{
private:
  //count delay during a run():
  int run_delay = 0;
  versat_t out;
  versat_t output_port[MEMP_LAT]; //output FIFO
public:
  CMem *my_mem;
  int versat_base, mem_base, data_base;
  int iter, per, duty, sel, start, shift, incr, delay, in_wr;
  int rvrs = 0, ext = 0, iter2 = 0, per2 = 0, shift2 = 0, incr2 = 0;

  //Default constructor
  CMemPort()
  {
  }
  //Constructor with an associated base
  CMemPort(int versat_base, int i, int offset)
  {
    this->versat_base = versat_base;
    this->mem_base = i;
    this->data_base = offset;
    this->my_mem = &stage[versat_base].mem[i];
  }
  //set MEMPort configuration to shadow register
  void update_shadow_reg_MEM()
  {
    if (data_base == 1)
    {
      shadow_reg[versat_base].memB[mem_base].iter = conf[versat_base].memB[mem_base].iter;
      shadow_reg[versat_base].memB[mem_base].start = conf[versat_base].memB[mem_base].start;
      shadow_reg[versat_base].memB[mem_base].per = conf[versat_base].memB[mem_base].per;
      shadow_reg[versat_base].memB[mem_base].duty = conf[versat_base].memB[mem_base].duty;
      shadow_reg[versat_base].memB[mem_base].sel = conf[versat_base].memB[mem_base].sel;
      shadow_reg[versat_base].memB[mem_base].shift = conf[versat_base].memB[mem_base].shift;
      shadow_reg[versat_base].memB[mem_base].incr = conf[versat_base].memB[mem_base].incr;
      shadow_reg[versat_base].memB[mem_base].delay = conf[versat_base].memB[mem_base].delay;
      shadow_reg[versat_base].memB[mem_base].ext = conf[versat_base].memB[mem_base].ext;
      shadow_reg[versat_base].memB[mem_base].in_wr = conf[versat_base].memB[mem_base].in_wr;
      shadow_reg[versat_base].memB[mem_base].rvrs = conf[versat_base].memB[mem_base].rvrs;
      shadow_reg[versat_base].memB[mem_base].iter2 = conf[versat_base].memB[mem_base].iter2;
      shadow_reg[versat_base].memB[mem_base].per2 = conf[versat_base].memB[mem_base].per2;
      shadow_reg[versat_base].memB[mem_base].shift2 = conf[versat_base].memB[mem_base].shift2;
      shadow_reg[versat_base].memB[mem_base].incr2 = conf[versat_base].memB[mem_base].incr2;
    }
    else
    {
      shadow_reg[versat_base].memA[mem_base].iter = conf[versat_base].memA[mem_base].iter;
      shadow_reg[versat_base].memA[mem_base].start = conf[versat_base].memA[mem_base].start;
      shadow_reg[versat_base].memA[mem_base].per = conf[versat_base].memA[mem_base].per;
      shadow_reg[versat_base].memA[mem_base].duty = conf[versat_base].memA[mem_base].duty;
      shadow_reg[versat_base].memA[mem_base].sel = conf[versat_base].memA[mem_base].sel;
      shadow_reg[versat_base].memA[mem_base].shift = conf[versat_base].memA[mem_base].shift;
      shadow_reg[versat_base].memA[mem_base].incr = conf[versat_base].memA[mem_base].incr;
      shadow_reg[versat_base].memA[mem_base].delay = conf[versat_base].memA[mem_base].delay;
      shadow_reg[versat_base].memA[mem_base].ext = conf[versat_base].memA[mem_base].ext;
      shadow_reg[versat_base].memA[mem_base].in_wr = conf[versat_base].memA[mem_base].in_wr;
      shadow_reg[versat_base].memA[mem_base].rvrs = conf[versat_base].memA[mem_base].rvrs;
      shadow_reg[versat_base].memA[mem_base].iter2 = conf[versat_base].memA[mem_base].iter2;
      shadow_reg[versat_base].memA[mem_base].per2 = conf[versat_base].memA[mem_base].per2;
      shadow_reg[versat_base].memA[mem_base].shift2 = conf[versat_base].memA[mem_base].shift2;
      shadow_reg[versat_base].memA[mem_base].incr2 = conf[versat_base].memA[mem_base].incr2;
    }
  }

  //start run
  void start_run()
  {
    //set run_delay
    if (data_base == 1)
    {
      run_delay = shadow_reg[versat_base].memB[mem_base].delay;
    }
    else
    {
      run_delay = shadow_reg[versat_base].memA[mem_base].delay;
    }
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
      run_delay--;
    }
    else
    {

      //update databus
      if (data_base == 0)
      {
        stage[versat_base].databus[sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
        //special case for stage 0
        if (versat_base == 0)
        {
          //2nd copy at the end of global databus
          global_databus[nSTAGE * N + sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
        }
      }
      else
      {
        stage[versat_base].databus[sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
        //special case for stage 0
        if (versat_base == 0)
        {
          //2nd copy at the end of global databus
          global_databus[nSTAGE * N + sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
        }
      }

      //trickle down all outputs in buffer
      for (i = 1; i < MEMP_LAT; i++)
      {
        output_port[i] = output_port[i - 1];
      }
      //insert new output
      output_port[0] = out; //TO DO: change according to output()
    }
  }

  versat_t output()
  {
    return 0;
  }
  //Full Configuration (includes ext and rvrs)
  void setConf(int start, int iter, int incr, int delay, int per, int duty, int sel, int shift, int in_wr, int rvrs, int ext)
  {
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
  void setConf(int start, int iter, int incr, int delay, int per, int duty, int sel, int shift, int in_wr)
  {
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

  void setConf(int iter2, int per2, int shift2, int incr2)
  {
    this->iter2 = iter2;
    this->per2 = per2;
    this->shift2 = shift2;
    this->incr2 = incr2;
  }

  void writeConf()
  {
    if (data_base == 1)
    {
      conf[versat_base].memB[mem_base].iter = iter;
      conf[versat_base].memB[mem_base].start = start;
      conf[versat_base].memB[mem_base].per = per;
      conf[versat_base].memB[mem_base].duty = duty;
      conf[versat_base].memB[mem_base].sel = sel;
      conf[versat_base].memB[mem_base].shift = shift;
      conf[versat_base].memB[mem_base].incr = incr;
      conf[versat_base].memB[mem_base].delay = delay;
      conf[versat_base].memB[mem_base].ext = ext;
      conf[versat_base].memB[mem_base].in_wr = in_wr;
      conf[versat_base].memB[mem_base].rvrs = rvrs;
      conf[versat_base].memB[mem_base].iter2 = iter2;
      conf[versat_base].memB[mem_base].per2 = per2;
      conf[versat_base].memB[mem_base].shift2 = shift2;
      conf[versat_base].memB[mem_base].incr2 = incr2;
    }
    else
    {
      conf[versat_base].memA[mem_base].iter = iter;
      conf[versat_base].memA[mem_base].start = start;
      conf[versat_base].memA[mem_base].per = per;
      conf[versat_base].memA[mem_base].duty = duty;
      conf[versat_base].memA[mem_base].sel = sel;
      conf[versat_base].memA[mem_base].shift = shift;
      conf[versat_base].memA[mem_base].incr = incr;
      conf[versat_base].memA[mem_base].delay = delay;
      conf[versat_base].memA[mem_base].ext = ext;
      conf[versat_base].memA[mem_base].in_wr = in_wr;
      conf[versat_base].memA[mem_base].rvrs = rvrs;
      conf[versat_base].memA[mem_base].iter2 = iter2;
      conf[versat_base].memA[mem_base].per2 = per2;
      conf[versat_base].memA[mem_base].shift2 = shift2;
      conf[versat_base].memA[mem_base].incr2 = incr2;
    }
  }
  void setIter(int iter)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].iter = iter;
    else
    {
      conf[versat_base].memA[mem_base].iter = iter;
    }
    this->iter = iter;
  }
  void setPer(int per)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].per = per;
    else
    {
      conf[versat_base].memA[mem_base].per = per;
    }
    this->per = per;
  }
  void setDuty(int duty)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].duty = duty;
    else
    {
      conf[versat_base].memA[mem_base].duty = duty;
    }
    this->duty = duty;
  }
  void setSel(int sel)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].sel = sel;
    else
    {
      conf[versat_base].memA[mem_base].sel = sel;
    }
    this->sel = sel;
  }
  void setStart(int start)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].start = start;
    else
    {
      conf[versat_base].memA[mem_base].start = start;
    }
    this->start = start;
  }
  void setIncr(int incr)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].incr = incr;
    else
    {
      conf[versat_base].memA[mem_base].incr = incr;
    }
    this->incr = incr;
  }
  void setShift(int shift)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].shift = shift;
    else
    {
      conf[versat_base].memA[mem_base].shift = shift;
    }
    this->shift = shift;
  }
  void setDelay(int delay)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].delay = delay;
    else
    {
      conf[versat_base].memA[mem_base].delay = delay;
    }
    this->delay = delay;
  }
  void setExt(int ext)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].ext = ext;
    else
    {
      conf[versat_base].memA[mem_base].ext = ext;
    }
    this->ext = ext;
  }
  void setRvrs(int rvrs)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].rvrs = rvrs;
    else
    {
      conf[versat_base].memA[mem_base].rvrs = rvrs;
    }
    this->rvrs = rvrs;
  }
  void setInWr(int in_wr)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].in_wr = in_wr;
    else
    {
      conf[versat_base].memA[mem_base].in_wr = in_wr;
    }
    this->in_wr = in_wr;
  }
  void setIter2(int iter2)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].iter2 = iter2;
    else
    {
      conf[versat_base].memA[mem_base].iter2 = iter2;
    }
    this->iter2 = iter2;
  }
  void setPer2(int per2)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].per2 = per2;
    else
    {
      conf[versat_base].memA[mem_base].per2 = per2;
    }
    this->per2 = per2;
  }
  void setIncr2(int incr)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].incr2 = incr2;
    else
    {
      conf[versat_base].memA[mem_base].incr2 = incr2;
    }
    this->incr2 = incr2;
  }
  void setShift2(int shift2)
  {
    if (data_base == 1)
      conf[versat_base].memB[mem_base].shift2 = shift2;
    else
    {
      conf[versat_base].memA[mem_base].shift2 = shift2;
    }
    this->shift2 = shift2;
  }

  void write(int addr, int val)
  {
    //MEMSET(versat_base, (this->data_base + addr), val);
    my_mem->write(addr, val);
  }
  int read(int addr)
  {
    //return MEMGET(versat_base, (this->data_base + addr));
    return my_mem->read(addr);
  }
}; //end class CMEM

#if nALU > 0
class CALU
{
private:
  versat_t ina, inb, out;
  versat_t output[ALU_LAT]; //output FIFO
public:
  int versat_base, alu_base;
  int opa, opb, fns;

  //Default constructor
  CALU()
  {
  }

  //set ALU configuration to shadow register
  void update_shadow_reg_ALU()
  {
    shadow_reg[versat_base].alu[alu_base].opa = conf[versat_base].alu[alu_base].opa;
    shadow_reg[versat_base].alu[alu_base].opb = conf[versat_base].alu[alu_base].opb;
    shadow_reg[versat_base].alu[alu_base].fns = conf[versat_base].alu[alu_base].fns;
  }

  //start run
  void start_run()
  {
    //CAlu has no delay
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;

    //update databus
    stage[versat_base].databus[sALU[alu_base]] = output[ALU_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
      //2nd copy at the end of global databus
      global_databus[nSTAGE * N + sALU[alu_base]] = output[ALU_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < ALU_LAT; i++)
    {
      output[i] = output[i - 1];
    }
    //insert new output
    output[0] = out;
  }

  versat_t output()
  {
    inb = stage[versat_base].databus[shadow_reg[versat_base].alu[alu_base].opa];
    ina = stage[versat_base].databus[shadow_reg[versat_base].alu[alu_base].opb];
    switch (fns)
    {
    case ALU_OR:
      /* code */
      break;
    case ALU_AND:
      /* code */
      break;
    case ALU_XOR:
      /* code */
      break;
    case ALU_SEXT8:
      /* code */
      break;
    case ALU_SEXT16:
      /* code */
      break;
    case ALU_SHIFTR_ARTH:
      /* code */
      break;
    case ALU_SHIFTR_LOG:
      /* code */
      break;
    case ALU_CMP_SIG:
      /* code */
      break;
    case ALU_CMP_UNS:
      /* code */
      break;
    case ALU_MUX:
      /* code */
      break;
    case ALU_ADD:
      /* code */
      break;
    case ALU_SUB:
      /* code */
      break;
    case ALU_MAX:
      /* code */
      break;
    case ALU_MIN:
      /* code */
      break;
    default:
      break;
    }
    return out;
  }
  CALU(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->alu_base = i;
  }

  void setConf(int opa, int opb, int fns)
  {
    this->opa = opa;
    this->opb = opb;
    this->fns = fns;
  }

  void writeConf()
  {
    conf[versat_base].alu[alu_base].opa = opa;
    conf[versat_base].alu[alu_base].opb = opb;
    conf[versat_base].alu[alu_base].fns = fns;
  }
  void setOpA(int opa)
  {
    conf[versat_base].alu[alu_base].opa = opa;
    this->opa = opa;
  }
  void setOpB(int opb)
  {
    conf[versat_base].alu[alu_base].opb = opb;
    this->opb = opb;
  }
  void setFNS(int fns)
  {
    conf[versat_base].alu[alu_base].fns = fns;
    this->fns = fns;
  }
}; //end class CALU
#endif

#if nALULITE > 0
class CALULite
{
private:
  versat_t ina, inb, out;
  versat_t output[ALULITE_LAT]; //output FIFO
public:
  int versat_base, alulite_base;
  int opa, opb, fns;

  //Default constructor
  CALULite()
  {
  }

  //set ALULite configuration to shadow register
  void update_shadow_reg_ALULite()
  {
    shadow_reg[versat_base].alulite[alulite_base].opa = conf[versat_base].alulite[alulite_base].opa;
    shadow_reg[versat_base].alulite[alulite_base].opb = conf[versat_base].alulite[alulite_base].opb;
    shadow_reg[versat_base].alulite[alulite_base].fns = conf[versat_base].alulite[alulite_base].fns;
  }

  //start run
  void start_run()
  {
    //CALULite has no delay
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;

    //update databus
    stage[versat_base].databus[sALULITE[alulite_base]] = output[ALULITE_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
      //2nd copy at the end of global databus
      global_databus[nSTAGE * N + sALULITE[alulite_base]] = output[ALULITE_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < ALULITE_LAT; i++)
    {
      output[i] = output[i - 1];
    }
    //insert new output
    output[0] = out;
  }

  versat_t output()
  {
    bitset<DATAPATH_W + 1> ai;
    bitset<DATAPATH_W + 1> bz;
    SET_BITS(ai, 0, DATAPATH_W + 1);
    SET_BITS(bz, 1, DATAPATH_W + 1);

    bitset<1> op_a_msb(0x0);
    bitset<1> cin(0x0);
    bitset<1> op_b_msb(0x0);
    //cast to int cin.to_ulong();
    bitset<DATAPATH_W> op_a_reg(stage[versat_base].databus[shadow_reg[versat_base].alulite[alulite_base].opa]);
    versat_t self_loop = shadow_reg[versat_base].alulite[alulite_base].fns < 0 ? 1 : 0;
    bitset<DATAPATH_W> result;
    bitset<DATAPATH_W> op_b_reg(stage[versat_base].databus[shadow_reg[versat_base].alulite[alulite_base].opb]);
    bitset<DATAPATH_W> op_a_int(self_loop ? out : op_a_reg);

    switch (shadow_reg[versat_base].alulite[alulite_base].fns)
    {
    case ALULITE_OR:
      result = op_a_int | op_b_reg;
      break;
    case ALULITE_AND:
      result = op_a_int & op_b_reg;
      break;
    case ALULITE_CMP_SIG: //it's a bit more complex TODO
      SET_BITS(ai, 1, DATAPATH_W + 1);
      cin.set(0, 1);
      int aux = (uint)op_a_int.to_ulong();
      MSB(aux, DATAPATH_W);
      int aux2 = aux ? 1 : 0;
      op_a_msb.set(0, aux2);
      aux = (uint)op_b_reg.to_ulong();
      MSB(aux, DATAPATH_W);
      aux2 = aux ? 1 : 0;
      op_b_msb.set(0, aux2);
      break;
    case ALULITE_MUX:
      result = inb;
      if (~(int)op_a_reg.to_ulong())
      {
        if (self_loop)
          result = out;
        else
        {
          result = 0;
        }
      }
      break;
    case ALULITE_SUB:
      break;
    case ALULITE_ADD:
      if (self_loop)
        if (op_a_reg.to_ulong())
          result = inb;
      break;
    case ALULITE_MAX: //TODO
      break;
    case ALULITE_MIN: //TODO
      break;
    default:
      break;
    }

    return out;
  }

  CALULite(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->alulite_base = i;
  }

  void setConf(int opa, int opb, int fns)
  {
    this->opa = opa;
    this->opb = opb;
    this->fns = fns;
  }

  void writeConf()
  {
    conf[versat_base].alulite[alulite_base].opa = opa;
    conf[versat_base].alulite[alulite_base].opb = opb;
    conf[versat_base].alulite[alulite_base].fns = fns;
  }
  void setOpA(int opa)
  {
    conf[versat_base].alulite[alulite_base].opa = opa;
    this->opa = opa;
  }
  void setOpB(int opb)
  {
    conf[versat_base].alulite[alulite_base].opb = opb;
    this->opb = opb;
  }
  void setFNS(int fns)
  {
    conf[versat_base].alulite[alulite_base].fns = fns;
    this->fns = fns;
  }
}; //end class CALUALITE
#endif

#if nBS > 0
class CBS
{
private:
  versat_t in, out;
  versat_t output[BS_LAT]; //output FIFO
public:
  int versat_base, bs_base;
  int data, shift, fns;

  //Default constructor
  CBS()
  {
  }

  //set BS configuration to shadow register
  void update_shadow_reg_BS()
  {
    shadow_reg[versat_base].bs[bs_base].data = conf[versat_base].bs[bs_base].data;
    shadow_reg[versat_base].bs[bs_base].fns = conf[versat_base].bs[bs_base].fns;
    shadow_reg[versat_base].bs[bs_base].shift = conf[versat_base].bs[bs_base].shift;
  }

  //start run
  void start_run()
  {
    //CBS has no delay
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;

    //update databus
    stage[versat_base].databus[sBS[bs_base]] = output[BS_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
      //2nd copy at the end of global databus
      global_databus[nSTAGE * N + sBS[bs_base]] = output[BS_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < BS_LAT; i++)
    {
      output[i] = output[i - 1];
    }
    //insert new output
    output[0] = out;
  }

  versat_t output()
  {
    in = stage[versat_base].databus[shadow_reg[versat_base].bs[bs_base].data];
    if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHR_A)
    {
      in = in >> shadow_reg[versat_base].bs[bs_base].shift;
    }
    else if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHR_L)
    {
      shift_t s = in;
      s = s >> shadow_reg[versat_base].bs[bs_base].shift;
      in = s;
    }
    else if (shadow_reg[versat_base].bs[bs_base].fns == BS_SHL)
    {
      in = in << shadow_reg[versat_base].bs[bs_base].shift;
    }

    out = in;

    return out;
  }

  CBS(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->bs_base = i;
  }

  void setConf(int data, int shift, int fns)
  {
    this->data = data;
    this->shift = shift;
    this->fns = fns;
  }

  void writeConf()
  {
    conf[versat_base].bs[bs_base].data = data;
    conf[versat_base].bs[bs_base].shift = shift;
    conf[versat_base].bs[bs_base].fns = fns;
  }
  void setData(int data)
  {
    conf[versat_base].bs[bs_base].data = data;
    this->data = data;
  }
  void setShift(int shift)
  {
    conf[versat_base].bs[bs_base].shift = shift;
    this->shift = shift;
  }
  void setFNS(int fns)
  {
    conf[versat_base].bs[bs_base].fns = fns;
    this->fns = fns;
  }
}; //end class CBS
#endif

#if nMUL > 0
class CMul
{
private:
  versat_t opa, opb;
  versat_t output[MUL_LAT]; //output FIFO
  versat_t out;

public:
  int versat_base, mul_base;
  int sela, selb, fns;
  //Default constructor
  CMul()
  {
  }

  CMul(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->mul_base = i;
  }

  //set Mul configuration to shadow register
  void update_shadow_reg_Mul()
  {
    shadow_reg[versat_base].mul[mul_base].sela = conf[versat_base].mul[mul_base].sela;
    shadow_reg[versat_base].mul[mul_base].selb = conf[versat_base].mul[mul_base].selb;
    shadow_reg[versat_base].mul[mul_base].fns = conf[versat_base].mul[mul_base].fns;
  }

  //start run
  void start_run()
  {
    //CMul has no delay
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;

    //update databus
    stage[versat_base].databus[sMUL[mul_base]] = output[MUL_LAT - 1];
    //special case for stage 0
    if (versat_base == 0)
    {
      //2nd copy at the end of global databus
      global_databus[nSTAGE * N + sMUL[mul_base]] = output[MUL_LAT - 1];
    }

    //trickle down all outputs in buffer
    for (i = 1; i < MUL_LAT; i++)
    {
      output[i] = output[i - 1];
    }
    //insert new output
    output[0] = out;
  }

  versat_t output()
  {
    //select inputs
    opa = stage[versat_base].databus[shadow_reg[versat_base].mul[mul_base].sela];
    opb = stage[versat_base].databus[shadow_reg[versat_base].mul[mul_base].selb];

    mul_t result_mult = opa * opb;
    if (shadow_reg[versat_base].mul[mul_base].fns == MUL_HI) //big brain time: to avoid left/right shifts, using a MASK of size mul_t and versat_t
    {
      result_mult = result_mult << 1;
      out = (versat_t)(result_mult >> (sizeof(versat_t) * 8));
    }
    else if (shadow_reg[versat_base].mul[mul_base].fns == MUL_DIV2_HI)
    {
      out = (versat_t)(result_mult >> (sizeof(versat_t) * 8));
    }
    else // MUL_LO
    {
      out = (versat_t)result_mult;
    }

    return out;
  }

  void setConf(int sela, int selb, int fns)
  {
    this->sela = sela;
    this->selb = selb;
    this->fns = fns;
  }

  void writeConf()
  {
    conf[versat_base].mul[mul_base].sela = sela;
    conf[versat_base].mul[mul_base].selb = selb;
    conf[versat_base].mul[mul_base].fns = fns;
  }
  void setSelA(int sela)
  {
    conf[versat_base].mul[mul_base].sela = sela;
    this->sela = sela;
  }
  void setSelB(int selb)
  {
    conf[versat_base].mul[mul_base].selb = selb;
    this->selb = selb;
  }
  void setFNS(int fns)
  {
    conf[versat_base].mul[mul_base].fns = fns;
    this->fns = fns;
  }
}; //end class CMUL
#endif

#if nMULADD > 0
class CMulAdd
{
private:
  //SIM VARIABLES
  versat_t opa, opb, out;
  mul_t acc, acc_w;
  int cnt_iter, cnt_per, cnt_addr;
  //count delay during a run():
  int run_delay = 0;
  versat_t output[MULADD_LAT]; //output FIFO
public:
  int versat_base, muladd_base;
  int sela, selb, fns, iter, per, delay, shift;

  //Default constructor
  CMulAdd()
  {
  }

  CMulAdd(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->muladd_base = i;
  }

  //set MulAdd configuration to shadow register
  void update_shadow_reg_MulAdd()
  {
    shadow_reg[versat_base].muladd[muladd_base].sela = conf[versat_base].muladd[muladd_base].sela;
    shadow_reg[versat_base].muladd[muladd_base].selb = conf[versat_base].muladd[muladd_base].selb;
    shadow_reg[versat_base].muladd[muladd_base].fns = conf[versat_base].muladd[muladd_base].fns;
    shadow_reg[versat_base].muladd[muladd_base].iter = conf[versat_base].muladd[muladd_base].iter;
    shadow_reg[versat_base].muladd[muladd_base].per = conf[versat_base].muladd[muladd_base].per;
    shadow_reg[versat_base].muladd[muladd_base].delay = conf[versat_base].muladd[muladd_base].delay;
    shadow_reg[versat_base].muladd[muladd_base].shift = conf[versat_base].muladd[muladd_base].shift;
  }

  //start run
  void start_run()
  {
    //set run_delay
    run_delay = shadow_reg[versat_base].muladd[muladd_base].delay;

    //set addrgen counter variables
    cnt_iter = 0;
    cnt_per = 0;
    cnt_addr = 0;
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
      run_delay--;
    }
    else
    {

      //update databus
      stage[versat_base].databus[sMULADD[muladd_base]] = output[MULADD_LAT - 1];
      //special case for stage 0
      if (versat_base == 0)
      {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sMULADD[muladd_base]] = output[MULADD_LAT - 1];
      }

      //trickle down all outputs in buffer
      for (i = 1; i < MULADD_LAT; i++)
      {
        output[i] = output[i - 1];
      }
      //insert new output
      output[0] = out;
    }
  }

  versat_t output() //implemented as PIPELINED MULADD
  {
    //wait for delay to end
    if (run_delay > 0)
    {
      return 0;
    }

    //select inputs
    opa = stage[versat_base].databus[shadow_reg[versat_base].muladd[muladd_base].sela];
    opb = stage[versat_base].databus[shadow_reg[versat_base].muladd[muladd_base].selb];

    //select acc_w value
    acc_w = (cnt_addr == 0) ? 0 : acc;

    //perform MAC operation
    mul_t result_mult = opa * opb;
    if (shadow_reg[versat_base].muladd[muladd_base].fns == MULADD_MACC)
    {
      acc = acc_w + result_mult;
    }
    else
    {
      acc = acc_w - result_mult;
    }
    out = (versat_t)(acc >> shadow_reg[versat_base].muladd[muladd_base].shift);

    //update addrgen counter - 1 iteration of nested for loop
    if (cnt_iter < shadow_reg[versat_base].muladd[muladd_base].iter)
    {
      if (cnt_per < shadow_reg[versat_base].muladd[muladd_base].per)
      {
        cnt_addr++;
        cnt_per++;
      }
      else
      {
        cnt_per = 0;
        cnt_addr += -(shadow_reg[versat_base].muladd[muladd_base].per);
        cnt_iter++;
      }
    }

    return out;
  }

  void setConf(int sela, int selb, int fns, int iter, int per, int delay, int shift)
  {
    this->sela = sela;
    this->selb = selb;
    this->fns = fns;
    this->iter = iter;
    this->per = per;
    this->delay = delay;
    this->shift = shift;
  }

  void writeConf()
  {
    conf[versat_base].muladd[muladd_base].sela = sela;
    conf[versat_base].muladd[muladd_base].selb = selb;
    conf[versat_base].muladd[muladd_base].fns = fns;
    conf[versat_base].muladd[muladd_base].iter = iter;
    conf[versat_base].muladd[muladd_base].per = per;
    conf[versat_base].muladd[muladd_base].delay = delay;
    conf[versat_base].muladd[muladd_base].shift = shift;
  }
  void setSelA(int sela)
  {
    conf[versat_base].muladd[muladd_base].sela = sela;
    this->sela = sela;
  }
  void setSelB(int selb)
  {
    conf[versat_base].muladd[muladd_base].selb = selb;
    this->selb = selb;
  }
  void setFNS(int fns)
  {
    conf[versat_base].muladd[muladd_base].fns = fns;
    this->fns = fns;
  }
  void setIter(int iter)
  {
    conf[versat_base].muladd[muladd_base].iter = iter;
    this->iter = iter;
  }
  void setPer(int per)
  {
    conf[versat_base].muladd[muladd_base].per = per;
    this->per = per;
  }
  void setDelay(int delay)
  {
    conf[versat_base].muladd[muladd_base].delay = delay;
    this->delay = delay;
  }
  void setShift(int shift)
  {
    conf[versat_base].muladd[muladd_base].shift = shift;
    this->shift = shift;
  }
}; //end class CMULADD
#endif

#if nMULADDLITE > 0
class CMulAddLite
{
private:
  //count delay during a run():
  int run_delay = 0;
  versat_t output[MULADDLITE_LAT]; //output FIFO
public:
  int versat_base, muladdlite_base;
  int sela, selb, iter, per, delay, shift;
  int selc = 0, accIN = 0, accOUT = 0, batch = 0;

  //Default constructor
  CMulAddLite()
  {
  }

  CMulAddLite(int versat_base, int i)
  {
    this->versat_base = versat_base;
    this->muladdlite_base = i;
  }

  //set MulAdd configuration to shadow register
  void update_shadow_reg_MulAddLite()
  {
    shadow_reg[versat_base].muladdlite[muladdlite_base].sela = conf[versat_base].muladdlite[muladdlite_base].sela;
    shadow_reg[versat_base].muladdlite[muladdlite_base].selb = conf[versat_base].muladdlite[muladdlite_base].selb;
    shadow_reg[versat_base].muladdlite[muladdlite_base].selc = conf[versat_base].muladdlite[muladdlite_base].selc;
    shadow_reg[versat_base].muladdlite[muladdlite_base].iter = conf[versat_base].muladdlite[muladdlite_base].iter;
    shadow_reg[versat_base].muladdlite[muladdlite_base].per = conf[versat_base].muladdlite[muladdlite_base].per;
    shadow_reg[versat_base].muladdlite[muladdlite_base].delay = conf[versat_base].muladdlite[muladdlite_base].delay;
    shadow_reg[versat_base].muladdlite[muladdlite_base].shift = conf[versat_base].muladdlite[muladdlite_base].shift;
    shadow_reg[versat_base].muladdlite[muladdlite_base].accIN = conf[versat_base].muladdlite[muladdlite_base].accIN;
    shadow_reg[versat_base].muladdlite[muladdlite_base].accOUT = conf[versat_base].muladdlite[muladdlite_base].accOUT;
    shadow_reg[versat_base].muladdlite[muladdlite_base].batch = conf[versat_base].muladdlite[muladdlite_base].batch;
  }

  //start run
  void start_run()
  {
    //set run_delay
    run_delay = shadow_reg[versat_base].muladdlite[muladdlite_base].delay;
  }

  //update output buffer, write results to databus
  void update()
  {
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
      run_delay--;
    }
    else
    {

      //update databus
      stage[versat_base].databus[sMULADDLITE[muladdlite_base]] = output[MULADDLITE_LAT - 1];
      //special case for stage 0
      if (versat_base == 0)
      {
        //2nd copy at the end of global databus
        global_databus[nSTAGE * N + sMULADDLITE[muladdlite_base]] = output[MULADDLITE_LAT - 1];
      }

      //trickle down all outputs in buffer
      for (i = 1; i < MULADDLITE_LAT; i++)
      {
        output[i] = output[i - 1];
      }
      //insert new output
      output[0] = out;
    }
  }

  versat_t output() //TO DO: need to implemente MulAddLite functionalities
  {

    //select inputs
    opa = stage[versat_base].databus[shadow_reg[versat_base].muladdlite[muladdlite_base].sela];
    opb = stage[versat_base].databus[shadow_reg[versat_base].muladdlite[muladdlite_base].selb];

    mul_t result_mult = opa * opb;
    if (shadow_reg[versat_base].muladdlite[muladdlite_base].fns == MULADD_MACC)
    {
      acc = acc_w + result_mult;
    }
    else
    {
      acc = acc_w - result_mult;
    }
    acc_w = (versat_t)(acc >> shift);

    return acc_w;
  }

  void setConf(int sela, int selb, int iter, int per, int delay, int shift)
  {
    this->sela = sela;
    this->selb = selb;
    this->iter = iter;
    this->per = per;
    this->delay = delay;
    this->shift = shift;
  }

  void setConf(int selc, int accIN, int accOUT, int batch)
  {
    this->selc = selc;
    this->accIN = accIN;
    this->accOUT = accOUT;
    this->batch = batch;
  }

  void writeConf()
  {
    conf[versat_base].muladdlite[muladdlite_base].sela = sela;
    conf[versat_base].muladdlite[muladdlite_base].selb = selb;
    conf[versat_base].muladdlite[muladdlite_base].selc = selc;
    conf[versat_base].muladdlite[muladdlite_base].iter = iter;
    conf[versat_base].muladdlite[muladdlite_base].per = per;
    conf[versat_base].muladdlite[muladdlite_base].delay = delay;
    conf[versat_base].muladdlite[muladdlite_base].shift = shift;
    conf[versat_base].muladdlite[muladdlite_base].accIN = accIN;
    conf[versat_base].muladdlite[muladdlite_base].accOUT = accOUT;
    conf[versat_base].muladdlite[muladdlite_base].batch = batch;
  }
  void setSelA(int sela)
  {
    conf[versat_base].muladdlite[muladdlite_base].sela = sela;
    this->sela = sela;
  }
  void setSelB(int selb)
  {
    conf[versat_base].muladdlite[muladdlite_base].selb = selb;
    this->selb = selb;
  }
  void setSelC(int selc)
  {
    conf[versat_base].muladdlite[muladdlite_base].selc = selc;
    this->selc = selc;
  }
  void setIter(int iter)
  {
    conf[versat_base].muladdlite[muladdlite_base].iter = iter;
    this->iter = iter;
  }
  void setPer(int per)
  {
    conf[versat_base].muladdlite[muladdlite_base].per = per;
    this->per = per;
  }
  void setDelay(int delay)
  {
    conf[versat_base].muladdlite[muladdlite_base].delay = delay;
    this->delay = delay;
  }
  void setShift(int shift)
  {
    conf[versat_base].muladdlite[muladdlite_base].shift = shift;
    this->shift = shift;
  }
  void setAccIN(int accIN)
  {
    conf[versat_base].muladdlite[muladdlite_base].accIN = accIN;
    this->accIN = accIN;
  }
  void setAccOUT(int accOUT)
  {
    conf[versat_base].muladdlite[muladdlite_base].accOUT = accOUT;
    this->accOUT = accOUT;
  }
  void setBatch(int batch)
  {
    conf[versat_base].muladdlite[muladdlite_base].batch = batch;
    this->batch = batch;
  }
}; //end class CMULADDLITE
#endif

class CStage
{
private:
public:
  int versat_base;
  versat_t *databus;
  //versat_t* databus[N*2];
  //Versat Function Units
  CMemPort memA[nMEM];
  CMemPort memB[nMEM];
  CMem mem[nMEM];
#if nALU > 0
  CALU alu[nALU];
#endif
#if nALULITE > 0
  CALULite alulite[nALULITE];
#endif
#if nBS > 0
  CBS bs[nBS];
#endif
#if nMUL > 0
  CMul mul[nMUL];
#endif
#if nMULADD > 0
  CMulAdd muladd[nMULADD];
#endif
#if nMULADDLITE > 0
  CMulAddLite muladdlite[nMULADDLITE];
#endif

  //Default constructor
  CStage()
  {
  }
  //Default Constructor
  CStage(int versat_base)
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
    for (i = 0; i < nMEM; i++)
      memA[i] = CMemPort(versat_base, i, 0);
    for (i = 0; i < nMEM; i++)
      memB[i] = CMemPort(versat_base, i, 1);
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
  void clearConf()
  {
    int i = versat_base;
    memset(&conf[versat_base], 0, sizeof(conf[versat_base]));
    conf[i] = CStage(i);
  }

#ifdef CONF_MEM_USE
  //write current config in conf_mem
  void confMemWrite(int addr)
  {
    if (addr < CONF_MEM_SIZE)
      MEMSET(versat_base, (CONF_BASE + CONF_MEM + addr), 0);
  }

  //set addressed config in conf_mem as current config
  void confMemRead(int addr)
  {
    if (addr < CONF_MEM_SIZE)
      MEMGET(versat_base, (CONF_BASE + CONF_MEM + addr));
  }
#endif

  //update shadow register with current configuration
  void update_shadow_reg()
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
  void start_all_FUs()
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
  void update_all_FUs()
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
  void output_all_FUs()
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

}; //end class CStage

//
//VERSAT global variables
//
#ifndef VERSAT_cpp // include guard
#define VERSAT_cpp
static int base;
CStage stage[nSTAGE];
CStage conf[nSTAGE];
CStage shadow_reg[nSTAGE];

/*databus vector
stage 0 is repeated in the start and at the end
stage order in databus
[ 0 | nSTAGE-1 | nSTAGE-2 | ... | 2  | 1 | 0 ]
^                                    ^
|                                    |
stage 0 databus                      stage 1 databus

*/
versat_t global_databus[(nSTAGE + 1) * N];

int sMEMA[nMEM], sMEMA_p[nMEM], sMEMB[nMEM], sMEMB_p[nMEM];
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

//
//VERSAT FUNCTIONS
//
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

int run_done = 0;

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
#endif

#endif
