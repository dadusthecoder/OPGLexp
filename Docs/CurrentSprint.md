# Current Sprint

## Objective
AAA Renderer Upgrade — DDGI + RTAO + TAA + IBL + Bloom + ACES Tonemap on `engine` branch.

## Branch
`engine` (merge to `master` when complete)

## Completed Tasks
- [x] BVH Infrastructure: `src/Renderer/Utils/BVH.h/.cpp`, `src/Renderer/Passes/BVHPass.h/.cpp` (SAH-binned, GPU SSBO upload)
- [x] G-Buffer Rewrite: `res/shaders/geometry.glsl` — oct-encoded normals, TBN normal mapping, velocity (motion vectors) as 4th attachment (RG16F)
- [x] Renderer.cpp updated: 4th G-Buffer attachment (velocity), `s_PrevViewProjection` tracking, `BVHPass::Build()` called after geometry upload
- [x] GLSL include system added to `OpenGLShader.cpp` (recursive `#include` resolution)
- [x] `res/shaders/include/common.glsl` — PI constants, SRGBToLinear, ReconstructWorldPos, OctEncode/Decode
- [x] `res/shaders/include/sampling.glsl` — Hammersley, Halton, WangHash, CosineHemisphere, UniformSphere, SphericalFibonacci, TangentToWorld, ImportanceSampleGGX, OctahedralEncode/Decode
- [x] `res/shaders/include/bvh.glsl` — BVHNode struct, HitInfo, IntersectAABB (slab), IntersectTriangle (Möller-Trumbore), TraceAnyHit, TraceClosestHit (iterative stack)
- [x] `res/shaders/include/brdf.glsl` — D_GGX, G_SmithGGX, F_Schlick, CookTorranceSpecular, DirectLight, IntegrateBRDF
- [x] `res/shaders/rtao.comp` — 8-ray hemisphere AO via BVH, blue noise jitter, spatiotemporal
- [x] `res/shaders/rtao_denoise.comp` — bilateral 5x5 spatial filter
- [x] `res/shaders/ddgi_probe_trace.comp` — 64 rays/probe, BVH closest hit, direct lighting + shadow, sky miss
- [x] `res/shaders/ddgi_probe_update.comp` — irradiance atlas update, octahedral projection, temporal blend (hysteresis=0.97)
- [x] `res/shaders/taa.glsl` — history reprojection via velocity, YCoCg neighbourhood clamp, disocclusion handling
- [x] `res/shaders/bloom_downsample.glsl` — 13-tap Jimenez 2014 downsample
- [x] `res/shaders/bloom_upsample.glsl` — 9-tap tent filter
- [x] `res/shaders/tonemap.glsl` — ACES filmic + gamma 2.2
- [x] `res/shaders/lighting.glsl` — PBR + IBL + DDGI sampling + RTAO occlusion + emissive (uncommitted, created by agent)
- [x] IBL shaders: `ibl_equirect.glsl`, `ibl_irradiance.glsl`, `ibl_prefilter.glsl`, `ibl_brdf_lut.glsl` (uncommitted)
- [x] C++ Pass classes created (uncommitted): `RTAOPass.h/.cpp`, `DDGIPass.h/.cpp`, `IBLPass.h/.cpp`, `BloomPass.h/.cpp`
- [x] TAAPass.h/.cpp — check if it exists in `src/Renderer/Passes/`, if not create it
- [x] CMakeLists.txt — add ALL new .cpp files (BVH.cpp, BVHPass.cpp, RTAOPass.cpp, DDGIPass.cpp, IBLPass.cpp, BloomPass.cpp, TAAPass.cpp)
- [x] Commit everything uncommitted — `git add -A && git commit -m "feat(renderer): add IBL, RTAO, DDGI, Bloom, TAA pass classes and lighting shaders"`
- [x] Re-establish Sponza benchmark scene — loaded `res/models/sponza/sponza.obj` in `src/main.cpp` and re-enabled RTAO execution in `Renderer.cpp`
- [x] Added `Renderer Settings` ImGui panel to toggle post-processing and RT effects dynamically.
- [x] Fixed Bloom blurring issue by adding threshold logic.
- [x] Implemented Ray-Traced Shadows (`RTShadowPass`) via Compute Shader BVH `TraceAnyHit` + bilateral `shadow_blur.comp`.
- [x] Implemented Compute-Based Clustered Deferred Light Culling (`LightCullingPass`), optimized SSBO structures, fixed GL_INVALID_OPERATION type mismatches.
- [x] Implemented Cascaded Shadow Maps (CSM) with PCF (`CSMPass`), utilizing GL_TEXTURE_2D_ARRAY for cascade layers and a customized depth shader.
- [x] Fixed missing shadows by adjusting CSM orthographic projection bounds to include geometry behind the camera.
- [x] Fixed GPU memory overwrite issue by correctly sizing `s_GlobalIndirectDrawBuffer` for meshlet counts when culling is disabled during shadow passes.

## Objective Status
**ACHIEVED**. AAA Renderer Upgrade complete and benchmark scene set up with Ray-Traced Shadows and dynamic Editor settings.

---

# Next Sprint (Upcoming)

## Objective
**Geometry Pipeline Overhaul & SPOM Integration**

## Planned Tasks
1. **Static/Dynamic Geometry Split**: Overhaul `Renderer::ExecuteQueue` to stop clearing and rebuilding meshlet buffers every frame. Introduce a dual-pipeline for static vs dynamic entities.
2. **Dynamic Instance Buffer**: Implement a dynamic buffer for moving geometry to be culled and drawn separately from the static background.
3. **Silhouette Parallax Occlusion Mapping (SPOM)**: Introduce advanced Parallax Occlusion Mapping into the material and G-Buffer system to "fake" dense geometric depth (Crimson Desert style) for brick and stone surfaces without increasing polygon count.
4. **Height Map Support**: Update bindless texture loading to support displacement/height maps required for SPOM.
