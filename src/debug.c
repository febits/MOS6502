#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "6502.h"
#include "debug.h"

#define DEBUG 1
#define DEBUGOPCODES 0

static const char *colors[] = {"\x1b[0m",    "\x1b[1;97m", "\x1b[1;93m",
                               "\x1b[1;31m", "\x1b[2;37m", "\x1B[1;90m"};
static const char *addrmodesstr[] = {
    "Implied",      "Accumulator", "Immediate",  "Zero Page",   "Zero Page, X",
    "Zero Page, Y", "Relative",    "Absolute",   "Absolute, X", "Absolute, Y",
    "Indirect",     "Indirect, X", "Indirect, Y"};

void printfc(Color c, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  printf(colors[c]);
  vprintf(fmt, args);
  printf(colors[RESET]);

  va_end(args);
}

size_t getprogramsize(const char *path) {
  struct stat st;
  stat(path, &st);

  return st.st_size;
}

static void drawline(int n) {
  printfc(RED, "\n<");
  for (int i = 0; i < n; i++)
    printfc(RED, "=");
  printfc(RED, ">\n\n");
}

static void print_memory_range(MOS6502 *cpu, const char *mnick, uint16_t start,
                               uint16_t end, uint8_t chunksize) {
  printfc(WHITE, "\n%s\n\t", mnick);
  for (uint16_t i = start; i < end; i++) {
    uint8_t data = cpu->bus.read(cpu, i);

    if (data != 0x00) {
      printfc(YELLOW, "%02x ", data);
    } else {
      printfc(GRAY, "%02x ", data);
    }

    int index = i + 1;
    if (index && !(index % chunksize)) {
      printf("\n\t");
    }
  }
  printf("\t\n");
}

void mos6502_printstatus(MOS6502 *cpu) {
#if DEBUG

  drawline(100);

  printfc(WHITE,
          "PC: 0x%04X\tA: 0x%02x\tX: 0x%02x\tY: 0x%02x\tSP: 0x%02X\t C: "
          "%02u\tZ:%02u\tI: %02u\tD: %02u\tB: %02u\tV: %02u\tN: %02u\n",
          cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->status.flags.C,
          cpu->status.flags.Z, cpu->status.flags.I, cpu->status.flags.D,
          cpu->status.flags.B, cpu->status.flags.V, cpu->status.flags.N);

  uint8_t chunksize = (1 << 5);
  print_memory_range(cpu, "Zero Page = (0x0000 - 0x00FF)", 0x0000, 0x00FF + 1,
                     chunksize);
  print_memory_range(cpu, "Stack = (0x0100 - 0x01FF)", 0x0100, 0x01FF + 1,
                     chunksize);
  print_memory_range(cpu, "RAM = (0x4e20 - 0x4f20)", 0x4e20, 0x4f20, chunksize);
  print_memory_range(cpu, "RAM = (0x8000 - 0x80FF)", 0x8000, 0x80FF + 1,
                     chunksize);
  // drawline(100);
  printf("\n");

#endif
}

void mos6502_printopcodes() {
#if DEBUG && DEBUGOPCODES

  drawline(100);
  printfc(WHITE, "OPCODES TABLE: \n\n");

  int entries = 0;
  int legalops = 0;
  int illegalops = 0;
  for (int i = 0; i < MAXOPCODESTABLE; i++) {
    if (opcodes[i].exec) {
      entries++;
      printfc(GREEN, "0x%02x ", opcodes[i].opcode);

      if (opcodes[i].mode == ILL) {
        illegalops++;
        int index = i + 1;
        if (index && !(index % 0x10)) {
          printfc(RED, "\t <Illegal>---------------------- %d\n", i + 1);
          continue;
        }
        printfc(RED, "\t <Illegal>\n");
        continue;
      }
      legalops++;

      printfc(WHITE, "\t%s - ", opcodes[i].mnemonic);
      printfc(YELLOW, "%s\n", addrmodesstr[opcodes[i].mode]);
    }
  }

  printfc(WHITE, "\nTotal entries: %d\n", entries);
  printfc(WHITE, "Legal Opcodes: %d\n", legalops);
  printfc(WHITE, "Illegal Opcodes: %d\n", illegalops);

#endif
}

void mos6502_disassemble(MOS6502 *cpu, uint8_t opcode, uint16_t pc) {
  switch (opcodes[opcode].mode) {
    case IMP: { // Implied
      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s\n", opcodes[opcode].mnemonic);
      break;
    }

    case ACC: { // Accumulator
      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s A\n", opcodes[opcode].mnemonic);
      break;
    }

    case IMM: { // Immediate
      uint8_t immediate = cpu->bus.read(cpu, pc + 1);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s #$%02x\n", opcodes[opcode].mnemonic, immediate);
      break;
    }

    case ZP0: { // Zero Page
      uint8_t addr = cpu->bus.read(cpu, pc + 1);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%02x\n", opcodes[opcode].mnemonic, addr);
      break;
    }

    case ZP0X: { // Zero Page with X
      uint8_t addr = cpu->bus.read(cpu, pc + 1);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%02x, X\n", opcodes[opcode].mnemonic, addr);
      break;
    }

    case ZP0Y: { // Zero Page with Y
      uint8_t addr = cpu->bus.read(cpu, pc + 1);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%02x, Y\n", opcodes[opcode].mnemonic, addr);
      break;
    }

    case RELT: { // Relative
      uint8_t relative = cpu->bus.read(cpu, pc + 1);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%02x\n", opcodes[opcode].mnemonic, relative + 2);
      break;
    }

    case ABS: { // Absolute
      uint8_t lo = cpu->bus.read(cpu, pc + 1);
      uint8_t hi = cpu->bus.read(cpu, pc + 2);
      uint16_t addr = (hi << 8) | lo;

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%04x\n", opcodes[opcode].mnemonic, addr);

      break;
    }

    case ABSX: { // Absolute with X
      uint8_t lo = cpu->bus.read(cpu, pc + 1);
      uint8_t hi = cpu->bus.read(cpu, pc + 2);
      uint16_t addr = ((hi << 8) | lo);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%04x, X\n", opcodes[opcode].mnemonic, addr);
      break;
    }

    case ABSY: { // Absolute with Y
      uint8_t lo = cpu->bus.read(cpu, pc + 1);
      uint8_t hi = cpu->bus.read(cpu, pc + 2);
      uint16_t addr = ((hi << 8) | lo);

      printfc(GREEN, "(%02x) ", pc);
      printfc(WHITE, "%s $%04x, Y\n", opcodes[opcode].mnemonic, addr);
      break;
    }

    case IND: { // Indirect
      break;
    }
    case IDEIND: // Indexed Indirect X
      break;
    case INDIDE: // Indirect Indexed Y
      break;
    case ILL:
      break;
  }
}