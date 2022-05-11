#include "arduinoAdapter.h"
#include "control.h"
#include "cpu.h"

void writeControlWord(uint16_t ctrlWord) {
  CPU::setCtrlWord(ctrlWord);
}

void writeControlData(uint8_t data) {
  CPU::setCtrlData(data);
}

void resetProgramCounter() {
  CPU::resetPC();
}

byte readInstruction() {
  return CPU::getInstruction();
}

void onClockEdge(bool clockEdge) {
  if(clockEdge) {
    clockAInterrupt();
  } else {
    clockBInterrupt();
  }
  
  loop();
}