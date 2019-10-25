#ifndef __BITTERS__GPIO__H
#define __BITTERS__GPIO__H

#define BITTERS_GPIO_DIR_IN  				0
#define BITTERS_GPIO_DIR_OUT 				1

#define BITTERS_GPIO_MODE_UNSPECIFIED			0
#define BITTERS_GPIO_MODE_OPEN_DRAIN			1
#define BITTERS_GPIO_MODE_OPEN_SOURCE			2



#define BITTERS_GPIO_PIN_NONE   			NULL


#define BITTERS_GPIO_PIN_INITIALIZER(_dev, _pin)			\
    {									\
       .id           = (_pin ),						\
       .ctrl_devname = (_dev),						\
       .ctrl         = NULL,						\
       .fd           = -1,						\
   }




typedef struct bitters_gpio_cfg {
    uint8_t dir;	/* gpio direction (input | output) 		*/
    uint8_t mode;	/* gpio mode (open drain, open source, ...)	*/
    char  *label;	/* informative label for system information	*/
    int    defval;	/* default value when enabling output 		*/
} bitters_gpio_cfg_t;

struct bitters_gpio_ctrl;
typedef struct bitters_gpio_pin {
    int   id;		/* pin id					*/
    char *ctrl_devname; /* controller device name			*/
    /* private */
    struct bitters_gpio_ctrl *ctrl;
    int fd;
} bitters_gpio_pin_t;


int bitters_gpio_init(void);
int bitters_gpio_pin_enable(bitters_gpio_pin_t *pin, bitters_gpio_cfg_t *mode);
int bitters_gpio_pin_disable(bitters_gpio_pin_t *pin);
int bitters_gpio_pin_read(bitters_gpio_pin_t *pin, int *value);
int bitters_gpio_pin_write(bitters_gpio_pin_t *pin, int value);

#endif
