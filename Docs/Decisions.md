# Architecture Decisions

This document summarizes major architectural decisions made during development. For detailed rationale, see the `Docs/ADR/` folder.

- **2026-07-29**: Implemented `ScriptRegistry` and a Game Dev MCP Server for automated game development workflows (ADR 003).
- **2026-07-27**: Adopted an AI-assisted development workflow using `AGENTS.md` and detailed documentation to synchronize context between LLMs.
- **2026-07-26**: Chose EnTT for the Entity Component System (ECS) to maintain data locality and performance.
- **2026-07-26**: Decided to use `meshoptimizer` to automatically optimize geometry for vertex cache and overdraw at model load time.
