#!/bin/sh
# `make features` is consumed, not just read: the README shows
# `eval "$(make -s features)"`, so its shape is an interface and not a
# convenience. Four things hold it up, and none of them runs bitters' code:
#
#   the output is shell        (eval accepts it, and sets both names)
#   the names are its own      (not the ones `make sources` already claims)
#   the flags move             (a knob turned off drops what it turned on)
#   the comment does not lie   (strip the `#`, feed it back, same answer)
#
# Run by `make check`.
set -e
top=`dirname "$0"`/..
mk=${MAKE:-make}
bad=0

run() { (cd "$top" && $mk -s features "$@"); }

# --- the output is shell, and sets what the README says it sets --------
out=`run` || { echo "  make features failed"; exit 1; }

# The names the output actually assigns -- read out of it rather than
# spelled here, so this checks the shape and the next check can compare
# whatever it emits against what `make sources` emits. A renamed variable
# is a decision; a variable colliding with another target's is a bug.
names=`printf '%s\n' "$out" | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p'`

BITTERS_FEATURE_CPPFLAGS=; BITTERS_FEATURE_LIBS=
if ! eval "$out" 2>/dev/null; then
    printf '  %-40s NOT SHELL\n' "the output evals"
    bad=1
elif [ -z "$names" ]; then
    printf '  %-40s evals but assigns nothing\n' "the output evals"
    bad=1
else
    missing=
    for n in BITTERS_FEATURE_CPPFLAGS BITTERS_FEATURE_LIBS; do
	case "
$names" in *"
$n"*) ;; *) missing="$missing $n" ;; esac
    done
    if [ -z "$missing" ]; then
	printf '  %-40s ok\n' "the output evals"
    else
	printf '  %-40s the README names are unset:%s\n' \
	    "the output evals" "$missing"
	bad=1
    fi
fi

# --- and does not claim a name `make sources` already claims ----------
# Same name, two answers, is the drift this tree spends its checks on:
# BITTERS_LIBS is what the sources require and does not move with the
# feature flags, while these do.
claimed=`(cd "$top" && $mk -s sources) | sed 's/=.*//'`
collide=
for n in $names; do
    case "
$claimed" in *"
$n"*) collide="$collide $n" ;; esac
done
if [ -z "$collide" ]; then
    printf '  %-40s ok\n' "the names are its own"
else
    printf '  %-40s also emitted by `make sources`:%s\n' \
	"the names are its own" "$collide"
    bad=1
fi

# --- the flags move with the knobs ------------------------------------
# Otherwise the target could print a constant and every check above would
# still pass.
off=`run THREADS=no GPIO_IRQ=no`
case "$off" in
    *-DBITTERS_WITH_THREADS*)
	printf '  %-40s THREADS=no still names it\n' "the flags move"
	bad=1 ;;
    *)  case "$out" in
	    *-DBITTERS_WITH_THREADS*)
		printf '  %-40s ok\n' "the flags move" ;;
	    *)  printf '  %-40s THREADS=yes never named it\n' "the flags move"
		bad=1 ;;
	esac ;;
esac

# --- and the comment line is the command line that produced it --------
knobs=`printf '%s\n' "$off" | sed -n '1s/^# //p'`
case $knobs in
    *THREADS=no*) ;;
    *)  printf '  %-40s not a knob line: %s\n' "the comment replays" "$knobs"
	bad=1 ;;
esac
# shellcheck disable=SC2086  # the knob line is deliberately word-split
if [ "`run $knobs`" = "$off" ]; then
    printf '  %-40s ok\n' "the comment replays"
else
    printf '  %-40s feeding it back gives something else\n' "the comment replays"
    bad=1
fi

if [ $bad -eq 0 ]; then
    echo "features: ok"
else
    echo "features: NOT the interface the README documents"
fi
exit $bad
