# Atlas Engine Architecture

## Overview
Atlas Engine (currently OPGLexp) is an ongoing project to build an AAA-grade, GPU-driven renderer and game engine using C++ and OpenGL 4.6.

## Subsystems
- **Core**: Input, Resource Management, Application Loop.
- **Scene**: EnTT-based Entity Component System, C++ NativeScript behaviors, JSON Serialization.
- **Renderer**: G-Buffer Deferred rendering, PBR, Directional/Point lights, ImGui integration.
- **Editor**: Built-in editor with Scene Hierarchy, Properties Panel, Viewport, and Console.
- **Helpers**: Zero-cost Debugging Stack (`LGT_DIST` macro), `spdlog` integration, and thread-safe `DebugStats` module for UI tracking.

## Current State
The engine is transitioning to a fully GPU-driven pipeline (Meshlets, Compute Frustum Culling, MultiDraw) and modern debugging workflows.
