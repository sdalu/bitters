#ifndef __BITTERS__GPIO__H
#define __BITTERS__GPIO__H

/**
 * @file  gpio.c
 * @brief SPI interface
 *
 * @addtogroup Bitters
 * @{
 */


#include <stddef.h>
#include <stdint.h>


/*== Constants =========================================================*/

/**
 * Input direction for GPIO pin
 */
#define BITTERS_GPIO_DIR_INPUT 				0
/**
 * Ouput direction for GPIO pin
 */
#define BITTERS_GPIO_DIR_OUTPUT				1


/**
 * Use behaviour defined by hardware
 * See: - raspio-gpio  (see: raspi-gpio help)
 *      - device-tree with brcm,pull (see: brcm,bcm2835-gpio.txt)
 *      - config.txt (see: config-txt/gpio.md)
 */
#define BITTERS_GPIO_MODE_DEFAULT			0
/**
 * Configure open drain beahviour for GPIO pin
 */
#define BITTERS_GPIO_MODE_OPEN_DRAIN			1
/**
 * Configure open drain source for GPIO pin
 */
#define BITTERS_GPIO_MODE_OPEN_SOURCE			2


/**
 * Disable interrupt processing
 */
#define BITTERS_GPIO_INTERRUPT_DISABLED			0x0
/**
 * Process interrupt on rising edge
 */
#define BITTERS_GPIO_INTERRUPT_RISING_EDGE		0x1
/**
 * Process interrupt on falling edge
 */
#define BITTERS_GPIO_INTERRUPT_FALLING_EDGE		0x2
/**
 * Process interrupt on raising and falling edge
 */
#define BITTERS_GPIO_INTERRUPT_BOTH_EDGE		0x3


/**
 * GPIO pin not used
 */
#define BITTERS_GPIO_PIN_NONE   			NULL


/**
 * Value to use when polling on gpio
 */
#define BITTERS_GPIO_POLL_EVENTS			POLLPRI | POLLIN



/*== Macros ============================================================*/

/**
 * Initialize a GPIO pin
 * Ex: bitters_gpio_pin_t rst = BITTERS_GPIO_PIN_INITIALIZER(dev, pin);
 */
#define BITTERS_GPIO_PIN_INITIALIZER(_dev, _pin)			\
    {									\
       .id           = (_pin ),						\
       .ctrl_devname = (_dev),						\
       .ctrl         = NULL,						\
       .fd           = -1,						\
   }



/*== Structures ========================================================*/

/* Forward declaration */
struct bitters_gpio_ctrl;
struct bitters_gpio_pin;

typedef void (*bitters_gpio_irq_cb_t)(struct bitters_gpio_pin *pin, void *args);
    

/**
 * GPIO pin configuration.
 */
typedef struct bitters_gpio_cfg {
    uint8_t dir;	/**< gpio direction (input | output) 		*/
    uint8_t mode;	/**< gpio mode (open drain, open source, ...)	*/
    uint8_t interrupt;	/**< interrupt processing 			*/
    char   *label;	/**< informative label for system information	*/
    int     defval;	/**< default value when enabling output 	*/
} bitters_gpio_cfg_t;


/**
 * GPIO pin definition.
 */
typedef struct bitters_gpio_pin {
    const int   id;			/**< pin id			*/
    const char const *ctrl_devname;	/**< controller device name	*/
    /* private */
    uint8_t flags;			// Flags for configuration state
    struct bitters_gpio_ctrl *ctrl;	// Back pointer on controller
    int fd;				// File descriptor for pin
#if defined(BITTERS_WITH_THREADS)
    bitters_gpio_irq_cb_t irq_cb;	// Callback for irq processing
    void *irq_cb_args;			// Data pointer for irq callback
#endif
} bitters_gpio_pin_t;


int bitters_gpio_init(void);

/**
 * Enable the pin according to the selected configuration.
 *
 * @param pin 		pin identification
 * @param cfg		pin configuration
 * @return < 0 in case of error
 */
int bitters_gpio_pin_enable(bitters_gpio_pin_t *pin, bitters_gpio_cfg_t *cfg);

/**
 * Disable the pin.
 *
 * @param pin 		pin identification
 * @return < 0 in case of error
 */

int bitters_gpio_pin_disable(bitters_gpio_pin_t *pin);
/**
 * Read pin value.
 *
 * @param pin 		pin identification
 * @param value		pin value (0=low, 1=high)
 * @return < 0 in case of error
 */

int bitters_gpio_pin_read(bitters_gpio_pin_t *pin, int *value);

/**
 * Write value to the pin.
 *
 * @param pin 		pin identification
 * @param value		pin value (0=low, 1=high)
 * @return < 0 in case of error
 */
int bitters_gpio_pin_write(bitters_gpio_pin_t *pin, int value);

/**
 * Wait for interrupt on pin.
 * @note Undefined behaviour if used with bitters_gpio_irq_callback
 *
 * @param pin 		pin identification
 * @return -EINVAL	if pin was not enabled for interrupt
 * @return < 0 in case of error
 */
int bitters_gpio_irq_wait(bitters_gpio_pin_t *pin);

#if defined(BITTERS_WITH_THREADS) || defined(__DOXYGEN__)
/**
 * Register a callback for processing interrupt on pin.
 * @note Undefined behaviour if used with bitters_gpio_irq_wait
 *
 * @param pin 		pin identification
 * @param cb		callback (use NULL to disable)
 * @param args		argument passed to the callback
 * @return -ENOSYS	if not compiled with thread support
 * @return < 0 in case of error
 */
int bitters_gpio_irq_callback(bitters_gpio_pin_t *pin,
			      bitters_gpio_irq_cb_t cb, void *args);
#endif
/** @} */

#endif
