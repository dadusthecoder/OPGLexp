# OPGLexp (Atlas Engine)

This project is my personal learning journey into modern graphics programming, game engine UI design, and real-time rendering pipelines built with C++ and OpenGL.

What started as a simple experimental renderer has now evolved into a fully GPU-driven engine (which I'm calling the Atlas Engine architecture). I built this to push the boundaries of what I can do with OpenGL 4.6, aiming for a Physically Based Renderer (PBR) from scratch with a highly optimized data-oriented design.

---

## The Two Branches

I currently maintain two different branches of this project depending on what I'm experimenting with:

### 1. `master` (The Engine Architecture)
This is the main focus of development right now. I've completely overhauled the architecture to support a **GPU-Driven Rendering Pipeline**. 
* **Current Features:**
  * Bindless vertex pulling using large global Shader Storage Buffer Objects (SSBOs).
  * Meshlet generation (via `meshoptimizer`) and frustum culling executed entirely on the GPU via Compute Shaders.
  * Multi-draw indirect rendering (`glMultiDrawElementsIndirectCount`).
  * A custom zero-cost debugging stack for tracking real-time rendering statistics (like meshlets passing the culling tests) directly inside the Editor UI.
  * A custom Editor Layer built with Dear ImGui (Docking branch).

<img width="1919" height="1079" alt="Screenshot 2026-07-27 021907" src="https://github.com/user-attachments/assets/9166b574-b289-4f2d-b26f-ba3e17de5713" />

### 2. `renderer-old` (Rendering Experiments Sandbox)
I didn't want to throw away my previous codebase because its simpler architecture makes it really good for rapid rendering experiments (like tweaking lighting, PBR, and basic scene setup). If you want to see standard forward/deferred rendering tests without the complexity of GPU-driven meshlet culling, check out this branch!

<img width="1909" height="1040" alt="Screenshot 2026-07-27 025127" src="https://github.com/user-attachments/assets/f78fb42a-137f-45d1-a780-89f91bab157e" />

---

## Planned Features
> Future development will progressively introduce more advanced rendering concepts into the main engine.
- Complete the RenderGraph implementation for modular render passes.
- Image-Based Lighting (IBL) with HDR.
- Advanced Post-processing effects (Bloom, FXAA, Tonemapping, Screen Space Reflections).
- Asset hot-reloading.

---

## External Libs
| Component              |                  Tool used                          |
|------------------------|-----------------------------------------------------|
| **Graphics API**       | OpenGL 4.6 (Core Profile)                           |
| **GUI**                | [Dear ImGui](https://github.com/ocornut/imgui) (Docking Branch) |
| **Windowing/Input**    | [GLFW](https://github.com/glfw/glfw)                |
| **Model Loading**      | [Assimp](https://github.com/assimp/assimp)          |
| **Mesh Optimization**  | [meshoptimizer](https://github.com/zeux/meshoptimizer) |
| **Math**               | [GLM](https://github.com/g-truc/glm)                |
| **Logging**            | Custom Logging Stack (formerly spdlog)              |
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
   *Note: On the first run, vcpkg will automatically download and build all dependencies statically. This may take several minutes.*

3. Run the application:
   ```powershell
   .\build\OPGLexp.exe
   ```

---

## Controls and UI
- **Camera Navigation**: Hold `Right Mouse Button` to lock the cursor and look around. Use `W A S D` to move. Press `ESC` to unlock the cursor.
- **Scene Interaction**: Select nodes in the **Scene Hierarchy**.
- **Inspector**: Modify node properties, translation, rotation, scale, and swap materials for distinct meshes on the selected node.
- **Asset Browser**: Drag models directly into the viewport or use the file browser UI to load them dynamically.
