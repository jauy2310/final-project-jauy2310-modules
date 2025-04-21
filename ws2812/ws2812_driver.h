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

// Broadcom (BCM) addresses/registers
// BCM base address in physical memory
#define BCM_PHY_BASE_ADDRESS                0x3F000000
#define BCM_GPIO_BASE_ADDRESS               (BCM_PHY_BASE_ADDRESS + 0x00200000)

// BCM GPIO addresses
#define BCM_GPIO_GPFSEL0                    (BCM_GPIO_BASE_ADDRESS + 0x00000000)
#define BCM_GPIO_GPFSEL1                    (BCM_GPIO_BASE_ADDRESS + 0x00000004)
#define BCM_GPIO_GPFSEL2                    (BCM_GPIO_BASE_ADDRESS + 0x00000008)
#define BCM_GPIO_GPFSEL3                    (BCM_GPIO_BASE_ADDRESS + 0x0000000C)
#define BCM_GPIO_GPFSEL4                    (BCM_GPIO_BASE_ADDRESS + 0x00000010)
#define BCM_GPIO_GPFSEL5                    (BCM_GPIO_BASE_ADDRESS + 0x00000014)

#define BCM_GPIO_GPSET0                     (BCM_GPIO_BASE_ADDRESS + 0x0000001C)
#define BCM_GPIO_GPSET1                     (BCM_GPIO_BASE_ADDRESS + 0x00000020)

#define BCM_GPIO_GPCLR0                     (BCM_GPIO_BASE_ADDRESS + 0x00000028)
#define BCM_GPIO_GPCLR1                     (BCM_GPIO_BASE_ADDRESS + 0x0000002C)

/**************************************************************************************
 * GLOBALS/STRUCTS
 **************************************************************************************/
// define an entry into the /proc device
static struct proc_dir_entry *ws2812_proc = NULL;

// define the registers for the GPIO peripheral
static unsigned int *gpio_registers = NULL;

/**************************************************************************************
 * MODULE LOAD/UNLOAD FUNCTION PROTOTYPES
 **************************************************************************************/

# endif /* _WS2812_H_ */