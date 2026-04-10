# Project: Astro Blitz

Top-down sci-fi roguelike shooter built during Fatshark's agentic coding workshop.

## Quick Reference

| Task | Command |
|------|---------|
| Run the game | *(TBD -- depends on chosen engine/framework)* |
| Run tests | *(TBD)* |
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

Before changing more than one file, refactoring, or implementing anything beyond a targeted fix: produce a plan and get it reviewed by the user. Do not start implementation until the user approves the plan. Workflow: **plan -> user review -> implement.**

### When the user corrects you

When the user corrects you or reminds you of something you missed, update `AGENTS.md` in your next action. Do not wait until commit time.

### Before starting work

1. Run `git status`. If there are uncommitted changes, stash or resolve them before proceeding.
2. Check out `main` and pull the latest changes.
3. Create a new branch from `main` -- `main` is protected: do not commit directly to it or push commits straight to it. All changes to `main` must go through a pull request.
   - Branch names: lowercase, digits, slashes, hyphens only (e.g. `bilal/add-player-movement`, `feature/weapon-system`)

### Before every commit

Do not commit (or amend) until all are satisfied.

1. Run the game (if applicable) to verify nothing is visibly broken.
2. Run tests (if they exist) to verify changes don't break existing behavior.
3. Verify that any user corrections from this session have been captured in `AGENTS.md`.
4. If any command, API call, or approach failed before succeeding, document the working pattern in `AGENTS.md` so future sessions don't repeat the failed attempts.
5. Update `STATUS.md` with what changed and any new bugs or workarounds discovered.
6. Update `CHANGELOG.md` under `[Unreleased]` if the change is user-facing.
7. Commit with a clear message. Keep commits small and logical -- one logical change per commit.
8. After committing, present a brief summary to the user: what changed, what files were affected, and the current state of the branch.

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

- All new gameplay systems need at least basic tests when the test infrastructure exists.
- No unrelated refactors in the same commit.
- When adding dependencies, verify the latest version and API against current docs -- do not rely on training data.
- When a bug's root cause is unclear, add logging first and reproduce -- do not guess at fixes.

### STATUS.md maintenance

- `STATUS.md` is a rolling log of project state. Keep it under ~150 lines.
- When it grows too long, summarize older entries and remove resolved items.
- Every commit that changes behavior must update `STATUS.md`.
- Sections: Current State, Recent Changes, Known Issues / Next Steps, Workarounds & Patterns.

### Design documents

- `design/DESIGN.md` is the source of truth for game design decisions.
- Reference images, concept art, and mockups go in `design/assets/`.
- When a design decision is made during implementation, capture it in `design/DESIGN.md` -- do not leave design rationale only in commit messages or PR descriptions.

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
| `src/` | Game source code |
| `src/main.c` | Entry point -- window init, main loop |
| `CMakeLists.txt` | Build configuration -- fetches Raylib 5.5 via FetchContent |
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
- **Asset formats:** PNG (sprites), WAV/OGG (audio)
- **Platform:** Windows (primary), cross-platform possible via Raylib
