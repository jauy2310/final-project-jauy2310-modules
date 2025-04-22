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
// Broadcom (BCM) defines
#define BCM_NUM_GPIO_PINS                   53

// define module information
#define WS2812_MODULE_NAME                  "ws2812"
#define WS2812_GPIO_PIN                     18
#define WS2812_CLK_DIV                      62
#define WS2812_CLK_DIV_FRAC                 2048

// BCM base address in physical memory
#define BCM_PHY_BASE_ADDRESS                0x3F000000
#define BCM_GPIO_BASE_ADDRESS               (BCM_PHY_BASE_ADDRESS + 0x00200000)
#define BCM_PWM_BASE_ADDRESS                (BCM_PHY_BASE_ADDRESS + 0x0000C000)
#define BCM_CM_BASE_ADDRESS                 (BCM_PHY_BASE_ADDRESS + 0x00101000)

// BCM peripheral base registers
#define BCM_GPIO_REG(offset)                ((unsigned int *)\
                                                (gpio_registers + ((offset) / (sizeof(unsigned int)))))
#define BCM_PWM_REG(offset)                 ((unsigned int *)\
                                                (pwm_registers + ((offset) / (sizeof(unsigned int)))))
#define BCM_CM_REG(offset)                  ((unsigned int *)\
                                                (cm_registers + ((offset) / (sizeof(unsigned int)))))

// GPIO ====================================================================================
// BCM GPIO offsets
#define BCM_GPIO_GPFSEL0                    (0x00000000)
#define BCM_GPIO_GPFSELN_MASK(pin)          ((0x7) << (((pin) % (10)) * (3)))
#define BCM_GPIO_GPFSELN_SET(mode, pin)     (((0x7) & (mode)) << (((pin) % (10)) * (3)))

// BCM GPIO GPSET
#define BCM_GPIO_GPSET0                     (0x0000001C)
#define BCM_GPIO_GPSET1                     (0x00000020)
#define BCM_GPIO_GPSETN_MASK(pin)           ((0x1) << (pin))
#define BCM_GPIO_GPSETN_SET(set, pin)       (((0x1) & (set)) << (pin))            

// BCM GPIO GPCLR
#define BCM_GPIO_GPCLR0                     (0x00000028)
#define BCM_GPIO_GPCLR1                     (0x0000002C)
#define BCM_GPIO_GPCLRN_MASK(pin)           ((0x1) << (pin))
#define BCM_GPIO_GPCLRN_SET(set, pin)       (((0x1) & (set)) << (pin)) 

// PWM =====================================================================================
// BCM PWM offsets
#define BCM_PWM_CTL                         (0x00000000)
#define BCM_PWM_DMAC                        (0x00000008)
#define BCM_PWM_RNG1                        (0x00000010)
#define BCM_PWM_DAT1                        (0x00000014)
#define BCM_PWM_FIF1                        (0x00000018)

// BCM PWM CTL
#define BCM_PWM_CTL_PWEN1_MASK              ((0x1) << (0))
#define BCM_PWM_CTL_MODE1_MASK              ((0x1) << (1))
#define BCM_PWM_CTL_SBIT1_MASK              ((0x1) << (3))
#define BCM_PWM_CTL_USEF1_MASK              ((0x1) << (5))
#define BCM_PWM_CTL_MSEN1_MASK              ((0x1) << (7))

// BCM PWM DMAC
#define BCM_PWM_DMAC_ENAB_MASK              ((0x1) << (31))

// BCM PWM RNG1
#define BCM_PWM_RNG1_MASK                   ((0xFFFFFFFF) << (0))

// BCM PWM DAT1
#define BCM_PWM_DAT1_MASK                   ((0xFFFFFFFF) << (0))

// BCM PWM FIF1
#define BCM_PWM_FIF1_PWM_FIFO_MASK          ((0xFFFFFFFF) << (0))

// CM =====================================================================================
// BCM CM constants
#define BCM_CM_PASSWD                       (0x5A000000)
#define BCM_CM_PASSWD_MASK                  (0xFF000000)

// BCM CM offsets
#define BCM_CM_PWMCTL                       (0x000000A0)
#define BCM_CM_PWMDIV                       (0x000000A4)

// BCM CM PWMCTL
#define BCM_CM_PWMCTL_SRC_MASK              ((0xF) << (0))
#define BCM_CM_PWMCTL_ENAB_MASK             ((0x1) << (4))
#define BCM_CM_PWMCTL_BUSY_MASK             ((0x1) << (7))

// BCM CM PWMDIV
#define BCM_CM_PWMDIV_MASK                  ((0x00FFFFFF) << (0))

// MISC ===================================================================================

// write a register field
#define REG_WRITE_FIELD(addr, mask, val)    (*(addr) = (*(addr) & ~(mask)) | ((val) & (mask)))


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
static unsigned int *pwm_registers = NULL;
static unsigned int *cm_registers = NULL;

/**************************************************************************************
 * FUNCTION PROTOTYPES
 **************************************************************************************/

# endif /* _WS2812_H_ */