# HEBS Roadmap

Version: Roadmap-0.1.3
Status: Active
Authority: `HEBS_SNAPSHOT.md`
Focus: single-core crown, 7/8-state correctness, determinism

## Phase 0.1 - Core Correctness Lock

Goal: bit-perfect and deterministic behavior with stable baselines.

1. Correctness parity
   - `make verify` must pass consistently.
   - Bit-exact equivalence to HighZ baseline where expected.
2. Determinism
   - Identical inputs/config => identical outputs across multiple runs.
   - Record determinism checks in artifacts.
3. Runtime contract enforcement
   - Sequential runtime = `DFF_P` only (hard-fail if unlowered sequential reaches runtime).
   - SRAM behavior stays explicit (either supported or hard-rejected per mode).

Deliverable: Core is locked; performance work is safe.

## Phase 0.2 - Measurement + Reporting Discipline

Goal: tuning is always guided by measurable outputs.

1. Engine counters (stats gated)
   - `gates_evaluated`
   - `scatter_entries`
   - `unique_dest_words`
   - `tray_bytes_touched`
   - `dff_commit_bytes`
   - optional: `rmw_count` (read-modify-write count)
2. Manifest-driven bench
   - `perf/BENCH_MANIFEST.json` drives all benches (no hardcoded lists).
   - 10 independent cold-start runs per bench.
   - Record median/high/low.
3. Regression guardrails
   - `perf/PERF_THRESHOLDS.json`.
   - Perf gate fails if regression exceeds thresholds vs previous.
4. Human report generator (always readable)
   - Compare: HighZ baseline + previous run + current run.
   - GEPS: HighZ, Prev, Curr, Delta Prev->Curr.
   - ICF: Prev, Curr, Delta Prev->Curr.
   - Latency: Curr value, Delta Prev->Curr only.
   - Include run fingerprint (commit/branch/cpu/os/flags/manifest).
5. Legacy salvage
   - Convert older raw reports into readable prev-vs-curr-vs-HighZ reports.

Deliverable: Every tuning change produces one clean report plus hard regression signal.

## Phase 0.25 - Hot Path Purity

Goal: remove taxes that weaken all later optimization work.

1. No per-batch branching
   - No `if`/`switch` in the tight batch loop for mode flags or gate kinds.
2. Inline the inner op
   - `static inline` op bodies.
   - No function pointer per batch.
3. Tick-level dispatch only
   - If multiple modes exist, choose tick function at init (`tick_fn`), not per batch.

Deliverable: hot loop is clean enough to respond to structural tuning.

## Phase 0.30 - Typed LEP Segments (Big, Safe Win)

Goal: predictable instruction stream and better SIMD utilization.

1. Segment LEP by gate type
   - AND segment loop.
   - XOR segment loop.
   - Etc.
   - No per-batch `gate_type`.
2. Segment-local micro-layout
   - Keep segment data hot and contiguous (SoA).

Deliverable: branchless loops per op-type.
Expected: +10% to +30% typical.

## Phase 0.35 - Scatter Dominance Attack (Highest ROI)

Goal: reduce random writes, reduce RMW, improve cacheline locality.

1. Measure scatter pressure
   - Ensure counters show where time is going.
2. Fanout compression
   - Prefer word/mask fanout forms when possible.
   - Compress runs and contiguous destinations.
   - Shrink scatter entry count.
3. Destination-word batching
   - Stage updates by `dest_word`.
   - Commit once per word (or per small block).
   - Minimize read-modify-write.
4. Avoid RMW when safe
   - Exploit write-once-per-bit-per-tick where guaranteed.

Deliverable: scatter becomes less random and less RMW-heavy.
Expected: +20% to +60%, depending on current scatter format.

## Phase 0.40 - Write-Once Commit Mode (Single-Driver Crown)

Goal: eliminate most scatter RMW by changing how outputs land.

This is the primary structural leap for the 500M-1B GEPS path.

1. Compile-time single-driver guarantee
   - Crown mode requires one driver per net.
   - Runtime multi-driver merge is not supported in crown mode.
   - Add LEP-time invariant check: if any output target is written by more than one batch in the same tick plan, fail compile.
2. Two-phase output model
   - Phase A: compute into `next` buffers/output trays using streaming stores.
   - Phase B: commit next->present by pointer swap or contiguous copy.
   - No patch-writing bits across present trays.
3. Runtime invariants
   - During tick execution, present tray is read-only.
   - Each output bit writes at most once per tick.
4. Validate
   - Delay=0 equivalence vs prior mode.
   - Determinism must hold.
   - Perf improvements must show reduced `rmw_count` and reduced random writes.

Deliverable: crown mode becomes streaming-write dominated, not scatter-RMW dominated.
Expected: +1.5x to +3x potential when current costs are RMW-heavy.

## Phase 0.45 - SIMD Exploitation (SIMD Available)

Goal: make SIMD matter more by feeding it clean data patterns.

1. Ensure typed segments and reduced scatter are in place first.
2. Validate SIMD lanes are not limited by scatter.
3. Add ISA-specialized variants only if warranted (optional)
   - AVX2 vs AVX-512.
   - Build-time variants or runtime dispatch.

Deliverable: SIMD throughput scales because memory-side bottlenecks are reduced.

## Phase 0.50 - SRAM as Memory System (Not Gate Cloud)

Goal: support real memory without collapsing LEP size/perf.

1. Add `sram_blocks[]` backing stores.
2. Add `sram_ops[]` boundary ops.
3. Tick order
   - `comb -> sram -> dff commit`.
4. 7/8-state boundary policy
   - X/Z addr => X word out.
   - WE/RE X/Z => deterministic policy.
   - DIN X/Z write => poison (word-level first, per-bit later).

Deliverable: memory behaves like memory; performance remains controlled.

## Phase 0.60 - Temporal Timing (Gate Delays)

Goal: add delays without polluting crown-mode performance.

1. Add temporal engine path via timing wheel / delayed commit.
2. Run A/B/C feature-tax test
   - A: ideal-only build.
   - B: dual-capable build, temporal disabled.
   - C: temporal enabled.
3. If B regresses A beyond about 1% to 2%, ship separate binaries
   - `hebs_core` (ideal).
   - `hebs_time` (temporal).

Deliverable: temporal is available while crown mode stays pure.

## Phase 0.70 - Advanced Scheduling (Optional, Non-Crown)

Goal: optional experiments without contaminating crown mode.

1. Region-based updates.
2. Partial ticking.
3. Multithread experiments (not default).

Deliverable: optional modes exist without compromising single-core crown.

## Phase 1.0 - Production Baseline

Goal: stability, docs, reproducibility, and ship readiness.

1. Frozen snapshot plus manifest.
2. Stable baselines.
3. Documented semantics.
4. Build and toolchain polish.

## Preferred Implementation Order

1. Phase 0.2 (manifest + human report + counters).
2. Phase 0.30 (typed segments).
3. Phase 0.35 (scatter compression + destination batching).
4. Phase 0.40 (write-once commit mode).
5. Revisit SIMD to measure unlocked headroom.

Rationale: this order maximizes measured wins and minimizes tuning rabbit holes.
