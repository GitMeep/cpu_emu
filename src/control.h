#pragma once

#include <cstdint>

void onClockEdge(bool clockEdge);

extern uint8_t memoryProgram[256];
extern bool booting;
extern bool fetching;