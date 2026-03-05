# HEBS Repository Agent Policy

Version: HBS-0.1.2
Scope: Entire repository

## 1. Command Authority

1. Default mode is constrained execution.
2. Read-only actions are implicitly allowed when directly requested.
3. State-changing actions require explicit approval token `APPROVED`.
4. `APPROVED` is scoped to the most recent proposal only.
5. Scope overrides use explicit directives in the form `DO: <scope>`.
6. If no proposal is pending, `APPROVED` is invalid.

## 2. Project Canon (Non-Negotiable)

1. HEBS must target correctness, determinism, repeatability, low latency, and high throughput together.
2. HEBS is procedural C and Data-Oriented Design (DOD) by default.
3. Object-oriented design patterns are prohibited in core engine code.
4. Loops are allowed only when they are the best measured option.
5. Testing and benchmarking tools must sample public engine interfaces and must not mutate core internals directly.

Core implementation files:
- `include/`
- `src/`

Governance and canonical files:
- `HEBS_SNAPSHOT.md`
- `HEBS_AGENTS.md`
- `HEBS_TECH_SPEC.md`
- `HEBS_ROADMAP.md`
- `HEBS_HISTORY.md`
- `HEBS_REPO_MAP.md`

## 3. Authorization Tiers

1. Standard edit (non-core): `APPROVED`.
2. Core implementation edit (non-canon): `DO: CORE_EDIT` + `APPROVED`.
3. Canon or governance change: `DO: CANON_CHANGE` + `APPROVED`.

## 4. Mandatory Canon Impact Notice

Before any state-changing proposal, include this notice exactly:

```text
TENET_IMPACT: NONE | POTENTIAL | REQUIRES_CANON_CHANGE
TENETS_AFFECTED: <list or NONE>
AUTH_PATH: APPROVED | DO: CORE_EDIT | DO: CANON_CHANGE
```

Interpretation:
1. `NONE`: no core file or canon impact identified.
2. `POTENTIAL`: touches core implementation files; canon impact not expected but possible.
3. `REQUIRES_CANON_CHANGE`: touches canonical governance or technical specification files.

Authorization mapping:
1. `TENET_IMPACT=NONE` -> `APPROVED`.
2. `TENET_IMPACT=POTENTIAL` -> `DO: CORE_EDIT` + `APPROVED`.
3. `TENET_IMPACT=REQUIRES_CANON_CHANGE` -> `DO: CANON_CHANGE` + `APPROVED`.

## 5. Document Roles

1. `HEBS_SNAPSHOT.md`
   - Role: canonical blueprint and project authority root.
   - Content: governance, architecture direction, phase model, logging and benchmarking canon.
   - Edit policy: Restricted to `DO: CANON_CHANGE`.
2. `HEBS_AGENTS.md`
   - Role: agent policy and execution controls.
   - Edit policy: Restricted to `DO: CANON_CHANGE`.
3. `HEBS_TECH_SPEC.md`
   - Role: technical contracts (API, data model, enums, constants, formulas).
   - Edit policy: Restricted to `DO: CANON_CHANGE`.
4. `HEBS_ROADMAP.md`
   - Role: delivery phases and acceptance gates.
5. `HEBS_HISTORY.md`
   - Role: phase chronology and outcomes.
6. `HEBS_REPO_MAP.md`
   - Role: repository-wide technical reference (directory map, API registry, symbol inventory, examples, and ledgers).
   - Edit policy: Restricted to `DO: CANON_CHANGE`.

## 6) Response Style
1. Use plain English and conversational tone.
2. Keep sentences short and easy to follow.
3. Avoid jargon unless necessary; define it briefly when used.
4. Prefer concrete examples over abstract statements.
5. Keep answers brief by default unless detail is requested.
6. Ask one quick clarifying question when requirements are unclear.

## 7. Audit Modes

1. `FULL_AUDIT_CORE`
   - Read-only.
   - Scope: `include/`, `src/`, canonical specification files, and `canon/` when present.
   - Excludes by default: generated outputs and benchmark artifacts.
   - Deliverable: invariant compliance report mapped to file paths.
2. `FULL_AUDIT_REPO`
   - Read-only.
   - Scope: full repository inventory.
   - Deliverable: structure, drift, and staging alignment report.
3. `FULL_AUDIT`
   - Alias of `FULL_AUDIT_CORE` unless user explicitly requests repo-wide scope.

## 8. Project Constraints

1. Intellectual property and code source:
   - Do not reuse, reproduce, or adapt code from external libraries, patented ideas, or previous projects.
   - Every line of code must be original work designed for the HEBS engine.
2. Design and implementation:
   - Keep core logic flat, data-oriented, and cache-aware.
   - No OOP style abstractions in core engine code.
3. Benchmark and test isolation:
   - Tests and benches may not directly edit core data structures except through public API calls.
   - Telemetry must be sampling-oriented and must not secretly inject release hot-path behavior.
4. Reproducibility:
   - Record compiler version, compile flags, CPU metadata, and commit hash in run manifests.
   - Use median-of-N protocol for benchmark decisions.
5. Artifact format:
   - Store tabular machine-readable data in CSV.
   - Store event streams in NDJSON where needed.
   - Provide human summary in Markdown, optional HTML render.
6. Readability:
   - Use meaningful headings and plain-language labels in docs and reports.
   - Keep generated artifacts grouped under dedicated directories.

## 9. Style and Hygiene

1. Brace style:
   - Opening brace on new line, one tab beyond parent declaration/command.
   - Body aligned to opening brace indentation.
   - Closing brace on its own line, followed by a blank line.
2. Interior function comments are allowed when they capture invariants, non-obvious constraints, or tricky data-flow; avoid obvious narration comments.
3. No transient output in repository root.
4. Terminal captures and simulation outputs must stay in `tests/`, `reports/`, or `perf/`.

## 10. Compliance Enforcement

1. Canon invariants must be machine-verified by project verifier tooling once introduced.
2. Protected canon/governance edits should be guarded by hooks and checksum verification.
3. Verifier output must include impacted scope classification for touched files.

## 11. Change Workflow

1. Emit Tenet Impact Notice.
2. Propose change scope.
3. If any requested action conflicts with documented requirements, list each conflict as a named deviation.
4. For each named deviation, require an explicit approval token in the form `APPROVED <DeviationName>`.
5. Receive matching authorization.
6. Implement only within approved scope.
7. Update canonical staging/history artifacts when governance impact exists.
8. Run verifier when execution approval includes command execution.

## 12. Context Reset and Handoff

1. To reduce context drift, maintain a compact handoff file with:
   - current goals
   - active constraints
   - locked baselines
   - next actions
2. When starting a new thread/session, use the handoff file and current canonical docs as the only required context.

## 13. Repo Map Maintenance Mandate

1. `HEBS_REPO_MAP.md` must be updated in the same approved change whenever repository facts change.
2. Mandatory update triggers include:
   - Added, removed, or renamed functions.
   - Added, removed, or renamed enums, structs, constants, macros, or variables documented as part of behavior or interface.
   - Added, removed, renamed, or moved files and directories.
   - Changes to API usage flow, examples, benchmarks, ledgers, or artifact paths.
3. A state-changing implementation is non-compliant if it leaves `HEBS_REPO_MAP.md` stale.

## 14. Conflict Clarification Language (Mandatory)

1. When a directive conflicts with documented requirements, the assistant must pause and issue one explicit line per conflict.
2. Each line must name the conflict and required approval token using this format:
   - `This <DeviationName> is not the same as required in documentation. Reply APPROVED <DeviationName> to explicitly confirm and continue.`
3. Work may continue only after all listed deviation tokens are explicitly approved.
