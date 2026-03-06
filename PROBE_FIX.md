# PROBE_FIX

Status: Canon staging document for `probe_fix_v03`
Scope: Probe architecture hardening and performance protection

## 1. Scope

`probe_fix_v03` is a narrow correction pass.

Goals:
- protect engine throughput
- preserve a minimal permanent probe surface
- isolate compatibility-only probe semantics from canonical benchmark reporting

`probe_fix_v03` must not expand probe surface beyond what is required for this correction.

## 2. Probe Profiles

Exactly one probe profile must be selected at build time.

Required profiles:
- `HEBS_PROBE_PROFILE_PERF`
- `HEBS_PROBE_PROFILE_COMPAT`

Build rule:
- exactly one profile macro must be active
- default canonical build profile is `HEBS_PROBE_PROFILE_PERF`

Derived macro:
- `HEBS_COMPAT_PROBES_ENABLED`

Semantics:
- `HEBS_PROBE_PROFILE_PERF`
  - permanent probes enabled
  - compatibility probes disabled
  - canonical benchmark suite profile
- `HEBS_PROBE_PROFILE_COMPAT`
  - permanent probes enabled
  - compatibility probes enabled
  - non-canonical compatibility analysis profile

## 3. Probe Classification for v03

Permanent probes (enabled in all profiles):
- `input_apply`
- `chunk_exec`
- `gate_eval`
- `dff_exec`

Compatibility probes (enabled only in compat profile):
- `input_toggle`
- `state_change_commit`

Remove in v03:
- `state_write_attempt`

Deferred out of v03:
- `tray_exec`
- `state_write_commit`

Deferred probes must not be expanded, redesigned, or newly consumed in `probe_fix_v03`.

## 4. Benchmark Suite Reporting Rules

### 4.1 Perf profile output

When `HEBS_PROBE_PROFILE_PERF` is active:
- compatibility-derived fields must emit numeric `0`
- output rows must append:
  - `Probe_Profile` column with value `perf`
  - `Compat_Metrics_Enabled` column with value `0`

Compatibility-derived fields include:
- `primary_input_transitions`
- `internal_transitions`
- `ICF`
- `toggle_rate`
- `activity_factor`
- `transition_entropy`
- all event and toggle-density style fields derived from compatibility probes

### 4.2 Compat profile output

When `HEBS_PROBE_PROFILE_COMPAT` is active:
- compatibility-derived fields are computed normally
- output rows must append:
  - `Probe_Profile` column with value `compat`
  - `Compat_Metrics_Enabled` column with value `1`

### 4.3 CSV schema rule

New output fields:
- must be appended at the end of each row
- must not reorder existing canonical columns in `probe_fix_v03`

## 5. Canonical Artifact Separation

Compatibility-profile benchmark suite runs must not pollute canonical benchmark history.

Required rule:
- perf profile benchmark suite runs append to canonical ledger:
  - `benchmarks/results/metrics_history.csv`
- compat profile benchmark suite runs append to non-canonical ledger:
  - `benchmarks/results/metrics_history_compat.csv`

Canonical regression history must contain perf profile runs only.

## 6. Regression Authority

Canonical regression decisions remain based on:
- GEPS
- latency and runtime statistics
- CRC and correctness stability

Compatibility-derived metrics:
- do not participate in canonical regression gating
- do not block canonical perf-profile acceptance

## 7. Test Suite vs Benchmark Suite Rules

Normative text must use explicit terms:
- `test suite`
- `benchmark suite`

Do not use mixed generic language for required behavior.

### 7.1 Test suite requirements

Perf profile test suite:
- built with `HEBS_PROBE_PROFILE_PERF`
- must not require compatibility probe values

Compat profile test suite:
- built with `HEBS_PROBE_PROFILE_COMPAT`
- may assert `input_toggle` and `state_change_commit`

Any test suite assertion that depends on compatibility probes must be isolated to compat profile only.

### 7.2 Benchmark suite requirements

Perf profile benchmark suite:
- canonical
- compatibility-derived metrics inactive
- canonical recording allowed

Compat profile benchmark suite:
- non-canonical
- compatibility-derived metrics active
- recording must not enter canonical regression history

## 8. Measurement Policy

All probe acceptance decisions in `probe_fix_v03` must be measured under controlled conditions.

Required conditions:
- same host
- same build class
- same benchmark suite inputs
- same runtime conditions
- median-of-10 runs as decision statistic

Baseline reference:
- use the agreed perf-oriented baseline established before enabling compatibility probes for this run series

## 9. Acceptance Thresholds

A probe may remain in the permanent perf profile only if its inclusion causes:
- no more than 1.0% median GEPS regression on the canonical benchmark suite
- no more than 2.5% regression on any individual benchmark unless explicitly approved

If a probe exceeds these limits:
- it must not remain in perf profile
- it may only be compatibility-only or removed

## 10. Required v03 Execution Order

Run 1 (`probe_fix_v03`):
1. harden canon documentation only
2. no code changes
3. commit and push

Run 2 (`probe_fix_v03_v01`):
1. remove `state_write_attempt`
2. add centrally enforced probe-profile selection
3. gate `input_toggle` and `state_change_commit` behind compat profile
4. commit and push

Run 3 (`probe_fix_v03_v02`):
1. append `Probe_Profile` and `Compat_Metrics_Enabled` to benchmark output rows
2. ensure perf profile emits numeric zero for compatibility-derived metrics
3. separate canonical vs compat benchmark artifacts
4. split test suite expectations by profile
5. run controlled A/B measurements using median-of-10
6. commit and push

If thresholds are not met after Run 3, further action requires a new approved scope.

## 11. Non-Goals

`probe_fix_v03` must not:
- introduce new permanent probes beyond the approved set
- redesign deferred probes
- change canonical regression authority
- reorder existing CSV columns beyond appending new fields
