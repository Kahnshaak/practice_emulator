#pragma once

enum RegNames {
  R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15,
  PC = 16, SL, SB, SP, FP, HP
};

enum CntrlRegNames {
  OPERATION = 0,
  OPERAND_1,
  OPERAND_2,
  OPERAND_3,
  IMMEDIATE,
};

enum InstrCodes {
  JMP = 1, JMR, BNZ, BGT, BLT, BRZ,
  MOV = 7, MOVI, LDA, STR, LDR, STB, LDB,
  ISTR = 14, ILDR, ISTB, ILDB,
  ADD = 18, ADDI, SUB, SUBI, MUL, MULI, DIV, SDIV, DIVI,
  AND = 27, OR,
  CMP = 29, CMPI,
  TRP = 31,
  ALCI = 32, ALLC, IALLC,
  PSHR = 35, PSHB, POPR, POPB, CALL, RET
};

enum Traps {
  HALT = 0, INT_OUT, INT_IN, CHAR_OUT, CHAR_IN, STRING_OUT, STRING_IN,
  PRINT_REG = 98
};

extern unsigned int reg_file[22];
extern unsigned int cntrl_regs[5];
extern unsigned char* prog_mem;
extern unsigned int mem_cycle_cntr;
extern unsigned int prog_mem_size;
extern bool test_mode;
extern bool memStream;

bool init_mem(unsigned int size);
bool init_registers(unsigned int code_section);
bool fetch();
bool decode();
bool execute();

unsigned char readByte(unsigned int address);
unsigned int readWord(unsigned int address);
void writeByte(unsigned int address, unsigned char byte);
void writeWord(unsigned int address, unsigned int word);

void init_cache(unsigned int cacheType);
