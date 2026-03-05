# HEBS Canonical Blueprint

Version: Canon-0.1.1
Status: Active Canon
Project: HEBS (Hardware Evaluated Batch Simulator)
Date: 2026-03-03

This document is the single source of truth for early HEBS design and build governance.

## 1. Purpose and Scope

HEBS is a clean-slate logic simulation engine focused on high throughput, deterministic behavior, and strict correctness.

## 2. Governance and Authority

Authority order:

1. `HEBS_SNAPSHOT.md`
2. `HEBS_AGENTS.md`
3. `HEBS_TECH_SPEC.md`
4. `HEBS_ROADMAP.md`
5. generated artifacts (`reports/`, `perf/`)

## 3. Planned Repository Structure

Top-level:

- `include/`
- `src/`
- `tools/`
- `tests/`
- `benchmarks/`
- `reports/`
- `perf/`
- `canon/`
- `bin/`

Canonical files:

- `HEBS_SNAPSHOT.md`
- `HEBS_AGENTS.md`
- `HEBS_TECH_SPEC.md`
- `HEBS_ROADMAP.md`
- `HEBS_HISTORY.md`
- `Makefile`

Compatibility pointers:

- `SNAPSHOT.md` -> points to `HEBS_SNAPSHOT.md`
- `AGENTS.md` -> points to `HEBS_AGENTS.md`
- `TECH_SPEC.md` -> points to `HEBS_TECH_SPEC.md`

## 4. Core Design Policy

1. Procedural C + DOD only.
2. No OOP patterns in core engine.
3. Loops only when best measured option.
4. Test and benchmark isolation from internal data structures.

## 5. Build and Benchmark Standards

1. Reproducible build metadata is mandatory.
2. Benchmark decisions use median-of-N.
3. Reports must provide machine-readable CSV and human-readable Markdown.

## 6. Logging and Artifact Standard

Machine-first:

- CSV for tables
- NDJSON for streams
- JSON manifest for run metadata

Human-first:

- Markdown summaries
- optional HTML views

## 7. Versioning

- `0.1.x` core functionality
- `0.2.x` perfops + telemetry
- `0.3.x` non-zero-time
- `0.4.x` SIMD backends
- `0.5.x` advanced scheduling
- `1.0.0` production baseline

## 8. Immediate Next Actions

1. Keep prefixed canonical files as the source of truth.
2. Implement `HEBS_TECH_SPEC.md` contracts before core code.
3. Create phase gates in `HEBS_ROADMAP.md`.
4. Record execution outcomes in `HEBS_HISTORY.md`.
