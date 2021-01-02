#include "ram.h"
#pragma warning( disable : 6385 )

uint8_t ram::read(uint16_t inAddr)
{
	if (inAddr <= MAXRAMSIZE) {
		return memory[inAddr];
	}
}

void ram::write(uint16_t inAddr, uint8_t inData)
{
	if (inAddr <= MAXRAMSIZE) {
		memory[inAddr] = inData;
	}
}
