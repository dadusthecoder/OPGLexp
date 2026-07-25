#pragma once
#include "RenderPass.h"

namespace lgt {

// ============================================================
// SkyboxPass — Renders a cubemap environment
// ============================================================
class SkyboxPass : public RenderPass {
public:
    SkyboxPass() = default;
    ~SkyboxPass() override = default;

    void Init(RenderContext& ctx) override {
        m_shader = new Pipeline("res/shaders/Skybox.shader");
        setupCube();
    }

    void Execute(RenderContext& ctx) override {
        if (!ctx.hasIBL || ctx.envCubemap == 0) return;
        if (!m_shader || !m_shader->isValid()) return;

        // Render to the currently bound FBO (usually the HDR FBO)
        glDepthFunc(GL_LEQUAL);  // Change depth function so depth test passes when values are equal to depth buffer's content
        glDepthMask(GL_FALSE);   // Disable depth writing

        m_shader->use();

        // Remove translation from the view matrix
        glm::mat4 view = glm::mat4(glm::mat3(ctx.view));
        m_shader->setMat4("u_View", view);
        m_shader->setMat4("u_Projection", ctx.projection);

        // Bind environment cubemap
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.envCubemap);
        m_shader->setInt("u_EnvironmentMap", 0);

        m_shader->setFloat("u_Exposure", ctx.hdr.exposure);
        m_shader->setFloat("u_Lod", ctx.skybox.lod);

        // Render cube
        glBindVertexArray(m_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // Restore default depth function
    }

    void Shutdown() override {
        delete m_shader;
        m_shader = nullptr;
        
        if (m_cubeVAO) {
            glDeleteVertexArrays(1, &m_cubeVAO);
            glDeleteBuffers(1, &m_cubeVBO);
            m_cubeVAO = 0;
            m_cubeVBO = 0;
        }
    }

    const char* GetName() const override { return "SkyboxPass"; }

private:
    Pipeline* m_shader = nullptr;
    GLuint m_cubeVAO = 0;
    GLuint m_cubeVBO = 0;

    void setupCube() {
        float vertices[] = {
            // positions          
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

        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glBindVertexArray(m_cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
};

} // namespace lgt
