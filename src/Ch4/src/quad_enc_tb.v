`timescale 1ns/1ps

module quad_enc_tb;
    reg clk;
    reg reset;
    reg channel_a;
    reg channel_b;
    wire signed [15:0] counter;
    integer i;

    // Instantiate UUT
    quad_enc uut (
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

        $display("Starting forward rotation tests");
        for (i = 0; i < 5; i = i + 1) begin
            $display("Forward loop %0d", i);
            {channel_a, channel_b} = 2'b00; #10 $display("AB=00, counter=%0d", counter);
            {channel_a, channel_b} = 2'b01; #10 $display("AB=01, counter=%0d", counter);
            {channel_a, channel_b} = 2'b11; #10 $display("AB=11, counter=%0d", counter);
            {channel_a, channel_b} = 2'b10; #10 $display("AB=10, counter=%0d", counter);
            {channel_a, channel_b} = 2'b00; #10 $display("AB=00, counter=%0d", counter);
        end

        $display("Starting reverse rotation tests");
        for (i = 0; i < 5; i = i + 1) begin
            $display("Reverse loop %0d", i);
            {channel_a, channel_b} = 2'b00; #10 $display("AB=00, counter=%0d", counter);
            {channel_a, channel_b} = 2'b10; #10 $display("AB=10, counter=%0d", counter);
            {channel_a, channel_b} = 2'b11; #10 $display("AB=11, counter=%0d", counter);
            {channel_a, channel_b} = 2'b01; #10 $display("AB=01, counter=%0d", counter);
            {channel_a, channel_b} = 2'b00; #10 $display("AB=00, counter=%0d", counter);
        end

        #10;
        $finish;
    end
endmodule