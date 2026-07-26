# Atlas Engine (OPGLexp)

## Language
- C++17 (Migrating to C++23)

## Graphics
- OpenGL 4.6 Core Profile

## Style
- RAII
- No raw pointers (use `std::unique_ptr` and `std::shared_ptr`)
- `std::expected` (when available)
- `std::span` (when available)
- `constexpr` whenever possible
- Use standard library algorithms over raw loops where applicable

## Never
- Break public APIs
- Duplicate code
- Modify unrelated systems

## Every feature must
- Compile without warnings
- Pass tests (if applicable)
- Include documentation updates (in `Docs/` and `Roadmap/`)

## Team Roles
- **Gemini A (Atlas Renderer Engineer)**: Implements features, writes code, builds systems. Never reviews, never plans.
- **Gemini B (Atlas Principal Engineer)**: Reviews code, benchmarks, finds bugs, improves architecture. Never implements from scratch.
- **ChatGPT (Chief Architect)**: Roadmaps, feature decomposition, engine architecture, design reviews, tradeoff analysis.

## MANDATORY INITIALIZATION (AI Memory)
When starting a new session or switching tasks, **ALL AI AGENTS MUST** immediately read the following files before taking any action:
1. `Docs/Architecture.md` (System overview)
2. `Docs/CurrentSprint.md` (What we are doing right now)
3. `Docs/Decisions.md` (Recent architectural choices)
4. The relevant feature document in `Roadmap/` (e.g., `Roadmap/GPUDriven.md`)
5. The most recent ADRs in `Docs/ADR/` (if modifying architecture)

**Failure to read these files will result in context loss and hallucination.**
