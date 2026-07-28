#include "IBLPass.h"
#include "../../Vendor/glad.h"
#include "../../Vendor/stb_image.h"
#include "../Core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace lgt {

    Texture* IBLPass::s_EnvCubemap = nullptr;
    Texture* IBLPass::s_IrradianceMap = nullptr;
    Texture* IBLPass::s_PrefilterMap = nullptr;
    Texture* IBLPass::s_BrdfLut = nullptr;
    bool IBLPass::s_Ready = false;

    static GLuint s_CubeVAO = 0;
    static GLuint s_QuadVAO = 0;

    static void RenderCube() {
        if (s_CubeVAO == 0) {
            float vertices[] = {
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f
            };
            GLuint vbo;
            glGenVertexArrays(1, &s_CubeVAO);
            glGenBuffers(1, &vbo);
            glBindVertexArray(s_CubeVAO);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        }
        glBindVertexArray(s_CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    static void RenderQuad() {
        if (s_QuadVAO == 0) {
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
        }
        glBindVertexArray(s_QuadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void IBLPass::BakeFromEquirect(const std::string& hdrPath) {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float* data = stbi_loadf(hdrPath.c_str(), &width, &height, &nrComponents, 0);
        if (!data) return;

        GLuint hdrTexture;
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);

        // Env cubemap
        TextureDescriptor envDesc;
        envDesc.width = 512;
        envDesc.height = 512;
        envDesc.type = TextureType::TextureCube;
        envDesc.format = TextureFormat::RGBA16F;
        envDesc.wrapS = TextureWrap::ClampToEdge;
        envDesc.wrapT = TextureWrap::ClampToEdge;
        envDesc.wrapR = TextureWrap::ClampToEdge;
        envDesc.minFilter = TextureFilter::LinearMipmapLinear;
        envDesc.magFilter = TextureFilter::Linear;
        s_EnvCubemap = Texture::Create(envDesc);

        Shader* equirectShader = Shader::Create("res/shaders/ibl_equirect.glsl");
        equirectShader->Bind();
        equirectShader->SetInt("u_EquirectMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        RenderToCubemap(equirectShader, s_EnvCubemap, 512);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_EnvCubemap->GetRendererID());
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        delete equirectShader;
        glDeleteTextures(1, &hdrTexture);

        // Irradiance
        TextureDescriptor irrDesc = envDesc;
        irrDesc.width = 32;
        irrDesc.height = 32;
        irrDesc.generateMipmaps = false;
        irrDesc.minFilter = TextureFilter::Linear;
        s_IrradianceMap = Texture::Create(irrDesc);

        Shader* irrShader = Shader::Create("res/shaders/ibl_irradiance.glsl");
        irrShader->Bind();
        irrShader->SetInt("u_EnvMap", 0);
        s_EnvCubemap->Bind(0);
        RenderToCubemap(irrShader, s_IrradianceMap, 32);
        delete irrShader;

        // Prefilter
        TextureDescriptor preDesc = envDesc;
        preDesc.width = 128;
        preDesc.height = 128;
        preDesc.generateMipmaps = true;
        s_PrefilterMap = Texture::Create(preDesc);

        Shader* prefilterShader = Shader::Create("res/shaders/ibl_prefilter.glsl");
        prefilterShader->Bind();
        prefilterShader->SetInt("u_EnvMap", 0);
        s_EnvCubemap->Bind(0);

        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        int maxMipLevels = 5;
        for (int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth  = 128 * std::pow(0.5, mip);
            unsigned int mipHeight = 128 * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->SetFloat("u_Roughness", roughness);

            for (int i = 0; i < 6; ++i) {
                prefilterShader->SetMat4("u_ViewProjection", captureProjection * captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_PrefilterMap->GetRendererID(), mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderCube();
            }
        }
        delete prefilterShader;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        BakeBrdfLut();
        s_Ready = true;
    }

    void IBLPass::RenderToCubemap(Shader* shader, Texture* target, int size) {
        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        glViewport(0, 0, size, size);
        for (int i = 0; i < 6; ++i) {
            shader->SetMat4("u_ViewProjection", captureProjection * captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, target->GetRendererID(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    void IBLPass::BakeBrdfLut() {
        TextureDescriptor desc;
        desc.width = 512;
        desc.height = 512;
        desc.format = TextureFormat::RG16F;
        desc.wrapS = TextureWrap::ClampToEdge;
        desc.wrapT = TextureWrap::ClampToEdge;
        desc.minFilter = TextureFilter::Linear;
        desc.magFilter = TextureFilter::Linear;
        desc.generateMipmaps = false;
        s_BrdfLut = Texture::Create(desc);

        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_BrdfLut->GetRendererID(), 0);
        
        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Shader* lutShader = Shader::Create("res/shaders/ibl_brdf_lut.glsl");
        lutShader->Bind();
        RenderQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
        delete lutShader;
    }

    bool IBLPass::IsReady() { return s_Ready; }
    
    void IBLPass::Shutdown() {
        if (s_EnvCubemap) delete s_EnvCubemap; s_EnvCubemap = nullptr;
        if (s_IrradianceMap) delete s_IrradianceMap; s_IrradianceMap = nullptr;
        if (s_PrefilterMap) delete s_PrefilterMap; s_PrefilterMap = nullptr;
        if (s_BrdfLut) delete s_BrdfLut; s_BrdfLut = nullptr;
        s_Ready = false;
    }
    
    uint32_t IBLPass::GetIrradianceMapID() { return s_IrradianceMap ? s_IrradianceMap->GetRendererID() : 0; }
    uint32_t IBLPass::GetPrefilterMapID() { return s_PrefilterMap ? s_PrefilterMap->GetRendererID() : 0; }
    uint32_t IBLPass::GetBrdfLutID() { return s_BrdfLut ? s_BrdfLut->GetRendererID() : 0; }
}
