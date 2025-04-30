
module soc_system (
	clk_clk,
	esl_demo_export,
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
	quad_enc_0_counter_count,
	quad_enc_0_channel_a_signal,
	quad_enc_0_channel_b_signal,
	quad_enc_1_counter_count,
	quad_enc_1_channel_a_signal,
	quad_enc_1_channel_b_signal);	

	input		clk_clk;
	output	[7:0]	esl_demo_export;
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
	output	[15:0]	quad_enc_0_counter_count;
	input		quad_enc_0_channel_a_signal;
	input		quad_enc_0_channel_b_signal;
	output	[15:0]	quad_enc_1_counter_count;
	input		quad_enc_1_channel_a_signal;
	input		quad_enc_1_channel_b_signal;
endmodule
