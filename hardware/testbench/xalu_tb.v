`timescale 1ns / 1ps

`include "xversat.vh"
`include "xaludefs.vh"

module xalu_tb;
   parameter clk_per = 10;
   parameter DATA_W = 32;
   integer k;
   integer 	  file;

   // control 
   reg 		  clk;
   reg 		  rst;
   
   //data
   wire [2*`DATABUS_W-1:0] data_bus;
   wire [DATA_W-1:0] alu_result;
   
   //config data
   wire [`ALU_CONF_BITS - 1:0] configdata;

   xalu # ( 
	.DATA_W(DATA_W)
   ) uut (
        .clk(clk),
        .rst(rst),
	.flow_in(data_bus),
	.flow_out(alu_result),
	.configdata(configdata)
	);

   reg [DATA_W-1:0] 		result_compare; 
   wire [DATA_W-1:0] 		pcmp_comp; 
   reg [`ALU_FNS_W-1:0] 	alu_fns; 
   reg [DATA_W-1:0] 		opa_comp; 
   reg [DATA_W-1:0] 		opb_comp;

   integer 			err_tb ;

   //use least significant DATABUS_W bits
   assign configdata[`ALU_FNS_W-1:0]= alu_fns;
   assign configdata[`ALU_CONF_BITS-1 -:`N_W] = 0; 
   assign configdata[`ALU_CONF_BITS-`N_W-1-:`N_W] = 1;
   assign data_bus[`DATA_MEM0A_B -: DATA_W]= opa_comp;
   assign data_bus[`DATA_MEM0A_B - DATA_W -: DATA_W]= opb_comp;

   initial begin
      
`ifdef DEBUG
      $dumpfile("xalu.vcd");
      $dumpvars();
`endif
      
      alu_fns = 0;
      err_tb=0;
      clk = 0;
      k=-1;
      rst=1;

      //reset tested, turn off reset
      #clk_per
      rst=0;
      #clk_per
      opa_comp = 25;
      opb_comp = 26;
      
      //test all functions 
      repeat(16) begin
         #clk_per k=k+1;
         alu_fns=alu_fns+1;
      end
      #clk_per
        k=k+1;

      if(err_tb==1)
        $display("\nALU test failed\n");
      else
        $display("\nALU test passed\n");

      #clk_per $finish;
   end

   always @ (posedge clk) begin

      if (rst == 1'b1)
        result_compare = {DATA_W{1'd0}};
      else
          case(alu_fns)
            `ALU_OR:        result_compare <= opa_comp | opb_comp ;

            `ALU_AND:       result_compare <= (opa_comp & opb_comp ) ;
	    
	    `ALU_MUX: begin
	      result_compare <= opb_comp;
	      if(opa_comp[31]) result_compare <= {DATA_W{1'b0}};
	    end

            `ALU_XOR:       result_compare <= (opa_comp ^ opb_comp ) ;

            `ALU_SEXT8:           result_compare[31:0] <= {{24{opa_comp[7]}}, opa_comp[7:0]}; 
            
            `ALU_SEXT16:          result_compare <= {{16{opa_comp[15]}}, opa_comp[15:0]} ;

            `ALU_SHIFTR_ARTH :    begin
               result_compare <= ( opa_comp>>1);
               result_compare[31] <= opa_comp[31];
            end
            
            `ALU_SHIFTR_LOG :     result_compare <= ( opa_comp>>1) ;

            `ALU_CMP_UNS :        result_compare <= opb_comp + ~opa_comp +1;

            `ALU_CMP_SIG :        result_compare <= opb_comp + ~opa_comp +1;

            `ALU_ADD :            result_compare <=  opa_comp + opb_comp;

            `ALU_SUB :            result_compare <=  opb_comp - opa_comp;

            `ALU_CLZ :          result_compare <= pcmp_comp;

            `ALU_MAX   :          if( opa_comp[31]==1'b1 ^ opb_comp[31]==1'b1)
              if ( (opa_comp < opb_comp) )
                result_compare <= opa_comp;
              else 
                result_compare <= opb_comp;
            else
              if ( (opa_comp > opb_comp) )
                result_compare <= opa_comp;
              else 
                result_compare <= opb_comp;

            `ALU_MIN    :         if( opa_comp[31]==1'b1 ^ opb_comp[31]==1'b1)
              if ( (opa_comp > opb_comp) )
                result_compare <= opa_comp;
              else 
                result_compare <= opb_comp;
            else
              if ( (opa_comp < opb_comp) )
                result_compare <= opa_comp;
              else 
                result_compare <= opb_comp;

            `ALU_ABS :            if ( opa_comp[31] ==1'b1 ) begin
               result_compare <= ~opa_comp+1;
            end else
              result_compare <= opa_comp;
            
          endcase
   end    
   
   xclz clz(
            .data_in(opa_comp),
	    .data_out(pcmp_comp[5:0]));

   assign pcmp_comp[DATA_W-1:6] = 0;
   
   
   always @ (negedge clk)
     if (result_compare != alu_result) begin
        $display ("ERROR AT TIME%d , OPERATION:%d ",$time , k);
        $display ("Expected value %d, Got Value %d", result_compare, alu_result);
        err_tb = 1;
     end

   always
     #(clk_per/2)  clk =  ~ clk;

endmodule
