#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include "Texture.h"

namespace lgt {

    struct FramebufferAttachment {
        TextureFormat format = TextureFormat::None;
        bool isDepth = false;
        
        FramebufferAttachment() = default;
        FramebufferAttachment(TextureFormat f) : format(f) {
            isDepth = (f == TextureFormat::Depth32F || f == TextureFormat::Depth24Stencil8);
        }
    };

    struct FramebufferDescriptor {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<FramebufferAttachment> attachments;
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual void AttachColorTexture(Texture* texture, uint32_t attachmentIndex, uint32_t mipLevel = 0, uint32_t layer = 0) = 0;
        virtual void AttachDepthTexture(Texture* texture, uint32_t mipLevel = 0, uint32_t layer = 0) = 0;

        virtual Texture* GetColorAttachment(uint32_t index = 0) const = 0;
        virtual Texture* GetDepthAttachment() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        static Framebuffer* Create(const FramebufferDescriptor& desc);
    };

}
