#include "RTShadowPass.h"
#include <glad.h>
#include "../Core/Shader.h"
#include <iostream>

namespace lgt {

    uint32_t RTShadowPass::s_Width = 0;
    uint32_t RTShadowPass::s_Height = 0;
    static Shader* s_RTShadowShader = nullptr;
    static Shader* s_ShadowBlurShader = nullptr;
    static uint32_t s_ShadowMaskTexture = 0;
    static uint32_t s_BlurredMaskTexture = 0;

    void RTShadowPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;

        s_RTShadowShader = Shader::Create("res/shaders/rt_shadow.comp");
        s_ShadowBlurShader = Shader::Create("res/shaders/shadow_blur.comp");

        glGenTextures(1, &s_ShadowMaskTexture);
        glBindTexture(GL_TEXTURE_2D, s_ShadowMaskTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenTextures(1, &s_BlurredMaskTexture);
        glBindTexture(GL_TEXTURE_2D, s_BlurredMaskTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void RTShadowPass::Shutdown() {
        if (s_RTShadowShader) { delete s_RTShadowShader; s_RTShadowShader = nullptr; }
        if (s_ShadowBlurShader) { delete s_ShadowBlurShader; s_ShadowBlurShader = nullptr; }
        if (s_ShadowMaskTexture) { glDeleteTextures(1, &s_ShadowMaskTexture); s_ShadowMaskTexture = 0; }
        if (s_BlurredMaskTexture) { glDeleteTextures(1, &s_BlurredMaskTexture); s_BlurredMaskTexture = 0; }
    }

    void RTShadowPass::Resize(uint32_t width, uint32_t height) {
        Shutdown();
        Init(width, height);
    }

    void RTShadowPass::Execute(uint32_t depthID, uint32_t normalID, const glm::mat4& invViewProj, const glm::vec3& cameraPos, const glm::vec3& lightDir) {
        if (!s_RTShadowShader || !s_ShadowBlurShader) return;

        // 1. Ray Trace Shadows
        s_RTShadowShader->Bind();
        glBindImageTexture(0, s_ShadowMaskTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthID);
        s_RTShadowShader->SetInt("u_gDepth", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalID);
        s_RTShadowShader->SetInt("u_gNormal", 2);

        s_RTShadowShader->SetMat4("u_InvViewProjection", invViewProj);
        s_RTShadowShader->SetFloat3("u_CameraPos", cameraPos);
        s_RTShadowShader->SetFloat3("u_LightDirection", lightDir);

        uint32_t numGroupsX = (s_Width + 7) / 8;
        uint32_t numGroupsY = (s_Height + 7) / 8;
        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // 2. Blur Shadows
        s_ShadowBlurShader->Bind();
        glBindImageTexture(0, s_BlurredMaskTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_ShadowMaskTexture);
        s_ShadowBlurShader->SetInt("u_InputMask", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthID);
        s_ShadowBlurShader->SetInt("u_gDepth", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalID);
        s_ShadowBlurShader->SetInt("u_gNormal", 2);

        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    uint32_t RTShadowPass::GetShadowMaskTextureID() {
        return s_BlurredMaskTexture;
    }
}
