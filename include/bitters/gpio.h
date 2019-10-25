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
#define BITTERS_GPIO_DIR_IN  				0
/**
 * Ouput direction for GPIO pin
 */
#define BITTERS_GPIO_DIR_OUT 				1


/**
 * Use behaviour defined by hardware
 * See: - raspio-gpio  (see: raspi-gpio help)
 *      - device-tree with brcm,pull (see: brcm,bcm2835-gpio.txt)
 *      - config.txt (see: config-txt/gpio.md)
 */
#define BITTERS_GPIO_MODE_UNSPECIFIED			0
/**
 * Configure open drain beahviour for GPIO pin
 */
#define BITTERS_GPIO_MODE_OPEN_DRAIN			1
/**
 * Configure open drain source for GPIO pin
 */
#define BITTERS_GPIO_MODE_OPEN_SOURCE			2


/**
 * GPIO pin not used
 */
#define BITTERS_GPIO_PIN_NONE   			NULL




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


/**
 * GPIO pin configuration.
 */
typedef struct bitters_gpio_cfg {
    uint8_t dir;	/**< gpio direction (input | output) 		*/
    uint8_t mode;	/**< gpio mode (open drain, open source, ...)	*/
    char  *label;	/**< informative label for system information	*/
    int    defval;	/**< default value when enabling output 	*/
} bitters_gpio_cfg_t;


/**
 * GPIO pin definition.
 */
typedef struct bitters_gpio_pin {
    int   id;		/**< pin id					*/
    char *ctrl_devname; /**< controller device name			*/
    /* private */
    struct bitters_gpio_ctrl *ctrl;
    int fd;
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

/** @} */

#endif
