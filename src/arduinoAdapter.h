#pragma once

#include <cstdint>

typedef uint8_t byte;

void writeControlWord(uint16_t ctrlWord);
void writeControlData(uint8_t data);
void resetProgramCounter();

byte readInstruction();

void clockAInterrupt();
void clockBInterrupt();
void loop();