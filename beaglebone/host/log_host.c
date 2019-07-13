// Dump log from shared info buffer.

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../pru/iface.h"

void printBinary(int n, int nbits) {
  for (int i = nbits-1; i >= 0; i--) {
    if (n & (1 << i)) {
      printf("1");
    } else {
      printf("0");
    }
  }
}

// Read /dev/mem
void main(int argc, char **argv) {
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

  // iface is at the start of the 8K PRU0 RAM
  volatile struct iface *iface = (volatile struct iface *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x4a300000); // PRU_ICSS
  if (iface == 0) {
    perror("Mmap /dev/mem failed on iface");
    exit(-1);
  }

  if (argc == 1) {
    printf("Shared memory dump...\n");
    for (uint32_t i = 0; i < 65536; i++) {
      if (shared[i] & 2) {
        continue;
      }
      printf("%6d ", i);
      printBinary(shared[i], 16);
      printf("\n");
    }
  } else {
    printf("PRU memory dump...\n");
    volatile uint16_t *pru_mem = (uint16_t *)iface;
    for (uint32_t i = 0; i < 4096; i++) {
      printf("%6d ", i);
      printBinary(pru_mem[i], 16);
      printf("\n");
    }
  }
}
