`timescale 1ns / 1ps
`include "xdefs.vh"

module xconf_reg_tb;
                 parameter                   clk_per = 10;
		 reg 			     clk;
		 reg 			     rst;
		 reg [`ADDR_W-1:0]       conf_addr;
		 reg [`DATA_W-1:0]           conf_word;
		 reg 			     conf_req;
		 reg 			     conf_rw_rnw;
		 reg [`CONF_BITS-1:0]      conf_in;
		 reg 			     conf_ld;
                 wire [`CONF_BITS-1:0]     conf_out;

      xconf_reg uut (
                 .clk(clk),
                 .rst(rst),
	         .conf_addr(conf_addr),
	         .conf_word(conf_word),
		 .conf_req(conf_req),
		 .conf_rnw(conf_rw_rnw),
	         .conf_in(conf_in),
	         .conf_ld(conf_ld),
                 .conf_out(conf_out)
       );


      initial begin
	 
`ifdef DEBUG
         $dumpfile("conf_reg.vcd");
         $dumpvars();
`endif
	 
         // Initialize Inputs
         clk=0;
         rst=1'b0;
         conf_ld=1'b0;
	 conf_req=1;
	 conf_rw_rnw=0;
         conf_in=0;
         conf_word=0;
         conf_addr=`MEM0A_CONFIG_ADDR;
         

         //Test reset
         #(clk_per/2-2)
         //conf_clr=1'b1;
         rst=1'b1;
         #clk_per
         rst=1'b0;
         conf_ld=1'b1;
         conf_in[416]=1'b1;
         #clk_per
         conf_ld=1'b0;
         conf_word[2:0]=$random;
         conf_addr=`MEM0A_CONFIG_ADDR;
         //Test mem's
         repeat(8) begin
            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;
            

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;
            
            conf_word=$random;
            #clk_per;
            conf_addr=conf_addr+1;
         end

         //Test ALU's
         repeat(4) begin
            conf_word[`ALU_FNS_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;
         end

         //Test Mult's
         repeat(4) begin
            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;
         end

         //Test BS's
         repeat(2) begin
            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[`N_W-1:0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;

            conf_word[0]=$random;
            #clk_per;
            conf_addr=conf_addr+1;
         end

         #100 $finish;
      end
 

      always
         #(clk_per/2)  clk =  ~ clk;

endmodule
