#include "main.h"
#include "bus.h"
#include <bitset>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>

std::vector<uint8_t> LoadBinaryFile(const char* filename)
{
	// open the file:
	std::basic_ifstream<uint8_t> file(filename, std::ios::binary);

	// read the data:
	return std::vector<uint8_t>((std::istreambuf_iterator<uint8_t>(file)), std::istreambuf_iterator<uint8_t>());
}

std::vector<uint8_t> load_rom_from_path(std::string instr)
{
	using namespace std::filesystem;

	path ReadPath;

	// If argument is not a full path append the string to the current path
	if (instr.find(':') && exists(instr)) {
		ReadPath = path(instr);
	}

	// Load the chip8 binary
	std::vector<uint8_t> program = LoadBinaryFile(ReadPath.string().c_str());

	return program;
}

int main(){
	bus nBUS;
	
	std::vector<uint8_t> program = load_rom_from_path("C:\\Users\\hayde\\Downloads\\6502_functional_test.bin");

	// Write the binary into memory
	uint16_t WritePtr = 0x0000;
	for (auto& instr : program) {
		nBUS.cRAM.write(WritePtr++, instr);
	}

	nBUS.cCPU.PC = 0x400;

	while (true) {
		nBUS.cCPU.clock();
	}

	std::cout << program.size() << " bytes" << std::endl;

	return 1;
}