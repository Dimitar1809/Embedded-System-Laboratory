
module soc_system (
	clk_clk,
	hps_0_h2f_reset_reset_n,
	memory_mem_a,
	memory_mem_ba,
	memory_mem_ck,
	memory_mem_ck_n,
	memory_mem_cke,
	memory_mem_cs_n,
	memory_mem_ras_n,
	memory_mem_cas_n,
	memory_mem_we_n,
	memory_mem_reset_n,
	memory_mem_dq,
	memory_mem_dqs,
	memory_mem_dqs_n,
	memory_mem_odt,
	memory_mem_dm,
	memory_oct_rzqin,
	reset_reset_n,
	quad_comm_0_yaw_channel_b_signal,
	quad_comm_0_yaw_channel_a_signal,
	quad_comm_0_pitch_channel_b_signal,
	quad_comm_0_pitch_channel_a_signal,
	quad_comm_0_slave_address,
	quad_comm_0_slave_read,
	quad_comm_0_slave_readdata,
	quad_comm_0_slave_write,
	quad_comm_0_slave_writedata,
	quad_comm_0_slave_byteenable);	

	input		clk_clk;
	output		hps_0_h2f_reset_reset_n;
	output	[14:0]	memory_mem_a;
	output	[2:0]	memory_mem_ba;
	output		memory_mem_ck;
	output		memory_mem_ck_n;
	output		memory_mem_cke;
	output		memory_mem_cs_n;
	output		memory_mem_ras_n;
	output		memory_mem_cas_n;
	output		memory_mem_we_n;
	output		memory_mem_reset_n;
	inout	[31:0]	memory_mem_dq;
	inout	[3:0]	memory_mem_dqs;
	inout	[3:0]	memory_mem_dqs_n;
	output		memory_mem_odt;
	output	[3:0]	memory_mem_dm;
	input		memory_oct_rzqin;
	input		reset_reset_n;
	input		quad_comm_0_yaw_channel_b_signal;
	input		quad_comm_0_yaw_channel_a_signal;
	input		quad_comm_0_pitch_channel_b_signal;
	input		quad_comm_0_pitch_channel_a_signal;
	input	[7:0]	quad_comm_0_slave_address;
	input		quad_comm_0_slave_read;
	output	[31:0]	quad_comm_0_slave_readdata;
	input		quad_comm_0_slave_write;
	input	[31:0]	quad_comm_0_slave_writedata;
	input	[3:0]	quad_comm_0_slave_byteenable;
endmodule
