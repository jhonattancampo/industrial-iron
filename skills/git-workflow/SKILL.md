# Skill: Git Workflow — Industrial Iron

## When to use
Any task involving git: commits, branches, PRs, tagging, versioning.
Read this file completely before touching git.

---

## Branch strategy

```
main          → stable, always compiles, always tested
dev           → integration branch, all features merge here first
feat/phase-*  → one branch per plugin phase
fix/*         → bug fixes
chore/*       → non-functional changes (docs, build, config)
```

### Branch naming
```
feat/phase-2a-asymclipper
feat/phase-2b-pre-eq-compressor
feat/phase-3-iron-wall-panner
feat/phase-4-cab-sim
feat/phase-5-ui-final
fix/oversample-aliasing
chore/update-agents-skills
```

---

## Commit format (Conventional Commits)

```
<type>(<scope>): <short description>

[optional body — what and why, not how]

[optional footer — breaking changes, refs]
```

### Types
| Type | When |
|------|------|
| `feat` | new DSP module, new parameter, new UI element |
| `fix` | bug fix in existing code |
| `chore` | build system, config, docs, skills update |
| `test` | QA checklist results, test files |
| `refactor` | restructure without behavior change |
| `perf` | performance improvement (CPU, memory) |

### Scope = module name
`asymclipper`, `pre-eq`, `compressor`, `cab-sim`, `iron-wall`, `ui`, `i18n`, `build`, `config`, `agents`

### Examples
```
feat(asymclipper): add x4 oversampling with polyphase filter

Replaces naive upsampling. Eliminates aliasing artifacts
above 18kHz measured with sine sweep test.

Closes #3

---

fix(iron-wall): prevent phase cancellation above Width 0.9

Correlation dropped below 0.5 at extreme width settings.
Added soft limiting above 0.85 to keep correlation > 0.6.

---

chore(agents): add git-workflow skill to registry

---

feat(config): update PLUG_LATENCY for x4 oversample

Latency = filter_order / 2 samples at current sample rate.
Hosts now compensate automatically for PDC.
```

---

## Workflow per phase

```bash
# 1. Start phase branch from dev
git checkout dev
git pull origin dev
git checkout -b feat/phase-2a-asymclipper

# 2. Work — commit often, small units
git add src/dsp/AsymClipper.h src/dsp/AsymClipper.cpp
git commit -m "feat(asymclipper): implement asymmetric hard clipper"

git add IndustrialIron.cpp CMakeLists.txt
git commit -m "feat(asymclipper): integrate into processing chain"

git add config.h
git commit -m "feat(config): update PLUG_LATENCY for x4 oversample"

# 3. Phase complete — merge to dev
git checkout dev
git merge --no-ff feat/phase-2a-asymclipper \
  -m "feat: Phase 2a complete — AsymClipper with x4 oversampling"

# 4. Tag stable milestones on main
git checkout main
git merge --no-ff dev -m "release: Phase 2a stable"
git tag -a v0.2.0 -m "Phase 2a: AsymClipper + oversampling"
git push origin main --tags
```

---

## Versioning

Follow semver during development:

| Version | Meaning |
|---------|---------|
| v0.1.0 | Phase 1 complete (scaffolding) |
| v0.2.0 | Phase 2a complete (AsymClipper) |
| v0.3.0 | Phase 2b complete (PreEQ + Compressor) |
| v0.4.0 | Phase 3 complete (IronWallPanner) |
| v0.5.0 | Phase 4 complete (CabSim) |
| v0.6.0 | Phase 5 complete (UI final) |
| v1.0.0 | QA passed, ready for Gumroad release |

---

## Commit rules (non-negotiable)

1. **Never commit binaries** — .vst3, .component, .dylib go in .gitignore
2. **Never commit build artifacts** — /build/ is ignored
3. **One logical change per commit** — if you did two things, make two commits
4. **Present tense, imperative mood** — "add oversample" not "added" or "adding"
5. **English only** — no Spanish in commits, branches, or PR titles
6. **Compile before committing** — never commit broken code to dev or main
7. **Reference phase** — include phase number in feat commits for traceability

---

## PR format (when using GitHub)

```markdown
## Summary
One paragraph: what this PR does and why.

## Phase
Phase 2a — AsymClipper with x4 oversampling

## Changes
- `src/dsp/AsymClipper.h/.cpp` — asymmetric clipper with polyphase oversample
- `IndustrialIron.cpp` — integrated into processing chain
- `config.h` — updated PLUG_LATENCY

## QA
- [ ] Compiles without warnings
- [ ] Opens in Ableton without crash
- [ ] No aliasing above 18kHz (sine sweep test)
- [ ] CPU < 5% at 256 sample buffer

## Notes
Any edge cases, known issues, or decisions made.
```

---

## Quick reference

```bash
# Status
git status
git log --oneline --graph --all

# Undo last commit (keep changes)
git reset --soft HEAD~1

# Amend last commit message
git commit --amend -m "corrected message"

# Discard unstaged changes in file
git checkout -- src/dsp/AsymClipper.cpp

# See what changed in last commit
git show --stat
```