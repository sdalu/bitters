# Bitters

Bitters is a portable C glue that brings microcontroller-style APIs to Linux
GPIO, SPI, and I2C interfaces. It uses Linux ioctl calls for both performance
and portability (no devmem, no sysfs).


## Quick start

Bitters needs a Linux host -- the sources include `linux/*.h` -- and a C
compiler with GNU extensions (`-D_GNU_SOURCE`); thread support is optional.
Build and install the library, with GNU make or BSD make:

```sh
make all                        # libbitters.so
sudo make install PREFIX=/usr   # headers, libbitters.so, bitters.pc
```

Then blink an LED on pin 11 of a Raspberry Pi header:

```c
#include <bitters.h>
#include <bitters/gpio.h>
#include <bitters/delay.h>
#include <bitters/rpi.h>

int main(void)
{
    bitters_gpio_pin_t led = BITTERS_GPIO_PIN_INITIALIZER(BITTERS_RPI_GPIO_CHIP,
                                                          BITTERS_RPI_P1_11);
    bitters_gpio_cfg_t cfg = {
        .dir    = BITTERS_GPIO_DIR_OUTPUT,
        .defval = 0,
        .label  = "led",
    };

    bitters_init();
    bitters_gpio_pin_enable(&led, &cfg);
    for (int i = 0; i < 10; i++) {
        bitters_gpio_pin_write(&led, 1);
        bitters_delay_msec(500);
        bitters_gpio_pin_write(&led, 0);
        bitters_delay_msec(500);
    }
    bitters_gpio_pin_disable(&led);
    return 0;
}
```

```sh
cc -o blink blink.c $(pkg-config --cflags --libs bitters)
./blink
```

Run it as a user that may open `/dev/gpiochip*` (root, or the `gpio` group
on Raspberry Pi OS). `pkg-config` supplies the include path and the
`BITTERS_WITH_*` gates the headers are declared under, so no other flag is
needed to build against the installed library.

To compile the sources into your own program instead of installing a
library, see [Vendoring](#vendoring). The [Examples](#examples) at the end
add an interrupt callback and an SPI transfer to this.


## Key features

* **Familiar API design** modeled after embedded SDK patterns
* **High performance** through direct ioctl calls
* **Raspberry Pi support** with pin mapping included (`bitters/rpi.h`)
* **Full documentation** via Doxygen (`make doc`)
* **Apache-2 licensed** (BSD-3-Clause for queue.h)


## Why Bitters?

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

```text
  ┌──────────────────────────────────────────────────────────┐
  │  your application, or a device SDK ported from an MCU    │
  └────────────────────────────┬─────────────────────────────┘
                               │  bitters_gpio_pin_write(&led, 1)
  ┌────────────────────────────┴─────────────────────────────┐
  │  bitters     gpio.c    spi.c    i2c.c    delay.c         │
  └────────────────────────────┬─────────────────────────────┘
                               │  ioctl()
  ┌────────────────────────────┴─────────────────────────────┐
  │  Linux   /dev/gpiochipN   /dev/spidevB.C   /dev/i2c-N    │
  └────────────────────────────┬─────────────────────────────┘
                               │
                            hardware
```

It fits:

* **Prototyping** hardware designs before committing to custom microcontrollers
* **SDK porting** when migrating device manufacturer code to Linux platforms
* **Education** for developers learning embedded Linux without fighting interface complexity


## Documents

* [DESIGN.md](DESIGN.md) -- why bitters is shaped the way it is
* [CHECKLIST.md](CHECKLIST.md) -- what closes a round of work on it
* [tests/README.md](tests/README.md) -- what each test pins down


## Build and configuration

### Building

The Makefile works with both GNU make and BSD make.

```sh
make                            # the help: a bare `make` builds nothing
make all                        # libbitters.so
make check                      # preflight: is this tree fit to build?
make tests                      # the suite, needing no privilege
sudo make tests-gpio            # the GPIO tests, see tests/README.md
sudo make install PREFIX=/usr   # headers, libbitters.so, bitters.pc
make static                     # libbitters.a, to link into a single program
make doc                        # the Doxygen documentation, into doc/
```

### Vendoring

Compiling the sources directly into your own build is supported too;
there is nothing to configure beyond the feature flags. `make sources`
emits shell variables so a build script can consume them, with absolute
paths so the caller need not know where bitters sits:

```sh
$ make -s sources
BITTERS_SOURCES_CORE='/path/to/bitters/src/bitters.c'
BITTERS_SOURCES_DELAY='/path/to/bitters/src/delay.c'
BITTERS_SOURCES_GPIO='/path/to/bitters/src/gpio.c'
BITTERS_SOURCES_I2C='/path/to/bitters/src/i2c.c'
BITTERS_SOURCES_SPI='/path/to/bitters/src/spi.c'
BITTERS_SOURCES='... all five ...'
BITTERS_SUBSYSTEMS='gpio i2c spi'
BITTERS_INCLUDE='/path/to/bitters/include'
BITTERS_CFLAGS='-D_GNU_SOURCE -I/path/to/bitters/include'
BITTERS_LIBS='-lpthread'
BITTERS_VERSION='1.2.0'
BITTERS_VERSION_GIT='+4.gabcdef0'
```

`BITTERS_VERSION_GIT` is what a build between releases adds to the release;
pass it on with `-DBITTERS_VERSION_GIT` and `bitters_version()` reports it.
It is empty for a release, for a tarball, and for a tree vendored inside
another project's repository.

The per-part lists are there so a build script can take the subsystems it
uses and leave the rest, the same choice `bitters.cmake` offers a CMake
consumer. Both come from `bitters.cmake`, which is the manifest: the
Makefile reads it through `scripts/manifest.sh` rather than keeping a
second copy, so there is no second copy to drift.

```sh
eval "$(make -s -C 3rd/bitters sources)"
cc $BITTERS_CFLAGS -c $BITTERS_SOURCES
```

`BITTERS_CFLAGS` is for compiling **bitters**, not for compiling against
it: only the `.c` files need `-D_GNU_SOURCE`. It carries what compiling
bitters *requires* and nothing else -- the feature selection (`THREADS`,
`GPIO_IRQ`, `ASSERT`, `LOG`) and the `BITTERS_WITH_*` gates for the parts
you took are yours to choose. To build an application against an installed
library, use `pkg-config` (below), which does carry the gates.

For CMake there is `bitters.cmake`, which needs no `make` at all:

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/3rd/bitters/bitters.cmake)
add_library(bitters INTERFACE)
target_include_directories(bitters INTERFACE ${BITTERS_INCLUDE_DIR})
target_compile_definitions(bitters INTERFACE _GNU_SOURCE)
target_sources(bitters INTERFACE ${BITTERS_SOURCES})
```

It sets variables rather than defining a target (`BITTERS_INCLUDE_DIR`
is the `BITTERS_INCLUDE` of `make sources`, and the source lists carry the
same names), so you can compile bitters differently for different targets
-- giving only your threaded program `BITTERS_WITH_THREADS`, say -- and
leave out a subsystem you do not use, through `BITTERS_SOURCES_CORE`,
`_GPIO`, `_SPI`, `_I2C` and `_DELAY`. It also sets `BITTERS_VERSION`,
which it reads from `include/bitters/version.h` -- the release, since a
source list cannot honestly claim to be more than that; a tree built
between releases says so through `bitters_version()` at run time.

Any combination links: the subsystems never reach into one another, and
`CORE` reaches into the ones the build says it has, so `CORE` alone links
too.

Name the subsystems you took, with `-DBITTERS_WITH_GPIO`,
`-DBITTERS_WITH_SPI` and `-DBITTERS_WITH_I2C` -- **on your own files as
well as on `bitters.c`**. Those are what `bitters/spi.h` and its siblings
are gated by, so a subsystem you did not name is not declared either, and a
call to it is a compile error in the file that made it, rather than a
missing symbol at link time or a runtime code a caller might ignore.
Unlike `BITTERS_WITH_THREADS`, they say nothing about how bitters itself
was built -- they say which sources your program carries. See
[DESIGN.md](DESIGN.md) for why that asymmetry is deliberate.

A consumer of an **installed** library did not pick a source list, so
`bitters.pc` hands the three over in its `Cflags` alongside the include
path, and `pkg-config --cflags bitters` is all such a consumer needs.

`_DELAY` has no flag and is simply left out when it is not wanted, because
nothing reaches into it.

`tests/check-subset.sh`, run by `make check`, links the subsets with their
flags so the table cannot quietly stop being true.

The tree builds warning-free with `-Wall -Wextra`; `make WERROR=yes`
turns warnings into errors, which is what CI should use. Those warning
flags are applied by the Makefile itself rather than through `CFLAGS`,
which remains yours to set.

### Version

`bitters/version.h` carries the release, and is the one place it is
written. `bitters.cmake` parses those three lines for `BITTERS_VERSION`,
and the Makefile asks `scripts/manifest.sh`, which parses them too, so
`make version`, `bitters.pc` and the soname cannot disagree with the
header; `make check` holds both parses against what the preprocessor
makes of the same file.
[DESIGN.md](DESIGN.md) says why the number lives in a header.

```c
#include <bitters/version.h>

#if !BITTERS_VERSION_AT_LEAST(1, 1, 0)
#error bitters 1.1.0 or newer is required
#endif

printf("built against %s, running against %s\n",
       BITTERS_VERSION_STRING, bitters_version());
```

| **Macro / call**                              | **What it says**                                          |
|-----------------------------------------------|-----------------------------------------------------------|
| `BITTERS_VERSION_MAJOR` / `_MINOR` / `_PATCH` | the release, as numbers                                   |
| `BITTERS_VERSION_STRING`                      | the release, as `"1.1.1"`                                 |
| `BITTERS_VERSION_NUMBER`                      | the release as one comparable integer -- 1.2.3 is `10203` |
| `BITTERS_VERSION_AT_LEAST(maj, min, pat)`     | for `#if`                                                 |
| `bitters_version()`                           | what the library you linked against is, at run time       |

The macros answer for the **headers**, which is what a `#if` can answer
for. `bitters_version()` answers for the **library**, which for a shared
one is only settled when it is loaded -- and the two differ exactly when
the library was replaced underneath you.

`bitters_version()` also carries the part that no file holds: which commit
a build made between releases came from. A release reports the release
alone, `1.1.1`, and anything else appends SemVer build metadata --
`1.1.1+3.gae9c67b`, three commits past the tag, plus `.dirty` if the
worktree had uncommitted changes. `make version` prints the release and
`make version-full` prints what this tree builds as, the two being equal
exactly when it is a release tree.

The git part is worked out by `scripts/gitversion.sh` and compiled in
(`-DBITTERS_VERSION_GIT`); the release alone goes into the soname and into
`bitters.pc`, which name a release and nothing else. It is empty for a
release, for a tarball, and for a tree vendored inside another project's
repository -- see [DESIGN.md](DESIGN.md) for why that last case matters.

Bumping a release is editing the three numbers in
`include/bitters/version.h`, committing, and `make tag`, which reads the
number out of the header rather than having it typed again -- so the tag
and the header cannot end up saying different things. It refuses on an
uncommitted worktree or an existing tag, and pushes nothing.

```sh
$EDITOR include/bitters/version.h    # the three numbers
git commit -am 'Bump the version to 1.2.1.'
make tag                             # v1.2.1, from the header
git push origin v1.2.1
```

`make tag` refuses an unclean worktree or an existing tag, then asks, then
runs `check` and `tests` before it tags -- so declining costs nothing and
nothing gets tagged that the suite has not passed. `YES=1` answers yes for
a script; a non-interactive run without it declines.

A tag made by hand can still disagree, so `make check` gates the other
direction (`scripts/checktag.sh`): on a tag with a clean worktree -- a
release build, which has no later chance to be wrong -- the header must
say what the tag says, and anywhere else it must be at or ahead of the
nearest tag. Bumped-but-not-yet-tagged is the normal state between
releases and passes; behind a tag that exists does not. It says nothing at
all where the answer would be somebody else's: no git, a tarball, or a
tree vendored inside another project's repository.

### Feature selection

Feature selection is done on the `make` command line; `make help` lists
the flags with their current values:

```sh
make THREADS=yes GPIO_IRQ=yes ASSERT=no LOG=no
```

`make features` reports what a given combination produces, as shell
variables, so a script compiling bitters with a chosen feature set can
consume it the way `make sources` is consumed:

```sh
$ make -s features THREADS=no GPIO_IRQ=no
# THREADS=no GPIO_IRQ=no ASSERT=no LOG=no
BITTERS_FEATURE_CPPFLAGS='-DBITTERS_WITH_GPIO -DBITTERS_WITH_I2C -DBITTERS_WITH_SPI'
BITTERS_FEATURE_LIBS=''
```

The first line is a comment, so the whole output evals; strip the `#` and
it is the `make` command line back again. The names are not
`BITTERS_CFLAGS` and `BITTERS_LIBS`: those come from `make sources` and
say what compiling the sources *requires*, which does not move with the
feature flags. These are this combination's answer, which does.

The feature flags are build-time only. The headers declare the whole API
whatever they are set to, and a function whose feature was not compiled
in returns `-ENOSYS` rather than going missing, so an application needs
no flag to match the library it links against:

```sh
cc -o app app.c $(pkg-config --cflags --libs bitters)
```

`bitters.pc` therefore exports no feature flag, and in particular not
`-D_GNU_SOURCE`: that is needed to compile bitters, not to use it. What it
does carry, beside the include path and the libraries to link, is the
three `BITTERS_WITH_*` subsystem gates -- see
[Vendoring](#vendoring) above.

The flags below configure **bitters itself**, whether you build it with
the Makefile or compile its sources into your own build. None of them is
needed to compile an application against bitters.

### Basic Compilation

Compile the `.c` files with `-D_GNU_SOURCE` to enable required GNU
extensions.

### Thread Support

Build bitters with `-DBITTERS_WITH_THREADS` (`make THREADS=yes`, the
default) for thread-safe operation and for the interrupt callbacks
below. An application does not need the flag itself.

### GPIO IRQ Callbacks

For interrupt callback processing (common when porting device SDKs):
* Build bitters with `-DBITTERS_WITH_GPIO_IRQ -DBITTERS_WITH_THREADS`
  (`make GPIO_IRQ=yes`, the default); thread support is required
* The library uses `SIGUSR1` internally (can be customized via
  `-DBITTERS_SIGIRQ=SIGNAME`) to notify processing thread of callback setting
  modifications.
* Built without it, `bitters_gpio_irq_callback()` is still declared and
  still links; it returns `-ENOSYS`

Callbacks run on a thread bitters starts per gpiochip. It polls a private
copy of the descriptor table, so registering or disabling a pin from
another thread never races the poll, and it releases its lock around your
callback, so the callback may itself enable, disable or re-register pins:

```text
      one interrupt processing thread per gpiochip

      ┌───────────────────────────────────────────┐
  ┌──▸│ copy the descriptor table under irq_lock  │
  │   └─────────────────────┬─────────────────────┘
  │                         ▾
  │   ┌───────────────────────────────────────────┐
  │   │ ppoll() the copy                          │◂── SIGUSR1 when a
  │   └─────────────────────┬─────────────────────┘    registration
  │                         ▾                         changes
  │   ┌───────────────────────────────────────────┐
  │   │ for every ready line that is still        │
  │   │ registered, and still on the descriptor   │
  │   │ we polled:                                │
  │   │     read the event                        │
  │   │     release irq_lock, run the callback,   │
  │   │     retake it                             │
  │   └─────────────────────┬─────────────────────┘
  └─────────────────────────┘
```

### Logging & Debug

Enable additional compilation flags to enable assertions and logging.

| Device | Assertion                  | Logging                 |
|--------|----------------------------|-------------------------|
| GPIO   | `BITTERS_GPIO_WITH_ASSERT` | `BITTERS_GPIO_WITH_LOG` |
| SPI    | `BITTERS_SPI_WITH_ASSERT`  | `BITTERS_SPI_WITH_LOG`  |
| I2C    | `BITTERS_I2C_WITH_ASSERT`  | `BITTERS_I2C_WITH_LOG`  |

Logging goes to `stderr` by default, and assertions use the standard
`assert()`. Both can be redirected by defining the `BITTERS_LOG(fmt, ...)`
and `BITTERS_ASSERT(expr)` macros (on the compiler command line, or before
including `bitters.h`).

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


## Library

`bitters_init()` initializes the library: one call, whatever you use. If
you only use one subsystem you can instead call its dedicated init
function (`bitters_gpio_init()`, `bitters_spi_init()`,
`bitters_i2c_init()`); calling an init twice, or in either order, is fine.

A vendored tree names the subsystems it took with `-DBITTERS_WITH_GPIO` /
`_SPI` / `_I2C`, and `bitters_init()` brings up those and is still the one
call -- see [Vendoring](#vendoring) above.

### API Functions

| **Function**                | **Description**                                                          |
|-----------------------------|--------------------------------------------------------------------------|
| `bitters_init()`            | Initialize the library (all subsystems present)                          |
| `bitters_reduced_latency()` | Reduce IO latency (raise scheduling priority, lock pages in memory)      |
| `bitters_version()`         | The version of the library actually linked against ([Version](#version)) |


## GPIO

On Linux, the gpio pull strength is considered to be part of the hardware
platform.
It needs to be configured at boot time, using either
* on Raspberry Pi
  * `pinctrl` command (previously `raspi-gpio`), run `pinctrl help` for details.
    Example for pull up: `pinctrl set _pin_ pu`
  * `config.txt` bootloader config, see rpi documentation `config-txt/gpio.md`
    for details.
    Example for pull up: adding entry `gpio=_pin-list_=pu`
* device-tree

### Naming the controller

A pin's controller is resolved when the pin is first enabled, and is one
of, or several separated by `|` and tried in order:

* a device name under `/dev`, such as `gpiochip0`;
* a chip label, such as `pinctrl-rp1`: what the driver calls the chip,
  and what `gpiodetect` prints in brackets. The lowest-numbered chip
  carrying it is taken.

A device number is assigned at probe and is not the same on every board
or kernel; a label names the silicon, and a list of labels names a family
of boards. A label and the device name it stands for share one controller,
and a name that resolves to nothing fails the enable with `-ENOENT`.

### Active low

`active_low` in `bitters_gpio_cfg_t` says the line's active state is
physical low: a relay board that closes on a low input, an open-collector
sensor. Everything the API then says about the pin is logical -- a `1`
written, read or given as `defval` is the line low, a rising edge is the
line going low -- and the kernel does the inversion. It is said once, at
enable, instead of at every read and write.

### Raspberry Pi pin names

`bitters/rpi.h` names the 40-pin header so you need not hard-code numbers:
`BITTERS_RPI_P1_11` for a header position, `BITTERS_RPI_BCM_GPIO_17` for a
BCM line, `BITTERS_RPI_SPI0_MOSI` and friends for the peripherals, and
`BITTERS_RPI_GPIO_CHIP` for the controller.

```text
  ┌────────────────────────┬────┬────┬────────────────────────┐
  │       P1 header        │odd │even│  BCM numbering         │
  ├────────────────────────┼────┼────┼────────────────────────┤
  │                    3V3 │ 1  │ 2  │ 5V                     │
  │            SDA1  GPIO2 │ 3  │ 4  │ 5V                     │
  │            SCL1  GPIO3 │ 5  │ 6  │ GND                    │
  │          GPCLK0  GPIO4 │ 7  │ 8  │ GPIO14  TXD0           │
  │                    GND │ 9  │ 10 │ GPIO15  RXD0           │
  │       SPI1_CE1  GPIO17 │ 11 │ 12 │ GPIO18  PWM0 SPI1_CE0  │
  │                 GPIO27 │ 13 │ 14 │ GND                    │
  │                 GPIO22 │ 15 │ 16 │ GPIO23                 │
  │                    3V3 │ 17 │ 18 │ GPIO24                 │
  │      SPI0_MOSI  GPIO10 │ 19 │ 20 │ GND                    │
  │       SPI0_MISO  GPIO9 │ 21 │ 22 │ GPIO25                 │
  │      SPI0_SCLK  GPIO11 │ 23 │ 24 │ GPIO8  SPI0_CE0        │
  │                    GND │ 25 │ 26 │ GPIO7  SPI0_CE1        │
  │           ID_SD  GPIO0 │ 27 │ 28 │ GPIO1  ID_SC           │
  │                  GPIO5 │ 29 │ 30 │ GND                    │
  │                  GPIO6 │ 31 │ 32 │ GPIO12                 │
  │           PWM1  GPIO13 │ 33 │ 34 │ GND                    │
  │      SPI1_MISO  GPIO19 │ 35 │ 36 │ GPIO16  SPI1_CE2       │
  │                 GPIO26 │ 37 │ 38 │ GPIO20  SPI1_MOSI      │
  │                    GND │ 39 │ 40 │ GPIO21  SPI1_SCLK      │
  └────────────────────────┴────┴────┴────────────────────────┘
```

`BITTERS_RPI_GPIO_CHIP` names the header bank by the label its pinctrl
driver gives the chip -- `pinctrl-rp1` on a Pi 5, `pinctrl-bcm2711` on a
Pi 4, `pinctrl-bcm2835` before that -- rather than by device number,
which is assigned at probe and moved when the Pi 5 put the header on its
RP1 southbridge. The alternatives are tried in order and `gpiochip0` is
the last of them, so a chip none of the labels match behaves as every
earlier release did. The line offsets are the same on all of them.
`BITTERS_RPI_GPIO_CHIP` is an alias of `BITTERS_RPI_BCM_GPIO_CHIP`, which
is defined only if it is not already, so a build that knows better can say
so on the compiler command line, as a string literal:
`-DBITTERS_RPI_BCM_GPIO_CHIP='"gpiochip4"'`.

### API Functions

| **Function**                     | **Description**                                       |
|----------------------------------|-------------------------------------------------------|
| `bitters_gpio_pin_enable()`      | Enable and configure pin                              |
| `bitters_gpio_pin_disable()`     | Disable pin                                           |
| `bitters_gpio_pin_read()`        | Read pin value                                        |
| `bitters_gpio_pin_write()`       | Write pin value                                       |
| `bitters_gpio_irq_wait()`        | Blocking wait for interrupt                           |
| `bitters_gpio_irq_fill_pollfd()` | Fill a `pollfd` structure for use with `poll`/`ppoll` |
| `bitters_gpio_irq_callback()`    | Register interrupt callback (requires thread support) |


## I2C

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

If you need to manipulate the device-tree for that, there is a write-up on
[setting the I2C speed on a Raspberry Pi](https://michael.franzl.name/blog/posts/2016-11-10-setting-i2c-speed-raspberry-pi).

### API Functions

| **Function**              | **Description**                        |
|---------------------------|----------------------------------------|
| `bitters_i2c_enable()`    | Enable and configure I2C               |
| `bitters_i2c_disable()`   | Disable I2C                            |
| `bitters_i2c_set_speed()` | Set bus speed (not supported on Linux) |
| `bitters_i2c_transfer()`  | Perform I2C transfer                   |


## SPI

On linux the SPI max transfer size is by default a page size (4096 bytes),
you could/should increase this value by adding the `spidev.bufsiz=65536`
parameter to the kernel. On a Raspberry Pi, this is done in `/boot/cmdline.txt`

### API Functions

| **Function**                 | **Description**          |
|------------------------------|--------------------------|
| `bitters_spi_enable()`       | Enable and configure SPI |
| `bitters_spi_disable()`      | Disable SPI              |
| `bitters_spi_set_speed()`    | Set bus speed            |
| `bitters_spi_set_wordsize()` | Set word size            |
| `bitters_spi_transfer()`     | Perform SPI transfer     |


## Delay

Delay helpers sleep with all signals masked, so the delay is not cut short
by signal delivery. If your application uses threads, compile with
`-DBITTERS_WITH_THREADS` so that only the calling thread's signal mask is
affected.

### API Functions

| **Function**           | **Description**                    |
|------------------------|------------------------------------|
| `bitters_delay_usec()` | Delay for a number of microseconds |
| `bitters_delay_msec()` | Delay for a number of milliseconds |


## Examples

The examples build against an installed library with `pkg-config`, as in
the [Quick start](#quick-start), or straight from the sources:

```sh
gcc ${bitters}/src/*.c -I ${bitters}/include .... \
    -D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ -pthread
```

or let the Makefile hand you the same thing, so the flags cannot drift:

```sh
eval "$(make -s -C ${bitters} sources)"
gcc $BITTERS_CFLAGS $BITTERS_SOURCES .... $BITTERS_LIBS
```

Those feature flags are for the bitters sources compiled in the same
command, not for your own files. `-DBITTERS_WITH_GPIO_IRQ` is only needed
if you use `bitters_gpio_irq_callback()` (as in the example below); it
requires `-DBITTERS_WITH_THREADS`.

### Reset, interrupt callback and SPI transfer

```c
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
```

### Interrupts through `poll()`

You could also find it easier (and it won't require thread support) to
process interrupt using the unix `poll`/`ppoll` to wait on multiple
events. The `pollfd` entry can be filled manually as below, or with the
`bitters_gpio_irq_fill_pollfd()` helper:

```c
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
```
