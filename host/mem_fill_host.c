// Client for shared memory testing: fill shared memory with pattern
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
  uint16_t val = 0x58;
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

  size_t len = 36 * 1024; // 36K words
  if (argc == 2 && argv[1][0] == 'a') {
    printf("Filling memory with bit sweep\n");
    for (int i = 0; i < len; i++) {
      shared[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
      // Put sweep at end
      shared[len - 16 + i] = 1 << i;
    }
  } else {
    if (argc == 2) {
      sscanf(argv[1], "%x", &val);
    }
    printf("Filling memory with %x\n", val);
    for (int i = 0; i < len; i++) {
      shared[i] = val;
    }
  }
  printf("Memory initialized.\n");
  while (1) {
    printf("%x: %x\n", iface->lastaddr, iface->lastdata);
  }
}
