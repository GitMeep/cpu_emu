#include <iostream>
#include <string>

#include "cpu.h"
#include "control.h"

int main() {
  while(true) {
    CPU::halfClock();

    std::string a;
    if(!booting) {
      std::cout << (int)CPU::getRegister(CPU::R_OUT) << std::endl;
    }
  }
}