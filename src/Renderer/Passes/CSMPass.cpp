#include "CSMPass.h"
#include "../../Vendor/glad.h"
#include "../Core/Shader.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace lgt {

    uint32_t CSMPass::s_Resolution = 2048;
    uint32_t CSMPass::s_FBO = 0;
    uint32_t CSMPass::s_ShadowMapArray = 0;
    glm::mat4 CSMPass::s_LightSpaceMatrices[4];
    float CSMPass::s_CascadeSplits[4];

    static Shader* s_CSMDepthShader = nullptr;

    void CSMPass::Init(uint32_t resolution) {
        s_Resolution = resolution;
        
        glGenFramebuffers(1, &s_FBO);
        
        glGenTextures(1, &s_ShadowMapArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, s_ShadowMapArray);
        glTexImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            GL_DEPTH_COMPONENT32F,
            s_Resolution,
            s_Resolution,
            4,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
        
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        glBindFramebuffer(GL_FRAMEBUFFER, s_FBO);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_ShadowMapArray, 0, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "CSM Framebuffer not complete!" << std::endl;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        s_CSMDepthShader = Shader::Create("res/shaders/csm_depth.glsl");
    }

    void CSMPass::Shutdown() {
        if (s_FBO) { glDeleteFramebuffers(1, &s_FBO); s_FBO = 0; }
        if (s_ShadowMapArray) { glDeleteTextures(1, &s_ShadowMapArray); s_ShadowMapArray = 0; }
        if (s_CSMDepthShader) { delete s_CSMDepthShader; s_CSMDepthShader = nullptr; }
    }

    void CSMPass::Resize(uint32_t resolution) {
        Shutdown();
        Init(resolution);
    }

    void CSMPass::Execute(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& lightDir, float nearPlane, float farPlane, std::function<void(const glm::mat4& lightVP)> renderCallback) {
        ComputeCascadeSplits(nearPlane, farPlane);
        ComputeLightSpaceMatrices(viewMatrix, projMatrix, lightDir, nearPlane, farPlane);

        if (s_CSMDepthShader) {
            s_CSMDepthShader->Bind();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, s_FBO);
        glViewport(0, 0, s_Resolution, s_Resolution);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        for (int i = 0; i < 4; ++i) {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_ShadowMapArray, 0, i);
            glClear(GL_DEPTH_BUFFER_BIT);

            if (s_CSMDepthShader) {
                s_CSMDepthShader->SetMat4("u_LightSpaceMatrix", s_LightSpaceMatrices[i]);
            }

            renderCallback(s_LightSpaceMatrices[i]);
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t CSMPass::GetShadowMapArrayID() {
        return s_ShadowMapArray;
    }

    const glm::mat4* CSMPass::GetLightSpaceMatrices() {
        return s_LightSpaceMatrices;
    }

    const float* CSMPass::GetCascadeSplits() {
        return s_CascadeSplits;
    }

    void CSMPass::ComputeCascadeSplits(float nearPlane, float farPlane) {
        float lambda = 0.5f;
        for (int i = 0; i < 4; i++) {
            float p = (i + 1) / 4.0f;
            float log = nearPlane * std::pow(farPlane / nearPlane, p);
            float uniform = nearPlane + (farPlane - nearPlane) * p;
            s_CascadeSplits[i] = log * lambda + uniform * (1.0f - lambda);
        }
    }

    std::vector<glm::vec4> CSMPass::GetFrustumCornersWorldSpace(const glm::mat4& projMatrix, const glm::mat4& viewMatrix) {
        const auto inv = glm::inverse(projMatrix * viewMatrix);
        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    const glm::vec4 pt = inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }
        return frustumCorners;
    }
    
    std::vector<glm::vec4> CSMPass::GetFrustumCornersWorldSpace(const glm::mat4& projMatrix, const glm::mat4& viewMatrix, float nearZ, float farZ) {
        float ndcNear = (projMatrix[2][2] * -nearZ + projMatrix[3][2]) / (projMatrix[2][3] * -nearZ);
        float ndcFar = (projMatrix[2][2] * -farZ + projMatrix[3][2]) / (projMatrix[2][3] * -farZ);
        
        const auto inv = glm::inverse(projMatrix * viewMatrix);
        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    float z_val = (z == 0) ? ndcNear : ndcFar;
                    const glm::vec4 pt = inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        z_val,
                        1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }
        return frustumCorners;
    }

    void CSMPass::ComputeLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& lightDir, float nearPlane, float farPlane) {
        float lastSplitDist = nearPlane;
        for (int i = 0; i < 4; i++) {
            float splitDist = s_CascadeSplits[i];
            auto corners = GetFrustumCornersWorldSpace(projMatrix, viewMatrix, lastSplitDist, splitDist);
            
            glm::vec3 center = glm::vec3(0.0f);
            for (const auto& v : corners) {
                center += glm::vec3(v);
            }
            center /= corners.size();
            
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(up, glm::normalize(-lightDir))) > 0.999f) {
                up = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            const auto lightView = glm::lookAt(center - lightDir, center, up);
            
            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::lowest();
            
            for (const auto& v : corners) {
                const auto trf = lightView * v;
                minX = std::min(minX, trf.x);
                maxX = std::max(maxX, trf.x);
                minY = std::min(minY, trf.y);
                maxY = std::max(maxY, trf.y);
                minZ = std::min(minZ, trf.z);
                maxZ = std::max(maxZ, trf.z);
            }
            
            constexpr float zMult = 10.0f;
            if (minZ < 0) {
                minZ *= zMult;
            } else {
                minZ /= zMult;
            }
            if (maxZ < 0) {
                maxZ /= zMult;
            } else {
                maxZ *= zMult;
            }
            
            const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
            s_LightSpaceMatrices[i] = lightProjection * lightView;
            
            lastSplitDist = splitDist;
        }
    }
}
