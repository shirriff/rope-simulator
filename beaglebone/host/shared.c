// Client for shared memory testing
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

// Read /dev/mem
void main(void) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
    perror("Open /dev/mem failed");
    exit(-1);
  }
  // volatile uint32_t *vaddr = (uint32_t *)mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x48300000); // PWM reg
  volatile uint32_t *vaddr = (uint32_t *)mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9fc00000); // shared
  if (vaddr == 0) {
    perror("Mmap /dev/mem failed");
    exit(-1);
  }
  uint32_t vaddr1, vaddr2, vaddr3;
  printf("%08x\n", vaddr[0]);
  while (1) {
  uint32_t v = vaddr[1];
  if (v != vaddr1) {
    if (v > vaddr1) {
      vaddr[4] = 1;
    } else {
      vaddr[4] = 2;
    }
    for (int i = 15; i >= 0; i--) {
      if ((v^vaddr1) & (1 << i)) {
        printf("%d\n", i);
      }
    }
    vaddr1 = v;
  }
  v = vaddr[2];
  if (v != vaddr2) {
    vaddr2 = v;
    printf("%d\n", v);
  }
  usleep(100000);
  }
}
