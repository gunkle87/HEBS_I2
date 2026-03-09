# PROBE_PROTOCOL.md

Defines rules governing engine probes used for benchmarking and tooling.

------------------------------------------------------------------------

## 1. Probe Purpose
    Engine probes expose raw execution facts.
    Probes exist only to allow external tools (benchmark runners, tooling,
    analysis) to observe engine behavior.
    Probes must not change simulation behavior.

## 2. Engine Boundary
    The engine may emit raw probe counters only.
    The engine must not compute or assemble benchmark metrics.
    Metric calculation must occur outside the engine.
    Engine output = facts. Tools derive conclusions.

## 3. Probe Update Rule
    Probe updates are permitted only at existing execution points.
    A probe update must never introduce a new execution path.

## 4. Cost Rule
    Probe updates must be constant time O(1).
    Probe updates must not introduce:
    -   scan passes
    -   state sweeps
    -   recomputation
    -   aggregation passes
    -   dirty-set construction
    -   classification loops

## 5. Allowed Probe Pattern
    Example valid probe updates:
        ctx->probe_gate_eval++;
        ctx->probe_chunk_exec++;
        ctx->probe_dff_exec++;
        ctx->probe_input_apply++;

    These occur on paths that already exist for simulation work.

## 6. Forbidden Probe Pattern
    The following patterns are not allowed when introduced only for
    measurement:
        for (...) { ... }
        if (...) { metric++; }
        metric += popcount(...);

    Any control flow introduced solely for benchmarking or testing is
    forbidden.

## 7. Probe Exposure
    Probes must be returned through a probe snapshot structure.
    Example:
        hebs_probes probes = hebs_get_probes(engine);

    Probe APIs must return raw counters only.
    Probe APIs must not return derived metrics.

## 8. Probe Classification
| Probe               | Lifecycle | Domain | Build Gate |
|---------------------|-----------|--------|------------|
| `input_apply`       | Permanent | Tool   | Always on  |
| `chunk_exec`        | Permanent | Bench  | Always on  |
| `gate_eval`         | Permanent | Bench  | Always on  |
| `dff_exec`          | Permanent | Bench  | Always on  |
| `tick_count`        | Permanent | Bench  | Always on  |
| `state_commit_count` | Permanent | Bench | Always on  |
| `input_toggle`      | Test      | Test   | `HEBS_TEST_PROBES` |
| `state_change_commit` | Test    | Test   | `HEBS_TEST_PROBES` |
| `contention_count`  | Test      | Test   | `HEBS_TEST_PROBES` |
| `unknown_state_materialize_count` | Test | Test | `HEBS_TEST_PROBES` |
| `highz_materialize_count` | Test | Test | `HEBS_TEST_PROBES` |
| `multi_driver_resolve_count` | Test | Test | `HEBS_TEST_PROBES` |
| `tri_no_drive_count` | Test     | Test   | `HEBS_TEST_PROBES` |
| `pup_z_source_count` | Test     | Test   | `HEBS_TEST_PROBES` |
| `pdn_z_source_count` | Test     | Test   | `HEBS_TEST_PROBES` |

Removed probes:
-   `state_write_attempt`

Deferred probes:
-   `tray_exec`
-   `state_write_commit`

## 8b. Domain Separation
    Bench domain probes exist for benchmark runners and throughput analysis.
    Tool domain probes exist for operational visibility and integration tooling.
    Bench and Tool domains share the Permanent probe field set.
    Test domain probes exist only for correctness/validation checks.
    Test domain probes must never be required for runtime simulation behavior.
    Test fields are validation-only and excluded from standard release benchmark CSV payloads.
    Test domain probes must be build-gated and excluded from PERF artifacts.

## 9. Probe Build Profiles
    Exactly one probe profile must be active at build time.
    Default release profile: PERF.
    Test-Probe profile: TEST.
    Test-Probe fields are controlled by `HEBS_TEST_PROBES`.
    PERF release expectation: `HEBS_TEST_PROBES=0`.

## 10. Benchmark Interaction
    Benchmark runners may read probe values.
    Benchmark runners may derive metrics from probe data.
    Benchmark runners must not modify probe values.

## 11. Compliance Test
    If code exists only to measure engine behavior and does not contribute
    to simulation execution, it must not exist inside the engine.
    Probe code must always satisfy:
    -   O(1) update
    -   existing execution path
    -   raw fact emission

## 12. Release Gate
    Release/PERF builds must not expose Test-domain probe surfaces.
    Required release checks:
    -   `HEBS_TEST_PROBES=0`
    -   No Test-domain fields in the public probe snapshot for PERF
    -   No Test-domain probe increments compiled into PERF codepaths
