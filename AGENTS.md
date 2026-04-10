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
| `AGENTS.md` | Agent instructions (you are here) |
| `STATUS.md` | Persistent memory -- recent changes, bugs, workarounds (~150 lines max) |
| `CHANGELOG.md` | User-facing change history |
| `design/DESIGN.md` | Game design document |
| `docs/REFERENCE.md` | Lookup material -- repo layout, environment, PR how-tos |

## Gates

**Mandatory checkpoints. Do not skip any step.**

### Before presenting results

**Stop and ask yourself:**

- Is this the best solution or just the first that works? Could it be simpler? If the answer is unsatisfying, iterate.
- Do not present results you would not follow yourself.
- If you identify a problem -- fix it. Obvious fixes: fix directly. Judgment calls: present with a recommended fix and apply unless the user redirects.
- Never rationalize a problem away or leave it for the user to catch.

### Before broad implementation

Before adding a new system, changing more than three files, refactoring across module boundaries, or making architectural changes: **plan -> user review -> implement.** Routine changes that touch a `.c` and its `.h` do not require a plan.

### When the user corrects you

Add the rule to `AGENTS.md` immediately. Do not wait until commit time. Skip for one-off mistakes (e.g. a typo).

### Before starting work

1. Run `git status`. Stash or resolve uncommitted changes before proceeding.
2. Check out `main` and pull latest.
3. Create a new branch from `main`. Do not commit or cherry-pick directly onto `main` -- all changes go through a pull request.
   - Branch names must match: `^[a-z0-9]+(?:[-\/][a-z0-9]+)*$`. Examples: `bilal/add-player-movement`, `feature/weapon-system`.
   - If resuming an existing feature branch, stay on it and merge or rebase from `main`.
   - **Parallel sessions:** Do not use `git checkout` when another session may be active. Use `git worktree add <path> <branch>` instead. Clean up with `git worktree remove <path>`.

### Before every commit

Do not commit until all are satisfied.

1. Run `task fmt`. Fix any issues.
2. Run the game if applicable. Skip if not yet buildable.
3. Run tests if they exist.
4. Update `STATUS.md` if this commit changes observable game behavior, project structure, dependencies, or known issues.
5. Update `CHANGELOG.md` under `[Unreleased]` if user-facing. Changes to `AGENTS.md` are always changelog-worthy.
6. Commit with a clear message -- one logical change per commit.
7. Present a brief summary: what changed, files affected, branch state.

### When done

Run these when the user asks to push/PR/merge, or when committing the final change of a task.

1. Restate the original goal and verify the work achieves it.
2. Review your diff -- look for accidental changes, debug leftovers, anything that doesn't belong.
3. Note out-of-scope improvements in `STATUS.md` under "Known Issues / Next Steps".
4. Update relevant documentation for new features or behavior changes.
5. Document new patterns and conventions in `AGENTS.md`.

### Before creating a PR

- Ask for confirmation before the initial push -- unless the user already asked you to push/create a PR.

### After PR feedback

- Only look at **unresolved** conversations. Do not re-review resolved threads.
- Mark each conversation as resolved via the API as you address it. Re-request reviews from all reviewers via the API (not comments).
- After re-requesting reviews, provide the PR URL to the user.
- Update `CHANGELOG.md` when feedback results in meaningful changes. Consolidate related entries.

### Before merging

- Complete all "When done" steps. Once merged and deleted, you cannot push to the branch.
- All review conversations must be resolved -- branch protection enforces this.

## Development Lifecycle

For non-trivial changes (skip for docs-only or single-line fixes):

1. **Plan** -- State the problem, constraints, and definition of done.
2. **Simplify** -- How could this be simpler? Revise the plan.
3. **Write a failing test** -- Express expected behavior as a Unity test before implementation. For bugs, reproduce first.
4. **Implement** -- Minimum code to pass the test.
5. **Green** -- Run `task test`, fix, repeat.
6. **Simplify again** -- Refactor and re-run tests.
7. **Self-maintain** -- Check whether `AGENTS.md`, `STATUS.md`, or `CHANGELOG.md` need updating. Triggers:
   - User corrected you or reminded you of something -> `AGENTS.md`.
   - A command/approach failed before succeeding -> document the working pattern in `AGENTS.md`.
   - Conventions or known issues changed -> `AGENTS.md` and `CHANGELOG.md`.
   - Observable game behavior, structure, or dependencies changed -> `STATUS.md`.

## Conventions

### Writing rules in this file

- Direct, imperative language -- no "consider", "try to", "should ideally".
- Every rule needs a clear trigger and verifiable outcome.
- Prefer qualitative wording over fragile numeric thresholds.

### Change philosophy

- Workshop project. Bias toward getting things working and iterating.
- Prefer the minimal change. Keep it simple.
- Clear code over clever code. No abstractions unless they solve a repeated, concrete problem.

### Code

- All code changes need tests. Bug fixes: reproduce first, then fix. Features: test the new behavior.
- One logical change per commit. No unrelated refactors in the same commit.
- Document all structs, public functions, and modules in header files. No excessive inline comments.
- When adding dependencies, verify latest version and API against current docs -- do not rely on training data.
- When a bug's root cause is unclear, add logging first and reproduce -- do not guess.

### C code style

- **Standard:** C99 (`-std=c99`).
- **Naming:** `snake_case` for functions/variables/files. `UPPER_SNAKE_CASE` for constants/macros. `PascalCase` for typedefs (e.g. `typedef struct { ... } Player;`).
- **Indentation:** 4 spaces, no tabs.
- **Braces:** Same line. Always use braces, even for single-line bodies.
- **Headers:** Each `.c` has a `.h`. Use `#pragma once` or include guards. Public API in header, internal helpers `static` in `.c`.
- **Warnings:** `-Wall -Wextra`. Treat as errors to fix, not suppress.

### Linting

`task fmt` detects changed file types and runs relevant linters:

| Changed files | Linter | Config |
|---------------|--------|--------|
| `*.c`, `*.h` | clang-format (auto-fix) | `.clang-format` |
| `*.yml`, `*.yaml` | yamllint (check-only) | `.yamllint.yml` |
| `*.md` | markdownlint-cli2 (check-only) | `.markdownlint-cli2.yaml` |

`Taskfile.yml` is excluded from yamllint (Go template syntax). After editing it, run `task --list-all` to verify it still parses.

### STATUS.md

- Rolling log of project state. Keep under ~150 lines.
- Update when a commit changes observable game behavior, structure, dependencies, or known issues.
- Sections: Current State, Recent Changes, Known Issues / Next Steps, Workarounds & Patterns.

### Design documents

- `design/DESIGN.md` is the source of truth for game design. Reference assets go in `design/assets/`.
- Capture design decisions during implementation -- do not leave rationale only in commit messages.

## Efficiency

### Delegate mechanical tasks

Use the lightweight subagents in `.opencode/agents/` for routine checks:

- `@lint-check` -- runs `task fmt`, reports issues (Haiku)
- `@build-and-test` -- builds and runs tests, reports failures (Haiku)
- `@changelog-drafter` -- drafts CHANGELOG.md entries from diffs (Haiku)

### Context hygiene

- Use the `explore` subagent for broad codebase searches -- results stay in an isolated context and only the summary returns.
- `gh api`: always use `--jq` to filter fields. Prefer over MCP GitHub tools when you only need specific fields.
- Do not re-read files you just wrote or edited. Trust the tool's success response.
- Filter non-actionable build noise (Raylib warnings). Keep compiler warnings visible.
- Merge from main once at session start. One final merge/rebase before push if main changed.
- Before creating new modules, check whether existing code or Raylib already provides the functionality.
