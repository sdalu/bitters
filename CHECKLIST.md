# Closing a round on bitters

What has to be true before a unit of work here is done. Every line is a
command or a question with a yes or a no at the end of it.

bitters is a **Linux** library: `make tests` needs `linux/*.h`, and
`make tests-gpio` needs root and the `gpio-mockup` module. A round done on
another host is not closed until the gates have run on a Linux one.


## Gates

Run in order. A red gate is answered before anything else, and the answer
is sometimes that the checker is wrong rather than the file.

- [ ] `make check` -- preflight: the manifest, the version readers against
      the compiler, the tag, the vendoring subsets. Runs none of bitters'
      code, so it runs anywhere and it runs first.
- [ ] `make tests` -- the suite that needs no privilege.
- [ ] `make tests-gpio` -- **if this round touched `src/gpio.c`.** Needs
      root, and `modprobe gpio-mockup gpio_mockup_ranges=-1,8,-1,8` first:
      two ranges, or `t_cross_teardown` reports SKIP rather than passing.
- [ ] `make tests-endian` -- **if this round touched the i2c bitfield view
      of `dir`, or `include/bitters/i2c.h`.**
- [ ] `make WERROR=yes` -- the README promises a warning-free tree and CI
      uses this; a new warning is a failure here, not a note.
- [ ] `make check && make tests` under **both** makes -- **if this round
      touched a Makefile.** GNU make and BSD make, and the project's claim
      is that they agree.


## Documents

Re-read each against what this round changed: a document that was true
this morning is a claim, not a fact.

- [ ] `README.md` -- does it still name only flags, paths, targets and
      commands that exist? Does a reader driving bitters the way this
      round changed it get the right answer?
- [ ] `DESIGN.md` -- did this round take a decision? Write it down here,
      with the alternative it rejected and why, in the same commit.
- [ ] `tests/README.md` -- a new test needs its row in *What each test
      pins down*, saying which defect it guards against. A guarantee that
      moved needs its row changed.
- [ ] `CLAUDE.md` -- only two kinds of line belong there: where things
      are, and a trap that cost this round time. Would the README or the
      code have answered it? Then it does not go there.


## Release

- [ ] Does the number move? A change that alters no behaviour usually
      does not. The one file that holds it:
      `include/bitters/version.h`.
- [ ] If it moves: bump the three numbers there, commit, then `make tag`.
      The tag is read out of the header, never typed. `make tag` refuses
      an unclean worktree and an existing tag, runs `check` and `tests`,
      and pushes nothing -- the push stays yours.
- [ ] Nothing else anywhere holds a copy of the number. If this round
      added a reader of `include/bitters/version.h`, then
      `tests/check-manifest.sh` has to know about it, or the new reader is
      ungated.
