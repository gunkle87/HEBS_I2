HEBS(Hardware Evaluated Batch Simulator)

THE LOADER:

The Loader will handle four critical functions that the Engine is too "lean" to care about and the Runner is too "high-level" to manage:

Topological Levelization: It analyzes the gate connections to determine the "Waterfall" order. It ensures that no gate is evaluated before its inputs are ready, which is the foundation of our determinism.

The Batch Packer: It takes individual gates (like a single AND gate) and packs them into 64-bit SIMD batches. It looks for "parallelism" in the netlist so the Engine can "ride the physics" of the CPU.

Static Distribution Mapping: It calculates the Fanout Map. It pre-determines exactly which bit-offset in Level 2 needs to receive the result from Level 1, so the Engine never has to "search" for a wire during the simulation.

Plan Hashing (lep_hash): It generates a unique signature of the compiled circuit. This allows the Runner to verify that the "Instructions" are identical across different versions of the code, catching silent compiler bugs.


THE ENGINE:

The Engine performs four specific, high-speed roles to "Ride the Physics" of the CPU hardware:

The Bit-Tray Manager: It maintains the Primary Input and Primary Output buffers. It manages the "Waterfall" transition, ensuring that as one level of logic completes, the results are ready to be the inputs for the next level.

The Batch Executor (The Hot Loop): This is the heart of HEBS. It iterates through the LEP (Linear Execution Plan) and performs 64-bit SIMD operations (AND, OR, XOR). Because the Loader already did the "thinking," the Engine only has to execute raw bitwise math without any branching or decision-making.

The Scatter-Store Specialist: After a 64-bit batch is calculated, the Engine handles the Fanout. It uses the pre-baked Distribution Map to "shatter" that 64-bit result and store each bit into its precise destination address in the next level's tray.

The Tick Synchronizer (The Master Clock): It manages the boundary between simulation cycles. Once all levels are processed, it performs the Sequential Flip, moving values from the Shadow Buffer (D-Flip-Flops) back to the Level 0 starting line for the next tick.


THE RUNNER:

The Core Roles of the Runner
The Runner performs four high-level, orchestrational roles to turn raw logic into actionable data:

The Campaign Manager (CLI & Config): It parses command-line arguments to decide which .bench files to load and how many ticks to run. It is responsible for setting up the "Environment"—pinning the process to a specific CPU core (Affinity) and setting the thread count for the Engine.

The Test Tank Orchestrator: It manages the "repeated run" protocol to filter out OS noise. It triggers the Engine multiple times to find the Median, Low, and High performance metrics, ensuring the GEPS and ICF data are statistically sound rather than "lucky" single-shot numbers.

The Truth Judge (Regression Lab): It performs the "Differential Comparison". It takes the final state_hash or bit-tray from the HEBS Engine and compares it against the HighZ baseline or "Truth" files to calculate the Regression Delta. If even a single bit is wrong, the Runner sounds the alarm.

The Artifact Registrar: It owns the Artifact Zone (/artifacts). It gathers the metadata from the Loader (LEP hash), the performance data from the Engine (Cycle counts), and the system info (Git hash, Compiler version) to produce the schema-valid JSON manifests and CSV reports.

Why this "Judge" stays separate
By keeping the Runner outside the src/ library, we ensure the Engine remains "Pure". The Runner can be swapped out for a Python script, a GUI, or a CI/CD bot without changing a single line of the simulation logic. It allows the simulation to run at maximum speed while the Runner handles the "heavy lifting" of file I/O and data formatting on the side.

The "Test Tank" Requirements (TAB_PROTOCOL.md)
Cold Start Policy: Each benchmark iteration must start and stop independently to prevent "cache stacking" from skewing results.

Thermal Settle: A 5-second "Warm-up" dummy load is mandatory before measuring to settle CPU frequency and thermal states.

The Median Rule: All final GEPS (Gates Per Second) and latency metrics must be reported as the Median (p50) of 10 iterations, not the average, to filter out OS-induced jitter.

8-State Integrity: We must implement a PopCount check for all 8 states to prove that no state (like Weak-0 or High-Z) is being accidentally "collapsed" into a simpler state during optimization.

The Determinism Gate
Under Section 5 of your protocol, we now have a hard failure condition:

Logic Fingerprint: A CRC32 of the final signal tray after the last tick.

Plan Fingerprint: A hash of the Linear Execution Plan (LEP).

Failure Trigger: If any code change alters these fingerprints on a stable benchmark, or if the median performance regresses by >2%, the revision is a hard failure.


CONCURRENCY:

1. Concurrency in the Loader (Parallel Compilation)The Loader is often the bottleneck for massive netlists. Since levelization and batch packing are mathematical transformations of a static graph, we can parallelize them:Independent Level Analysis: While Level 1 is being packed into 64-bit batches, a second thread can begin identifying the gates for Level 2.Partitioned Packing: If a level has 64,000 gates, we can spawn 10 threads to pack 6,400 gates each into their respective LEP structures simultaneously.2. Concurrency in the Engine (SIMD + Multi-Core)This is where the "Logic Press" truly scales. Because the Engine uses a Static Execution Plan, there are no "Race Conditions" inside a single level:Data Sharding: If Level 5 has 1,000 batches, we can give Batch 0–500 to Core 1 and Batch 501–1000 to Core 2.Zero Locking: Since all threads read from the Primary Input Buffer and write to unique offsets in the Primary Output Buffer, they never compete for the same memory.The Barrier: The only "Sync Point" is at the end of a Level. All threads must finish Level $N$ before any thread starts Level $N+1$.3. Concurrency in the Runner (The Multi-Tank)The Runner can achieve a different kind of parallelism called Task Parallelism:Multi-Baseline Shootout: The Runner can spawn 5 separate Engine instances (Baselines 1–5) on 5 different CPU cores.Batch Regression: If you need to test 100 different .bench files, the Runner can process them all at once, each with its own Loader and Engine instance, filling the artifacts/ folder in a fraction of the time.The "Golden Rule" of HEBS ConcurrencyEven with 128 threads running, the result must be Bit-Exact to the single-threaded version. Because our memory offsets are hard-coded by the Loader, the order of thread execution doesn't matter—the bits will always land in the same spot.


DIRECTORY STRUCTURE:

/
├── core/                   # The High-Performance Library
│   ├── engine.c            # The Logic Press (SIMD, Trays, Ticks)
│   ├── loader.c            # The Architect (Parsing, Levelizing, LEP)
│   ├── primitives.c        # The Bitwise Logic (AND/OR/XOR/DFF)
│   └── includes/           # The API Contracts
│       ├── hebs_engine.h   # Public API for Engine & Loader
│       └── primitives.h    # Bitwise function definitions
│
├── benchmarks/             # The Performance Lab
│   ├── runner.c            # Orchestrator (Multi-run, Medians, HighZ compare)
│   ├── benches/            # Input files (.bench netlists)
│   └── results/            # Artifact Zone (CSV, MD, HTML reports)
│
├── tests/                  # The Stress-Test Suite
│   ├── test_runner.c       # Unit tests for Engine/Loader isolation
│   └── results/            # Coverage and Stress-test logs
│
├── build/                  # Artifacts of the compilation process
├── .git/                   # Version history
├── Makefile                # The Build Master
└── README.md               # Unified Governance & Roadmap



HEBS_PLAN.MD - REVISION_STRUCTURE_V01


0. THE GROUND TRUTH: STATIC-PLANNED ARCHITECTURE

This is the definitive "Ground Truth" for the HEBS_CLEAN reboot. We are shifting 
from a reactive development style to a Static-Planned Architecture where the 
documentation is the map, and the code is simply the terrain.

1. LOGIC SEMANTICS: THE 8-STATE STANDARD
To achieve top-notch semantics and speed, we are locking in 8-State Logic. 
This aligns perfectly with Byte-Boundary Memory (2 bits per state), making it 
more efficient for the CPU to mask and shift in 64-bit batches.

STATE   BIT_1   BIT_0   SEMANTIC   DESCRIPTION
-----   -----   -----   --------   ---------------------------------
0       0       0       S0         Strong 0 (GND)
1       0       1       S1         Strong 1 (VCC)
2       1       0       W0         Weak 0 (Pull-down)
3       1       1       W1         Weak 1 (Pull-up)
4       0       0       Z          High Impedance (Floating)
5       1       1       X          Unknown / Contention
6       0       1       SX         Strong Contention (Error)
7       1       0       WX         Weak Contention (Warning)

DETERMINISTIC RULE: 
Using 8 states allows us to use a simple 3-bit or 4-bit Lookup Table (LUT) 
per gate, which is faster than complex if-else logic for 7 states.

2. VERSIONING & GOVERNANCE LEXICON
To avoid "Agent Runaway" and maintain human readability, we will use the 
following hierarchy for every change:

REVISION (Major Phase): 
    A significant architectural shift (e.g., Revision_Alpha_Core).
MINOR REVISION (Iteration): 
    Tuning or bug fixes within a phase (e.g., Alpha_Core_v01).
IDENTIFIER: 
    Every report/file is stamped: [Revision]_[Iteration]_[Timestamp]_[GitHash].


3. THE HEBS CODE MAP (THE LIVING DIRECTORY)
This document (stored in docs/CODE_MAP.md) serves as the index for both the 
user and the AI agent.

CORE PILLAR (The Library):
    - core/engine.c: The Logic Press.
        - hebs_tick_execute(): High-speed loop processing 64-bit batches.
        - bit_tray_in/out: Flat uint64_t arrays representing logic levels.
    - core/loader.c: The Circuit Architect.
        - hebs_compile_lep(): Translates .bench files into the LEP.


4. GOVERNANCE ADDENDUM (MANDATORY)

REPO MAP MAINTENANCE:
    - HEBS_REPO_MAP.md must be updated whenever functions, variables, types, constants,
      files, or directories are added, removed, renamed, or behaviorally changed.

REVISION-BASED ARTIFACT NAMING:
    - Generated report artifact names must use the active Revision token.
    - HTML output for a major Revision is overwritten per minor revision.
    - The last minor revision HTML is the final HTML for that Revision and must never
      be overwritten by subsequent Revisions.

DIRECTIVE VS DOCUMENT CONFLICT RULE:
    - If a directive conflicts with documented requirements, each conflict must be named
      and explicitly approved before work continues.
    - Required language per conflict:
      "This <DeviationName> is not the same as required in documentation. Reply APPROVED
      <DeviationName> to explicitly confirm and continue."

ICF CANONICAL DEFINITION:
    - ICF is defined as:
      internal node transitions / primary input transitions
    - Historical ICF values produced with any prior formula are not directly comparable
      to the canonical formula and must be labeled as pre-canonical in reports.
