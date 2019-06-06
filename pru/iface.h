/*
 * iface.h
 *
 * Interface between PRU and host
 */

#ifndef IFACE_H_
#define IFACE_H_
#include <stdint.h>

#define BUFSIZE 400

#define STATE_IDLE 1 // Waiting for memory access: green
#define STATE_ACTIVE 2 // OUTPUT_ACTIVE: read started, magenta
#define STATE_GOT_ADDR 3 // ADDR_SET: blue
#define STATE_WRITING 4 // Writing to sense: yellow
#define STATE_CLEAR_CANCEL 5 // Access canceled by CLEAR
#define STATE_FAULT 6 // Unexpected state: red

// Interface between host and PRU
struct iface {
	uint32_t lastaddr; // Last address accessed (out)
	uint32_t lastdata; // Last address accessed (out)
        uint32_t state; // Current state
        uint32_t bufend; // last written entry in buf. (out)
	uint32_t buf[BUFSIZE]; // Circular buffer (out)
};
#endif /* IFACE_H_ */
