# Renderer Report: Clustered Light Culling & Cascaded Shadow Maps

## Overview
Implemented Compute-Based Clustered Deferred Light Culling using OpenGL Compute Shaders and SSBOs.
The system partitions the view frustum into a 3D grid (clusters) and culls lights against the AABBs of these clusters.

Also implemented Cascaded Shadow Maps (CSM) to replace the old RTShadowPass for directional lights.

## Implementation Details (Clustered Light Culling)
1. **Pass Structure**: Created `LightCullingPass` (`LightCullingPass.h/cpp`) which dispatches two compute shaders:
   - `cluster_aabb.comp`: Computes view-space AABBs for each cluster slice. (Only runs when screen resizes).
   - `cluster_cull.comp`: Intersects all lights against the cluster AABBs and populates a global light index list and a light grid offset/count list.
2. **SSBOs**:
   - `ClusterAABBBuffer` (Binding 0)
   - `LightDataBuffer` (Binding 1)
   - `LightGridBuffer` (Binding 2)
   - `LightIndexBuffer` (Binding 3)
   - `GlobalIndexCountBuffer` (Binding 4)
3. **Lighting Pass**: Updated `lighting.glsl` to remove the uniform light loop and instead use the SSBOs to fetch the lights specifically for the current pixel's cluster.

## Implementation Details (Cascaded Shadow Maps)
1. **Pass Structure**: Created `CSMPass` (`CSMPass.h/cpp`).
   - Generates 4 cascade layers in a `GL_TEXTURE_2D_ARRAY`.
   - Computes practical split scheme distances and builds light-space orthographic projections per cascade.
   - Executes by rebinding the global instance SSBOs and indirect draw buffers to re-render the scene from the directional light's perspective.
2. **Shaders**:
   - `csm_depth.glsl`: Transform-only vertex shader reusing the SSBO vertex pull pattern. Empty fragment shader.
   - `lighting.glsl`: Computes PCF shadowing by selecting the appropriate cascade slice based on view-space depth.

## Debugging / Fixes
- **Link Error**: Fixed undefined symbols by adding `src/Renderer/Passes/LightCullingPass.cpp` to `CMakeLists.txt`.
- **GL_INVALID_OPERATION Error**: 
   - Found that `u_GridSize` was being set as `ivec3` in C++ via `glProgramUniform3i`, but the compute shaders declared it as `uvec3`. Changed shaders and C++ to use `ivec3`.
   - Found that `u_LightCount` in `cluster_cull.comp` was declared as `uint`, but C++ was setting it with `glProgramUniform1i`. Changed the shader declaration to `int` and casted to `uint` internally.
   - Found that the old uniform `u_LightCount` was still being passed to `s_LightingShader`, and the new uniforms (`u_ViewMatrix`, `u_GridSize`, `u_ZNear`, `u_ZFar`) were missing. Updated `Renderer.cpp` to set the correct uniforms.

## Next Steps
The renderer is now fully operational with Clustered Light Culling and Cascaded Shadow Maps. We can proceed to the next objective: **Geometry Pipeline Overhaul & SPOM Integration**.
