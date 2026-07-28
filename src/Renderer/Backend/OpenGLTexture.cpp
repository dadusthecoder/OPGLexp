#include "OpenGLTexture.h"
#include "../../Vendor/glad.h"
#include <iostream>

namespace lgt {

    static void GetGLFormats(TextureFormat format, GLenum& internalFormat, GLenum& dataFormat, GLenum& dataType) {
        switch (format) {
            case TextureFormat::R8: internalFormat = GL_R8; dataFormat = GL_RED; dataType = GL_UNSIGNED_BYTE; break;
            case TextureFormat::RGB8: internalFormat = GL_RGB8; dataFormat = GL_RGB; dataType = GL_UNSIGNED_BYTE; break;
            case TextureFormat::RGBA8: internalFormat = GL_RGBA8; dataFormat = GL_RGBA; dataType = GL_UNSIGNED_BYTE; break;
            case TextureFormat::RGBA16F: internalFormat = GL_RGBA16F; dataFormat = GL_RGBA; dataType = GL_FLOAT; break;
            case TextureFormat::RGBA32F: internalFormat = GL_RGBA32F; dataFormat = GL_RGBA; dataType = GL_FLOAT; break;
            case TextureFormat::RG16F: internalFormat = GL_RG16F; dataFormat = GL_RG; dataType = GL_FLOAT; break;
            case TextureFormat::RG32F: internalFormat = GL_RG32F; dataFormat = GL_RG; dataType = GL_FLOAT; break;
            case TextureFormat::Depth32F: internalFormat = GL_DEPTH_COMPONENT32F; dataFormat = GL_DEPTH_COMPONENT; dataType = GL_FLOAT; break;
            case TextureFormat::Depth24Stencil8: internalFormat = GL_DEPTH24_STENCIL8; dataFormat = GL_DEPTH_STENCIL; dataType = GL_UNSIGNED_INT_24_8; break;
            case TextureFormat::SRGB8: internalFormat = GL_SRGB8; dataFormat = GL_RGB; dataType = GL_UNSIGNED_BYTE; break;
            case TextureFormat::SRGB8_ALPHA8: internalFormat = GL_SRGB8_ALPHA8; dataFormat = GL_RGBA; dataType = GL_UNSIGNED_BYTE; break;
            default: internalFormat = 0; dataFormat = 0; dataType = 0; break;
        }
    }

    static GLenum GetGLTextureTarget(TextureType type) {
        switch (type) {
            case TextureType::Texture2D: return GL_TEXTURE_2D;
            case TextureType::TextureCube: return GL_TEXTURE_CUBE_MAP;
            case TextureType::Texture3D: return GL_TEXTURE_3D;
            case TextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
            default: return GL_TEXTURE_2D;
        }
    }

    OpenGLTexture::OpenGLTexture(const TextureDescriptor& desc)
        : m_Width(desc.width), m_Height(desc.height), m_Depth(desc.depth), m_Type(desc.type), m_BindlessHandle(0) {
        
        GLenum dataType;
        GetGLFormats(desc.format, m_InternalFormat, m_DataFormat, dataType);

        GLenum target = GetGLTextureTarget(m_Type);
        glCreateTextures(target, 1, &m_RendererID);
        
        if (m_Type == TextureType::Texture2D || m_Type == TextureType::TextureCube) {
            glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);
        } else if (m_Type == TextureType::Texture3D || m_Type == TextureType::Texture2DArray) {
            glTextureStorage3D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height, m_Depth);
        }

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (m_Type == TextureType::TextureCube || m_Type == TextureType::Texture3D) {
            glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_REPEAT);
        }

        // Create bindless handle
        m_BindlessHandle = glGetTextureHandleARB(m_RendererID);
        glMakeTextureHandleResidentARB(m_BindlessHandle);
    }

    OpenGLTexture::~OpenGLTexture() {
        if (m_BindlessHandle) {
            glMakeTextureHandleNonResidentARB(m_BindlessHandle);
        }
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTexture::Bind(uint32_t slot) const {
        glBindTextureUnit(slot, m_RendererID);
    }

    void OpenGLTexture::BindImage(uint32_t unit, uint32_t level, bool layered, uint32_t layer, TextureAccess access) const {
        GLenum glAccess = GL_READ_ONLY;
        if (access == TextureAccess::WriteOnly) glAccess = GL_WRITE_ONLY;
        else if (access == TextureAccess::ReadWrite) glAccess = GL_READ_WRITE;
        glBindImageTexture(unit, m_RendererID, level, layered ? GL_TRUE : GL_FALSE, layer, glAccess, m_InternalFormat);
    }

    void OpenGLTexture::Unbind() const {
        glBindTextureUnit(0, 0); 
    }

    void OpenGLTexture::SetData(void* data, uint32_t size) {
        if (m_Type == TextureType::Texture2D || m_Type == TextureType::TextureCube) {
            glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
        } else {
            glTextureSubImage3D(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, m_Depth, m_DataFormat, GL_UNSIGNED_BYTE, data);
        }
    }

    Texture* Texture::Create(const TextureDescriptor& desc) {
        return new OpenGLTexture(desc);
    }

    Texture* Texture::Create(const std::string& path, bool srgb) {
        // Dummy implementation for now
        TextureDescriptor desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
        return new OpenGLTexture(desc);
    }

}
