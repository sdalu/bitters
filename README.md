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

