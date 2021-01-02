#pragma once
#include <cstdint>

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
	inline static uint16_t FullAddrConv(uint8_t, uint8_t); // Read little endian Byte
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
	/*
	* Registers
	*/
	const enum flag {
		Empty_Flag = 0,
		CF = 0x1,		// Carry
		ZF = 0x1 << 1,	// Zero
		ID = 0x1 << 2,	// Interrupt Disable
		DM = 0x1 << 3,	// Decimal
		BC = 0x1 << 4,	// Break Commands
		UnusedFlag = 0x1 << 5,
		O = 0x1 << 6,	// Overflow
		N = 0x1 << 7	// Negative
	};
	uint16_t PC; // Program Counter
	uint8_t SP; // Stack Pointer
	uint8_t PF; // Processor Flags
	inline void SetFlag(flag);

	//General Purpose Registers
	uint8_t A;
	uint8_t X;
	uint8_t Y;

public:
	uint8_t opcode;
	void clock();

private:
	/*
	* Addressing modes
	* - These set the program counter to the data we want to read
	* by setting the full_addr variable
	*/
	void acc();
	void abs();
	void absx();
	void absy();
	void imm();
};

