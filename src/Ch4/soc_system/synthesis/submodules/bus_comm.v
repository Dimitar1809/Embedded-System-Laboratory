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
        input wire      pitch_channel_b, // Pitch channel B
        output wire     pwm_yaw_c, // Yaw PWM output
        output wire     pwm_yaw_ina, // Yaw PWM INA output
        output wire     pwm_yaw_inb, // Yaw PWM INB output
        output wire     pwm_pitch_c, // Pitch PWM output
        output wire     pwm_pitch_ina, // Pitch PWM INA output
        output wire     pwm_pitch_inb // Pitch PWM INB output

	);
    localparam READ_COUNTERS_OFFSET   = 8'h00;

    reg [13:0] yaw_duty_cycle;
    reg [1:0] yaw_direction;
    reg [13:0] pitch_duty_cycle;
    reg [1:0] pitch_direction;
    
    wire [COUNTER_WIDTH-1:0] mem_masked_yaw;
    wire [COUNTER_WIDTH-1:0] mem_masked_pitch;

    // Definition of the counter
    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) yaw_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(yaw_channel_a),
        .channel_b(yaw_channel_b),
        .counter(mem_masked_yaw)
    );

    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) pitch_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(pitch_channel_a),
        .channel_b(pitch_channel_b),
        .counter(mem_masked_pitch)
    );

    pwm_gen yaw_pwm (  
        .clk(clk),
        .reset(reset),
        .duty_cycle(yaw_duty_cycle),
        .direction(yaw_direction),
        .pwm_out_c(pwm_yaw_c),
        .pwm_out_ina(pwm_yaw_ina),
        .pwm_out_inb(pwm_yaw_inb)
    );

    pwm_gen pitch_pwm (  
        .clk(clk),
        .reset(reset),
        .duty_cycle(pitch_duty_cycle),
        .direction(pitch_direction),
        .pwm_out_c(pwm_pitch_c),
        .pwm_out_ina(pwm_pitch_ina),
        .pwm_out_inb(pwm_pitch_inb)
    );
    

    always @(posedge clk or posedge reset) begin
        if (reset) begin
        end else begin
            if (slave_read) begin
                case (slave_address)
                    READ_COUNTERS_OFFSET: slave_readdata <= {mem_masked_pitch, mem_masked_yaw};
                    default: slave_readdata <= 32'b0;
                endcase
            end
            if (slave_write) begin
                pitch_duty_cycle <= slave_writedata[13:0];
                pitch_direction <= slave_writedata[15:14];
                yaw_duty_cycle <= slave_writedata[29:16];
                yaw_direction <= slave_writedata[31:30];
            end;
        end;
    end



endmodule