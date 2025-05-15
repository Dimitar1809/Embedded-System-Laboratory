`timescale 1 ps / 1 ps
module de10_top #(
        parameter DATA_WIDTH = 32,
        parameter COUNTER_WIDTH = 16
	) (
		input  wire [7:0]                   slave_address,     //      avs_s0.address
		input  wire                         slave_read,        //            .read
		output reg  [DATA_WIDTH-1:0]        slave_readdata,    //            .readdata
		input  wire                         slave_write,       //            .write
		input  wire [DATA_WIDTH-1:0]        slave_writedata,   //            .writedata
		input  wire        clk,          //       clock.clk
		input  wire        reset,        //       reset.reset
        input  wire [(DATA_WIDTH/8)-1:0]    slave_byteenable,

        input wire                          yaw_enc_a,
        input wire                          yaw_enc_b,

        output wire                         yaw_pwm_val,
        output wire                         yaw_pwm_dira,
        output wire                         yaw_pwm_dirb,

        input wire                          pitch_enc_a,
        input wire                          pitch_enc_b,

        output wire                         pitch_pwm_val,
        output wire                         pitch_pwm_dira,
        output wire                         pitch_pwm_dirb

	);
    localparam READ_COUNTERS_OFFSET   = 8'h00;

    reg [13:0] yaw_duty_cycle;
    reg [1:0] yaw_direction;
    reg [13:0] pitch_duty_cycle;
    reg [1:0] pitch_direction;
    
    wire [COUNTER_WIDTH-1:0] mem_masked_yaw;
    wire [COUNTER_WIDTH-1:0] mem_masked_pitch;

    jiwy #(
        .COUNTER_WIDTH(COUNTER_WIDTH),
        .DUTY_CYCLE_WIDTH(14)
    ) jiwy_inst (
        .clk(clk),
        .reset(reset),
        .yaw_enc_a(yaw_enc_a),
        .yaw_enc_b(yaw_enc_b),
        .yaw_enc_counter(mem_masked_yaw),
        .yaw_duty_cycle(yaw_duty_cycle),
        .yaw_direction(yaw_direction),
        .yaw_pwm_val(yaw_pwm_val),
        .yaw_pwm_dira(yaw_pwm_dira),
        .yaw_pwm_dirb(yaw_pwm_dirb),

        .pitch_enc_a(pitch_enc_a),
        .pitch_enc_b(pitch_enc_b),
        .pitch_enc_counter(mem_masked_pitch),
        .pitch_duty_cycle(pitch_duty_cycle),
        .pitch_direction(pitch_direction),
        .pitch_pwm_val(pitch_pwm_val),
        .pitch_pwm_dira(pitch_pwm_dira),
        .pitch_pwm_dirb(pitch_pwm_dirb)
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
                pitch_direction <= slave_writedata[1:0];
                pitch_duty_cycle <= slave_writedata[15:2];
                yaw_direction <= slave_writedata[17:16];
                yaw_duty_cycle <= slave_writedata[31:18];
                
            end;
        end;
    end



endmodule