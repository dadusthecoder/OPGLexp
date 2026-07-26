# Atlas Engine Architecture

## Overview
Atlas Engine (currently OPGLexp) is an ongoing project to build an AAA-grade, GPU-driven renderer and game engine using C++ and OpenGL 4.6.

## Subsystems
- **Core**: Input, Resource Management, Application Loop.
- **Scene**: EnTT-based Entity Component System, C++ NativeScript behaviors, JSON Serialization.
- **Renderer**: G-Buffer Deferred rendering, PBR, Directional/Point lights, ImGui integration.
- **Editor**: Built-in editor with Scene Hierarchy, Properties Panel, Viewport, and Console.

## Current State
The engine relies heavily on CPU-driven `glDrawElementsInstanced` calls. We are transitioning towards a GPU-driven pipeline, Render Graph architecture, and advanced features (DDGI, RTAO).
