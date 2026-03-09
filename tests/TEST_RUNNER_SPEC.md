# Test Runner Technical Specification

## 1. Scope
This document specifies the test runner tooling in `tests/`.
It does not define engine architecture or benchmark policy.
Tests validate behavior through public APIs and public helper interfaces.

Primary implementation source:
- `tests/test_runner.c`

## 2. Entry Point
Test entry point is `main` in `tests/test_runner.c`.
Execution model is assert-driven, single-process, deterministic checks.

## 3. Dependencies
The test runner currently depends on:
- `include/hebs_engine.h`
- `include/primitives.h`
- `benchmarks/protocol_helper.h` for math helper validation

## 4. Execution Contract
Each test function executes in-process and uses `assert`.
A failing assert terminates process execution immediately.
Exit behavior:
- success: process returns `0`
- failure: process aborts due to assert failure

## 5. Current Test Catalog
`test_loader_contract_from_s27`
- validates loader and plan structural expectations using `s27.bench`
- validates basic probe behavior under PERF versus TEST profile

`test_pending_state_accumulator_closure_proof`
- validates the proven closure mapping between mailbox resolution semantics and the resolved-state accumulator domain

`test_live_pending_accumulator_semantics`
- validates direct pending-state accumulation on the live engine path for duplicate strong drive, strong-vs-weak, weak-vs-weak, TRI no-drive, and pull behavior

`test_live_pending_batched_fallback_equivalence`
- validates accumulator-path equivalence between batched spans and fallback evaluation

`test_live_pending_dff_deferred_visibility`
- validates that DFF outputs remain next-tick visible under pending-state accumulation

`test_functional_gate_suite`
- validates logical correctness of AND/OR flows over packed state variants
- validates primary input write and signal read behavior

`test_interleaved_bit_offset_mapping`
- validates signal-to-tray mapping for non-zero tray index and bit offset

`test_icf_math_assertion`
- validates probe-derived transition expectations for deterministic PI pattern
- validates `calculate_icf` numeric behavior

`test_parallel_dff_tray_commit`
- validates DFF commit behavior and state tray propagation path

`test_loaded_dff_captures_declared_data_only`
- validates loaded single-input DFF capture against the live engine contract

`test_loaded_dff_unknown_capture_ignores_unrelated_state`
- validates DFF unknown capture behavior stays isolated from unrelated tray state

`test_loader_batched_specialized_span_execution`
- validates loader-built batched execution across the specialized span-kernel path

`test_protocol_helper_stats`
- validates min/max/p50/percentile/stddev helper results

`test_extended_primitive_suite`
- validates scalar primitive helpers

`test_init_rejects_oversized_signal_count`
- validates engine initialization rejects plans that exceed fixed signal capacity

`test_fallback_invalid_src_b_is_safely_skipped`
- validates malformed fallback source bounds are skipped without execution

`test_batched_invalid_src_b_is_safely_skipped`
- validates malformed batched span source bounds are skipped without output writes

`test_dff_invalid_destination_is_safely_skipped`
- validates malformed DFF destination records are skipped without state corruption

## 6. Profile-Aware Assertions
Tests include explicit profile-conditional assertions with `HEBS_TEST_PROBES`.
Test-domain probe expectations are asserted only in TEST profile.
PERF profile paths assert that test-domain counters stay at zero.

## 7. Determinism Expectations
Test vectors and control flow are deterministic.
No test uses random input generation.
Input patterns are explicitly constructed in code.

## 8. Boundaries
Tests are external to engine core implementation.
Tests may initialize engine, apply inputs, tick engine, and read outputs/probes.
Tests must not patch or mutate core internals outside public API contracts.
Tests should read engine state through public API and public helper interfaces rather than direct core-field peeks.

## 9. Extension Guidelines
When adding engine-facing functionality:
1. Add at least one test function that checks that functionality.
2. Keep setup explicit and deterministic.
3. Prefer API-level assertions over internal field mutation.
4. If profile-specific behavior exists, add PERF and TEST expectations.
5. Keep each test focused on one behavior family.

## 10. Non-Goals
This specification does not define benchmark metrics or reporting.
That belongs to benchmark tooling specifications under `benchmarks/`.
