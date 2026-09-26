# Regression tests for bitters.
# Portable between GNU make and BSD make (no conditionals, no pattern
# rules, no GNU-only functions).
#
#   make              build everything
#   make tests        run the tests that need no special privilege
#   make tests-gpio   run the GPIO tests (needs root + the gpio-mockup module)
#   make tests-endian check the i2c bitfield view on other-endian targets
#
# These are the suite: they build the test programs and run them. The
# preflight that runs none of bitters' code -- check-manifest.sh and
# check-subset.sh, in this directory -- belongs to `make check` at the top
# of the tree, which is a different phase and a different question.
#
# The GPIO tests drive a *virtual* chip (gpio-mockup); they never touch
# real hardware. They need root to load the module and write debugfs.

CC      ?= cc
CFLAGS  ?= -O1 -g
WARNINGS = -Wall -Wextra -Wundef
WERROR  ?= no
W_yes    = -Werror
W_no     =

CPPFLAGS = -D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ \
           -DBITTERS_WITH_GPIO -DBITTERS_WITH_SPI -DBITTERS_WITH_I2C \
           -I../include -I../src
ALL_CFLAGS = $(CFLAGS) $(WARNINGS) $(W_$(WERROR))
LDLIBS   = -lpthread
GPIOSRC  = ../src/gpio.c

# t_init_repeat is built WITHOUT -DNDEBUG on purpose: it checks that a
# repeated bitters_gpio_init() does not trip the assertion.
# t_pins_dangling #includes ../src/gpio.c itself, for the private struct.
PLAIN   = t_i2c_endian t_delay t_spi_args t_i2c_args t_spi_bufsize t_version
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
# Built the way the difference it tests really arises: the library half
# gets a git part, the consumer half does not (and names no subsystem, so
# bitters.c links alone). $(CPPFLAGS) is not used here for that reason.
GITFAKE = +7.gdeadbee
t_version: t_version.c ../src/bitters.c ../include/bitters/version.h
	$(CC) $(ALL_CFLAGS) -D_GNU_SOURCE -I../include \
	    -DBITTERS_VERSION_GIT='"$(GITFAKE)"' -c -o bitters_version.o \
	    ../src/bitters.c
	$(CC) $(ALL_CFLAGS) -D_GNU_SOURCE -I../include \
	    -DBH_EXPECT_GIT='"$(GITFAKE)"' -o $@ t_version.c bitters_version.o

t_i2c_endian: t_i2c_endian.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -o $@ t_i2c_endian.c

t_delay: t_delay.c ../src/delay.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -o $@ t_delay.c ../src/delay.c $(LDLIBS)

t_spi_args: t_spi_args.c ../src/spi.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ t_spi_args.c ../src/spi.c $(LDLIBS)

# t_spi_bufsize is about a warning, so it must not be built with the flag
# that compiles warnings out.
t_spi_bufsize: t_spi_bufsize.c ../src/spi.c
	$(CC) $(ALL_CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ t_spi_bufsize.c ../src/spi.c $(LDLIBS)

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
tests: $(PLAIN)
	@./t_version
	@BITTERS_SILENCE_WARNING=1 ./t_i2c_endian
	@BITTERS_SILENCE_WARNING=1 ./t_delay
	@BITTERS_SILENCE_WARNING=1 ./t_spi_args
	@BITTERS_SILENCE_WARNING=1 ./t_i2c_args
	@./t_spi_bufsize

tests-gpio: all
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
# Needs a cross-capable clang; override with  make tests-endian CLANG=clang-XX
# Only the struct.anon byte is read: padding differs per ABI (m68k emits a
# single i8, powerpc64 a [7 x i8]), so a looser match would misreport.
ENDIAN_TARGETS ?= x86_64 i386 aarch64 riscv64 \
                  powerpc64 powerpc s390x mips mips64 sparc64 m68k \
                  armeb aarch64_be
tests-endian:
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

.PHONY: all tests tests-gpio tests-endian clean
