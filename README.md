Bitters
=======

Provide access to linux GPIO, SPI using the same kind of API that can
be found for micro-controller. It is using linux ioctl for
portability and performance (no devmem, no sysfs)


* If using threads, this library must be compiled with the 
  `-DBITTERS_WITH_THREADS` flag
* If irq callback processing is required (which is generally the case),
  library require threads support and will internally use `SIGUSR1`
  (which can be changed by defining `BITTERS_SIGIRQ`).
* Full documentation can be generated using doxygen
* the include `bitters/rpi.h` define the pin mapping found on Raspbery Pi
* license is Apache-2 except for queue.h file which is BSD-3-Clause
* extra log and debugging can be enabled by defininig 
  `BITTERS_GPIO_WITH_ASSERT`, `BITTERS_GPIO_WITH_LOG`, 
  `BITTERS_SPI_WITH_ASSERT`, `BITTERS_SPI_WITH_LOG`.

# GPIO
* `bitters_gpio_pin_enable`: enable and configure pin
* `bitters_gpio_pin_disable`: disable pin
* `bitters_gpio_pin_read`: read pin value
* `bitters_gpio_pin_write`: write pin value
* `bitters_gpio_irq_wait`: busy wait on irq
* `bitters_gpio_irq_callback` register irq callback (require thread support)

# SPI
* `bitters_spi_enable`: enable and configure spi
* `bitters_spi_disable`: disable spi
* `bitters_spi_set_speed`: set spi bus speed
* `bitters_spi_transfert`: perform sppi transfert

# Compiling
~~~sh
gcc ${bitters}/src/*.c -I ${bitters}/include .... \
    -DBITTERS_WITH_THREADS -pthread
~~~

# Example

~~~c
#include "bitters.h"
#include "bitters/rpi.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"

void my_irq_callback(bitters_gpio_pin_t *pin, void *args) {
    // irq detected... processing
}

int main() {
  bitters_gpio_pin_t reset = BITTERS_GPIO_PIN_INITIALIZER(BITTERS_RPI_GPIO_CHIP,
                                                          BITTERS_RPI_P1_15);
  bitters_gpio_pin_t irq   = BITTERS_GPIO_PIN_INITIALIZER(BITTERS_RPI_GPIO_CHIP,
                                                          BITTERS_RPI_P1_11);
  bitters_spi_t spi0       = BITTERS_SPI_INITIALIZER(BITTERS_RPI_SPI0, 0);


  struct bitters_gpio_cfg reset_cfg  = {
    .dir       = BITTERS_GPIO_DIR_OUT,
    .defval    = 1,
    .label     = "reset",
  };

  struct bitters_gpio_cfg irq_cfg  = {
    .dir       = BITTERS_GPIO_DIR_INPUT,
    .interrupt = BITTERS_GPIO_INTERRUPT_RISING_EDGE,
    .label     = "irq",
  };

  struct bitters_spi_cfg spi0_cfg = {
    .mode      = BITTERS_SPI_MODE_0,
    .transfert = BITTERS_SPI_TRANSFERT_MSB,
    .word      = BITTERS_SPI_WORDSIZE(8),
    .speed     =  3000000,
  };

  bitters_init();
  bitters_gpio_pin_enable(&reset , &reset_cfg);
  bitters_gpio_pin_enable(&irq ,   &irq_cfg  );
  bitters_gpio_irq_callback(&irq, my_irq_callback, NULL);
  bitters_spi_enable(&spi0, &spi0_cfg);

  bitters_gpio_pin_write(&reset, 0);
  bitters_delay_us(100);
  bitters_gpio_pin_write(&reset, 0);
  
  uint8_t data[8];
  const struct bitters_spi_transfert xfr[] = {
    { .tx = "cmd", .len = 3            },
    { .rx = data,  .len = sizeof(data) }
  };
  bitters_spi_transfert(spi->dev, xfr, 2);

  return 0;
}
~~~
