// Display address from shared info buffer. Filter out oscillations.
// cc -o display_addr display_addr.c -lncurses

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
void main(void) {
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
  char *labels[] = {"R1", "R2", "S1", "S2", "T1", "T2"};

  uint16_t lastaddr = 0;
  while (1) {
    time_t start = time(NULL);
    uint16_t addr = 0;
    for (int i = 0; i < 1000; i++) {
      uint16_t addr0 = iface->lastaddr;
      addr |= addr0;
    }
    if (addr == lastaddr) continue;
    lastaddr = addr;
    printBinary(addr, 16);
    printf(" %d ", start);

    uint16_t il = addr & 0x7f;
    uint16_t plane = (addr >> 7) & 3; // SetAB, SetCD, Reset
    uint16_t strand = (addr >> 9) % 12;
    uint16_t mod = (addr >> 9) / 12;
    char *label = "!!!";
    if (mod < 6) {
      label = labels[mod];
    }
    printf("   %06x %s %2d %d %2x %d %d  ", addr, label, strand, plane, il, iface->buf[0], iface->buf[1]);
    printf("\n");
    usleep(10000);
  }
}
