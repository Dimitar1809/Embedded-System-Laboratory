`timescale 1 ps / 1 ps
module bus_comm #(
        parameter DATA_WIDTH = 32,
        parameter COUNTER_WIDTH = 16
	) (
		input  wire [7:0]  slave_address,     //      avs_s0.address
		input  wire        slave_read,        //            .read
		output reg  [DATA_WIDTH-1:0] slave_readdata,    //            .readdata
		input  wire        slave_write,       //            .write
		input  wire [DATA_WIDTH-1:0] slave_writedata,   //            .writedata
		input  wire        clk,          //       clock.clk
		input  wire        reset,        //       reset.reset
        input  wire [(DATA_WIDTH/8)-1:0] slave_byteenable,
        input wire      yaw_channel_a, // Yaw channel A
        input wire      yaw_channel_b, // Yaw channel B
        input wire      pitch_channel_a, // Pitch channel A
        input wire      pitch_channel_b  // Pitch channel B

	);
    localparam HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE = 32'hFF200000;
    localparam ADDR_YAW_COUNTER =  HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE;
    localparam ADDR_PITCH_COUNTER = HPS_0_ARM_A9_0_ESL_BUS_DEMO_0_BASE + COUNTER_WIDTH;

    wire [COUNTER_WIDTH-1:0] yaw_counter;
    wire [COUNTER_WIDTH-1:0] pitch_counter;

    // Definition of the counter
    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) yaw_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(yaw_channel_a),
        .channel_b(yaw_channel_b),
        .counter(yaw_counter)
    );

    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) pitch_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(pitch_channel_a),
        .channel_b(pitch_channel_b),
        .counter(pitch_counter)
    );

    always @(posedge clk or posedge reset) begin
        if (reset) begin
        end else begin
            if (slave_read) begin
                case (slave_address)
                    ADDR_YAW_COUNTER: slave_readdata <= yaw_counter;
                    ADDR_PITCH_COUNTER: slave_readdata <= pitch_counter;
                    default: slave_readdata <= 32'b0;
                endcase
            end
            if (slave_write) begin
                //mem <= slave_writedata;
            end;
        end;
    end



endmodule