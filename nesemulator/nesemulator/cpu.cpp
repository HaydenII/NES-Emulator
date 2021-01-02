#include "cpu.h"
#include "bus.h"

inline uint16_t cpu::FullAddrConv(uint8_t inHi, uint8_t inLo)
{
	return ((inHi << 8) & inLo);
}

cpu::cpu(bus* inBus)
{
	cBUS = inBus;
	reset();
}

void cpu::reset()
{
	PC = 0x200;
	SP = 0x0;
	PF = Empty_Flag;
	A = 0;
	X = 0;
	Y = 0;
}

inline void cpu::SetFlag(flag inFlag)
{
	PF += inFlag;
}

void cpu::clock()
{
	// Read the next opcode
	opcode = cBUS->cRAM.read(PC++);

	// Get the addressing mode and obtain the data

}

// Data is in the accumilator. Nothing to do here
void cpu::acc() {}

void cpu::abs() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);	
	full_addr = (hi << 8) | lo;
}

void cpu::absx() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);
	full_addr = (hi << 8) | lo + X;

	// If the increment to the lo bite has ticked the hi byte over
	// set the carry flag
	if ((full_addr && 0xFF00) != (hi << 8)) {
		SetFlag(CF);
	}
}

void cpu::absy() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);
	full_addr = (hi << 8) | lo + X;

	// If the increment to the lo bite has ticked the hi byte over
	// set the carry flag
	if ((full_addr && 0xFF00) != (hi << 8)) {
		SetFlag(CF);
	}
}

void cpu::imm() {
	// Data is in the next byte. PC is already there. Do nothing
}

void cpu::impl() {
	/*
	* The operation involves a register or something else.
	* no data needs to be read from memory. Do nothing
	*/
}

void cpu::ind() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);

	full_addr = (hi << 8) | lo + X;

	// Hardware bug in original system
	if (lo == 0x00FF) {
		full_addr = (cBUS->cRAM.read(full_addr & 0xFF00) << 8) | cBUS->cRAM.read(full_addr);
	}
	else {
		full_addr = (cBUS->cRAM.read(full_addr + 1) << 8) | cBUS->cRAM.read(full_addr);
	}
}

void cpu::xind()
{
	uint16_t ptr = cBUS->cRAM.read(PC++);

	lo = cBUS->cRAM.read(ptr + ((uint16_t)X) & 0x00FF);
	hi = cBUS->cRAM.read((uint16_t)(ptr + (uint16_t)X + 1) & 0x00FF);

	full_addr = (hi << 8) | lo + X;
}

void cpu::yind()
{
	uint16_t ptr = cBUS->cRAM.read(PC++);

	lo = cBUS->cRAM.read(ptr & 0x00FF);
	hi = cBUS->cRAM.read((ptr + 1) & 0x00FF);

	full_addr = ((hi << 8) | lo) + Y;

	// If the increment to the lo bite has ticked the hi byte over
	// set the carry flag
	if ((full_addr && 0xFF00) != (hi << 8)) {
		SetFlag(CF);
	}
}

void cpu::rel()
{
	uint8_t NB = cBUS->cRAM.read(PC++);

	// If the highest bit is a 1 the number is negative
	if (NB & 0x80) {
		full_addr = PC - ((NB & 0x7F) + 1);
	}
	else {
		full_addr = PC + (NB & 0x7F);
	}
}

void cpu::zpg()
{
	uint8_t NB = cBUS->cRAM.read(PC++);

	full_addr = 0x0000 | lo;
}

void cpu::zpgx()
{
	uint8_t NB = cBUS->cRAM.read(X + PC++);

	full_addr = (0x0000 | lo) + X;
}

void cpu::zpgy()
{
	uint8_t NB = cBUS->cRAM.read(Y + PC++);

	full_addr = (0x0000 | lo);
}
