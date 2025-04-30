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
#include <linux/version.h>

// extra linux header includes
#include <linux/slab.h>             // memory allocation
#include <asm/io.h>                 // provides arch-specific memory mapping
#include <linux/dma-mapping.h>      // provides memory mapping for DMA peripheral
#include <linux/fs.h>               // file operations
#include <linux/cdev.h>             // character device
#include <linux/platform_device.h>  // platform device
#include <linux/miscdevice.h>       // misc. device interface
#include <linux/uaccess.h>          // user/kernel memory interfacing
#include <linux/delay.h>            // delays

// local includes
#include "log.h"

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
// Broadcom (BCM) defines
#define NUM_GPIO_PINS                       53

// define module information
#define WS2812_MODULE_NAME                  "ws2812"
#define WS2812_GPIO_PIN                     18
#define WS2812_MAX_LEDS                     5
#define WS2812_BITS_PER_LED                 24
#define WS2812_BUFFER_BITS                  8
#define WS2812_RESET_LATCH_BITS             100
#define WS2812_DMA_BUFFER_LEN               WS2812_BUFFER_BITS + (WS2812_MAX_LEDS * WS2812_BITS_PER_LED) + WS2812_RESET_LATCH_BITS

// constants
#define PULSE_BIT_0                         33
#define PULSE_BIT_1                         67
#define DELAY_SHORT                         10
#define DELAY_LONG                          100

// test defines
#define BREATH_STEPS                        200

/**
 * CLOCK/PWM CONFIGURATION
 * 
 * 1. to keep it simple, we should assume 100 "ticks" per data bit sent to the WS2812,
 * so we can configure duty cycle as a percentage
 * 
 * 2. the WS2812 needs 1.25us per bit, so (1.25 us/bit) / (100 ticks/bit) = 12.5ns/tick
 * 
 * 3. this equates to 1/(12.5ns/tick) = 80 megaticks/s = 80 MHz for the clock frequency
 * 
 * 4. divide the source clock by the desired clock to get the divider, so 500/80 = 6.25
 * 
 * 5. Since the PWM uses the clock divider register as a 12.12 fixed-point number, we
 *    can do some bit math to figure this out
 * 
 *      Integer component = (6 << 12) & 0x00FFF000;
 *      Fractional component = ((float)25/(float)100) * (1 << 12)
 * 
 * 6. OR'ing these together gives the register's set value (0x00006400)
 */
#define PWMDIV_REGISTER                     (0x00006500)
#define PWMDIV_REGISTER_BREATHE             (0x00180000)

// BCM base address in physical memory
#define PHY_BASE_ADDRESS                    (0x3F000000)
#define GPIO_BASE_ADDRESS                   (PHY_BASE_ADDRESS + 0x00200000)
#define PWM_BASE_ADDRESS                    (PHY_BASE_ADDRESS + 0x0020C000)
#define CM_BASE_ADDRESS                     (PHY_BASE_ADDRESS + 0x00101000)
#define DMA_BASE_ADDRESS                    (PHY_BASE_ADDRESS + 0x00007000)

// BCM bus addresses
#define BUS_BASE_ADDRESS                    (0x7E000000)
#define PWM_BUS_BASE_ADDRESS                (BUS_BASE_ADDRESS + 0x0020C000)

// BCM peripheral base registers
#define GPIO_REG(offset)                    ((volatile unsigned int *)(((char *)gpio_registers) + (offset)))
#define PWM_REG(offset)                     ((volatile unsigned int *)(((char *)pwm_registers) + (offset)))
#define CM_REG(offset)                      ((volatile unsigned int *)(((char *)cm_registers) + (offset)))
#define DMA_REG(offset)                     ((volatile unsigned int *)(((char *)dma_registers) + (offset)))

// GPIO ====================================================================================
// BCM GPIO offsets
#define GPIO_GPFSEL0_OFFSET                 (0x00000000)
#define GPIO_GPSET0_OFFSET                  (0x0000001C)
#define GPIO_GPSET1_OFFSET                  (0x00000020)
#define GPIO_GPCLR0_OFFSET                  (0x00000028)
#define GPIO_GPCLR1_OFFSET                  (0x0000002C)

// GPFSELn
#define GPIO_GPFSEL_SHIFT(pin)              (((pin) % (10)) * (3))
#define GPIO_GPFSEL_MASK(pin)               ((0x7) << (GPIO_GPFSEL_SHIFT(pin)))
#define GPIO_GPFSEL_VALUE(pin, mode)        (((mode) & (0x7)) << (GPIO_GPFSEL_SHIFT(pin)))
#define GPIO_GPFSEL(pin, mode)              ((GPIO_GPFSEL_MASK(pin)) & ((GPIO_GPFSEL_VALUE(pin, mode))))

// BCM GPIO GPSET
#define GPIO_GPSETN_SHIFT(pin)              (pin)
#define GPIO_GPSETN_MASK(pin)               ((0x1) << (GPIO_GPSETN_SHIFT(pin)))
#define GPIO_GPSETN(pin)                    ((0x1) << (pin))

// BCM GPIO GPCLR
#define GPIO_GPCLRN_SHIFT(pin)              (pin)
#define GPIO_GPCLRN_MASK(pin)               ((0x1) << (GPIO_GPCLRN_SHIFT(pin)))
#define GPIO_GPCLRN(pin)                    ((0x1) << (pin))

// PWM =====================================================================================
// BCM PWM offsets
#define PWM_CTL_OFFSET                      (0x00000000)
#define PWM_DMAC_OFFSET                     (0x00000008)
#define PWM_RNG1_OFFSET                     (0x00000010)
#define PWM_DAT1_OFFSET                     (0x00000014)
#define PWM_FIF1_OFFSET                     (0x00000018)

// BCM PWM CTL
#define PWM_CTL_PWEN1_SHIFT                 (0)
#define PWM_CTL_PWEN1_MASK                  ((0x1) << (PWM_CTL_PWEN1_SHIFT))
#define PWM_CTL_PWEN1(val)                  ((PWM_CTL_PWEN1_MASK) & ((val) << (PWM_CTL_PWEN1_SHIFT)))

#define PWM_CTL_MODE1_SHIFT                 (1)
#define PWM_CTL_MODE1_MASK                  ((0x1) << (PWM_CTL_MODE1_SHIFT))
#define PWM_CTL_MODE1(val)                  ((PWM_CTL_MODE1_MASK) & ((val) << (PWM_CTL_MODE1_SHIFT)))

#define PWM_CTL_SBIT1_SHIFT                 (3)
#define PWM_CTL_SBIT1_MASK                  ((0x1) << (PWM_CTL_SBIT1_SHIFT))
#define PWM_CTL_SBIT1(val)                  ((PWM_CTL_SBIT1_MASK) & ((val) << (PWM_CTL_SBIT1_SHIFT)))

#define PWM_CTL_USEF1_SHIFT                 (5)
#define PWM_CTL_USEF1_MASK                  ((0x1) << (PWM_CTL_USEF1_SHIFT))
#define PWM_CTL_USEF1(val)                  ((PWM_CTL_USEF1_MASK) & ((val) << (PWM_CTL_USEF1_SHIFT)))

#define PWM_CTL_CLRF1_SHIFT                 (6)
#define PWM_CTL_CLRF1_MASK                  ((0x1) << (PWM_CTL_CLRF1_SHIFT))
#define PWM_CTL_CLRF1(val)                  ((PWM_CTL_CLRF1_MASK) & ((val) << (PWM_CTL_CLRF1_SHIFT)))

#define PWM_CTL_MSEN1_SHIFT                 (7)
#define PWM_CTL_MSEN1_MASK                  ((0x1) << (PWM_CTL_MSEN1_SHIFT))
#define PWM_CTL_MSEN1(val)                  ((PWM_CTL_MSEN1_MASK) & ((val) << (PWM_CTL_MSEN1_SHIFT)))

// BCM PWM DMAC
#define PWM_DMAC_ENAB_SHIFT                 (31)
#define PWM_DMAC_ENAB_MASK                  ((0x1) << (PWM_DMAC_ENAB_SHIFT))
#define PWM_DMAC_ENAB(val)                  ((PWM_DMAC_ENAB_MASK) & ((val) << (PWM_DMAC_ENAB_SHIFT)))

// BCM PWM RNG1
#define PWM_RNG1_SHIFT                      (0)
#define PWM_RNG1_MASK                       ((0xFFFFFFFF) << (PWM_RNG1_SHIFT))
#define PWM_RNG1(val)                       ((PWM_RNG1_MASK) & ((val) << (PWM_RNG1_SHIFT)))

// BCM PWM DAT1
#define PWM_DAT1_SHIFT                      (0)
#define PWM_DAT1_MASK                       ((0xFFFFFFFF) << (PWM_DAT1_SHIFT))
#define PWM_DAT1(val)                       ((PWM_DAT1_MASK) & ((val) << (PWM_DAT1_SHIFT)))

// BCM PWM FIF1
#define PWM_FIF1_PWM_FIFO_SHIFT             (0)
#define PWM_FIF1_PWM_FIFO_MASK              ((0xFFFFFFF) << PWM_FIF1_PWM_FIFO_SHIFT)
#define PWM_FIF1_PWM_FIFO(val)              ((PWM_FIF1_PWM_FIFO_MASK) & ((val) << (PWM_FIF1_PWM_FIFO_SHIFT)))

// CM =====================================================================================
// BCM CM constants
#define CM_PASSWD                           (0x5A000000)
#define CM_PASSWD_MASK                      (0xFF000000)

// BCM CM offsets
#define CM_PWMCTL_OFFSET                    (0x000000A0)
#define CM_PWMDIV_OFFSET                   	(0x000000A4)

// BCM CM PWMCTL_SRC
#define CM_PWMCTL_SRC_SHIFT                 (0)
#define CM_PWMCTL_SRC_MASK                  ((0xF) << (CM_PWMCTL_SRC_SHIFT))
#define CM_PWMCTL_SRC(val)                  ((CM_PWMCTL_SRC_MASK) & ((val) << (CM_PWMCTL_SRC_SHIFT)))

// BCM CM PWMCTL_ENAB
#define CM_PWMCTL_ENAB_SHIFT                (4)
#define CM_PWMCTL_ENAB_MASK                 ((0x1) << (CM_PWMCTL_ENAB_SHIFT))
#define CM_PWMCTL_ENAB(val)                 ((CM_PWMCTL_ENAB_MASK) & ((val) << (CM_PWMCTL_ENAB_SHIFT)))

// BCM CM PWMCTL_BUSY
#define CM_PWMCTL_BUSY_SHIFT                (7)
#define CM_PWMCTL_BUSY_MASK                 ((0x1) << (CM_PWMCTL_BUSY_SHIFT))
#define CM_PWMCTL_BUSY(val)                 ((CM_PWMCTL_BUSY_MASK) & ((val) << (CM_PWMCTL_BUSY_SHIFT)))

// BCM CM PWMCTL_MASH
#define CM_PWMCTL_MASH_SHIFT                (9)
#define CM_PWMCTL_MASH_MASK                 ((0x3) << (CM_PWMCTL_MASH_SHIFT))
#define CM_PWMCTL_MASH(val)                 ((CM_PWMCTL_MASH_MASK) & ((val) << (CM_PWMCTL_MASH_SHIFT)))

// BCM CM PWMDIV
#define CM_PWMDIV_SHIFT                     (0)
#define CM_PWMDIV_MASK                      ((0x00FFFFFF) << (CM_PWMDIV_SHIFT))
#define CM_PWMDIV(val)                      ((CM_PWMDIV_MASK) & ((val) << (CM_PWMDIV_SHIFT)))

// DMA =====================================================================================
// BCM DMA constants
#define DMA_CHANNEL                         5
#define DMA_PERMAP_PWM                      5

// BCM DMA5 offsets
#define DMA_CHANNEL_OFFSET                  (0x100 * DMA_CHANNEL)
#define DMA_CHANNEL_BASE_ADDRESS            (DMA_BASE_ADDRESS + DMA_CHANNEL_OFFSET)
#define DMA_CS_OFFSET                       (DMA_CHANNEL_OFFSET + 0x00000000)
#define DMA_CONBLKAD_OFFSET                 (DMA_CHANNEL_OFFSET + 0x00000004)

// BCM DMA CS_ACTIVE
#define DMA_CS_ACTIVE_SHIFT                 (0)
#define DMA_CS_ACTIVE_MASK                  ((0x1) << (DMA_CS_ACTIVE_SHIFT))
#define DMA_CS_ACTIVE(val)                  ((DMA_CS_ACTIVE_MASK) & ((val) << (DMA_CS_ACTIVE_SHIFT)))

// BCM DMA CS_END
#define DMA_CS_END_SHIFT                    (1)
#define DMA_CS_END_MASK                     ((0x1) << (DMA_CS_END_SHIFT))
#define DMA_CS_END(val)                     ((DMA_CS_END_MASK) & ((val) << (DMA_CS_END_SHIFT)))

// BCM DMA CS_RESET
#define DMA_CS_RESET_SHIFT                  (31)
#define DMA_CS_RESET_MASK                   ((0x1) << (DMA_CS_RESET_SHIFT))
#define DMA_CS_RESET(val)                   ((DMA_CS_RESET_MASK) & ((val) << (DMA_CS_RESET_SHIFT)))

// BCM DMA CONBLKAD_SCB_ADDR
#define DMA_CONBLKAD_SCB_SHIFT              (0)
#define DMA_CONBLKAD_SCB_MASK               ((0xFFFFFFFF) << (DMA_CONBLKAD_SCB_SHIFT))
#define DMA_CONBLKAD_SCB(val)               ((DMA_CONBLKAD_SCB_MASK) & ((val) << (DMA_CONBLKAD_SCB_SHIFT)))

// BCM DMA TI_DESTDREQ
#define DMA_TI_DESTDREQ_SHIFT               (6)
#define DMA_TI_DESTDREQ_MASK                ((0x1) << (DMA_TI_DESTDREQ_SHIFT))
#define DMA_TI_DESTDREQ(val)                ((DMA_TI_DESTDREQ_MASK) & ((val) << (DMA_TI_DESTDREQ_SHIFT)))

// BCM DMA TI_SRCINC
#define DMA_TI_SRCINC_SHIFT                 (8)
#define DMA_TI_SRCINC_MASK                  ((0x1) << (DMA_TI_SRCINC_SHIFT))
#define DMA_TI_SRCINC(val)                  ((DMA_TI_SRCINC_MASK) & ((val) << (DMA_TI_SRCINC_SHIFT)))

// BCM DMA TI_PERMAP
#define DMA_TI_PERMAP_SHIFT                 (16)
#define DMA_TI_PERMAP_MASK                  ((0x1F) << (DMA_TI_PERMAP_SHIFT))
#define DMA_TI_PERMAP(val)                  ((DMA_TI_PERMAP_MASK) & ((val) << (DMA_TI_PERMAP_SHIFT)))

/**************************************************************************************
 * TYPEDEFS
 **************************************************************************************/
/**
 * gpfsel_mode_t
 * 
 * Enumeration to control the GPFSEL registers
 */
 typedef enum {
	GPFSEL_INPUT        = 0b000,
	GPFSEL_OUTPUT       = 0b001,
	GPFSEL_ALT0         = 0b100,
	GPFSEL_ALT1         = 0b101,
	GPFSEL_ALT2         = 0b110,
	GPFSEL_ALT3         = 0b111,
	GPFSEL_ALT4         = 0b011,
	GPFSEL_ALT5         = 0b010,
} gpfsel_mode_t;

/**
 * pwmctl_src_t
 * 
 * Enumeration to control the clock source of the CM (clock manager)
 */
typedef enum {
	PWMCTL_GND          = 0b000,
	PWMCTL_OSC          = 0b001,
	PWMCTL_TESTDEBUG0   = 0B010,
	PWMCTL_TESTDEBUG1   = 0b011,
	PWMCTL_PLLA         = 0b100,
	PWMCTL_PLLC         = 0b101,
	PWMCTL_PLLD         = 0b110,
	PWMCTL_HDMIAUX      = 0b111,
} pwmctl_src_t;

/**
 * pwmctl_mash_t
 * 
 * Enumeration to determine the MASH algorithm
 */
typedef enum {
	PWMCTL_MASHINT      = 0b000,
	PWMCTL_MASH1STAGE   = 0b001,
	PWMCTL_MASH2STAGE   = 0b010,
	PWMCTL_MASH3STAGE   = 0b011,
} pwmctl_mash_t;

/**
 * dma_cb_t
 * 
 * Define the structure of a DMA control block (CB) that will be stored in memory
 * According to the datasheet, these are defined in memory - the address of this
 * memory is automatically loaded into the proper registers at the start of a DMA
 * transfer, so I won't need to configure the CB registers directly
 */
typedef struct dma_cb_t {
    uint32_t ti;
    uint32_t source_ad;
    uint32_t dest_ad;
    uint32_t txfr_len;
    uint32_t stride;
    uint32_t nextconbk;
    uint32_t _reserved1; // padding; don't use!
    uint32_t _reserved2; // padding; don't use!
} dma_cb_t;

/**
 * led_t
 * 
 * Defines an LED struct representing a single RGB led
 */
typedef struct led {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} led_t;

/**
 * struct ws2812_dev
 * 
 * Defines the structure of the module's device
 */
struct ws2812_dev {
    // array of LEDs
    led_t leds[WS2812_MAX_LEDS];
    int num_leds;
    int duty_cycle;

    // dma buffer and physical handle
    uint32_t *dma_buffer;
    dma_addr_t dma_buffer_phys;

    // dma control block
    dma_cb_t *dma_cb;
    dma_addr_t cb_phys;

    // misc device
    struct miscdevice mdev;

    // device
    struct device *device;
};

/**************************************************************************************
 * GLOBALS
 **************************************************************************************/
// define the registers for peripherals
static volatile unsigned int *gpio_registers = NULL;
static volatile unsigned int *pwm_registers = NULL;
static volatile unsigned int *cm_registers = NULL;
static volatile unsigned int *dma_registers = NULL;

// for breathing animation; pre-computed table for a sine wave
static const uint8_t breathing_table[200] = {
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
    2,  2,  3,  4,  4,  5,  6,  6,  7,  8,
    9, 10, 11, 12, 13, 14, 15, 16, 18, 19,
   20, 21, 23, 24, 25, 27, 28, 30, 31, 33,
   34, 36, 37, 39, 40, 42, 43, 45, 46, 48,
   49, 51, 53, 54, 56, 57, 59, 60, 62, 63,
   65, 66, 68, 69, 71, 72, 74, 75, 76, 78,
   79, 80, 81, 83, 84, 85, 86, 87, 88, 89,
   90, 91, 92, 93, 93, 94, 95, 95, 96, 97,
   97, 98, 98, 98, 99, 99, 99, 99, 99, 99,
  100, 99, 99, 99, 99, 99, 99, 98, 98, 98,
   97, 97, 96, 95, 95, 94, 93, 93, 92, 91,
   90, 89, 88, 87, 86, 85, 84, 83, 81, 80,
   79, 78, 76, 75, 74, 72, 71, 69, 68, 66,
   65, 63, 62, 60, 59, 57, 56, 54, 53, 51,
   49, 48, 46, 45, 43, 42, 40, 39, 37, 36,
   34, 33, 31, 30, 28, 27, 25, 24, 23, 21,
   20, 19, 18, 16, 15, 14, 13, 12, 11, 10,
    9,  8,  7,  6,  6,  5,  4,  4,  3,  2,
    2,  1,  1,  1,  0,  0,  0,  0,  0,  0
  };

  uint8_t test_colors[5][3] = {
    {0xFF, 0x00, 0x00},  // Red
    {0x00, 0xFF, 0x00},  // Green
    {0x00, 0x00, 0xFF},  // Blue
    {0xFF, 0xFF, 0x00},  // Yellow
    {0xFF, 0x00, 0xFF},  // Magenta
};

/**************************************************************************************
 * FUNCTION PROTOTYPES
 **************************************************************************************/
// module/driver setup
static int ws2812_probe(struct platform_device *pdev);
static int ws2812_remove(struct platform_device *pdev);


// file operations
static int ws2812_open(struct inode *inode, struct file *file);
static ssize_t ws2812_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

// module functions
static int pwm_setduty(int duty);
static void restart_dma_transfer(void);
void encode_leds_to_dma(struct ws2812_dev *dev);
static void log_dma_buffer_for_all_leds(struct ws2812_dev *dev);

# endif /* _WS2812_H_ */