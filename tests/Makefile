# Regression tests for bitters.  Requires GNU make (pattern rules).
#
#   make            build everything
#   make check      run the tests that need no special privilege
#   make check-gpio run the GPIO tests (needs root + the gpio-mockup module)
#   make endian     check the i2c bitfield view on other-endian targets
#
# The GPIO tests drive a *virtual* chip (gpio-mockup); they never touch
# real hardware. They need root to load the module and write debugfs.

CC      ?= cc
CFLAGS  ?= -O1 -g -Wall -Wextra
CPPFLAGS = -D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ \
           -I../include -I../src
LDLIBS   = -lpthread
SRC      = ../src/gpio.c ../src/spi.c ../src/i2c.c ../src/bitters.c ../src/delay.c

# t_init_repeat must be built WITHOUT -DNDEBUG: it checks that a repeated
# bitters_gpio_init() does not trip the assertion.
ASSERTED = t_init_repeat
# t_pins_dangling #includes ../src/gpio.c itself for the private struct.
SELFCONTAINED = t_pins_dangling

PLAIN = t_i2c_endian t_delay
GPIO  = t_init_repeat t_irq_delivery t_irq_sibling_reconfig t_pins_dangling \
        t_race_after t_race_enable t_lock_stress t_errno t_c8 t_starvation \
        t_soak t_allocfail t_cross_teardown t_irq_wait
SUPPORT = support/close_clobber.so support/fcntl_fail.so support/alloc_fail.so

all: $(PLAIN) $(GPIO) $(SUPPORT)

t_i2c_endian: t_i2c_endian.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

t_delay: t_delay.c ../src/delay.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

t_init_repeat: t_init_repeat.c ../src/gpio.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

t_pins_dangling: t_pins_dangling.c ../src/gpio.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ $< $(LDLIBS)

t_%: t_%.c ../src/gpio.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DNDEBUG -o $@ $^ $(LDLIBS)

support/%.so: support/%.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< -ldl

# --- running ---------------------------------------------------------
# First and second gpio-mockup chips. t_cross_teardown needs two; it
# reports 77 (skip) when CHIP2 is absent.
MOCKUPS := $(shell gpiodetect 2>/dev/null | sed -n 's/^\(gpiochip[0-9]*\) \[gpio-mockup.*/\1/p')
CHIP    ?= $(word 1,$(MOCKUPS))
CHIP2   ?= $(word 2,$(MOCKUPS))

check: t_i2c_endian t_delay
	./t_i2c_endian
	./t_delay

check-gpio: all
	@test -n "$(CHIP)" || { echo "no gpio-mockup chip: modprobe gpio-mockup gpio_mockup_ranges=-1,8,-1,8"; exit 1; }
	@echo "using virtual chip: $(CHIP)$(if $(CHIP2), and $(CHIP2),  (only one: two-chip tests will skip))"
	@fail=0; for t in $(GPIO); do \
	    printf '  %-26s ' $$t; \
	    BH_CHIP=$(CHIP) BH_CHIP2=$(CHIP2) BITTERS_SILENCE_WARNING=1 \
	        timeout 120 ./$$t >/dev/null 2>&1; rc=$$?; \
	    if [ $$rc -eq 0 ];  then echo PASS; \
	    elif [ $$rc -eq 77 ]; then echo "SKIP (needs a second mockup chip)"; \
	    else echo "FAIL (exit $$rc)"; fail=1; fi; \
	done; exit $$fail

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
