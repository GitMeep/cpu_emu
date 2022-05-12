#include <cstdint>
#include "arduinoAdapter.h"

// definerer de mulige opcodes så de kan sammenlignes med i en switch
enum Opcode {
  LDI, // 0000
  LDM, // 0001
  ST,  // 0010
  ADR, // 0011
  DIR, // 0100
  ADM, // 0101
  DIM, // 0110
  PCR, // 0111
  HALT // 1000
};

enum Register {
  R_OUT,  // 0000
  R_A,    // 0001
  R_B,    // 0010
  R_RE,   // 0011
  R_AR,   // 0100
  R_MO    // 0101
};

// tested: LDI, LDM, ADR, ADM, DIR, DIM, PCR, HALT, ST

#define OPC(O) O << 4

// loades ind dynamisk i emulatoren
uint8_t memoryProgram[256] = {0};

// kontrol signaler
const uint16_t AOUT   = 0b0000000000000001; // Address register out
const uint16_t AIN    = 0b0000000000000010; // A register in
const uint16_t BRIN   = 0b0000000000000100; // B register in
const uint16_t MOUT   = 0b0000000000001000; // M register out
const uint16_t MIN    = 0b0000000000010000; // Memory in
const uint16_t ARIN   = 0b0000000000100000; // Address register in
const uint16_t REOUT  = 0b0000000001000000; // Result register (ALU) out
const uint16_t ALUSUB = 0b0000000000000000; // ALU subtract
const uint16_t ALUADD = 0b0000000010000000; // ALU add (subtract og add er samme signal, det afhænger blot af om det er 0: add eller 1: sub)
const uint16_t PCOUT  = 0b0000000100000000; // Program counter out
const uint16_t PCEN   = 0b0000001000000000; // Program counter enable
const uint16_t RST    = 0b0000010000000000; // Program counter reset (til hvad der er sat på DIP switch)
const uint16_t IRIN   = 0b0000100000000000; // Instruction register in
const uint16_t CDOUT  = 0b0001000000000000; // Control data register out
const uint16_t OUTIN  = 0b0010000000000000; // Output register in

const uint16_t instructionSteps[][5] = {
  { // 0000: LDI
    PCOUT | ARIN,       // 0B
    0,                  // 0A
    MOUT | PCEN         // 1B
  },

  { // 0001: LDM
    PCOUT | ARIN,       // 0B
    0,                  // 0A
    MOUT | ARIN | PCEN, // 1B
    0,                  // 1A
    MOUT                // 2B
  },

  { // 0010: ST
    PCOUT | ARIN,         // 1B
    0,                    // 1A
    MOUT | ARIN | PCEN,   // 2B
    0,                    // 2A
    MIN                   // 3B
  },

  { // 0011: ADR
    ALUADD | REOUT      // 1B
  },

  { // 0100: DIR
    ALUSUB | REOUT      // 1B
  },

  { // 0101: ADM
    PCOUT | ARIN,         // 1B
    0,                    // 1A
    MOUT | ARIN | PCEN,   // 2B
    0,                    // 2A
    ALUADD | REOUT | MIN  // 3B
  },

  { // 0110: DIM
    PCOUT | ARIN,         // 1B
    0,                    // 1A
    MOUT | ARIN | PCEN,   // 2B
    0,                    // 2A
    ALUSUB | REOUT | MIN  // 3B
  },

  { // 0111: PCR
    RST                   // 1B
  },

  { // 1000: HALT
    0
  }

};

// hvor mange mikrokode steps (A og B flanker) tager hver instruktion?
const uint8_t numSteps[] = {
  2, // LDI
  3, // LDM
  3, // ST
  1, // ADR
  1, // DIR
  3, // ADM
  3, // DIM
  1, // PCR
  1  // HALT
};

const uint16_t registersIn[] = {
  0b0010000000000000, // OUT register
  0b0000000000000010, // A register
  0b0000000000000100, // B register
  0b0000000000000000, // ALU output: read only
  0b0000000000100000, // Address register
  0b0000000000000000  // Memory out: read only
};

const uint16_t registersOut[] = {
  0b0000000000000000, // OUT register: write only
  0b0000000000000001, // A register
  0b0000000000000000, // B register: write only
  0b0000000001000000, // ALU output
  0b0000000000000000, // Address register: write only
  0b0000000000001000  // Memory out
};

// state variabler
byte clockNumber = 0;   // holder styr på hvilken clock-flanke vi er på
bool booting = true;    // er vi i gang med at boote eller køre programmet?
bool fetching = true;   // er vi i gang med at fetche eller eksevere?
bool resetting = true;  // skal programmet nulstilles?
bool halted = false;    // er programmet stoppet

byte programIndex = 0;

volatile bool clockADetected = false;
volatile bool clockBDetected = false;
bool clockBSeen = false; // bruges til at holde styr på om clockB har været der, sådan at der kan skippes hvis der starters op på en A flanke

const byte csData = 10, csLatch = 11, csClk = 12; // control signal shift register pins
const byte cdData = 13, cdLatch = 14, cdClk = 15;
const byte ir1 = 2, ir2 = 3, ir3 = 4, ir4 = 5, ir5 = 6, ir6 = 7, ir7 = 8, ir8 = 9; // instruktionsregister pins
const byte clockAPin = 2, clockBPin = 3;
const byte counterResetPin = 16;

void clockAInterrupt() {
  clockADetected = true;
}

void clockBInterrupt() {
  clockBDetected = true;
}

void boot() {
  if(clockBDetected) {
    switch(clockNumber) {
      case 0:
        writeControlData(programIndex);
        writeControlWord(CDOUT | ARIN);
        break;
      case 1:
        writeControlData(memoryProgram[programIndex]);
        programIndex++;
        writeControlWord(CDOUT | MIN);
        break;
    }
  } else { // clockADetected
    writeControlWord(0);
    clockNumber++;

    if(programIndex >= 255) { // vi er nu færdige med at skrive alle bytes til hukmmelsen, nulstil boot tilstandens variabler og reset program counter
      programIndex = 0;
      clockNumber = 0;
      booting = false;
      resetting = true;
    }
  }
}

void reset() {
  if(clockBDetected) {
    resetProgramCounter();
  } else { // clockADetected
    resetting = false;
    halted = false;
    fetching = true;
  }
}

void fetch() {
  if(clockBDetected) {
    switch(clockNumber) {
      case 0:
        writeControlWord(PCOUT | ARIN);
        break;
      case 1:
        writeControlWord(MOUT | IRIN | PCEN);
        break;
    }
  } else { // clockADetected
    writeControlWord(0);
    clockNumber++;
    if(clockNumber == 2) {
      clockNumber = 0;
      fetching = false;
    }
  }
}

void executeInstruction() {
  uint16_t controlWord;
  byte instruction = readInstruction();
  byte operand = instruction & 0b00001111;
  instruction = (instruction >> 4) & 0b00001111;
  if(clockBDetected) {
    controlWord = instructionSteps[instruction][2 * clockNumber];
    switch(clockNumber) {
      case 0: // clock number 0
        switch(instruction) {
          case ADR:
          case DIR:
            controlWord |= registersIn[operand];
            break;
        }
        break;
      case 1: // clock number 1
        switch(instruction) {
          case LDI:
            controlWord |= registersIn[operand];
            break;
        }
        break;
      case 2: // clock number 2
        switch(instruction) {
          case LDM:
            controlWord |= registersIn[operand];
            break;
          case ST:
            controlWord |= registersOut[operand];
            break;
        }
        break;
    }
    writeControlWord(controlWord);
  } else { // clockADetected
    if(clockNumber + 1 == numSteps[instruction]) {
      // instruktionen er færdig, fetch næste
      clockNumber = 0;
      fetching = true;
      if(instruction == HALT) {
        halted = true;
      }
      return;
    }
   
    controlWord = instructionSteps[instruction][2 * clockNumber + 1];
    writeControlWord(controlWord);

    clockNumber++;
  }
}

void runProgram() {
  if(halted) {
    writeControlWord(0);
    return;
  }
  if(fetching) {
    fetch();
  } else {
    executeInstruction();
  }
}

void onClock() {
  if(booting) {
    // load kode og data
    boot();
  } else if(resetting) {
    // nulstil program counter
    reset();
  } else {
    // kør program
    runProgram();
  }
}

void loop() {
  if(clockADetected) {
    if(!clockBSeen) {
      clockADetected = false;
      return;
    }
    clockBSeen = false;
    onClock();
    clockADetected = false;
  }

  if(clockBDetected) {
    onClock();
    clockBSeen = true;
    clockBDetected = false;
  }
}