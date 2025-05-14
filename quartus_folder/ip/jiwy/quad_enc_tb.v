`timescale 1ns/1ps

module quad_enc_tb;

    // Define a variable COUNTER_WIDTH
    parameter COUNTER_WIDTH = 16;

    reg clk;
    reg reset;
    reg channel_a;
    reg channel_b;
    wire signed [COUNTER_WIDTH-1:0] counter;
    integer i;

    // Instantiate UUT with parameter override
    quad_enc #(.COUNTER_WIDTH(COUNTER_WIDTH)) uut (
        .clk(clk),
        .reset(reset),
        .channel_a(channel_a),
        .channel_b(channel_b),
        .counter(counter)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    initial begin
        $dumpfile("signals.vcd"); // Name of the signal dump file
        $dumpvars(0, quad_enc_tb); 
        // Initialize
        channel_a = 0;
        channel_b = 0;
        reset = 1;
        #10;
        reset = 0;
        #10;

        for (i = 0; i < 2; i = i + 1) begin
            {channel_a, channel_b} = 2'b00; #10; 
            {channel_a, channel_b} = 2'b01; #10;
            {channel_a, channel_b} = 2'b11; #10;
            {channel_a, channel_b} = 2'b10; #10;
            {channel_a, channel_b} = 2'b00; #10;
        end

        for (i = 0; i < 2; i = i + 1) begin
            {channel_a, channel_b} = 2'b00; #10;
            {channel_a, channel_b} = 2'b10; #10; 
            {channel_a, channel_b} = 2'b11; #10;
            {channel_a, channel_b} = 2'b01; #10; 
            {channel_a, channel_b} = 2'b00; #10; 
        end

        #10;
        $finish;
    end
endmodule