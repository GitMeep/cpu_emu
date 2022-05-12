#include <iostream>
#include <string>

#include <chrono>
#include <thread>

#include "cpu.h"
#include "control.h"

#define DELAY(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

int main() {
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