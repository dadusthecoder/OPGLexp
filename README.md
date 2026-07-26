# OPGLexp (OpenGL Experimental Renderer)

This project is an **experimental renderer** built with **C++ and OpenGL**, created as a personal learning journey into the world of modern graphics programming, game engine UI design, and real-time rendering pipelines.

The long-term goal is to evolve this project into a **Physically Based Renderer (PBR)** from scratch. For now, it serves as a sandbox for experimenting with rendering techniques, shaders, and scene management systems.

---

## Screenshots
*(Note: These are early implementations meant to support learning and experimentation)*
<img width="1909" height="1040" alt="Screenshot 2026-07-27 025127" src="https://github.com/user-attachments/assets/91386c15-501f-4156-93e7-4105cb9b3ebf" />
<img width="1900" height="1040" alt="image" src="https://github.com/user-attachments/assets/1167ddd5-886b-4bd5-90b5-94e512cba9f9" />

---

## Planned Features
> Future development will progressively introduce more advanced rendering concepts.
- Image-Based Lighting (IBL) with HDR
- Post-processing effects (Bloom, FXAA, Tonemapping)
- Asset hot-reloading

---

## External Libs
| Component              |                  Tool used                          |
|------------------------|-----------------------------------------------------|
| **Graphics API**       | OpenGL 4.6 (Core Profile)                           |
| **GUI**                | [Dear ImGui](https://github.com/ocornut/imgui) (Docking Branch) |
| **Windowing/Input**    | [GLFW](https://github.com/glfw/glfw)                |
| **Model Loading**      | [Assimp](https://github.com/assimp/assimp)          |
| **Math**               | [GLM](https://github.com/g-truc/glm)                |
| **Logging**            | [spdlog](https://github.com/gabime/spdlog)          |
| **Build System**       | CMake, Ninja, Clang-CL, PowerShell                  |
| **Package Manager**    | vcpkg (Manifest Mode)                               |

---

## Building the Project

### Prerequisites
- **Visual Studio 2022** (with C++ Desktop Development tools)
- **CMake** & **Ninja**
- **LLVM / Clang** installed
- **vcpkg** (Ensure `VCPKG_ROOT` environment variable is set)

### Build Instructions
The project uses an automated PowerShell script to configure CMake and build the project via Ninja and Clang-CL.

1. Clone the repo:
   ```bash
   git clone https://github.com/dadusthecoder/OPGLexp.git
   cd OPGLexp
   ```

2. Run the build script:
   ```powershell
   .\build.ps1
   ```
   *Note: On the first run, vcpkg will automatically download and build all dependencies statically (Assimp, ImGui, spdlog, GLFW, etc.). This may take several minutes.*

3. Run the application:
   ```powershell
   .\build\OPGLexp.exe
   ```

---

##  Controls and UI
- **Camera Navigation**: Hold `Right Mouse Button` to lock the cursor and look around. Use `W A S D` to move. Press `ESC` to unlock the cursor.
- **Scene Interaction**: Select nodes in the **Scene Hierarchy**.
- **Inspector**: Modify node properties, translation, rotation, scale, and swap materials for distinct meshes on the selected node.
- **Asset Browser**: Drag models directly into the viewport or use the file browser UI to load them dynamically.
