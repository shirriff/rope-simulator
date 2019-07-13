// Client for shared memory testing
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../pru/iface.h"


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

  FILE *f = fopen("rope.bin", "r");
  if (f == NULL) {
    perror("rope.bin");
    exit(-1);
  }

  size_t len = 36 * 2048; // 36K words
  size_t res = fread(shared, len, 1, f);
  if (res != 1) {
    perror("rope.bin read");
    exit(-1);
  }
  fclose(f);

  // iface is at the start of the 8K PRU0 RAM
  volatile struct iface *iface = (volatile struct iface *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x4a300000); // PRU_ICSS
  if (iface == 0) {
    perror("Mmap /dev/mem failed on iface");
    exit(-1);
  }

  for (int i = 0; i < len; i++) {
    iface->lastaddr = i;
    iface->state = 1; // Top byte
    while (iface->state == 1) {}
    if (iface->lastdata != shared[i] >> 8) {
      printf("Data mismatch at %d: %d vs %d\n", i, iface->lastdata, shared[i]);
    }
  }

  for (int i = 0; i < len; i++) {
    iface->lastaddr = i;
    iface->state = 2; // Bottom byte
    while (iface->state == 2) {}
    if (iface->lastdata != shared[i] & 0xff) {
      printf("Data mismatch at %d: %d vs %d\n", i, iface->lastdata, shared[i]);
    }
  }

  printf("Test done\n");
}
