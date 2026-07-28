# DDGI (Dynamic Diffuse Global Illumination)

- [x] BVH CPU Construction and SSBO Upload
- [x] Probe Grid layout definition and volume generation
- [x] Ray Tracing pass (`ddgi_probe_trace.comp`) — Rodrigues rotation, multi-bounce, backface detection
- [x] Probe Irradiance/Distance Update (`ddgi_probe_update.comp`) — Dual-mode (irradiance RGBA16F + distance RG16F), gamma 5.0 encoding
- [x] Probe Border Update (`ddgi_border_copy.comp`) — Octahedral edge/corner mirroring for seamless bilinear filtering
- [x] Probe Classification & Relocation (`ddgi_probe_classify.comp`) — Backface counting, dead probe detection, relocation offsets
- [x] Probe Sampling in Deferred Lighting Pass — Trilinear + backface rejection + Chebyshev visibility + probe state weighting
