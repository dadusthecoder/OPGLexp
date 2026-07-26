#include "RenderGraph.h"
#include <iostream>

namespace lgt {

    void RenderGraph::AddPass(const std::string& name, 
                              const std::vector<std::string>& inputs, 
                              const std::vector<std::string>& outputs, 
                              std::function<void()> executeCallback) {
        RenderPass pass;
        pass.name = name;
        pass.inputs = inputs;
        pass.outputs = outputs;
        pass.execute = executeCallback;
        m_Passes.push_back(pass);
    }

    void RenderGraph::RegisterTexture(const std::string& name, Texture* texture) {
        m_Resources[name] = { name, texture, nullptr };
    }

    void RenderGraph::RegisterFramebuffer(const std::string& name, Framebuffer* fbo) {
        m_Resources[name] = { name, nullptr, fbo };
    }

    Texture* RenderGraph::GetTexture(const std::string& name) {
        if (m_Resources.find(name) != m_Resources.end()) {
            return m_Resources[name].texture;
        }
        return nullptr;
    }

    Framebuffer* RenderGraph::GetFramebuffer(const std::string& name) {
        if (m_Resources.find(name) != m_Resources.end()) {
            return m_Resources[name].fbo;
        }
        return nullptr;
    }

    void RenderGraph::Compile() {
        // Simplified compilation: just executes in the order they were added for now.
        // A real implementation would build a directed acyclic graph (DAG) based on inputs/outputs,
        // prune unused passes, and allocate transient resources.
        m_ExecutionOrder.clear();
        for (auto& pass : m_Passes) {
            m_ExecutionOrder.push_back(&pass);
        }
    }

    void RenderGraph::Execute() {
        for (auto* pass : m_ExecutionOrder) {
            if (pass->execute) {
                pass->execute();
            }
        }
    }

}
