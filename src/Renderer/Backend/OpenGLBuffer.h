#pragma once
#include "../Core/Buffer.h"

namespace lgt {

    class OpenGLBuffer : public Buffer {
    public:
        OpenGLBuffer(BufferType type, uint32_t size, const void* data, BufferUsage usage);
        virtual ~OpenGLBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual void BindBase(uint32_t index) const override;
        
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        
        virtual void* Map() override;
        virtual void Unmap() override;
        virtual void* GetMappedPointer() const override { return m_MappedPointer; }
        
        virtual uint32_t GetSize() const override { return m_Size; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Size;
        uint32_t m_GLTarget;
        void* m_MappedPointer = nullptr;
    };

}
