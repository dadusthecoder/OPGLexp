#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include "Texture.h"
#include "Framebuffer.h"

namespace lgt {

    struct RenderGraphResource {
        std::string name;
        Texture* texture = nullptr;
        Framebuffer* fbo = nullptr;
    };

    struct RenderPass {
        std::string name;
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;
        std::function<void()> execute;
    };

    class RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddPass(const std::string& name, 
                     const std::vector<std::string>& inputs, 
                     const std::vector<std::string>& outputs, 
                     std::function<void()> executeCallback);

        void RegisterTexture(const std::string& name, Texture* texture);
        void RegisterFramebuffer(const std::string& name, Framebuffer* fbo);

        Texture* GetTexture(const std::string& name);
        Framebuffer* GetFramebuffer(const std::string& name);

        void Compile();
        void Execute();

    private:
        std::vector<RenderPass> m_Passes;
        std::unordered_map<std::string, RenderGraphResource> m_Resources;
        std::vector<RenderPass*> m_ExecutionOrder;
    };

}
