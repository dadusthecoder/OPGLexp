# Engine Core

## Application Flow
The engine initializes through `Renderer::Init()` and `UIManager::Init()`. The main loop calculates delta time, polls input (`lgt::Input::Update()`), updates the `Scene` (NativeScripts and transforms), and submits rendering commands to the `RenderCommandQueue`.

## Resource Management
Assets are managed via a generic template cache (`ResourceManager`) to ensure resources (like Textures and Shaders) are loaded once and shared via `std::shared_ptr`. Models are loaded via `ModelLoader` using Assimp and optimized with `meshoptimizer`.
