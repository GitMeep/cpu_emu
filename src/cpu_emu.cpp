#include <iostream>
#include <fstream>
#include <string>

#include <chrono>
#include <thread>

#include "cpu.h"
#include "control.h"

#define DELAY(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cout << "Usage: cpuemu [binary]" << std::endl;
    return 1;
  }

  std::ifstream memoryFile(argv[1]);
  if(!memoryFile) {
    std::cerr << "Couldn't open " << argv[1] << std::endl;
    return 1;
  }

  memoryFile.read((char *)memoryProgram, 256);

  if(memoryFile.eof()) {
    std::cout << "Warning: memory file not 256 bytes long." << std::endl;
  }

  memoryFile.close();

  while(true) {
    CPU::halfClock();

    std::string a;
    if(!booting) {
      if(fetching) {
        DELAY(10);
        //std::cin.get();
      }
    }
  }
}