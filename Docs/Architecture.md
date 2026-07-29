# Atlas Engine Architecture

## Overview
Atlas Engine (currently OPGLexp) is an ongoing project to build an AAA-grade, GPU-driven renderer and game engine using C++ and OpenGL 4.6.

## Subsystems
- **Core**: Input, Resource Management, Application Loop.
- **Scene**: EnTT-based Entity Component System, C++ NativeScript behaviors, JSON Serialization.
- **Renderer**: G-Buffer Deferred rendering, PBR, Directional/Point lights, ImGui integration.
- **Game**: Uses `ScriptRegistry` to dynamically map `NativeScriptComponent` scripts to runtime instances. C++ Game logic is orchestrated using the `scripts/mcp_game_dev.py` MCP server.
- **Editor**: Built-in editor with Scene Hierarchy, Properties Panel, Viewport, and Console.
- **Helpers**: Zero-cost Debugging Stack (`LGT_DIST` macro), `spdlog` integration, and thread-safe `DebugStats` module for UI tracking.

## Current State
The engine has robust rendering capabilities and is actively expanding its game-development tools. An MCP server automates AI-assisted game development.
