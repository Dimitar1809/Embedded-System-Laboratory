`timescale 1ns/1ps

module tb_TopEntity;
  // System & SPI signals
  reg        clk;
  reg        SPI_CLK;
  reg        SPI_PICO;
  reg        SPI_CS;
  wire       SPI_POCI;
  wire       led2;
  reg [7:0]  rx_byte;

  // Instantiate DUT with a small TIMER_MAX for fast simulation
  TopEntity #(
    .TIMER_MAX(5)        // 5 cycles @100 MHz => 50 ns half-period
  ) dut (
    .clk      (clk),
    .SPI_CLK  (SPI_CLK),
    .SPI_PICO (SPI_PICO),
    .SPI_CS   (SPI_CS),
    .SPI_POCI (SPI_POCI),
    .led2     (led2)
  );

  // 100 MHz system clock
  initial clk = 0;
  always #5 clk = ~clk;

  initial begin
    // Initialize and settle synchronizers
    SPI_CLK  = 0;
    SPI_PICO = 0;
    SPI_CS   = 1;
    repeat (4) @(posedge clk);

    // 1) Send target_count = 3
    $display("[%0t] Writing target_count = 3...", $time);
    write_spi(8'h03);

    // 2) Wait for 3 full blinks:
    //    3 blinks × 2 half-periods/blink × 5 cycles/half × 10 ns/cycle = 300 ns
    repeat (40) @(posedge clk);

    // 3) Read back blink_count
    $display("[%0t] Reading back blink_count...", $time);
    read_spi(rx_byte);
    $display("[%0t] Read blink_count = %0d", $time, rx_byte);

    // 4) Check result
    if (rx_byte == 8'h03)
      $display("[%0t] >>> PASS", $time);
    else
      $display("[%0t] >>> FAIL", $time);

    #50 $finish;
  end

  //-------------------------------------------------------------------------
  // SPI write task: all signal changes synced to posedge clk, with idle before/after
  task write_spi(input [7:0] data);
    integer i;
    begin
      // Idle CS high for at least 2 cycles
      @(posedge clk);
      SPI_CS  = 1;
      SPI_CLK = 0;
      SPI_PICO = 0;
      repeat (2) @(posedge clk);

      // Assert CS to start
      @(posedge clk);
      SPI_CS = 0;

      // Shift out 8 bits
      for (i = 7; i >= 0; i = i - 1) begin
        // Set MOSI one cycle before clock edge
        @(posedge clk);
        SPI_PICO = data[i];
        // Rising edge of SPI_CLK
        @(posedge clk);
        SPI_CLK = 1;
        // Falling edge of SPI_CLK
        @(posedge clk);
        SPI_CLK = 0;
      end

      // Deassert CS and idle
      @(posedge clk);
      SPI_CS = 1;
      SPI_PICO = 0;
      repeat (2) @(posedge clk);
    end
  endtask

  // SPI read task: same sync, sample after shift
  task read_spi(output reg [7:0] data);
    integer i;
    begin
      // Idle & assert CS
      @(posedge clk);
      SPI_CS = 1;
      repeat (2) @(posedge clk);
      @(posedge clk);
      SPI_CS = 0;

      // Shift in 8 bits
      data = 0;
      for (i = 7; i >= 0; i = i - 1) begin
        // ensure data line isn't driving
        @(posedge clk);
        SPI_PICO = 0;
        // Rising edge
        @(posedge clk);
        SPI_CLK = 1;
        // Falling edge -> shift happens
        @(posedge clk);
        SPI_CLK = 0;
        // Next cycle: sample MISO
        @(posedge clk);
        data[i] = SPI_POCI;
      end

      // Deassert CS and idle
      @(posedge clk);
      SPI_CS = 1;
      repeat (2) @(posedge clk);
    end
  endtask

endmodule