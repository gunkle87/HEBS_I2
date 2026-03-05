# HEBS Phase 0.1 Rebuild - Milestones & Planning

This document tracks our progress, architectural decisions, and milestones as we plan the clean-slate rebuild of the HEBS engine.

## Current Status
- [x] Consolidate foundational governance and technical specifications into `The engine.md`.
- [x] Analyze the legacy engine implementation to identify strengths (performance drivers) and weaknesses (drift/technical debt).
- [x] Define the definitive Phase 0.1.0 architectural plan and memory layouts.
- [x] Establish the Phase 0.1.0 test and benchmarking protocol.
- [ ] Lock in the core file structure and build system.

## Architectural Notes & Decisions

### Legacy Analysis
*   **Strengths (The "Good" Core):** The batched logic press found inside the legacy `hebs_bench_runner.c` is the key to the high performance. It levelizes the topology into `level_plan` chunks, executing wide bitwise operations (e.g., `eval_segment_and`) directly on `uint64_t` bit-trays. It uses a highly optimized scatter/gather mechanism (`scatter_bucket` / `scatter_fanout2`) to map outputs to inputs for the next level.
*   **Weaknesses (The "Drift"):**
    *   The core engine logic (netlist compilation, scatter/gather execution loops, and state buffers) was improperly hardcoded directly into the benchmark runner instead of being a modular library.
    *   Legacy `hebs_primitives.c` drifted into using slow, loop-heavy byte-level evaluations that conflicted with the 64-bit batch mandate.
    *   Complex sequentials were artificially "lowered" into massive webs of combinational logic, bloating the evaluation time.
*   **SRAM "Black Box" Architecture (Future-Proofing):**
    *   **Storage:** A structured Data Blob (`uint8_t` or `uint64_t` array) separate from the standard signal nets.
    *   **Execution (End-of-Level Hook):** Triggered cleanly at the boundary of the specific logic level containing the SRAM interface signals. This completely eliminates `if(is_sram)` branching in the fast combinational `eval_segment_*` loops.
    *   **Determinism:** Strictly adheres to "Read-Before-Write" via shadowed buffers. Writes are committed only at the Tick Boundary to prevent RAW (Read-After-Write) ambiguity, mirroring exactly how DFFs are staged.

### Phase 0.1.0 Core Plan: The Concrete Hardware Model
We are transitioning from abstract logic simulation to a memory-mapped execution model:
1.  **Linear Execution Plan (LEP):** 
    *   Dynamic event-driven scheduling is replaced entirely by a static, pre-calculated sequence of bitwise operations.
    *   The netlist is **levelized** during compilation so that every gate in a 64-bit batch is ready for evaluation simultaneously.
2.  **Typed LEP Segments (Hot Path Purity):**
    *   As outlined in Phase 0.30 of the Roadmap, the LEP is grouped by gate type (e.g., all ANDs together, all XORs together).
    *   This eliminates `if(gate_type)` or `switch` statements inside the hot loop, maximizing SIMD utilization and branch prediction.
3.  **Two-Phase Memory Model (Write-Once Commit):**
    *   **Phase A (Sweep):** Compute operations write strictly into `next` buffers/output trays.
    *   **Phase B (Tick Boundary):** A bulk synchronous commit (pointer swap or contiguous `memcpy`) applies `next` to `present`.
    *   This eliminates intra-tick read-modify-write (RMW) ambiguity and inherently solves the DFF and SRAM timing constraints.

### Test & Benchmarking Protocol (TAB_PROTOCOL)
*   **Isolation:** The tooling must sample the engine without polluting its internals.
*   **Compilation:** Strict `perf` profile (`gcc -O3 -march=native -DNDEBUG -static -fno-plt`).
*   **Execution:** 10-iteration cold starts, median metric extraction.
*   **Correctness:** 8-state integrity checks, logic hash fingerprinting (CRC32), and plan hashing. Hard failure on state collapse or >2% GEPS regression.
