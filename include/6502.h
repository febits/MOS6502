#ifndef _6502_H
#define _6502_H

#include <inttypes.h>
#include <stdint.h>

#define RAM (1 << 16)

#define RESETVL 0xFFFC
#define RESETVH 0xFFFD
#define STARTL 0x00
#define STARTH 0x80
#define START 0x8000
#define STACKBASE 0x0100
#define VECTORSLEN 6

#define MAXMNEMONIC 4
#define MAXOPCODESTABLE 256
#define ILLEGAL "ILLG"

typedef struct cpu MOS6502;
typedef struct instruction_context MOS6502IContext;

#define CPU (cpu)
#define ZZ (CPU->status.flags.Z)
#define VV (CPU->status.flags.V)
#define NN (CPU->status.flags.N)

typedef uint8_t (*readbusfunc)(MOS6502 *cpu, uint16_t addr);
typedef uint8_t (*writebusfunc)(MOS6502 *cpu, uint16_t addr, uint8_t data);
typedef uint8_t (*executeop)(MOS6502 *cpu, MOS6502IContext *context);

// cpu bus
typedef struct cpubus {
  uint8_t ram[RAM];

  readbusfunc read;
  writebusfunc write;
} MOS6502Bus;

// cpu interface
typedef struct cpu {
  uint8_t X, Y;
  uint8_t A;
  uint8_t SP;
  uint16_t PC;

  union {
    struct {
      uint8_t C : 1;      // 0 - Carry Flag
      uint8_t Z : 1;      // 1 - Zero Flag
      uint8_t I : 1;      // 2 - Interrupt
      uint8_t D : 1;      // 3 - Decimal
      uint8_t B : 1;      // 4 - Break
      uint8_t unused : 1; // 5 - Unused
      uint8_t V : 1;      // 6 - Overflow
      uint8_t N : 1;      // 7 - Negative
    } flags;
    uint8_t ps;
  } status;

  MOS6502Bus bus;
} MOS6502;

typedef enum addressing_modes {
  IMP = 0, // Implied
  ACC,     // Accumulator
  IMM,     // Immediate
  ZP0,     // Zero Page
  ZP0X,    // Zero Page with X
  ZP0Y,    // Zero Page with Y
  RELT,    // Relative
  ABS,     // Absolute
  ABSX,    // Absolute with X
  ABSY,    // Absolute with Y
  IND,     // Indirect
  IDEIND,  // Indexed Indirect X
  INDIDE,  // Indirect Indexed Y
  ILL      // Illegal Opcodes
} MOS6502AddressingModes;

typedef struct instruction_context {
  uint8_t operand_immediate;
  uint8_t zeropage_addr;
  uint8_t relativebranch;
  uint16_t absolute_addr;
} MOS6502IContext;

typedef struct instruction {
  uint8_t opcode;
  char mnemonic[MAXMNEMONIC];

  executeop exec;
  MOS6502AddressingModes mode;
} MOS6502Instruction;

extern struct instruction opcodes[MAXOPCODESTABLE];

MOS6502 *mos6502_init();
void mos6502_uninit(MOS6502 *cpu);
uint8_t mos6502_reset(MOS6502 *cpu);
uint16_t mos6502_loadbytes(MOS6502 *cpu, uint8_t *bytes, uint16_t size);
uint16_t mos6502_execute(MOS6502 *cpu);

#endif
