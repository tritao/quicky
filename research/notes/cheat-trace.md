# Cheat-input trace

Status: static executable analysis is confirmed; the debugger has also
verified the runtime segment selectors and reached the comparison branch.
The successful branch has not yet been captured with a real typed sequence.

## Confirmed executable facts

`QUIKY.EXE` is an NE protected-mode executable. The relevant runtime selectors
in DOSBox's debugger are:

| NE segment | Runtime selector | Role |
| ---: | ---: | --- |
| 1 | `01D7` | game code and the length-prefixed cheat strings |
| 3 | `01F7` | keyboard interrupt/polling code |
| 4 | `0207` | string-buffer helper routines |
| 5 | `0227` | string comparison routines |
| — | `0237` | current data selector during the game loop |

The cheat table is in NE segment 1. Each entry is a Pascal-style
length-prefixed string:

```text
01 49                         # unrelated preceding data
0E QUIKYISTHEBEST
0E GOQUIKYGOQUIKY
0E NESQUIKISGREAT
0E RUNQUIKYRUNRUN
0E THROWQUIKYDOIT
0E QUIKYSUPERHERO             # length byte at 0x1065, text at 0x1066
04 INFO
05 SOUND
04 EXIT
```

The input/cheat loop is in NE segment 1. Its important local offsets are:

```text
0x11B5  far call to segment 3:0xF1A8, fetch one queued keyboard scan code
0x11B9  translate Q-P, A-L, Z-M, and Space scan codes to characters
0x1231  call segment 4:0x1115 to prepare the current input string
0x128B  compare with segment 1:0x101A (QUIKYISTHEBEST)
0x12B1  compare with segment 1:0x1029 (GOQUIKYGOQUIKY)
0x12D7  compare with segment 1:0x1038 (NESQUIKISGREAT)
0x12FD  compare with segment 1:0x1047 (RUNQUIKYRUNRUN)
0x1323  compare with segment 1:0x1056 (THROWQUIKYDOIT)
0x1343  load segment 1:0x1065 (QUIKYSUPERHERO)
0x1349  call segment 5:0x0F78, compare the input string
0x134D  branch if the comparison is not equal
0x134F  set byte DS:0x89F2 to 1 when the cheat matches
```

Segment 5 routine `0x0F78` is a length-aware string comparison: it reads the
length byte from each Pascal string, compares up to the shorter length with
`CMPSB`, then compares the lengths. This is the comparison used by the cheat
branch; it is not a guessed address derived from the visible text.

The keyboard code at segment 3 `0xF17F` is an interrupt handler. It reads the
keyboard controller at I/O port `0x60` into a ring buffer. Segment 3 `0xF1A8`
consumes that buffer and maps arrow scan codes to flags. The game-level input
loop then maps the letter scan-code ranges to characters. `P` is scan code
`0x19` in the Q-P range, so it is handled by the game and is not a DOSBox host
shortcut.

## Reproduce the debugger breakpoint

Run:

```sh
./scripts/run-dosbox-debug.sh
```

Press `Alt+Pause` to enter the ncurses debugger, then enter:

```text
BP 01D7:1343
F5
```

The breakpoint should stop before the cheat comparison and show the call to
`0227:0F78`. `F5` resumes execution. The debug build's `Alt+Pause` debugger
is separate from the SDL game window.

## Skip typing the full cheat

The game does not jump to a level merely because the text is present: the
successful comparison first sets `DS:89F2`, and the next input selects the
level-jump path. The branch at `01D7:491D` checks that flag. Its `4` case is
the scan code `0x05`, which is why the article says to type `4` after the
cheat.

With the game paused in the debugger, use this sequence:

```text
BPDEL *
BP 01D7:491D
F5
```

When execution stops at `01D7:491D`, inject the post-cheat state and resume:

```text
SM DS:89F2 01
SM DS:88BA 05 00
F5
```

This is a debugger-only memory edit. It does not modify `QUIKY.EXE` or
`NESTLE.DAT`. The `4` path opens the game's level-selection route; selecting
`W1L3` afterward still exercises the original level-loading code. To jump
directly to a specific level without the selection screen, the next target is
the state written after `01D7:495D` (`DS:89F4`), which should be traced in the
debugger rather than guessed.

For a direct W1L3 selection after the selector appears, pause again and set
the selector index observed in the run:

```text
SM DS:85D4 02 00
F5
```

Then send Space to launch the selected level.

### Observed run

On 23 August 2026, this procedure was executed in the debug build. The
breakpoint showed `DS:89F2=1`; replacing the current key value with `DS:88BA=5`
entered the selector. Setting `DS:85D4=2` changed its visible label from
`Nature 1` to `Nature 3`, and pressing Space launched the level scene. The
mapping from that selector value to the archive label `W1L3` is supported by
the adjacent `Nature 1/2/3` and `W1L1/W1L2/W1L3` tables, while the screen and
launch were directly observed.

## Evidence labels

- Confirmed from executable bytes and NE relocation records: string addresses,
  comparison targets, keyboard routine locations, and the `DS:89F2` success
  flag.
- Confirmed experimentally in DOSBox's debugger: the protected-mode selector
  mapping, the runtime breakpoint at `01D7:1343`, the shortcut branch at
  `01D7:491D`, and the selector launch sequence.
- Not yet captured: the live `JE`/fall-through result after entering the full
  cheat through the emulated keyboard.
