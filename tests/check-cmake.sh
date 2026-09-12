#!/bin/sh
# bitters.cmake repeats what the Makefile already knows: the source list
# and the version. Two copies drift -- this series already shipped a
# Makefile claiming 0.1.0 while the tags said v1.0.0 -- so check they
# agree. Run by `make check`.
set -e
top=`dirname "$0"`/..
bad=0

mk_src=`sed -n 's/^SRC *= *//p' "$top/Makefile" | tr ' ' '\n' | sed 's|.*/||' | sort | tr -d '\r'`
cm_src=`sed -n 's|^set(BITTERS_SOURCES_[A-Z0-9]* *${CMAKE_CURRENT_LIST_DIR}/src/\([a-z0-9]*\.c\))|\1|p' \
        "$top/bitters.cmake" | sort`
if [ "$mk_src" != "$cm_src" ]; then
    echo "  source lists differ between Makefile and bitters.cmake:"
    echo "    Makefile     : `echo $mk_src`"
    echo "    bitters.cmake: `echo $cm_src`"
    bad=1
fi

mk_ver=`sed -n 's/^VERSION *= *//p' "$top/Makefile" | tr -d ' \r'`
cm_ver=`sed -n 's/^set(BITTERS_VERSION *\([0-9.]*\))/\1/p' "$top/bitters.cmake"`
if [ "$mk_ver" != "$cm_ver" ]; then
    echo "  version differs: Makefile=$mk_ver bitters.cmake=$cm_ver"
    bad=1
fi

if [ $bad -eq 0 ]; then
    echo "cmake/Makefile agree: $mk_ver, `echo $mk_src | wc -w | tr -d ' '` sources"
else
    echo "cmake/Makefile: MISMATCH"
fi
exit $bad
