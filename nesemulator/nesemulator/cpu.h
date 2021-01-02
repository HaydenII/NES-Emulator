#pragma once
#include <cstdint>

class bus; // Forward declared to avoid circular dependency

/*
* Addressing modes
* ---
* Implicit
* Accumultator
* Immediate
* Zero Page
* Zero page, X
* Zero Page, Y
* Relative
* Absolute
* Absolute, X
* Absolute, Y
* Indirect
* Indexted Indirect
* Indirect Indexed
*/
class cpu
{
public:
	/*
	* Constructor
	*/
	cpu(bus*);

public:
	/*
	* Connection to Bus
	*/
	const bus* cBUS;

public:
	/*
	* Registers
	*/
	const enum flag {
		Empty_Flag = 0,
		CF = 0x1,
		ZF = 0x1 << 1,
		ID = 0x1 << 2,
		DM = 0x1 << 3,
		BC = 0x1 << 4,
		UnusedFlag = 0x1 << 5,
		O = 0x1 << 6,
		N = 0x1 << 7
	};
	uint8_t PC; // Program Counter
	uint8_t SP; // Stack Pointer
	uint8_t PF = Empty_Flag; // Processor Flags
	inline void SetCarryFlag(flag);

};

