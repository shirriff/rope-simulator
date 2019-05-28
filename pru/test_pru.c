// Test reading through the PRU
#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"

// 64K of reserved RAM for sharing with ARM
// #define SHARED ((volatile void *)(0x9fdf0000))
#define SHARED ((volatile void *)(0x81400000))
#define GPIO0	0x44e07000		// GPIO Bank 0  See Table 2.2 of TRM <1>
#define GPIO1	0x4804c000		// GPIO Bank 1
#define GPIO2	0x481ac000		// GPIO Bank 2
#define GPIO3	0x481ae000		// GPIO Bank 3
#define GPIO_CLEARDATAOUT	0x190	// For clearing the GPIO registers
#define GPIO_SETDATAOUT		0x194	// For setting the GPIO registers
#define GPIO_DATAIN		0x138	// For reading the GPIO registers
#define GPIO_DATAOUT		0x13c	// For writing the GPIO registers
#define P9_11	(0x1<<30)			// Bit position tied to P9_11

volatile register uint32_t __R30;
volatile register uint32_t __R31;

#define PWM00 ((volatile uint16_t *)0x48300212) // P9_22
#define PWM01 ((volatile uint16_t *)0x48300214) // P9_21
#define PWM21 ((volatile uint16_t *)0x48302214) // P8_34

void main(void)
{
	volatile uint32_t *gpio0 = (uint32_t *)GPIO0;
	volatile uint32_t *gpio1 = (uint32_t *)GPIO1;
	volatile uint32_t *gpio2 = (uint32_t *)GPIO2;
	volatile uint32_t *gpio3 = (uint32_t *)GPIO3;
	volatile uint32_t *ptr = (uint32_t *)SHARED;
	// Turn on PWM
	*PWM00 = 0x63;
	*PWM01 = 0x63;
	*PWM21 = 0x63;
	uint8_t addr_init = 0;
	uint16_t oldaddr;
	uint8_t oldmisc; // addr_okay << 1 | clear
	uint16_t sense = 0xffff;
	while (1) {
	    // Read inputs. Flash light on any change. Write to shared mem.

	    uint16_t addr = (gpio0[GPIO_DATAIN/4] & 0xff00) | ((gpio1[GPIO_DATAIN/4] >> 12) & 0xff);
	    uint8_t misc = __R31 & 3;
	    ptr[0] = addr;
	    ptr[1] = misc;

	    // Blink response
	    if (!addr_init) {
	      oldaddr = addr; // Initialize
	      addr_init = 1;
	    }
	    if ((oldaddr & 0xff) != (addr & 0xff)) {
	      *PWM00 = 0xc350;
	      *PWM01 = 0;
	      *PWM21 = 0;
	      __delay_cycles(10000000);
	    } else if (oldaddr != addr) {
	      *PWM00 = 0;
	      *PWM01 = 0xc350;
	      *PWM21 = 0;
	      __delay_cycles(10000000);
	    } else if (oldmisc != misc) {
	      *PWM00 = 0;
	      *PWM01 = 0;
	      *PWM21 = 0xc350;
	      __delay_cycles(10000000);
	    }
	    oldaddr = addr;
	    oldmisc = misc;
	    *PWM00 = 0;
	    *PWM01 = 0;
	    *PWM21 = 0;

	    // Sequence through sense outputs, 4 bit chunks

	    gpio2[GPIO_DATAOUT / 4] = (sense << 1) | (sense << 5) | (sense << 9) | (sense << 13);
	    sense = (sense + 1) & 0xf;
	}
}
