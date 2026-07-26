#pragma once

#include <stdint.h>
#include <stddef.h>

namespace lgt {

    enum class BufferType {
        VertexBuffer = 0,
        IndexBuffer,
        UniformBuffer,
        ShaderStorageBuffer,
        DrawIndirectBuffer
    };

    enum class BufferUsage {
        StaticDraw = 0,
        DynamicDraw,
        StreamDraw
    };

    class Buffer {
    public:
        virtual ~Buffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual void BindBase(uint32_t index) const = 0;
        
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        
        virtual uint32_t GetSize() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        static Buffer* Create(BufferType type, uint32_t size, const void* data = nullptr, BufferUsage usage = BufferUsage::StaticDraw);
    };

}
