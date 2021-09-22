`timescale 1ns / 1ps
`include "xversat.vh"

module xaddrgen # (
		 parameter				MEM_ADDR_W = `MEM_ADDR_W,
		 parameter				PERIOD_W = `PERIOD_W
		) (
		 input                         		clk,
		 input                         		rst,
		 input                         		init,
		 input                        		run,
		 input 					pause,      

		 //configurations 
		 input [MEM_ADDR_W - 1:0]        	iterations,
		 input [PERIOD_W - 1:0]       		period,
		 input [PERIOD_W - 1:0]       		duty,
		 input [PERIOD_W - 1:0]       		delay,
		 input [MEM_ADDR_W - 1:0]        	start,
		 input signed [MEM_ADDR_W - 1:0] 	shift,
		 input signed [MEM_ADDR_W - 1:0] 	incr,

		 //outputs 
		 output reg [MEM_ADDR_W - 1:0]   	addr,
		 output reg                    		mem_en,
		 output reg                    		done
		 );

   reg signed [PERIOD_W :0]                   		per_cnt, per_cnt_nxt; //period count
   wire [PERIOD_W :0]                         		period_int, duty_int;
   
   reg [MEM_ADDR_W - 1:0]                        	addr_nxt, addr_r0, addr_r1;
   
   reg [MEM_ADDR_W - 1:0]                        	iter, iter_nxt; //iterations count 

   reg 					       		mem_en_nxt;
   reg 					       		done_nxt;
   reg							run_reg, run_nxt;
   reg							rep, rep_nxt;

   parameter IDLE=1'b0, RUN=1'b1;   
   reg 					       		state, state_nxt;
   
   assign period_int = {1'b0, period};
   assign duty_int = {1'b0, duty};

   always @ *  begin
      state_nxt = state;
      done_nxt  = done;
      mem_en_nxt = mem_en;
      per_cnt_nxt = per_cnt;
      addr_nxt = addr_r0;
      iter_nxt = iter;
      run_nxt = run_reg;
      rep_nxt = rep;

      if (state == IDLE) begin 
	 done_nxt = 1'b1;
	 mem_en_nxt = 1'b0;

	 if (init) begin 
	    per_cnt_nxt = -{{1'b0}, delay}+1'b1;
	    addr_nxt = start;
	    iter_nxt = {{MEM_ADDR_W-1{1'b0}}, 1'b1};
	 end 

	 if(run) begin
	    state_nxt = RUN;
	    done_nxt = 1'b0;
	    run_nxt = 1'b0;

	    if(delay == {PERIOD_W{1'b0}})
	      mem_en_nxt = 1'b1;
	    
	 end 
      end else begin //state = RUN

	 //check for successive run
	 if(run_reg) begin
	   done_nxt = 1'b0;
	   run_nxt = 1'b0;
	 end

	 //compute mem_en_nxt
	 if ((per_cnt == {{1'b0},{PERIOD_W{1'b0}}} && period == duty) || per_cnt == period_int)
	   mem_en_nxt = 1'b1;
	 
	 if (per_cnt == duty_int && (period != duty || iter == iterations))
	   mem_en_nxt = 1'b0;
	 
	 //compute per_cnt_nxt
	 per_cnt_nxt = per_cnt + 1'b1;
	 
	 if (per_cnt == period_int) 
	   per_cnt_nxt = {(PERIOD_W+1){1'b0}}+1'b1;

	 //compute iter_nxt
	 if (per_cnt == period_int) 
	   iter_nxt = iter + 1'b1;

	 //compute addr_nxt
	 if(mem_en)
	   addr_nxt = addr_r0 + incr;
	 
	 if (per_cnt == period_int) begin
	    addr_nxt = addr_r0 + incr + shift;
	 end
	 
	 if(mem_en == 1'b1 && per_cnt == duty_int && period != duty)
	   addr_nxt = addr_r0;
	 
	 //compute state_nxt
	 if(iter == iterations && per_cnt == period_int) begin 
	    if(rep) begin
	       done_nxt = 1'b1;
	       rep_nxt = 1'b0;
	    end else
	       done_nxt = 1'b0;
	    if(run) begin
	      addr_nxt = start;
	      iter_nxt = {{MEM_ADDR_W-1{1'b0}}, 1'b1};
	      mem_en_nxt = 1'b1;
	      run_nxt = 1'b1;
	    end else state_nxt = IDLE;
	 end else
	    rep_nxt = 1'b1;
	 
      end // else: !if(state == IDLE)
   end // always @ *
   

   // register update
   always @ (posedge clk) 
     if (rst) begin
	state <= IDLE;
	mem_en <= 1'b0;
	done <= 1'b1;
	addr_r0 <= {MEM_ADDR_W{1'b0}};
	addr_r1 <= {MEM_ADDR_W{1'b0}};
	per_cnt <= {PERIOD_W{1'b0}};
	iter <= {MEM_ADDR_W{1'b0}};
        run_reg <= 1'b0;
	rep <= 1'b1;
     end else begin 
	done <= done_nxt;
        run_reg <= run_nxt;
	rep <= rep_nxt;
	if (!pause) begin 
	   mem_en <= mem_en_nxt;
	   state <= state_nxt;
	   addr_r0 <= addr_nxt;
	   addr_r1 <= addr_r0;
	   per_cnt <= per_cnt_nxt;
	   iter <= iter_nxt;
	end
     end 
   assign addr = (period == duty) ? addr_r0 : addr_r1;

endmodule
