# Renderer Design

## Current Pipeline (Deferred)
1. **Geometry Pass**: Renders meshes into a G-Buffer (AlbedoSpec, Normal, PBR, Depth).
2. **Lighting Pass**: Fullscreen quad computing PBR lighting (Cook-Torrance BRDF) into an HDR buffer.
3. **Post-Process Pass**: Fullscreen quad handling exposure and tone mapping into the final color buffer.

## Future Architecture (GPU-Driven)
The renderer will transition to a Render Graph based system handling:
- Frustum & Occlusion Culling via Compute Shaders
- Indirect drawing (`glMultiDrawElementsIndirect`)
- Meshlet generation and cluster culling
- Clustered Forward / Tile-based Deferred lighting
