// Main program for serving AGC. Loads memory, displays log.
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
  const char *fname = NULL;
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

  if (fname == NULL) {
    printf("No file read, using memory.\n");
  } else {
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
    printf("Read %s\n", fname);
  }

  // iface is at the start of the 8K PRU0 RAM
  volatile struct iface *iface = (volatile struct iface *) mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x4a300000); // PRU_ICSS
  if (iface == 0) {
    perror("Mmap /dev/mem failed on iface");
    exit(-1);
  }

  printf("state r31_addr r31_active count [addr data]\n");
  uint16_t i = 0;
  while (1) {
    while (i == iface->bufend) {
      // Wait if buffer empty
      usleep(1000);
    }
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
      uint16_t addr = iface->faultaddr;
      uint16_t data = iface->faultdata;
      uint16_t data2 = ((data & 0x8000) >> 1) | (data & 0x3fff);
      printf(" a:%x d:%x   a:%06o d:%05o %05o", addr, data, addr, data, data2);
    } else if (state == STATE_DONE) {
      printf("DONE   ");
    } else {
      printf("?%x ", iface->buf[i]);
    }
    printf("%d %d %d", r31 >> 1, r31 & 1, count);
    if (state == STATE_GOT_ADDR) {
      // Double entry
      uint16_t addr = iface->buf[i+1] >> 16;
      uint16_t data = iface->buf[i+1] & 0xffff;
      uint16_t data2 = ((data & 0x8000) >> 1) | (data & 0x3fff);
      printf(" a:%x d:%x   a:%06o d:%05o %05o", addr, data, addr, data, data2);
      i++;
    }
    printf("\n");
    if (state == STATE_DONE) {
      printf("\n");
    }
    i = (i + 1) % BUFSIZE;
  }
}
