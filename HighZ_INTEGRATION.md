# HighZ Integration Plan (Hybrid HEBS Engine)

Status: Draft for review  
Date: 2026-03-06  
Owner: HEBS core

## 1. Purpose

This document describes how to implement one HEBS loader/plan model with two runtimes:

- Zero-delay waterfall mode (current HEBS behavior)
- Timed event runtime (transport delay, event-driven)

The loader and primitive semantics remain unified so both modes are deterministic and comparable.

## 2. Scope and Intent

### In Scope

- One compiled plan format used by both runtimes
- Timing-aware metadata in the plan
- Event queue runtime for transport delay
- API mode switch and converged state hash behavior
- Fanout adjacency for O(1) event injection by net
- Inertial rejection scaffolding (disabled in v1)

### Out of Scope (v1)

- Inertial rejection behavior enabled
- Multithreaded timed simulation
- Replacing existing zero-delay optimized hot path

## 3. Critical Constraints

1. HEBS core remains procedural C and data-oriented.
2. No direct code reuse from HighZ. HighZ is design reference only.
3. c6288 CRC lock must remain stable in zero-delay mode.
4. Timed mode must be deterministic with strict event ordering.
5. If all delays are zero, timed mode must match zero-delay mode at cycle boundaries.

## 4. Current Baseline (HEBS)

Current HEBS already has:

- Compiled gate plan and span execution in `core/loader.c` and `core/engine.c`
- 8-state primitive API surface in `core/primitives.c`
- DFF commit manager in `core/state_manager.c`
- Stable benchmark/report pipeline

Gap to close:

- No timing fields in compiled plan
- No event queue runtime
- No fanout adjacency in plan for event injection

## 5. Target Hybrid Architecture

## 5.1 Single Loader, Two Runtimes

The loader emits one compiled plan containing:

- Gate metadata (`type`, inputs, output)
- Optional delay metadata per gate
- Stable topological sequence index (`topo_seq`)
- Net fanout adjacency (CSR arrays)

Then runtime dispatch is selected by mode:

- `HEBS_MODE_ZERO_DELAY`: current waterfall path
- `HEBS_MODE_TIMED_EVENT`: new event queue path

## 5.2 Shared Semantics Layer

Both runtimes must call the same primitive logic semantics for resolved state decisions.  
This avoids divergence and keeps hash parity feasible when timing collapses to zero.

## 6. API and Type Additions

## 6.1 Mode Enum

Add to `include/hebs_engine.h`:

```c
typedef enum hebs_mode_e
{
	HEBS_MODE_ZERO_DELAY = 0,
	HEBS_MODE_TIMED_EVENT = 1

} hebs_mode_t;
```

## 6.2 Public API

Add:

- `hebs_status_t hebs_set_mode(hebs_engine* ctx, hebs_mode_t mode);`
- `hebs_mode_t hebs_get_mode(const hebs_engine* ctx);`
- `hebs_status_t hebs_set_default_gate_delay_ps(hebs_plan* plan, uint32_t delay_ps);`
- `hebs_status_t hebs_set_gate_delay_ps(hebs_plan* plan, uint32_t gate_idx, uint32_t delay_ps);`
- `hebs_status_t hebs_run_until_ps(hebs_engine* ctx, hebs_plan* plan, uint64_t target_time_ps);` (timed mode helper)

`hebs_tick()` stays public and valid in both modes:

- zero-delay: one combinational+sequential step
- timed-event: process all events at current `time_ps`, including same-time spawned events across delta cycles, until quiescent at that `time_ps`

## 6.3 Plan Extensions

Add to `hebs_plan`:

- `uint32_t* gate_delay_ps;` (size `gate_count`)
- `uint32_t* gate_topo_seq;` (size `gate_count`)
- `uint32_t* net_fanout_offsets;` (size `signal_count + 1`)
- `uint32_t* net_fanout_gates;` (CSR payload)
- `uint32_t fanout_edge_count;`

## 6.4 Engine Extensions

Add to `hebs_engine`:

- `hebs_mode_t mode;`
- `uint64_t sim_time_ps;`
- event queue state
- per-gate driver cache / per-net drive aggregation buffers

## 7. Timing Source Strategy (v1)

Because `.bench` files are delayless, use a deterministic layered policy:

1. Default per-gate delay table by gate type (compile-time constants)
2. Optional override API (`hebs_set_gate_delay_ps`)
3. Optional sidecar file in future (not required for v1)

This keeps v1 usable without netlist format changes.

## 8. Timed Event Runtime Design (Transport v1)

## 8.1 Event Record

```c
typedef struct hebs_event_s
{
	uint64_t time_ps;
	uint32_t delta_cycle;
	uint32_t topo_seq;
	uint32_t gate_idx;
	uint8_t reason; /* input-net changed, initial seed, etc */

} hebs_event_t;
```

Queue key order (strict):

1. `time_ps` (ascending)
2. `delta_cycle` (ascending)
3. `topo_seq` (ascending)
4. `gate_idx` (ascending, final tie-break)

This is the determinism contract.

## 8.2 Queue Choice

Use binary min-heap first.  
Reason: simple, deterministic, low implementation risk.

Future option: timing wheel if event density justifies.

## 8.3 Runtime Flow

1. Seed events for gates driven by changed primary inputs.
2. Pop next event.
3. Evaluate gate using current settled net states.
4. Compute output state.
5. If output net state changed:
   - update net state
   - enqueue fanout gates at `time_ps + gate_delay_ps[fanout_gate]`
6. Continue until queue empty or target time reached.

De-dup rules:

- Dedup is allowed only when all event identity fields match and event is still pending:
  - same `gate_idx`
  - same `time_ps`
  - same `delta_cycle`
  - same `topo_seq`
  - same `generation` token
- Any key mismatch must enqueue as a distinct legal event.

## 8.4 Transport Delay Semantics

No pulse rejection in v1:

- every valid output transition produces a scheduled fanout event
- no cancellation of pending events

## 9. Inertial Scaffolding (Disabled in v1)

Add structures now:

- `pending_event_id` per gate output
- `event_generation` token for lazy cancel
- optional minimum pulse width per gate

Keep compile/runtime flag off in v1.  
This avoids redesign later.

## 10. Loader Work Plan

In `core/loader.c`:

1. Emit `gate_topo_seq` while finalizing LEP order.
2. Build CSR fanout arrays from gate input usage:
   - count fanouts per net
   - prefix sum offsets
   - fill gate indices
3. Allocate and initialize `gate_delay_ps` with defaults.
4. Keep existing span plan unchanged for zero-delay.

Validation:

- CSR bounds checks
- deterministic stable ordering in `net_fanout_gates`

## 11. Engine Work Plan

In `core/engine.c`:

1. Add mode dispatch inside `hebs_tick()`.
2. Keep current zero-delay path untouched by default.
3. Add timed path entry:
   - seed PI-triggered events
   - process heap
   - update counters/metrics
4. Ensure `hebs_get_state_hash()` reads the same canonical trays/state arrays in both modes.

Recommendation:

- Put timed queue implementation in a new file (`core/timed_runtime.c`) to keep engine.c readable.

## 12. Primitive and State Manager Plan

## 12.1 Primitives

No semantic split by mode.  
Timed mode must use the same primitive evaluation functions.

## 12.2 Sequential

For v1, lock this ordering contract:

1. Resolve all events at current `time_ps` to quiescence.
2. Detect clock transition edges from resolved net states.
3. Sample D on edge using the same resolved-state snapshot and tie-break order.
4. Apply Q updates at the same `time_ps` with deterministic ordering (`time_ps`, `delta_cycle`, `topo_seq`, `gate_idx`).
5. Only then advance to future `time_ps`.

If needed, add a dedicated timed sequential helper in `core/state_manager.c` later.

## 13. Determinism and Hash Contract

Required:

1. Same inputs + same delays + same seed order => same hash and metrics.
2. Tie-break key order is fixed and documented.
3. Queue operations must avoid nondeterministic iteration order.
4. If all delays are set to zero, timed mode must match zero-delay at defined cycle checkpoints.

## 13.1 Seam Contract (Mandatory)

These definitions are hard requirements before T2:

1. Zero-delay boundary
- One `hebs_tick()` in zero-delay mode means:
  - full combinational quiescence
  - then sequential commit

2. Timed quiescence boundary
- At a given `time_ps`, timed mode must:
  - process all events scheduled for `time_ps`
  - continue processing any new same-time events (`delta_cycle` growth)
  - stop only when no events remain at that same `time_ps`

3. All-delays-zero parity
- If every gate delay is `0`, timed mode must match zero-delay mode at:
  - post-combinational-quiescence / pre-sequential-commit checkpoint
  - post-sequential-commit checkpoint

4. Deterministic tie-break order
- Event processing key order is fixed:
  - `time_ps`
  - `delta_cycle`
  - `topo_seq`
  - `gate_idx`

## 14. Metrics Additions

Add timed-only fields (engine counters + runner output):

- `events_enqueued`
- `events_processed`
- `max_queue_depth`
- `avg_delta_cycles_per_vector` (timed)
- `avg_event_fanout`
- `timed_mode_time_ps`

Keep existing GEPS/ICF outputs intact.

## 15. Validation Matrix

## 15.1 Functional

1. Primitive parity tests (existing).
2. Timed smoke tests on tiny hand-built circuits.
3. Zero-delay parity regression on current 10-bench suite.
4. Zero-delay vs timed (all delays=0) hash parity tests.

## 15.2 Determinism

Run same timed test N times; assert identical:

- final hash
- event count
- queue max depth

## 15.3 Performance

Track separately:

- zero-delay GEPS (must not regress materially)
- timed mode events/sec and ns/event

## 16. Phased Implementation Sequence

### Phase T1: Data Plumbing

- Add mode enum/APIs
- Add delay + fanout CSR fields
- Build/fill loader data
- No timed execution yet

Exit gate: build + tests green.

### Phase T2: Minimal Timed Engine

- Add heap queue and event struct
- Implement transport event loop for combinational gates
- PI event seeding

Exit gate: tiny-circuit timed tests pass.

### Phase T3: Sequential Timed Integration

- Integrate DFF edge behaviors into timed loop
- Validate s27/s298 hashes for deterministic timed runs

Exit gate: deterministic repeated runs pass.

### Phase T4: Parity and Bench Harness

- Add zero-delay/timed comparison tests
- Add zero-delay parity with delays=0
- Add timed telemetry fields in runner

Exit gate: 10-bench zero-delay baseline lock remains valid.

### Phase T5: Inertial Scaffold Ready

- Add pending-event token/cancel metadata
- Keep feature off

Exit gate: no behavior changes in transport mode.

## 17. Known Risks and Mitigations

1. Risk: queue churn on dense circuits  
Mitigation: early dedup stamps + bounded re-enqueue policy.

2. Risk: divergence between runtimes  
Mitigation: shared primitive semantics + parity tests.

3. Risk: zero-delay regression  
Mitigation: mode dispatch with zero-delay hot path unchanged.

4. Risk: ambiguous cycle boundary in timed mode  
Mitigation: explicit documented boundary rule in API and runner.

## 18. Open Decisions for Final Plan

1. Delay defaults by gate type (exact ps values)
2. Where timed stats are surfaced in runner reports
3. Whether timed mode gets dedicated benchmark artifacts or combined report sections

## 19. Recommended Immediate Next Step

Start Phase T1 only (data plumbing).  
Do not implement queue logic until T1 tests are green and reviewed.
