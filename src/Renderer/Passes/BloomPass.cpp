#include "BloomPass.h"
#include "../../Vendor/glad.h"

namespace lgt {

    Shader* BloomPass::s_DownsampleShader = nullptr;
    Shader* BloomPass::s_UpsampleShader = nullptr;
    std::vector<Framebuffer*> BloomPass::s_MipFBOs;
    uint32_t BloomPass::s_Width = 0;
    uint32_t BloomPass::s_Height = 0;
    uint32_t BloomPass::s_QuadVAO = 0;

    void BloomPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;
        s_DownsampleShader = Shader::Create("res/shaders/bloom_downsample.glsl");
        s_UpsampleShader = Shader::Create("res/shaders/bloom_upsample.glsl");

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

    void BloomPass::Resize(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;

        for (auto fbo : s_MipFBOs) {
            delete fbo;
        }
        s_MipFBOs.clear();

        glm::vec2 mipSize((float)width, (float)height);
        for (int i = 0; i < NUM_MIPS; i++) {
            mipSize *= 0.5f;
            mipSize.x = std::max(1.0f, mipSize.x);
            mipSize.y = std::max(1.0f, mipSize.y);

            FramebufferDescriptor desc;
            desc.width = (uint32_t)mipSize.x;
            desc.height = (uint32_t)mipSize.y;
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));
            
            s_MipFBOs.push_back(Framebuffer::Create(desc));
        }
    }

    void BloomPass::Execute(uint32_t hdrTextureID, float threshold, float strength) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        s_DownsampleShader->Bind();
        s_DownsampleShader->SetInt("u_Source", 0);
        s_DownsampleShader->SetFloat("u_Threshold", threshold);
        
        uint32_t srcTex = hdrTextureID;
        uint32_t currentWidth = s_Width;
        uint32_t currentHeight = s_Height;

        for (int i = 0; i < NUM_MIPS; i++) {
            s_MipFBOs[i]->Bind();
            glViewport(0, 0, s_MipFBOs[i]->GetWidth(), s_MipFBOs[i]->GetHeight());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, srcTex);
            
            s_DownsampleShader->SetInt("u_MipLevel", i);
            s_DownsampleShader->SetFloat2("u_SrcTexelSize", glm::vec2(1.0f / (float)currentWidth, 1.0f / (float)currentHeight));
            
            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            srcTex = s_MipFBOs[i]->GetColorAttachment(0)->GetRendererID();
            currentWidth = s_MipFBOs[i]->GetWidth();
            currentHeight = s_MipFBOs[i]->GetHeight();
        }

        // Upsample
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        s_UpsampleShader->Bind();
        s_UpsampleShader->SetInt("u_Source", 0);
        s_UpsampleShader->SetFloat("u_Strength", strength);

        for (int i = NUM_MIPS - 1; i > 0; i--) {
            s_MipFBOs[i - 1]->Bind();
            glViewport(0, 0, s_MipFBOs[i - 1]->GetWidth(), s_MipFBOs[i - 1]->GetHeight());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s_MipFBOs[i]->GetColorAttachment(0)->GetRendererID());
            
            s_UpsampleShader->SetFloat2("u_FilterRadius", glm::vec2(0.005f, 0.005f));

            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t BloomPass::GetBloomTextureID() {
        return s_MipFBOs.empty() ? 0 : s_MipFBOs[0]->GetColorAttachment(0)->GetRendererID();
    }

    void BloomPass::Shutdown() {
        for (auto fbo : s_MipFBOs) {
            delete fbo;
        }
        s_MipFBOs.clear();
        delete s_DownsampleShader; s_DownsampleShader = nullptr;
        delete s_UpsampleShader; s_UpsampleShader = nullptr;
    }
}
