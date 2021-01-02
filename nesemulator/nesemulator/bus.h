#pragma once
#include <cstdint>
#include "cpu.h"
#include "ram.h"

class bus
{
public:
	cpu cCPU; // Connected CPU
	ram cRAM;

public:
	bus();
};

