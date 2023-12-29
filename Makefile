emu:
	gcc -DSDLMODE emu.c -I /usr/local/include -L /usr/local/lib -l SDL2 -o emu

console_debug:
	gcc -DDEBUG emu.c -o console_debug -lcurses
	./console_debug $(ROM)

.PHONY: clean test

clean:
	rm -f emu console_debug *.o

test:
	make clean
	make console_debug ROM=roms/test_opcode.ch8
