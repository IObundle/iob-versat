`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module xaddrgen #(
   parameter MEM_ADDR_W = 10,
   parameter PERIOD_W   = 10
) (
   input clk_i,
   input rst_i,
   input init_i,
   input run_i,
   input pause_i,

   //configurations 
   input        [MEM_ADDR_W - 1:0] iterations_i,
   input        [  PERIOD_W - 1:0] period_i,
   input        [  PERIOD_W - 1:0] duty_i,
   input        [  PERIOD_W - 1:0] delay_i,
   input        [MEM_ADDR_W - 1:0] start_i,
   input signed [MEM_ADDR_W - 1:0] shift_i,
   input signed [MEM_ADDR_W - 1:0] incr_i,

   //outputs 
   output     [MEM_ADDR_W - 1:0] addr_o,
   output reg                    mem_en_o,
   output reg                    done_o
);

   reg signed [PERIOD_I_W : 0] per_cnt, per_cnt_nxt;  //period_i count
   wire [PERIOD_I_W : 0] period_i_int, duty_i_int;

   reg [MEM_ADDR_O_W - 1:0] addr_o_nxt, addr_o_r0, addr_o_r1;

   reg [MEM_ADDR_O_W - 1:0] iter, iter_nxt;  //iterations_i count 

   reg mem_en_o_nxt;
   reg done_o_nxt;
   reg run_i_reg, run_i_nxt;
   reg rep, rep_nxt;

   parameter IDLE = 1'b0, RUN_I = 1'b1;
   reg state, state_nxt;

   assign period_i_int = {1'b0, period_i};
   assign duty_i_int   = {1'b0, duty_i};

   always @* begin
      state_nxt    = state;
      done_o_nxt   = done_o;
      mem_en_o_nxt = mem_en_o;
      per_cnt_nxt  = per_cnt;
      addr_o_nxt   = addr_o_r0;
      iter_nxt     = iter;
      run_i_nxt    = run_i_reg;
      rep_nxt      = rep;

      if (state == IDLE) begin
         done_o_nxt   = 1'b1;
         mem_en_o_nxt = 1'b0;

         if (init_i) begin
            per_cnt_nxt = -{{1'b0}, delay_i} + 1'b1;
            addr_o_nxt  = start_i;
            iter_nxt    = {{MEM_ADDR_O_W - 1{1'b0}}, 1'b1};
         end

         if (run_i) begin
            state_nxt  = RUN_I;
            done_o_nxt = 1'b0;
            run_i_nxt  = 1'b0;

            if (delay_i == {PERIOD_I_W{1'b0}}) mem_en_o_nxt = 1'b1;

         end
      end else begin  //state = RUN_I

         //check for successive run_i
         if (run_i_reg) begin
            done_o_nxt = 1'b0;
            run_i_nxt  = 1'b0;
         end

         //compute mem_en_o_nxt
         if ((per_cnt == {{1'b0},{PERIOD_I_W{1'b0}}} && period_i == duty_i) || per_cnt == period_i_int)
            mem_en_o_nxt = 1'b1;

         if (per_cnt == duty_i_int && (period_i != duty_i || iter == iterations_i))
            mem_en_o_nxt = 1'b0;

         //compute per_cnt_nxt
         per_cnt_nxt = per_cnt + 1'b1;

         if (per_cnt == period_i_int) per_cnt_nxt = {(PERIOD_I_W + 1) {1'b0}} + 1'b1;

         //compute iter_nxt
         if (per_cnt == period_i_int) iter_nxt = iter + 1'b1;

         //compute addr_o_nxt
         if (mem_en_o) addr_o_nxt = addr_o_r0 + incr_i;

         if (per_cnt == period_i_int) begin
            addr_o_nxt = addr_o_r0 + incr_i + shift_i;
         end

         if (mem_en_o == 1'b1 && per_cnt == duty_i_int && period_i != duty_i)
            addr_o_nxt = addr_o_r0;

         //compute state_nxt
         if (iter == iterations_i && per_cnt == period_i_int) begin
            if (rep) begin
               done_o_nxt = 1'b1;
               rep_nxt    = 1'b0;
            end else done_o_nxt = 1'b0;
            if (run_i) begin
               addr_o_nxt   = start_i;
               iter_nxt     = {{MEM_ADDR_O_W - 1{1'b0}}, 1'b1};
               mem_en_o_nxt = 1'b1;
               run_i_nxt    = 1'b1;
            end else state_nxt = IDLE;
         end else rep_nxt = 1'b1;

      end  // else: !if(state == IDLE)
   end  // always @ *


   // register update
   always @(posedge clk_i)
      if (rst_i) begin
         state     <= IDLE;
         mem_en_o  <= 1'b0;
         done_o    <= 1'b1;
         addr_o_r0 <= {MEM_ADDR_O_W{1'b0}};
         addr_o_r1 <= {MEM_ADDR_O_W{1'b0}};
         per_cnt   <= {(PERIOD_I_W + 1) {1'b0}};
         iter      <= {MEM_ADDR_O_W{1'b0}};
         run_i_reg <= 1'b0;
         rep       <= 1'b1;
      end else begin
         done_o    <= done_o_nxt;
         run_i_reg <= run_i_nxt;
         rep       <= rep_nxt;
         if (!pause_i) begin
            mem_en_o  <= mem_en_o_nxt;
            state     <= state_nxt;
            addr_o_r0 <= addr_o_nxt;
            addr_o_r1 <= addr_o_r0;
            per_cnt   <= per_cnt_nxt;
            iter      <= iter_nxt;
         end
      end
   assign addr_o = (period_i == duty_i) ? addr_o_r0 : addr_o_r1;

endmodule  // xaddrgen

`default_nettype wire
