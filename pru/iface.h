/*
 * iface.h
 *
 * Interface between PRU and host
 */

#ifndef IFACE_H_
#define IFACE_H_
#include <stdint.h>

#define BUFSIZE 400

#define STATE_IDLE 1 // Waiting for memory access: green/blue
#define STATE_GOT_ADDRESS 2 // OUTPUT_ADDR_OKAY indicated address available: magenta
#define STATE_WAIT_CLEAR_GATE 3 // Waiting to get past CLEAR: cyan
#define STATE_WRITING 4 // Writing to sense: yellow
#define STATE_CLEAR_CANCEL 5 // Access canceled by CLEAR: red

// Interface between host and PRU
struct iface {
	uint32_t lastaddr; // Last address accessed (out)
	uint32_t lastdata; // Last address accessed (out)
        uint32_t state; // Current state
        uint32_t bufend; // last written entry in buf. (out)
	uint32_t buf[BUFSIZE]; // Circular buffer (out)
};
#endif /* IFACE_H_ */
