# bitters -- build, install and test.   Requires GNU make.
#
#   make                       build static + shared library
#   make check                 run the test suite (see tests/README.md)
#   make install PREFIX=/usr   install headers, libraries and bitters.pc
#   make doc                   generate the Doxygen documentation
#
# Feature selection (see README.md):
#
#   make THREADS=yes GPIO_IRQ=yes ASSERT=no LOG=no
#
# NOTE: the feature flags change the layout of bitters_gpio_pin_t, so an
# application MUST be compiled with the same set as the library. The
# generated bitters.pc carries them in Cflags for exactly that reason;
# build your application with `pkg-config --cflags bitters` and the two
# cannot drift apart.

NAME      := bitters
VERSION   := 0.1.0
SOMAJOR   := 0

PREFIX    ?= /usr/local
LIBDIR    ?= $(PREFIX)/lib
INCLUDEDIR?= $(PREFIX)/include
PKGCONFDIR?= $(LIBDIR)/pkgconfig
DESTDIR   ?=

CC        ?= cc
AR        ?= ar
INSTALL   ?= install
DOXYGEN   ?= doxygen
CFLAGS    ?= -O2 -g -Wall -Wextra
LDFLAGS   ?=

# --- features ---------------------------------------------------------
THREADS   ?= yes
GPIO_IRQ  ?= yes
ASSERT    ?= no
LOG       ?= no

FEATURES  :=
LIBS      :=
ifeq ($(THREADS),yes)
  FEATURES += -DBITTERS_WITH_THREADS
  LIBS     += -lpthread
endif
ifeq ($(GPIO_IRQ),yes)
  ifneq ($(THREADS),yes)
    $(error GPIO_IRQ=yes requires THREADS=yes)
  endif
  FEATURES += -DBITTERS_WITH_GPIO_IRQ
endif
ifeq ($(ASSERT),yes)
  FEATURES += -DBITTERS_GPIO_WITH_ASSERT -DBITTERS_SPI_WITH_ASSERT \
              -DBITTERS_I2C_WITH_ASSERT
endif
ifeq ($(LOG),yes)
  FEATURES += -DBITTERS_GPIO_WITH_LOG -DBITTERS_SPI_WITH_LOG \
              -DBITTERS_I2C_WITH_LOG
endif

# _GNU_SOURCE is not optional: src/gpio.c refuses to compile without it.
PUBLIC_CPPFLAGS := -D_GNU_SOURCE $(FEATURES)
ALL_CPPFLAGS    := $(PUBLIC_CPPFLAGS) -Iinclude -Isrc $(CPPFLAGS)

SRC       := $(wildcard src/*.c)
OBJ       := $(SRC:.c=.o)
PICOBJ    := $(SRC:.c=.lo)

STATIC    := lib$(NAME).a
SONAME    := lib$(NAME).so.$(SOMAJOR)
SHARED    := lib$(NAME).so.$(VERSION)

HEADERS   := include/bitters.h
SUBHEADERS:= $(wildcard include/bitters/*.h)

.PHONY: all static shared check check-gpio doc install uninstall clean distclean features

all: static shared

static: $(STATIC)
shared: $(SHARED)

$(STATIC): $(OBJ)
	$(AR) rcs $@ $^

$(SHARED): $(PICOBJ)
	$(CC) -shared -Wl,-soname,$(SONAME) $(LDFLAGS) -o $@ $^ $(LIBS)
	ln -sf $(SHARED) $(SONAME)
	ln -sf $(SONAME) lib$(NAME).so

%.o: %.c
	$(CC) $(CFLAGS) $(ALL_CPPFLAGS) -c -o $@ $<

%.lo: %.c
	$(CC) $(CFLAGS) $(ALL_CPPFLAGS) -fPIC -c -o $@ $<

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
	  'Cflags: -I$${includedir} $(PUBLIC_CPPFLAGS)' > $@

# Report exactly how this tree would be built.
features:
	@echo 'THREADS=$(THREADS) GPIO_IRQ=$(GPIO_IRQ) ASSERT=$(ASSERT) LOG=$(LOG)'
	@echo 'public cppflags: $(PUBLIC_CPPFLAGS)'
	@echo 'libs           : $(LIBS)'

check:
	$(MAKE) -C tests check

check-gpio:
	$(MAKE) -C tests check-gpio

doc:
	$(DOXYGEN) Doxyfile

install: all $(NAME).pc
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)/bitters
	$(INSTALL) -m 644 $(HEADERS)    $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m 644 $(SUBHEADERS) $(DESTDIR)$(INCLUDEDIR)/bitters
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 $(STATIC) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 755 $(SHARED) $(DESTDIR)$(LIBDIR)
	ln -sf $(SHARED) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/lib$(NAME).so
	$(INSTALL) -d $(DESTDIR)$(PKGCONFDIR)
	$(INSTALL) -m 644 $(NAME).pc $(DESTDIR)$(PKGCONFDIR)

uninstall:
	rm -f  $(DESTDIR)$(INCLUDEDIR)/bitters.h
	rm -rf $(DESTDIR)$(INCLUDEDIR)/bitters
	rm -f  $(DESTDIR)$(LIBDIR)/$(STATIC) \
	       $(DESTDIR)$(LIBDIR)/$(SHARED) \
	       $(DESTDIR)$(LIBDIR)/$(SONAME) \
	       $(DESTDIR)$(LIBDIR)/lib$(NAME).so \
	       $(DESTDIR)$(PKGCONFDIR)/$(NAME).pc

clean:
	rm -f $(OBJ) $(PICOBJ) $(STATIC) $(SHARED) $(SONAME) lib$(NAME).so \
	      $(NAME).pc
	$(MAKE) -C tests clean

distclean: clean
	rm -rf doc/html doc/latex
