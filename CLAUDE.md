# bitters

Microcontroller-style GPIO, SPI and I2C over the Linux ioctl interfaces.

## Where things are

- `README.md` — what it does, how to build it, how to vendor it, the API
- `DESIGN.md` — why it is shaped this way
- `CHECKLIST.md` — what has to be true before a round here is done
- `tests/README.md` — what each test pins down, and what is not covered
- `bitters.cmake` — the source list. It is **not** in the Makefile; a new
  `src/*.c` is added there, and `scripts/manifest.sh` is how make reads it.
- `include/bitters/version.h` — the release, and the only file that holds it

## Gate

Gate: `make check && make tests` — `check` is preflight and runs none of
bitters' code; `tests` builds the test programs and runs them.
`CHECKLIST.md` has the rest of what closes a round, including when
`make tests-gpio` is owed, and it is the one to read before committing.

## Traps

- **`make tests` needs a Linux host** — the sources include `linux/*.h`,
  so the suite does not build anywhere else. `make check` does run
  anywhere, which makes it the useful gate on a non-Linux box; do not read
  a passing `check` as a passing suite.
- **A green `make tests-gpio` may have skipped a test.** `t_cross_teardown`
  exits 77 when there is no second mockup chip; the runner prints SKIP and
  still exits 0. Load the module with two ranges
  (`gpio_mockup_ranges=-1,8,-1,8`) and read the output, not the exit code.
- **`make tests-gpio` needs debugfs mounted, and a stock Raspberry Pi OS
  does not mount it.** The runner now refuses without it; running a test
  binary by hand without it does not fail cleanly -- `t_pin_rw` reports
  mismatches, `t_active_low` hangs in `irq_wait()` -- because the lines
  are driven through `/sys/kernel/debug/gpio-mockup/`.
  `mount -t debugfs none /sys/kernel/debug` first.
- **Run a test binary by hand and it may look like it failed.**
  `tests/Makefile` sets `BITTERS_SILENCE_WARNING=1` for most of them;
  without it they print configuration warnings, which are not failures.
  `t_spi_bufsize` is the deliberate exception — it is *about* a warning.
