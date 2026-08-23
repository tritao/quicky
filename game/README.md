# Bundled DOS runtime

This directory contains the patched DKIA runtime copy of Tricky Quiky Games
II / Nesquik / Quiky. The [DKIA article](https://www.dkia.at/en/node/76) says
that its archive is patched for PCs faster than 100MHz and that the game may
be copied for private, non-commercial use.

The source archive was DKIA's `quiky.zip`:

```text
SHA-256 d62123cc60f39c3e5d6769607bd498cbe2334e2ac3cf666d01d695cf9d2bca53
```

The bundled files are the archive's complete runnable set:

```text
DPMI16BI.OVL  DOS protected-mode support
NESTLE.DAT    game data archive
NESTLE.INI    configuration (German, Sound Blaster 220/IRQ 7/DMA 1)
QUIKY.EXE     game executable
RTM.EXE       Borland runtime
SCORE.DAT     score data
SETUP.EXE     configuration utility
```

Run `../scripts/run-dosbox.sh setup` once if configuration needs changing,
then run `../scripts/run-dosbox.sh` to start the game. The launcher mounts
only this directory as DOSBox's C: drive and starts at 16,000 cycles.

Before pushing this repository to a public remote, verify that the stated
private/non-commercial permission covers that form of redistribution.
