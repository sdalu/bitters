#!/bin/sh
# bitters.cmake is the manifest: the Makefile reads it through
# scripts/manifest.sh, and CMake consumers include it. There is no second
# copy to compare it against any more -- what is left to check is that it
# still describes the tree, and that nothing has quietly gone back to
# keeping its own list. Run by `make check`.
set -e
top=`dirname "$0"`/..
m="sh $top/scripts/manifest.sh"
bad=0

# --- the manifest parses at all ---------------------------------------
# Every query returns the empty string when the awk stops matching, and an
# empty source list would make the build succeed by compiling nothing.
# Fail loudly instead.
version=`$m version`
case $version in
    [0-9]*.[0-9]*) ;;
    *) echo "  version is not a version: '$version'"; bad=1 ;;
esac

incdir=`$m incdir`
if [ -z "$incdir" ] || [ ! -d "$top/$incdir" ]; then
    echo "  manifest: incdir names no directory: '$incdir'"; bad=1
fi

# --- every part is really there ---------------------------------------
parts=`$m parts`
[ -n "$parts" ] || { echo "  manifest: no parts"; bad=1; }
for p in $parts; do
    src=`$m src $p`
    if [ -z "$src" ]; then
	echo "  part $p: listed by the manifest but not defined in bitters.cmake"
	bad=1
    elif [ ! -f "$top/$src" ]; then
	echo "  part $p: no file $src"; bad=1
    fi
done

# --- and every source is a part ---------------------------------------
# The other direction: a new src/*.c that nobody added to bitters.cmake
# would be silently left out of every build that reads it.
listed=`$m sources`
for f in "$top"/src/*.c; do
    base=src/`basename "$f"`
    case " $listed " in
	*" $base "*) ;;
	*) echo "  $base exists but the manifest does not name it"; bad=1 ;;
    esac
done

# --- the subsystems, and the gates derived from them -------------------
# Each gated subsystem must be a part, and its source must read the gate:
# a subsystem listed here whose header is not gated would let a consumer
# leave it out and still compile calls into it.
for s in `$m subsystems`; do
    case " $parts " in
	*" $s "*) ;;
	*) echo "  subsystem $s is not one of the parts"; bad=1; continue ;;
    esac
    up=`echo "$s" | tr 'a-z' 'A-Z'`
    grep -q "BITTERS_WITH_$up" "$top/include/bitters/$s.h" || {
	echo "  subsystem $s: include/bitters/$s.h is not gated on BITTERS_WITH_$up"
	bad=1; }
    grep -q "BITTERS_WITH_$up" "$top/src/bitters.c" || {
	echo "  subsystem $s: src/bitters.c does not gate its init on BITTERS_WITH_$up"
	bad=1; }
done

# --- nothing keeps its own copy ---------------------------------------
# The Makefile used to hold the whole list, and tests/check-cmake.sh
# existed to notice when the two disagreed. It reads the manifest now;
# fail if it starts spelling source paths itself again.
if grep -q '^[A-Z_]* *= *src/' "$top/Makefile"; then
    echo "  Makefile names sources directly again"
    bad=1
fi

if [ $bad -eq 0 ]; then
    echo "manifest: $version, `echo $parts | wc -w | tr -d ' '` parts, all present"
else
    echo "manifest: BROKEN"
fi
exit $bad
