#pragma once
#include <cstdint>

#define MAXRAMSIZE 0xFFFF

class ram
{
private:
	uint8_t memory[MAXRAMSIZE];
public:
	uint8_t read(uint16_t);
	void write(uint16_t, uint8_t);
};

