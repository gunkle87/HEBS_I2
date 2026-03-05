# HEBS Technical Specification

Version: Tech-0.1.2
Status: Draft Canon
Project: HEBS (Hardware Evaluated Batch Simulator)
Date: 2026-03-03

## 1. Scope

This document defines canonical contracts for:

- public API behavior
- memory layout and ownership
- execution ordering and determinism
- metrics and timing sources
- machine-readable artifact schemas
- sequential lowering and SRAM system behavior

Normative companion contracts:

1. `canon/sequential_lowering_v1.md`
2. `canon/sram_system_v1.md`

## 2. Design Constraints

1. Core implementation is procedural C with Data-Oriented Design (DOD).
2. Core data structures must be flat and contiguous unless measured data justifies exceptions.
3. Loops are allowed only when they are the best measured option.
4. Hot-path logic should be branch-minimized and cache-aware.

## 3. Data Model and Memory Layout

### 3.1 Lane Width and Alignment

- `HEBS_LANE_BITS` defines logical lane width. Default is `64`.
- `HEBS_CACHELINE_BYTES` defines minimum alignment target. Default is `64`.
- Buffers must be aligned to at least `HEBS_CACHELINE_BYTES`.

### 3.2 Bit-Trays (Signal Storage)

Signals are stored in packed words in contiguous trays.

- `hebs_buffer`: flattened signal storage.
- A signal location is represented by `word_index` and `bit_offset`.
- `bit_offset` range is `[0, HEBS_LANE_BITS-1]`.

### 3.3 Linear Execution Plan (LEP)

LEP is a contiguous array of fixed-size batch instructions.

```c
typedef struct {
    uint32_t in_a_word;
    uint32_t in_b_word;
    uint32_t scatter_ptr;
    uint16_t scatter_len;
    uint8_t gate_type;
} hebs_batch;
```

### 3.4 Scatter Distribution Map

Scatter map defines where each result bit is written in the destination tray.

```c
typedef struct {
    uint32_t dest_word;
    uint8_t dest_bit;
    uint8_t src_bit;
} hebs_fanout;
```

## 4. Public API Contract

Canonical API surface (names may be finalized, behavior is canonical):

```c
int hebs_init(struct hebs_engine* eng, const struct hebs_config* cfg);
int hebs_load_plan(struct hebs_engine* eng, const struct hebs_plan* plan);
int hebs_run_ticks(struct hebs_engine* eng, uint64_t tick_count);
int hebs_export_state(const struct hebs_engine* eng, struct hebs_state_view* out);
void hebs_shutdown(struct hebs_engine* eng);
```

Rules:

1. API must remain C ABI friendly.
2. Ownership and lifetime of every pointer argument must be explicit.
3. No hidden hot-path allocation in `hebs_run_ticks`.
4. API error returns must use `HEBS_ERR_*` enum values.

## 5. Execution and Sequential Contract

### 5.1 Batch Evaluation

- Batches evaluate packed lanes using bitwise operators.
- Example forms:
  - `result = in_a & in_b`
  - `result = in_a | in_b`
- Implementation must be branch-minimized in hot paths.

### 5.2 Tick Order Invariant

Canonical tick order is:

1. combinational batches
2. SRAM ops
3. DFF commit

Invariant form:

1. `comb -> sram -> dff`

### 5.3 Sequential Lowering Policy

1. Engine-native sequential core is `DFF_P`.
2. `DFF_N`, `DLATCH`, `TFF`, and `DFF_AR` are lowered at compile stage to `comb + DFF_P`.
3. Lowering must be deterministic and reflected in LEP hash.
4. Full rules are defined in `canon/sequential_lowering_v1.md`.

### 5.4 SRAM System Policy

1. SRAM is a distinct memory system with backing store and boundary ops.
2. SRAM is not flattened into gate-cloud expansion for production path.
3. Boundary nets must obey 7-state policy while backing store may remain 2-state with poison tracking.
4. Full rules are defined in `canon/sram_system_v1.md`.

## 6. Determinism and Correctness Modes

### 6.1 Determinism Rules

1. Stable ordering for LEP traversal and scatter writes is mandatory.
2. Tie-break behavior must be deterministic and documented.
3. Equal input + config + plan + seed + build profile must produce equal output.

### 6.2 Verify and Differential Modes

- In `verify` / `differential` mode, each compared tick/state must match reference bit-for-bit.
- Any non-zero regression delta is a hard failure in those modes.
- Performance mode may run without reference comparison, but must keep deterministic ordering rules.

## 7. Error Model

Required baseline error family:

- `HEBS_ERR_OK`
- `HEBS_ERR_INVALID_ARG`
- `HEBS_ERR_ALLOC`
- `HEBS_ERR_PLAN_INVALID`
- `HEBS_ERR_PLAN_BOUNDS`
- `HEBS_ERR_VERIFY_MISMATCH`
- `HEBS_ERR_INTERNAL`

Exact numeric values are implementation-defined but must remain stable once released.

## 8. Metrics and Timing Contract

Primary metrics:

- `GEPS`
- `ICF_ns_per_eval`
- `LatencyNsPerVector`
- `ActivityFactor`
- `DeltaPerVector`

Timing sources:

1. Portable monotonic timer is required.
2. `rdtsc` may be used as an optional supplemental metric.
3. Report must declare timer source for each timing field.

## 9. Artifact Schema Contract

Machine-readable outputs:

- CSV for tabular metrics
- NDJSON for event/tick streams
- JSON run manifest for environment/build metadata

Minimum manifest fields:

- schema_version
- timestamp
- git_commit
- git_dirty
- compiler
- compile_flags
- os
- cpu_model
- lane_bits
- cacheline_bytes
- timer_source

Schema changes must increment schema version and keep backward parsing notes.

## 10. Versioning and Change Control

1. Canon changes to this file require `DO: CANON_CHANGE` + `APPROVED`.
2. Proposed changes must include rationale and verification impact.
3. Performance changes without deterministic equivalence evidence cannot be promoted to canonical behavior.
