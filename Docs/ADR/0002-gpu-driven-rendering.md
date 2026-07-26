# ADR 0002: GPU-Driven Rendering Architecture

## Status
Accepted

## Context
The engine previously relied on a traditional CPU-bound rendering loop, issuing `glDrawElementsInstanced` for each mesh individually. This approach scales poorly with large scenes due to CPU overhead. We need to transition to a GPU-driven pipeline to take advantage of modern GPU capabilities for culling and drawing.

## Decision
We implemented a GPU-driven pipeline with the following characteristics:
1. **Meshlet Generation**: We integrated `meshoptimizer` to slice loaded meshes into small chunks called "Meshlets" (max 64 vertices, 124 triangles).
2. **Global Geometry Buffers**: The `Renderer` now maintains large persistent SSBOs for all geometry (`GlobalVertexBuffer`, `GlobalIndexBuffer`, `GlobalMeshletBuffer`).
3. **Bindless Vertex Pulling**: The vertex shader (`geometry_pass.glsl`) was rewritten to ignore traditional VAO attributes (`layout(location=0)`) and instead fetch vertices manually from the `GlobalVertexBuffer` SSBO using `gl_VertexID`.
4. **Compute Frustum Culling**: A new compute shader (`cull.comp`) iterates over instances and their associated meshlets, performs frustum culling against the meshlet bounding spheres, and atomic-appends visible meshlets to an `IndirectDrawBuffer`.
5. **Indirect Dispatch**: `Renderer::ExecuteQueue` now issues a single `glMultiDrawElementsIndirectCount` (or standard multi-draw) to render all visible geometry in one API call.
6. **Static vs Dynamic Separation**: As per project requirements, static geometry is optimized by uploading it once to the global buffers, while dynamic instances update an `InstanceBuffer` SSBO every frame.

## Consequences
**Positive**:
- Significantly reduced CPU overhead.
- Fine-grained frustum culling at the meshlet level, improving GPU fillrate.
- Foundation for future GPU-driven features like two-pass occlusion culling.

**Negative**:
- Increased shader complexity (bindless fetching).
- Hard dependency on OpenGL 4.6 (Compute Shaders, SSBOs, `gl_BaseInstance`, MultiDrawIndirect).
- `ModelLoader` now has to upload geometry to global buffers, slightly coupling asset loading with renderer state.

## Notes
Future passes will implement two-pass Occlusion Culling using a Hi-Z depth pyramid to further reduce triangle counts sent to the rasterizer.
