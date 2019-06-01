// PRU code to read address lines and write to iface->lastaddr
// This version uses OUTPUT_ADDR_OKAY

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

        int toggle = 0;
        pwm_green();
        iface->state = STATE_IDLE;

	while (1) {
	    count = 0;
            // Wait for OUTPUT_ADDR_SET and OUTPUT_ACTIVE
            while (( __R31 & 3) != 1) {
	        count++;
	    }
	    iface->buf[0] = count;

            // At this point, the address should be good.
            // Get the address
            uint32_t address = (gpio0[GPIO_DATAIN] & 0xff00) | ((gpio1[GPIO_DATAIN] >> 12) & 0x00ff);
            iface->lastaddr = address;
            pwm_magenta();

            // Wait for OUTPUT_ACTIVE low indicating the read is done
            while ((__R31 & 3) != 2) {
	        count++;
	    }
	    iface->buf[1] = count;

            iface->state = STATE_IDLE;
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
