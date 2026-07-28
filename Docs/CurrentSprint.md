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

## NEXT TASK (incomplete — pick up here)
1. **TAAPass.h/.cpp** — check if it exists in `src/Renderer/Passes/`, if not create it
2. **CMakeLists.txt** — add ALL new .cpp files (BVH.cpp, BVHPass.cpp, RTAOPass.cpp, DDGIPass.cpp, IBLPass.cpp, BloomPass.cpp, TAAPass.cpp)
3. **Commit everything uncommitted** — `git add -A && git commit -m "feat(renderer): add IBL, RTAO, DDGI, Bloom, TAA pass classes and lighting shaders"`
4. **Renderer.cpp integration** — wire all passes into `ExecuteQueue()`:
   - After geometry pass: run `RTAOPass::Execute(gDepthID, gNormalID, ...)`
   - Run `DDGIPass::Execute(sunDir, sunColor, ...)` 
   - In lighting pass: bind RTAO output (slot 7) + DDGI atlas (slot 8) + IBL maps (slots 4,5,6)
   - After HDR buffer: run `TAAPass::Execute(hdrColorID, velocityID, depthID, frame)`
   - After TAA: run `BloomPass::Execute(taaResolvedID, threshold, strength)`
   - Replace post_process.glsl with `tonemap.glsl` (ACES)
   - Track `s_FrameIndex` counter in Renderer.cpp
   - Apply Halton jitter to projection matrix before BeginScene
5. **Build clean** — `cmake --build build --config Debug`
6. **Commit integration** — `feat(renderer): integrate AAA pipeline into Renderer::ExecuteQueue`
7. **Merge to master** — `git checkout master && git merge engine && git push origin master`
8. **Update Docs/Architecture.md and Docs/Decisions.md**
