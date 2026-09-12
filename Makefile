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
VERSION    = 1.0.0
SOMAJOR    = 1

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
WARNINGS   = -Wall -Wextra
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
P_yes      = -lpthread
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

FEATURES   = $(T_$(THREADS)) $(G_$(GPIO_IRQ)) $(A_$(ASSERT)) $(O_$(LOG))
LIBS       = $(P_$(THREADS))

# Building bitters itself needs the feature flags and _GNU_SOURCE, which
# src/gpio.c refuses to compile without. A consumer needs neither: the
# headers are flag-independent and compile without _GNU_SOURCE, so
# bitters.pc exports nothing but the include path.
ALL_CPPFLAGS = -D_GNU_SOURCE $(FEATURES) -Iinclude -Isrc $(CPPFLAGS)
ALL_CFLAGS   = $(CFLAGS) $(WARNINGS) $(W_$(WERROR))

# Listed rather than globbed: $(wildcard) is GNU-only, and an explicit
# list is what a vendoring consumer wants from `make sources` anyway.
SRC        = src/bitters.c src/delay.c src/gpio.c src/i2c.c src/spi.c
OBJ        = $(SRC:.c=.o)
PICOBJ     = $(SRC:.c=.lo)

STATIC     = lib$(NAME).a
SONAME     = lib$(NAME).so.$(SOMAJOR)
SHARED     = lib$(NAME).so.$(VERSION)

HEADERS    = include/bitters.h
SUBHEADERS = include/bitters/delay.h include/bitters/gpio.h \
             include/bitters/i2c.h include/bitters/rpi.h include/bitters/spi.h

.SUFFIXES:
.SUFFIXES: .c .o .lo

# Shared by default; the static archive is opt-in.
all: featurecheck shared				## build the shared library (default)

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

.c.o:
	$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -c -o $@ $<

.c.lo:
	$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -fPIC -c -o $@ $<

# Depends on the Makefile: a stale .pc carrying the previous feature set
# is exactly the drift it exists to prevent.
$(NAME).pc: Makefile
	@printf '%s\n' \
	  'prefix=$(PREFIX)' \
	  'libdir=$(LIBDIR)' \
	  'includedir=$(INCLUDEDIR)' \
	  '' \
	  'Name: $(NAME)' \
	  'Description: microcontroller-style GPIO, SPI and I2C for Linux' \
	  'Version: $(VERSION)' \
	  'Libs: -L$${libdir} -l$(NAME) $(LIBS)' \
	  'Cflags: -I$${includedir}' > $@

features:					## print the feature selection in force
	@echo 'THREADS=$(THREADS) GPIO_IRQ=$(GPIO_IRQ) ASSERT=$(ASSERT) LOG=$(LOG)'
	@echo 'build cppflags : $(FEATURES)'
	@echo 'libs           : $(LIBS)'

# Everything needed to compile bitters straight into another project,
# emitted as shell variables so a build script can consume it:
#
#     eval "$$(make -s -C 3rd/bitters sources)"
#     cc $$BITTERS_CFLAGS -c $$BITTERS_SOURCES
#
# Paths are absolute, so the caller need not know where bitters sits.
sources:					## print vendoring files and flags as shell variables
	@d=`pwd`; out=""; \
	 for f in $(SRC); do \
	     if [ -z "$$out" ]; then out="$$d/$$f"; else out="$$out $$d/$$f"; fi; \
	 done; \
	 printf "BITTERS_SOURCES='%s'\n" "$$out"; \
	 printf "BITTERS_INCLUDE='%s'\n" "$$d/include"; \
	 printf "BITTERS_CFLAGS='%s'\n" \
	     "`echo -D_GNU_SOURCE $(FEATURES) -I$$d/include | tr -s ' '`"; \
	 printf "BITTERS_LIBS='%s'\n" "`echo $(LIBS) | tr -s ' '`"

check:						## run the tests needing no privilege
	cd tests && $(MAKE) check

check-gpio:					## run the GPIO tests (needs root + gpio-mockup)
	cd tests && $(MAKE) check-gpio

doc:						## generate the Doxygen documentation into doc/
	$(DOXYGEN) Doxyfile

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
	      $(NAME).pc
	cd tests && $(MAKE) clean

distclean: clean				## clean, plus the generated documentation
	rm -rf doc/html doc/latex

help:						## show this help
	@echo 'bitters -- microcontroller-style GPIO, SPI and I2C for Linux'
	@echo ''
	@echo 'Targets:'
	@awk -F':.*## ' '/^[a-z][a-z-]*:.*## /{printf "  %-12s %s\n", $$1, $$2}' \
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
	    PREFIX   '$(PREFIX)' \
	    DESTDIR  '$(DESTDIR)  (staging prefix for packaging)'
	@echo ''
	@echo 'This Makefile works with both GNU make and BSD make.'
	@echo 'The test suite has its own targets; see tests/README.md.'

.PHONY: all static shared featurecheck check check-gpio doc install \
	uninstall clean distclean features sources help
