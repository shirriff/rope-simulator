// Dump log from shared info buffer while running agc_pru..

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

  printf("%x\n", iface->lastaddr);
  printf("state r31_addr r31_active count [addr data]\n");
  for (int i = 0; i < iface->bufend; i++) {
    int state = iface->buf[i] & 0xf;
    int r31 = (iface->buf[i] >> 4) & 0x3;
    int count = iface->buf[i] >> 6;
    if (state == STATE_IDLE) {
      printf("IDLE   ");
    } else if (state == STATE_ACTIVE) {
      printf("ACTIVE ");
    } else if (state == STATE_GOT_ADDR) {
      printf("ADDR   ");
    } else if (state == STATE_WRITING) {
      printf("WRITE  ");
    } else if (state == STATE_CLEAR_CANCEL) {
      printf("CANCEL ");
    } else if (state == STATE_FAULT) {
      printf("FAULT  ");
    } else if (state == STATE_DONE) {
      printf("DONE   ");
    } else {
      printf("?%4d? ", state);
    }
    printf("%d %d %d", r31 >> 1, r31 & 1, count);
    if (state == STATE_GOT_ADDR) {
      // Double entry
      printf(" %x %x", iface->buf[i+1] >> 16, iface->buf[i+1] & 0xffff);
      i++;
    }
    printf("\n");
    if (state == STATE_DONE) {
      printf("\n");
    }
  }
}
