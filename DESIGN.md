Design notes
============

Why bitters has the shape it has. What it does and how to drive it is in
[README.md](README.md).


One place per fact
------------------

Two files are the sources of truth for everything derived, and nothing
keeps a second copy of either:

* `bitters.cmake` is the **manifest** -- the list of sources, the parts a
  vendored tree may take, the subsystems, the link libraries. CMake
  consumers `include()` it. The Makefile cannot, so it asks
  `scripts/manifest.sh`, which parses it.
* `include/bitters/version.h` is the **release** -- three `#define`s.
  `bitters.cmake` parses them; `manifest.sh` parses them; `make version`,
  `bitters.pc` and the soname all come from there.

A fact stated twice diverges, and the comment saying "keep these in step"
is the rule that gets broken by the person who wrote it. So the rule here
is mechanical rather than written: there is no second copy left to
disagree. `tests/check-manifest.sh` and `tests/check-subset.sh` hold the
derivation up -- see *check is preflight* below.

Neither file is generated, and that is the point. A generated manifest and
a generated version header are absent in exactly the tree that needs them
most: an unpacked tarball, or a copy of these sources sitting inside
somebody else's repository with no build step of its own.


The release lives in a C header
-------------------------------

A C header can read no other file. If a consumer is to have the version
without running anything -- in a `#if`, before any build system has been
chosen -- then the version has to be written somewhere a header can see
it, which means in a header. Everything else reads it from there, and the
direction is never reversed.

`tests/check-manifest.sh` asks the **preprocessor** what
`BITTERS_VERSION_STRING` expands to and compares that against what the two
text parsers made of the same file. Two parsers can agree with each other
and both be wrong about what the compiler sees; a second `#define`, a
comment in the wrong place or a clever macro would be invisible to them.

`make tag` reads the number out of the header rather than having it typed,
so the tag and the header cannot say different things.
`scripts/checktag.sh` gates the other direction, for a tag made by hand: on
a tag with a clean worktree the header must say what the tag says, and
anywhere else it must be at or ahead of the nearest tag, because
bumped-but-not-yet-tagged is the normal state between releases.


What no file holds: the commit
------------------------------

Which commit a build came from is the one version fact that belongs in no
file. `scripts/gitversion.sh` works it out at build time and the Makefile
compiles it in as `-DBITTERS_VERSION_GIT`; `bitters_version()` reports it.

It is empty for a release, empty for a tarball, and empty for a tree
vendored inside another project's repository. That last case is the reason
for the care: a copy of these sources under somebody else's `.git` would
otherwise report *their* tags and *their* uncommitted changes as bitters'.
An empty answer is never wrong, only less precise -- it says "the release
these files say it is", which is exactly what a tarball is.

Because the git part is in no file, nothing would make an object stale when
`HEAD` moves, and a library would go on reporting the commit it was first
built at. The `.gitversion` stamp remembers the previous answer; when it
changes, the rule **deletes** the objects rather than touching the stamp
and letting a timestamp comparison notice. BSD make compares against the
mtime it read before the rule ran, so a timestamp would be acted on one
`make` late -- and shipping a library that reports the wrong commit is the
whole failure being avoided.


The API is declared whole; the features are build-time
------------------------------------------------------

`THREADS`, `GPIO_IRQ`, `ASSERT` and `LOG` configure **bitters itself**.
None of them changes what the public headers declare: the whole API is
declared whatever they are set to, and a function whose feature was not
compiled in returns `-ENOSYS` rather than going missing.

The reason is that a consumer cannot know how the library it links against
was built -- a shared library can be replaced underneath it -- so anything
a consumer must match is a hazard. `bitters.pc` therefore exports neither
`-D_GNU_SOURCE` (needed to compile bitters, not to use it) nor the feature
selection. A runtime `-ENOSYS` a caller can check beats a link error, and
beats a silent ABI disagreement by much more.


...except the subsystem gates, which do gate declarations
---------------------------------------------------------

`BITTERS_WITH_GPIO`, `_SPI` and `_I2C` are the deliberate exception:
`bitters/gpio.h` and its siblings declare nothing without them.

The asymmetry is the whole design. You cannot know how the library was
compiled, but you certainly know what **you** compiled. A vendored tree
picks a subset of the sources, and naming what it took turns a call into a
subsystem it left behind into a compile error in the file that made the
call -- earlier than a missing symbol at link time, and unmissable beside a
runtime code that a caller ignoring return values would never see.

A consumer of an *installed* library picked no source list and so cannot
know either, which is why `bitters.pc` does carry these three in its
`Cflags`, alongside the include path. They are the one thing about the
build a consumer is told, because for an installed library the answer is
"all of them" and it is the `.pc` that knows it.

`_DELAY` has no gate and is simply left out when it is not wanted, because
nothing reaches into it.


The pin structure has one layout
--------------------------------

`bitters_gpio_pin_t` is caller-allocated, so its size and layout are ABI.
The two interrupt-callback fields are present in every configuration,
including builds without thread support that can never use them. Two
pointers are cheaper than a structure whose layout depends on a compile
flag: when they were conditional, a library and an application built with
different settings disagreed about the structure with no link error and no
warning -- the symbol resolved, the offsets did not. Do not make a public
structure's layout conditional again.


A controller is named by what the kernel reports
------------------------------------------------

`ctrl_devname` on a pin is a device name under `/dev`, or a chip label,
or several of either separated by `|`, and the library resolves it to a
device when the pin is first enabled. The reason is the Raspberry Pi 5:
it moved the header GPIO to the RP1 southbridge, and a fixed `gpiochip0`
-- which every earlier release wrote into `BITTERS_RPI_GPIO_CHIP` -- then
drove the wrong controller. Device numbers are assigned at probe; the
label is what the driver calls the chip and names the silicon, so a list
of labels names a family of boards, and the Pi macro is now three labels
with the old device name last.

Two other shapes were rejected. A `bitters_rpi_gpio_chip()` function
returning the device name cannot sit in a static initializer, which is
how `BITTERS_GPIO_PIN_INITIALIZER` is meant to be used and how the Pi
macro is used in the README; it would also have needed a source of its
own for what is otherwise a header of numbers. Changing the field to an
array of names would have changed the layout of `bitters_gpio_pin_t`,
which is ABI (see below). A separator in the string keeps the field, the
macro and every existing caller as they were.

The resolved device name, not the name the pin gave, is what controllers
are matched on, so a label and the device it stands for share one
controller -- one descriptor, one interrupt thread. The resolution runs
outside the controller lock: it may scan `/dev`, and needs nothing shared.


Active low is a field, not a mode
---------------------------------

`active_low` is its own byte in `bitters_gpio_cfg_t`, appended so that
positional initializers keep their meaning, and maps to the one kernel
flag. It could have been a `BITTERS_GPIO_MODE_*` value, but mode is
output-only in this API and active low applies to both directions;
folding it in would have meant an input with a mode. The inversion is
the kernel's, so read, write, `defval` and the reported edges are all
logical with no code in bitters to keep consistent.


One interrupt thread per gpiochip
---------------------------------

Callbacks run on a thread bitters starts per controller. It copies the
descriptor table under `irq_lock`, `ppoll()`s the copy, and for each ready
line that is *still* registered and *still* on the descriptor it polled, it
releases `irq_lock`, runs the callback, and retakes it.

Two properties follow, and both are things callers rely on. Registering or
disabling a pin from another thread never races the poll, because the poll
is on a private copy and `BITTERS_SIGIRQ` (`SIGUSR1` by default) interrupts
it when a registration changes -- the signal is unblocked only inside
`ppoll()`, so it cannot be lost between the check and the wait. And a
callback may itself enable, disable or re-register pins, including
siblings, because the lock is not held while it runs.


The transmit buffer is const
----------------------------

`bitters_spi_transfer.tx` is `const uint8_t *`: bitters only hands it to
the kernel, which only reads it. As a plain `uint8_t *` the commonest
transmit buffer, a constant command table, could only go in with a cast
that drops `const`, and a cast in every caller teaches the wrong thing.
It does nothing for a string literal: that is a `char[]`, so the mismatch
there is `char` against `uint8_t`, not a qualifier. Constifying is
source-compatible for anyone who assigns to the field; only code reading
a pointer back out of `.tx` into a non-const one is affected. `rx` stays
writable, it is written. `t_spi_args` pins the qualifier at compile
time, so it holds without `-Werror`.

One Makefile for two makes
--------------------------

The Makefiles are written to the intersection of GNU make and BSD make: no
`ifeq`, no pattern rules, no `$(shell)` or `$(wildcard)`. Feature selection
is variable indirection -- `THREADS=yes` selects `$(T_yes)` -- which both
expand identically, and the source list comes from `!=` rather than a glob.

`featurecheck` catches the combination a per-variable default cannot:
`GPIO_IRQ=yes` with `THREADS=no` is a contradiction, and `$(error)` is
GNU-only, so it is a prerequisite target with a shell test rather than a
parse-time check.

The project's warning set lives in `$(WARNINGS)` and is always applied,
never in `CFLAGS`. BSD make's `sys.mk` predefines `CFLAGS`, so `CFLAGS ?=`
never takes there and the warnings would be dropped on exactly one of the
two makes, silently. `CFLAGS` stays the user's.


check is preflight, tests is the suite
--------------------------------------

Two different questions, and conflating them costs what each is for.

`make check` asks *is this tree fit to build*. It runs none of bitters'
code: the manifest still describes the tree, both text parses of the
version agree with what the compiler makes of the file, the tag does not
contradict the header, every vendoring subset still compiles and still
refuses what it must (`-fsyntax-only` and link, nothing run). It is
cheap, it needs nothing built, and it is the first thing to run when
something looks wrong.

`make tests` asks *does the built thing behave*. It builds the test
programs and runs them; `make tests-gpio` does the same against a virtual
gpiochip, and `make tests-endian` inspects cross-compiled codegen.

`make tag` runs both before it tags, because tagging something the suite
has not passed is the mistake it exists to prevent.
