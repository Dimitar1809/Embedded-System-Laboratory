module quad_enc #(
    parameter COUNTER_WIDTH = 16
) (
    input wire clk,
    input wire reset,
    input wire channel_a,
    input wire channel_b,
    output reg unsigned [COUNTER_WIDTH-1:0] counter
);

    reg prev_a;
    reg prev_b;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            prev_a  <= 1'b0;
            prev_b  <= 1'b0;
            counter <= {COUNTER_WIDTH{1'b0}};
        end else begin
            // build a 4‑bit word from last and current AB
            case ({prev_a, prev_b, channel_a, channel_b})
                // forward: 00→01→11→10→00
                4'b0001, 4'b0111, 4'b1110, 4'b1000:
                    counter <= counter + 1;
                // reverse: 00→10→11→01→00
                4'b0010, 4'b0100, 4'b1101, 4'b1011:
                    counter <= counter - 1;
                default: ;
            endcase

            prev_a <= channel_a;
            prev_b <= channel_b;
        end
    end

endmodule