#include "RayTracingSubsystem.h"

namespace lgt {
    Buffer* RayTracingSubsystem::s_BVHBuffer = nullptr;
    Buffer* RayTracingSubsystem::s_TriBuffer = nullptr;
    uint32_t RayTracingSubsystem::s_TriCount = 0;
    bool RayTracingSubsystem::s_Ready = false;

    void RayTracingSubsystem::Build(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
        BVHBuilder builder;
        builder.Build(vertices, indices);

        const auto& nodes = builder.GetNodes();
        const auto& triangles = builder.GetTriangles();

        s_TriCount = (uint32_t)triangles.size();

        if (s_BVHBuffer) {
            delete s_BVHBuffer;
        }
        if (s_TriBuffer) {
            delete s_TriBuffer;
        }

        if (!nodes.empty()) {
            s_BVHBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, nodes.size() * sizeof(BVHNode), nodes.data(), BufferUsage::StaticCopy);
        }
        
        if (!triangles.empty()) {
            s_TriBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, triangles.size() * sizeof(BVHTriangle), triangles.data(), BufferUsage::StaticCopy);
        }

        s_Ready = true;
    }

    void RayTracingSubsystem::Bind(uint32_t bvhBinding, uint32_t triBinding) {
        if (s_BVHBuffer) s_BVHBuffer->BindBase(bvhBinding);
        if (s_TriBuffer) s_TriBuffer->BindBase(triBinding);
    }

    bool RayTracingSubsystem::IsReady() {
        return s_Ready;
    }

    uint32_t RayTracingSubsystem::GetTriangleCount() {
        return s_TriCount;
    }
}
