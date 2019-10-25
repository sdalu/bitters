Bitters
=======

Provide access to linux GPIO, SPI using the same kind of API that can
be found for micro-controller. It is using linux ioctl for
portability and performance (no devmem, no sysfs)


# GPIO
* `bitters_gpio_pin_enable`
* `bitters_gpio_pin_disable`
* `bitters_gpio_pin_read`
* `bitters_gpio_pin_write`

# SPI
* `bitters_spi_enable`
* `bitters_spi_set_speed`
* `bitters_spi_transfert`

# Example

~~~
#include "bitters.h"
#include "bitters/rpi.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"

int main() {
  bitters_gpio_pin_t reset = BITTERS_GPIO_PIN_INITIALIZER(BITTERS_RPI_GPIO_CHIP,
	                                                      BITTERS_RPI_P1_15);
  bitters_spi_t spi0       = BITTERS_SPI_INITIALIZER(BITTERS_RPI_SPI0, 0);


  struct bitters_gpio_cfg reset_cfg  = {
    .dir    = BITTERS_GPIO_DIR_OUT,
	.defval = 1,
	.label  = "reset",
  };

  struct bitters_spi_cfg spi0_cfg = {
    .mode      = BITTERS_SPI_MODE_0,
    .transfert = BITTERS_SPI_TRANSFERT_MSB,
    .word      = BITTERS_SPI_WORDSIZE(8),
    .speed     =  3000000,
  };

  bitters_init();
  bitters_gpio_pin_enable(&reset , &reset_cfg);
  bitters_spi_enable(&spi0, &spi0_cfg);

  bitters_gpio_pin_write(&reset, 0);
  
  uint8_t data[8];
  const struct bitters_spi_transfert xfr[] = {
    { .tx = "cmd", .len = 3            },
	{ .rx = data,  .len = sizeof(data) }
  };
  bitters_spi_transfert(spi->dev, xfr, 2);

}
~~~
