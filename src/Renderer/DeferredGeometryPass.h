#pragma once
#include "RenderPass.h"
#include "Camera.h"
#include "Scene.h"

namespace lgt {

class DeferredGeometryPass : public RenderPass {
public:
    DeferredGeometryPass() = default;
    ~DeferredGeometryPass() override = default;

    void Init(RenderContext& ctx) override {
        m_shader = new Pipeline("res/shaders/DeferredGeometry.shader");
    }

    void Execute(RenderContext& ctx) override {
        if (!m_shader || !m_shader->isValid()) return;
        if (!ctx.scene || !ctx.camera) return;

        glBindFramebuffer(GL_FRAMEBUFFER, ctx.gBufferFBO);
        glViewport(0, 0, ctx.screenWidth, ctx.screenHeight);

        // Clear G-Buffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        m_shader->use();
        m_shader->setMat4("u_View", ctx.view);
        m_shader->setMat4("u_Projection", ctx.projection);
        m_shader->setMat4("u_PrevViewProj", ctx.prevProjection * ctx.prevView);

        // Bind Material SSBO is handled globally, but we can do it here if needed
        
        // Render all objects
        auto& rootNodes = ctx.scene->getRootNodes();
        for (auto& node : rootNodes) {
            renderNode(node.get(), m_shader);
        }
    }

    void Shutdown() override {
        delete m_shader;
        m_shader = nullptr;
    }

    const char* GetName() const override { return "DeferredGeometryPass"; }

private:
    Pipeline* m_shader = nullptr;

    void renderNode(SceneNode* node, Pipeline* shader) {
        if (!node) return;
        
        shader->setMat4("u_Model", node->globalTransform);
        for (auto& mesh : node->meshes) {
            shader->setInt("u_MaterialIndex", mesh.materialIndex);
            
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
        }
        for (auto& child : node->children) {
            renderNode(child.get(), shader);
        }
    }
};

} // namespace lgt
