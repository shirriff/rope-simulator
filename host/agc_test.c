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
  volatile uint16_t *shared = (uint16_t *)mmap(NULL, 65536 * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x81400000); // shared
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
  uint16_t oldaddr = 0xff;
  uint16_t oldmisc = 0xff;

  printf("     plane   inhibit\n");
  printf("a c 87654321 07654321\n");
  while (1) {
      uint16_t addr = shared[0];
      uint16_t misc = shared[2];
      if (oldaddr != addr || oldmisc != misc) {
          printf("%d %d ", misc & 1, (misc & 2) >> 1);
	  for (int i = 15; i >= 0; i--) {
	    if (addr & (1 << i)) {
	      printf("1");
	    } else {
	      printf("0");
	    }
	    if (i == 8) {
	      printf(" ");
	    }
	  }
	  printf("\n");
          usleep(100000);
      }
      oldaddr = addr;
      oldmisc = misc;
  }
}
