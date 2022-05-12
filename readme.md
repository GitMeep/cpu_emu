# A simple CPU emulator

A very simple emulator for a very simple 8 bit CPU.

It's based on a real CPU with an Arduino running the control logic. To be able to debug the Arduino code in the emulator it's code is in control.cpp, and various functions have been adapted to control the virtual CPU instead of setting signals in the real world.

### Usage
```bash
cpuemu [binary]
```
Where `[binary]` is a path to a binary file containing the initial contents of memory, loaded in at boot time.