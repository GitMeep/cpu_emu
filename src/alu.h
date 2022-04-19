#include <stdint.h>

struct ALURegisters {
    uint8_t a;
    uint8_t b;
    uint8_t result;
};

extern ALURegisters alu_registers;
