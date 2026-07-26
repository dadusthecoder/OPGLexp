#pragma once
#include "../Core/Framebuffer.h"

namespace lgt {

    class OpenGLFramebuffer : public Framebuffer {
    public:
        OpenGLFramebuffer(const FramebufferDescriptor& desc);
        virtual ~OpenGLFramebuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void Resize(uint32_t width, uint32_t height) override;

        virtual Texture* GetColorAttachment(uint32_t index = 0) const override;
        virtual Texture* GetDepthAttachment() const override;

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height;
        FramebufferDescriptor m_Descriptor;
        
        std::vector<Texture*> m_ColorAttachments;
        Texture* m_DepthAttachment = nullptr;

        void Invalidate();
    };

}
