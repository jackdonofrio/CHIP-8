"""
CHIP-8 Assembler
converts CHIP-8 Asm code -> bytes in .ch8 file
partially adapted from my 8080 assembler

usage: python3 assembler.py [infile] [outfile]
"""

from sys import argv


no_params = {
	'cls' : 0x00E0,
	'ret' : 0x00EE
}

addr_1param = {
	'sys' : 0x0000,
	'jp'  : 0x1000,
	'call': 0x2000,
}

addr_2params = {
	'jp' : 0xB000,
}

x_1param = {
	'skp'  : 0xE09E, # Ex9E
	'sknp' : 0xE0A1, # ExA1
}


x_byte_params = {
	'se'  : 0x3000,
	'sne' : 0x4000,
	'add' : 0x7000,
	'rnd' : 0xC000,
}

x_y_params = {
	'se'   : 0x5000,
	'or'   : 0x8001,
	'and'  : 0x8002,
	'xor'  : 0x8003,
	'add'  : 0x8004,
	'sub'  : 0x8005,
	'shr'  : 0x8006,
	'subn' : 0x8007,
	'shl'  : 0x800e,
	'sne'  : 0x9000
}


def is_hex_s(s):
	try:
		int(s, 16)
		return True
	except ValueError:
		return False

def convert_to_bytecode(tokens, line_number):
	n_params = len(tokens) - 1
	if n_params < 0:
		return 0 # should never happen

	if n_params == 0 and tokens[0] in no_params:
		return no_params[tokens[0]]

	if n_params == 1:
		
		if tokens[0] in addr_1param:
			return addr_1param[tokens[0]] + int(tokens[1], 16)
		
		if tokens[0] in x_1param:
			x = tokens[1][1]
			return x_1param[tokens[0]] + (int(x, 16) << 0x8) 

	if n_params == 2:
		# handle LD separately - 11 cases ;(
		params = [tokens[1][:-1], tokens[2]]
		if tokens[0] == 'ld':
			if params[0] == 'i':
				return 0xA000 + int(params[1], 16)
			if params[0][0] == 'v':
				x = (int(params[0][1], 16)) << 8
				if is_hex_s(params[1]):
					return 0x6000 + x + int(params[1], 16)
				if params[1][0] == 'v':
					y = (int(params[1][1], 16)) << 4
					return 0x8000 + x + y
				if params[1] == 'dt':
					return 0xF007 + x
				if params[1] == 'k':
					return 0xF00A + x
				if params[1] == '[i]':
					return 0xF065 + x
			if params[1][0] == 'v':
				x = (int(params[1][1], 16)) << 8
				if params[0] == 'dt':
					return 0xF015 + x
				if params[0] == 'st':
					return 0xF018 + x
				if params[0] == 'f':
					return 0xF029 + x
				if params[0] == 'b':
					return 0xF033 + x
				if params[0] == '[i]':
					return 0xF055 + x
		if params[0][0] == 'v':
			x = (int(params[0][1], 16)) << 8
			
			if params[1][0] == 'v' and tokens[0] in x_y_params:
				y = (int(params[1][1], 16)) << 4
				return x_y_params[tokens[0]] + x + y
			if is_hex_s(params[1]) and tokens[0] in x_byte_params:
				return x_byte_params[tokens[0]] + x + int(params[1], 16)
		# misc
		if tokens[0] == 'add' and params[0] == 'i':
			x = (int(params[1][1], 16)) << 8 
			return 0xF01E + x
		if tokens[0] == 'jp' and params[0] == 'v0':
			return 0xB000 + int(params[1], 16)
	if n_params == 3 and tokens[0] == 'drw':
		x = (int(tokens[1][1], 16)) << 8
		y = (int(tokens[2][1], 16)) << 4
		n = (int(tokens[3][0], 16))
		return 0xD000 + x + y + n

	return 0






if __name__ == '__main__':
	if len(argv) < 3:
		print("usage: python3 assembler.py [asm in-file] [ch8 out-file]")
		exit()
	infile, outfile = argv[1], argv[2]
	asm_file = open(infile, 'r')
	byte_file = open(outfile, 'wb')
	line_number = 0
	for line in asm_file.readlines():
		line_number += 1
		line = line.strip()
		if line.startswith('#') or len(line) <= 1:
			continue
		if '#' in line:
			line = line[:line.index('#')]
		tokens = line.lower().split(' ')
		bytecode = convert_to_bytecode(tokens, line_number)
		byte_file.write(bytecode.to_bytes(2, 'big'))
