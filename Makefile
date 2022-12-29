emu:
	gcc emu.c hardware/ssd1306_i2c.c -lwiringPi -o emu

.PHONY: clean console_debug test_hardware

console_debug:
	./emu test_opcode.ch8 debug
test_hardware:
	./emu test_opcode.ch8

clean:
	rm -f emu
