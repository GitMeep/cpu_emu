#include <iostream>
#include <iomanip>

#include "cpu.h"
#include "control.h"

uint16_t CPU::controlWord;
uint8_t CPU::registers[9];
uint8_t CPU::memory[256];

/*
void CPU::init() {

}*/

struct ControlSignals {
  bool AOUT;
  bool AIN;
  bool BIN;
  bool MOUT;
  bool MIN;
  bool ARIN;
  bool REOUT;
  bool ALU_PM;
  bool PCOUT;
  bool PCEN;
  bool PCR;     // not used
  bool IRIN;
  bool CDOUT;
  bool OUTIN;
};

#define getFlag(val,pos) ((val >> pos) & 0b1) == 0b1
ControlSignals parseControlWord(uint16_t controlWord) {
  return {
    getFlag(controlWord, 0),
    getFlag(controlWord, 1),
    getFlag(controlWord, 2),
    getFlag(controlWord, 3),
    getFlag(controlWord, 4),
    getFlag(controlWord, 5),
    getFlag(controlWord, 6),
    getFlag(controlWord, 7),
    getFlag(controlWord, 8),
    getFlag(controlWord, 9),
    getFlag(controlWord, 10),
    getFlag(controlWord, 11),
    getFlag(controlWord, 12),
    getFlag(controlWord, 13)
  };
}

bool clockEdge = true; // true: A, false: B
void CPU::halfClock() {
  if(clockEdge) { // move data on A edge
    ControlSignals signals = parseControlWord(controlWord);

    uint8_t busValue;
    // "read" to the bus
    if(signals.AOUT)  busValue                = registers[R_A];
    if(signals.MOUT)  busValue                = memory[registers[R_AR]];
    if(signals.REOUT) busValue                = signals.ALU_PM ? (registers[R_A] + registers[R_B]) : (registers[R_A] - registers[R_B]); // ALU_PM (true: high signal: add) (false: low signal: subtract)
    if(signals.PCOUT) busValue                = registers[R_PC];
    if(signals.CDOUT) busValue                = registers[R_CD];

    // write from the bus to the relevant place
    if(signals.AIN)   registers[R_A]          = busValue;
    if(signals.BIN)   registers[R_B]          = busValue;
    if(signals.MIN)   memory[registers[R_AR]] = busValue;
    if(signals.ARIN)  registers[R_AR]         = busValue;
    if(signals.IRIN)  registers[R_IR]         = busValue;
    if(signals.OUTIN) {registers[R_OUT]       = busValue; std::cout << (int)registers[R_OUT] << std::endl; }

    if(clockEdge && signals.PCEN) registers[R_PC]++;

    if(signals.PCR) CPU::resetPC();

    // Control signales are updated after data has been moved around on the A edge. In the real world
    // control logic and components would receive the A edge at the same time, hopefully latching data
    // before signals are changed. In the world of code, thing's happen sequentially, so we update control
    // signals afterwards.
    onClockEdge(clockEdge);
  } else {
    onClockEdge(clockEdge); // B edge
  }
  clockEdge = !clockEdge;
}

void CPU::resetPC() {
  registers[R_PC] = 0; // maybe make it possible to reset to something else?
}

void CPU::setCtrlWord(uint16_t ctrlWord) {
  controlWord = ctrlWord;
}

void CPU::setCtrlData(uint8_t data) {
  registers[R_CD] = data;
}

uint8_t CPU::getInstruction() {
  return registers[R_IR];
}

void CPU::printRam() {
  for (int addr = 0; addr < MEM_SIZE; addr++) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)memory[addr] << ' ';
        if(addr % 16 == 15) std::cout << std::endl;
    }
    
    std::cout << std::endl;
}

uint8_t CPU::getValAtAddr(uint8_t addr) {
  return memory[addr];
}

uint8_t CPU::getRegister(Register reg) {
  return registers[reg];
}
