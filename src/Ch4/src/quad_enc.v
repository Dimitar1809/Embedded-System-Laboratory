

module quad_enc (
    input wire clk,
    input wire reset,
    input wire channel_a,
    input wire channel_b,
    output reg signed [15:0] counter
);

    reg prev_a;
    reg prev_b;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            prev_a <= 1'b0;
            prev_b <= 1'b0;
            counter <= 16'sd0;
        end else begin
            prev_a <= channel_a;
            prev_b <= channel_b;

            if ({channel_a, channel_b} != {prev_a, prev_b}) begin
                if (prev_a ^ channel_b) begin
                     counter <= counter + 1;
                end else if (prev_b ^ channel_a) begin
                     counter <= counter - 1;
                end
            end
        end
    end

endmodule