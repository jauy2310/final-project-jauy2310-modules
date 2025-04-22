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
#define NUM_GPIO_PINS                   	53

// define module information
#define WS2812_MODULE_NAME                  "ws2812"
#define WS2812_GPIO_PIN                     18
#define WS2812_CLK_DIV                      62
#define WS2812_CLK_DIV_FRAC                 2048

// BCM base address in physical memory
#define PHY_BASE_ADDRESS                	0x3F000000
#define GPIO_BASE_ADDRESS               	(PHY_BASE_ADDRESS + 0x00200000)
#define PWM_BASE_ADDRESS                	(PHY_BASE_ADDRESS + 0x0000C000)
#define CM_BASE_ADDRESS                 	(PHY_BASE_ADDRESS + 0x00101000)

// BCM peripheral base registers
#define GPIO_REG(offset)                	((unsigned int *)\
                                                (gpio_registers + ((offset) / (sizeof(unsigned int)))))
#define PWM_REG(offset)                 	((unsigned int *)\
                                                (pwm_registers + ((offset) / (sizeof(unsigned int)))))
#define CM_REG(offset)                  	((unsigned int *)\
                                                (cm_registers + ((offset) / (sizeof(unsigned int)))))

// GPIO ====================================================================================
// BCM GPIO offsets
#define GPIO_GPFSEL0_OFFSET					(0x00000000)
#define GPIO_GPSET0_OFFSET					(0x0000001C)
#define GPIO_GPSET1_OFFSET					(0x00000020)
#define GPIO_GPCLR0_OFFSET					(0x00000028)
#define GPIO_GPCLR1_OFFSET					(0x0000002C)

// GPFSELn
#define GPIO_GPFSEL_SHIFT(pin)				(((pin) % (10)) * (3))
#define GPIO_GPFSEL_MASK(pin)				((0x7) << (GPIO_GPFSEL_SHIFT(pin)))
#define GPIO_GPFSEL_VALUE(pin, mode)		(((mode) & (0x7)) << (GPIO_GPFSEL_SHIFT(pin)))
#define GPIO_GPFSEL(pin, mode)				((GPIO_GPFSEL_MASK(pin)) & ((GPIO_GPFSEL_VALUE(pin, mode))))

// BCM GPIO GPSET
#define GPIO_GPSETN_SHIFT(pin)				(pin)
#define GPIO_GPSETN_MASK(pin)				((0x1) << (GPIO_GPSETN_SHIFT(pin)))
#define GPIO_GPSETN(pin)				    ((0x1) << (pin))

// BCM GPIO GPCLR
#define GPIO_GPCLRN_SHIFT(pin)				(pin)
#define GPIO_GPCLRN_MASK(pin)				((0x1) << (GPIO_GPCLRN_SHIFT(pin)))
#define GPIO_GPCLRN(pin)				    ((0x1) << (pin))

// PWM =====================================================================================
// BCM PWM offsets
#define PWM_CTL_OFFSET                      (0x00000000)
#define PWM_DMAC_OFFSET                     (0x00000008)
#define PWM_RNG1_OFFSET                     (0x00000010)
#define PWM_DAT1_OFFSET                     (0x00000014)
#define PWM_FIF1_OFFSET                     (0x00000018)

// BCM PWM CTL
#define PWM_CTL_PWEN1_SHIFT					(0)
#define PWM_CTL_PWEN1_MASK					((0x1) << (PWM_CTL_PWEN1_SHIFT))
#define PWM_CTL_PWEN1(val)					((PWM_CTL_PWEN1_MASK) & ((val) << (PWM_CTL_PWEN1_SHIFT)))

#define PWM_CTL_MODE1_SHIFT					(1)
#define PWM_CTL_MODE1_MASK					((0x1) << (PWM_CTL_MODE1_SHIFT))
#define PWM_CTL_MODE1(val)					((PWM_CTL_MODE1_MASK) & ((val) << (PWM_CTL_MODE1_SHIFT)))

#define PWM_CTL_SBIT1_SHIFT					(3)
#define PWM_CTL_SBIT1_MASK					((0x1) << (PWM_CTL_SBIT1_SHIFT))
#define PWM_CTL_SBIT1(val)					((PWM_CTL_SBIT1_MASK) & ((val) << (PWM_CTL_SBIT1_SHIFT)))

#define PWM_CTL_USEF1_SHIFT					(5)
#define PWM_CTL_USEF1_MASK					((0x1) << (PWM_CTL_USEF1_SHIFT))
#define PWM_CTL_USEF1(val)					((PWM_CTL_USEF1_MASK) & ((val) << (PWM_CTL_USEF1_SHIFT)))

#define PWM_CTL_MSEN1_SHIFT					(7)
#define PWM_CTL_MSEN1_MASK					((0x1) << (PWM_CTL_MSEN1_SHIFT))
#define PWM_CTL_MSEN1(val)					((PWM_CTL_MSEN1_MASK) & ((val) << (PWM_CTL_MSEN1_SHIFT)))

// BCM PWM DMAC
#define PWM_DMAC_ENAB_SHIFT					(31)
#define PWM_DMAC_ENAB_MASK              	((0x1) << (PWM_DMAC_ENAB_SHIFT))
#define PWM_DMAC_ENAB(val)					((PWM_DMAC_ENAB_MASK) & ((val) << (PWM_DMAC_ENAB_SHIFT)))

// BCM PWM RNG1
#define PWM_RNG1_SHIFT						(0)
#define PWM_RNG1_MASK                   	((0xFFFFFFFF) << (PWM_RNG1_SHIFT))
#define PWM_RNG1(val)						((PWM_RNG1_MASK) & ((val) << (PWM_RNG1_SHIFT)))

// BCM PWM DAT1
#define PWM_DAT1_SHIFT						(0)
#define PWM_DAT1_MASK                   	((0xFFFFFFFF) << (PWM_DAT1_SHIFT))
#define PWM_DAT1(val)						((PWM_DAT1_MASK) & ((val) << (PWM_DAT1_SHIFT)))

// BCM PWM FIF1
#define PWM_FIF1_PWM_FIFO_SHIFT				(0)
#define PWM_FIF1_PWM_FIFO_MASK          	((0xFFFFFFF) << PWM_FIF1_PWM_FIFO_SHIFT)
#define PWM_FIF1_PWM_FIFO(val)				((PWM_FIF1_PWM_FIFO_MASK) & ((val) << (PWM_FIF1_PWM_FIFO_SHIFT)))

// CM =====================================================================================
// BCM CM constants
#define CM_PASSWD                       	(0x5A000000)
#define CM_PASSWD_MASK                  	(0xFF000000)

// BCM CM offsets
#define CM_PWMCTL                       	(0x000000A0)
#define CM_PWMDIV                       	(0x000000A4)

// BCM CM PWMCTL_SRC
#define CM_PWMCTL_SRC_SHIFT					(0)
#define CM_PWMCTL_SRC_MASK              	((0xF) << (CM_PWMCTL_SRC_SHIFT))
#define CM_PWMCTL_SRC(val)					((CM_PWMCTL_SRC_MASK) & ((val) << (CM_PWMCTL_SRC_SHIFT)))

// BCM CM PWMCTL_ENAB
#define CM_PWMCTL_ENAB_SHIFT				(4)
#define CM_PWMCTL_ENAB_MASK             	((0x1) << (CM_PWMCTL_ENAB_SHIFT))
#define CM_PWMCTL_ENAB(val)					((CM_PWMCTL_ENAB_MASK) & ((val) << (CM_PWMCTL_ENAB_SHIFT)))

// BCM CM PWMCTL_BUSY
#define CM_PWMCTL_BUSY_SHIFT				(7)
#define CM_PWMCTL_BUSY_MASK             	((0x1) << (CM_PWMCTL_BUSY_SHIFT))
#define CM_PWMCTL_BUSY(val)					((CM_PWMCTL_BUSY_MASK) & ((val) << (CM_PWMCTL_BUSY_SHIFT)))

// BCM CM PWMDIV
#define CM_PWMDIV_MASK                  	((0x00FFFFFF) << (0))

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