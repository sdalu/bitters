Regression tests
================

Each test here corresponds to a defect that was found, reproduced, and
fixed. They are written to fail loudly on the unfixed code, so they are
worth keeping: the input is known and it demonstrably failed once.

Running
-------

    make check                       # no privilege needed
    sudo modprobe gpio-mockup gpio_mockup_ranges=-1,8,-1,8
    sudo make check-gpio             # GPIO tests, on a *virtual* chip
    sudo rmmod gpio-mockup
    make endian                      # i2c bitfield on other-endian ABIs

The GPIO tests drive `gpio-mockup`, a kernel-provided virtual gpiochip
whose lines are driven from `/sys/kernel/debug/gpio-mockup/`. They never
touch real hardware. `make check-gpio` finds the mockup chip itself;
override with `make check-gpio CHIP=gpiochip2`.

`make check-gpio` reports PASS/FAIL per test; a non-zero exit means at
least one failed.

What each test pins down
------------------------

| test | defect it guards against |
|---|---|
| `t_init_repeat` | a second `bitters_gpio_init()` aborting on its own signal handler (built **without** `-DNDEBUG` on purpose; also checks a genuine third-party `SIGUSR1` handler is still detected) |
| `t_irq_sibling_reconfig` | IRQ thread blocking in `read()` under `irq_lock` after a callback reconfigures a sibling pin — exit 42 means it hung |
| `t_starvation` | a busy low-numbered line starving a higher-numbered one |
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
| `t_i2c_endian` | the `read`/`write` bitfield view of `dir` matching the transfer constants |

`support/` holds `LD_PRELOAD` interposers used to reach failure paths the
kernel will not produce on demand: `close_clobber` (a `close()` that
leaves `errno` set), `fcntl_fail` (`F_GETFL` failing), `alloc_fail`
(fails the Nth allocation, selected by `BH_FAIL_AT`).

Not covered here
----------------

* Big-endian behaviour is checked by inspecting cross-compiled codegen
  (`make endian`), not by running — no big-endian host was available.
* `t_starvation` documents the fairness property but did not reproduce
  starvation on a Pi 4: the dispatch loop drains faster than
  `gpio-mockup` can generate edges. It is a guard, not a reproduction.
