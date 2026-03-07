# HEBS_REPO_MAP

Status: Active technical reference
Generated: 2026-03-06
Repository Root: `C:\DEV\HEBS_I2`
Primary Language: C (procedural, data-oriented)

## 1. Purpose

This document is the working map for the full HEBS repository.
It is designed for maintainers, reviewers, and agents as a direct operational reference.

It covers:
- exact directory and file roles
- public API contracts and usage flow
- enums, structs, constants, and key runtime state
- canonical term glossary
- git commit and push ledger
- baseline performance ledger for `Revision_Structure_v04`
- governance mandates for map maintenance, artifact naming, and deviation approvals

## 2. Canonical Name Mapping (Plan vs Current Symbols)

`HEBS_PLAN.md` describes some conceptual names that differ from currently exported symbols.

| Canonical Concept Name | Current Implemented Symbol | Reference |
|---|---|---|
| `hebs_tick_execute()` | internal `hebs_tick_execute()` called by public `hebs_tick()` | `core/engine.c` |
| `hebs_compile_lep()` | `hebs_load_bench()` + internal parse/levelize/pack | `core/loader.c:735`, `core/loader.c:544` |
| LEP hash generation | `hebs_calculate_lep_hash()` | `core/loader.c:685` |

## 3. Directory Dictionary

### 3.1 Top-Level

| Path | Type | Role |
|---|---|---|
| `AGENTS.md` | Governance | Agent authorization and execution policy.
| `HEBS_PLAN.md` | Architecture Plan | High-level architecture, component roles, target structure.
| `PROBE_FIX.md` | Canon Staging Plan | Probe profile hardening and staged `probe_fix_v03` through `probe_fix_v05` execution lock.
| `HighZ_INTEGRATION.md` | Integration Blueprint | Start-to-finish implementation plan for HEBS hybrid zero-delay + timed-event runtime integration.
| `TAB_PROTOCOL.md` | Canonical Protocol | Mandatory test/benchmark rules and acceptance gates.
| `Makefile` | Build Orchestration | Build, clean, and verify entry points.
| `HEBS_REPO_MAP.md` | Repo Map | This technical reference.
| `.git/` | VCS Metadata | Local git object/database and refs.
| `core/` | Core Engine Source | Loader, engine loop, primitive logic.
| `include/` | Public Headers | API contracts and primitive declarations.
| `benchmarks/` | Benchmark Lab | Runner, metric schemas, protocol helpers, datasets, results.
| `tests/` | Validation Suite | Unit/integration assertions against public API.
| `build/` | Build Artifacts | Compiled executables.

### 3.2 `/core` (Implementation)

| File | Role | Key Symbols |
|---|---|---|
| `core/engine.c` | Simulation execution and bit-tray state updates with span-driven 64-gate SIMD batch execution. | `hebs_init_engine`, `hebs_tick_execute` (internal), `hebs_tick_execute_fallback` (internal), `hebs_tick`, `hebs_get_probes`, `hebs_get_signal_tray`, `hebs_get_state_hash`, `hebs_set_primary_input`, `hebs_get_primary_input` |
| `core/loader.c` | `.bench` parsing, signal table build, levelization, LEP packing, batch-span precompute, LEP hash. | `hebs_load_bench`, `hebs_levelize_and_pack`, `hebs_build_batch_spans`, `hebs_calculate_lep_hash`, `hebs_free_plan` |
| `core/primitives.c` | 8-state primitive library with SIMD tray operators and scalar evaluators. | `hebs_gate_and/or/xor/nand/nor/xnor/not/buf`, `hebs_gate_weak_pull/weak_pull_down/strong_pull/vcc/gnd/tristate`, `hebs_eval_*` family |
| `core/state_manager.c` | Sequential state manager for SIMD/vectorized DFF tray commit. | `hebs_sequential_commit` |

### 3.3 `/include` (Public API Headers)

| File | Role | Key Contracts |
|---|---|---|
| `include/hebs_engine.h` | Public engine/loader API and core types. | enums, plan/engine/probe structs, engine+loader+probe API prototypes |
| `include/hebs_probe_profile.h` | Probe profile policy | profile-selection macros and compat probe gate |
| `include/hebs_types.h` | SIMD abstraction type map. | `hebs_vec_t`, `HEBS_VEC_BYTES`, `HEBS_VEC_ALIGN`, `HEBS_VEC_IS_AVX512` |
| `include/primitives.h` | Primitive logic interface. | SIMD tray operators + scalar primitive evaluators |
| `include/state_manager.h` | Sequential state manager interface. | `hebs_sequential_commit` |

### 3.4 `/benchmarks` (Runner + Reporting)

| File | Role | Key Symbols |
|---|---|---|
| `benchmarks/runner.c` | Benchmark executor, warmup, 10-iteration protocol, CSV/HTML output. | profile-aware CSV path (`metrics_history.csv` perf / `metrics_history_compat.csv` compat), fixed 10-bench anchor target list, `hebs_run_single_bench`, `main` |
| `benchmarks/protocol_helper.h` | Statistical helpers (median/percentiles/ICF). | `calculate_p50`, `calculate_percentile`, `calculate_icf` |
| `benchmarks/report_types.h` | Metric row schema. | `hebs_metric_row_s` (`:6`) |
| `benchmarks/timing_helper.h` | High-resolution timer wrapper. | `hebs_timer_s` (`:6`) |
| `benchmarks/html_report.h` | HTML report emitter. | `generate_master_report` (`:18`) |
| `benchmarks/csv_export.h` | CSV parse/history lookup/append. | `lookup_history_for_bench_suite`, `lookup_revision_mean_geps`, `append_metrics_history_csv` |
| `benchmarks/benches/ISCAS85/c17.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c432.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c499.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c6288.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c7552.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c880.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS89/s27.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s298.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s382.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s38584.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s526.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s5378.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s820.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/results/metrics_history.csv` | Historical metric ledger | revision blocks + per-bench rows |
| `benchmarks/results/metrics_history_compat.csv` | Compatibility metric ledger | non-canonical compatibility-profile revision blocks + per-bench rows |
| `benchmarks/results/revision_structure_v01.html` | HTML benchmark report | legacy Revision_Structure artifact |
| `benchmarks/results/revision_combinational_v01.html` | HTML benchmark report | prior combinational artifact |
| `benchmarks/results/revision_combinational_v02.html` | HTML benchmark report | prior combinational artifact |
| `benchmarks/results/revision_combinational_v03.html` | HTML benchmark report | prior combinational artifact |
| `benchmarks/results/revision_combinational_v04.html` | HTML benchmark report | prior combinational artifact |
| `benchmarks/results/revision_combinational_v05.html` | HTML benchmark report | prior combinational artifact |
| `benchmarks/results/archive/non_canon_passB_2026-03-06.md` | Non-canon tuning archive | removed Pass B threshold-packing run block for historical reference |
| `benchmarks/results/revision_optimized_v01.html` | HTML benchmark report | prior optimized artifact |
| `benchmarks/results/revision_optimized_v02.html` | HTML benchmark report | prior optimized artifact |
| `benchmarks/results/revision_optimized_v03.html` | HTML benchmark report | prior optimized artifact |
| `benchmarks/results/probe_fix_v01.html` | HTML benchmark report | `probe_fix_v01` perf-profile artifact |
| `benchmarks/results/probe_fix_v02.html` | HTML benchmark report | `probe_fix_v02` perf-profile artifact |
| `benchmarks/results/probe_fix_v04.html` | HTML benchmark report | `probe_fix_v04` perf-profile artifact |
| `benchmarks/results/probe_fix_v05.html` | HTML benchmark report | `probe_fix_v05` perf-profile artifact |
| `benchmarks/results/probe_fix_v05_compat.html` | HTML benchmark report | `probe_fix_v05_compat` compatibility-profile artifact |

### 3.5 `/tests`

| File | Role | Key Symbols |
|---|---|---|
| `tests/test_runner.c` | API-level regression checks and protocol helper tests. | `test_loader_contract_from_s27` (`:22`), `test_functional_gate_suite` (`:42`), `test_parallel_dff_tray_commit`, `main` |
| `tests/results/` | Output directory | reserved for test artifacts |

### 3.6 `/build`

| File | Role |
|---|---|
| `build/hebs_cli.exe` | Benchmark runner binary.
| `build/hebs_test.exe` | Test binary.

## 4. Glossary

| Term | Definition | Code Anchor |
|---|---|---|
| LEP (Linear Execution Plan) | Flattened, level-ordered gate instruction stream consumed by the engine tick loop. | `include/hebs_engine.h:40`, `core/loader.c:544` |
| Bit-Tray | Packed signal storage (`uint64_t` trays) carrying 2-bit logic states. | `include/hebs_engine.h:72`, `core/engine.c:5` |
| Scatter-Store | Write stage where computed gate result is written to destination bit offset in trays. | `core/engine.c:143` |
| 8-State Logic | Discrete logic set `{S0,S1,W0,W1,Z,X,SX,WX}` used in simulation semantics. | `include/hebs_engine.h:9` |
| ICF | Input Change Factor (`internal_node_transitions / primary_input_transitions`). | `benchmarks/protocol_helper.h` |
| Logic Fingerprint | CRC32 of final tray state after simulation run. | `benchmarks/runner.c:157` |
| Plan Fingerprint | Hash over LEP layout and instruction fields. | `core/loader.c:685` |

## 5. Type and API Registry

### 5.1 Enumerations

#### `hebs_logic_t` (`include/hebs_engine.h:9`)

| Value | Symbol | Meaning |
|---|---|---|
| 0 | `HEBS_S0` | Strong 0 |
| 1 | `HEBS_S1` | Strong 1 |
| 2 | `HEBS_W0` | Weak 0 |
| 3 | `HEBS_W1` | Weak 1 |
| 4 | `HEBS_Z` | High impedance |
| 5 | `HEBS_X` | Unknown/Contention |
| 6 | `HEBS_SX` | Strong contention |
| 7 | `HEBS_WX` | Weak contention |

#### `hebs_status_t` (`include/hebs_engine.h:22`)

| Value | Symbol |
|---|---|
| 0 | `HEBS_OK` |
| 1 | `HEBS_ERR_LOGIC` |

#### `hebs_gate_type_t` (`include/hebs_engine.h:29`)

| Value | Symbol |
|---|---|
| 0 | `HEBS_GATE_AND` |
| 1 | `HEBS_GATE_OR` |
| 2 | `HEBS_GATE_NOT` |
| 3 | `HEBS_GATE_NAND` |
| 4 | `HEBS_GATE_NOR` |
| 5 | `HEBS_GATE_BUF` |
| 6 | `HEBS_GATE_DFF` |
| 7 | `HEBS_GATE_XOR` |
| 8 | `HEBS_GATE_XNOR` |
| 9 | `HEBS_GATE_TRI` |
| 10 | `HEBS_GATE_VCC` |
| 11 | `HEBS_GATE_GND` |
| 12 | `HEBS_GATE_PUP` |
| 13 | `HEBS_GATE_PDN` |

### 5.2 Structs

#### `hebs_lep_instruction_t` (`include/hebs_engine.h:40`)
- `gate_type`, `input_count`, `level`
- `src_a_bit_offset`, `src_b_bit_offset`, `dst_bit_offset`

#### `hebs_batch_span_t` (`include/hebs_engine.h`)
- `start_index`, `count`, `gate_type`, `level`
- precomputed contiguous LEP run metadata used by `hebs_tick_execute()`

#### `hebs_plan` (`include/hebs_engine.h:51`)
- Metadata: `lep_hash`, `level_count`, `signal_count`, `gate_count`, `tray_count`, `propagation_depth`, `fanout_avg`, `fanout_max`
- IO counts: `num_primary_inputs`, `num_primary_outputs`
- Buffers: `primary_input_ids`, `lep_data`, `batch_spans`
- Batch span index: `batch_span_count`
- Sequential index: `dff_instruction_count`, `dff_instruction_indices`
- Internal transition mask: `internal_transition_lsb_mask` (LSB-per-node mask for non-PI signals)

#### `hebs_engine` (`include/hebs_engine.h:68`)
- Runtime counters: `current_tick`, `cycles_executed`, `vectors_applied`
- Storage planes: `tray_plane_a`, `tray_plane_b`, `dff_state_trays`
- Active pointers: `signal_trays`, `next_signal_trays` (zero-copy swap)
- Probe counters: `probe_input_apply`, `probe_input_toggle`, `probe_tray_exec`, `probe_chunk_exec`, `probe_gate_eval`, `probe_state_write_commit`, `probe_state_change_commit`, `probe_dff_exec`

#### `hebs_probes` (`include/hebs_engine.h`)
- Exposed probe snapshot: `input_apply`, `input_toggle`, `tray_exec`, `chunk_exec`, `gate_eval`, `state_write_commit`, `state_change_commit`, `dff_exec`

#### Benchmark data structs
- `hebs_metric_row_t` (`benchmarks/report_types.h:6`): full benchmark output schema including profile metadata fields (`probe_profile`, `compat_metrics_enabled`).
- `hebs_timer_t` (`benchmarks/timing_helper.h:6`): QPC timer state.

### 5.3 Constants

#### Core/API limits (`include/hebs_engine.h`)
- `HEBS_MAX_PRIMARY_INPUTS = 4096` (`:6`)
- `HEBS_MAX_SIGNAL_TRAYS = 4096` (`:7`)
- `HEBS_BATCH_GATE_COUNT = 64` default, compile-time override supported (`:8-10`)

#### Runner protocol constants (`benchmarks/runner.c`)
- `ITERATIONS = 10`
- `WARMUP_SECONDS = 5.0`
- `SIM_CYCLES = 1000` (default, compile-time override)
- `BENCH_ROOT = "benchmarks/benches/"`
- `REVISION_NAME = "Revision_Combinational_v05"` (default, compile-time override)
- `HEBS_ANCHOR_TOKEN = "Revision_Structure_v07"`
- `METRICS_CSV_PATH` default:
  - perf profile: `benchmarks/results/metrics_history.csv`
  - compat profile: `benchmarks/results/metrics_history_compat.csv`
- `REPORT_HTML_PATH = "benchmarks/results/revision_combinational_v05.html"` (default, compile-time override)
- fixed target set (10 benches): `c17,c432,c499,c6288,c880,s27,s298,s382,s526,s820`

## 6. Public API Usage Language

### 6.1 Expected Flow

1. Compile plan from netlist.
2. Initialize engine with plan.
3. Set primary input values per cycle.
4. Execute `hebs_tick()` per cycle.
5. Read state hash or tracked outputs.
6. Free plan.

### 6.2 Minimal Example (Public API)

```c
#include "hebs_engine.h"

hebs_plan* plan = hebs_load_bench("benchmarks/benches/ISCAS85/c499.bench");
hebs_engine engine = {0};

if (plan && hebs_init_engine(&engine, plan) == HEBS_OK)
{
    for (uint32_t cycle = 0; cycle < 100; ++cycle)
    {
        for (uint32_t pi = 0; pi < plan->num_primary_inputs; ++pi)
        {
            hebs_logic_t v = ((cycle + pi) & 1U) ? HEBS_S1 : HEBS_S0;
            hebs_set_primary_input(&engine, plan, pi, v);
        }
        hebs_tick(&engine, plan);
    }

    uint64_t state = hebs_get_state_hash(&engine);
    (void)state;
}

hebs_free_plan(plan);
```

Example pattern source reference: `benchmarks/runner.c:337-406`.

### 6.3 Primitive API Example

```c
#include "primitives.h"

hebs_logic_t r_and = hebs_eval_and(HEBS_S1, HEBS_W1);
hebs_logic_t r_or  = hebs_eval_or(HEBS_W0, HEBS_Z);
hebs_logic_t r_not = hebs_eval_not(HEBS_S0);

uint64_t packed_and = hebs_gate_and_simd(0x5555555555555555ULL, 0x3333333333333333ULL);
```

Symbol references: `include/primitives.h`, implementations in `core/primitives.c`.

### 6.4 Worth It Probes API

`hebs_get_probes(const hebs_engine* ctx)` returns the raw probe snapshot.

```c
hebs_probes p = hebs_get_probes(&engine);
printf("GE=%llu SW=%llu ICF_num=%llu ICF_den=%llu\n",
	(unsigned long long)p.gate_eval,
	(unsigned long long)p.state_write_commit,
	(unsigned long long)p.state_change_commit,
	(unsigned long long)p.input_toggle);
```

Vectorized tray probe path:
- `const uint64_t* hebs_get_signal_tray(const hebs_engine* ctx, uint32_t tray_index)`
- Returns a direct pointer to the active 64-bit tray so callers can sample packed multi-signal state in one read.
- Runner integration: tray-level CRC fingerprinting now reads trays through this API instead of direct struct pointer access.

## 7. Simulation Data Layout Notes

- Logic states are packed as 2-bit values inside 64-bit trays.
- Signal ID to bit offset mapping: `bit_offset = signal_id * 2`.
- Tray addressing in engine: `tray_index = bit_offset / 64`, `bit_position = bit_offset % 64`.
- Read/write helpers:
  - Checked path: `hebs_read_logic_at_offset`, `hebs_write_logic_at_offset`.
  - Hot-loop path: `hebs_read_logic_u32_fast`, `hebs_write_logic_u32_fast`.

### 7.1 SIMD Batch Executor (Revision_Optimized_v03)

- Core hot loop now executes combinational work in level-bounded batches up to 64 LEP instructions per pass.
- Batch runs are precomputed once in the loader (`hebs_build_batch_spans`) and consumed directly in the tick path.
- Gate evaluation is batch-specialized and branchless within each gate-family lane loop.
- Binary fast path uses direct bit-plane math (2-bit packed values) and LUT fallback preserves 8-state correctness.
- Loop throughput tuning in hot path:
  - local register masks for bit extraction and binary-state checks,
  - explicit `#pragma GCC unroll 4` hints on batch loops,
  - 64-byte alignment on tray storage arrays in `hebs_engine`.
- Compatibility fallback remains in engine for ad hoc plans that do not carry `batch_spans` (for tests and API safety).
- DFF instructions are preserved via destination-value masks in the batch stage, then canonical sequential behavior is finalized by `hebs_sequential_commit`.
- Zero-copy tray swap remains unchanged (`signal_trays <-> next_signal_trays`) after sequential commit.
- Combinational waterfall now has an AVX2 vector path in `core/engine.c` for gate-family spans (`AND/OR/NOT/NAND/NOR/BUF`):
  - processes four 64-bit trays per vector step (`__m256i`),
  - keeps vector math in YMM registers from vector load to vector blend before destination commit,
  - uses a span-width gate (`batch_count >= 64`) to avoid small-span setup tax and preserve throughput stability.
- Scalar path remains as compatibility fallback for narrow spans and unsupported conditions.

### 7.2 SIMD Sequential DFF Commit (Revision_Optimized_v02)

- DFF state is maintained in bit-packed `uint64_t` trays (`dff_state_trays`).
- Commit path consumes precomputed execution metadata:
  - batch spans (`batch_spans`) for loader-generated plans,
  - fallback DFF index/gate scans only for ad hoc plans.
- Adaptive commit selector:
  - if DFF count `< 64`, use lean scalar commit path,
  - otherwise use tray-vectorized commit path.
- Commit flow per tick:
  - gather DFF source values from `next_signal_trays` into tray-local pending masks/values,
  - commit tray-wise writes to `dff_state_trays` and mirrored destinations in `next_signal_trays`,
  - accumulate `internal_transition_count` from tray deltas via `__builtin_popcountll(old_tray ^ new_tray)`.
- AVX2 active path for non-trivial DFF sets:
  - tray commit uses `_mm256_store_si256` on 32-byte aligned tray blocks,
  - transition counting uses vectorized SAD popcount (`_mm256_sad_epu8`) over byte popcount vectors.
- This keeps DFF commit deterministic with the zero-copy tray swap model while lifting wide sequential throughput on AVX2 hosts.

### 7.3 Multi-Tier SIMD Architecture Notes

- Boot-time feature detection is performed in `core/engine.c` and tracks:
  - `AVX2`
  - `AVX512F`
  - `AVX512BW`
  - `AVX512DQ`
  - `AVX512VPOPCNTDQ`
- Runtime behavior:
  - all AVX-512 required extensions present -> AVX-512 sequential path is enabled,
  - AVX-512 missing but AVX2 present -> AVX2 high-tier path is enabled,
  - AVX2 missing -> fallback to v05 optimized 64-bit path.
- Runner logs active mode and missing-extension cause explicitly.

### 7.4 YMM Usage Map (AVX2 Sequential Commit)

- `YMM mask_vec`: pending DFF destination mask (4 x 64-bit trays).
- `YMM old_dff_vec`: previous `dff_state_trays` block.
- `YMM pending_vec`: collected next-state values for DFF destinations.
- `YMM new_dff_vec`: merged committed DFF tray values.
- `YMM delta_vec`: `old_dff_vec ^ new_dff_vec` masked for transition accounting.
- `YMM byte_counts`: nibble-LUT expanded byte popcount vector.
- `YMM sad`: byte-count accumulation via `_mm256_sad_epu8`.
- `YMM next_vec` / `new_next_vec`: merged `next_signal_trays` block committed via `_mm256_store_si256`.

### 7.5 ZMM Usage Map

- `ZMM mask_vec`: pending DFF destination mask (8 x 64-bit trays).
- `ZMM old_dff_vec`: previous `dff_state_trays` block.
- `ZMM pending_vec`: collected next-state values for DFF destinations.
- `ZMM new_dff_vec`: merged committed DFF tray values.
- `ZMM delta_vec`: `old_dff_vec ^ new_dff_vec` masked for transition accounting.
- `ZMM pop_vec`: `_mm512_popcnt_epi64(delta_vec)` transition counts.
- `ZMM next_vec` / `new_next_vec`: merged `next_signal_trays` block committed via `_mm512_store_si512`.
- Primitive ZMM library in `core/primitives.c` includes double-plane `AND`, `NAND`, `XOR`, and `TriSTATE`.
- Primitive AVX2 library in `core/primitives.c` includes double-plane `AND`, `OR`, `NAND`, `XOR`, and `TriSTATE`.

### 7.6 ICF Canonical Formula Lock

- Canonical formula:
  - `ICF = internal_node_transitions / primary_input_transitions`
- Numerator source:
  - `internal_transition_count` is updated in `hebs_sequential_commit`.
  - Commit-path/intrinsic logic:
    - `delta = dff_state_value ^ dff_input_value`
    - `internal_node_transitions += __builtin_popcountll(delta)`
- Denominator source:
  - `input_toggle_count` in engine, accumulated from primary input changes per tick.
- Runner usage:
  - `benchmarks/runner.c` computes ICF from accumulated engine counters via `calculate_icf(internal, primary_input)`.
- History comparability:
  - ICF rows generated before formula lock are pre-canonical and not directly comparable to post-lock rows unless re-run under the locked formula.

## 8. Build and Operation Reference

### 8.1 Build

```bash
make clean
make
```

Makefile include path now uses root include directory:
- `CFLAGS = -Iinclude -O3 -mavx2 -march=native ...` (`Makefile`)

### 8.2 Test

```bash
./build/hebs_test
```

### 8.3 Benchmark

```bash
./build/hebs_cli
```

The stock runner scans all benchmark suites under `benchmarks/benches/`.

## 9. Git Ledger

### 9.1 Commit History (local)

| Commit | Date (local) | Author | Subject |
|---|---|---|---|
| `da7ebff` | 2026-03-05 00:33:51 -0600 | David Cole | Revision_Structure_v04: Integrity Lock |
| `2c25ed2` | 2026-03-04 19:30:21 -0600 | David Cole | Milestone 1: Project Bootstrap & Architectural Plan |

Source command:
```bash
git log --date=iso --pretty=format:"%h|%ad|%an|%s"
```

### 9.2 Push / Remote Update Ledger (`origin/main` reflog)

| Commit | Remote Ref Event Time | Event |
|---|---|---|
| `da7ebff` | 2026-03-05 00:33:58 -0600 | update by push |
| `2c25ed2` | 2026-03-04 19:30:24 -0600 | update by push |

Source command:
```bash
git reflog show --date=iso refs/remotes/origin/main
```

## 10. Revision_Structure_v04 Performance Ledger

### 10.1 Baseline Block (from metrics history)

Revision header:
- `Revision_Structure_v04 | 00:07:34 | 2026-03-05 | 2c25ed2`

Target baselines:

| Bench | GEPS_p50 | Logic_CRC32 |
|---|---:|---|
| `c17` | 120000000.00 | `0xB018CC06` |
| `c499` | 263535663.35 | `0xB2966C37` |
| `c6288` | 167452241.48 | `0x90B28CB8` |

Source: `benchmarks/results/metrics_history.csv`.

### 10.2 Post-Normalization Validation Snapshot

Latest stock runner block header:
- `Revision_Structure_v04 | 10:20:02 | 2026-03-05 | da7ebff`

`c499` row in latest stock runner block:
- `GEPS_p50 = 199309372.63`
- `Logic_CRC32 = 0xB2966C37`
- `Regression_Gate = FAIL` (throughput regression)

Sequential benchmark sample (`s27`) in latest stock runner block:
- `Logic_CRC32 = 0x58693AD0`
- Baseline value differed (`0x37B75CD5`), reflecting sequential-path behavior change after Parallel DFF Tray implementation.

Interpretation:
- Combinational benchmark fingerprints remain stable.
- Sequential benchmark fingerprints changed due DFF commit-path behavior change.
- Throughput is currently below prior v04 baselines and requires optimization work.

### 10.3 Revision_Combinational_v01 Validation Snapshot

Latest runner block header:
- `Revision_Combinational_v01 | 2026-03-05` (latest appended run in `metrics_history.csv`)

`c6288` validation row:
- `GEPS_p50 = 183943051.01`
- `Logic_CRC32 = 0x90B28CB8`
- `Fingerprint_Stable = 1`
- `Regression_Gate = PASS`

Interpretation:
- `c6288` throughput exceeds the 182M GEPS target gate.
- Logic fingerprint remains locked to the established baseline CRC32.

### 10.4 Delta Anchor Rule (Current)

- `Prev_GEPS` and `Prev_ICF` are now anchored to the latest available metrics from prior revisions only.
- Rows from the active revision token are excluded from the `Prev_*` anchor to prevent same-revision self-comparison drift.
- `Base_GEPS` for `Revision_Combinational*` runs is now anchored to the mean GEPS of token `Revision_Structure_v07` when that token exists in the ledger.
- Implementation anchors:
  - `benchmarks/csv_export.h`: `lookup_history_for_bench_suite(...)`
  - `benchmarks/csv_export.h`: `lookup_revision_mean_geps(...)`
  - `benchmarks/runner.c`: history lookup call with `REVISION_NAME` and `ANCHOR_REVISION_TOKEN`

### 10.5 Revision_Combinational_v02 Validation Snapshot

Latest runner block header:
- `Revision_Combinational_v02 | 2026-03-05` (latest appended run in `metrics_history.csv`)

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

`c6288` validation row:
- `GEPS_p50 = 268220927.83`
- `Logic_CRC32 = 0x90B28CB8`
- `Fingerprint_Stable = 1`

Hot path profile summary (`c6288` microprofile):
- Pre-optimization:
  - batch overhead: `2.551 ns/op`
  - NAND math: `1.892 ns/op`
  - XOR math: `1.921 ns/op`
- Post-optimization:
  - batch overhead: `2.205 ns/op`
  - NAND math: `1.919 ns/op`
  - XOR math: `1.911 ns/op`
- Interpretation:
  - dominant cost source is batch loop overhead, not primitive NAND/XOR math.
  - alignment and loop-overhead reduction improved executor throughput while preserving CRC determinism.

### 10.6 Revision_Combinational_v03 Validation Snapshot

Latest runner block header:
- `Revision_Combinational_v03 | 2026-03-05` (latest appended run in `metrics_history.csv`)

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

Global mean GEPS:
- `242589192.16` (`+32.76%` vs v07 anchor)

`c6288` validation row:
- `GEPS_p50 = 327172212.21`
- `Logic_CRC32 = 0x90B28CB8`
- `Fingerprint_Stable = 1`

Batch-size audit (`c6288` probe, p50 GEPS):
- `batch=32` -> `312710328.76`
- `batch=64` -> `328752211.19` (selected)
- `batch=96` -> `328573371.41`
- `batch=128` -> `327682083.28`
- CRC lock preserved (`0x90B28CB8`) across all tested sizes.

Secondary hot-path profile (`c6288`, v03):
- `overhead_ns_per_span = 3.817`
- `overhead_ns_per_gate_equiv = 0.250`
- `nand_ns_per_op = 2.103`
- `xor_ns_per_op = 1.664`
- overhead share vs NAND (`gate-equivalent basis`) = `10.61%`

Interpretation:
- precomputed batch spans removed most setup overhead from the tick hot path.
- overhead share dropped below the 50% target while maintaining deterministic CRC32 output.

### 10.7 Revision_Combinational_v04 Validation Snapshot

Latest runner block header:
- `Revision_Combinational_v04 | 14:05:51 | 2026-03-05` (latest appended run in `metrics_history.csv`)

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

Global mean GEPS:
- `232215599.71` (`+27.08%` vs v07 anchor)

`c6288` determinism gate:
- `GEPS_p50 = 325102605.25`
- `Logic_CRC32 = 0x90B28CB8`
- `Fingerprint_Stable = 1`

Sequential focus checks:
- `s298`:
  - `GEPS_p50 = 251180582.80`
  - delta vs latest v03 (`285407725.32`) = `-11.99%`
  - `Logic_CRC32 = 0x945FC7DE`
- `s5378`:
  - `GEPS_p50 = 385759103.41`
  - first tracked baseline appears in v04 ledger; nearest prior row delta (`v04-to-v04`) = `-0.01%`
  - `Logic_CRC32 = 0x8E95C42D`

Interpretation:
- SIMD sequential commit preserves determinism fingerprints.
- throughput impact is benchmark-dependent and currently negative on several small/medium benches relative to v03.

### 10.8 Revision_Combinational_v05 Validation Snapshot

Latest runner block header:
- `Revision_Combinational_v05 | 14:18:32 | 2026-03-05` (latest appended run in `metrics_history.csv`)

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

Global mean GEPS:
- Full discovered suite (`13` benches, includes `s5378`): `265074344.93` (`+45.07%` vs v07 anchor)
- Titan-target suite (`12` benches = original 10 + `c7552` + `s38584`): `255057064.67` (`+39.58%` vs v07 anchor)

Titan table (p50 GEPS / CRC32):
- `c6288`: `321297960.06` / `0x90B28CB8`
- `c7552`: `238741134.88` / `0x4551B972`
- `s5378`: `385281708.06` / `0x8E95C42D`
- `s38584`: `420937996.87` / `0x99D32148`

Sequential stabilization check:
- `s298` moved from v04 `251180582.80` to v05 `295555555.56` (`+17.67%` vs v04), which resolves the v04 regression against the v03 baseline.

Protocol gate note:
- suite run still flagged one regression gate (`c880`, `-2.79%` vs previous row) even though Titan/global objectives were met.

Determinism gate:
- `c6288` CRC remains locked at `0x90B28CB8`.
- `s5378` CRC remains locked at `0x8E95C42D`.

### 10.9 Revision_Optimized_v01 Validation Snapshot

Latest runner block header:
- `Revision_Optimized_v01 | 14:42:22 | 2026-03-05` (latest appended run in `metrics_history.csv`)

AVX-512 availability:
- Runtime reported missing extensions: `AVX512F AVX512BW AVX512DQ AVX512VPOPCNTDQ`
- Execution used fallback v05 optimized 64-bit path.

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

Global mean GEPS (full discovered suite, 13 benches):
- `265418633.52` (`+45.25%` vs v07 anchor)

Titan table (p50 GEPS / CRC32):
- `c6288`: `322305596.56` / `0x90B28CB8`
- `c7552`: `237522088.89` / `0x4551B972`
- `s5378`: `383956392.76` / `0x8E95C42D`
- `s38584`: `422209993.12` / `0x99D32148`

Performance objective check:
- `s38584` peak target `>500M GEPS` was not met on this host in fallback mode (`422.21M` p50).

### 10.10 Revision_Optimized_v02 Validation Snapshot

Latest runner block header:
- `Revision_Optimized_v02 | 15:07:27 | 2026-03-05` (latest appended run in `metrics_history.csv`)

SIMD mode:
- Runtime mode: `AVX2 active`
- Missing extensions reported: `AVX512F AVX512BW AVX512DQ AVX512VPOPCNTDQ`
- Fallback path: AVX2 high-tier path selected (not v05 scalar fallback).

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

Global mean GEPS (full discovered suite, 13 benches):
- `262517468.10` (`+43.67%` vs v07 anchor)

Titan table (p50 GEPS / CRC32):
- `c6288`: `321426197.13` / `0x90B28CB8`
- `c7552`: `235610128.01` / `0x4551B972`
- `s5378`: `383483504.56` / `0x8E95C42D`
- `s38584`: `413934950.21` / `0x99D32148`

Determinism hard gate:
- c6288 hard-stop lock implemented in runner with required CRC32 `0x90B28CB8`.
- Latest run passed lock and completed all 13 benches; regression gates tripped on `s27` and `s526` (`failed=2`).

Direct v05 (64-bit) vs v02 (AVX2) comparison:
- v05 global mean (13 benches): `265074344.93`
- v02 global mean (13 benches): `262517468.10`
- absolute delta: `-2556876.83 GEPS`
- relative delta: `-0.96%` global mean
- per-bench mean delta (v02 vs v05): `-1.04%`

### 10.11 Revision_Optimized_v03 Validation Snapshot

Latest runner block header:
- `Revision_Optimized_v03 | 15:22:10 | 2026-03-05` (latest appended run in `metrics_history.csv`)

Baseline scope:
- Protocol temporarily reverted to the original 10-bench anchor set from `Revision_Structure_v07`.
- Excluded from v03 protocol run: `c7552`, `s5378`, `s38584`.

SIMD mode:
- Runtime mode: `AVX2 active`
- Missing extensions reported: `AVX512F AVX512BW AVX512DQ AVX512VPOPCNTDQ`
- Compile flags remain enforced: `-mavx2 -march=native` (`Makefile`).

Global anchor:
- Anchor token: `Revision_Structure_v07`
- Anchor mean GEPS: `182726469.90`

10-bench global mean GEPS:
- `231727360.94`
- Architecture Gain vs v07 anchor: `+26.82%`

c6288 determinism gate:
- `GEPS_p50 = 295407097.79`
- `Logic_CRC32 = 0x90B28CB8` (hard-stop lock preserved)

Protocol result:
- Run completed all 10 benches with CRC lock intact.
- Regression gate counters still tripped on several benches versus immediate previous-row history due the new 2000-cycle protocol and waterfall path.

## 11. Structural Normalization Ledger

Executed normalization changes:
1. Created root `include/` directory.
2. Moved headers from `core/includes/` to `include/`.
3. Updated `Makefile` include path to `-Iinclude`.
4. Rebuilt with `make clean && make` successfully.

Current normalized tree (functional roots):

```text
C:\DEV\HEBS_I2
|-- core/
|   |-- engine.c
|   |-- loader.c
|   |-- primitives.c
|   `-- state_manager.c
|-- include/
|   |-- hebs_engine.h
|   |-- hebs_types.h
|   |-- primitives.h
|   `-- state_manager.h
|-- benchmarks/
|   |-- benches/
|   |-- results/
|   |-- runner.c
|   |-- protocol_helper.h
|   |-- report_types.h
|   |-- timing_helper.h
|   |-- html_report.h
|   `-- csv_export.h
`-- tests/
    |-- test_runner.c
    `-- results/
```

## 12. Governance Mandates

1. Governance and canonical docs remain in repository root (`AGENTS.md`, `HEBS_PLAN.md`, `TAB_PROTOCOL.md`, `HEBS_REPO_MAP.md`) and references should point to those root paths.
2. `HEBS_REPO_MAP.md` must be updated in the same approved change whenever new functions, variables, types, constants, files, or directories are added, removed, renamed, or behaviorally changed.
3. Report artifact naming must use flat revision tokens (`<RevisionName>_vNN`) and benchmark HTML filenames must be `<RevisionToken>.html`.
4. HTML lifecycle is mandatory and immutable:
   - each minor run writes a new incremented HTML file;
   - existing HTML artifacts must never be overwritten or deleted by the runner.
5. Engine probe constraint is mandatory:
   - engine may expose raw probes only;
   - probe APIs may expose raw probe structs only and must not return derived metrics;
   - benchmark/test metric derivation belongs to tools/runners, not engine internals.
6. When a directive conflicts with documented requirements, each conflict must be named and approved individually before execution.
7. Mandatory approval language for each conflict:
   - `This <DeviationName> is not the same as required in documentation. Reply APPROVED <DeviationName> to explicitly confirm and continue.`
8. ICF definition is locked:
   - `ICF = internal_node_transitions / primary_input_transitions`
   - any alternate ICF formula is a documented deviation and requires explicit conflict approval.
9. Probe profiles are canon-locked for `probe_fix_v03` through `probe_fix_v05`:
   - exactly one of `HEBS_PROBE_PROFILE_PERF` or `HEBS_PROBE_PROFILE_COMPAT` must be active;
   - canonical benchmark history must use perf profile only.
10. Benchmark suite output schema stability:
   - new profile metadata columns must append at row end and must not reorder existing canonical columns.
11. Canonical and compatibility benchmark histories are separated:
   - canonical: `benchmarks/results/metrics_history.csv`
   - compatibility-only: `benchmarks/results/metrics_history_compat.csv`
   - compatibility run tokens use `_compat` suffix (example: `probe_fix_v05_compat`).
12. Normative references must explicitly distinguish test suite rules from benchmark suite rules when behavior differs by profile.
13. Probe-fix minor revision naming is flat-token only (`probe_fix_vNN`), with compatibility exception `probe_fix_vNN_compat`; nested suffix naming is non-canonical.

## 13. Known Gaps / Follow-Ups

- `HEBS_PLAN.md` still describes conceptual paths/names that do not exactly match current symbol names.
- If strict canonical naming is required, a dedicated alignment pass should map or rename exported APIs deliberately.
- AVX2 and AVX-512 primitive libraries are available, but combinational gate evaluation still uses tray-scalar branchless bit-plane math in `core/engine.c`.
- Storage remains 2-bit tray encoded in the runtime engine; any future expansion of stored logic cardinality requires a storage-format migration plan.

## 14. Rebuild Canon Track (2026-03-05)

### Revision_Combinational_v01 (rebuild)

Engine/data model changes:
1. Added combinational execution metadata types:
   - `hebs_exec_instruction_t`
   - `hebs_gate_span_t`
2. Added `hebs_plan` execution-plan fields:
   - `comb_instruction_count`
   - `comb_instruction_indices`
   - `comb_exec_data`
   - `comb_span_count`
   - `comb_spans`
3. Loader now precomputes combinational level/type spans and per-gate lane metadata in:
   - `hebs_build_comb_execution_plan(...)`
4. `hebs_tick(...)` now dispatches combinational math through 64-gate chunk execution spans:
   - `hebs_execute_binary_chunk(...)`
   - `hebs_execute_unary_chunk(...)`
   - `hebs_execute_combinational_batched(...)`
5. Fallback scalar combinational path retained:
   - `hebs_execute_combinational_fallback(...)`

### Revision_Combinational_v02 (rebuild)

Benchmark/anchor changes:
1. Added revision-anchor lookup helper in CSV layer:
   - `lookup_revision_mean_geps(...)` in `benchmarks/csv_export.h`
2. `benchmarks/runner.c` now anchors `Base_GEPS` for combinational revisions to:
   - token: `Revision_Structure_v07`
   - anchor mean GEPS: first matching v07 block mean

Hot-path profiling additions:
1. Added c6288 profile helper in runner:
   - `hebs_profile_c6288_hot_path(...)`
2. Profile output reports:
   - batch overhead ns per gate-equivalent
   - NAND bit-plane math ns/op
   - XOR bit-plane math ns/op
   - overhead share vs NAND path

Batch-loop tuning:
1. Added 64-byte tray alignment macro:
   - `HEBS_ALIGN64`
2. Applied aligned trays to runtime buffers:
   - `tray_plane_a`
   - `tray_plane_b`
   - `dff_state_trays`
3. Added local tray pointer pinning and `#pragma GCC unroll 4` in chunk hot loops.

### Revision_Combinational_v03 (rebuild)

Tight-loop overhead reductions:
1. Batch chunk control is now compile-time configurable:
   - `HEBS_BATCH_GATE_CHUNK` (default `64`)
2. Loader now pre-splits combinational spans into fixed-size chunks at load-time:
   - runtime `while` chunk splitting removed from hot path
3. Engine now executes pre-split spans directly:
   - one span dispatch -> one chunk loop

Protocol support changes:
1. Added non-canon runner controls:
   - `HEBS_SKIP_ARTIFACTS=1` to suppress CSV/HTML writes
   - `HEBS_ENABLE_TITAN_BENCHES=1` to append Titan benches when present

Batch-size audit (non-canon, c6288 p50 GEPS):
1. chunk 32: `338055833.89`
2. chunk 64: `337444445.62`
3. chunk 96: `337861935.29`
4. chunk 128: `337058373.47`

Hot-path profile (canonical v03 run, c6288):
1. overhead ns/gate-equiv: `1.285`
2. NAND ns/op: `1.951`
3. XOR ns/op: `2.581`
4. overhead share vs NAND path: `39.72%`

### Revision_Combinational_v04 (rebuild)

Sequential/vectorized updates:
1. Added DFF execution metadata in `hebs_plan`:
   - `dff_exec_count`
   - `dff_exec_data`
   - `dff_commit_mask`
2. Loader now precomputes DFF commit metadata:
   - `hebs_build_dff_execution_plan(...)`
3. `hebs_sequential_commit(...)` now stages DFF tray updates and commits by tray mask:
   - tray-level XOR delta accumulation with `__builtin_popcountll`
   - tray-level DFF state transfer into `next_signal_trays`

Probe API:
1. Added `hebs_get_signal_tray(const hebs_engine*, uint32_t)` in public API.
2. Runner logic CRC path now reads trays through `hebs_get_signal_tray(...)`.

Canonical v04 notes:
1. c6288 CRC lock remained stable: `0x90B28CB8`.
2. Non-canon Titan sanity mode is wired, but Titan bench files are absent in this repo snapshot (`c7552/s5378/s38584` missing), so only base 10 benches execute.

### Revision_Combinational_v05 (rebuild)

Adaptive sequential commit:
1. Added DFF-mode threshold in `core/state_manager.c`:
   - `HEBS_DFF_SCALAR_THRESHOLD = 64`
2. Added scalar bypass path:
   - `hebs_sequential_commit_scalar(...)`
3. Added large-circuit tray-vector path:
   - `hebs_sequential_commit_vectorized(...)`
4. Dispatcher in `hebs_sequential_commit(...)` now selects scalar/vector mode by `dff_exec_count`.

Canonical v05 notes:
1. c6288 CRC lock remained stable: `0x90B28CB8`.
2. Non-canon Titan sanity mode remains active but Titan bench files are absent in this repo snapshot (`c7552/s5378/s38584` missing), so Titan GEPS checks are blocked by missing inputs.
3. Pass A locality packing is active in loader combinational plan construction:
   - sort key: `src_a_tray`, `src_b_tray`, `dst_tray`, then lane shifts, then instruction index.
4. Rejected Pass B threshold-packing trial was removed from active `metrics_history.csv` and archived at:
   - `benchmarks/results/archive/non_canon_passB_2026-03-06.md`
5. Latest canonical v05 run after Pass B removal:
   - header: `Revision_Combinational_v05 | 01:04:23 | 2026-03-06 | 18fae8a`
   - global mean GEPS: `284269232.16`
   - c6288 p50 GEPS: `352790933.62`
   - c6288 CRC32: `0x90B28CB8`
6. Combinational gate matrix expanded in parser + executor:
   - Added loader/engine support for `XOR`, `XNOR`, `TRI`/`TRISTATE`, `VCC`, `GND`, `PUP`/`PULLUP`, `PDN`/`PULLDOWN`.
   - Unknown gate opcodes now fail bench load instead of silently degrading to `BUF`.
   - Added weak pull-down primitive path (`hebs_gate_weak_pull_down_simd`, `hebs_eval_weak_pull_down`) for `PDN`.

