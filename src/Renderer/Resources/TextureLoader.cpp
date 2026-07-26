#include "TextureLoader.h"
#include "../../Vendor/stb_image.h"
#include "../../Vendor/glad.h"
#include <spdlog/spdlog.h>

namespace lgt {

    class LoadedGLTexture : public Texture {
    public:
        LoadedGLTexture(uint32_t width, uint32_t height, uint32_t rendererID)
            : m_RendererID(rendererID), m_Width(width), m_Height(height) {
            m_BindlessHandle = glGetTextureHandleARB(m_RendererID);
            glMakeTextureHandleResidentARB(m_BindlessHandle);
        }

        virtual ~LoadedGLTexture() {
            if (m_BindlessHandle) {
                glMakeTextureHandleNonResidentARB(m_BindlessHandle);
            }
            glDeleteTextures(1, &m_RendererID);
        }

        virtual void Bind(uint32_t slot = 0) const override {
            glBindTextureUnit(slot, m_RendererID);
        }
        
        virtual void BindImage(uint32_t unit, uint32_t level, bool layered, uint32_t layer, TextureAccess access) const override {
            GLenum glAccess = GL_READ_ONLY;
            if (access == TextureAccess::WriteOnly) glAccess = GL_WRITE_ONLY;
            else if (access == TextureAccess::ReadWrite) glAccess = GL_READ_WRITE;
            // Assumes loaded textures are typical RGBA8 2D
            glBindImageTexture(unit, m_RendererID, level, layered ? GL_TRUE : GL_FALSE, layer, glAccess, GL_RGBA8);
        }

        virtual void Unbind() const override {
            glBindTextureUnit(0, 0);
        }

        virtual void SetData(void* data, uint32_t size) override {
            // Not supported for loaded textures
        }

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        virtual uint64_t GetBindlessHandle() const override { return m_BindlessHandle; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width;
        uint32_t m_Height;
        uint64_t m_BindlessHandle;
    };

    std::shared_ptr<Texture> TextureLoader::LoadFromFile(const std::string& path, bool sRGB) {
        stbi_set_flip_vertically_on_load(1);
        
        int width, height, channels;
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        
        if (!data) {
            spdlog::error("Failed to load texture: {}", path);
            return nullptr;
        }
        
        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4) {
            internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            dataFormat = GL_RGBA;
        } else if (channels == 3) {
            internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
            dataFormat = GL_RGB;
        } else if (channels == 1) {
            internalFormat = GL_R8;
            dataFormat = GL_RED;
        } else {
            spdlog::error("Unsupported number of channels in texture: {}", path);
            stbi_image_free(data);
            return nullptr;
        }
        
        uint32_t rendererID;
        glCreateTextures(GL_TEXTURE_2D, 1, &rendererID);
        
        // Calculate mip levels
        int mipLevels = 1;
        int maxDim = std::max(width, height);
        while (maxDim > 1) {
            maxDim /= 2;
            mipLevels++;
        }
        
        glTextureStorage2D(rendererID, mipLevels, internalFormat, width, height);
        glTextureSubImage2D(rendererID, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(rendererID);
        
        glTextureParameteri(rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        stbi_image_free(data);
        
        return std::make_shared<LoadedGLTexture>(width, height, rendererID);
    }
}
