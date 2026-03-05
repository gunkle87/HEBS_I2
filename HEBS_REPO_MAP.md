# HEBS_REPO_MAP

Status: Active technical reference
Generated: 2026-03-05
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
| `hebs_tick_execute()` | `hebs_tick()` | `core/engine.c:84` |
| `hebs_compile_lep()` | `hebs_load_bench()` + internal parse/levelize/pack | `core/loader.c:735`, `core/loader.c:544` |
| LEP hash generation | `hebs_calculate_lep_hash()` | `core/loader.c:685` |

## 3. Directory Dictionary

### 3.1 Top-Level

| Path | Type | Role |
|---|---|---|
| `AGENTS.md` | Governance | Agent authorization and execution policy.
| `HEBS_PLAN.md` | Architecture Plan | High-level architecture, component roles, target structure.
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
| `core/engine.c` | Simulation execution and bit-tray state updates. | `hebs_init_engine` (`:55`), `hebs_tick` (`:84`), `hebs_get_state_hash` (`:151`), `hebs_set_primary_input` (`:177`), `hebs_get_primary_input` (`:201`) |
| `core/loader.c` | `.bench` parsing, signal table build, levelization, LEP packing, LEP hash. | `hebs_load_bench` (`:735`), `hebs_levelize_and_pack` (`:544`), `hebs_calculate_lep_hash` (`:685`), `hebs_free_plan` (`:809`) |
| `core/primitives.c` | 8-state primitive library with SIMD tray operators and scalar evaluators. | `hebs_gate_and/or/xor/nand/nor/xnor/not/buf`, `hebs_gate_weak_pull/strong_pull/vcc/gnd/tristate`, `hebs_eval_*` family |
| `core/state_manager.c` | Sequential state manager for parallel DFF tray commit. | `hebs_sequential_commit` |

### 3.3 `/include` (Public API Headers)

| File | Role | Key Contracts |
|---|---|---|
| `include/hebs_engine.h` | Public engine/loader API and core types. | enums, plan/engine/metrics structs, engine+loader+metrics API prototypes |
| `include/primitives.h` | Primitive logic interface. | SIMD tray operators + scalar primitive evaluators |
| `include/state_manager.h` | Sequential state manager interface. | `hebs_sequential_commit` |

### 3.4 `/benchmarks` (Runner + Reporting)

| File | Role | Key Symbols |
|---|---|---|
| `benchmarks/runner.c` | Benchmark executor, warmup, 10-iteration protocol, CSV/HTML output. | runner constants (`:15-22`), `hebs_run_single_bench` (`:309`), `main` (`:433`) |
| `benchmarks/protocol_helper.h` | Statistical helpers (median/percentiles/ICF). | `calculate_p50`, `calculate_percentile`, `calculate_icf` |
| `benchmarks/report_types.h` | Metric row schema. | `hebs_metric_row_s` (`:6`) |
| `benchmarks/timing_helper.h` | High-resolution timer wrapper. | `hebs_timer_s` (`:6`) |
| `benchmarks/html_report.h` | HTML report emitter. | `generate_master_report` (`:18`) |
| `benchmarks/csv_export.h` | CSV parse/history lookup/append. | `lookup_history_for_bench_suite` (`:129`), `append_metrics_history_csv` (`:217`) |
| `benchmarks/benches/ISCAS85/c17.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c432.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c499.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c6288.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS85/c880.bench` | Benchmark netlist | ISCAS85 sample |
| `benchmarks/benches/ISCAS89/s27.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s298.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s382.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s526.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/benches/ISCAS89/s820.bench` | Benchmark netlist | ISCAS89 sample |
| `benchmarks/results/metrics_history.csv` | Historical metric ledger | revision blocks + per-bench rows |
| `benchmarks/results/revision_structure.html` | HTML benchmark report | latest runner report |

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

### 5.2 Structs

#### `hebs_lep_instruction_t` (`include/hebs_engine.h:40`)
- `gate_type`, `input_count`, `level`
- `src_a_bit_offset`, `src_b_bit_offset`, `dst_bit_offset`

#### `hebs_plan` (`include/hebs_engine.h:51`)
- Metadata: `lep_hash`, `level_count`, `signal_count`, `gate_count`, `tray_count`, `propagation_depth`, `fanout_avg`, `fanout_max`
- IO counts: `num_primary_inputs`, `num_primary_outputs`
- Buffers: `primary_input_ids`, `lep_data`
- Sequential index: `dff_instruction_count`, `dff_instruction_indices`
- Internal transition mask: `internal_transition_lsb_mask` (LSB-per-node mask for non-PI signals)

#### `hebs_engine` (`include/hebs_engine.h:68`)
- Runtime counters: `current_tick`, `input_toggle_count`, `internal_transition_count`, `cycles_executed`, `vectors_applied`, `gate_evals`, `signal_writes_committed`
- Storage planes: `tray_plane_a`, `tray_plane_b`, `dff_state_trays`
- Active pointers: `signal_trays`, `next_signal_trays` (zero-copy swap)
- PI tracking: `previous_input_state[HEBS_MAX_PRIMARY_INPUTS]`

#### `hebs_metrics` (`include/hebs_engine.h`)
- Topology (loader): `gate_count`, `net_count`, `pi_count`, `po_count`, `fanout_avg`, `fanout_max`, `level_depth`
- Activity (engine): `cycles_executed`, `vectors_applied`, `gate_evals`, `signal_writes_committed`
- ICF counters: `primary_input_transitions`, `internal_node_transitions`

#### Benchmark data structs
- `hebs_metric_row_t` (`benchmarks/report_types.h:6`): full benchmark output schema.
- `hebs_timer_t` (`benchmarks/timing_helper.h:6`): QPC timer state.

### 5.3 Constants

#### Core/API limits (`include/hebs_engine.h`)
- `HEBS_MAX_PRIMARY_INPUTS = 4096` (`:6`)
- `HEBS_MAX_SIGNAL_TRAYS = 4096` (`:7`)

#### Runner protocol constants (`benchmarks/runner.c:15-22`)
- `ITERATIONS = 10`
- `WARMUP_SECONDS = 5.0`
- `SIM_CYCLES = 100`
- `BENCH_ROOT = "benchmarks/benches/"`
- `REVISION_NAME = "Revision_Structure_v07"`
- `METRICS_CSV_PATH = "benchmarks/results/metrics_history.csv"`
- `REPORT_HTML_PATH = "benchmarks/results/revision_structure_v07.html"`

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

`hebs_get_metrics(const hebs_engine* ctx, const hebs_plan* plan)` returns a high-precision probe snapshot for loader topology, runtime activity, and ICF counters.

```c
hebs_metrics m = hebs_get_metrics(&engine, plan);
printf("GE=%llu SW=%llu ICF_num=%llu ICF_den=%llu\n",
	(unsigned long long)m.gate_evals,
	(unsigned long long)m.signal_writes_committed,
	(unsigned long long)m.internal_node_transitions,
	(unsigned long long)m.primary_input_transitions);
```

## 7. Simulation Data Layout Notes

- Logic states are packed as 2-bit values inside 64-bit trays.
- Signal ID to bit offset mapping: `bit_offset = signal_id * 2`.
- Tray addressing in engine: `tray_index = bit_offset / 64`, `bit_position = bit_offset % 64`.
- Read/write helpers: `hebs_read_logic_at_offset` and `hebs_write_logic_at_offset` (`core/engine.c:5`, `core/engine.c:30`).

### 7.1 Parallel DFF Tray Architecture

- DFF state is maintained in bit-packed `uint64_t` trays (`dff_state_trays`), not a scalar byte-array model.
- DFF instructions are pre-indexed in the compiled plan (`dff_instruction_count`, `dff_instruction_indices`) so sequential commit does not scan all gates each tick.
- Per tick flow:
  - Combinational writes go to `next_signal_trays`.
  - `hebs_sequential_commit` copies DFF source nodes from `next_signal_trays` into `dff_state_trays`.
  - The same committed value is written to the DFF destination in `next_signal_trays`.
  - Engine performs a zero-copy pointer swap (`signal_trays <-> next_signal_trays`) for the next tick.
- Deviation note: this intentionally replaces old byte-array DFF handling. Future agents must not revert to scalar per-node DFF state storage.

### 7.2 ICF Canonical Formula Lock

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
- `CFLAGS = -Iinclude ...` (`Makefile`)

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

1. `HEBS_REPO_MAP.md` must be updated in the same approved change whenever new functions, variables, types, constants, files, or directories are added, removed, renamed, or behaviorally changed.
2. Report artifact naming must use the active revision token as the filename root.
3. HTML lifecycle is mandatory:
   - Overwrite HTML per minor revision within the same major revision.
   - Freeze the last minor revision HTML as that major revision's final HTML.
   - Never overwrite a prior major revision final HTML from a subsequent revision.
4. When a directive conflicts with documented requirements, each conflict must be named and approved individually before execution.
5. Mandatory approval language for each conflict:
   - `This <DeviationName> is not the same as required in documentation. Reply APPROVED <DeviationName> to explicitly confirm and continue.`
6. ICF definition is locked:
   - `ICF = internal_node_transitions / primary_input_transitions`
   - any alternate ICF formula is a documented deviation and requires explicit conflict approval.

## 13. Known Gaps / Follow-Ups

- `HEBS_PLAN.md` still describes conceptual paths/names that do not exactly match current symbol names.
- If strict canonical naming is required, a dedicated alignment pass should map or rename exported APIs deliberately.
- Parallel DFF Tray architecture introduced measurable GEPS regression in current runs; hot-path optimization is required to recover throughput.
- Extended primitive functions are implemented in `primitives.c`, but the current gate parser/dispatcher still actively routes LEP execution through `AND/OR/NOT/NAND/NOR/BUF/DFF` gate types.

