# HEBS Repository Agent Policy
Scope: Entire repository
This file defines agent execution policy only. 
Repository architecture, design plans, and technical specifications belong in other documents.
   Use plain English and conversational tone. This is redundant for a reason. Understand it's importance and follow it.

## 1. Command Authority

   1. Default mode is constrained execution.
   2. Read-only actions are implicitly allowed when directly requested.
   3. State-changing actions require explicit approval token `APPROVED`.
   4. `APPROVED` is scoped to the most recent proposal only.
   5. Scope overrides use explicit directives in the form `DO: <scope>`.
   6. If no proposal is pending, `APPROVED` is invalid.

## 2. Project Documentation
   1. HEBS must target correctness, determinism, repeatability, low latency, and high throughput together.
   2. HEBS is procedural C and Data-Oriented Design (DOD) by default.
   3. Object-oriented design patterns are prohibited in core engine code.
   4. Loops are allowed only when they are the best measured option.
   5. Testing and benchmarking tools must sample public engine interfaces and must not mutate core internals directly.

   Governance and canonical files:
   - `AGENTS.md`
   - `HEBS_REPO_MAP.md`
   - `TAB_PROTOCOL.md`
   - `PROBE_PROTOCOL.md`

   Other useful files:
   - `BENCH_METRICS.md` - contains metrics that we should be recording
   - `HEBS_PLAN.md` - defines the core architecture and goals for this repo
   - `git_history.md` - dedicated repository commit/reflag history reference

## 3. Authorization Tiers

1. Standard edit (non-core): `APPROVED`.
2. Core implementation edit (non-canon): `DO: CORE_EDIT` + `APPROVED`.
3. Canon or governance change: `DO: CANON_CHANGE` + `APPROVED`.
4. Authorization mapping:
   1. `TENET_IMPACT=NONE` -> `APPROVED`.
   2. `TENET_IMPACT=POTENTIAL` -> `DO: CORE_EDIT` + `APPROVED`.
   3. `TENET_IMPACT=REQUIRES_CANON_CHANGE` -> `DO: CANON_CHANGE` + `APPROVED`.

## 4. Mandatory Canon Impact Notice
   Interpretation:
   1. `NONE`: no core file or canon impact identified.
   2. `POTENTIAL`: touches core implementation files; canon impact not expected but possible.
   3. `REQUIRES_CANON_CHANGE`: touches canonical governance or technical specification files.

## 5. Response Style
   1. Use plain English and conversational tone.
   2. Keep sentences short and easy to follow.
   3. Avoid jargon unless necessary; define it briefly when used.
   4. Prefer concrete examples over abstract statements.
   5. Keep answers brief by default unless detail is requested.
   6. Ask one quick clarifying question when requirements are unclear.

## 6. Project Constraints
   1. Intellectual property and code source:
      - Do not reuse, reproduce, or adapt code from external libraries, patented ideas, or previous projects.
      - Every line of code must be original work designed for the HEBS engine.
   2. Design and implementation:
      - Keep core logic flat, data-oriented, and cache-aware.
      - No OOP style abstractions in core engine code.
   3. Brace style:
      - Opening brace on new line, one tab beyond parent declaration/command.
      - Body aligned to opening brace indentation.
      - Closing brace on its own line, followed by a blank line.
   4. Interior function comments are allowed when they capture invariants, non-obvious constraints, or tricky data-flow; avoid obvious narration comments.

## 7. Conflict Clarification Language (Mandatory)
   1. When a directive conflicts with documented requirements, the assistant must pause and issue one explicit line per conflict.
   2. Each line must name the conflict and required approval token using this format:
      - `This <DeviationName> is not the same as required in documentation. Reply APPROVED <DeviationName> to explicitly confirm and continue.`
   3. Work may continue only after all listed deviation tokens are explicitly approved.
