#!/bin/sh
# bitters.cmake is the manifest: the Makefile reads it through
# scripts/manifest.sh, and CMake consumers include it. The release is the
# same arrangement one file over -- written in include/bitters/version.h,
# because a C header can read no other file, and parsed from there by
# bitters.cmake and by manifest.sh. There is no second copy of either to
# compare against any more -- what is left to check is that they still
# describe the tree, that both text parses of the version agree with the
# compiler's, and that nothing has quietly gone back to keeping its own
# list. Run by `make check`.
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

# --- and the compiler reads the same version --------------------------
# The manifest parses version.h with awk and bitters.cmake with a regex,
# but what a consumer actually gets is what the *preprocessor* makes of
# it. Ask it, so that a second #define, a comment in the wrong place or a
# clever macro cannot make the readers disagree.
cc=${CC:-cc}
cppversion=$(printf '#include <bitters/version.h>\nBITTERS_VERSION_STRING\n' \
	    | $cc -E -I"$top/include" -x c - 2>/dev/null \
	    | tr -d '" \t' | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | tail -n 1)
if [ -z "$cppversion" ]; then
    echo "  version: the preprocessor makes nothing of BITTERS_VERSION_STRING"
    bad=1
elif [ "$cppversion" != "$version" ]; then
    echo "  version: the manifest says $version, the preprocessor $cppversion"
    bad=1
fi

# --- and so does the third reader -------------------------------------
# bitters.cmake parses the same three lines with its own regex, and a CMake
# consumer gets that answer without ever running the Makefile or this
# script's awk. Two text parsers agreeing with each other proves nothing
# about the third, so ask CMake itself rather than re-implementing its
# regex here -- a fourth parser would only be a fourth thing to be wrong.
# Skipped, loudly, where there is no cmake to ask.
if command -v cmake >/dev/null 2>&1; then
    ctmp=`mktemp -d`
    {
	echo "include(\"`cd \"$top\" && pwd`/bitters.cmake\")"
	echo 'message(STATUS "BITTERS_VERSION=${BITTERS_VERSION}")'
    } > "$ctmp/v.cmake"
    cmakeversion=`cmake -P "$ctmp/v.cmake" 2>&1 \
		  | sed -n 's/^-- BITTERS_VERSION=//p'`
    rm -rf "$ctmp"
    if [ -z "$cmakeversion" ]; then
	echo "  version: bitters.cmake makes nothing of BITTERS_VERSION"
	bad=1
    elif [ "$cmakeversion" != "$version" ]; then
	echo "  version: the manifest says $version, bitters.cmake $cmakeversion"
	bad=1
    fi
else
    echo "  version: bitters.cmake's parse unchecked (no cmake)"
fi

# --- and the tags say the same thing ----------------------------------
# The header is one half of naming a release; `git tag` is the other, and
# nothing in the tree makes them agree. `make tag` makes the tag from the
# header so that they cannot disagree, but a tag made by hand can, and a
# release build has no later chance to notice. Silent when there is no git,
# or when the repository around this tree is somebody else's.
if out=$(sh "$top/scripts/checktag.sh" "$version"); then
    :
else
    echo "$out"
    bad=1
fi

# --- the git part, when there is one ----------------------------------
# It is appended to the release, so it has to be an addition and not a
# replacement: SemVer build metadata, starting with '+'. Empty is the
# right answer for a release, a tarball, or a vendored tree.
gitver=$(sh "$top/scripts/gitversion.sh")
case $gitver in
    "") ;;
    +*) ;;
    *) echo "  git part does not start with '+': '$gitver'"; bad=1 ;;
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

# --- every public header is installed ---------------------------------
# The Makefile names them (HEADERS and SUBHEADERS) because install(1)
# needs the list; a new public header nobody added there compiles here and
# is simply missing from the installed tree, where the consumer who
# notices is somebody else.
# $( ) and not backticks on purpose: backticks would eat one level of
# backslashes on the way in, and this awk program is made of them -- the
# continuation test would arrive as /$/ and match nothing.
installed=$(awk '
	/^HEADERS|^SUBHEADERS/ { inlist = 1 }
	inlist {
	    cont = ($0 ~ /\\$/)
	    sub(/\\$/, "")
	    for (i = 1; i <= NF; i++)
		if ($i ~ /\.h$/)
		    print $i
	    if (!cont)
		inlist = 0
	}
    ' "$top/Makefile")
for f in "$top"/include/*.h "$top"/include/bitters/*.h; do
    base=${f#"$top/"}
    case "
$installed" in
	*"
$base"*) ;;
	*) echo "  $base exists but the Makefile does not install it"; bad=1 ;;
    esac
done

# --- nothing keeps its own copy ---------------------------------------
# The Makefile used to hold the whole list, and tests/check-cmake.sh
# existed to notice when the two disagreed. It reads the manifest now;
# fail if it starts spelling source paths itself again.
if grep -q '^[A-Z_]* *= *src/' "$top/Makefile"; then
    echo "  Makefile names sources directly again"
    bad=1
fi

# Counted with awk rather than `echo $parts | wc -w`, which would need the
# expansion left unquoted in order to split.
nparts=$(printf '%s\n' "$parts" | awk '{print NF}')

if [ $bad -eq 0 ]; then
    echo "manifest: $version$gitver, $nparts parts, all present"
else
    echo "manifest: BROKEN"
fi
exit $bad
