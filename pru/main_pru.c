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
inline void log(volatile struct iface *iface, uint32_t data) {
      iface->buf[iface->bufend] = data;
      iface->bufend = (iface->bufend + 1) % BUFSIZE;
}

inline void update_status(volatile struct iface *iface, uint16_t count, uint32_t r31, uint32_t state) {
      iface->state = state;
      log(iface, (count << 6) | (r31 << 4) | state);
}


inline void pwm_red() {
	*PWM00 = 0xc350;
	*PWM01 = 0;
	*PWM21 = 0;
}

inline void pwm_white() {
	*PWM00 = 0xc350;
	*PWM01 = 0xc350;
	*PWM21 = 0xc350;
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

void fault(volatile struct iface *iface) {
        pwm_red();
	uint32_t r31 = __R31 & 3;
	do {
	  r31 = __R31 & 3;
	  update_status(iface, 998, r31, STATE_FAULT);
	  __delay_cycles(100000); // .5 second
	} while (r31 != 2);
	update_status(iface, 999, r31, STATE_FAULT);
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

	uint32_t r31 = __R31 & 3;

	// Initial wait for idle
	pwm_white();
	update_status(iface, count, r31, STATE_IDLE);
	__delay_cycles(50000); // .25 second
	while (r31 != 2) {
	  r31 = __R31 & 3;
	  count++;
	}

	while (1) {
idle_loop:
	    count = 0;
	    pwm_green();
	    r31 = __R31 & 3;
	    update_status(iface, count, r31, STATE_IDLE);
	    gpio2[GPIO_DATAOUT] = 0xffff << 1; // Clear sense lines
            // Wait for OUTPUT_ACTIVE

	    // Wait in idle state until OUTPUT_ACTIVE
	    while (r31 == 2) {
	      r31 = __R31 & 3;
	      count++;
	    }

	    if (r31 != 3) {
	      update_status(iface, count, r31, STATE_FAULT);
	      fault(iface);
	      continue;
	    }

active:
	    // Now active state
            pwm_magenta();
	    update_status(iface, count, r31, STATE_ACTIVE);

	    while (r31 == 3) {
	      r31 = __R31 & 3;
	      count++;
	    }

	    if (r31 == 2) {
	      // Active signal dropped. Either clear signal blocked read or single-stepping through memory.
	      pwm_blue();
	      update_status(iface, count, r31, STATE_CLEAR_CANCEL);
	      while (r31 == 2) {
	        r31 = __R31 & 3;
	        count++;
	      }
	      if (r31 == 3) {
		// Previous op must have been canceled
	        goto active;
	      } else if (r31 == 0) {
	        // Single-stepped, finally got the STRGAT
	      } else {
	        // Unexpected
		fault(iface);
		continue;
	      }
	    } else if (r31 != 1) {
	      // Unexpected state
	      update_status(iface, count, r31, STATE_FAULT);
	      fault(iface);
	      continue;
	    }

	    // GOT_ADDR 
	    pwm_blue();
	    update_status(iface, count, r31, STATE_GOT_ADDR);

	    // At this point, the address should be good.
	    // Get the address
	    uint32_t address = (gpio0[GPIO_DATAIN] & 0xff00) | ((gpio1[GPIO_DATAIN] >> 12) & 0x00ff);
	    uint16_t data = shared[address];
	    gpio2[GPIO_DATAOUT] = ((uint32_t) (data ^ 0xffff)) << 1; // Sense data starts in bit 1 of gpio2. Flip bits; active-low.
	    iface->lastaddr = address;
	    iface->lastdata = data;
	    log(iface, (address << 16) | data);

	    // Wait for GOT_ADDR drop
	    while (r31 == 1) {
	      r31 = __R31 & 3;
	      count++;
	    }

	    if (r31 != 3) {
	      // Unexpected state
	      update_status(iface, count, r31, STATE_FAULT);
	      fault(iface);
	      goto idle_loop;
	      // continue;
	    }

	    // GOT_ADDR dropped
	    pwm_yellow();
	    update_status(iface, count, r31, STATE_WRITING);

	    while (r31 == 3) {
	      r31 = __R31 & 3;
	      count++;
	    }

	    if (r31 != 2) {
	      // Unexpected state
	      update_status(iface, count, r31, STATE_FAULT);
	      fault(iface);
	      continue;
	    }

	    // Done
	    update_status(iface, count, r31, STATE_DONE);
	}
	pwm_yellow();
}
