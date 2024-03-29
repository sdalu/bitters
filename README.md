Bitters
=======

Provide access to linux GPIO, SPI, I2C using the same kind of API that can
be found for micro-controller. It is using linux ioctl for
portability and performance (no devmem, no sysfs)

* Full documentation can be generated using doxygen
* The include `bitters/rpi.h` define the pin mapping found on Raspberry Pi
* License is Apache-2 except for queue.h file which is BSD-3-Clause
* Source require GNU extention to C library, compile with `-D_GNU_SOURCE`
* If using threads, this library must be compiled with the 
  `-DBITTERS_WITH_THREADS` flag
* If irq callback processing is required (which is generally the case
  when porting straight from device manufacturer SDK) you need to
  compile with the `-DBITTERS_WITH_GPIO_IRQ -DBITTERS_WITH_THREADS` flags 
  and add thread support,
  this will also internally use `SIGUSR1` (which can be changed by defining
  `BITTERS_SIGIRQ` to the desired signal).
* Extra log and debugging can be enabled by defininig 
  `BITTERS_{GPIO,SPI,I2C}_WITH_ASSERT`, `BITTERS_{GPIO,SPI,I2C}_WITH_LOG`.
* Warning about Raspberry Pi gpio pull up/down/no configuration or I2C speed
  can be disabled at compile time by defining `BITTERS_SILENCE_RPI_WARNING`,
  or at runtime using the environment variable `BITTERS_SILENCE_RPI_WARNING`.

Devices
=======
If you need to manipulate device-tree, you can read about it:
https://blog.michael.franzl.name/2016/11/10/setting-i2c-speed-raspberry-pi/


GPIO
----
On Linux, the gpio pull strengh is considered to be part of the hardware
platform. 
It need to be configured at boot time, using either
* on Raspberry Pi
  * `raspi-gpio` commande, run `raspi-gpio help` for details.
     Example for pull up: `raspi-gpio set _pin_ pu`
  * `config.txt` bootloader config, see rpi documentation `config-txt/gpio.md`
     for details. 
	 Example for pull up: adding entry `gpio=_pin-list_=pu`
* device-tree

### API
* `bitters_gpio_pin_enable`: enable and configure pin
* `bitters_gpio_pin_disable`: disable pin
* `bitters_gpio_pin_read`: read pin value
* `bitters_gpio_pin_write`: write pin value
* `bitters_gpio_irq_wait`: busy wait on irq
* `bitters_gpio_irq_callback` register irq callback (require thread support)


I2C
---
On Linux, the I2C bus speed is considered to be part of the hardware
platform, using a fixed speed based on the lowest common speed of 
the I2C devices attached to the bus. 
It need to be configured at boot time, using either:
* on Raspberry Pi
  * `config.txt`: adding the `i2c_arm_baudrate=xxxx` parameter to the 
   `dtparam=i2c_arm=on` entry
* modprobe: passing the `baudrate=xxx` parameter to the driver kernel module
* device-tree: the `clock-frequency` parameter found in
  `brcm,bcm2835-i2c` in case of a Raspberry Pi

### API
* `bitters_i2c_enable`: enable and configure i2c
* `bitters_i2c_disable`: disable i2c
* `bitters_i2c_set_speed`: set i2c bus speed (not supported on linux)
* `bitters_i2c_transfert`: perform i2c transfert

SPI
---

### API
* `bitters_spi_enable`: enable and configure spi
* `bitters_spi_disable`: disable spi
* `bitters_spi_set_speed`: set spi bus speed
* `bitters_spi_transfert`: perform spi transfert



Compiling
=========
~~~sh
gcc ${bitters}/src/*.c -I ${bitters}/include .... \
    -D_GNU_SOURCE -DBITTERS_WITH_THREADS -pthread
~~~

Example
=======

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
    .dir       = BITTERS_GPIO_DIR_OUTPUT,
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

  bitters_gpio_pin_write(&reset, 1);
  bitters_delay_us(100);
  bitters_gpio_pin_write(&reset, 0);
  
  uint8_t data[8];
  const struct bitters_spi_transfert xfr[] = {
    { .tx = "cmd", .len = 3            },
    { .rx = data,  .len = sizeof(data) }
  };
  bitters_spi_transfert(&spi0, xfr, 2);

  return 0;
}
~~~


You could also find it easier (and it won't require thread support) to
process interrupt using the unix `poll`/`ppoll` to wait on multiple
events:

~~~c
// Fill the pollfd structure with all the file descriptor
// for which you are waiting for an event
struct pollfd pfds[] = {
    { .fd     = BITTERS_GPIO_IRQ_FD(pin),
      .events = BITTERS_GPIO_POLL_EVENTS },
    ....
}

// Perform the poll request
int rc = poll(pfds, __arraycount(pfds), -1);
if (rc < 0) {
   // Deal with error (interrupted syscal, ...)
   ....
}

// Look if we got an event for our file descriptor
if (BITTERS_GPIO_IRQ_FD(pin) >= 0) {
    for (int i = 0 ; i < __arraycount(pfds) ; i++) {
        if ((pfds[i].fd == BITTERS_GPIO_IRQ_FD(pin)) &&
            (pfds[i].revents != 0)) {
            // Consume the event
            bitters_gpio_irq_wait(pin);
            // Take necessary action
            ....
        }
    }
}
~~~
