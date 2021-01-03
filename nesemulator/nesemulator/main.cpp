#include "main.h"
#include "bus.h"
#include <bitset>


int main() {
	bus nBUS;

	int i = 0x0;
	while (i <= 0xFF) {
		std::string str = nBUS.cCPU.allinstructions[i].title;
		std::cout << std::hex << i << " | " ;
		if (str != "") {
			std::cout << str << std::endl;
		}
		else {
			std::cout << "_" << std::endl;
		}
		i++;
	}
	
	return 1;
}