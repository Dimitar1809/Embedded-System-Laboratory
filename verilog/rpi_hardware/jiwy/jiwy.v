`timescale 1 ps / 1 ps
module jiwy # (
    parameter COUNTER_WIDTH = 16,
    parameter DUTY_CYCLE_WIDTH = 14,
    parameter DATA_WIDTH = 32
) (
    input  wire     clk,          //       clock.clk
    input  wire     reset,        //       reset.reset

    input wire                          yaw_enc_a,
    input wire                          yaw_enc_b,
    output wire [COUNTER_WIDTH-1:0]     yaw_enc_counter,

    input wire [DUTY_CYCLE_WIDTH-1:0]   yaw_duty_cycle,
    input wire [1:0]                    yaw_direction,
    output wire                         yaw_pwm_val,
    output wire                         yaw_pwm_dira,
    output wire                         yaw_pwm_dirb,

    input wire                          pitch_enc_a,
    input wire                          pitch_enc_b,
    output wire [COUNTER_WIDTH-1:0]     pitch_enc_counter,

    input wire [DUTY_CYCLE_WIDTH-1:0]   pitch_duty_cycle,
    input wire [1:0]                    pitch_direction,
    output wire                         pitch_pwm_val,
    output wire                         pitch_pwm_dira,
    output wire                         pitch_pwm_dirb
);

    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) yaw_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(yaw_enc_a),
        .channel_b(yaw_enc_b),
        .counter(yaw_enc_counter)
    );

    quad_enc #(
        .COUNTER_WIDTH(COUNTER_WIDTH)
    ) pitch_enc (
        .clk(clk),
        .reset(reset),
        .channel_a(pitch_enc_a),
        .channel_b(pitch_enc_b),
        .counter(pitch_enc_counter)
    );

    pwm_gen yaw_pwm (  
        .clk(clk),
        .reset(reset),
        .duty_cycle(yaw_duty_cycle),
        .direction(yaw_direction),
        .pwm_out_c(yaw_pwm_val),
        .pwm_out_ina(yaw_pwm_dira),
        .pwm_out_inb(yaw_pwm_dirb)
    );

    pwm_gen pitch_pwm (  
        .clk(clk),
        .reset(reset),
        .duty_cycle(pitch_duty_cycle),
        .direction(pitch_direction),
        .pwm_out_c(pitch_pwm_val),
        .pwm_out_ina(pitch_pwm_dira),
        .pwm_out_inb(pitch_pwm_dirb)
    );

endmodule
    