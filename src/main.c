#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "6502.h"
#include "debug.h"

#define NOP 0xEA
#define INVALID 0x7FFF
#define OPTS "::p:"

int main(int argc, char **argv) {

  // Parse Args
  char *programpath = NULL;
  int option = 0;

  while ((option = getopt(argc, argv, OPTS)) != -1) {
    switch (option) {
      case 'p':
        programpath = optarg;
        break;
      case ':':
        fprintf(stderr, "Missing argument!\n");
        exit(EXIT_FAILURE);
      case '?':
        fprintf(stderr, "Usage: %s [-p program]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  // Init
  size_t programsize = getprogramsize(programpath);
  uint8_t programbytes[programsize];
  memset(&programbytes, 0, sizeof(programbytes));

  FILE *file = fopen(programpath, "rb");
  if (!file) {
    printfc(RED, "Error: 'open file' failed!\n");
    exit(EXIT_FAILURE);
  }

  printfc(WHITE, "[-] Program: %s\n", programpath);
  printfc(WHITE, "[-] Size: %zu bytes\n\n\n", programsize);

  MOS6502 *cpu = mos6502_init();

  fread(&programbytes, programsize, sizeof(uint8_t), file);
  size_t nbytes = mos6502_loadbytes(cpu, programbytes, programsize);
  if (nbytes != programsize) {
    printfc(RED, "Error: 'load bytes' failed! (%d bytes)\n", nbytes);
    exit(EXIT_FAILURE);
  }

  // Writing in some areas for testing
  cpu->bus.write(cpu, 0x0000, 0xd5);
  cpu->bus.write(cpu, 0x0001, 0xae);
  cpu->bus.write(cpu, 0x0002, 0xc9);

  cpu->bus.write(cpu, 0x4e20, 0xd4);
  cpu->bus.write(cpu, 0x4e21, 0xb5);
  cpu->bus.write(cpu, 0x4e22, 0xa5);

  cpu->bus.write(cpu, 0x00FF, 89);

  // Exec Loop
  while (1) {
    uint16_t backuppc = cpu->PC;
    uint16_t result = mos6502_execute(cpu);
    if (result == INVALID) {
      continue;
    }

    // Disassemble
    mos6502_disassemble(cpu, result, backuppc);

    // Print CPU and Bus Status
    mos6502_printstatus(cpu);

    // No Operation!
    if (result == NOP) {
      printfc(RED, "Stop!\n");
      break;
    }
  }
  
  mos6502_printopcodes();
  mos6502_uninit(cpu);
  return EXIT_SUCCESS;
}