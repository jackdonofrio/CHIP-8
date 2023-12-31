


emu: CFLAGS := -DSDLMODE
	 OBJS := opcodes.o state.o emu.o sdl_utils.o
emu: main.c $(OBJS)
	gcc $(CFLAGS) $^ -I /usr/local/include -L /usr/local/lib -l SDL2 -o emu


console_debug: CFLAGS := -DDEBUG
			   OBJS := opcodes.o state.o emu.o
console_debug: main.c $(OBJS)
	gcc $(CFLAGS) $^ -o console_debug -lcurses



.PHONY: clean test

clean:
	rm -f emu console_debug *.o

test:
	make clean
	make console_debug ROM=roms/test_opcode.ch8
