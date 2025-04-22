#ifndef _WS2812_H_
#define _WS2812_H_

/**************************************************************************************
 * INCLUDES
 **************************************************************************************/
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h> // size_t
#include <stdint.h> // uintx_t
#include <stdbool.h>
#endif

// kernel module includes
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

// extra linux header includes
#include <linux/slab.h>             // memory allocation
#include <linux/proc_fs.h>          // expose the /proc filesystem
#include <asm/io.h>                 // provides arch-specific memory mapping

// local includes
#include "log.h"
#include "led.h"

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
// define module information
#define WS2812_MODULE_NAME "ws2812"
#define WS2812_GPIO_PIN 18

// Broadcom (BCM) defines
#define BCM_NUM_GPIO_PINS                   53

// BCM base address in physical memory
#define BCM_PHY_BASE_ADDRESS                0x3F000000
#define BCM_GPIO_BASE_ADDRESS               (BCM_PHY_BASE_ADDRESS + 0x00200000)
#define BCM_GPIO_REG(offset)                ((volatile unsigned int *)\
                                                (gpio_registers + ((offset) / (sizeof(unsigned int)))))

// BCM GPIO offsets
#define BCM_GPIO_GPFSEL0                    (0x00000000)
#define BCM_GPIO_GPFSEL1                    (0x00000004)
#define BCM_GPIO_GPFSEL2                    (0x00000008)
#define BCM_GPIO_GPFSEL3                    (0x0000000C)
#define BCM_GPIO_GPFSEL4                    (0x00000010)
#define BCM_GPIO_GPFSEL5                    (0x00000014)
#define BCM_GPIO_GPFSELN_MASK(pin)          ((0x7) << (((pin) % (10)) * (3)))
#define BCM_GPIO_GPFSELN_SET(mode, pin)     (((0x7) & (mode)) << (((pin) % (10)) * (3)))

#define BCM_GPIO_GPSET0                     (0x0000001C)
#define BCM_GPIO_GPSET1                     (0x00000020)
#define BCM_GPIO_GPSETN_MASK(pin)           ((0x1) << (pin))
#define BCM_GPIO_GPSETN_SET(set, pin)       (((0x1) & (set)) << (pin))            

#define BCM_GPIO_GPCLR0                     (0x00000028)
#define BCM_GPIO_GPCLR1                     (0x0000002C)

/**************************************************************************************
 * TYPEDEFS
 **************************************************************************************/
/**
 * gpfsel_mode_t
 * 
 * Enumeration to control the GPFSEL registers
 */
 typedef enum {
    GPFSEL_INPUT    = 000,
    GPFSEL_OUTPUT   = 001,
    GPFSEL_ALT0     = 100,
    GPFSEL_ALT1     = 101,
    GPFSEL_ALT2     = 110,
    GPFSEL_ALT3     = 111,
    GPFSEL_ALT4     = 011,
    GPFSEL_ALT5     = 010,
} gpfsel_mode_t;

/**************************************************************************************
 * GLOBALS
 **************************************************************************************/
// define an entry into the /proc device
static struct proc_dir_entry *ws2812_proc = NULL;

// define the registers for the GPIO peripheral
static unsigned int *gpio_registers = NULL;

/**************************************************************************************
 * FUNCTION PROTOTYPES
 **************************************************************************************/

# endif /* _WS2812_H_ */