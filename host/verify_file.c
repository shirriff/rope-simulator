// Verify file in shared memory against file on disk.
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

int parity(uint16_t word) {
  int p = 0;
  for (int i = 0; i < 16; i++) {
    if (word & (1 << i)) {
      p ^= 1;
    }
  }
  return p;
}

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
  uint16_t buf[36 * 1024];
  size_t res = fread((uint16_t *)buf, len, 1, f);
  if (res != 1) {
    perror(fname);
    exit(-1);
  }
  fclose(f);
  printf("Checking %s\n", fname);

  // Byte swap
  for (int i = 0; i < 36 * 1024; i++) {
    uint16_t word = shared[i];
    uint16_t wanted = (buf[i] << 8) | (buf[i] >> 8);
    if (word != wanted) {
      printf("Mismatch at %x: %x vs %x\n", i, wanted, word);
    }
    if (parity(word) == 0) {
      printf("Parity error at %x: %x\n", i, word);
    }
  }
}
