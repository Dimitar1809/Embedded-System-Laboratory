`timescale 1ns / 1ps

module pwm_gen_tb;

    // Parameters
    localparam CLK_PERIOD = 10; // For a 100 MHz clock (10 ns period)
    localparam PWM_PERIOD_TB = 5000; // Match DUT's PWM_PERIOD

    // Inputs to DUT
    reg clk;
    reg reset;
    reg [15:0] duty_cycle;
    reg [1:0] direction;

    // Outputs from DUT
    wire pwm_out_c;
    wire pwm_out_ina;
    wire pwm_out_inb;

    // Instantiate the Device Under Test (DUT)
    pwm_gen dut (
        .clk(clk),
        .reset(reset),
        .duty_cycle(duty_cycle),
        .direction(direction),
        .pwm_out_c(pwm_out_c),
        .pwm_out_ina(pwm_out_ina),
        .pwm_out_inb(pwm_out_inb)
    );

    // Clock generation
    always begin
        clk = 1'b0;
        #(CLK_PERIOD / 2);
        clk = 1'b1;
        #(CLK_PERIOD / 2);
    end

    // Test sequence
    initial begin
        $dumpfile("signals.vcd"); // Name of the signal dump file
        $dumpvars(0, pwm_gen_tb); // Dump all variables in this module
        $display("Starting PWM_GEN Testbench...");
        $monitor("Time=%0t: reset=%b, duty_cycle=%d, direction=%b => pwm_c=%b, ina=%b, inb=%b",
                 $time, reset, duty_cycle, direction, pwm_out_c, pwm_out_ina, pwm_out_inb);

        // 1. Initialize and Reset
        reset = 1'b1;
        duty_cycle = 0;
        direction = 2'b00; // Stop
        #(CLK_PERIOD * 5); // Hold reset for a few clock cycles

        reset = 1'b0;
        #(CLK_PERIOD * 2); // Wait for reset to de-assert

        // 2. Test Stop (0% duty cycle, direction stop)
        $display("Test Case 1: Stop (0%% Duty, Direction Stop)");
        duty_cycle = 0;
        direction = 2'b00; // Stop
        #(PWM_PERIOD_TB * CLK_PERIOD * 2); // Run for 2 PWM periods

        // 3. Test Clockwise (25% duty cycle)
        $display("Test Case 2: Clockwise (25%% Duty)");
        duty_cycle = PWM_PERIOD_TB / 4; // 25% duty cycle (5000 / 4 = 1250)
        direction = 2'b10; // Clockwise
        #(PWM_PERIOD_TB * CLK_PERIOD * 3); // Run for 3 PWM periods

        // 4. Test Counter-Clockwise (50% duty cycle)
        $display("Test Case 3: Counter-Clockwise (50%% Duty)");
        duty_cycle = PWM_PERIOD_TB / 2; // 50% duty cycle (5000 / 2 = 2500)
        direction = 2'b01; // Counter-Clockwise
        #(PWM_PERIOD_TB * CLK_PERIOD * 3);

        // 5. Test Brake (75% duty cycle - PWM still runs, direction is brake)
        $display("Test Case 4: Brake (75%% Duty, Direction Brake)");
        duty_cycle = (PWM_PERIOD_TB * 3) / 4; // 75% duty cycle (5000 * 3 / 4 = 3750)
        direction = 2'b11; // Brake
        #(PWM_PERIOD_TB * CLK_PERIOD * 3);

        // 6. Test Full Duty Cycle (Clockwise)
        $display("Test Case 5: Full Duty (Clockwise)");
        duty_cycle = PWM_PERIOD_TB; // Effectively 100% duty (high for PWM_PERIOD cycles, low for 1)
                                    // or PWM_PERIOD_TB -1 for always high except one cycle
        direction = 2'b10; // Clockwise
        #(PWM_PERIOD_TB * CLK_PERIOD * 3);

        // 7. Test change direction while running (e.g., from CW to Stop)
        $display("Test Case 6: Change Direction (CW to Stop)");
        duty_cycle = PWM_PERIOD_TB / 2; // 50%
        direction = 2'b10; // Clockwise
        #(PWM_PERIOD_TB * CLK_PERIOD * 1);
        direction = 2'b00; // Stop
        #(PWM_PERIOD_TB * CLK_PERIOD * 2);

        // 8. Test change duty cycle while running
        $display("Test Case 7: Change Duty Cycle (50%% to 10%%, CCW)");
        direction = 2'b01; // Counter-Clockwise
        duty_cycle = PWM_PERIOD_TB / 2; // 50%
        #(PWM_PERIOD_TB * CLK_PERIOD * 1);
        duty_cycle = PWM_PERIOD_TB / 10; // 10%
        #(PWM_PERIOD_TB * CLK_PERIOD * 2);

        $display("Testbench Finished.");
        $finish;
    end

endmodule