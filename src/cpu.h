#pragma once

#include <cstdint>

#define MEM_SIZE 256

class CPU {
public:
  enum Register {
    R_OUT = 0, // Output register
    R_A   = 1, // A register
    R_B   = 2, // B register
    R_RE  = 3, // ALU Result register
    R_AR  = 4, // Address register
    R_MO  = 5, // Memory output register

    // inaccessible registers (to the programmer)
    R_CD  = 6, // Control data out
    R_IR  = 7, // Instruction register
    R_PC  = 8  // Program counter
  };

  //static void init();
  static void halfClock();

  static void resetPC();
  static void setCtrlWord(uint16_t ctrlWord);
  static void setCtrlData(uint8_t ctrlWord);
  static uint8_t getInstruction();

  static void printRam();
  static uint8_t getValAtAddr(uint8_t addr);
  static uint8_t getRegister(Register reg);

protected:
  // control signals
  static uint16_t controlWord;

  // registers
  static uint8_t registers[9];

  // ram
  static uint8_t memory[MEM_SIZE];

};