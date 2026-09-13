#!/bin/sh
# Read bitters.cmake, which is the one place the file list lives.
#
# CMake consumers include it directly. The Makefile cannot, so it asks
# here instead -- `SRC != sh scripts/manifest.sh sources` and so on --
# and shell-driven builds get the whole answer at once from
# `make sources`, which is `vars` below. Nothing keeps a second copy, so
# there is no second copy to drift.
#
# Paths come out relative to the top of the tree, since that is what the
# Makefile wants; `vars` takes a directory to make them absolute with,
# because that is what a consumer elsewhere wants.
#
# POSIX sh and awk only.

set -e

top=`dirname "$0"`/..
cm="$top/bitters.cmake"

if [ ! -f "$cm" ]; then
    echo "manifest: no bitters.cmake next to $0" >&2
    exit 1
fi

# The values of one set(NAME ...), whether it is written on one line or
# spread over several, with the ${CMAKE_CURRENT_LIST_DIR}/ prefix taken
# off and the whitespace squeezed to single spaces.
cmvar() {
    awk -v want="$1" '
	/^set\(/         { collecting = 1; buf = "" }
	collecting       { buf = buf " " $0 }
	collecting && /\)/ {
	    collecting = 0
	    sub(/^[ \t]*set\(/, "", buf)
	    sub(/\)[ \t]*$/,    "", buf)
	    name = buf
	    sub(/[ \t].*$/, "", name)
	    if (name == want) {
		sub(/^[^ \t]+[ \t]*/, "", buf)
		gsub(/\$\{CMAKE_CURRENT_LIST_DIR\}\//, "", buf)
		gsub(/[ \t]+/, " ", buf)
		sub(/^ /, "", buf); sub(/ $/, "", buf)
		print buf
	    }
	}
    ' "$cm"
}

# set(BITTERS_SOURCES_GPIO ...) for part gpio. An unknown part gives an
# empty answer rather than an error, which the checks turn into a message.
partvar() {
    up=`echo "$1" | tr 'a-z' 'A-Z'`
    cmvar "BITTERS_SOURCES_$up"
}

# Every part the library is made of, gated or not, in the order the
# archive wants them.
PARTS='core delay gpio i2c spi'

what=$1
[ $# -gt 0 ] && shift

case $what in
version)    cmvar BITTERS_VERSION ;;
incdir)     cmvar BITTERS_INCLUDE_DIR ;;
subsystems) cmvar BITTERS_SUBSYSTEMS ;;
parts)      echo $PARTS ;;
src)        partvar "$1" ;;

# BITTERS_SOURCES is composed of the parts in cmake syntax the Makefile
# cannot expand, so compose it here from the same pieces.
sources)    out=; for p in $PARTS; do out="$out `partvar $p`"; done
	    echo $out ;;

# The gates the subsystems' headers are read through. Derived from the
# subsystem list rather than written out again, so adding a subsystem to
# bitters.cmake is all it takes.
withflags)  out=; for s in `cmvar BITTERS_SUBSYSTEMS`; do
		up=`echo "$s" | tr 'a-z' 'A-Z'`
		out="$out -DBITTERS_WITH_$up"
	    done
	    echo $out ;;

libs)       out=; for l in `cmvar BITTERS_LIBS`; do out="$out -l$l"; done
	    echo $out ;;

objs)       for p in $PARTS; do partvar "$p" | sed 's/\.c$/.o/'; done \
		| tr '\n' ' '
	    echo ;;

# What `make sources` prints: everything a build script needs, as shell
# variables, with $1 (the tree, absolutely) prefixed onto every path.
#
# The per-part lists are there so a consumer can take the subsystems it
# uses and leave the rest, which is what BITTERS_SUBSYSTEMS and the gates
# are for -- BITTERS_CFLAGS carries only what compiling bitters *requires*,
# so the gates for the parts you actually took are yours to add, as is the
# feature selection (THREADS, GPIO_IRQ, ASSERT, LOG). Printing a default
# for either would be a second place they are decided.
vars)
    d=$1
    [ -n "$d" ] || { echo "manifest: vars needs a directory" >&2; exit 1; }

    all=
    for p in $PARTS; do
	v=`partvar "$p"`
	up=`echo "$p" | tr 'a-z' 'A-Z'`
	printf "BITTERS_SOURCES_%s='%s'\n" "$up" "$d/$v"
	all="$all $d/$v"
    done
    inc=$d/`cmvar BITTERS_INCLUDE_DIR`
    libs=
    for l in `cmvar BITTERS_LIBS`; do libs="$libs -l$l"; done

    printf "BITTERS_SOURCES='%s'\n"    "`echo $all`"
    printf "BITTERS_SUBSYSTEMS='%s'\n" "`cmvar BITTERS_SUBSYSTEMS`"
    printf "BITTERS_INCLUDE='%s'\n"    "$inc"
    printf "BITTERS_CFLAGS='%s'\n"     "-D_GNU_SOURCE -I$inc"
    printf "BITTERS_LIBS='%s'\n"       "`echo $libs`"
    ;;

*)
    echo "usage: manifest.sh version|incdir|sources|parts|subsystems|withflags|libs|objs" >&2
    echo "       manifest.sh src <part>" >&2
    echo "       manifest.sh vars <directory>" >&2
    exit 1
    ;;
esac
