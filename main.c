#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"

//Memory Location
const int vram_start = 0xFFFFFF80; // address location
const int vram_size = 8;		   // size in bytes

// Masks
const uint8_t op_code_mask = 0xF0;
const uint8_t bit0_mask = 0x01;
const uint8_t bit1_mask = 0x02;
const uint8_t bit2_mask = 0x04;
const uint8_t bit3_mask = 0x08;

// CPU stuff
uint8_t pc = 0x0;
uint8_t ir = 0x90;
int8_t R[2];
int8_t M[256];
int8_t Run = 1;
int8_t SP = 0xFF;

void instruction_not_implemented()
{
	printf("INSTRUCTION NOT IMPLEMENTED!");
	exit(1);
}

//two lazy to implement correctly
uint8_t two_bit_two_comp(uint8_t byte)
{
	switch (byte)
	{
	case 0x0:
		return 0;
	case 0x01:
		return 1;
	case 0x10:
		return -1;
	case 0x11:
		return -2;
	default:
		return 0;
	}
}

void display_state(int print_mem)
{
	printf("\nPC: %X\nInstruction: %X\nRegister 0: %X\nRegister 1: %X", pc, ir, R[0], R[1]);

	if (print_mem)
	{
		printf("\nMemory:");
		for (int i = 0; i < 256; i++)
		{
			if (i % 16 == 0)
				printf("\n");

			printf("%X ", M[i]);
		}
	}

	printf("\nAny Button To Continue...");
	getchar();
}

void draw_row(uint8_t byte, int row, int pixelScale)
{
	const uint8_t column_masks[8] = {
		0x01,
		0x02,
		0x04,
		0x08,
		0x10,
		0x20,
		0x40,
		0x80};

	for (int i = 0; i < 8; i++)
	{
		uint8_t val = (byte & column_masks[i]) >> i;
		if (val)
		{
			DrawRectangle(i * pixelScale, row * pixelScale, pixelScale, pixelScale, BLACK);
		}
	}
}

void load(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	int8_t ra = bit3 >> 3;
	int8_t rb = bit2 >> 2;
	int8_t c2 = two_bit_two_comp(bit0 | bit1);

	switch (rb)
	{
	case 1:
		R[ra] = M[R[1] + c2];
		break;
	case 0:
		R[ra] = M[c2];
		break;
	}
}

void store(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t rb = bit2 >> 2;
	int8_t c2 = two_bit_two_comp(bit0 | bit1);

	switch (rb)
	{
	case 1:
		M[R[1] + c2] = R[ra];
		break;
	case 0:
		M[c2] = R[ra];
		break;
	}
}

void load_immediate_extended(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t ix = bit0;

	switch (ix)
	{
	case 1:
		R[ra] = M[pc + 1];
		break;
	case 0:
		R[ra] = M[M[pc + 1]];
		break;
	}

	pc++;
}

void store_extended(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	M[M[pc + 1]] = R[ra];
	pc++;
}

void add_subtract(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t rb = bit2 >> 2;
	uint8_t rc = bit1 >> 1;
	uint8_t as = bit0;

	switch (as)
	{
	case 1:
		R[ra] = R[rb] + R[rc];
		break;
	case 0:
		R[ra] = R[rb] - R[rc];
		break;
	}
}

void enable_IRQ(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	// uint8_t ra = bit3 >> 3;
	instruction_not_implemented();
}

void and_or(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t rb = bit2 >> 2;
	uint8_t rc = bit1 >> 1;
	uint8_t ao = bit0;

	switch (ao)
	{
	case 1:
		R[ra] = R[rb] & R[rc];
		break;
	case 0:
		R[ra] = R[rb] | R[rc];
		break;
	}
}

void branch(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t rb = bit2 >> 2;
	uint8_t c = bit0 | bit1;

	switch (c)
	{
	case 0x00:
		pc = R[ra];
		break;
	case 0x01:
		pc = (R[rb] == 0) ? R[ra] : pc;
		break;
	case 0x02:
		pc = (R[rb] != 0) ? R[ra] : pc;
		break;
	case 0x03:
		pc = (R[rb] < 0) ? R[ra] : pc;
		break;
	}
	pc--;
}

void shift_left_right(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	instruction_not_implemented();
}

void stop(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	Run = 0;
}

void return_from_isr(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	instruction_not_implemented();
}

void negate(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	instruction_not_implemented();
}

void increment_decremenet(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	instruction_not_implemented();
}

void call_return_sub(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	uint8_t ra = bit3 >> 3;
	uint8_t cr = bit0;

	switch (cr)
	{
	case 1:
		M[SP] = pc;
		SP--;
		pc = R[ra];
		break;
	case 0:
		pc = M[SP + 1];
		SP++;
		break;
	}
}

void push_pull(uint8_t opcode, uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3)
{
	instruction_not_implemented();
}

int main(int argc, char **argv)
{
	// Setting for if we step through program one instruction at a time
	int step = 0;
	int print_mem = 0;

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-step") == 0)
		{
			step = 1;
		}
		else if (strcmp(argv[i], "-mem") == 0)
		{
			print_mem = 1;
		}
	}

	// Program To Draw Smile
	const int programLength = 15;
	int8_t testProgram[15] = {
		0x21, 0x42, //LDA #$42
		0x30, 0x81, //STA #$81
		0x21, 0x3C, //LDA #$3C
		0x30, 0x85, //STA #$83
		0x21, 0x42, //LDA #$42
		0x30, 0x84, //STA #$84
		0x21, 0x0,	//LDA #$0,
		0x70		//JMP A
	};

	// Load Program into memory
	for (int i = 0; i < programLength; i++)
	{
		if (i > 255)
			break;

		M[i] = testProgram[i];
	}

	if (step)
	{
		display_state(print_mem);
	}

	const int screenWidth = 600;
	const int screenHeight = 600;
	const int pixelScale = 75;

	InitWindow(screenWidth, screenHeight, "RISC Processor Emulator");

	SetTargetFPS(60);

	while (!WindowShouldClose() && Run)
	{
		ir = M[pc];

		uint8_t opcode = ir & op_code_mask;
		uint8_t bit0 = ir & bit0_mask;
		uint8_t bit1 = ir & bit1_mask;
		uint8_t bit2 = ir & bit2_mask;
		uint8_t bit3 = ir & bit3_mask;

		if (step)
		{
			display_state(print_mem);
		}

		switch (opcode)
		{
		case 0x00: // load
			load(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x10: // store
			store(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x20: // load immediate
			load_immediate_extended(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x30: // store extended
			store_extended(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x40: // Add / Substract
			add_subtract(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x50: //enable/disable IRQ
			enable_IRQ(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x60: // And / Or
			and_or(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x70: // Branch
			branch(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x80: //Shift Right / Left
			shift_left_right(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0x90: //No Operation
			break;
		case 0xa0: // Stop
			stop(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0xb0: // return from ISR
			return_from_isr(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0xc0: // Negate
			negate(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0xd0: // Increment / Decrement
			increment_decremenet(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0xe0: // call sub / return from sub
			call_return_sub(opcode, bit0, bit1, bit2, bit3);
			break;
		case 0xf0: // push / pull
			push_pull(opcode, bit0, bit1, bit2, bit3);
			break;
		}

		// if (step)
		// {
		// 	display_state(print_mem);
		// }

		pc++;

		BeginDrawing();

		ClearBackground(RAYWHITE);

		for (uint8_t row = 0x0; row < 8; row++)
		{
			draw_row(M[vram_start + row], row, pixelScale);
		}

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
