emu:
	gcc -DSDLMODE emu.c -I /usr/local/include -L /usr/local/lib -l SDL2 -o emu

console_debug:
	gcc -DDEBUG emu.c -o console_debug -lcurses
	./console_debug $(ROM)

hardware_debug:
	gcc -DDEBUG -DHARDWARE emu.c hardware/ssd1306_i2c.c -lwiringPi -o hardware_debug
	./hardware_debug $(ROM)

.PHONY: clean test

clean:
	rm -f emu console_debug hardware_debug

test:
	make clean
	make console_debug ROM=test_opcode.ch8
