Regression tests
================

Each test here corresponds to a defect that was found, reproduced, and
fixed. They are written to fail loudly on the unfixed code, so they are
worth keeping: the input is known and it demonstrably failed once.

Running
-------

    make tests                       # no privilege needed, touches no bus
    sudo modprobe gpio-mockup gpio_mockup_ranges=-1,8,-1,8
    sudo mount -t debugfs none /sys/kernel/debug   # if not already mounted
    sudo make tests-gpio             # GPIO tests, on a *virtual* chip
    sudo rmmod gpio-mockup
    make tests-endian                # i2c bitfield on other-endian ABIs

They run from the top of the tree under the same names, and they are the
suite. The preflight that runs none of bitters' code — `check-manifest.sh`
and `check-subset.sh`, both in this directory — belongs to `make check`,
which is a different phase and a different question.

The GPIO tests drive `gpio-mockup`, a kernel-provided virtual gpiochip
whose lines are driven from `/sys/kernel/debug/gpio-mockup/`, so debugfs
has to be mounted -- a stock Raspberry Pi OS does not -- and the runner
refuses to start without it, because a test that cannot drive its line
does not fail: `t_pin_rw` reports mismatches and `t_active_low` waits for
an edge that never comes. They never touch real hardware. `make tests-gpio` finds the mockup chip itself, by
asking `gpiodetect`, and hands it to each test as `BH_CHIP` — and a second
one, where there is one, as `BH_CHIP2`. There is no override: to run
against a particular chip, set those in the environment and run the test
binary directly.

`make tests-gpio` reports PASS/FAIL per test; a non-zero exit means at
least one failed.

What each test pins down
------------------------

| test | defect it guards against |
|---|---|
| `t_init_repeat` | a second `bitters_gpio_init()` aborting on its own signal handler (built **without** `-DNDEBUG` on purpose; also checks a genuine third-party `SIGUSR1` handler is still detected) |
| `t_irq_sibling_reconfig` | IRQ thread blocking in `read()` under `irq_lock` after a callback reconfigures a sibling pin — exit 42 means it hung |
| `t_starvation` | a busy low-numbered line starving a higher-numbered one |
| `t_irq_wait` | the documented poll-it-yourself path — a pin with no callback registered must keep blocking `irq_wait()` semantics, which the `O_NONBLOCK` toggle for callback pins must not leak into |
| `t_irq_delivery` | plain edge delivery, 6/6 — the regression guard for all of the above |
| `t_pins_dangling` | `pin_disable()` leaving a released pin in the controller table (white-box: includes `../src/gpio.c`) |
| `t_race_after` | using a pin after two threads raced on it — must not crash, invariant must hold |
| `t_race_enable` | controller/descriptor leak after concurrent `pin_enable()` on one pin |
| `t_cross_teardown` | two controllers released from each other's callback — their IRQ threads joining each other. Needs a **second** mockup chip (`gpio_mockup_ranges=-1,8,-1,8`); reports SKIP without one. Detects the hang by counting leaked `/dev/gpiochip` descriptors, because the main thread never blocks |
| `t_lock_stress` | deadlock between the pins lock and controller teardown (60 teardowns with callbacks reconfiguring siblings); exit 43 means deadlock |
| `t_c8` | registering an IRQ callback when the non-blocking toggle cannot be applied — must refuse, not silently install |
| `t_errno` | `errno` surviving the cleanup in `_bitters_gpio_ctrl_create()` |
| `t_soak` | descriptor leak and `O_NONBLOCK` restore over 400 enable/disable cycles |
| `t_allocfail` | allocation-failure paths in controller creation leaking descriptors |
| `t_pin_rw` | `pin_read()` / `pin_write()`: an input line driven from debugfs and read through the API, an output line written through the API and observed on the chip, including `defval` and a non-1 truth value |
| `t_ctrl_lookup` | the controller name resolving to a device: a chip label (asked of the kernel, not assumed) enables a pin, a list of alternatives takes the first that exists, a name that is nothing fails with `-ENOENT`, a label and the device name it stands for share one controller, and the scan leaks no descriptor. The Pi 5 defect -- a fixed `gpiochip0` driving the wrong controller -- is what the label route exists for |
| `t_active_low` | `cfg.active_low` inverting the line for both directions, read and write, `defval`, and the edges `irq_wait()` reports -- all logical, the debugfs line physical. Dropping the flag, or applying it to one direction only, fails here |
| `t_spi_args` | SPI argument validation — the `SPI_IOC_MESSAGE` 14-bit count bound (511 passes, 512 does not), the 32-bit `len` field, `set_speed`/`set_wordsize`, the enable/truncation error paths, and `.tx` taking a const buffer without a cast (a `_Static_assert`, so it fails the build, not a run). Touches no SPI bus |
| `t_i2c_args` | I2C argument validation — `I2C_RDWR_IOCTL_MAX_MSGS`, the 16-bit `len` field, the direction switch (neither/both rejected), and the enable/truncation error paths. Touches no I2C bus |
| `t_delay` | a delay being cut short by an ordinary signal, and the signal mask not being restored afterwards |
| `t_version` | the two version answers coming apart: `BITTERS_VERSION_STRING` (the headers, settled when the caller compiled) against `bitters_version()` (the library, which carries the git part of a build made between releases). Built the way the difference really arises — the library half gets a `-DBITTERS_VERSION_GIT`, the consumer half does not — so a leak of the macro into a consumer's view, or a lost git part, fails here |
| `check-manifest.sh` | `bitters.cmake` and `include/bitters/version.h` no longer describing the tree: a part or a source missing from one of them, an ungated subsystem, a public header the Makefile does not install, either parse of the version — the manifest's awk and `bitters.cmake`'s own regex, the latter asked through `cmake` — disagreeing with what the *preprocessor* makes of it, or the Makefile going back to keeping its own source list |
| `check-subset.sh` | the vendoring subsets in `bitters.cmake` and the README: each one has to link, and a call into a subsystem that was not named has to be refused at compile time |
| `check-features.sh` | `make features` no longer being the interface the README documents: output that is not shell, either name gone from it, a name `make sources` already claims, flags that do not move when a knob is turned off, or a comment line that does not replay to the same answer |
| `t_i2c_endian` | the `read`/`write` bitfield view of `dir` matching the transfer constants |

`support/` holds `LD_PRELOAD` interposers used to reach failure paths the
kernel will not produce on demand: `close_clobber` (a `close()` that
leaves `errno` set), `fcntl_fail` (`F_GETFL` failing), `alloc_fail`
(fails the Nth allocation, selected by `BH_FAIL_AT`).

Not covered here
----------------

* Big-endian behaviour is checked by inspecting cross-compiled codegen
  (`make tests-endian`), not by running — no big-endian host was
  available.
* A real SPI or I2C **data transfer** is never executed: that would mean
  driving a live bus, which on a development board may have hardware
  attached. `t_spi_args` and `t_i2c_args` cover every path that returns
  before the descriptor is used, plus the error paths; the ioctl itself
  is reached only against a closed descriptor (`-EBADF`), which proves
  validation passed but not that the transfer works.
* `bitters_init()` and `bitters_reduced_latency()` are untested; the
  latter changes scheduling policy and locks memory, so it needs root
  and affects the machine it runs on.
* `t_starvation` documents the fairness property but did not reproduce
  starvation on a Pi 4: the dispatch loop drains faster than
  `gpio-mockup` can generate edges. It is a guard, not a reproduction.
