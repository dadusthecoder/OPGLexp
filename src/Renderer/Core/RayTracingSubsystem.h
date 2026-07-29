#pragma once
#include "Buffer.h"
#include "../Utils/BVH.h"
#include <vector>

namespace lgt {
    class RayTracingSubsystem {
    public:
        static void Build(const std::vector<float>& vertices,
                          const std::vector<uint32_t>& indices);
        static void Bind(uint32_t bvhBinding, uint32_t triBinding);
        static bool IsReady();
        static uint32_t GetTriangleCount();

    private:
        static Buffer* s_BVHBuffer;     // SSBO binding point 6
        static Buffer* s_TriBuffer;     // SSBO binding point 7
        static uint32_t s_TriCount;
        static bool s_Ready;
    };
}
