module pwm_gen(
    input clk,
    input reset,
    input [13:0] duty_cycle, // 16-bit duty cycle value
    input [1:0] direction,

    output reg pwm_out_c, // PWM output
    output reg pwm_out_ina,
    output reg pwm_out_inb
);

localparam PWM_PERIOD = 5000; // 100 MHz / 20kHz = 5000 cycles
reg [15:0] counter; // 16-bit counter for PWM period

always @(posedge clk or posedge reset) begin
    if (reset) begin
        pwm_out_c <= 0;
        counter <= 0;
    end else begin
        // Generate PWM signal based on duty cycle
        if (counter >= PWM_PERIOD) begin
            counter <= 0;
        end else begin
            counter <= counter + 1;
        end

        if (counter < duty_cycle) begin
            pwm_out_c <= 1; // Set PWM output high
        end else begin
            pwm_out_c <= 0; // Set PWM output low
        end
    end
end

// Direction control logic
always @(direction) begin
    case (direction)
        2'b10: begin // clockwise
            pwm_out_ina <= 1;
            pwm_out_inb <= 0;
        end
        2'b01: begin // counterclockwise
            pwm_out_ina <= 0;
            pwm_out_inb <= 1;
        end
        2'b00: begin // stop
            pwm_out_ina <= 0;
            pwm_out_inb <= 0;
        end
        2'b11: begin // brake
            pwm_out_ina <= 1;
            pwm_out_inb <= 1;
        end
        default: begin // Stop
            pwm_out_ina <= 0;
            pwm_out_inb <= 0;
        end
    endcase
end

endmodule