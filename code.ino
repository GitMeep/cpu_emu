#include <Arduino.h> // inkludere arduino funktioner

// definerer de mulige opcodes så de kan sammenlignes med i en switch,compiletime constant
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

#define OPC(O) O << 4 // preproccesor makro

// array vi loader ind i cpu, program/data array.
byte memoryProgram[256] = {
  OPC(LDI) | R_A, 1,   // 0000 << 4  0001, 00000001
  OPC(LDI) | R_B, 1,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_A,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_B,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_A,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_B,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_A,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_B,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_A,
  OPC(ADR) | R_OUT,
  OPC(ADR) | R_B,
  OPC(HALT)
};

// kontrol signaler
const uint16_t AOUT   = 0b0000000000000001; // Address register out
const uint16_t AIN    = 0b0000000000000010; // A register in
const uint16_t BRIN   = 0b0000000000000100; // B register in
const uint16_t MOUT   = 0b0000000000001000; // M register out
const uint16_t MIN    = 0b0000000000010000; // Memory in
const uint16_t ARIN   = 0b0000000000100000; // Address register in
const uint16_t REOUT  = 0b0000000001000000; // Result register (ALU) out
const uint16_t ALUSUB = 0b0000000000000000; // ALU subtract
const uint16_t ALUADD = 0b0000000010000000; // ALU add (subtract og add er samme signal, det afhænger blot af om det er 0 eller 1)
const uint16_t PCOUT  = 0b0000000100000000; // Program counter out
const uint16_t PCEN   = 0b0000001000000000; // Program counter enable
const uint16_t RST    = 0b0000010000000000; // Program counter reset (til hvad der er sat på DIP switch)
const uint16_t IRIN   = 0b0000100000000000; // Instruction register in
const uint16_t CDOUT  = 0b0001000000000000; // Control data register out
const uint16_t OUTIN  = 0b0010000000000000; // Output register in

// multi dimentinal arrays, over programmets operationer
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

// hvor mange mikrokode steps (A og B flanker) tager hver instruktion
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

// definere id for in registere
const uint16_t registersIn[] = {
  0b0010000000000000, // OUT register
  0b0000000000000010, // A register
  0b0000000000000100, // B register
  0b0000000000000000, // ALU output: read only
  0b0000000000100000, // Address register
  0b0000000000000000  // Memory out: read only
};

// definere id for out registere
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

byte programIndex = 0; // bruges til booting, hvor langt vi er nået i loadingen af memory data/program array

volatile bool clockADetected = false; // detector om a clock A har været der
volatile bool clockBDetected = false; // detector om a clock B har været der
bool clockBSeen = false; // bruges til at holde styr på om clockB har været der, sådan at der kan skippes hvis der starters op på en A flanke

const byte csData = 10, csLatch = 11, csClk = 12; // control signal shift register pins
const byte cdData = 13, cdLatch = 14, cdClk = 15;
const byte ir1 = 2, ir2 = 3, ir3 = 4, ir4 = 5, ir5 = 6, ir6 = 7, ir7 = 8, ir8 = 9; // instruktionsregister pins
const byte clockAPin = 2, clockBPin = 3;
const byte counterResetPin = 16;

// tager en 16 bit værdi putter ind i ctrlWord og putter det ind i to skifteregistere
void writeControlWord(uint16_t ctrlWord) {
  digitalWrite(csLatch, LOW);
  shiftOut(csData, csClk, MSBFIRST, ~ctrlWord >> 8); // inverteres fordi kontrolsignaler er aktiv low
  shiftOut(csData, csClk, MSBFIRST, ~ctrlWord);
  digitalWrite(csLatch, HIGH);
}

// tager en 16 bit værdi putter ind i data og putter det ind i to skifteregister
void writeControlData(uint8_t data) {
  digitalWrite(cdLatch, LOW);
  shiftOut(cdData, cdClk, MSBFIRST, data);
  digitalWrite(cdLatch, HIGH);
}

// function der retunere en byte fra instruktions registeret
byte readInstruction(){
  byte instruction = 0;
  instruction |= digitalRead(ir1);
  instruction |= digitalRead(ir2) << 1;
  instruction |= digitalRead(ir3) << 2;
  instruction |= digitalRead(ir4) << 3;
  instruction |= digitalRead(ir5) << 4;
  instruction |= digitalRead(ir6) << 5;
  instruction |= digitalRead(ir7) << 6;
  instruction |= digitalRead(ir8) << 7;
  return instruction;
}

// sætter clockADetected true, køre efter interrupt
void clockAInterrupt() {
  clockADetected = true;
}

// sætter clockADetected true, køre efter interrupt
void clockBInterrupt() {
  clockBDetected = true;
}

// køre ved setup
void setup() {
  Serial.begin(9600); // begynder komunikation mellem computer og arduino, 9600 bits per sec/få data til/fra computer behøver den egentlig ikke 

  pinMode(counterResetPin, OUTPUT);
  pinMode(ir1,INPUT);
  pinMode(ir2,INPUT);
  pinMode(ir3,INPUT);
  pinMode(ir4,INPUT);
  pinMode(ir5,INPUT);
  pinMode(ir6,INPUT);
  pinMode(ir7,INPUT);
  pinMode(ir8,INPUT);
  pinMode(csData, OUTPUT);
  pinMode(csLatch, OUTPUT);
  pinMode(csClk, OUTPUT);
  pinMode(cdData, OUTPUT);
  pinMode(cdLatch, OUTPUT);
  pinMode(cdClk, OUTPUT);
  pinMode(clockAPin, INPUT);
  pinMode(clockBPin, INPUT);

  digitalWrite(counterResetPin, LOW);

  // interrupt ved rising edge på A og B clock
  attachInterrupt(digitalPinToInterrupt(clockAPin), clockAInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(clockBPin), clockBInterrupt, RISING);

}

void boot() {
  if(clockBDetected) {
    switch(clockNumber) {
      case 0:
        writeControlData(programIndex); // shifter adressen ud på skifteregisteret, sender det ud på bussen
        writeControlWord(CDOUT | ARIN); // tager det der er på control data skifteregister og putter det ind i adresse registeren på næste A flanke
        break;
      case 1:
        writeControlData(memoryProgram[programIndex]); // shifter værdien som programIndex peger på, ind i data skifteregisteret
        programIndex++;
        writeControlWord(CDOUT | MIN); // tager det der er på data skifteregisteret ind i memory
        break;
    }
  } else { // clockADetected
    writeControlWord(0);
    clockNumber++;
    if(clockNumber >= 2) clockNumber = 0; // nulstiller clockNumber hvis den bliver inkrementeret for meget

    if(programIndex >= 255) { // vi er nu færdige med at skrive alle bytes til hukmmelsen, nulstil boot tilstandens variabler og reset program counter
      programIndex = 0;
      clockNumber = 0;
      booting = false;
      resetting = true; // sætter gang i reset 
    }
  }
}

void reset() {
  if(clockBDetected) {
    digitalWrite(counterResetPin, HIGH); // preload program counter til 00000000
    delayMicroseconds(1); // ifølge datablad kræves min 220 ns puls, sådan her er vi sikre på at den resetter
    digitalWrite(counterResetPin, LOW);
  } else { // clockADetected
    resetting = false;
    halted = false; // hvis vi gerne ville ud af halted tilstand
    fetching = true; // sætter gang i fetching
  }
}

void fetch() {
  if(clockBDetected) {
    switch(clockNumber) {
      case 0:
        writeControlWord(PCOUT | ARIN); // program counter putter data ud på bussen, og adresse registeren tager imod dataen
        break;
      case 1:
        writeControlWord(MOUT | IRIN | PCEN); // tager instruktionen ud af memory og ind i instruktions registeret, og enabler program counteren
        break;
    }
  } else { // clockADetected
    writeControlWord(0); // resetter skifteregisterne til control logic
    clockNumber++;
    if(clockNumber >= 2) {
      clockNumber = 0;
      fetching = false;
    }
  }
}

void executeInstruction() {
  uint16_t controlWord;
  byte instruction = readInstruction(); // loader instruktionen ind i arduinoen fra intruktion registeret 
  byte operand = instruction & 0b00001111; // bruger en bitmaske til at ekstrahere operanden fra instructions byten 
  byte opcode = (instruction >> 4) & 0b00001111; // bruger bitshift til at flytte de de sidste 4 bits til starten, for at ekstrahere opcoden 
  if(clockBDetected) {
    controlWord = instructionSteps[opcode][2 * clockNumber]; // finder controlWord for den operation som skal køres
    // nogle instrukmtioner skal sætte kontrol signaler, som afhænger af operanden, så de kan ikke hard codes i mikrokoden 
    // forskellige instrukitoner skal sætter forskellige variable kontrol signaler på forskellige mikrokode steps, dette håndtere den yderste switch 
    switch(clockNumber) {
      case 0: // clock number 0
        switch(opcode) { // vi skal ikke altid sætte variable kontrol signaler
          case ADR:
          case DIR:
            controlWord |= registersIn[operand]; // vælger kontrol signalet for det register vi bruger, og bitwise or det med controlWord 
            break;
        }
        break;
      case 1: // clock number 1
        switch(opcode) {
          case LDI:
            controlWord |= registersIn[operand];
            break;
        }
        break;
      case 2: // clock number 2
        switch(opcode) {
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
    if(clockNumber + 1 == numSteps[opcode]) {  // instruktionen er færdig, fetch næste
      clockNumber = 0;
      fetching = true;
      if(opcode == HALT) {
        halted = true;
      }
      return;
    }
   
    controlWord = instructionSteps[opcode][2 * clockNumber + 1]; // henter controlWord fra A flanken  af mikro kode steppet
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
// bliver kaldt for evigt af arduino
void loop() {


  if(clockADetected) {

    // kun køre koden hvis forrige flanke var en B flanke, i tilfælde af fejlsynkronisering 
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