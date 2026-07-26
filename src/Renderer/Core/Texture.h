#pragma once

#include <stdint.h>
#include <string>

namespace lgt {

    enum class TextureFormat {
        None = 0,
        R8,
        RGB8,
        RGBA8,
        RGBA16F,
        RGBA32F,
        RG16F,
        RG32F,
        Depth32F,
        Depth24Stencil8,
        SRGB8,
        SRGB8_ALPHA8
    };

    enum class TextureType {
        Texture2D = 0,
        TextureCube,
        Texture3D,
        Texture2DArray
    };

    enum class TextureWrap {
        Repeat = 0,
        ClampToEdge,
        ClampToBorder,
        MirroredRepeat
    };

    enum class TextureFilter {
        Nearest = 0,
        Linear,
        LinearMipmapLinear,
        LinearMipmapNearest,
        NearestMipmapLinear,
        NearestMipmapNearest
    };
    
    enum class TextureAccess {
        ReadOnly = 0,
        WriteOnly,
        ReadWrite
    };

    struct TextureDescriptor {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1; // used for 3D textures or array layers
        TextureFormat format = TextureFormat::RGBA8;
        TextureType type = TextureType::Texture2D;
        TextureWrap wrapS = TextureWrap::Repeat;
        TextureWrap wrapT = TextureWrap::Repeat;
        TextureWrap wrapR = TextureWrap::Repeat;
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        bool generateMipmaps = true;
        std::string name = "Texture";
    };

    class Texture {
    public:
        virtual ~Texture() = default;

        virtual void Bind(uint32_t slot = 0) const = 0;
        virtual void BindImage(uint32_t unit, uint32_t level, bool layered, uint32_t layer, TextureAccess access) const = 0;
        virtual void Unbind() const = 0;
        virtual void SetData(void* data, uint32_t size) = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetRendererID() const = 0;
        virtual uint64_t GetBindlessHandle() const = 0;

        // Static factory
        static Texture* Create(const TextureDescriptor& desc);
        static Texture* Create(const std::string& path, bool srgb = false);
    };

}
