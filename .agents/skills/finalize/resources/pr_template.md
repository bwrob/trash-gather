# Pull Request Template (`.agents/skills/finalize/resources/pr_template.md`)

This template defines the mandatory structure for Pull Requests created during milestone finalization via `gh pr create`.

______

```markdown
## Summary

- **Milestone:** [`<hash_id>`](roadmap/<hash_id>_<slug>.md) - <Milestone Title>
- **Scope:** <1-2 sentence overview of implemented features, refactors, or container protocols>

### Architectural Changes
- **Memory Layout & Data Structures:** <Detail struct layouts, flexible array members, alignment holes, or tagged union data variants>
- **Runtime Mechanics & Ownership:** <Detail parameter ownership (borrowed vs. owned), reference count increments, and allocation failure rollbacks>

### Verification & Quality Gates
- [x] **Compiler Verification:** Clean build with zero warnings under ISO C17 (`-std=c17 -Wall -Wextra`) (`just build`)
- [x] **Unit & Leak Tests:** Full test suite passes cleanly with zero memory leaks via `assert(boot_all_freed())` (`just test`)
- [x] **100.00% Line Coverage:** Every line and defensive error guard in `src/` verified via adversarial probes (`just coverage`)
- [x] **Sanitizer Clean:** Zero issues detected under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)
- [x] **Code Quality & Linters:** Passes `clang-tidy`, Doxygen docstring checks, and all pre-commit hooks (`just check`)

### Educational Lesson & Roadmap
- Extracted [Lesson NN: <Topic Title>](lessons/NN_<slug>.md)
- Roadmap milestone status updated to `✅ Completed` in [`roadmap/README.md`](roadmap/README.md)
```
