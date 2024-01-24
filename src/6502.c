#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502.h"

static uint8_t readbyte(MOS6502 *cpu, uint16_t addr) {
  if (addr >= 0x0000 && addr <= 0xFFFF)
    return cpu->bus.ram[addr];

  return -1;
}

static uint8_t writebyte(MOS6502 *cpu, uint16_t addr, uint8_t data) {
  if (addr >= 0x0000 && addr <= 0xFFFF) {
    cpu->bus.ram[addr] = data;
    return 1;
  }

  return -1;
}

static uint16_t resetvector(MOS6502 *cpu) {
  return ((cpu->bus.read(cpu, RESETVH) << 8) | cpu->bus.read(cpu, RESETVL));
}

uint8_t mos6502_reset(MOS6502 *cpu) {
  cpu->PC = resetvector(cpu);
  cpu->A = cpu->X = cpu->Y = 0;
  cpu->SP = 0xFF;

  return 1;
}

MOS6502 *mos6502_init() {
  MOS6502 *cpu = malloc(sizeof(MOS6502));
  if (!cpu)
    return NULL;

  cpu->A = cpu->X = cpu->Y = 0;
  cpu->SP = 0xFF;
  cpu->status.ps = 0x00;

  cpu->bus.read = readbyte;
  cpu->bus.write = writebyte;
  cpu->bus.write(cpu, RESETVL, STARTL);
  cpu->bus.write(cpu, RESETVH, STARTH);

  cpu->PC = resetvector(cpu);

  return cpu;
}

void mos6502_uninit(MOS6502 *cpu) {
  if (!cpu)
    return;

  free(cpu);
}

uint16_t mos6502_loadbytes(MOS6502 *cpu, uint8_t *bytes, uint16_t size) {
  uint16_t max = (RAM / 2) - VECTORSLEN;

  if (!cpu || !bytes || size > max) {
    return -1;
  }

  size_t amount = 0;
  for (uint16_t i = START; i < START + size; i++) {
    amount += cpu->bus.write(cpu, i, bytes[amount]);
  }

  return amount;
}

// Instructions
static uint8_t setzeroandnegative(MOS6502 *cpu, uint8_t value) {
  cpu->status.flags.Z = value == 0;
  cpu->status.flags.N = (value & 0x80) == 0x80;

  return 1;
}

// Illegal
static uint8_t illg(MOS6502 *cpu, MOS6502IContext *ctx) { return 1; }

// Load/Store ---------------------------------------
static uint8_t lda(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t ldx(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->X = ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->X);
  return 1;
}
static uint8_t ldy(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->Y = ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->Y);
  return 1;
}
static uint8_t sta(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->bus.write(cpu, ctx->absolute_addr, cpu->A);

  return 1;
}
static uint8_t stx(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->bus.write(cpu, ctx->absolute_addr, cpu->X);

  return 1;
}
static uint8_t sty(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->bus.write(cpu, ctx->absolute_addr, cpu->Y);

  return 1;
}

// Registers Transfer -----------------------------
static uint8_t tax(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->X = cpu->A;

  setzeroandnegative(cpu, cpu->X);
  return 1;
}
static uint8_t tay(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->Y = cpu->A;

  setzeroandnegative(cpu, cpu->Y);
  return 1;
}
static uint8_t txa(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->X;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t tya(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->Y;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}

// Stack Operations -----------------------------
static uint8_t tsx(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->X = cpu->SP;

  setzeroandnegative(cpu, cpu->X);
  return 1;
}
static uint8_t txs(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->SP = cpu->X;

  return 1;
}
static uint8_t pha(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->bus.write(cpu, STACKBASE | cpu->SP--, cpu->A);

  return 1;
}
static uint8_t php(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->bus.write(cpu, STACKBASE | cpu->SP--, cpu->status.ps);

  return 1;
}
static uint8_t pla(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->bus.read(cpu, STACKBASE | ++cpu->SP);

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t plp(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.ps = cpu->bus.read(cpu, STACKBASE | ++cpu->SP);

  return 1;
}

// Logical ------------------------------------------
static uint8_t and (MOS6502 * cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->A & ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t eor(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->A ^ ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t ora(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->A = cpu->A | ctx->operand_immediate;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t bit(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint8_t result = cpu->A & ctx->operand_immediate;

  cpu->status.flags.V = (result & (1 << 6)) == (1 << 6);
  setzeroandnegative(cpu, result);
  return 1;
}

// Arithmetic --------------------------------------
static uint8_t adc(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t result = cpu->A + ctx->operand_immediate + cpu->status.flags.C;

  cpu->status.flags.C = result > 0xFF;
  cpu->status.flags.V =
      ((~(cpu->A ^ ctx->operand_immediate) & (cpu->A ^ result)) & 0x0080) ==
      0x0080;
  cpu->A = result & 0x00FF;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t sbc(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t result = ctx->operand_immediate ^ 0x00FF;
  uint16_t temp = cpu->A + result + cpu->status.flags.C;

  cpu->status.flags.C = (temp & 0x00FF) != 0;
  cpu->status.flags.V = ((temp ^ cpu->A) & (temp ^ result) & 0x0080) == 0x0080;
  cpu->A = temp & 0x00FF;

  setzeroandnegative(cpu, cpu->A);
  return 1;
}
static uint8_t cmp(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t result = cpu->A - ctx->operand_immediate;

  cpu->status.flags.C = cpu->A >= ctx->operand_immediate;
  cpu->status.flags.Z = cpu->A == ctx->operand_immediate;
  cpu->status.flags.N = (result & 0x0080) == 0x0080;

  return 1;
}
static uint8_t cpx(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t result = cpu->X - ctx->operand_immediate;

  cpu->status.flags.C = cpu->X >= ctx->operand_immediate;
  cpu->status.flags.Z = cpu->A == ctx->operand_immediate;
  cpu->status.flags.N = (result & 0x0080) == 0x0080;

  return 1;
}
static uint8_t cpy(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t result = cpu->Y - ctx->operand_immediate;

  cpu->status.flags.C = cpu->Y >= ctx->operand_immediate;
  cpu->status.flags.Z = cpu->Y == ctx->operand_immediate;
  cpu->status.flags.N = (result & 0x0080) == 0x0080;

  return 1;
}

// Increment and decrement
static uint8_t inc(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint8_t value = cpu->bus.read(cpu, ctx->absolute_addr);
  cpu->bus.write(cpu, ctx->absolute_addr, ++value);

  setzeroandnegative(cpu, value);
  return 1;
}
static uint8_t inx(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->X++;

  setzeroandnegative(cpu, cpu->X);
  return 1;
}
static uint8_t iny(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->Y++;

  setzeroandnegative(cpu, cpu->Y);
  return 1;
}
static uint8_t dec(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint8_t value = cpu->bus.read(cpu, ctx->absolute_addr);
  cpu->bus.write(cpu, ctx->absolute_addr, --value);

  setzeroandnegative(cpu, value);
  return 1;
}
static uint8_t dex(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->X--;

  setzeroandnegative(cpu, cpu->X);
  return 1;
}
static uint8_t dey(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->Y--;

  setzeroandnegative(cpu, cpu->Y);
  return 1;
}

// Shifts
// -----

// Jumps
static uint8_t jmp(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->PC = START | ctx->absolute_addr;

  return 1;
}
static uint8_t jsr(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint16_t return_address = cpu->PC + 3;
  cpu->bus.write(cpu, STACKBASE | cpu->SP--, return_address >> 8);
  cpu->bus.write(cpu, STACKBASE | cpu->SP--, return_address & 0x00FF);
  cpu->PC = START | ctx->absolute_addr;

  return 1;
}
static uint8_t rts(MOS6502 *cpu, MOS6502IContext *ctx) {
  uint8_t lo = cpu->bus.read(cpu, STACKBASE | ++cpu->SP);
  uint8_t hi = cpu->bus.read(cpu, STACKBASE | ++cpu->SP);
  uint16_t return_address = (hi << 8) | lo;

  cpu->PC = return_address;
  return 1;
}

// Branches
static void branch(MOS6502 *cpu, uint8_t relative, uint8_t flag) {
  if (flag) {
    cpu->PC += relative + 2;
  } else {
    cpu->PC += 2;
  }
}
static uint8_t bcc(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, !cpu->status.flags.C);
  return 1;
}
static uint8_t bcs(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, cpu->status.flags.C);
  return 1;
}
static uint8_t beq(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, cpu->status.flags.Z);
  return 1;
}
static uint8_t bmi(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, cpu->status.flags.N);
  return 1;
}
static uint8_t bne(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, !cpu->status.flags.Z);
  return 1;
}
static uint8_t bpl(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, !cpu->status.flags.N);
  return 1;
}
static uint8_t bvc(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, !cpu->status.flags.V);
  return 1;
}
static uint8_t bvs(MOS6502 *cpu, MOS6502IContext *ctx) {
  branch(cpu, ctx->operand_immediate, cpu->status.flags.V);
  return 1;
}

// Status Flags
static uint8_t clc(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.C = 0;
  return 1;
}
static uint8_t cld(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.D = 0;
  return 1;
}
static uint8_t cli(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.I = 0;
  return 1;
}
static uint8_t clv(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.V = 0;
  return 1;
}
static uint8_t sec(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.C = 1;
  return 1;
}
static uint8_t sed(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.D = 1;
  return 1;
}
static uint8_t sei(MOS6502 *cpu, MOS6502IContext *ctx) {
  cpu->status.flags.I = 1;
  return 1;
}

// Other
static uint8_t nop(MOS6502 *cpu, MOS6502IContext *ctx) {
  return 1;
}

// -----------------------------------------

MOS6502Instruction opcodes[MAXOPCODESTABLE] = {
    {0x00, "BRK", illg, IMP},
    {0x01, "ORA", ora, IDEIND},
    {0x02, ILLEGAL, illg, ILL},
    {0x03, ILLEGAL, illg, ILL},
    {0x04, ILLEGAL, illg, ILL},
    {0x05, "ORA", ora, ZP0},
    {0x06, "ASL", illg, ZP0},
    {0x07, ILLEGAL, illg, ILL},
    {0x08, "PHP", php, IMP},
    {0x09, "ORA", ora, IMM},
    {0x0a, "ASL", illg, ACC},
    {0x0b, ILLEGAL, illg, ILL},
    {0x0c, ILLEGAL, illg, ILL},
    {0x0d, "ORA", ora, ABS},
    {0x0e, "ASL", illg, ABS},
    {0x0f, ILLEGAL, illg, ILL}, // 1
    // -----------------
    {0x10, "BPL", bpl, RELT},
    {0x11, "ORA", ora, INDIDE},
    {0x12, ILLEGAL, illg, ILL},
    {0x13, ILLEGAL, illg, ILL},
    {0x14, ILLEGAL, illg, ILL},
    {0x15, "ORA", ora, ZP0X},
    {0x16, "ASL", illg, ZP0X},
    {0x17, ILLEGAL, illg, ILL},
    {0x18, "CLC", clc, IMP},
    {0x19, "ORA", ora, ABSY},
    {0x1a, ILLEGAL, illg, ILL},
    {0x1b, ILLEGAL, illg, ILL},
    {0x1c, ILLEGAL, illg, ILL},
    {0x1d, "ORA", ora, ABSX},
    {0x1e, "ASL", illg, ABSX},
    {0x1f, ILLEGAL, illg, ILL}, // 2
    // ------------------
    {0x20, "JSR", jsr, ABS},
    {0x21, "AND", and, IDEIND},
    {0x22, ILLEGAL, illg, ILL},
    {0x23, ILLEGAL, illg, ILL},
    {0x24, "BIT", bit, ZP0},
    {0x25, "AND", and, ZP0},
    {0x26, "ROL", illg, ZP0},
    {0x27, ILLEGAL, illg, ILL},
    {0x28, "PLP", plp, IMP},
    {0x29, "AND", and, IMM},
    {0x2a, "ROL", illg, ACC},
    {0x2b, ILLEGAL, illg, ILL},
    {0x2c, "BIT", bit, ABS},
    {0x2d, "AND", and, ABS},
    {0x2e, "ROL", illg, ABS},
    {0x2f, ILLEGAL, illg, ILL}, // 3
    // -------------------
    {0x30, "BMI", bmi, RELT},
    {0x31, "AND", and, INDIDE},
    {0x32, ILLEGAL, illg, ILL},
    {0x33, ILLEGAL, illg, ILL},
    {0x34, ILLEGAL, illg, ILL},
    {0x35, "AND", and, ZP0X},
    {0x36, "ROL", illg, ZP0X},
    {0x37, ILLEGAL, illg, ILL},
    {0x38, "SEC", sec, IMP},
    {0x39, "AND", and, ABSY},
    {0x3a, ILLEGAL, illg, ILL},
    {0x3b, ILLEGAL, illg, ILL},
    {0x3c, ILLEGAL, illg, ILL},
    {0x3d, "AND", and, ABSX},
    {0x3e, "ROL", illg, ABSX},
    {0x3f, ILLEGAL, illg, ILL}, // 4
    // ---------------------
    {0x40, "RTI", illg, IMP},
    {0x41, "EOR", eor, IDEIND},
    {0x42, ILLEGAL, illg, ILL},
    {0x43, ILLEGAL, illg, ILL},
    {0x44, ILLEGAL, illg, ILL},
    {0x45, "EOR", eor, ZP0},
    {0x46, "LSR", illg, ZP0},
    {0x47, ILLEGAL, illg, ILL},
    {0x48, "PHA", pha, IMP},
    {0x49, "EOR", eor, IMM},
    {0x4a, "LSR", illg, ACC},
    {0x4b, ILLEGAL, illg, ILL},
    {0x4c, "JMP", jmp, ABS},
    {0x4d, "EOR", eor, ABS},
    {0x4e, "LSR", illg, ABS},
    {0x4f, ILLEGAL, illg, ILL}, // 5
    // --------------------
    {0x50, "BVC", bvc, RELT},
    {0x51, "EOR", eor, INDIDE},
    {0x52, ILLEGAL, illg, ILL},
    {0x53, ILLEGAL, illg, ILL},
    {0x54, ILLEGAL, illg, ILL},
    {0x55, "EOR", eor, ZP0X},
    {0x56, "LSR", illg, ZP0X},
    {0x57, ILLEGAL, illg, ILL},
    {0x58, "CLI", cli, IMP},
    {0x59, "EOR", eor, ABSY},
    {0x5a, ILLEGAL, illg, ILL},
    {0x5b, ILLEGAL, illg, ILL},
    {0x5c, ILLEGAL, illg, ILL},
    {0x5d, "EOR", eor, ABSX},
    {0x5e, "LSR", illg, ABSX},
    {0x5f, ILLEGAL, illg, ILL}, // 6
    // ----------------------
    {0x60, "RTS", rts, IMP},
    {0x61, "ADC", adc, IDEIND},
    {0x62, ILLEGAL, illg, ILL},
    {0x63, ILLEGAL, illg, ILL},
    {0x64, ILLEGAL, illg, ILL},
    {0x65, "ADC", adc, ZP0},
    {0x66, "ROR", illg, ZP0},
    {0x67, ILLEGAL, illg, ILL},
    {0x68, "PLA", pla, IMP},
    {0x69, "ADC", adc, IMM},
    {0x6a, "ROR", illg, ACC},
    {0x6b, ILLEGAL, illg, ILL},
    {0x6c, "JMP", jmp, IND},
    {0x6d, "ADC", adc, ABS},
    {0x6e, "ROR", illg, ABS},
    {0x6f, ILLEGAL, illg, ILL}, // 7
    // ----------------------
    {0x70, "BVS", bvs, RELT},
    {0x71, "ADC", adc, INDIDE},
    {0x72, ILLEGAL, illg, ILL},
    {0x73, ILLEGAL, illg, ILL},
    {0x74, ILLEGAL, illg, ILL},
    {0x75, "ADC", adc, ZP0X},
    {0x76, "ROR", illg, ZP0X},
    {0x77, ILLEGAL, illg, ILL},
    {0x78, "SEI", sei, IMP},
    {0x79, "ADC", adc, ABSY},
    {0x7a, ILLEGAL, illg, ILL},
    {0x7b, ILLEGAL, illg, ILL},
    {0x7c, ILLEGAL, illg, ILL},
    {0x7d, "ADC", adc, ABSX},
    {0x7e, "ROR", illg, ABSX},
    {0x7f, ILLEGAL, illg, ILL}, // 8
    // ----------------------
    {0x80, ILLEGAL, illg, ILL},
    {0x81, "STA", sta, IDEIND},
    {0x82, ILLEGAL, illg, ILL},
    {0x83, ILLEGAL, illg, ILL},
    {0x84, "STY", sty, ZP0},
    {0x85, "STA", sta, ZP0},
    {0x86, "STX", stx, ZP0},
    {0x87, ILLEGAL, illg, ILL},
    {0x88, "DEY", dey, IMP},
    {0x89, ILLEGAL, illg, ILL},
    {0x8a, "TXA", txa, IMP},
    {0x8b, ILLEGAL, illg, ILL},
    {0x8c, "STY", sty, ABS},
    {0x8d, "STA", sta, ABS},
    {0x8e, "STX", stx, ABS},
    {0x8f, ILLEGAL, illg, ILL}, // 9
    // ---------------------
    {0x90, "BCC", bcc, RELT},
    {0x91, "STA", sta, INDIDE},
    {0x92, ILLEGAL, illg, ILL},
    {0x93, ILLEGAL, illg, ILL},
    {0x94, "STY", sty, ZP0X},
    {0x95, "STA", sta, ZP0X},
    {0x96, "STX", stx, ZP0Y},
    {0x97, ILLEGAL, illg, ILL},
    {0x98, "TYA", tya, IMP},
    {0x99, "STA", sta, ABSY},
    {0x9a, "TXS", txs, IMP},
    {0x9b, ILLEGAL, illg, ILL},
    {0x9c, ILLEGAL, illg, ILL},
    {0x9d, "STA", sta, ABSX},
    {0x9e, ILLEGAL, illg, ILL},
    {0x9f, ILLEGAL, illg, ILL}, // 10
    // ----------------------
    {0xa0, "LDY", ldy, IMM},
    {0xa1, "LDA", lda, IDEIND},
    {0xa2, "LDX", ldx, IMM},
    {0xa3, ILLEGAL, illg, ILL},
    {0xa4, "LDY", ldy, ZP0},
    {0xa5, "LDA", lda, ZP0},
    {0xa6, "LDX", ldx, ZP0},
    {0xa7, ILLEGAL, illg, ILL},
    {0xa8, "TAY", tay, IMP},
    {0xa9, "LDA", lda, IMM},
    {0xaa, "TAX", tax, IMP},
    {0xab, ILLEGAL, illg, ILL},
    {0xac, "LDY", ldy, ABS},
    {0xad, "LDA", lda, ABS},
    {0xae, "LDX", ldx, ABS},
    {0xaf, ILLEGAL, illg, ILL}, // 11
    // --------------------
    {0xb0, "BCS", bcs, RELT},
    {0xb1, "LDA", lda, INDIDE},
    {0xb2, ILLEGAL, illg, ILL},
    {0xb3, ILLEGAL, illg, ILL},
    {0xb4, "LDY", ldy, ZP0X},
    {0xb5, "LDA", lda, ZP0X},
    {0xb6, "LDX", ldx, ZP0Y},
    {0xb7, ILLEGAL, illg, ILL},
    {0xb8, "CLV", clv, IMP},
    {0xb9, "LDA", lda, ABSY},
    {0xba, "TSX", tsx, IMP},
    {0xbb, ILLEGAL, illg, ILL},
    {0xbc, "LDY", ldy, ABSX},
    {0xbd, "LDA", lda, ABSX},
    {0xbe, "LDX", ldx, ABSY},
    {0xbf, ILLEGAL, illg, ILL}, // 12
    // -------------------
    {0xc0, "CPY", cpy, IMM},
    {0xc1, "CMP", cmp, IDEIND},
    {0xc2, ILLEGAL, illg, ILL},
    {0xc3, ILLEGAL, illg, ILL},
    {0xc4, "CPY", cpy, ZP0},
    {0xc5, "CMP", cmp, ZP0},
    {0xc6, "DEC", dec, ZP0},
    {0xc7, ILLEGAL, illg, ILL},
    {0xc8, "INY", iny, IMP},
    {0xc9, "CMP", cmp, IMM},
    {0xca, "DEX", dex, IMP},
    {0xcb, ILLEGAL, illg, ILL},
    {0xcc, "CPY", cpy, ABS},
    {0xcd, "CMP", cmp, ABS},
    {0xce, "DEC", dec, ABS},
    {0xcf, ILLEGAL, illg, ILL}, // 13
    // --------------------
    {0xd0, "BNE", bne, RELT},
    {0xd1, "CMP", cmp, INDIDE},
    {0xd2, ILLEGAL, illg, ILL},
    {0xd3, ILLEGAL, illg, ILL},
    {0xd4, ILLEGAL, illg, ILL},
    {0xd5, "CMP", cmp, ZP0X},
    {0xd6, "DEC", dec, ZP0X},
    {0xd7, ILLEGAL, illg, ILL},
    {0xd8, "CLD", cld, IMP},
    {0xd9, "CMP", cmp, ABSY},
    {0xda, ILLEGAL, illg, ILL},
    {0xdb, ILLEGAL, illg, ILL},
    {0xdc, ILLEGAL, illg, ILL},
    {0xdd, "CMP", cmp, ABSX},
    {0xde, "DEC", dec, ABSX},
    {0xdf, ILLEGAL, illg, ILL}, // 14
    // --------------------
    {0xe0, "CPX", cpx, IMM},
    {0xe1, "SBC", sbc, IDEIND},
    {0xe2, ILLEGAL, illg, ILL},
    {0xe3, ILLEGAL, illg, ILL},
    {0xe4, "CPX", cpx, ZP0},
    {0xe5, "SBC", sbc, ZP0},
    {0xe6, "INC", inc, ZP0},
    {0xe7, ILLEGAL, illg, ILL},
    {0xe8, "INX", inx, IMP},
    {0xe9, "SBC", sbc, IMM},
    {0xea, "NOP", nop, IMP},
    {0xeb, ILLEGAL, illg, ILL},
    {0xec, "CPX", cpx, ABS},
    {0xed, "SBC", sbc, ABS},
    {0xee, "INC", inc, ABS},
    {0xef, ILLEGAL, illg, ILL}, // 15
    // ----------------------
    {0xf0, "BEQ", beq, RELT},
    {0xf1, "SBC", sbc, INDIDE},
    {0xf2, ILLEGAL, illg, ILL},
    {0xf3, ILLEGAL, illg, ILL},
    {0xf4, ILLEGAL, illg, ILL},
    {0xf5, "SBC", sbc, ZP0X},
    {0xf6, "INC", inc, ZP0X},
    {0xf7, ILLEGAL, illg, ILL},
    {0xf8, "SED", sed, IMP},
    {0xf9, "SBC", sbc, ABSY},
    {0xfa, ILLEGAL, illg, ILL},
    {0xfb, ILLEGAL, illg, ILL},
    {0xfc, ILLEGAL, illg, ILL},
    {0xfd, "SBC", sbc, ABSX},
    {0xfe, "INC", inc, ABSX},
    {0xff, ILLEGAL, illg, ILL} // 16
                               // ----------------------
                               // ----------------------
};

static uint8_t isvalidopcode(uint8_t opcode) {
  if (opcode <= 0x00 || opcode > 0xff)
    return 0;

  return 1;
}

uint16_t mos6502_execute(MOS6502 *cpu) {
  uint8_t opcode = cpu->bus.read(cpu, cpu->PC);

  if (!isvalidopcode(opcode)) {
    return 0x7FFF;
  }

  MOS6502IContext context = {0};

  switch (opcodes[opcode].mode) {
    case IMP: { // Implied
      opcodes[opcode].exec(cpu, NULL);

      if ((void *)opcodes[opcode].exec == (void *)rts) {
        return opcode;
      }

      cpu->PC++;
      return opcode;
    }

    case ACC: { // Accumulator
      opcodes[opcode].exec(cpu, NULL);

      cpu->PC++;
      return opcode;
    }

    case IMM: { // Immediate
      context.operand_immediate = cpu->bus.read(cpu, cpu->PC + 1);
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 2;
      return opcode;
    }

    case ZP0: { // Zero Page
      uint8_t addr = cpu->bus.read(cpu, cpu->PC + 1);
      context.operand_immediate = cpu->bus.read(cpu, addr);
      context.absolute_addr = addr;
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 2;
      return opcode;
    }

    case ZP0X: { // Zero Page with X
      uint8_t addr = cpu->bus.read(cpu, cpu->PC + 1);
      context.operand_immediate = cpu->bus.read(cpu, addr + cpu->X);
      context.absolute_addr = addr + cpu->X;
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 2;
      return opcode;
    }

    case ZP0Y: { // Zero Page with Y
      uint8_t addr = cpu->bus.read(cpu, cpu->PC + 1);
      context.operand_immediate = cpu->bus.read(cpu, addr + cpu->Y);
      context.absolute_addr = addr + cpu->Y;
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 2;
      return opcode;
    }

    case RELT: { // Relative
      context.operand_immediate = cpu->bus.read(cpu, cpu->PC + 1);
      opcodes[opcode].exec(cpu, &context);
      
      return opcode;
    }

    case ABS: { // Absolute
      uint8_t lo = cpu->bus.read(cpu, cpu->PC + 1);
      uint8_t hi = cpu->bus.read(cpu, cpu->PC + 2);
      uint16_t addr = ((hi << 8) | lo);

      context.operand_immediate = cpu->bus.read(cpu, addr);
      context.absolute_addr = addr;
      opcodes[opcode].exec(cpu, &context);

      if ((void *)opcodes[opcode].exec == (void *)jmp ||
          (void *)opcodes[opcode].exec == (void *)jsr) {
        return opcode;
      }

      cpu->PC += 3;
      return opcode;
    }

    case ABSX: { // Absolute with X
      uint8_t lo = cpu->bus.read(cpu, cpu->PC + 1);
      uint8_t hi = cpu->bus.read(cpu, cpu->PC + 2);
      uint16_t addr = ((hi << 8) | lo) + cpu->X;

      context.operand_immediate = cpu->bus.read(cpu, addr);
      context.absolute_addr = addr;
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 3;
      return opcode;
    }

    case ABSY: { // Absolute with Y
      uint8_t lo = cpu->bus.read(cpu, cpu->PC + 1);
      uint8_t hi = cpu->bus.read(cpu, cpu->PC + 2);
      uint16_t addr = ((hi << 8) | lo) + cpu->Y;

      context.operand_immediate = cpu->bus.read(cpu, addr);
      context.absolute_addr = addr;
      opcodes[opcode].exec(cpu, &context);

      cpu->PC += 3;
      return opcode;
    }

    case IND: { // Indirect
      break;
    }
    case IDEIND: // Indexed Indirect X
      break;
    case INDIDE: // Indirect Indexed Y
      break;
    case ILL:
      return 0x7FFF;
  }

  return 0x7FFF;
}
