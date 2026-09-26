# bitters -- build, install and test.
#
# Portable between GNU make and BSD make: no conditionals, no pattern
# rules, no GNU-only functions. Feature selection uses variable
# indirection ($(T_$(THREADS))), which both makes expand the same way.
#
# Run `make help` for the targets and the variables you can override.
#
# The default is the shared library; `make static` builds an archive
# instead, for linking bitters into a single program.
#
# Vendoring is supported too -- there is nothing to configure at build
# time beyond the feature flags. `make sources` prints the files and the
# flags to compile them with. Note that the public headers need no
# special flags; only the .c files require -D_GNU_SOURCE.
#
# Feature selection (see README.md):
#
#   make THREADS=yes GPIO_IRQ=yes ASSERT=no LOG=no
#
# The tree builds warning-free with -Wall -Wextra; `make WERROR=yes`
# turns warnings into errors (use it in CI).
#
# The feature flags are build-time only. The public headers declare the
# whole API whatever they are set to, and a function whose feature was
# not compiled in returns -ENOSYS rather than going missing, so a
# consumer needs no flag to match the library it links against.

NAME       = bitters
MANIFEST   = sh scripts/manifest.sh

# The release, from include/bitters/version.h, which is the one place it
# is written -- bump it there and tag v<VERSION>. `make version` prints
# this, and it is what bitters.pc and the soname carry: fixed, so that a
# library named libbitters.so.1.1.1 is that release and nothing else.
VERSION   != $(MANIFEST) version
SOMAJOR    = 1

# What a build between releases adds to it: +3.gae9c67b[.dirty], and
# nothing for a release, for a tarball, or for a tree vendored inside
# another project's repository (whose git state is not bitters'). It is
# compiled into the library, where bitters_version() reports it; it stays
# out of the soname and out of bitters.pc, which name the release.
GITVER    != sh scripts/gitversion.sh

PREFIX    ?= /usr/local
LIBDIR    ?= $(PREFIX)/lib
INCLUDEDIR?= $(PREFIX)/include
PKGCONFDIR?= $(LIBDIR)/pkgconfig
DESTDIR   ?=

CC        ?= cc
AR        ?= ar
INSTALL   ?= install
DOXYGEN   ?= doxygen
# BSD make's sys.mk predefines CFLAGS, so `CFLAGS ?=` never applies there
# and the warning set would be silently dropped. Keep the flags the
# project requires in their own variable, always applied, and leave
# CFLAGS to the user and to the environment.
CFLAGS    ?= -O2 -g
# -Wundef: an undefined name in an #if evaluates quietly to 0, so a typo
# in one is worth hearing about. The tree is clean under it.
WARNINGS   = -Wall -Wextra -Wundef
LDFLAGS   ?=

# --- features ---------------------------------------------------------
# Selected by indirection rather than conditionals, so that one Makefile
# serves both makes: THREADS=yes picks $(T_yes), THREADS=no picks $(T_no).
THREADS   ?= yes
GPIO_IRQ  ?= yes
ASSERT    ?= no
LOG       ?= no
# Opt-in rather than default: a newer compiler inventing a new warning
# should not break an ordinary user's build, but CI should stay clean.
WERROR    ?= no

T_yes      = -DBITTERS_WITH_THREADS
T_no       =
P_yes     != $(MANIFEST) libs
P_no       =
G_yes      = -DBITTERS_WITH_GPIO_IRQ
G_no       =
A_yes      = -DBITTERS_GPIO_WITH_ASSERT -DBITTERS_SPI_WITH_ASSERT \
             -DBITTERS_I2C_WITH_ASSERT
A_no       =
O_yes      = -DBITTERS_GPIO_WITH_LOG -DBITTERS_SPI_WITH_LOG \
             -DBITTERS_I2C_WITH_LOG
O_no       =
W_yes      = -Werror
W_no       =

# Which subsystems this build has. Not switches: the library carries all
# of them, so all of them are named -- from bitters.cmake, which is where
# the list of them lives. They exist for a vendored tree that took only
# some of the sources (see the README's Vendoring section) and they gate
# the declarations, so bitters.pc carries them too.
SUBSYSTEMS != $(MANIFEST) withflags

FEATURES   = $(T_$(THREADS)) $(G_$(GPIO_IRQ)) $(A_$(ASSERT)) $(O_$(LOG)) \
             $(SUBSYSTEMS)
LIBS       = $(P_$(THREADS))

# Building bitters itself needs the feature flags and _GNU_SOURCE, which
# src/gpio.c refuses to compile without. A consumer needs _GNU_SOURCE for
# none of it, and of the flags only $(SUBSYSTEMS), which gate the
# declarations of the subsystems this build has -- so that is what
# bitters.pc exports beside the include path, and nothing else.
INCDIR    != $(MANIFEST) incdir
VERSIONHDR = $(INCDIR)/bitters/version.h

# BITTERS_VERSION_GIT is the one thing here that no file holds; src/bitters.c
# is what reads it, and version.h defaults it to "" for everyone who
# compiles these sources without it.
ALL_CPPFLAGS = -D_GNU_SOURCE $(FEATURES) -DBITTERS_VERSION_GIT='"$(GITVER)"' \
               -I$(INCDIR) -Isrc $(CPPFLAGS)
ALL_CFLAGS   = $(CFLAGS) $(WARNINGS) $(W_$(WERROR))

# Read from the manifest rather than listed here or globbed: bitters.cmake
# is the one place the list lives, and $(wildcard) is GNU-only besides.
SRC       != $(MANIFEST) sources
OBJ        = $(SRC:.c=.o)
PICOBJ     = $(SRC:.c=.lo)

STATIC     = lib$(NAME).a
SONAME     = lib$(NAME).so.$(SOMAJOR)
SHARED     = lib$(NAME).so.$(VERSION)

HEADERS    = include/bitters.h
SUBHEADERS = include/bitters/delay.h include/bitters/gpio.h \
             include/bitters/i2c.h include/bitters/rpi.h \
             include/bitters/spi.h include/bitters/version.h

# --- targets ----------------------------------------------------------
# help comes first so that a bare `make` prints it: both makes take the
# first target as the default goal, and what to build is usually the wrong
# question for a tree that is mostly vendored into other projects.
help:						## show this help
	@echo 'bitters -- microcontroller-style GPIO, SPI and I2C for Linux'
	@echo ''
	@echo 'Targets (a bare `make` prints this):'
	@awk -F':.*## ' '/^[a-z][a-z-]*:.*## /{printf "  %-16s %s\n", $$1, $$2}' \
	    Makefile
	@echo ''
	@echo 'Features (build-time only; the public API never depends on them):'
	@printf '  %-12s %-5s %s\n' \
	    THREADS  '$(THREADS)'  'thread support' \
	    GPIO_IRQ '$(GPIO_IRQ)' 'interrupt callbacks (implies THREADS=yes)' \
	    ASSERT   '$(ASSERT)'   'assertions in gpio/spi/i2c' \
	    LOG      '$(LOG)'      'logging to stderr'
	@echo ''
	@echo 'Other variables (current value):'
	@printf '  %-12s %s\n' \
	    CC       '$(CC)' \
	    CFLAGS   '$(CFLAGS)  (yours; the project always adds $(WARNINGS))' \
	    WERROR   '$(WERROR)  (yes turns warnings into errors)' \
	    YES      '$(YES)  (1 answers yes to `make tag`)' \
	    PREFIX   '$(PREFIX)' \
	    DESTDIR  '$(DESTDIR)  (staging prefix for packaging)'
	@echo ''
	@echo 'This Makefile works with both GNU make and BSD make.'
	@echo 'The GPIO tests drive a virtual chip; see tests/README.md.'

.SUFFIXES:
.SUFFIXES: .c .o .lo

# Shared by default; the static archive is opt-in.
all: featurecheck shared				## build the shared library

static: $(STATIC)				## build libbitters.a, to link into one program
shared: $(SHARED)				## build libbitters.so

# GPIO_IRQ=yes without THREADS=yes is rejected by src/gpio.c with an
# #error; this catches it before the compiler does. It cannot be a
# parse-time check, since $(error) is GNU-only.
featurecheck:
	@if [ "$(GPIO_IRQ)" = yes ] && [ "$(THREADS)" != yes ]; then \
	    echo "make: GPIO_IRQ=yes requires THREADS=yes" >&2; exit 1; fi

$(STATIC): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(SHARED): $(PICOBJ)
	$(CC) -shared -Wl,-soname,$(SONAME) $(LDFLAGS) -o $@ $(PICOBJ) $(LIBS)
	ln -sf $(SHARED) $(SONAME)
	ln -sf $(SONAME) lib$(NAME).so

# The git part is in no file, so nothing would make an object stale when
# HEAD moves, and the library would keep reporting the commit it was first
# built at. This stamp remembers the answer instead. The rule runs every
# time -- FORCE is a target that never exists, which is how both makes are
# told to -- and does nothing at all unless the answer changed, so an
# unmoved HEAD costs a `cat`.
#
# When it did change, the objects are removed rather than left to a
# timestamp comparison: BSD make compares against the mtime it read before
# this rule ran, so it would notice one `make` late and ship a library
# reporting the wrong commit. Deleting what was compiled with the old
# answer says the same thing in a way both makes act on at once.
FORCE:

.gitversion: FORCE
	@if [ "`cat $@ 2>/dev/null`" != '$(GITVER)' ]; then \
	    echo '$(GITVER)' > $@; rm -f $(OBJ) $(PICOBJ); fi

# ... and on the header the release is written in, so that bumping it
# recompiles what carries it. The only header dependency here: the rest of
# the API does not change what an object *says about itself*.
$(OBJ) $(PICOBJ): .gitversion $(VERSIONHDR)

.c.o:
	$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -c -o $@ $<

.c.lo:
	$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -fPIC -c -o $@ $<

# Depends on the Makefile and on the version header: a stale .pc carrying
# the previous feature set, or the previous release, is exactly the drift
# it exists to prevent.
$(NAME).pc: Makefile $(VERSIONHDR)
	@printf '%s\n' \
	  'prefix=$(PREFIX)' \
	  'libdir=$(LIBDIR)' \
	  'includedir=$(INCLUDEDIR)' \
	  '' \
	  'Name: $(NAME)' \
	  'Description: microcontroller-style GPIO, SPI and I2C for Linux' \
	  'Version: $(VERSION)' \
	  'Libs: -L$${libdir} -l$(NAME) $(LIBS)' \
	  'Cflags: -I$${includedir} $(SUBSYSTEMS)' > $@

# Bare, so a script can use it:  v=`make -s version`
version:					## print the release version
	@echo '$(VERSION)'

# The same, plus what a build from this tree adds to it -- which is what
# the library built here reports through bitters_version(). Equal to
# `make version` exactly when this is a release tree.
version-full:					## print the version this tree builds as
	@echo '$(VERSION)$(GITVER)'

# Tag the release from the version header, so that the tag and the header
# cannot say different things: the number is not typed here, it is read
# from $(VERSIONHDR). tests/check-manifest.sh checks the other direction,
# for a tag made by hand -- scripts/checktag.sh, run below so that a tag
# this target just made is confirmed rather than assumed.
#
# Refuses on an unclean worktree: a release tag names committed work, and
# the version a build reports would otherwise include `.dirty`. Nothing is
# pushed; that stays yours.
#
# In that order on purpose: the two refusals are instant, so they come
# before the question -- there is no point asking about a tag that cannot be
# made -- and `check tests` comes after it, because a minute of tests is
# not worth spending on a `make tag` the answer to which is no. Tagging
# something the suite has not passed is the mistake this exists to prevent,
# so both are inside the recipe rather than prerequisites, which would
# have run before the prompt.
#
# `make tag YES=1` answers yes for a script, and a non-interactive run with
# no YES=1 reads EOF and declines -- the safe way round.
tag: $(VERSIONHDR)				## tag this release, from the version header
	@if [ -n "`git status --porcelain --untracked-files=no`" ]; then \
	    echo 'make: uncommitted changes; commit them before tagging' >&2; \
	    exit 1; fi
	@if git rev-parse -q --verify 'refs/tags/v$(VERSION)' >/dev/null; then \
	    echo 'make: v$(VERSION) exists already; bump $(VERSIONHDR) first' >&2; \
	    exit 1; fi
	@if [ "$(YES)" != 1 ]; then \
	    printf 'tag v%s at %s? (check and tests run first) [y/N] ' \
		'$(VERSION)' "`git rev-parse --short HEAD`"; \
	    read -r ans || ans=; \
	    case "$$ans" in \
		y|Y|yes|YES) ;; \
		*) echo 'make: not tagged'; exit 1 ;; \
	    esac; \
	fi
	@$(MAKE) check tests
	git tag -a -m '$(NAME) $(VERSION)' 'v$(VERSION)'
	@sh scripts/checktag.sh '$(VERSION)'
	@echo 'tagged v$(VERSION) -- push it with: git push origin v$(VERSION)'

# What a feature combination expands to, as shell, the way `make sources`
# is -- so that the knobs and the flags they produce are one representation
# rather than two:
#
#     eval "$$(make -s features THREADS=no)"
#     cc $$BITTERS_FEATURE_CPPFLAGS -c ...
#
# The knob line is a shell *comment*, which is what keeps it in the same
# language as the rest without putting four unprefixed names into the
# caller's shell. Strip the `#` and it is the make command line back again.
#
# The names are prefixed like everything `make sources` emits, and they are
# deliberately not BITTERS_CFLAGS or BITTERS_LIBS: those answer what
# compiling *the sources* requires and do not move with the feature flags,
# while these are this combination's answer and do. Two questions, so two
# names -- one name with two answers is the drift this tree keeps
# removing.
#
# $(FEATURES) is echoed unquoted on purpose: a feature set to `no`
# contributes the empty string, and the shell's word splitting is what
# removes the runs of whitespace that leaves between the flags.
features:					## print this feature selection as shell variables
	@echo '# THREADS=$(THREADS) GPIO_IRQ=$(GPIO_IRQ) ASSERT=$(ASSERT) LOG=$(LOG)'
	@printf "BITTERS_FEATURE_CPPFLAGS='%s'\n" "`echo $(FEATURES)`"
	@printf "BITTERS_FEATURE_LIBS='%s'\n"     "`echo $(LIBS)`"

# Everything needed to compile bitters straight into another project,
# emitted as shell variables so a build script can consume it:
#
#     eval "$$(make -s -C 3rd/bitters sources)"
#     cc $$BITTERS_CFLAGS -c $$BITTERS_SOURCES
#
# Paths are absolute, so the caller need not know where bitters sits.
sources:					## print vendoring files and flags as shell variables
	@$(MANIFEST) vars "`pwd`"

# Preflight: is this tree fit to build? It runs none of bitters' own code
# -- the manifest still describes the tree, both text parses of the version
# agree with the compiler's, the tag does not contradict the header, every
# vendoring subset still compiles and still refuses what it must, and
# `make features` still evals to what the README says. So it is
# cheap, it needs nothing built, and it is the first thing to run when
# something looks wrong. Running the code is `tests`, below.
check: featurecheck check-manifest check-features check-subset	## preflight: this tree is fit to build

check-manifest:					## bitters.cmake and version.h still describe the tree
	@sh tests/check-manifest.sh

# $(MAKE) is passed on so the check asks the make that is running: `gmake
# check` then holds gmake's answer up, not this host's default make's.
check-features:					## `make features` is still the interface the README documents
	@MAKE='$(MAKE)' sh tests/check-features.sh

check-subset:					## every vendoring subset still compiles, and refuses
	@sh tests/check-subset.sh

# The suite. It builds what it needs and runs it; tests/Makefile owns how.
tests:						## build and run the tests needing no privilege
	cd tests && $(MAKE) tests

tests-gpio:					## the GPIO tests (needs root + gpio-mockup)
	cd tests && $(MAKE) tests-gpio

tests-endian:					## the i2c bitfield view on other-endian ABIs
	cd tests && $(MAKE) tests-endian

# PROJECT_NUMBER is appended rather than written in the Doxyfile, so the
# documentation says which version it documents without that being a
# second place the version is kept.
doc:						## generate the Doxygen documentation into doc/
	{ cat Doxyfile; echo 'PROJECT_NUMBER = $(VERSION)$(GITVER)'; } \
	    | $(DOXYGEN) -

# Installs whatever was built: the shared library always, the archive
# only if `make static` produced one.
install: shared $(NAME).pc			## install headers, library and bitters.pc
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)/bitters
	$(INSTALL) -m 644 $(HEADERS)    $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m 644 $(SUBHEADERS) $(DESTDIR)$(INCLUDEDIR)/bitters
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 755 $(SHARED) $(DESTDIR)$(LIBDIR)
	ln -sf $(SHARED) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/lib$(NAME).so
	@if [ -f $(STATIC) ]; then \
	    echo "$(INSTALL) -m 644 $(STATIC) $(DESTDIR)$(LIBDIR)"; \
	    $(INSTALL) -m 644 $(STATIC) $(DESTDIR)$(LIBDIR); \
	fi
	$(INSTALL) -d $(DESTDIR)$(PKGCONFDIR)
	$(INSTALL) -m 644 $(NAME).pc $(DESTDIR)$(PKGCONFDIR)

uninstall:					## remove what install put down
	rm -f  $(DESTDIR)$(INCLUDEDIR)/bitters.h
	rm -rf $(DESTDIR)$(INCLUDEDIR)/bitters
	rm -f  $(DESTDIR)$(LIBDIR)/$(STATIC) \
	       $(DESTDIR)$(LIBDIR)/$(SHARED) \
	       $(DESTDIR)$(LIBDIR)/$(SONAME) \
	       $(DESTDIR)$(LIBDIR)/lib$(NAME).so \
	       $(DESTDIR)$(PKGCONFDIR)/$(NAME).pc

clean:						## remove build products
	rm -f $(OBJ) $(PICOBJ) $(STATIC) $(SHARED) $(SONAME) lib$(NAME).so \
	      $(NAME).pc .gitversion
	cd tests && $(MAKE) clean

distclean: clean				## clean, plus the generated documentation
	rm -rf doc/html doc/latex


.PHONY: all static shared featurecheck check check-manifest check-subset \
	check-features tests tests-gpio tests-endian doc install uninstall \
	clean distclean version version-full tag features sources help
