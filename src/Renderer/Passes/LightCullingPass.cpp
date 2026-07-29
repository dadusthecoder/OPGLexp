#include "LightCullingPass.h"
#include "../../Vendor/glad.h"
#include "../../Helpers/Logger.h"

namespace lgt {
    Shader* LightCullingPass::s_ClusterAABBShader = nullptr;
    Shader* LightCullingPass::s_ClusterCullShader = nullptr;

    Buffer* LightCullingPass::s_ClusterAABBBuffer = nullptr;
    Buffer* LightCullingPass::s_LightGridBuffer = nullptr;
    Buffer* LightCullingPass::s_LightIndexBuffer = nullptr;
    Buffer* LightCullingPass::s_GlobalIndexCountBuffer = nullptr;

    glm::ivec3 LightCullingPass::s_GridSize = glm::ivec3(16, 9, 24);
    uint32_t LightCullingPass::s_Width = 0;
    uint32_t LightCullingPass::s_Height = 0;
    bool LightCullingPass::s_AABBsNeedUpdate = true;

    void LightCullingPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;

        s_ClusterAABBShader = Shader::CreateCompute("res/shaders/cluster_aabb.comp");
        s_ClusterCullShader = Shader::CreateCompute("res/shaders/cluster_cull.comp");

        uint32_t numClusters = s_GridSize.x * s_GridSize.y * s_GridSize.z;
        uint32_t maxLightsPerCluster = 1024; // Must match shader

        s_ClusterAABBBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, numClusters * sizeof(glm::vec4) * 2, nullptr, BufferUsage::StaticCopy);
        s_LightGridBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, numClusters * sizeof(uint32_t) * 2, nullptr, BufferUsage::StaticCopy);
        s_LightIndexBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, numClusters * maxLightsPerCluster * sizeof(uint32_t), nullptr, BufferUsage::StaticCopy);
        
        uint32_t zero = 0;
        s_GlobalIndexCountBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, sizeof(uint32_t), &zero, BufferUsage::StaticCopy);
    }

    void LightCullingPass::Shutdown() {
        delete s_ClusterAABBShader;
        delete s_ClusterCullShader;

        delete s_ClusterAABBBuffer;
        delete s_LightGridBuffer;
        delete s_LightIndexBuffer;
        delete s_GlobalIndexCountBuffer;
    }

    void LightCullingPass::Resize(uint32_t width, uint32_t height) {
        if (s_Width != width || s_Height != height) {
            s_Width = width;
            s_Height = height;
            s_AABBsNeedUpdate = true;
        }
    }

    void LightCullingPass::RecomputeAABBs(const glm::mat4& invProjMatrix, float nearPlane, float farPlane) {
        s_ClusterAABBShader->Bind();
        s_ClusterAABBShader->SetMat4("u_InvProjection", invProjMatrix);
        s_ClusterAABBShader->SetInt3("u_GridSize", s_GridSize);
        s_ClusterAABBShader->SetFloat("u_ZNear", nearPlane);
        s_ClusterAABBShader->SetFloat("u_ZFar", farPlane);

        s_ClusterAABBBuffer->BindBase(0);
        
        glDispatchCompute(s_GridSize.x, s_GridSize.y, s_GridSize.z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
        s_AABBsNeedUpdate = false;
    }

    void LightCullingPass::Execute(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& invProjMatrix, uint32_t lightCount, float nearPlane, float farPlane) {
        if (s_AABBsNeedUpdate) {
            RecomputeAABBs(invProjMatrix, nearPlane, farPlane);
        }

        // Reset global index count
        uint32_t zero = 0;
        s_GlobalIndexCountBuffer->SetData(&zero, sizeof(uint32_t));

        s_ClusterCullShader->Bind();
        s_ClusterCullShader->SetMat4("u_ViewMatrix", viewMatrix);
        s_ClusterCullShader->SetInt("u_LightCount", lightCount);
        s_ClusterCullShader->SetInt3("u_GridSize", s_GridSize);

        s_ClusterAABBBuffer->BindBase(0);
        // Light buffer is bound to 1 in Renderer::ExecuteQueue
        s_LightGridBuffer->BindBase(2);
        s_LightIndexBuffer->BindBase(3);
        s_GlobalIndexCountBuffer->BindBase(4);

        glDispatchCompute(s_GridSize.x, s_GridSize.y, s_GridSize.z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
