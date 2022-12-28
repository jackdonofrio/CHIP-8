emu:
	gcc emu.c -o emu

.PHONY: clean test

test:
	./emu test_opcode.ch8 debug

clean:
	rm -f emu