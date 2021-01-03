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
		PF &= ~inFlag;
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
	cBUS->cRAM.write(new_SP--, inVal);
}

inline uint8_t cpu::RemoveFromStack()
{
	return cBUS->cRAM.read(new_SP++);
}

void cpu::clock()
{
	clock_cycles = 0;

	// Read the next opcode
	opcode = cBUS->cRAM.read(PC++);

	// Call the address mode method 
	(this->*allinstructions[opcode].addr_mode)();

	// Run the operation
	(this->*allinstructions[opcode].operation)();

	/*
	* Add the cycles for the instruction.
	* ---
	* Additinoal cycles added by operations and address modes
	* are added inside the op and adrmode methods
	*/
	clock_cycles += this->allinstructions[opcode].MC;

	std::cout << "~ " << this->allinstructions[opcode].title << std::endl;
	std::cout << "~ A " << (int) A << std::endl;
	std::cout << "~ X " << (int) X << std::endl;
	std::cout << "~ Y " << (int) Y << std::endl << std::endl;
}

void cpu::load_to_data()
{
	if (!(allinstructions[opcode].addr_mode == &cpu::impl)) {
		data = cBUS->cRAM.read(full_addr);
	}
	if (allinstructions[opcode].addr_mode == &cpu::acc) {
		data = A;
	}
}

/*
* Addressing modes
* - These set the program counter to the data we want to read
* by setting the full_addr variable
*/
void cpu::acc() {
}

void cpu::abs() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);	
	full_addr = (hi << 8) | lo;
}

void cpu::absx() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);
	full_addr = ((hi << 8) | lo) + X;

	if ((full_addr & 0xFF00) != (hi << 8)) {
		clock_cycles++;
	}
}

void cpu::absy() {
	lo = cBUS->cRAM.read(PC++);
	hi = cBUS->cRAM.read(PC++);
	full_addr = ((hi << 8) | lo) + Y;

	if ((full_addr & 0xFF00) != (hi << 8)) {
		clock_cycles++;
	}
}

void cpu::imm() {
	// Data is in the next byte. PC is already there. Do nothing
	full_addr = PC++;
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

	full_addr = ((hi << 8) | lo);

	// Hardware bug in original system
	if (lo == 0x00FF) {
		/*
		* Simulated hardware bug:
		* If LSB is at page boundry, the least significant bit is is grabbed from where you'd expect
		* But the MSB is taken from 0x--00, where -- are the most significant bytes
		*/
		full_addr = ((uint16_t)cBUS->cRAM.read(full_addr & 0xFF00) << 8) | cBUS->cRAM.read(full_addr);
	}
	// normal behavior
	else {
		full_addr = (cBUS->cRAM.read(full_addr + 1) << 8) | cBUS->cRAM.read(full_addr);
	}
}

void cpu::xind()
{
	uint16_t ptr = cBUS->cRAM.read(PC++);

	lo = cBUS->cRAM.read(ptr + ((uint16_t)X) & 0x00FF);
	hi = cBUS->cRAM.read((uint16_t)(ptr + (uint16_t)X + 1) & 0x00FF);

	full_addr = ((hi << 8) | lo) + X;
}

void cpu::yind()
{
	uint16_t ptr = cBUS->cRAM.read(PC++);

	lo = cBUS->cRAM.read(ptr & 0x00FF);
	hi = cBUS->cRAM.read((ptr + 1) & 0x00FF);

	full_addr = ((hi << 8) | lo) + Y;

	if ((full_addr & 0xFF00) != (hi << 8)) {
		clock_cycles++;
	}
}

void cpu::rel()
{
	rel_addr = cBUS->cRAM.read(PC++);

	// If the highest bit is a 1 the number is negative
	if (rel_addr & 0x80) {
		rel_addr |= 0xFF00;
	}
}

void cpu::zpg()
{
	full_addr = cBUS->cRAM.read(PC++);

	// Clears the high order bits from previous clock cycles
	full_addr &= 0x00FF;
}

void cpu::zpgx()
{
	full_addr = cBUS->cRAM.read(PC++) + X;
	// Clears the high order bits from previous clock cycles
	full_addr &= 0x00FF;
}

void cpu::zpgy()
{
	full_addr = cBUS->cRAM.read(PC++) + Y;
	// Clears the high order bits from previous clock cycles
	full_addr &= 0x00FF;
}

/*
* Instructions
*/
void cpu::BRK() {
	set_flag(flag_I);

	AddToStack((new_PC.get_ptr() >> 8) & 0x00FF);
	AddToStack(new_PC.get_ptr() & 0x00FF);

	set_flag(flag_B);
	AddToStack(PF);
	set_flag(flag_B, false);

	PC = (uint16_t)cBUS->cRAM.read(0xFFFE) | ((uint16_t)cBUS->cRAM.read(0xFFFF) << 8);
}

void cpu::ORA()
{
	load_to_data();

	A |= data;
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
	clock_cycles++;
}

void cpu::ASL()
{
	load_to_data();

	uint16_t temp = data << 1;

	// If Highest order bit is 1, set negative flag
	set_flag(flag_C, (data & 0x80) >> 7);

	// If A == 0 set zero flag
	set_flag(flag_Z, A == 0);

	// If 8th bit is set, set negative flag
	set_flag(flag_N, (data & 0x80) >> 7);

	// If accumulator is the target, set to the accumulator
	if (allinstructions[opcode].addr_mode == &cpu::acc) {
		A = temp;
	}
	// Else it's a var in memory, write to that
	else {
		cBUS->cRAM.write(full_addr, temp);
	}

	data = data << 1;

}

void cpu::PHP()
{
	AddToStack(PF);
}

void cpu::BPL()
{
	if (!get_status(flag_N)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::CLC()
{
	set_flag(flag_C, false);
}

void cpu::JSR()
{
	new_PC--;
	AddToStack((new_PC.get_ptr() >> 8));	// High order bits
	AddToStack(new_PC.get_ptr());			// Low order bits
	new_PC.set_ptr(full_addr);
}

void cpu::AND()
{
	load_to_data();
	A &= data;
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, (A >> 7) & 0x1);
	clock_cycles++;
}

void cpu::BIT()
{
	load_to_data();

	uint8_t result = A & data;

	set_flag(flag_Z, result == 0);

	set_flag(flag_O, (result >> 6) & 0x1);

	set_flag(flag_N, (result >> 7) & 0x1);
}

void cpu::ROL()
{
	load_to_data();

	uint8_t temp = (uint16_t)(data << 1) | get_status(flag_C);
	set_flag(flag_C, temp & 0xFF00);
	set_flag(flag_Z, (temp & 0x00FF) == 0);
	set_flag(flag_N, temp & 0x80);

	if (allinstructions[opcode].addr_mode == &cpu::impl) {
		A = temp;
	}
	else {
		cBUS->cRAM.write(full_addr, temp);
	}

}

void cpu::PLP()
{
	PF = RemoveFromStack();
}

void cpu::BMI()
{
	if (get_status(flag_N)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::SEC()
{
	set_flag(flag_C);
}

void cpu::RTI()
{
	PF = RemoveFromStack();

	lo = RemoveFromStack();
	hi = RemoveFromStack();

	full_addr = (hi << 8) | lo;
	new_PC.set_ptr(full_addr);
}

void cpu::EOR()
{
	load_to_data();

	A ^= data;

	set_flag(flag_Z, A == 0);

	set_flag(flag_N, (A >> 7) & 0x1);

}

void cpu::LSR()
{
	load_to_data();
	uint8_t temp = data >> 1;

	set_flag(flag_C, data & 0x1);
	set_flag(flag_Z, temp > 0);
	set_flag(flag_N, temp & 0x80);


	// If accumulator is the target, set to the accumulator
	if (allinstructions[opcode].addr_mode == &cpu::acc) {
		A = temp;
	}
	// Else it's a var in memory, write to that
	else {
		cBUS->cRAM.write(full_addr, temp);
	}
}

void cpu::PHA()
{
	AddToStack(A);
}

void cpu::JMP()
{
	if (PC == full_addr) {
		std::cout << "LOOPING" << std::endl;
	}
	PC = full_addr;
}

void cpu::BVC()
{
	if (get_status(flag_N)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::CLI()
{
	set_flag(flag_I, 0);
}

void cpu::RTS()
{
	lo = RemoveFromStack();
	hi = RemoveFromStack();

	full_addr = (hi << 8) | lo;
	new_PC.set_ptr(full_addr);
}

void cpu::ADC()
{
	load_to_data();

	uint16_t temp = (uint16_t)A + (uint16_t)data + (uint16_t)get_status(flag_C);

	if (get_status(flag_C)) {
		temp += (uint16_t)0x1;
	}

	set_flag(flag_C, temp > 255);
	set_flag(flag_Z, (temp & 0x00FF) == 0);
	set_flag(flag_O, (~((uint16_t)A ^ (uint16_t)data) & ((uint16_t)A ^ (uint16_t)temp)) & 0x0080);
	set_flag(flag_N, temp & 0x80);

	A = temp & 0x00FF;

	clock_cycles++;
}

void cpu::ROR()
{
	load_to_data();

	uint8_t temp = (uint16_t)(get_status(flag_C) << 7) | (data >> 1);
	set_flag(flag_C, data & 0x1);
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, temp & 0x80);

	if (allinstructions[opcode].addr_mode == &cpu::impl) {
		A = temp;
	}
	else {
		cBUS->cRAM.write(full_addr, temp);
	}
}

void cpu::PLA()
{
	A = RemoveFromStack();
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, A & 0x80);
}

void cpu::BVS()
{
	if (get_status(flag_O)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::SEI()
{
	set_flag(flag_I);
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
	set_flag(flag_Z, Y == 0);
	set_flag(flag_N, Y & 0x80);
}

void cpu::TXA()
{
	A = X;
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, A & 80);
}

void cpu::BCC()
{
	if (!get_status(flag_C)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::TYA()
{
	A = Y;
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, A & 0x80);
}

void cpu::TXS()
{
	new_SP.set_ptr(X);
}

void cpu::LDY()
{
	load_to_data();
	Y = data;
	set_flag(flag_Z, Y == 0);
	set_flag(flag_N, Y & 0x80);
	clock_cycles++;
}

void cpu::LDA()
{
	load_to_data();
	A = data;
	set_flag(flag_Z, A == 0);
	set_flag(flag_N, A & 0x80);
	clock_cycles++;
}

void cpu::LDX()
{
	load_to_data();
	X = data;
	set_flag(flag_Z, X == 0);
	set_flag(flag_N, X & 0x80);
	clock_cycles++;
}

void cpu::TAY()
{
	Y = A;
	set_flag(flag_Z, Y == 0);
	set_flag(flag_N, Y & 0x80);
}

void cpu::TAX()
{
	X = A;
	set_flag(flag_Z, X == 0);
	set_flag(flag_N, X & 0x80);
}

void cpu::BCS()
{
	if (get_status(flag_C)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::CLV()
{
	set_flag(flag_O, false);
}

void cpu::TSX()
{
	X = new_SP.get_ptr();
	set_flag(flag_Z, X == 0);
	set_flag(flag_N, X & 0x80);
}

void cpu::CPY()
{
	load_to_data();

	uint8_t comparison = (uint16_t)Y - (uint16_t)data;

	set_flag(flag_C, Y >= data);
	set_flag(flag_Z, comparison == 0);
	set_flag(flag_N, comparison & 0x80);
}

void cpu::CMP()
{
	load_to_data();

	uint8_t comparison = (uint16_t)A - (uint16_t)data;

	set_flag(flag_C, A >= data);
	set_flag(flag_Z, comparison == data);
	set_flag(flag_N, comparison & 0x80);
	clock_cycles++;
}

void cpu::DEC()
{
	load_to_data();

	data--;

	set_flag(flag_Z, data == 0);
	set_flag(flag_N, data & 0x80);

	cBUS->cRAM.write(full_addr, data);
}

void cpu::INY()
{
	Y++;
	set_flag(flag_Z, Y == 0);
	set_flag(flag_N, Y & 0x80);
}

void cpu::DEX()
{
	X--;
	set_flag(flag_Z, X == 0);
	set_flag(flag_N, X & 0x80);
}

void cpu::BNE()
{
	if (!get_status(flag_Z)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::CLD()
{
	set_flag(flag_D, false);
}

void cpu::CPX()
{
	load_to_data();

	uint8_t comparison = (uint16_t)X - (uint16_t)data;

	set_flag(flag_C, X >= data);
	set_flag(flag_Z, comparison == 0);
	set_flag(flag_N, comparison & 0x80);
}

void cpu::SBC()
{
	load_to_data();

	uint16_t value = ((uint16_t)data) ^ 0x00FF;

	uint16_t temp = (uint16_t)A + value + (uint16_t)get_status(flag_C);
	set_flag(flag_C, temp & 0xFF00);
	set_flag(flag_Z, ((temp & 0x00FF) == 0));
	set_flag(flag_O, (temp ^ (uint16_t)A) & (temp ^ value) & 0x0080);
	set_flag(flag_N, temp & 0x0080);
	A = temp & 0x00FF;

	clock_cycles++;
}

void cpu::INC()
{
	load_to_data();
	cBUS->cRAM.write(full_addr, ++data);
	set_flag(flag_Z, data == 0);
	set_flag(flag_N, data & 0x80);
}

void cpu::INX()
{
	X++;
	set_flag(flag_Z, X == 0);
	set_flag(flag_N, X & 0x80);
}

void cpu::NOP()
{
}

void cpu::BEQ()
{
	if (get_status(flag_Z)) {
		clock_cycles++; // Beacuse the branch succeeded
		if ((PC & 0xFF00) < ((PC += rel_addr) & 0xFF00)) {
			clock_cycles++; // Because the branch went onto a new page 
		}
	}
}

void cpu::SED()
{
	set_flag(flag_D);
}
