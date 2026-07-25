#pragma once
#include "RenderPass.h"

namespace lgt {

class LightCullingPass : public RenderPass {
public:
    LightCullingPass() = default;
    ~LightCullingPass() override = default;

    void Init(RenderContext& ctx) override {
        m_computeShader = new Pipeline("res/shaders/LightCulling.comp");
    }

    void Execute(RenderContext& ctx) override {
        if (!m_computeShader || !m_computeShader->isValid()) return;

        m_computeShader->use();
        
        // Use gDepth from the G-Buffer instead of a separate depth prepass
        m_computeShader->setInt("depthMap", 4);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
        
        m_computeShader->setVec2("screenSize", static_cast<float>(ctx.screenWidth), static_cast<float>(ctx.screenHeight));
        m_computeShader->setInt("lightCount", ctx.scene ? ctx.scene->getLights().size() : 0);
        m_computeShader->setFloat("u_zNear", ctx.nearPlane);
        m_computeShader->setFloat("u_zFar", ctx.farPlane);

        // Required matrices for computing frustums
        m_computeShader->setMat4("view", ctx.view);
        m_computeShader->setMat4("projection", ctx.projection);
        
        m_computeShader->dispatch(ctx.workGroupsX, ctx.workGroupsY, 1);
        
        // Ensure compute writes are visible to fragment shader in lighting pass
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void Shutdown() override {
        delete m_computeShader;
        m_computeShader = nullptr;
    }

    const char* GetName() const override { return "LightCullingPass"; }

private:
    Pipeline* m_computeShader = nullptr;
};

} // namespace lgt
