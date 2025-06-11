Put this folder on Pi and then to synthesize and put on fpga

yosys -p 'synth_ice40 -top TopEntity -json ice40.json' TopEntity.v jiwy/*
nextpnr-ice40 --hx8k --json ice40.json --pcf ico-jiwy.pcf --asc ice40.asc
icepack ice40.asc ice40.bin

sudo modprobe spi-bcm2835 -r
./icoprog/icoprog -R
./icoprog/icoprog -p < icoprog/ice40.bin
sudo modprobe spi-bcm2835