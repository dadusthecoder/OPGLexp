#pragma once
#include "../Core/Texture.h"
#include "../Core/Shader.h"
#include "../Validation/RendererValidationFramework.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace lgt {
    class RadianceCascadesPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static void Execute(uint32_t gDepthID, uint32_t gNormalID, const glm::mat4& invVP, const glm::vec3& cameraPos, int frameIndex, const glm::mat4& prevInvVP, const glm::vec3& sunDir, const glm::vec3& sunColor, float sunIntensity);
        static void Resize(uint32_t width, uint32_t height);
        static void Shutdown();

        static uint32_t GetRadianceAtlasID();

#ifdef ATLAS_VALIDATION
    enum class RCDebugCategory { None = 0, Infrastructure, Geometry, Radiance, Lighting };
    enum class RCInfrastructureDebug { ShaderDispatch = 0 };
    enum class RCGeometryDebug { ProbeGrid = 0, RayDirections, WorldPosition, Intervals, RayOrigins, BVH, BVHHitData };
    enum class RCRadianceDebug { Atlas = 0, RadianceInterval, Beta, CascadeCurrent, CascadeParent, CascadeMerged, MergeDifference };
    enum class RCLightingDebug { DiffuseOnly = 0, SpecularOnly, Combined };
#endif

        // Config properties
        static int& GetCascadeCount();
        static int& GetBaseProbeSpacing();
        static float& GetBaseInterval();
        static float& GetRayIntensity();
        static int& GetDebugCascade(); // -1 for none, 0..N-1 for specific cascade visualization
        
#ifdef ATLAS_VALIDATION
        static int& GetDebugCategory();
        static int& GetDebugMode();
        
        struct CascadeStats {
            uint32_t RaysTraced = 0;
            uint32_t Hits = 0;
            uint32_t Misses = 0;
            uint32_t Beta0 = 0;
            uint32_t Beta1 = 0;
        };
        static CascadeStats* GetStatistics(); // Returns array of size GetCascadeCount()
#endif
        
    private:
        static void RecreateAtlas();

    private:
        static Shader* s_TraceShader;
        static Shader* s_MergeShader;
        static Shader* s_TemporalBlendShader;
        static Shader* s_DebugShader;

        static Texture* s_RadianceAtlas;
        static Texture* s_HistoryAtlas;

        static uint32_t s_Width;
        static uint32_t s_Height;

        static int s_CascadeCount;
        static int s_BaseProbeSpacing;
        static float s_BaseInterval;
        static float s_RayIntensity;
        static int s_DebugCascade;
    };
}
