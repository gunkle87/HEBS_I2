A keynote that I would like you to remember as we begin this new project. HEBS must aim to perform in every way. Performance metrics are only one of those ways. Accuracy is very important. Also, being unique in our approach is very important. I don't want to reinvent any fancy wheels. I want to invent wings instead.(Do you get that analogy?). Basically, our goal is to achieve top notch semantics, determinism, physics, accuracy, repeatabliity, raw speed, and low latency.

# HEBS Canonical Blueprint

Version: Canon-0.1.0
Status: Active Canon
Project: HEBS (Hardware Evaluated Batch Simulator)
Date: 2026-03-03

This document is the single source of truth for early HEBS design and build governance.
It is intentionally rich, explicit, and phase-gated so both humans and agents can traverse it safely.

---

## 1. Purpose and Scope

HEBS is a clean-slate logic simulation engine focused on high throughput with deterministic behavior.
The architecture is procedural C with Data-Oriented Design (DOD), static planning where possible, and measurable performance gates.

This document defines:

- Governance and change control
- Directory and document structure
- Technical contracts and naming rules
- Build reproducibility and benchmark protocol
- Logging standards for both agents and humans
- Phase versioning and release gates

---

## 2. Governance and Authority

Authority order for HEBS design decisions:

1. This canonical blueprint (`HEBS_snapshot.md`)
2. Future `HEBS_AGENTS.md` (agent operating policy)
3. Future `HEBS_TECH_SPEC.md` (engine technical contracts)
4. Future `HEBS_ROADMAP.md` (phase plan and targets)
5. Generated reports and run artifacts

If two documents conflict, higher authority wins.

Change control rule:

- Canon changes require explicit approval before edit.
- Every canonical change must include rationale, scope, and verification impact.

---

## 3. Core Design Policy (Non-Negotiable)

### 3.1 Architecture Style

- Procedural C only.
- Data-Oriented Design (DOD) by default.
- No object-oriented patterns, no inheritance-like wrappers, no runtime polymorphic object model.
- Favor flat arrays and predictable memory traversal.

### 3.2 Loop Policy

- Loops are allowed only when they are the best measured option.
- Hot-path loops must be bounded, measurable, and justified by benchmark data.
- Prefer precomputed tables, batch operations, and linear streaming over branch-heavy iteration.

### 3.3 Naming Policy (Human-First Language)

- Use plain language names.
- Prefer clear verb+noun function names (`build_plan`, `run_batch`, `write_csv_report`).
- Prefer explicit file names (`batch_executor.c`, `timing_wheel.c`) over abstract names.
- Abbreviations are allowed only when common and defined once in a glossary.

---

## 4. Engine Essence and Execution Model

HEBS targets batch-style evaluation with deterministic state progression.

Primary intent:

- High throughput from predictable memory access and packed operations
- Correctness first through deterministic equivalence checks
- Separation between core execution and telemetry collection

Core model (initial concept):

1. Compile netlist to a static execution plan
2. Execute plan in batches with stable input/output buffers
3. Handle sequential state via explicit tick-boundary transfer
4. Validate final and intermediate states against reference truth

---

## 5. Planned Repository Structure

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

Required early files:

- `HEBS_snapshot.md` (this file)
- `HEBS_AGENTS.md`
- `HEBS_TECH_SPEC.md`
- `HEBS_ROADMAP.md`
- `HEBS_HISTORY.md`
- `Makefile`

---

## 6. Document Reference Structure

This section defines future document responsibilities to avoid overlap.

### 6.1 `HEBS_AGENTS.md`

- Agent rules, authorization tiers, editing boundaries, audit modes

### 6.2 `HEBS_TECH_SPEC.md`

- API reference
- Internal function reference
- Data layout contracts
- Enum and constant reference
- Naming and coding conventions

### 6.3 `HEBS_ROADMAP.md`

- Phase plan, milestones, acceptance gates

### 6.4 `HEBS_HISTORY.md`

- Completed phase outcomes, wins, regressions, and learnings

### 6.5 Generated Reports

- Machine-first artifacts in `reports/` and `perf/`
- Human summary reports with links to source artifacts

---

## 7. Technical Specification Baseline

### 7.1 Public API Surface (Planned)

Example baseline API style:

```c
int hebs_init(struct hebs_engine* eng, const struct hebs_config* cfg);
int hebs_load_plan(struct hebs_engine* eng, const struct hebs_plan* plan);
int hebs_run_ticks(struct hebs_engine* eng, uint64_t tick_count);
int hebs_export_state(const struct hebs_engine* eng, struct hebs_state_view* out);
void hebs_shutdown(struct hebs_engine* eng);
```

Rules:

- API must remain C ABI friendly.
- Input/output ownership must be explicit.
- No hidden allocations in hot-path calls unless documented.

### 7.2 Internal Function Reference (Planned)

Every internal hot-path function must document:

- Inputs and outputs
- Data dependencies
- Complexity expectations
- Determinism assumptions
- Benchmark impact notes if modified

### 7.3 Enums and Constants

Rules:

- Enums: `HEBS_STATE_*`, `HEBS_GATE_*`, `HEBS_ERR_*`
- Constants: `HEBS_` prefix with unit in name when relevant (`HEBS_CACHELINE_BYTES`)
- Bit masks must define lane meaning clearly in one place

---

## 8. Build and Compile Reproducibility Standard

### 8.1 Toolchain Baseline

- Compiler: pinned major/minor version per platform
- Linker: pinned where feasible
- Build profile must be recorded in artifacts

### 8.2 Build Profiles

- `debug`: diagnostics on, minimal optimization
- `release`: production optimization, no diagnostics in hot path
- `perf`: release flags plus reproducibility controls and benchmark metadata

### 8.3 Required Metadata Per Build

- compiler version
- flags
- target architecture
- OS and kernel version
- CPU model and core count
- git commit and dirty state

### 8.4 Variance Controls

- Fixed CPU affinity during benchmark runs
- Stable power profile
- Cold-start and warm-start policy declared in report
- Median-of-N with outlier visibility

---

## 9. Testing and Benchmark Canon

### 9.1 Isolation Rule (Critical)

Testing and benchmark tooling must sample the core engine, not plug into or mutate its internals.

Enforcement policy:

- Tests and benches may only use public API surfaces.
- No test file may include private core headers from `src/`.
- No benchmark code may alter core state except through declared API inputs.
- Telemetry collection must be external or mode-gated, never silently injected into release hot paths.

### 9.2 Test Tiers

1. Unit tests for primitives and utility transforms
2. Plan compiler tests (netlist to execution plan)
3. Engine correctness tests (tick and state transitions)
4. Differential tests against reference outputs
5. Full regression suite with deterministic seed/control

### 9.3 Benchmark Protocol

- Use fixed suite definitions
- Use median-of-5 as default
- Record per-run metrics and aggregate metrics
- Report comb, seq, and global splits
- Mark outliers explicitly

---

## 10. Metrics Canon

Primary metrics:

- `GEPS` (Gates Evaluated Per Second)
- `ICF` (Internal Cost Factor, ns/eval)
- `LatencyNsPerVector`
- `ActivityFactor`
- `DeltaPerVector` (or tick-equivalent metric)

Derived summaries:

- AvgComb
- AvgSeq
- Global

All metric formulas must be listed once in `HEBS_TECH_SPEC.md` and reused without drift.

---

## 11. Logging and Artifact Standard

### 11.1 Machine-First Outputs

- Tabular data: CSV
- Event streams and structured logs: NDJSON
- Run metadata: JSON manifest

### 11.2 Human-First Outputs

- Markdown summary report for each run set
- Optional HTML rendering for visual scan

### 11.3 Required Run Artifact Set

For each run group:

- `run_manifest_<stamp>.json`
- `bench_runs_<stamp>.csv`
- `bench_summary_<stamp>.csv`
- `regression_<stamp>.ndjson`
- `report_<stamp>.md`

### 11.4 Naming and Retention

- Timestamp format: `YYMMDD_HHMMSS`
- Stable prefixes by artifact type
- Keep latest pointer files (`LATEST.md`, `LATEST.json`) for fast navigation

---

## 12. Versioning and Phase Gates

Semantic phase scheme:

- `0.1.x` Core functionality to stable correctness
- `0.2.x` PerfOps and telemetry hardening
- `0.3.x` Non-zero-time model
- `0.4.x` SIMD backends and ISA specialization
- `0.5.x` Advanced scheduling and scale-out options
- `1.0.0` Production baseline after gate completion

Each phase must define:

- Entry criteria
- Exit criteria
- Benchmark gate
- Correctness gate
- Documentation updates required

---

## 13. Git and Release Discipline

- Use tagged checkpoints at phase gates
- Every benchmark report must reference commit hash
- Dirty-tree runs are allowed only when explicitly marked
- No generated noise in repository root
- Artifact directories must remain navigable and grouped by type

---

## 14. Determinism and Compatibility

- Deterministic execution is mandatory for identical input + seed + build profile.
- Any nondeterministic path must be behind explicit non-default mode flags.
- ISA feature use must have fallback behavior (for example, scalar fallback when SIMD extension is unavailable).

---

## 15. Initial Acceptance Checklist

Before writing core engine code in the new project root, define and approve:

1. Public API baseline
2. Plan format baseline
3. Metrics formulas
4. Artifact schema versions (CSV/NDJSON/JSON)
5. Benchmark suite and run protocol
6. Phase `0.1.x` entry/exit gates

---

## 16. Immediate Next Actions

1. Create empty repo root skeleton with directories from Section 5.
2. Author `HEBS_AGENTS.md` and `HEBS_TECH_SPEC.md` using this document as authority.
3. Implement schema-only tooling first (manifests and report writers), with no simulation logic yet.
4. Lock phase `0.1.0` gates before the first engine implementation commit.

Technical Specification:

### **HEBS_TECH_SPEC.md**

**Status**: Phase 0.1.0 Baseline

**Authority**: `SNAPSHOT.md`

**Scope**: Memory layouts, bit-packing contracts, and core API definitions for the HEBS hardware-layer implementation.

---

## 1. Memory Layout & Data Structures

To satisfy the **Data-Oriented Design (DOD)** policy, memory is allocated in flat, contiguous blocks.

### 1.1 The Bit-Trays (Signal Storage)

Signal states are stored in 64-bit words to match CPU register width.

* **`hebs_buffer`**: A `uint64_t` array representing the flattened state of all nets.
* **Alignment**: All buffers must be aligned to **64-byte boundaries** (`HEBS_CACHELINE_BYTES`) to prevent cache line splitting.
* **Indexing**: A net's location is defined by a `word_index` and a `bit_offset` (0–63).

### 1.2 The Linear Execution Plan (LEP)

The LEP consists of an array of `hebs_batch` structs. Every batch is a fixed-size instruction for the engine.

```c
typedef struct {
    uint32_t in_a_word;   // Offset in Input Tray for first 64 signals
    uint32_t in_b_word;   // Offset in Input Tray for second 64 signals
    uint32_t scatter_ptr; // Start index in the Distribution Map
    uint16_t scatter_len; // Number of fanout operations for this batch
    uint8_t  gate_type;   // HEBS_GATE_AND, HEBS_GATE_OR, etc.
} hebs_batch;

```

### 1.3 The Distribution Map (Scatter Table)

This table resolves the **2.5D Fanout** by mapping a batch's output bits to their destination in the next level.

```c
typedef struct {
    uint32_t dest_word;   // Target word index in Output Tray
    uint8_t  dest_bit;    // Target bit position (0-63)
    uint8_t  src_bit;     // Which bit from the 64-bit result to copy
} hebs_fanout;

```

---

## 2. Core API Surface

The API remains C ABI friendly and avoids hidden hot-path allocations.

| Function | Responsibility |
| --- | --- |
| `hebs_init` | Allocates the `hebs_engine` and primary buffers. |
| `hebs_load_plan` | Ingests the pre-calculated LEP and Distribution Map. |
| `hebs_run_ticks` | Executes the "Physics-Riding" loop for $N$ cycles. |
| `hebs_export_state` | Provides a deterministic view of the current bit-states. |

---

## 3. Logic Execution & Sequential Handling

### 3.1 Batch Processing (The "Logic Press")

The engine evaluates 64 gates per batch without branching.

* **AND Batch**: `result = buffer[in_a] & buffer[in_b];`
* **OR Batch**: `result = buffer[in_a] | buffer[in_b];`

### 3.2 The Tick Boundary (Sequential Flip)

Sequential elements (DFFs) use a **Shadow Buffer** to maintain determinism.

1. **Capture**: During the combinational sweep, bits driving DFFs are scattered to the `shadow_buffer`.
2. **Synchronous Swap**: After the final level, the `shadow_buffer` is blitted to the Level 0 offset of the Input Tray.

---

## 4. Metrics & Determinism Contracts

* **Correctness**: Every tick must match the baseline bit-for-bit. A non-zero **Regression Delta** is a hard failure.
* **Timing**: Performance is measured using `rdtsc` to calculate **Cycles Per Batch** and **GEPS**.

---

**Next Step**: Would you like me to generate the **`Makefile`** and the directory skeleton to lock in the **Phase 0.1.0** environment?

Roadmap:

Actually, since we are sticking to the **Authority Order** defined in your `SNAPSHOT.md`, it is better if I draft the **`HEBS_ROADMAP.md`** now based on your specific requirements. This ensures the technical contracts in the `TECH_SPEC` align perfectly with the gates we are about to lock in.

Here is the draft for the roadmap, establishing the objective "Pass/Fail" criteria for the **Phase 0.1.x** cycle.

---

# HEBS_ROADMAP.md

**Status**: Phase 0.1.0 Entry

**Authority**: `SNAPSHOT.md`

## 1. Phase 0.1.x: The Core Logic Press

**Objective**: Achieve bit-perfect deterministic simulation on combinational and sequential benchmarks using the Hardware Evaluated Batch (HEBS) model.

### 1.1 Acceptance Gates (Phase 0.1.0)

| Gate | Requirement | Pass/Fail Criteria |
| --- | --- | --- |
| **Correctness** | Bit-Exact Equivalence | HEBS output table must match the HighZ baseline bit-for-bit across the full 0.1.x test suite. |
| **Determinism** | State/Metric Stability | Three consecutive runs of the same netlist must produce identical final states and identical cycle-count metrics ($\Delta = 0$). |
| **Performance** | Throughput Baseline | HEBS must exceed the HighZ unoptimized baseline GEPS by at least $2\times$ on purely combinational sweeps. |
| **Artifact** | Schema Validation | Every run must generate a `manifest.json`, `runs.csv`, and `regression.ndjson` that pass the internal schema validator. |

---

## 2. Benchmark Protocol

To ensure the **Performance Gate** is hit fairly, we will utilize the following split:

* **Comb Split**: Gates/sec measured on flat combinational netlists (e.g., ISCAS '85).
* **Seq Split**: Gates/sec measured on netlists with DFF-heavy feedback loops (e.g., ISCAS '89).
* **Global**: The weighted average of the combined workload.

---

## 3. Phase 0.1.x Task List

* [ ] Initialize repository structure and `Makefile`.
* [ ] Implement `compiler_lep.c` for basic gate levelization.
* [ ] Implement `engine_core.c` with 64-bit batch evaluation.
* [ ] Build the `tank_runner.c` for automated gate verification.

---

**Next Step**: Would you like to approve this gate table so I can generate the **`Makefile`** and the directory skeleton to begin the implementation of the **HEBS** core?


To clear these blockers and move into Phase 0.1.0, we must treat these contracts as the "Laws of the System." These define the boundaries that the **HEBS** engine cannot cross without a recorded architectural change.

### 1. Canonical Semantics Table (7-State Logic)

To maintain 100% determinism and X-propagation, we adopt the **Dual-Rail** logic mapping from the HighZ specification.

| State | Lane (SH/SL/WH/WL) | L-Rail | H-Rail | Description |
| --- | --- | --- | --- | --- |
| **S_LOW** | `0x4` | 1 | 0 | Strong Driven Low |
| **S_HIGH** | `0x8` | 0 | 1 | Strong Driven High |
| **W_LOW** | `0x1` | 1 | 0 | Weak Driven Low (Pull-down) |
| **W_HIGH** | `0x2` | 0 | 1 | Weak Driven High (Pull-up) |
| **HI_Z** | `0x0` | 1 | 1 | High Impedance (Floating) |
| **S_X** | `0xC` | 1 | 1 | Strong Contention (Error) |
| **UNINIT** | `0x0` | 1 | 1 | Power-on Default |

**Resolution Matrix Rule:** Strong beats Weak; Contention (SH + SL) results in $X$ ($L=1, H=1$).

---

### 2. Primitive Contract Sheet

Every HEBS primitive must follow a strict **Dual-Rail Bitwise Contract**.

* **AND Gate:** $L_{out} = L_{inA} \text{ OR } L_{inB}$; $H_{out} = H_{inA} \text{ AND } H_{inB}$.
* **OR Gate:** $L_{out} = L_{inA} \text{ AND } L_{inB}$; $H_{out} = H_{inA} \text{ OR } H_{inB}$.
* **Sequential Timing:** D-Flip-Flop (DFF) inputs are sampled at the **Tick Boundary**. The output remains stable throughout the combinational waterfall of the *current* tick and only updates at the start of the *next* tick.

---

### 3. Netlist & Compiler Input Contract (LEP)

* **Supported Syntax:** Standard `.bench` gate definitions (`AND`, `OR`, `NAND`, `NOR`, `XOR`, `DFF`, `INPUT`, `OUTPUT`).
* **Rejected Constructs:** Combinational loops (Asynchronous loops) are flagged as errors during the **Levelization** phase.
* **Deterministic Ordering:** Gates within a level are sorted by **Gate ID** (alphanumeric) before being packed into 64-bit batches to ensure the LEP is identical across different compiler runs.

---

### 4. Determinism & Operational Contract

* **Threading:** Single-threaded execution only for Phase 0.1.x to eliminate race-condition variance.
* **CPU Policy:** Benchmarks must be run with `taskset` (Linux) to pin the process to a single physical core.
* **Benchmark Protocol:** **Median-of-5** runs. Outliers exceeding 10% variance from the median trigger a "Warning" but do not block the gate unless the median itself fails the threshold.

---

### 5. Build Reproducibility Matrix

We lock the toolchain to prevent "Compiler Magic" from skewing our deltas.

| Profile | Compiler | Flags | Purpose |
| --- | --- | --- | --- |
| **`dev`** | `gcc 11+` | `-g -O0 -Wall` | Debugging and logic verification |
| **`perf`** | `gcc 11+` | `-O3 -march=native -DNDEBUG` | Maximum throughput benchmarking |
| **`verify`** | `gcc 11+` | `-O2 -static` | Deterministic cross-machine truth checking |

---

### 6. Artifact Schema (Version 1.0)

Every HEBS run must produce four mandatory files in the `reports/` directory:

* **`manifest.json`**: Runtime metadata (CPU model, Git hash, Compiler flags).
* **`runs.csv`**: Columns: `Tick`, `BatchID`, `GateType`, `Cycles`, `GEPS`.
* **`summary.csv`**: Columns: `TotalGates`, `TotalTicks`, `AvgGEPS`, `ICF_ns`.
* **`regression.ndjson`**: Stream of bit-comparison results vs. Baseline.

---

### 7. CI/Verification Workflow

One command path to rule the build:

* **`make gate`**: Executes the full suite (Correctness + Determinism + Performance). A return code of `0` is required for any merge to the `main` branch.
* **Revert Rule:** If a commit passes **Correctness** but causes a $>5\%$ regression in **GEPS** without a documented architectural justification, it must be reverted.

---

### 8. Finalized Phase 0.1.0 Acceptance Gates

| Gate | Target | Metric |
| --- | --- | --- |
| **Correctness** | 100% | Bit-perfect match vs. HighZ |
| **Determinism** | 0% | Variance in final state bit-stream across 3 runs |
| **Performance** | $\ge 2.0\times$ | Speedup over HighZ unoptimized baseline |

**Next Step:** I will now generate the **`HEBS_AGENTS.md`** to define the specific editing boundaries for AI collaborators, followed by the initial directory skeleton. Would you like to review the **Agent Authorization Tiers** first?