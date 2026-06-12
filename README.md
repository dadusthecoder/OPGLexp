# 🌌 OpenGL Experimental Renderer

This project is an **experimental renderer** built with **C++ and OpenGL**, created as a personal learning journey into the world of modern graphics programming and real-time rendering pipelines.This is my first Computer graphics related project , the main goal here is to learn about the graphics api , and how dose the conputer actually rasterize the pxles.
The long-term goal is to evolve this project into a **Physically Based Renderer (PBR)** from scratch. For now, it serves as a sandbox for experimenting with rendering techniques, shaders, and scene management systems.

---
## screenshots
![Image](https://github.com/user-attachments/assets/488a5f97-6180-4530-9470-11fa5bdac939)

## 📸 Current Features (Experimental)

> These are early implementations meant to support learning and experimentation.

- 🎮 **Interactive Camera System** with configurable movement speed
- 💡 **Lighting Settings Panel**
  - Position control
  - RGB color picker
  - Auto-alignment with the camera
- 🎨 **Real-Time Shader Reloading**
  - Reload vertex/fragment shaders without restarting the app
- 📦 **Model Controls**
  - Load `.fbx` or `.obj` files
  - Position, Rotation (in degrees), and Scaling transforms
  - Reset buttons for convenience
- 🌌 **Environment Controls**
  - Background color configuration
- 🧭 **Camera Diagnostics Panel**
  - Real-time position and direction info
- 🧪 **Render Mode Switching**
  - Fill, Wireframe, Point modes
- ⚖️ **Gravity Toggle** (basic simulation toggle)
- 📈 **FPS Display** (updated in real time via ImGui)

---

## 🚀 Planned Features

> Future development will progressively introduce more advanced rendering concepts.

- ✅ Physically Based Rendering (PBR)
  - Albedo, Roughness, Metalness, Normal maps
- ✅ Image-Based Lighting (IBL) with HDR
- ✅ Shadow Mapping (Directional + Point lights)
- ✅ Material system with GUI-based editing
- ✅ Post-processing effects (Bloom, FXAA, Tonemapping)
- ✅ Scene hierarchy and editor
- ✅ Entity Component System (ECS) for simulation
- ✅ Asset hot-reloading

---

## 🧱 Tech Stack

| Component              |                  Tool used                          |
|------------------------|-----------------------------------------------------|
| **Graphics API**       | OpenGL (Core Profile)                               |
| **GUI**                | [Dear ImGui](https://github.com/ocornut/imgui)      |
| **Windowing/Input**    | [GLFW](https://github.com/glfw/glfw)                |
| **Model Loading**      | [Assimp](https://github.com/assimp/assimp)          |
| **Math**               | [GLM](https://github.com/g-truc/glm)                |
| **Development IDE**    | Visual Studio 2022                                  |

---

## 🛠️ Building the Project

### Prerequisites
- Visual Studio 2022 (or later)
- CMake (optional if using VS solution directly)
- OpenGL-compatible GPU

### Build Instructions
1. Clone the repo:
   bash: git clone https://github.com/dadusthecoder/opengl2.git

2. Open the .sln file in Visual Studio 2022.
  - Make sure the following directories are linked properly using vcpkg or you can link them manully:
  - /Dependencies ( GLFW,GLEW,Assimp ,glm , imGui)
  ## resources
  - /res (Shaders, models) 

 3. Build and Run.
 

### Controls and UI
  - Use mouse and WASD keys to navigate the camera.
  - Use the ImGui panel to:
  - Change model position, rotation, scale
  - Toggle gravity
  - Change light position and color
  - Reload shaders or models in real-time

  
