To compile full jiwy system run:

iverilog -o jiwy jiwy.v quad_enc.v pwm_gen.v debouncer.v

For de10 avalon bus is added, compile with:

iverilog -o de10_top de10_top.v jiwy.v quad_enc.v pwm_gen.v debouncer.v   





