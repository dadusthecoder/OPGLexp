import os
import sys
import json
import re
import subprocess
from mcp.server.fastmcp import FastMCP

# Initialize FastMCP server
mcp = FastMCP("AtlasGameDev")

ENGINE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
GAME_SRC_DIR = os.path.join(ENGINE_ROOT, "src", "Game")

@mcp.tool()
def create_script(script_name: str) -> str:
    """Creates a new NativeScript C++ class for the game."""
    if not os.path.exists(GAME_SRC_DIR):
        os.makedirs(GAME_SRC_DIR)
        
    header_path = os.path.join(GAME_SRC_DIR, f"{script_name}.h")
    cpp_path = os.path.join(GAME_SRC_DIR, f"{script_name}.cpp")
    
    if os.path.exists(header_path) or os.path.exists(cpp_path):
        return f"Error: Script {script_name} already exists."

    # Generate Header
    with open(header_path, "w") as f:
        f.write(f"""#pragma once
#include "../Scene/NativeScript.h"

class {script_name} : public lgt::NativeScript {{
public:
    void OnCreate() override;
    void OnUpdate(float deltaTime) override;
    void OnDestroy() override;
}};
""")

    # Generate CPP
    with open(cpp_path, "w") as f:
        f.write(f"""#include "{script_name}.h"
#include <iostream>

void {script_name}::OnCreate() {{
    std::cout << "{script_name} created on entity " << (uint32_t)GetEntity().GetID() << std::endl;
}}

void {script_name}::OnUpdate(float deltaTime) {{
    // Game logic here
}}

void {script_name}::OnDestroy() {{
}}
""")

    # Auto-register script in ScriptRegistry.cpp
    registry_path = os.path.join(GAME_SRC_DIR, "ScriptRegistry.cpp")
    if os.path.exists(registry_path):
        with open(registry_path, "r") as f:
            content = f.read()
        
        # Add include if missing
        include_stmt = f'#include "{script_name}.h"'
        if include_stmt not in content:
            content = include_stmt + "\n" + content
            
        # Add REGISTER_SCRIPT
        reg_stmt = f"        REGISTER_SCRIPT({script_name});"
        if reg_stmt not in content:
            content = content.replace(
                "void ScriptRegistry::RegisterAllScripts() {",
                f"void ScriptRegistry::RegisterAllScripts() {{\n{reg_stmt}"
            )
            
        with open(registry_path, "w") as f:
            f.write(content)

    return f"Successfully created {script_name} script in src/Game/"

@mcp.tool()
def build_engine() -> str:
    """Builds the Atlas Engine project."""
    build_script = os.path.join(ENGINE_ROOT, "build.ps1")
    if os.path.exists(build_script):
        try:
            result = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", build_script], capture_output=True, text=True, cwd=ENGINE_ROOT)
            if result.returncode == 0:
                return f"Build Successful:\n{result.stdout}"
            else:
                return f"Build Failed:\n{result.stderr}\n{result.stdout}"
        except Exception as e:
            return f"Error executing build script: {str(e)}"
    
    return "Error: build.ps1 not found."

@mcp.tool()
def get_components_list() -> str:
    """Returns a list of available ECS components in the engine."""
    components_path = os.path.join(ENGINE_ROOT, "src", "Scene", "Components.h")
    if not os.path.exists(components_path):
        return "Error: Components.h not found."
        
    components = []
    with open(components_path, "r") as f:
        content = f.read()
        # Simple regex to find structs ending in Component
        matches = re.findall(r'struct\s+([A-Za-z0-9_]+Component)', content)
        for match in matches:
            components.append(match)
            
    return "Available Components:\n" + "\n".join(f"- {c}" for c in components)

if __name__ == "__main__":
    mcp.run()
