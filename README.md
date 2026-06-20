Bitters
=======

Bitters is a portable C glue that brings microcontroller-style APIs to Linux
GPIO, SPI, and I2C interfaces. It uses Linux ioctl calls for both performance
and portability (no devmem, no sysfs).


Key features
------------
* **Familiar API design** modeled after embedded SDK patterns
* **High performance** through direct ioctl calls
* **Raspberry Pi support** with pin mapping included (`bitters/rpi.h`)
* **Full documentation** via Doxygen
* **Apache-2 licensed** (BSD-3-Clause for queue.h)


Requirements
------------
* GNU C library extensions
* Optional thread support for advanced features


Use case
--------
* **Prototyping** hardware designs before committing to custom microcontrollers
* **SDK porting** when migrating device manufacturer code to Linux platforms
* **Education** for developers learning embedded Linux without fighting interface complexity


Why Bitters?
------------
If you've worked with microcontrollers, you know how straightforward hardware
access can be. Manufacturers provide SDKs with consistent APIs: enable a
peripheral, configure it, read or write.

Linux is different. Hardware access has evolved through multiple approaches:

* **Sysfs**: File-based interface that's easy to use but slow. Each GPIO
  operation requires opening files, reading or writing strings, and closing
  files. The overhead makes it unsuitable for time-sensitive applications.

* **devmem**: Direct memory access that's fast but dangerous and non-portable.
  Different kernel versions, different hardware platforms, different memory
  mappings. Your code breaks when you move to a new board.

* **ioctl**: The kernel's standard control interface. Fast, safe, and portable
  across platforms. But the low-level API is verbose and unfamiliar to embedded
  developers.

**Bitters** bridges this gap by wrapping Linux ioctl calls in APIs that match what
you'd find in microcontroller SDKs, providing you with:
**easier code migration**, **reduced learning curve**, **ioctl-style performance**



Build and configuration
=======================

### Basic Compilation

Compile with `-D_GNU_SOURCE` to enable required GNU extensions.

### Thread Support

Add `-DBITTERS_WITH_THREADS` if your application uses threads.

### GPIO IRQ Callbacks

For interrupt callback processing (common when porting device SDKs):
* Compile with `-DBITTERS_WITH_GPIO_IRQ -DBITTERS_WITH_THREADS`
* The library uses `SIGUSR1` internally (can be customized via
  `-DBITTERS_SIGIRQ=SIGNAME`) to notify processing thread of callback setting
  modifications.
* Thread support is required

### Logging & Debug

Enable additional compilation flags to enable assertions and logging.

| Device | Assertion                  | Logging                 |
|--------|----------------------------|-------------------------|
| GPIO   | `BITTERS_GPIO_WITH_ASSERT` | `BITTERS_GPIO_WITH_LOG` |
| SPI    | `BITTERS_SPI_WITH_ASSERT`  | `BITTERS_SPI_WITH_LOG`  |
| I2C    | `BITTERS_I2C_WITH_ASSERT`  | `BITTERS_I2C_WITH_LOG`  |


### Suppress Warnings

Several configuration warnings are emitted at run-time to notify
of common source of misconfiguration.

Warnings can be removed at build-time using compilation flag or at run-time
using environment variable.

| Type of warnings                    | Compilation flag / Env. variable    |
|-------------------------------------|-------------------------------------|
| Raspberry Pi (GPIO pull, I2C speed) | BITTERS_SILENCE_RPI_WARNING         |
| SPI buffer size                     | BITTERS_SILENCE_SPI_BUFSIZE_WARNING |
| All                                 | BITTERS_SILENCE_WARNING             |



Devices
=======
If you need to manipulate device-tree, you can read about it:
https://michael.franzl.name/blog/posts/2016-11-10-setting-i2c-speed-raspberry-pi


GPIO
----
On Linux, the gpio pull strength is considered to be part of the hardware
platform.
It needs to be configured at boot time, using either
* on Raspberry Pi
  *  `pinctrl` command (previously `raspi-gpio`), run `pinctrl help` for details.
     Example for pull up: `pinctrl set _pin_ pu`
  * `config.txt` bootloader config, see rpi documentation `config-txt/gpio.md`
     for details.
	 Example for pull up: adding entry `gpio=_pin-list_=pu`
* device-tree


### API Functions

| **Function**                  | **Description**                                       |
|-------------------------------|-------------------------------------------------------|
| `bitters_gpio_pin_enable()`   | Enable and configure pin                              |
| `bitters_gpio_pin_disable()`  | Disable pin                                           |
| `bitters_gpio_pin_read()`     | Read pin value                                        |
| `bitters_gpio_pin_write()`    | Write pin value                                       |
| `bitters_gpio_irq_wait()`     | Busy wait on interrupt                                |
| `bitters_gpio_irq_callback()` | Register interrupt callback (requires thread support) |



I2C
---
On Linux, the I2C bus speed is considered to be part of the hardware
platform, using a fixed speed based on the lowest common speed of
the I2C devices attached to the bus.

It needs to be configured at boot time, using either:
* on Raspberry Pi
  * `config.txt`: adding the `i2c_arm_baudrate=xxxx` parameter to the
   `dtparam=i2c_arm=on` entry
* modprobe: passing the `baudrate=xxx` parameter to the driver kernel module
* device-tree: the `clock-frequency` parameter found in
  `brcm,bcm2835-i2c` in case of a Raspberry Pi

### API Functions

| **Function**              | **Description**                        |
|---------------------------|----------------------------------------|
| `bitters_i2c_enable()`    | Enable and configure I2C               |
| `bitters_i2c_disable()`   | Disable I2C                            |
| `bitters_i2c_set_speed()` | Set bus speed (not supported on Linux) |
| `bitters_i2c_transfer()`  | Perform I2C transfer                   |


SPI
---
On linux the SPI max transfer size is by default a page size (4096 bytes),
you could/should increase this value by adding the `spidev.bufsiz=65536`
parameter to the kernel. On a Raspberry Pi, this is done in `/boot/cmdline.txt`


### API Functions

| **Function**                 | **Description**            |
|------------------------------|----------------------------|
| `bitters_spi_enable()`       | Enable and configure SPI   |
| `bitters_spi_disable()`      | Disable SPI                |
| `bitters_spi_set_speed()`    | Set bus speed              |
| `bitters_spi_set_wordsize()` | Set word size              |
| `bitters_spi_transfer()`     | Perform SPI transfer       |


Getting started
===============
~~~sh
gcc ${bitters}/src/*.c -I ${bitters}/include .... \
    -D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ -pthread
~~~

The `-DBITTERS_WITH_GPIO_IRQ` flag is only needed if you use
`bitters_gpio_irq_callback()` (as in the example below); it requires
`-DBITTERS_WITH_THREADS`.

Example
-------

~~~c
#include "bitters.h"
#include "bitters/rpi.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"
#include "bitters/delay.h"

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
    .transfer  = BITTERS_SPI_TRANSFER_MSB,
    .word      = BITTERS_SPI_WORDSIZE(8),
    .speed     = 3000000,
  };

  bitters_init();
  bitters_gpio_pin_enable(&reset , &reset_cfg);
  bitters_gpio_pin_enable(&irq ,   &irq_cfg  );
  bitters_gpio_irq_callback(&irq, my_irq_callback, NULL);
  bitters_spi_enable(&spi0, &spi0_cfg);

  bitters_gpio_pin_write(&reset, 1);
  bitters_delay_usec(100);
  bitters_gpio_pin_write(&reset, 0);

  uint8_t data[8];
  const struct bitters_spi_transfer xfr[] = {
    { .tx = "cmd", .len = 3            },
    { .rx = data,  .len = sizeof(data) }
  };
  bitters_spi_transfer(&spi0, xfr, 2);

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
   // Deal with error (interrupted syscall, ...)
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
