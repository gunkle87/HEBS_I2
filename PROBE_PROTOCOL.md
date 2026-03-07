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
    Permanent probes (always enabled):
    -   input_apply
    -   chunk_exec
    -   gate_eval
    -   dff_exec

    Compatibility probes (optional build profile):
    -   input_toggle
    -   state_change_commit

    Removed probes:
    -   state_write_attempt

    Deferred probes:
    -   tray_exec
    -   state_write_commit

## 9. Probe Build Profiles
    Exactly one probe profile must be active at build time.
    PERF profile Compatibility probes disabled.
    COMPAT profile Compatibility probes enabled.
    Default build profile: PERF.

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
