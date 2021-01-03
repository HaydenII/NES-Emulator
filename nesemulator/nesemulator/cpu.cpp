#include "cpu.h"
#include "bus.h"

#include <iostream>

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

inline void cpu::set_flag(flag inFlag, bool inState = true)
{
	if (inState) {
		PF |= inFlag;
	}
	else {
		PF & ~inFlag;
	}
}

inline bool cpu::get_status(flag inFlag)
{
	if ((PF & inFlag) > 0) {
		return true;
	}
	return false;

}

inline void cpu::AddToStack(uint8_t inVal)
{
	cBUS->cRAM.write(++new_SP, inVal);
}

inline uint8_t cpu::RemoveFromStack()
{
	return cBUS->cRAM.read(new_SP--);
}

void cpu::clock()
{
	// Read the next opcode
	opcode = cBUS->cRAM.read(PC++);

	// Call the address mode method 
	(this->*allinstructions[opcode].operation)();

	// Fetch the data with the address set by the address mode
	data = cBUS->cRAM.read(full_addr);
}

/*
* Addressing modes
* - These set the program counter to the data we want to read
* by setting the full_addr variable
*/
void cpu::acc() {
	data = A;
}

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
	set_flag(flag_CF, (full_addr && 0xFF00) != (hi << 8));
}

void cpu::absy() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);
	full_addr = (hi << 8) | lo + X;

	// If the increment to the lo bite has ticked the hi byte over
	// set the carry flag
	set_flag(flag_CF, (full_addr && 0xFF00) != (hi << 8));
}

void cpu::imm() {
	// Data is in the next byte. PC is already there. Do nothing
	full_addr = PC;
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
	set_flag(flag_CF, (full_addr && 0xFF00) != (hi << 8));
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
	uint8_t NB = cBUS->cRAM.read(PC++ + X);

	full_addr = (0x0000 | lo) + X;
}

void cpu::zpgy()
{
	uint8_t NB = cBUS->cRAM.read(PC++ + Y);

	full_addr = (0x0000 | lo);
}

/*
* Instructions
*/
void cpu::BRK() {

}

void cpu::ORA()
{
	A |= data;
	set_flag(flag_ZF, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
}

void cpu::ASL()
{
	// If Highest order bit is 1, set negative flag
	set_flag(flag_CF, (data & 0x80) >> 7);
	// If A == 0 set zero flag
	set_flag(flag_ZF, !data);

	data = data << 1;

	// If 8th bit is set, set negative flag
	set_flag(flag_N, (data & 0x80) >> 7);
}

void cpu::PHP()
{
	AddToStack(PF);
}

void cpu::BPL()
{
	if (!get_status(flag_N)) {
		PC += full_addr;
	}
}

void cpu::CLC()
{
	PF &= ~flag_CF;
}

void cpu::JSR()
{
	AddToStack(PC);
	PC = full_addr;
}

void cpu::AND()
{
	A &= data;
}

void cpu::BIT()
{
	uint8_t result = A & data;
	set_flag(flag_ZF, result == 0);

	set_flag(flag_O, (result >> 6) & 0x01);

	set_flag(flag_N, (result >> 7) & 0x01);
}

void cpu::ROL()
{
	set_flag(flag_CF, (data >> 7) & 0x1);

	set_flag(flag_ZF, A == 0);

	data <<= 1;

	if (get_status(flag_CF)) {
		data |= 0x1;
	}

	set_flag(flag_N, (data >> 7) & 0x01);

}

void cpu::PLP()
{
	SP = data;
}

void cpu::BMI()
{
	if (get_status(flag_N)) {
		PC += full_addr;
	}
}

void cpu::SEC()
{
	set_flag(flag_CF);
}

void cpu::RTI()
{
	PF = RemoveFromStack();
}

void cpu::EOR()
{
	A ^= data;

	set_flag(flag_ZF, A == 0);

	set_flag(flag_N, (A >> 7) & 0x01);

}

void cpu::LSR()
{
	set_flag(flag_CF, data & 0x1);
	data >>= 1;
	set_flag(flag_CF, data > 0);
	set_flag(flag_N, (data >> 7) & 0x1);

}

void cpu::PHA()
{
	AddToStack(A);
}

void cpu::JMP()
{
	PC = full_addr;
}

void cpu::BVC()
{
	if (get_status(flag_O)) {
		PC + full_addr;
	}
}

void cpu::CLI()
{
	set_flag(flag_ID, 0);
}

void cpu::RTS()
{
	PC = RemoveFromStack();
}

void cpu::ADC()
{
	int8_t oldA = A;
	A += data;
	set_flag(flag_CF, (A >> 7) != (oldA >> 7));
}

void cpu::ROR()
{
	set_flag(flag_CF, data & 0x1);

	set_flag(flag_ZF, A == 0);

	data >>= 1;

	set_flag(flag_N, (data >> 7) & 0x1);
}

void cpu::PLA()
{
	A = RemoveFromStack();
	set_flag(flag_ZF, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
}

void cpu::BVS()
{
	if (get_status(flag_O)) {
		PC = full_addr;
	}
}

void cpu::SEI()
{
	set_flag(flag_ID);
}

void cpu::STA()
{
	cBUS->cRAM.write(full_addr, A);
}

void cpu::STY()
{
	cBUS->cRAM.write(full_addr, Y);
}

void cpu::STX()
{
	cBUS->cRAM.write(full_addr, X);
}

void cpu::DEY()
{
	Y--;
	set_flag(flag_ZF, Y == 0);
	set_flag(flag_N, (Y >> 7) & 0x1);
}

void cpu::TXA()
{
	A = X;
	set_flag(flag_ZF, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
}

void cpu::BCC()
{
	if(get_status(flag_CF)) {
		PC += full_addr;
	}
}

void cpu::TYA()
{
	Y = A;
	set_flag(flag_ZF, Y == 0);
	set_flag(flag_N, (Y >> 7) & 0x1);
}

void cpu::TXS()
{
	SP = X;
}

void cpu::LDY()
{
	Y = cBUS->cRAM.read(full_addr);
	set_flag(flag_ZF, Y == 0);
	set_flag(flag_N, (Y >> 7) & 0x1);
}

void cpu::LDA()
{
	A = cBUS->cRAM.read(full_addr);
	set_flag(flag_ZF, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
}

void cpu::LDX()
{
	X = cBUS->cRAM.read(full_addr);
	set_flag(flag_ZF, X == 0);
	set_flag(flag_N, (X >> 7) & 0x1);
}

void cpu::TAY()
{
	Y = A;
	set_flag(flag_ZF, Y == 0);
	set_flag(flag_N, (Y >> 7) & 0x1);
}

void cpu::TAX()
{
	X = A;
	set_flag(flag_ZF, X == 0);
	set_flag(flag_N, (X >> 7) & 0x1);
}

void cpu::BCS()
{
	if (get_status(flag_CF)) {
		PC + full_addr;
	}
}

void cpu::CLV()
{
	set_flag(flag_O, false);
}

void cpu::TSX()
{
	X = SP;
}

void cpu::CPY()
{
	uint8_t comparison = Y-data;

	set_flag(flag_CF, Y >= data);
	set_flag(flag_ZF, Y == data);
	set_flag(flag_N, (comparison >> 7) & 0x1);
}

void cpu::CMP()
{
	uint8_t comparison = A - data;

	set_flag(flag_CF, A >= data);
	set_flag(flag_ZF, A == data);
	set_flag(flag_N, (comparison >> 7) & 0x1);
}

void cpu::DEC()
{
	data--;
	set_flag(flag_ZF, data == 0);
	set_flag(flag_N, (data >> 7 ) & 0x1);
}

void cpu::INY()
{
	Y++;
	set_flag(flag_ZF, Y == 0);
	set_flag(flag_N, (Y >> 7) & 0x1);
}

void cpu::DEX()
{
	X--;
	set_flag(flag_ZF, X == 0);
	set_flag(flag_N, (X >> 7) & 0x1);
}

void cpu::BNE()
{
	if (!get_status(flag_ZF)) {
		PC += full_addr;
	}
}

void cpu::CLD()
{
	set_flag(flag_DM, false);
}

void cpu::CPX()
{
	uint8_t comparison = X - data;

	set_flag(flag_CF, X >= data);
	set_flag(flag_ZF, X == data);
	set_flag(flag_N, (comparison >> 7) & 0x1);
}

void cpu::SBC()
{
	// We can invert the bottom 8 bits with bitwise xor
	uint16_t value = ((uint16_t)data) ^ 0x00FF;

	// Notice this is exactly the same as addition from here!
	uint16_t temp = (uint16_t)A + value + (uint16_t)get_status(flag_CF);
	set_flag(flag_CF, temp & 0xFF00);
	set_flag(flag_ZF, ((temp & 0x00FF) == 0));
	set_flag(flag_O, (temp ^ (uint16_t)A) & (temp ^ value) & 0x0080);
	set_flag(flag_N, temp & 0x0080);
	A = temp & 0x00FF;
}

void cpu::INC()
{
	data++;
	set_flag(flag_ZF, data == 0);
	set_flag(flag_N, (data >> 7) & 0x1);
}

void cpu::INX()
{
	X++;
	set_flag(flag_ZF, X == 0);
	set_flag(flag_N, (X >> 7) & 0x1);
}

void cpu::NOP()
{
}

void cpu::BEQ()
{
	if (get_status(flag_ZF)) {
		PC + full_addr;
	}
}

void cpu::SED()
{
	set_flag(flag_DM);
}
