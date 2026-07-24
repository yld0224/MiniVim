# MiniVim

A small terminal text viewer that is being evolved into a Vim-like editor.
The project targets modern C++17 and POSIX terminals.

## Build and test

```sh
make
make test
./MiniVim [file]
```

`MiniVim.cpp` is retained as the original single-file reference. The Makefile
builds the implementation under `src/`.

## Viewer commands

- `h`, `j`, `k`, `l` or the arrow keys: move the cursor
- `0`, `^`, `$`: start, first non-blank, or end of line
- `gg`, `G`: first or last line
- `[count]` before a motion, for example `12j` or `3G`
- `H`, `M`, `L`: top, middle, or bottom of the visible window
- `Ctrl-U`, `Ctrl-D`: move half a page
- `Ctrl-B`, `Ctrl-F`, Page Up, Page Down: move a full page
- `:q`, `:quit`, or `Ctrl-Q`: quit
- `:help`: show the compact command reminder

Normal-mode horizontal movement stops at line boundaries, and vertical
movement preserves the desired display column across shorter lines.

## Architecture

- `Terminal`: raw-mode lifetime, key decoding, terminal size, output
- `Buffer`: file-backed text and buffer invariants
- `Window`: cursor, desired column, and viewport
- `NormalCommandParser`: converts Normal-mode key sequences into actions
- `Renderer`: converts read-only state into an ANSI frame
- `Editor`: coordinates modes and executes actions

The next editing stage can add Buffer mutation methods, dirty/save state and
Insert mode without putting POSIX I/O into the text model. Operator-pending
support can extend `NormalCommandParser` so commands are represented as
`[count] + operator + motion` rather than one-off key combinations.

The current display-column implementation is intentionally small: tabs and
ASCII control characters are handled, while full Unicode terminal-width
support is outside the current scope.
