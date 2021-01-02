#include "cpu.h"
#include "bus.h"

cpu::cpu(bus* inBus)
{
	cBUS = inBus;
}

inline void cpu::SetCarryFlag(flag inFlag)
{
	PF += inFlag;
}
