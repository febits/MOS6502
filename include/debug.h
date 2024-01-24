#ifndef _UTILS_H
#define _UTILS_H

#include "6502.h"

typedef enum {
  RESET = 0,
  WHITE,
  YELLOW,
  RED,
  GRAY,
  GREEN
} Color;

void printfc(Color c, const char *fmt, ...);
size_t getprogramsize(const char *path);
void mos6502_printstatus(MOS6502 *cpu);
void mos6502_printopcodes();
void mos6502_disassemble(MOS6502 *cpu, uint8_t opcode, uint16_t pc);

#endif