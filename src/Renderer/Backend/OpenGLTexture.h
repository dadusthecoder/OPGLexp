#pragma once
#include "../Core/Texture.h"

namespace lgt {

    class OpenGLTexture : public Texture {
    public:
        OpenGLTexture(const TextureDescriptor& desc);
        virtual ~OpenGLTexture();

        virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;
        virtual void SetData(void* data, uint32_t size) override;

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        virtual uint64_t GetBindlessHandle() const override { return m_BindlessHandle; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height;
        uint32_t m_InternalFormat, m_DataFormat;
        uint64_t m_BindlessHandle;
    };

}
