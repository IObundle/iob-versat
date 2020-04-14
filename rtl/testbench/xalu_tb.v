`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xaludefs.vh"

module xalu_tb;
   parameter clk_per = 10;
   integer k;
   integer 	  file;
   // control 
   reg 		  clk;
   reg 		  rst;
   
   //controller interface
   reg 		  rw_req;
   reg 		  rw_rnw;
   reg [`DATA_W-1:0] rw_data_to_wr;

   //data
   wire [`N*`DATA_W-1:0] data_bus;
   wire [31:0] 		alu_result;
   wire 		c_out;
   
   //config data
   wire [`ALU_CONF_BITS - 1:0] configdata;

   xalu uut (
             .clk(clk),
             .rst(rst),
	     .rw_req(rw_req),
	     .rw_rnw(rw_rnw),
   	     .rw_data_to_wr(rw_data_to_wr),
	     .data_bus(data_bus),
	     .alu_result(alu_result),
	     .c_out(c_out),
	     .configdata(configdata)
	     );

   reg [`DATA_W-1:0] 		result_compare; 
   wire [`DATA_W-1:0] 		pcmp_comp; 
   reg [`ALU_FNS_W-1:0] 	alu_fns; 
   reg [`DATA_W-1:0] 		opa_comp; 
   reg [`DATA_W-1:0] 		opb_comp;

   integer 			err_tb ;

   assign configdata[`ALU_FNS_W-1:0]= alu_fns;
   assign configdata[`ALU_CONF_BITS-1 -:`N_W]=`s0;
   assign configdata[`ALU_CONF_BITS-`N_W-1-:`N_W]=`s1;

   assign data_bus[`DATA_BITS-`DATA_W*(`s0-1)-1 -: `DATA_W]= opa_comp;
   assign data_bus[`DATA_BITS-`DATA_W*(`s1-1)-1 -: `DATA_W]= opb_comp;


   
   initial begin
      
`ifdef DEBUG
      $dumpfile("xalu.vcd");
      $dumpvars();
`endif
      
      alu_fns = 0;
      
      err_tb=0;
      clk = 0;
      rw_req=0;
      rw_rnw=0;
      k=-1;
      rw_data_to_wr=32'h0000;
      rst=1;

      #(clk_per/2+1);
      //reset tested, turn off reset
      rst=0;

      //rw_req=1 and rw_rnw=0
      rw_req=1;
      rw_data_to_wr=32'd20;
      #clk_per
        rst=0;
      rw_rnw=1;

      opa_comp = 25;
      opb_comp = 26;
      
      //test all functions 
      repeat(16) begin
         #clk_per k=k+1;
         alu_fns=alu_fns+1;
      end
      #clk_per
        k=k+1;

      file = $fopen ("xalu_tb_result.txt", "w");
      if(err_tb==1)
        $fwrite (file, "Failed");
      else
        $fwrite (file, "Passed");

      #clk_per $finish;
   end

   always @ (posedge clk) begin

      if (rst == 1'b1)
        result_compare=32'd0;
      else
        if ( rw_req & ~rw_rnw )
          result_compare <= rw_data_to_wr;
        else
          case(alu_fns)
            `ALU_LOGIC_OR:        result_compare <= opa_comp | opb_comp ;

            `ALU_LOGIC_AND:       result_compare <= (opa_comp & opb_comp ) ;
	    
            `ALU_LOGIC_ANDN:      result_compare <= (opa_comp & ~opb_comp ) ;

            `ALU_LOGIC_XOR:       result_compare <= (opa_comp ^ opb_comp ) ;

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

   assign pcmp_comp[`DATA_W-1:6] = 0;
   
   
   always @ (negedge clk)
     if (result_compare != alu_result) begin
        $display ("ERROR AT TIME%d , OPERATION:%d ",$time , k);
        $display ("Expected value %d, Got Value %d", result_compare, alu_result);
        err_tb = 1;
     end

   always
     #(clk_per/2)  clk =  ~ clk;

endmodule
