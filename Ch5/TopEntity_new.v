// TopEntity.v
`timescale 1ns/1ps

module TopEntity #(
    parameter integer TIMER_MAX = 50_000_000  // half-second ticks @ 100 MHz
  ) (
    input        clk,
    input        SPI_CLK,
    input        SPI_PICO,   // MOSI
    input        SPI_CS,     // active-low CS
    output       SPI_POCI,   // MISO
    output reg   led2
);

  // 1) Synchronizers & edge detect
  reg [2:0] SPI_CLKr, SPI_CSr;
  reg [1:0] SPI_PICOr;
  always @(posedge clk) SPI_CLKr  <= {SPI_CLKr[1:0], SPI_CLK};
  always @(posedge clk) SPI_CSr   <= {SPI_CSr [1:0], SPI_CS};
  always @(posedge clk) SPI_PICOr <= {SPI_PICOr[0],  SPI_PICO};

  wire SPI_CLK_risingedge  = (SPI_CLKr[2:1] == 2'b01);
  wire SPI_CLK_fallingedge = (SPI_CLKr[2:1] == 2'b10);
  wire SPI_CS_active       = ~SPI_CSr[1];
  wire SPI_CS_startmessage = (SPI_CSr[2:1] == 2'b10);
  wire mosi_bit            = SPI_PICOr[1];

  // 2) Receive shift + strobe
  reg [2:0] bitcnt;
  reg [7:0] byte_data_received;
  reg       byte_received;

  always @(posedge clk) begin
    if (!SPI_CS_active)
      bitcnt <= 0;
    else if (SPI_CLK_risingedge) begin
      bitcnt             <= bitcnt + 1;
      byte_data_received <= {byte_data_received[6:0], mosi_bit};
    end
  end

  always @(posedge clk)
    byte_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 3'b111);

  // 3) Blink FSM + timer
  localparam IDLE  = 2'b00, BLINK = 2'b01, DONE = 2'b10;
  reg [1:0]  state       = IDLE;
  reg [7:0]  target_count, blink_count;
  reg [24:0] timer;

  always @(posedge clk) begin
    case (state)
      IDLE: begin
        led2        <= 0;
        blink_count <= 0;
        timer       <= 0;
        if (byte_received) begin
          target_count <= byte_data_received;
          state        <= BLINK;
        end
      end

      BLINK: begin
        if (timer < TIMER_MAX) begin
          timer <= timer + 1;
        end else begin
          timer <= 0;
          led2  <= ~led2;
          // count on the LED-on edge
          if (~led2) begin
            blink_count <= blink_count + 1;
            if (blink_count + 1 == target_count)
              state <= DONE;
          end
        end
      end

      DONE: begin
        led2 <= 0;
      end
    endcase
  end

  // 4) Transmit logic: load only at CS start, shift on falling edge
  reg [7:0] byte_data_sent;
  always @(posedge clk) begin
    if (SPI_CS_startmessage) begin
      byte_data_sent <= blink_count;
    end
    else if (SPI_CS_active && SPI_CLK_fallingedge) begin
      byte_data_sent <= {byte_data_sent[6:0], 1'b0};
    end
  end

  assign SPI_POCI = byte_data_sent[7];

endmodule
