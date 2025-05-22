module TopEntity (
    input  clk,
    input  SPI_CLK,
    input  SPI_PICO,
    input  SPI_CS,
    output SPI_POCI,
    output led2,
    output PITCH_DIRA,
    output PITCH_DIRB,
    output PITCH_PWM_VAL,
    input PITCH_ENC_A,
    input PITCH_ENC_B,
    output YAW_DIRA,
    output YAW_DIRB,
    output YAW_PWM_VAL,
    input YAW_ENC_A,
    input YAW_ENC_B
);

  wire [15:0] yaw_enc_counter;
  wire [13:0] yaw_duty_cycle;
  wire [1:0] yaw_direction;
  wire [15:0] pitch_enc_counter;
  wire [13:0] pitch_duty_cycle;
  wire [1:0] pitch_direction;

  jiwy #(
        .COUNTER_WIDTH(16),
        .DUTY_CYCLE_WIDTH(14)
  ) jiwy_inst (
        .clk(clk),
        .reset(1'b0),
        .yaw_enc_a(YAW_ENC_A),
        .yaw_enc_b(YAW_ENC_B),
        .yaw_enc_counter(yaw_enc_counter),
        .yaw_duty_cycle(yaw_duty_cycle),
        .yaw_direction(yaw_direction),
        .yaw_pwm_val(YAW_PWM_VAL),
        .yaw_pwm_dira(YAW_DIRA),
        .yaw_pwm_dirb(YAW_DIRB),
        .pitch_enc_a(PITCH_ENC_A),
        .pitch_enc_b(PITCH_ENC_B),
        .pitch_enc_counter(pitch_enc_counter),
        .pitch_duty_cycle(pitch_duty_cycle),
        .pitch_direction(pitch_direction),
        .pitch_pwm_val(PITCH_PWM_VAL),
        .pitch_pwm_dira(PITCH_DIRA),
        .pitch_pwm_dirb(PITCH_DIRB)
    );


  reg [2:0] SPI_CLKr;
  always @(posedge clk) SPI_CLKr <= {SPI_CLKr[1:0], SPI_CLK};
  wire SPI_CLK_risingedge = (SPI_CLKr[2:1] == 2'b01);
  wire SPI_CLK_fallingedge = (SPI_CLKr[2:1] == 2'b10);

  reg [2:0] SPI_CSr;
  always @(posedge clk) SPI_CSr <= {SPI_CSr[1:0], SPI_CS};
  wire SPI_CS_active = ~SPI_CSr[1];
  wire SPI_CS_startmessage = (SPI_CSr[2:1] == 2'b10);
  wire SPI_CS_endmessage = (SPI_CSr[2:1] == 2'b01);

  reg [1:0] SPI_PICOr;
  always @(posedge clk) SPI_PICOr <= {SPI_PICOr[0], SPI_PICO};
  wire SPI_PICO_data = SPI_PICOr[1];

  reg [4:0] bitcnt;
  reg byte_received;
  reg [31:0] byte_data_received;

  // receiving part, if CS not active set bitcnt to 0, at rising edge add one to bitcount
  always @(posedge clk) begin
    if (~SPI_CS_active) bitcnt <= 5'b00000;
    else if (SPI_CLK_risingedge) begin
      bitcnt <= bitcnt + 5'b00001;
      byte_data_received <= {byte_data_received[30:0], SPI_PICO_data}; //shifts the bits
    end
  end

  // if 32 bits are received, set byte received
  always @(posedge clk) byte_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 5'b11111);

  reg [31:0] last_received;
  always @(posedge clk) if (byte_received) begin
    pitch_direction <= byte_data_received[1:0];
    pitch_duty_cycle <= byte_data_received[15:2];
    yaw_direction <= byte_data_received[17:16];
    yaw_duty_cycle <= byte_data_received[31:18];
  end

  reg [31:0] byte_data_sent;

  always @(posedge clk)
    if (SPI_CS_active) begin
      if (SPI_CS_startmessage) byte_data_sent <= {pitch_enc_counter, yaw_enc_counter};
      else if (SPI_CLK_fallingedge) begin
        byte_data_sent <= {byte_data_sent[30:0], 1'b0};
      end
    end

  assign SPI_POCI = byte_data_sent[31];

endmodule
