#include "OpenGLFramebuffer.h"
#include "../../Vendor/glad.h"
#include <iostream>

namespace lgt {

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferDescriptor& desc)
        : m_Width(desc.width), m_Height(desc.height), m_Descriptor(desc) {
        Invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer() {
        glDeleteFramebuffers(1, &m_RendererID);
        for (auto tex : m_ColorAttachments) delete tex;
        if (m_DepthAttachment) delete m_DepthAttachment;
    }

    void OpenGLFramebuffer::Invalidate() {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            for (auto tex : m_ColorAttachments) delete tex;
            if (m_DepthAttachment) delete m_DepthAttachment;
            m_ColorAttachments.clear();
            m_DepthAttachment = nullptr;
        }

        glCreateFramebuffers(1, &m_RendererID);

        std::vector<GLenum> drawBuffers;
        int colorAttachmentIndex = 0;

        for (auto& attachment : m_Descriptor.attachments) {
            TextureDescriptor texDesc;
            texDesc.width = m_Width;
            texDesc.height = m_Height;
            texDesc.format = attachment.format;
            texDesc.generateMipmaps = false;
            texDesc.minFilter = TextureFilter::Nearest;
            texDesc.magFilter = TextureFilter::Nearest;

            Texture* tex = Texture::Create(texDesc);

            if (attachment.isDepth) {
                GLenum attachmentType = (attachment.format == TextureFormat::Depth32F) ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
                glNamedFramebufferTexture(m_RendererID, attachmentType, tex->GetRendererID(), 0);
                m_DepthAttachment = tex;
            } else {
                glNamedFramebufferTexture(m_RendererID, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, tex->GetRendererID(), 0);
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + colorAttachmentIndex);
                m_ColorAttachments.push_back(tex);
                colorAttachmentIndex++;
            }
        } // Closing brace for the for loop

        if (m_ColorAttachments.empty()) {
            glNamedFramebufferDrawBuffer(m_RendererID, GL_NONE);
            glNamedFramebufferReadBuffer(m_RendererID, GL_NONE);
        } else {
            glNamedFramebufferDrawBuffers(m_RendererID, drawBuffers.size(), drawBuffers.data());
        }

        if (glCheckNamedFramebufferStatus(m_RendererID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer is incomplete!" << std::endl;
        }
    }

    void OpenGLFramebuffer::AttachColorTexture(Texture* texture, uint32_t attachmentIndex, uint32_t mipLevel, uint32_t layer) {
        if (!texture) return;
        glNamedFramebufferTextureLayer(m_RendererID, GL_COLOR_ATTACHMENT0 + attachmentIndex, texture->GetRendererID(), mipLevel, layer);
        if (attachmentIndex >= m_ColorAttachments.size()) {
            m_ColorAttachments.resize(attachmentIndex + 1, nullptr);
        }
        m_ColorAttachments[attachmentIndex] = texture;
    }

    void OpenGLFramebuffer::AttachDepthTexture(Texture* texture, uint32_t mipLevel, uint32_t layer) {
        if (!texture) return;
        // Simplified: assuming Depth24Stencil8 for now, but could inspect texture format if exposed
        glNamedFramebufferTextureLayer(m_RendererID, GL_DEPTH_STENCIL_ATTACHMENT, texture->GetRendererID(), mipLevel, layer);
        m_DepthAttachment = texture;
    }

    void OpenGLFramebuffer::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, m_Width, m_Height);
    }

    void OpenGLFramebuffer::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0 || (m_Width == width && m_Height == height)) return;
        m_Width = width;
        m_Height = height;
        Invalidate();
    }

    Texture* OpenGLFramebuffer::GetColorAttachment(uint32_t index) const {
        if (index < m_ColorAttachments.size()) return m_ColorAttachments[index];
        return nullptr;
    }

    Texture* OpenGLFramebuffer::GetDepthAttachment() const {
        return m_DepthAttachment;
    }

    Framebuffer* Framebuffer::Create(const FramebufferDescriptor& desc) {
        return new OpenGLFramebuffer(desc);
    }

}
