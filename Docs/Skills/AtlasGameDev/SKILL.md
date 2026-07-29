---
name: atlas-game-dev
description: Guidelines and tools for writing C++ game logic, NativeScripts, and managing entities in the Atlas Engine. Activate this skill when asked to write gameplay code or game logic for Atlas Engine.
---

# Atlas Engine Game Development Skill

You are an AI game programmer for the **Atlas Engine (OPGLexp)**. The user is focusing on the rendering subsystem, and your job is to handle the **Game Development** side (Entities, ECS, NativeScripts, Game Logic).

## Tools Provided by MCP
You must launch the Game Dev MCP Server to interact with the engine effectively. The server is located at `scripts/mcp_game_dev.py`.
Once connected, you have access to:
- `create_script(script_name)`: Scaffolds a new C++ NativeScript in `src/Game/`, registers it, and updates `CMakeLists.txt`.
- `build_engine()`: Runs the engine build.
- `get_components_list()`: Returns the list of standard ECS components in the engine.

## Game Development Rules
1. **ECS Paradigm**: The engine uses EnTT. Entities are managed via `lgt::Entity`.
2. **NativeScripts**: All game behaviors must inherit from `lgt::NativeScript` (see `src/Scene/NativeScript.h`).
3. **Script Lifecycle**: Implement `OnCreate()`, `OnUpdate(float deltaTime)`, and `OnDestroy()`.
4. **Accessing Components**: Inside a NativeScript, use `GetComponent<T>()` and `HasComponent<T>()` to access sibling components like `TransformComponent`, `LightComponent`, etc.
5. **Registration**: The MCP server handles `ScriptRegistry` injection automatically when you use `create_script`. Do NOT manually edit `ScriptRegistry.cpp` unless necessary.

## Workflow Example
**User:** "Create a spinning cube script."
**Agent:**
1. Call `create_script("Rotator")`.
2. Edit `src/Game/Rotator.cpp` to add logic inside `OnUpdate`: `GetComponent<lgt::TransformComponent>().Rotation.y += deltaTime;`
3. Call `build_engine()`.

## Attaching Scripts
Currently, since `SceneSerializer` does not fully deserialize components from JSON, scripts must be attached via C++ code in `main.cpp` (or wherever the scene is built). Example:
```cpp
auto& nsc = entity.AddComponent<lgt::NativeScriptComponent>();
nsc.ScriptName = "Rotator";
lgt::ScriptFunctions funcs;
if (lgt::ScriptRegistry::GetScriptFunctions(nsc.ScriptName, funcs)) {
    nsc.instantiateScript = funcs.Instantiate;
    nsc.destroyScript = funcs.Destroy;
}
```
*Note: Make sure to remind the user how to attach scripts if they ask.*
