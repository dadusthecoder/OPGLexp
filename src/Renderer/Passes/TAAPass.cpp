#include "TAAPass.h"
#include "../../Vendor/glad.h"
#include "../Core/Renderer.h"

namespace lgt {

    Shader*     TAAPass::s_TAAShader = nullptr;
    Framebuffer* TAAPass::s_HistoryFBOs[2] = {nullptr, nullptr};
    int         TAAPass::s_CurrentHistory = 0;
    uint32_t    TAAPass::s_Width = 0;
    uint32_t    TAAPass::s_Height = 0;
    uint32_t      TAAPass::s_QuadVAO = 0;

    static float Halton(int index, int base) {
        float f = 1.0f;
        float r = 0.0f;
        while (index > 0) {
            f /= base;
            r += f * (index % base);
            index /= base;
        }
        return r;
    }

    void TAAPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;
        s_TAAShader = Shader::Create("res/shaders/taa.glsl");

        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
        GLuint vbo;
        glGenVertexArrays(1, &s_QuadVAO);
        glGenBuffers(1, &vbo);
        glBindVertexArray(s_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        Resize(width, height);
    }

    glm::vec2 TAAPass::GetJitter(int frameIndex) {
        // 16x Halton sequence
        int i = (frameIndex % 16) + 1;
        float x = Halton(i, 2) * 2.0f - 1.0f;
        float y = Halton(i, 3) * 2.0f - 1.0f;
        
        // Return jitter in NDC space
        return glm::vec2(x / (float)s_Width, y / (float)s_Height);
    }

    void TAAPass::Resize(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;

        if (s_HistoryFBOs[0]) delete s_HistoryFBOs[0];
        if (s_HistoryFBOs[1]) delete s_HistoryFBOs[1];

        FramebufferDescriptor desc;
        desc.width = width;
        desc.height = height;
        desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));

        s_HistoryFBOs[0] = Framebuffer::Create(desc);
        s_HistoryFBOs[1] = Framebuffer::Create(desc);

        // Clear history
        for (int i = 0; i < 2; i++) {
            s_HistoryFBOs[i]->Bind();
            glClearColor(0,0,0,0);
            glClear(GL_COLOR_BUFFER_BIT);
            s_HistoryFBOs[i]->Unbind();
        }
    }

    void TAAPass::Execute(uint32_t currentColorID, uint32_t velocityID, uint32_t depthID, int frameIndex) {
        int nextHistory = 1 - s_CurrentHistory;
        
        s_HistoryFBOs[nextHistory]->Bind();
        glViewport(0, 0, s_Width, s_Height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        s_TAAShader->Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentColorID);
        s_TAAShader->SetInt("u_CurrentColor", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, s_HistoryFBOs[s_CurrentHistory]->GetColorAttachment(0)->GetRendererID());
        s_TAAShader->SetInt("u_HistoryColor", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, velocityID);
        s_TAAShader->SetInt("u_Velocity", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthID);
        s_TAAShader->SetInt("u_Depth", 3);

        glBindVertexArray(s_QuadVAO); // Can be empty VAO
        glDrawArrays(GL_TRIANGLES, 0, 3);

        s_HistoryFBOs[nextHistory]->Unbind();
        s_CurrentHistory = nextHistory;
    }

    uint32_t TAAPass::GetResolvedTextureID() {
        return s_HistoryFBOs[s_CurrentHistory] ? s_HistoryFBOs[s_CurrentHistory]->GetColorAttachment(0)->GetRendererID() : 0;
    }

    void TAAPass::Shutdown() {
        if (s_HistoryFBOs[0]) delete s_HistoryFBOs[0];
        if (s_HistoryFBOs[1]) delete s_HistoryFBOs[1];
        s_HistoryFBOs[0] = nullptr;
        s_HistoryFBOs[1] = nullptr;
        delete s_TAAShader; s_TAAShader = nullptr;
    }
}
