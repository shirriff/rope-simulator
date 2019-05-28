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

inline void log(volatile struct iface *iface, uint32_t data) {
      iface->bufend = (iface->bufend + 1) % BUFSIZE;
      iface->buf[iface->bufend] = data;
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
        pwm_green();
        iface->state = STATE_IDLE;

	while (1) {
	    count++;
            // Wait for OUTPUT_ADDR_OKAY
            while (( __R31 & 1) == 0) {
	        count++;
	    }

            // At this point, the address should be good.
            // Get the address
            uint32_t address = (((gpio1[GPIO_DATAIN]) >> 12) & 0xff) | (((gpio0[GPIO_DATAIN]) >> 8) & 0xff);
	    address = count & 0xffff;
            uint16_t data = shared[address];
            iface->state = STATE_GOT_ADDRESS;
	    log(iface, (count << 8) | STATE_GOT_ADDRESS);
	    log(iface, (address << 16) | data);
            iface->lastaddr = address;
            iface->lastdata = data;
            pwm_magenta();

            // Wait for OUTPUT_CLEAR_GATE to go high, or OUTPUT_ADDR_OKAY to go low

            while ((__R31 & 3) == 1) {
	        count++;
	    }

            if ((__R31 & 2) == 0) {
                // CLEAR' should be high to proceed. If not, quit this operation.
                iface->state = STATE_CLEAR_CANCEL;
		log(iface, (count << 8) | STATE_CLEAR_CANCEL);
                pwm_red();
                break;
            }
            // OUTPUT_CLEAR_GATE went high, no clear so read is proceeding.

            iface->state = STATE_WRITING;
	    log(iface, (count << 8) | STATE_WRITING);
            pwm_cyan();

            gpio2[GPIO_DATAOUT] = ((uint32_t) data) << 1; // Sense data starts in bit 1 of gpio2

            // Wait for OUTPUT_ADDR_OKAY low indicating the read is done
            while ((__R31 & 1) == 1) {
	        count++;
	    }

            iface->state = STATE_IDLE;
	    log(iface, (count << 8) | STATE_IDLE);
            // Indicate idle color
            if (toggle) {
              pwm_green();
              toggle = 0;
            } else {
              pwm_blue();
              toggle = 1;
            }
	}
}
