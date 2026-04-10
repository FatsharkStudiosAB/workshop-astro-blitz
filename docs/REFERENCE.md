# Reference

Lookup material for the Astro Blitz project. Behavioral rules live in `AGENTS.md`.

## PR Reference

How-to patterns for PR operations. The gates for when to do these are in
[Before creating a PR](../AGENTS.md#before-creating-a-pr),
[After PR feedback](../AGENTS.md#after-pr-feedback), and
[Before merging](../AGENTS.md#before-merging).

- Use PowerShell heredoc syntax (`@' ... '@`) when formatting PR body via `gh pr create`.
- `gh pr edit --add-reviewer` does not work for bot accounts. Use the REST API instead:

  ```sh
  echo '{"reviewers":["username","bot-name[bot]"]}' | gh api repos/OWNER/REPO/pulls/NUMBER/requested_reviewers -X POST --input - --jq '.requested_reviewers[].login'
  ```

- Resolving PR review conversations requires the GraphQL API (`resolveReviewThread` mutation) -- the REST API does not support this.
- When re-requesting reviews from bot reviewers (e.g. Copilot), use their full login with the `[bot]` suffix: `copilot-pull-request-reviewer[bot]`.
- When resolving merge conflicts in `CHANGELOG.md`, recount entries afterward -- conflicts can silently combine entries from both sides.

## Repository Layout

| Path | What |
|------|------|
| `src/` | Game source code -- `main.c` (entry point), plus one `.c/.h` pair per module |
| `tests/` | Unit tests (Unity framework) -- one `test_<module>.c` per module |
| `design/` | Game design docs and reference assets |
| `docs/` | Project documentation and reference material |
| `CMakeLists.txt` | Build config -- fetches Raylib 5.5 + Unity 2.6.1 via FetchContent |
| `Taskfile.yml` | Task runner configuration (go-task) |
| `build/` | Build output (gitignored) |

## Environment

- **Engine/Framework:** Raylib 5.5
- **Language:** C (C99)
- **Build:** CMake with FetchContent (auto-downloads Raylib). Use `task configure` / `task build` / `task run`.
- **Test framework:** Unity (ThrowTheSwitch) v2.6.1 via FetchContent
- **Asset formats:** PNG (sprites), WAV/OGG (audio)
- **Platform:** Windows (primary), cross-platform possible via Raylib
