#!/bin/sh
# bitters.cmake and the README say a vendored tree can take the subsystems
# it uses and leave the rest, naming what it took with
# -DBITTERS_WITH_<SUBSYSTEM>. Two properties hold that up, and both live in
# what the sources do *not* do -- which a build of the whole library cannot
# notice losing, and the consumer who can is somebody else:
#
#   the subsets link      (no source reaches into one it did not take)
#   what was not named does not compile   (the headers are gated by the
#                                          same macros bitters.c is)
#
# Compile and link only; nothing is run, so no device and no privilege is
# needed. Run by `make check`.
set -e
top=`dirname "$0"`/..
cc=${CC:-cc}
tmp=`mktemp -d`
trap 'rm -rf "$tmp"' EXIT INT TERM
bad=0
BASE="-D_GNU_SOURCE -DBITTERS_WITH_THREADS -DBITTERS_WITH_GPIO_IRQ"

G=-DBITTERS_WITH_GPIO
S=-DBITTERS_WITH_SPI
I=-DBITTERS_WITH_I2C
ALL="$G $S $I"

# --- the subsets link --------------------------------------------------
try() {
    name=$1; srcs=$2; flags=$3; body=$4
    cat > "$tmp/main.c" <<EOF
#include <bitters.h>
#include <bitters/gpio.h>
#include <bitters/spi.h>
#include <bitters/i2c.h>
#include <bitters/delay.h>
int main(void) { $body return 0; }
EOF
    # Turn the source names into paths. $srcs is split on purpose: sh has
    # no arrays, so the positional parameters are the list, and each name
    # is appended as a path while the bare name is shifted off the front.
    # $flags likewise, and is empty for a subset that names nothing.
    set -- $srcs
    for s in "$@"; do set -- "$@" "$top/src/$s"; shift; done
    if $cc $BASE $flags -I"$top/include" -o "$tmp/a.out" "$tmp/main.c" "$@" \
           -lpthread > "$tmp/log" 2>&1
    then
        printf '  %-40s links\n' "$name"
    else
        printf '  %-40s FAILED TO LINK\n' "$name"
        sed 's/^/      /' "$tmp/log"
        bad=1
    fi
}

# The subsystems, alone: each stands without the others and without core
try "gpio alone"            "gpio.c"  "$G"     "bitters_gpio_init();"
try "spi alone"             "spi.c"   "$S"     "bitters_spi_init();"
try "i2c alone"             "i2c.c"   "$I"     "bitters_i2c_init();"
try "delay alone"           "delay.c" ""       "bitters_delay_msec(0);"
try "gpio + spi"            "gpio.c spi.c" "$G $S" \
    "bitters_gpio_init(); bitters_spi_init();"

# ... and core, which calls the init of each subsystem this build names
try "core + gpio"           "bitters.c gpio.c" "$G" \
    "bitters_init(); bitters_reduced_latency();"
try "core + gpio + spi"     "bitters.c gpio.c spi.c" "$G $S" "bitters_init();"
try "core + spi"            "bitters.c spi.c" "$S" "bitters_init();"
try "core alone"            "bitters.c" "" \
    "bitters_init(); bitters_reduced_latency();"
try "everything"            "bitters.c gpio.c spi.c i2c.c delay.c" "$ALL" \
    "bitters_init(); bitters_spi_init(); bitters_i2c_init();"

# --- and what was not named does not compile ---------------------------
# The whole reason the gate is in the header: a call into a subsystem this
# build does not carry has to fail here, in the file that made it, and not
# later as a missing symbol or never as an ignored return value.
refuses() {
    name=$1; flags=$2; body=$3
    cat > "$tmp/no.c" <<EOF
#include <bitters.h>
#include <bitters/gpio.h>
#include <bitters/spi.h>
#include <bitters/i2c.h>
int main(void) { $body return 0; }
EOF
    if $cc $BASE $flags -I"$top/include" -fsyntax-only "$tmp/no.c" \
           > "$tmp/log" 2>&1
    then
        printf '  %-40s COMPILED, and should not have\n' "$name"
        bad=1
    else
        printf '  %-40s refused at compile time\n' "$name"
    fi
}

SPI_CALL='bitters_spi_t s = BITTERS_SPI_INITIALIZER(0,0); bitters_spi_disable(&s);'
I2C_CALL='bitters_i2c_t i = BITTERS_I2C_INITIALIZER(0); bitters_i2c_disable(&i);'
GPIO_CALL='bitters_gpio_pin_t p = BITTERS_GPIO_PIN_INITIALIZER("c", 0); bitters_gpio_pin_disable(&p);'

refuses "spi call, spi not named"   "$G $I" "$SPI_CALL"
refuses "i2c call, i2c not named"   "$G $S" "$I2C_CALL"
refuses "gpio call, gpio not named" "$S $I" "$GPIO_CALL"

# ... and the price of naming what you have rather than what you lack: a
# consumer naming nothing sees nothing. Deliberate, and asserted so that it
# stays a known property. An installed library hands the flags over in
# bitters.pc; a vendored one takes them from the source list it chose.
refuses "nothing named, nothing declared" "" "$SPI_CALL"

# ... while the same calls compile once the subsystems are named, so the
# refusals above are the flags talking and not a broken test
cat > "$tmp/yes.c" <<EOF
#include <bitters.h>
#include <bitters/gpio.h>
#include <bitters/spi.h>
#include <bitters/i2c.h>
int main(void) { $SPI_CALL $I2C_CALL $GPIO_CALL return 0; }
EOF
if $cc $BASE $ALL -I"$top/include" -fsyntax-only "$tmp/yes.c" \
       > "$tmp/log" 2>&1
then
    printf '  %-40s compiles, as it must\n' "the same calls, all named"
else
    printf '  %-40s WRONGLY REFUSED\n' "the same calls, all named"
    sed 's/^/      /' "$tmp/log"
    bad=1
fi

# ... and bitters.pc is what hands them to a consumer of an installed
# library, which cannot know the source list because it did not pick one.
#
# Two halves, and both are asserted: the .pc recipe still puts
# $(SUBSYSTEMS) into Cflags, and $(SUBSYSTEMS) is still one gate per
# subsystem rather than the empty string. Reading the count off the
# Makefile line is what this used to do, and it printed 0 from the day
# that assignment became `!=` -- a number nobody compared against
# anything, which is the same as no check at all.
m="sh $top/scripts/manifest.sh"
nsub=`$m subsystems | wc -w | tr -d ' '`
nflag=`$m withflags  | wc -w | tr -d ' '`
if ! grep -q 'Cflags:.*$(SUBSYSTEMS)' "$top/Makefile"; then
    printf '  %-40s NOT EXPORTED\n' "bitters.pc exports them"
    bad=1
elif [ "$nsub" -eq 0 ] || [ "$nflag" -ne "$nsub" ]; then
    printf '  %-40s %s gates for %s subsystems\n' \
	"bitters.pc exports them" "$nflag" "$nsub"
    bad=1
else
    printf '  %-40s %s flags\n' "bitters.pc exports them" "$nflag"
fi

if [ $bad -eq 0 ]; then
    echo "vendoring subsets: ok"
else
    echo "vendoring subsets: MISMATCH with what bitters.cmake promises"
fi
exit $bad
