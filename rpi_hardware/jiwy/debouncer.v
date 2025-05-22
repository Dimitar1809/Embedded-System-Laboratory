module debouncer #(
    parameter STABILITY_CYCLES = 20 // Default, adjust as needed
) (
    input wire clk,
    input wire reset,
    input wire noisy_input,
    output reg debounced_output
);

    reg [7:0] count; // Counter for stability
    reg last_stable_candidate; // Stores the input value that is currently being timed for stability

    initial begin
        // Initialize output to a known state, can be 0 or 1 depending on expectation
        debounced_output = 1'b0;
    end

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            count <= 0;
            last_stable_candidate <= 1'b0; // Initialize to a known state
            debounced_output <= 1'b0;      // Initialize output on reset
        end else begin
            if (noisy_input != last_stable_candidate) begin
                count <= 0; 
                last_stable_candidate <= noisy_input;
            end else begin
                if (count < STABILITY_CYCLES) begin
                    count <= count + 1;
                end else begin
                    if (debounced_output != last_stable_candidate) begin
                        debounced_output <= last_stable_candidate;
                    end
                end
            end
        end
    end
endmodule