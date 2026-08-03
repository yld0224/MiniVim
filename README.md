# MiniVim

MiniVim is a small Vim-like terminal editor written in C++17 for POSIX
terminals. It is maintained as a teaching reference for first-year students:
the goal is to demonstrate clear module boundaries, terminal I/O, parser
state, cursor invariants, and basic text editing rather than reproduce all of
Vim.

The core editing milestone is complete. MiniVim can open or create a buffer,
edit text in Insert mode, execute common Normal-mode commands, track unsaved
changes, and write the buffer to disk.

## Build and test

```sh
make
make test
./MiniVim [file]
```

The Makefile builds the modular implementation under `src/`. Running MiniVim
without a file creates a `[No Name]` buffer; use `:w filename` to name and save
it.

## Normal-mode movement

- `h`, `j`, `k`, `l` or the arrow keys: move the cursor
- `w`, `b`, `e`: move by word start, previous word start, or word end
- `0`, `^`, `$`: start, first non-blank, or end of line
- `gg`, `G`: first or last line
- `H`, `M`, `L`: top, middle, or bottom of the visible window
- `Ctrl-U`, `Ctrl-D`: move half a page
- `Ctrl-B`, `Ctrl-F`, Page Up, Page Down: move a full page
- `[count]` before a motion, for example `12j`, `3w`, or `3G`

Normal-mode horizontal movement stops at line boundaries, and vertical
movement preserves the desired display column across shorter lines.

## Editing commands

- `i`, `a`: enter Insert mode before or after the cursor
- `I`, `A`: enter Insert mode at the first non-blank or end of the line
- `o`, `O`: open a new line below or above the current line
- `x`: delete the character under the cursor
- `dd`: delete the current line
- `D`: delete from the cursor through the end of the line
- `J`: join the current line with the next line

Counts are supported by `x`, `dd`, and `J`, for example `3x`, `2dd`, and
`3J`. General operator-plus-motion commands such as `dw` and `d$` are not yet
implemented.

In Insert mode, printable ASCII characters and Tab insert text. Enter splits a
line, Backspace and Delete remove text or join adjacent lines, and Escape
returns to Normal mode.

## Command-line commands

- `:w [file]`, `:write [file]`: write the current buffer
- `:wq [file]`: write and quit
- `:x [file]`: write modified text and quit
- `:q`, `:quit`: quit only when there are no unsaved changes
- `:q!`, `:quit!`: discard unsaved changes and quit
- `:help`: show the compact command reminder
- `Ctrl-Q`: emergency exit from any mode

The status line displays `[+]` while the buffer has unsaved changes. Supplying
a path to a write command updates the name of a `[No Name]` buffer.

## Architecture

- `Terminal`: raw-mode lifetime, key decoding, terminal size, output
- `Buffer`: text ownership, mutation, dirty state, loading, and saving
- `Window`: cursor, desired column, and viewport
- `NormalCommandParser`: converts Normal-mode key sequences into actions
- `Renderer`: converts editor state into an ANSI frame
- `Editor`: coordinates modes and executes actions

The main data flow is:

```text
Terminal -> KeyEvent -> NormalCommandParser -> EditorAction
         -> Editor -> Buffer/Window -> Renderer -> Terminal
```

`Buffer` never exposes mutable line references, `Window` owns cursor and
viewport state, and `Terminal` is the only POSIX I/O boundary.

## Course-project status and next step

The implemented feature set is sufficient for the required course-project
mainline. The next development stage should prioritize verification and
explanation rather than adding more Vim commands:

1. Add unit tests for Buffer mutations and saving, parser actions and counts,
   `w`/`b`/`e`, cursor updates after edits, and dirty-state rendering.
2. Add PTY integration tests for complete workflows such as
   `i -> edit -> Escape -> :w -> :q` and unsaved-change protection.
3. Add focused teaching comments around invariants and non-obvious state
   transitions. Avoid comments that merely repeat individual statements.

Undo/redo, registers and yank/paste, `/` search, general
`[count] + operator + motion`, and syntax highlighting are suitable optional
extensions after the tested baseline is stable.

## Current limitations

- Text input and word classification are byte-oriented; full Unicode input and
  terminal cell-width handling are outside the current scope.
- Insert-mode cursor movement, `W`/`B`/`E`, undo, registers, search, and general
  operator-pending commands are not implemented.
- Writing to an explicitly supplied path uses simplified teaching semantics and
  replaces an existing target without Vim's `!` confirmation rules.
- The checked-in test suite currently covers only the earlier core components;
  most editing and saving behavior still needs dedicated tests.
