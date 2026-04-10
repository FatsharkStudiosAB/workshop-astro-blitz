# Project: Astro Blitz

Top-down sci-fi roguelike shooter built during Fatshark's agentic coding workshop.

## Quick Reference

| Task | Command |
|------|---------|
| Configure build | `task configure` |
| Build the game | `task build` |
| Run the game | `task run` (or just `task`) |
| Clean build artifacts | `task clean` |
| Run tests | `task test` |
| Build tests only | `cmake --build build --target test_sample` |
| Run tests (ctest) | `ctest --test-dir build -C Debug --output-on-failure` |
| Lint changed file types | `task fmt` |
| Lint all files | `task fmt:all` |
| List all tasks | `task --list-all` |

## Key Files

| File | Purpose |
|------|---------|
| `AGENTS.md` | Agent instructions -- how to work in this repo (you are here) |
| `STATUS.md` | Persistent memory -- recent changes, bugs, workarounds (~150 lines max) |
| `CHANGELOG.md` | User-facing change history |
| `design/DESIGN.md` | Game design document |
| `design/assets/` | Reference images, mockups, sprites |

## Gates

**These are mandatory checkpoints, not suggestions. Do not skip any step.**

### Before presenting results

**Before presenting any result to the user -- stop.**

- Is this the best solution or just the first one that works? Could it be simpler?
- Do not present results you would not follow yourself.
- Do not rely on the user to catch issues you can already see.
- If you identify a problem -- fix it. For obvious fixes, fix directly. For judgment calls, present the problem with a recommended fix and apply it unless the user redirects.
- Never identify a problem and then rationalize it away or leave it for the user to catch.

### Before broad implementation

Before adding a new system, changing more than three files, refactoring across module boundaries, or making architectural changes: produce a plan and get it reviewed by the user. Do not start implementation until the user approves the plan. Workflow: **plan -> user review -> implement.** Routine changes that touch a `.c` and its `.h` file do not require a plan.

### When the user corrects you

When the user corrects you on a workflow pattern or convention not already covered in this file, add the rule to `AGENTS.md` in your next action. Do not wait until commit time. If the correction is a one-off mistake (e.g. a typo), do not add a rule.

### Before starting work

1. Run `git status`. If there are uncommitted changes, stash or resolve them before proceeding.
2. Check out `main` and pull the latest changes.
3. Create a new branch from `main` -- `main` is protected: do not commit directly to it or push commits straight to it. All changes to `main` must go through a pull request.
   - Branch names: lowercase, digits, slashes, hyphens only (e.g. `bilal/add-player-movement`, `feature/weapon-system`)
   - If resuming work on an existing feature branch, stay on that branch and merge or rebase from `main` instead of creating a new branch.

### Before every commit

Do not commit (or amend) until all are satisfied.

1. Run `task fmt` to catch style issues. This automatically detects which file types changed and runs only the relevant linters (clang-format, yamllint, markdownlint-cli2). Fix any issues before proceeding.
2. Run the game (if applicable) to verify nothing is visibly broken. Skip this step if the game is not yet buildable (e.g. missing source files or prerequisites).
3. Run tests (if they exist) to verify changes don't break existing behavior.
4. Verify that any user corrections from this session have been captured in `AGENTS.md`.
5. If any command, API call, or approach failed before succeeding, document the working pattern in `AGENTS.md` so future sessions don't repeat the failed attempts.
6. Update `STATUS.md` if this commit changes observable game behavior, project structure, dependencies, or introduces/resolves a known issue (see STATUS.md maintenance rules below).
7. Update `CHANGELOG.md` under `[Unreleased]` if the change is user-facing.
8. Commit with a clear message. Keep commits small and logical -- one logical change per commit.
9. After committing, present a brief summary to the user: what changed, what files were affected, and the current state of the branch.

### When done

When the user asks to push, create a PR, or merge -- run these completion steps first.

1. Restate the original goal and verify the work achieves it.
2. Review your diff (`git diff`) -- look for accidental changes, debug leftovers, or anything that doesn't belong.
3. If you identify valid improvements that are out of scope, note them in `STATUS.md` under "Known Issues / Next Steps" so they aren't lost.
4. When adding new features or changing behavior, update relevant documentation.
5. Document new patterns and conventions in `AGENTS.md` so future sessions can follow them.

### Before creating a PR

- Use PowerShell heredoc syntax (`@' ... '@`) when formatting PR body via `gh pr create`.
- Ask for confirmation before the initial push -- unless the user already explicitly asked you to push and/or create a PR.

## Development Lifecycle

Follow this lifecycle for non-trivial changes (skip for documentation-only or single-line fixes):

1. **Plan** -- State the problem, constraints, and definition of done. If you can't restate the goal, the plan isn't ready.
2. **Simplify** -- Step back: how could this be simpler and less clever while still achieving the goal? Revise the plan.
3. **Write a failing test** -- Express expected behavior as a Unity test before implementation. For bug fixes, write a test that reproduces the bug first.
4. **Implement** -- Minimum code to pass the test.
5. **Green** -- Run `task test`, fix, repeat until green.
6. **Simplify (second pass)** -- Could this be simpler? Refactor and re-run tests.
7. **Self-maintain** -- Check whether `AGENTS.md`, `STATUS.md`, or `CHANGELOG.md` need updating. See `## Self-Maintenance` for the full trigger list.

## Conventions

### Writing rules in this file

- Use direct, imperative language -- no "consider", "try to", "should ideally".
- Be unambiguous -- every rule needs a clear trigger ("when X happens, do Y") and a verifiable outcome.
- Place rules at the correct scope -- general conventions go in this section, not under feature-specific subsections.

### Change philosophy

- This is a workshop project. Bias toward getting things working and iterating.
- Prefer the minimal change that achieves the goal. Keep it simple.
- Write clear code over clever code. Do not introduce abstractions unless they solve a repeated, concrete problem.

### Code

- All code changes need tests -- new features, bug fixes, and refactors. When fixing a bug, write a test that reproduces the bug first, then fix it. When adding a feature, write tests covering the new behavior. Do not submit code changes without corresponding test coverage.
- One logical change per commit. Commit after each logical unit of work is complete -- do not accumulate multiple unrelated changes in the working tree.
- No unrelated refactors in the same commit.
- Document all structs, public functions, and modules with comments in the header file. No excessive inline comments inside function bodies.
- When adding dependencies, verify the latest version and API against current docs -- do not rely on training data.
- When a bug's root cause is unclear, add logging first and reproduce -- do not guess at fixes.

### C code style

- **Standard:** C99 (`-std=c99`).
- **Naming:** `snake_case` for functions, variables, and file names. `UPPER_SNAKE_CASE` for constants and macros. `PascalCase` for typedefs of structs/enums (e.g. `typedef struct { ... } Player;`).
- **Indentation:** 4 spaces, no tabs.
- **Braces:** Opening brace on the same line as the statement. Always use braces for `if`/`else`/`for`/`while`, even for single-line bodies.
- **Headers:** Each `.c` file has a corresponding `.h` file. Use `#pragma once` or traditional include guards. Public API in the header, internal helpers `static` in the `.c` file.
- **Compiler warnings:** Compile with `-Wall -Wextra`. Treat warnings as errors to fix, not suppress.

### Linting and formatting

`task fmt` detects which file types have uncommitted changes and runs only the relevant linters:

| Changed files | Linter | Config |
|---------------|--------|--------|
| `*.c`, `*.h`, `.clang-format` | clang-format (auto-fix) | `.clang-format` |
| `*.yml`, `*.yaml`, `.yamllint*` | yamllint (check-only) | `.yamllint.yml` |
| `*.md`, `.markdownlint*` | markdownlint-cli2 (check-only) | `.markdownlint-cli2.yaml` |

`Taskfile.yml` is excluded from yamllint because it uses Go template syntax (`{{.VAR}}`) that is not valid YAML. No Taskfile-specific linter exists. When editing `Taskfile.yml`, always run `task --list-all` to verify the file still parses -- YAML quoting errors around Go templates are the most common breakage.

### STATUS.md maintenance

- `STATUS.md` is a rolling log of project state. Keep it under ~150 lines.
- Before committing, if `STATUS.md` exceeds 150 lines, summarize older entries and remove resolved items to bring it back under the limit.
- Update `STATUS.md` when a commit changes observable game behavior, project structure, dependencies, or introduces/resolves a known issue. Purely internal refactors and comment-only changes do not require an update.
- Sections: Current State, Recent Changes, Known Issues / Next Steps, Workarounds & Patterns.

### Design documents

- `design/DESIGN.md` is the source of truth for game design decisions.
- Reference images, concept art, and mockups go in `design/assets/`.
- When a design decision is made during implementation, capture it in `design/DESIGN.md` -- do not leave design rationale only in commit messages or PR descriptions.

## Self-Maintenance

At the end of every work session, check whether changes made in this session affect conventions or patterns documented in `AGENTS.md`, `STATUS.md`, or `CHANGELOG.md`. If they do, update the relevant files. The triggers:

- If the user corrected you or reminded you of something, add it to `AGENTS.md`.
- If any command, API call, or approach failed before succeeding, document the working pattern in `AGENTS.md`.
- If conventions, commands, or known issues changed, update `AGENTS.md` and `CHANGELOG.md`.
- If the commit changes observable game behavior, project structure, or dependencies, update `STATUS.md`.

## Efficiency

- When using `gh api`, always use `--jq` to filter response fields.
- Use the Task tool with `explore` or `general` subagents for broad codebase searches.
- Use `/compact` to manually trigger context compaction when a session gets heavy.

---

## Reference

Everything below this line is lookup material. Behavioral rules are all above.

## Repository Layout

| Path | What |
|------|------|
| `Taskfile.yml` | Task runner configuration (go-task) |
| `src/` | Game source code |
| `src/main.c` | Entry point -- window init, main loop |
| `tests/` | Unit tests (Unity framework) |
| `tests/test_sample.c` | Sample test verifying framework works |
| `CMakeLists.txt` | Build configuration -- fetches Raylib 5.5 + Unity 2.6.1 via FetchContent |
| `build/` | Build output (gitignored) |
| `design/` | Game design documents and reference assets |
| `design/DESIGN.md` | Game design document |
| `design/assets/` | Reference images, mockups, sprites |
| `STATUS.md` | Persistent memory -- recent changes, bugs, workarounds |
| `CHANGELOG.md` | User-facing change history |
| `AGENTS.md` | Agent instructions (this file) |

## Environment

- **Engine/Framework:** Raylib 5.5
- **Language:** C (C99)
- **Build:** CMake with FetchContent (auto-downloads Raylib). Configure: `cmake -B build -G "Visual Studio 17 2022" -A x64`. Build: `cmake --build build --config Release`. Executable: `build/Release/astro_blitz.exe`.
- **Test framework:** Unity (ThrowTheSwitch) v2.6.1 via FetchContent
- **Asset formats:** PNG (sprites), WAV/OGG (audio)
- **Platform:** Windows (primary), cross-platform possible via Raylib
