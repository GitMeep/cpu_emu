#include <iostream>
#include <iomanip>

#include "ram.h"

uint8_t ram[RAM_SIZE];

void print_ram() {
    for (int addr = 0; addr < RAM_SIZE; addr++) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)ram[addr] << ' ';
        if(addr % 16 == 15) std::cout << std::endl;
    }
    
    std::cout << std::endl;
}