# Regression tests for bitters.
# Portable between GNU make and BSD make (no conditionals, no pattern
# rules, no GNU-only functions).
#
#   make            build everything
#   make check      run the tests that need no special privilege
#   make check-gpio run the GPIO tests (needs root + the gpio-mockup module)
#   make endian     check the i2c bitfield view on other-endian targets
#
# The GPIO tests drive a *virtual* chip (gpio-mockup); they never touch
# real hardware. They need root to load the module and write debugfs.

CC      ?= cc
CFLAGS  ?= -O1 -g
WARNINGS = -Wall -Wextra
WERROR  ?= no
W_yes    = -Werror
W_no     =

CPPFLAGS = -D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ \
           -I../include -I../src
ALL_CFLAGS = $(CFLAGS) $(WARNINGS) $(W_$(WERROR))
LDLIBS   = -lpthread
GPIOSRC  = ../src/gpio.c

# t_init_repeat is built WITHOUT -DNDEBUG on purpose: it checks that a
# repeated bitters_gpio_init() does not trip the assertion.
# t_pins_dangling #includes ../src/gpio.c itself, for the private struct.
PLAIN   = t_i2c_endian t_delay t_spi_args t_i2c_args
GPIO    = t_init_repeat t_irq_delivery t_irq_sibling_reconfig t_pins_dangling \
          t_race_after t_race_enable t_lock_stress t_errno t_c8 t_starvation \
          t_soak t_allocfail t_cross_teardown t_irq_wait t_pin_rw
SUPPORT = support/close_clobber.so support/fcntl_fail.so support/alloc_fail.so

.SUFFIXES:
.SUFFIXES: .c .so

all: $(PLAIN) $(GPIO) $(SUPPORT)

# Most GPIO tests are one source plus gpio.c, with NDEBUG.
.c:
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ $< $(GPIOSRC) $(LDLIBS)

# and the exceptions
t_i2c_endian: t_i2c_endian.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -o $@ t_i2c_endian.c

t_delay: t_delay.c ../src/delay.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -o $@ t_delay.c ../src/delay.c $(LDLIBS)

t_spi_args: t_spi_args.c ../src/spi.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ t_spi_args.c ../src/spi.c $(LDLIBS)

t_i2c_args: t_i2c_args.c ../src/i2c.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ t_i2c_args.c ../src/i2c.c $(LDLIBS)

t_init_repeat: t_init_repeat.c $(GPIOSRC)
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -o $@ t_init_repeat.c $(GPIOSRC) $(LDLIBS)

t_pins_dangling: t_pins_dangling.c $(GPIOSRC)
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ t_pins_dangling.c $(LDLIBS)

.c.so:
	$(CC) $(ALL_CFLAGS) -shared -fPIC -o $@ $< -ldl

# --- running ---------------------------------------------------------
# The mockup chips are discovered in the recipe: $(shell) is GNU-only.
# t_cross_teardown needs a second chip and reports 77 (skip) without one.
check: $(PLAIN)
	@BITTERS_SILENCE_WARNING=1 ./t_i2c_endian
	@BITTERS_SILENCE_WARNING=1 ./t_delay
	@BITTERS_SILENCE_WARNING=1 ./t_spi_args
	@BITTERS_SILENCE_WARNING=1 ./t_i2c_args

check-gpio: all
	@set -- `gpiodetect 2>/dev/null | sed -n 's/^\(gpiochip[0-9]*\) \[gpio-mockup.*/\1/p'`; \
	 chip=$$1; chip2=$$2; \
	 if [ -z "$$chip" ]; then \
	     echo "no gpio-mockup chip: modprobe gpio-mockup gpio_mockup_ranges=-1,8,-1,8" >&2; \
	     exit 1; fi; \
	 if [ -n "$$chip2" ]; then echo "using virtual chip: $$chip and $$chip2"; \
	 else echo "using virtual chip: $$chip  (only one: two-chip tests will skip)"; fi; \
	 fail=0; \
	 for t in $(GPIO); do \
	     printf '  %-26s ' $$t; \
	     BH_CHIP=$$chip BH_CHIP2=$$chip2 BITTERS_SILENCE_WARNING=1 \
	         timeout 120 ./$$t >/dev/null 2>&1; rc=$$?; \
	     if [ $$rc -eq 0 ];  then echo PASS; \
	     elif [ $$rc -eq 77 ]; then echo "SKIP (needs a second mockup chip)"; \
	     else echo "FAIL (exit $$rc)"; fail=1; fi; \
	 done; \
	 exit $$fail

# i2c bitfield view on other ABIs, by inspecting the emitted `dir` byte.
# Needs a cross-capable clang; override with  make endian CLANG=clang-XX
# Only the struct.anon byte is read: padding differs per ABI (m68k emits a
# single i8, powerpc64 a [7 x i8]), so a looser match would misreport.
ENDIAN_TARGETS ?= x86_64 i386 aarch64 riscv64 \
                  powerpc64 powerpc s390x mips mips64 sparc64 m68k \
                  armeb aarch64_be
endian:
	@CLANG=$${CLANG:-`command -v clang22 || command -v clang || true`}; \
	if [ -z "$$CLANG" ]; then \
	    echo "  skipped: no cross-capable clang found (set CLANG=...)"; exit 0; fi; \
	echo "using $$CLANG"; fail=0; for t in $(ENDIAN_TARGETS); do \
	  b=`$$CLANG -target $$t-unknown-linux-gnu -nostdlibinc -ffreestanding \
	       -O0 -S -emit-llvm -I../include endian_probe.c -o - 2>/dev/null \
	     | grep -oE '%struct\.anon \{ i8 -?[0-9]+ \}' \
	     | sed -E 's/.*i8 (-?[0-9]+) \}/\1/' | tr '\n' ' '`; \
	  if [ -z "$$b" ]; then \
	      printf '  %-12s skipped (target not supported by this clang)\n' $$t; \
	  elif [ "$$b" = "2 1 " ]; then \
	      printf '  %-12s read/write dir bytes: %-10s ok\n' $$t "$$b"; \
	  else \
	      printf '  %-12s read/write dir bytes: %-10s MISMATCH (want 2 1)\n' $$t "$$b"; \
	      fail=1; \
	  fi; \
	done; \
	if [ $$fail -ne 0 ]; then exit 1; fi

clean:
	rm -f $(PLAIN) $(GPIO) $(SUPPORT) *.o

.PHONY: all check check-gpio endian clean
