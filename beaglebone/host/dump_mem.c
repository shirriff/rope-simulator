// Loads memory from file, dumps out content.
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../pru/iface.h"


// Read /dev/mem
void main(int argc, char **argv) {
  const char *fname = "Aurora12.bin";
  if (argc > 1) {
    fname = argv[1];
  }
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
    perror("Open /dev/mem failed");
    exit(-1);
  }
  // volatile uint32_t *vaddr = (uint32_t *)mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x48300000); // PWM reg

  // 64K buffer shared with the PRU
  volatile uint16_t *shared = (uint16_t *)mmap(NULL, 65536 * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9fc00000); // shared
  if (shared == 0) {
    perror("Mmap /dev/mem failed");
    exit(-1);
  }

  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    perror(fname);
    exit(-1);
  }

  size_t len = 36 * 2048; // 36K words
  size_t res = fread((uint16_t *)shared, len, 1, f);
  if (res != 1) {
    perror(fname);
    exit(-1);
  }
  fclose(f);

  // Byte swap
  for (int i = 0; i < 36 * 1024; i++) {
    uint16_t word = shared[i];
    shared[i] = (word << 8) | (word >> 8);
  }
  printf("Reading %s\n", fname);

  // iface is at the start of the 8K PRU0 RAM
  volatile struct iface *iface = (volatile struct iface *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x4a300000); // PRU_ICSS
  if (iface == 0) {
    perror("Mmap /dev/mem failed on iface");
    exit(-1);
  }

  for (uint16_t i = 0; i < 36 * 1024; i++) {
    int bank = i / 1024;
    int addr = i % 1024;
    uint16_t hw_word = shared[i];
    // Shift out the parity bit (bit 14)
    uint16_t agc_word = ((hw_word & 0x8000) >> 1) | (hw_word & 0x3fff);
    printf("%02o,%04o  %05o %04X\n", bank, addr + 02000, agc_word, hw_word);
  }
}
