# ADR 003: Game Development Workflows & MCP Server

## Status
Accepted

## Date
2026-07-29

## Context
The engine was primarily focused on rendering (GPU-driven pipeline, PBR, etc.). The user needed a workflow to develop actual game logic, using C++ `NativeScript` classes and attaching them to EnTT entities via the Scene. However, since C++ lacks reflection, there was no centralized way to map script class names to instantiation functions, which prevented serialization and tool-assisted script creation. Furthermore, an AI agent workflow was requested to assist in game development without mixing with the rendering pipeline.

## Decision
1. **Script Registry**: Implemented `ScriptRegistry` to map string names to `NativeScriptComponent::Bind<T>()` lambdas, allowing serialization of attached scripts.
2. **MCP Game Dev Server**: Created a Python FastMCP server (`scripts/mcp_game_dev.py`) to automate boilerplate for new NativeScripts, auto-register them in `ScriptRegistry`, and trigger builds.
3. **Agent Skill**: Created a dedicated `SKILL.md` for AI game programming in Atlas Engine.

## Consequences
- **Positive**: Agents and humans can rapidly create game logic without worrying about manual `#include` and registry boilerplate. Scene serialization for NativeScripts is now possible.
- **Negative**: Requires python and the `mcp` pip package for the developer workflow. `ScriptRegistry.cpp` is automatically updated, which might cause merge conflicts in a team environment if not careful.
