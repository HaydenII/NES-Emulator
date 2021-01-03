#pragma once
#include <cstdint>
#include <string>

class bus; // Forward declared to avoid circular dependency

/*
* Addressing modes
* ---
* Accumulator			--	Data is in the accumilator, not memory
* Absolute				--	Next two bytes define the full address
* Absolute, X			--	The full address is the byte in the x register added to the next two bytes in memory
* Absolute, Y			--	The full address is the byte in the y register added to the next two bytes in memory
* Immediate				--	The address is in the following byte in the 16 bit instruction
* Implied				--	implied in the instruction
* indirect				--	Effectively a pointer. Desired data is at address given in the next two bytes. The Following 2 bytes is not the desired data, just an address to the data.
* X-indexed, indirect	--	(Next byte + X), (Next byte + X + 1) increment without carry
* indirect, Y-indexed	--	((Next byte), (Next byte + 1)) + Y increment with carry
* relative				--	Program counter + Next Byte
* zeropage				--	Address is on the zeropage. Hi-byte is 0, the next byte defines location on zeropage
* zeropage, X-indexed	--	Address is on the zeropage. Hi-byte is 0, the next byte + X defines location on zeropage. No carry.
* zeropage, Y-indexed	--	Address is on the zeropage. Hi-byte is 0, the next byte + Y defines location on zeropage. No carry.
*/
/*
* CPU Clock Process:
* 1. Read first byte opcode
* 2. Figure out addressing mode from opcode
* 3. Read data according to addressing mode
* 4. Compute data
*/
class cpu
{
private:
	/*
	* Useful variables and functions for reading little endian values
	*/
	uint16_t hi;
	uint16_t lo;
	uint16_t full_addr;

public:
	/*
	* Constructor
	*/
	cpu(bus*);
	void reset();

public:
	/*
	* Connection to and interaction with Bus
	*/
	bus* cBUS;

public:
	struct mem_ptr {
		uint16_t range_start;
		uint16_t range_end;
		uint16_t ptr = range_end;

		bool postfixedplus = false;
		bool postfixedminus = false;
	public:
		uint16_t get_ptr() {
			if (postfixedminus) { postfixedminus = false; --(*this); }
			if (postfixedplus) { postfixedplus = false; ++(*this); }
			return ptr;
		}

		// Prefix
		uint16_t operator ++() {
			if (postfixedplus) { postfixedplus = false; ++(*this); }
			if (ptr == range_end) {
				ptr = range_start;
			}
			else {
				ptr++;
			}
			return (ptr);
		}

		// Postfix
		uint16_t operator ++(int) {
			if (postfixedplus) { postfixedplus = false; ++(*this); }
			postfixedplus = true;
			return (ptr);
		}

		// Prefix
		uint16_t operator --() {
			if (postfixedminus) { postfixedminus = false; --(*this); }
			if (ptr == range_start) {
				ptr = range_end;
			}
			else {
				ptr--;
			}
			return (ptr);
		}

		// Postfix
		uint16_t operator --(int) {
			if (postfixedminus) { postfixedminus = false; --(*this); }
			postfixedminus = true;
			return (ptr);
		}
	};

	/*
	* Registers
	*/
	const enum flag {
		Empty_Flag = 0,
		flag_CF = 0x1,		// Carry
		flag_ZF = 0x1 << 1,	// Zero
		flag_ID = 0x1 << 2,	// Interrupt Disable
		flag_DM = 0x1 << 3,	// Decimal
		flag_BC = 0x1 << 4,	// Break Commands
		UnusedFlag = 0x1 << 5,
		flag_O = 0x1 << 6,	// Overflow
		flag_N = 0x1 << 7	// Negative
	};
	uint16_t PC; // Program Counter
	uint8_t SP; // Stack Pointer

	mem_ptr new_SP{0x100, 0x1FF};

	uint8_t PF; // Processor Flags
	inline void set_flag(flag, bool);
	inline bool get_status(flag);
	inline void AddToStack(uint8_t);
	inline uint8_t RemoveFromStack();

	//General Purpose Registers
	uint8_t A;
	uint8_t X;
	uint8_t Y;

public:
	uint8_t opcode;
	void clock();

public:
	uint8_t data;
	struct instruction {
		std::string title = "";			// Title of Pperation
		void (cpu::* operation)() = nullptr;	// Pointer to operation method
		void (cpu::* addr_mode)() = nullptr;	// Pointer to address mode method
		uint8_t bytes = 0;				// Instruction bytes
		uint8_t MC = 0;					// Machine cycles
	};
	/*
	* the instruction struct holds all the relevant data for each instruction
	* including pointers to the address mode and op methods
	*/
	// This is where all the instrucitons are stored
	instruction allinstructions[0xFF]
	{
		{"BRK", &cpu::BRK, &cpu::impl, 1, 7 },	// 0
		{"ORA", &cpu::ORA, &cpu::xind, 2, 6 },
		{}, {}, {},
		{"ORA", &cpu::ORA, &cpu::zpg, 2, 3 },
		{"ASL", &cpu::ASL, &cpu::zpg, 2, 5 },
		{},
		{"PHP", &cpu::PHP, &cpu::impl, 1, 3 },
		{"ORA", &cpu::ORA, &cpu::imm, 2, 2 },
		{"ASL", &cpu::ASL, &cpu::acc, 1, 2 },
		{}, {},
		{"ORA", &cpu::ORA, &cpu::abs, 3, 4 },
		{"ASL", &cpu::ASL, &cpu::abs, 3, 6 },
		{},

		{"BPL", &cpu::BPL, &cpu::rel, 2, 2 },	// 1
		{"ORA", &cpu::ORA, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"ORA", &cpu::ORA, &cpu::zpgx, 2, 4 },
		{"ASL", &cpu::ASL, &cpu::zpgx, 2, 6 },
		{},
		{"CLC", &cpu::CLC, &cpu::impl, 1, 2 },
		{"ORA", &cpu::ORA, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"ORA", &cpu::ORA, &cpu::absx, 3, 4 },
		{"ASL", &cpu::ASL, &cpu::absx, 3, 7 },
		{},

		{"JSR", &cpu::JSR, &cpu::abs, 3, 6 },	// 2
		{"AND", &cpu::AND, &cpu::xind, 2, 6 },
		{}, {},
		{"BIT", &cpu::BIT, &cpu::zpg, 2, 3 },
		{"AND", &cpu::AND, &cpu::zpg, 2, 3 },
		{"ROL", &cpu::ROL, &cpu::zpg, 2, 5 },
		{},
		{"PLP", &cpu::PLP, &cpu::impl, 1, 4 },
		{"AND", &cpu::AND, &cpu::imm, 2, 2 },
		{"ROL", &cpu::ROL, &cpu::acc, 1, 2 },
		{},
		{"BIT", &cpu::BIT, &cpu::abs, 3, 4 },
		{"AND", &cpu::AND, &cpu::abs, 3, 4 },
		{"ROL", &cpu::ROL, &cpu::abs, 3, 6 },
		{},

		{"BMI", &cpu::BMI, &cpu::rel, 2, 2 },	// 3
		{"AND", &cpu::AND, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"AND", &cpu::AND, &cpu::zpgx, 2, 4 },
		{"ROL", &cpu::ROL, &cpu::zpgx, 2, 6 },
		{},
		{"SEC", &cpu::SEC, &cpu::impl, 1, 2 },
		{"AND", &cpu::AND, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"AND", &cpu::AND, &cpu::absx, 3, 4 },
		{"ROL", &cpu::ROL, &cpu::absx, 3, 7 },
		{},

		{"RTI", &cpu::RTI, &cpu::impl, 1, 6 },	// 4
		{"EOR", &cpu::EOR, &cpu::xind, 2, 6 },
		{}, {}, {},
		{"EOR", &cpu::EOR, &cpu::zpg, 2, 3 },
		{"LSR", &cpu::LSR, &cpu::zpg, 2, 5 },
		{},
		{"PHA", &cpu::PHA, &cpu::impl, 1, 3 },
		{"EOR", &cpu::EOR, &cpu::imm, 2, 2 },
		{"LSR", &cpu::LSR, &cpu::acc, 1, 2 },
		{},
		{"JMP", &cpu::JMP, &cpu::abs, 3, 3 },
		{"EOR", &cpu::EOR, &cpu::abs, 3, 4 },
		{"LSR", &cpu::LSR, &cpu::abs, 3, 6 },
		{},

		{"BVC", &cpu::BVC, &cpu::rel, 2, 2 },	// 5
		{"EOR", &cpu::EOR, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"EOR", &cpu::EOR, &cpu::zpgx, 2, 4 },
		{"LSR", &cpu::LSR, &cpu::zpgx, 2, 6 },
		{},
		{"CLI", &cpu::CLI, &cpu::impl, 1, 2 },
		{"EOR", &cpu::EOR, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"EOR", &cpu::EOR, &cpu::absx, 3, 4 },
		{"LSR", &cpu::LSR, &cpu::absx, 3, 7 },
		{},

		{"RTS", &cpu::RTS, &cpu::impl, 1, 6 },	// 6
		{"ADC", &cpu::ADC, &cpu::xind, 2, 6 },
		{}, {}, {},
		{"ADC", &cpu::ADC, &cpu::zpg, 2, 3 },
		{"ROR", &cpu::ROR, &cpu::zpg, 2, 5 },
		{},
		{"PLA", &cpu::PLA, &cpu::impl, 1, 4 },
		{"ADC", &cpu::ADC, &cpu::imm, 2, 2 },
		{"ROR", &cpu::ROR, &cpu::acc, 1, 2 },
		{},
		{"JMP", &cpu::JMP, &cpu::ind, 3, 5 },
		{"ADC", &cpu::ADC, &cpu::abs, 3, 4 },
		{"ROR", &cpu::ROR, &cpu::abs, 3, 6 },
		{},

		{"BVS", &cpu::BVS, &cpu::rel, 2, 2 },	// 7
		{"ADC", &cpu::ADC, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"ADC", &cpu::ADC, &cpu::zpgx, 2, 4 },
		{"ROR", &cpu::ROR, &cpu::zpgx, 2, 6 },
		{},
		{"SEI", &cpu::SEI, &cpu::impl, 1, 2 },
		{"ADC", &cpu::ADC, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"ADC", &cpu::ADC, &cpu::absx, 3, 4 },
		{"ROR", &cpu::ROR, &cpu::absx, 3, 7 },
		{},

		{},										// 8
		{"STA", &cpu::STA, &cpu::xind, 2, 6 },
		{}, {},
		{"STY", &cpu::STY, &cpu::zpg, 2, 3 },
		{"STA", &cpu::STA, &cpu::zpg, 2, 3 },
		{"STX", &cpu::STX, &cpu::zpg, 2, 3 },
		{},
		{"DEY", &cpu::DEY, &cpu::impl, 1, 2 },
		{},
		{"TXA", &cpu::TXA, &cpu::impl, 1, 2 },
		{},
		{"STY", &cpu::STY, &cpu::abs, 3, 4 },
		{"STA", &cpu::STA, &cpu::abs, 3, 4 },
		{"STX", &cpu::STX, &cpu::abs, 3, 4 },
		{},

		{"BCC", &cpu::BCC, &cpu::rel, 2, 2 },	// 9
		{"STA", &cpu::STA, &cpu::yind, 2, 6 },
		{}, {},
		{"STY", &cpu::STY, &cpu::zpgx, 2, 4 },
		{"STA", &cpu::STA, &cpu::zpgx, 2, 4 },
		{"STX", &cpu::STX, &cpu::zpgy, 2, 4 },
		{},
		{"TYA", &cpu::TYA, &cpu::impl, 1, 2 },
		{"STA", &cpu::STA, &cpu::absy, 3, 5 },
		{"TXS", &cpu::TXS, &cpu::impl, 1, 2 },
		{}, {},
		{"STA", &cpu::STA, &cpu::absx, 3, 5 },
		{}, {},

		{"LDY", &cpu::LDY, &cpu::imm, 2, 2 },	// A
		{"LDA", &cpu::LDA, &cpu::xind, 2, 6 },
		{"LDX", &cpu::LDX, &cpu::imm, 2, 2 },
		{},
		{"LDY", &cpu::LDY, &cpu::zpg, 2, 3 },
		{"LDA", &cpu::LDA, &cpu::zpg, 2, 3 },
		{"LDX", &cpu::LDX, &cpu::zpg, 2, 3 },
		{},
		{"TAY", &cpu::TAY, &cpu::impl, 1, 2 },
		{"LDA", &cpu::LDA, &cpu::imm, 2, 2 },
		{"TAX", &cpu::TAX, &cpu::impl, 1, 2 },
		{},
		{"LDY", &cpu::LDY, &cpu::abs, 3, 4 },
		{"LDA", &cpu::LDA, &cpu::abs, 3, 4 },
		{"LDX", &cpu::LDX, &cpu::abs, 3, 4 },
		{},

		{"BCS", &cpu::BCS, &cpu::rel, 2, 2 },	// B
		{"LDA", &cpu::LDA, &cpu::yind, 2, 5 },
		{}, {},
		{"LDY", &cpu::LDY, &cpu::zpgx, 2, 4 },
		{"LDA", &cpu::LDA, &cpu::zpgx, 2, 4 },
		{"LDX", &cpu::LDX, &cpu::zpgy, 2, 4 },
		{},
		{"CLV", &cpu::CLV, &cpu::impl, 1, 2 },
		{"LDA", &cpu::LDA, &cpu::absy, 3, 4 },
		{"TSX", &cpu::TSX, &cpu::impl, 1, 2 },
		{},
		{"LDY", &cpu::LDY, &cpu::absx, 3, 4 },
		{"LDA", &cpu::LDA, &cpu::absx, 3, 4 },
		{"LDX", &cpu::LDX, &cpu::absy, 3, 4 },
		{},

		{"CPY", &cpu::CPY, &cpu::imm, 2, 2 },	// C
		{"CMP", &cpu::CMP, &cpu::xind, 2, 6 },
		{}, {},
		{"CPY", &cpu::CPY, &cpu::zpg, 2, 3 },
		{"CMP", &cpu::CMP, &cpu::zpg, 2, 3 },
		{"DEC", &cpu::DEC, &cpu::zpg, 2, 5 },
		{},
		{"INY", &cpu::INY, &cpu::impl, 1, 2 },
		{"CMP", &cpu::CMP, &cpu::imm, 2, 2 },
		{"DEX", &cpu::DEX, &cpu::impl, 1, 2 },
		{},
		{"CPY", &cpu::CPY, &cpu::abs, 3, 4 },
		{"CMP", &cpu::CMP, &cpu::abs, 3, 4 },
		{"DEC", &cpu::DEC, &cpu::abs, 3, 6 },
		{},

		{"BNE", &cpu::BNE, &cpu::rel, 2, 2 },	// D
		{"CMP", &cpu::CMP, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"CMP", &cpu::CMP, &cpu::zpgx, 2, 4 },
		{"DEC", &cpu::DEC, &cpu::zpgx, 2, 6 },
		{},
		{"CLD", &cpu::CLD, &cpu::impl, 1, 2 },
		{"CMP", &cpu::CMP, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"CMP", &cpu::CMP, &cpu::absx, 3, 4 },
		{"DEC", &cpu::DEC, &cpu::absx, 3, 7 },
		{},

		{"CPX", &cpu::CPX, &cpu::imm, 2, 2 },	// E
		{"SBC", &cpu::SBC, &cpu::xind, 2, 6 },
		{}, {},
		{"CPX", &cpu::CPX, &cpu::zpg, 2, 3 },
		{"SBC", &cpu::SBC, &cpu::zpg, 2, 3 },
		{"INC", &cpu::INC, &cpu::zpg, 2, 5 },
		{},
		{"INX", &cpu::INX, &cpu::impl, 1, 2 },
		{"SBC", &cpu::SBC, &cpu::imm, 2, 2 },
		{"NOP", &cpu::NOP, &cpu::impl, 1, 2 },
		{},
		{"CPX", &cpu::CPX, &cpu::abs, 3, 4 },
		{"SBC", &cpu::SBC, &cpu::abs, 3, 4 },
		{"INC", &cpu::INC, &cpu::abs, 3, 6 },
		{},

		{"BEQ", &cpu::BEQ, &cpu::rel, 2, 2 },	// F
		{"SBC", &cpu::SBC, &cpu::yind, 2, 5 },
		{}, {}, {},
		{"SBC", &cpu::SBC, &cpu::zpgx, 2, 4 },
		{"INC", &cpu::INC, &cpu::zpgx, 2, 6 },
		{},
		{"SED", &cpu::SED, &cpu::impl, 1, 2 },
		{"SBC", &cpu::SBC, &cpu::absy, 3, 4 },
		{}, {}, {},
		{"SBC", &cpu::SBC, &cpu::absx, 3, 4 },
		{"INC", &cpu::INC, &cpu::absx, 3, 7 },
	};

	/*
	* Addressing modes
	* - These set the program counter to the data we want to read
	* by setting the full_addr variable
	*/
	void acc();		// Accumilator
	void abs();		// Absolute
	void absx();	// Absolute, X
	void absy();	// Absolute, Y
	void imm();		// Immediate
	void impl();	// Implied
	void ind();		// Indirect
	void xind();	// X, Indirect
	void yind();	// Y, Indirect
	void rel();		// Relative
	void zpg();		// Zeropage
	void zpgx();	// Zeropage X
	void zpgy();	// Zerpage Y

private:
	/*
	* operations
	*/
	void BRK();		// Force Break
	void ORA();		// Or with Accumulator
	void ASL();		// Arithmetic Shift Left
	void PHP();		// Push Processor Status
	void BPL();		// Branch if Positive
	void CLC();		// Clear Carry Flag
	void JSR();		// Jump to Subroutine
	void AND();		// Jump to Subroutine
	void BIT();		// Bit test
	void ROL();		// Rotate Left
	void PLP();		// Pull Processor Status
	void BMI();		// Branch if Minus
	void SEC();		// Set carry flag
	void RTI();		// Return from Interrupt
	void EOR();		// Exclusive or
	void LSR();		// Logical shift right
	void PHA();		// Push Accumulator to stack
	void JMP();		// Jump to address
	void BVC();		// Branch if Overflow Clear
	void CLI();		// Clear Interrupt Disable
	void RTS();		// Return from Subroutine
	void ADC();		// Add with carry
	void ROR();		// Rotate right
	void PLA();		// Pull from stack into accumulator
	void BVS();		// Branch if Overflow set
	void SEI();		// Set interrupt disable
	void STA();		// Store accumulator into memory
	void STY();		// Store Y register into memory
	void STX();		// Store X register into memory
	void DEY();		// Decrement Y register
	void TXA();		// Transfer X to accumulator
	void BCC();		// Branch if carry clear
	void TYA();		// Transfer Accululator to Y
	void TXS();		// Transfer X to stack
	void LDY();		// Load accumulator with Y
	void LDA();		// Load accumulator with A
	void LDX();		// Load accumulator with X
	void TAY();		// Transfer accumulator to Y
	void TAX();		// Transfer accumulator to X
	void BCS();		// Branch if carry flag set
	void CLV();		// Clear overflow flag
	void TSX();		// Transfer stack pointer to X
	void CPY();		// Compare Y register to read in data
	void CMP();		// Compare A register to read in data
	void DEC();		// Decrement from value held at mem address
	void INY();		// Increment Y register
	void DEX();		// Decrement X register
	void BNE();		// Branch if not equal
	void CLD();		// Clear decimal mode
	void CPX();		// Compare X register
	void SBC();		// Subtract with carry
	void INC();		// Increment Memory
	void INX();		// Increment X register
	void NOP();		// No operation. Does nothing.
	void BEQ();		// Branch if equal
	void SED();		// Set decimal flag
};