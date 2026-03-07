# Testing & Benchmarking Protocol

## 1. Test Suite Purpose
The **test suite** verifies engine correctness during development.
Tests must detect regressions introduced by code edits.
Tests must validate core engine behavior only.
Tests must not measure engine performance.

## 2. Benchmark Suite Purpose
The **benchmark suite** measures engine performance and behavior.
Benchmarks must observe engine execution.
Benchmarks must not modify engine behavior.
Benchmarks must not enforce engine correctness.

## 3. Test / Benchmark Independence Rule
The test suite and benchmark suite must operate independently.
Testing must not depend on benchmarking.
Benchmarking must not depend on testing.
Failure or removal of one system must not affect the operation of the
other.

## 4. Engine Independence Rule
Testing and benchmarking must exist **outside the engine
implementation**.
Tests and benchmarks may **read engine state**.
Tests and benchmarks must **not create or modify engine state**.
Tests and benchmarks must not introduce logic inside the engine.

## 5. Engine Probe Rule
The engine may expose **raw probes only**.
Probes must be updated only at **existing execution points**.
Probe updates must be **constant cost O(1)**.
The engine must not compute or derive benchmark metrics.
The engine must not scan, sweep, or reconstruct state for measurement.
Metric computation must occur outside the engine.

## 6. Benchmark Runner Requirements
The benchmark runner must be **general purpose**.
Benchmark suites must be discovered by directory scanning.
No benchmark suite may be hardcoded.
The runner must support execution of: - a single benchmark - all
benchmarks within a suite - all benchmarks across suites

## 7. Benchmark Execution Parameters
The runner must support parameterized execution.
Supported modes:
Cold Start\Continuous
Configurable parameters:
Iterations per mode\Cycles per mode

## 8. Benchmark Output Rules
All available metrics must be recorded.
Unavailable metrics must output:

`0` or `N/A`

Each benchmark report must include:
    Revision identifier\
    System date\
    System time

Results must be appended to the benchmark CSV history.
Existing records must not be modified.

## 9. Test Suite Rules
Tests must be simple and deterministic.
Tests must verify specific engine behaviors.
Tests must not modify engine internals.
When tests fail:
The failure must be reported.
Possible causes may be suggested.
Engine fixes must not be applied automatically.
All fixes require explicit approval.

## 10. Test Creation Rule
When new engine functionality is introduced:
At least one test must be created to verify that functionality.
