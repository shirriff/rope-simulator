// PRU code to support the core rope simulator for the AGC
#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "iface.h"

// 64K of reserved RAM for sharing with ARM
#define SHARED ((volatile void *)(0x9fc00000))
#define GPIO0	0x44e07000		// GPIO Bank 0  See Table 2.2 of TRM <1>
#define GPIO1	0x4804c000		// GPIO Bank 1
#define GPIO2	0x481ac000		// GPIO Bank 2
#define GPIO3	0x481ae000		// GPIO Bank 3
#define MEM_START 0x00000000

#define GPIO_DATAIN		(0x138 >> 2)	// For reading the GPIO registers
#define GPIO_DATAOUT		(0x13c >> 2)	// For writing the GPIO registers


volatile register uint32_t __R30;
volatile register uint32_t __R31;

#define PWM00 ((volatile uint16_t *)0x48300212) // P9_22
#define PWM01 ((volatile uint16_t *)0x48300214) // P9_21
#define PWM21 ((volatile uint16_t *)0x48302214) // P8_34

inline void pwm_red() {
	*PWM00 = 0xc350;
	*PWM01 = 0;
	*PWM21 = 0;
}

inline void log(volatile struct iface *iface, uint16_t count, uint32_t r31, uint32_t state) {
      iface->state = state;
      iface->bufend = (iface->bufend + 1) % BUFSIZE;
      iface->buf[iface->bufend] = (count << 6) | (r31 << 4) | state;;
}


inline void pwm_green() {
	*PWM00 = 0;
	*PWM01 = 0xc350;
	*PWM21 = 0;
}


inline void pwm_blue() {
	*PWM00 = 0;
	*PWM01 = 0;
	*PWM21 = 0xc350;
}


inline void pwm_cyan() {
	*PWM00 = 0;
	*PWM01 = 0xc350;
	*PWM21 = 0xc350;
}


inline void pwm_magenta() {
	*PWM00 = 0xc350;
	*PWM01 = 0;
	*PWM21 = 0xc350;
}

inline void pwm_yellow() {
	*PWM00 = 0xc350;
	*PWM01 = 0xc350 / 2;
	*PWM21 = 0;
}

void fault() {
        red();
	__delay_cycles(200000); // 1 second
	// Wait until back in idle state
	while ((__R31 & 3) != 2) {};
}

void main(void)
{
	volatile uint32_t *gpio0 = (uint32_t *)GPIO0;
	volatile uint32_t *gpio1 = (uint32_t *)GPIO1;
	volatile uint32_t *gpio2 = (uint32_t *)GPIO2;
	volatile uint32_t *gpio3 = (uint32_t *)GPIO3;
	volatile uint16_t *shared = (uint16_t *)SHARED;
	uint32_t count = 0;
        volatile struct iface *iface = (volatile struct iface * )MEM_START;
	// Turn on PWM
	*PWM00 = 0x63;
	*PWM01 = 0x63;
	*PWM21 = 0x63;
	// Write to GPIO2
	gpio2[GPIO_DATAOUT/4] = 0xffffffff;
	// Write to GPIO3
	gpio3[GPIO_DATAOUT/4] = 1<<16;

        int toggle = 0;
	uint32_t r31 = __R31 & 3;

	while (1) {
	    pwm_green();
	    log(iface, count, r31, STATE_IDLE);
	    gpio2[GPIO_DATAOUT] = 0; // Clear sense lines
            // Wait for OUTPUT_ACTIVE

	    // Wait in idle state until OUTPUT_ACTIVE
	    while (r31 == 2) {
	      r31 == __R31 & 3;
	      count++;
	    }

	    if (r31 != 3) {
	      log(iface, count, r31, STATE_FAULT);
	      fault();
	      break;
	    }

	    // Now active state
            pwm_magenta();
	    log(iface, count, r31, STATE_ACTIVE);

	    while (r31 == 3) {
	      r31 == __R31 & 3;
	      count++;
	    }

	    if (r31 == 2) {
	      // Active signal dropped. Clear signal must have blocked read.
	      log(iface, count, r31, STATE_CLEAR_CANCEL);
	      break;
	    } else if (r31 != 1) {
	      // Unexpected state
	      log(iface, count, r31, STATE_FAULT);
	      fault();
	      break;
	    }

	    // GOT_ADDR 
	    blue();
	    log(iface, count, r31, STATE_GOT_ADDR);

	    // At this point, the address should be good.
	    // Get the address
	    uint32_t address = (gpio0[GPIO_DATAIN] & 0xff00) | ((gpio1[GPIO_DATAIN] >> 12) & 0x00ff);
	    address = count & 0xffff;
	    uint16_t data = shared[address];
	    gpio2[GPIO_DATAOUT] = ((uint32_t) data) << 1; // Sense data starts in bit 1 of gpio2
	    iface->lastaddr = address;
	    iface->lastdata = data;

	    // Wait for GOT_ADDR drop
	    while (r31 == 1) {
	      r31 == __R31 & 3;
	      count++;
	    }

	    if (r31 != 3) {
	      // Unexpected state
	      log(iface, count, r31, STATE_FAULT);
	      fault();
	      break;
	    }

	    // GOT_ADDR dropped
	    yellow();
	    log(iface, count, r31, STATE_WRITING);

	    while (r31 == 3) {
	      r31 == __R31 & 3;
	      count++;
	    }

	    if (r31 != 2) {
	      // Unexpected state
	      log(iface, count, r31, STATE_FAULT);
	      fault();
	      break;
	    }

	    // Done
	    log(iface, count, r31, STATE_IDLE);
	    pwm_green();
	}
}
