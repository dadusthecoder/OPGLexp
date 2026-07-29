# Current Sprint

## Objective
Initialize Game Development capabilities via ScriptRegistry and MCP Server tools.

## Branch
`game-dev` (merge to `master` when complete)

## Completed Tasks
- [x] Create `src/Game/ScriptRegistry.h` and `.cpp` for dynamic component binding.
- [x] Update `NativeScriptComponent` and `SceneSerializer.cpp` to store `ScriptName`.
- [x] Include and initialize `ScriptRegistry::RegisterAllScripts()` in `src/main.cpp`.
- [x] Create python-based `scripts/mcp_game_dev.py` and `scripts/requirements_mcp.txt` for automation.
- [x] Add Antigravity skill `SKILL.md` (ADR 003).

---

# Completed Sprint: AAA Renderer Upgrade

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
- [x] `res/shaders/lighting.glsl` — PBR + IBL + DDGI sampling + RTAO occlusion + emissive (uncommitted)
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
**ACHIEVED** — DDGI Production-Grade Overhaul complete.

---

# Completed Sprint: DDGI Production-Grade Overhaul

## All Tasks Complete
- [x] `DDGIPass.h` — Rewritten (distance atlas, probe state buffer, 4 shaders)
- [x] `DDGIPass.cpp` — Rewritten (6-pass pipeline, correct atlas dimensions)
- [x] `ddgi_probe_trace.comp` — Rewritten (Rodrigues rotation, multi-bounce, backface detect, TraceAnyHit fixed)
- [x] `ddgi_probe_update.comp` — Rewritten (dual mode irradiance/distance, gamma 5.0 encoding, change detection)
- [x] `ddgi_border_copy.comp` — NEW (octahedral border mirroring, probeIndex→3D→atlas decomposition fixed)
- [x] `ddgi_probe_classify.comp` — NEW (backface counting, probe relocation, state management)
- [x] `lighting.glsl` — SampleDDGI rewritten (Chebyshev visibility + backface rejection + probe state + gamma decode)
- [x] `Renderer.cpp` — Distance atlas (unit 10), probe state SSBO (binding 10), probesPerRow uniform
- [x] EditorLayer updated with DDGI tuning sliders (Intensity, Bounce Intensity, Hysteresis, Max Ray Distance).
- [x] Build Debug succeeds, shaders copied
- [x] Roadmap/DDGI.md updated
