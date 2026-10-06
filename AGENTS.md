# Repository Guidelines

## Project Structure & Module Organization

This repository is a C implementation of `ft_traceroute`. Put implementation files in `sources/` and public declarations in `includes/`. The Makefile currently expects `sources/main.c`; add other `.c` files to its `SRC` list as the program grows. Object files go in `objs/`, and the executable is created at the repository root. `ft_traceroute.en.pdf` is the assignment specification; check it when deciding required behavior. There is no test directory yet.

## Build, Test, and Development Commands

- `make` builds `ft_traceroute` with `gcc`, `-Wall -Wextra -Werror`, and `-lm`. The build will fail until `sources/main.c` exists.
- `make clean` removes `objs/`; `make fclean` also removes the executable.
- `make re` performs a clean rebuild.
- After implementing the program, run `./ft_traceroute 127.0.0.1` for a local smoke test. Network probes may require suitable socket permissions.

## Coding Style & Naming Conventions

Follow the C style used in the neighboring `ft_ping` project: indent with tabs, place opening braces on the same line as functions and control statements, and use `snake_case` for functions and variables. Name structure tags `s_...` and their typedefs `t_...` (for example, `s_traceroute` and `t_traceroute`); use uppercase names for macros. Start headers with `#pragma once`, declare shared interfaces in `includes/`, and mark file-local helpers `static`. Use parenthesized return values, such as `return (0);`. Check system call failures and release resources on error paths. No formatter or linter is configured; compile with the Makefile's warning flags.

## Testing Guidelines

There is no test framework or coverage target yet. For each change, run `make re`, then test a reachable local target and at least one failure case, such as an invalid host or argument. If automated tests are added, put them in `tests/`, name files after the behavior they exercise, and document their command in the Makefile or this guide.

## Commit & Pull Request Guidelines

Git history contains only the initial `pdf` commit, so no commit convention is established. Use short, imperative commit subjects that describe the change, such as `Handle DNS lookup errors`. Pull requests should explain the behavior changed, list the commands and targets tested, and reference a related issue when one exists. Include terminal output for changes to traceroute output formatting.
