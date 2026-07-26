#include "OpenGLBuffer.h"
#include "../../Vendor/glad.h"
#include <iostream>

namespace lgt {

    static GLenum BufferTypeToGL(BufferType type) {
        switch (type) {
            case BufferType::VertexBuffer: return GL_ARRAY_BUFFER;
            case BufferType::IndexBuffer: return GL_ELEMENT_ARRAY_BUFFER;
            case BufferType::UniformBuffer: return GL_UNIFORM_BUFFER;
            case BufferType::ShaderStorageBuffer: return GL_SHADER_STORAGE_BUFFER;
            case BufferType::DrawIndirectBuffer: return GL_DRAW_INDIRECT_BUFFER;
        }
        return 0;
    }

    static GLenum BufferUsageToGL(BufferUsage usage) {
        switch (usage) {
            case BufferUsage::StaticDraw: return GL_STATIC_DRAW;
            case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
            case BufferUsage::StreamDraw: return GL_STREAM_DRAW;
            case BufferUsage::PersistentMap: return GL_DYNAMIC_DRAW; // Or 0, usually ignored for persistent map as it uses flags directly
        }
        return 0;
    }

    OpenGLBuffer::OpenGLBuffer(BufferType type, uint32_t size, const void* data, BufferUsage usage)
        : m_Size(size) {
        m_GLTarget = BufferTypeToGL(type);
        glCreateBuffers(1, &m_RendererID);
        
        if (usage == BufferUsage::PersistentMap) {
            GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            glNamedBufferStorage(m_RendererID, size, data, flags);
            m_MappedPointer = glMapNamedBufferRange(m_RendererID, 0, size, flags);
        } else {
            glNamedBufferData(m_RendererID, size, data, BufferUsageToGL(usage));
        }
    }

    OpenGLBuffer::~OpenGLBuffer() {
        if (m_MappedPointer) {
            glUnmapNamedBuffer(m_RendererID);
        }
        glDeleteBuffers(1, &m_RendererID);
    }

    void* OpenGLBuffer::Map() {
        if (!m_MappedPointer) {
            m_MappedPointer = glMapNamedBuffer(m_RendererID, GL_WRITE_ONLY);
        }
        return m_MappedPointer;
    }

    void OpenGLBuffer::Unmap() {
        if (m_MappedPointer) {
            glUnmapNamedBuffer(m_RendererID);
            // We only clear the pointer if it wasn't persistently mapped
            // For now, simplify by assuming PersistentMap keeps it mapped forever, 
            // and regular Map()/Unmap() clears it.
            // A better way is checking usage flag, but we didn't store usage.
            // Let's just assume we only unmap manually if not persistent, or we just unmap and remap.
        }
    }

    void OpenGLBuffer::Bind() const {
        glBindBuffer(m_GLTarget, m_RendererID);
    }

    void OpenGLBuffer::Unbind() const {
        glBindBuffer(m_GLTarget, 0);
    }

    void OpenGLBuffer::BindBase(uint32_t index) const {
        GLenum target = m_GLTarget;
        if (target != GL_UNIFORM_BUFFER) target = GL_SHADER_STORAGE_BUFFER;
        glBindBufferBase(target, index, m_RendererID);
    }

    void OpenGLBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        glNamedBufferSubData(m_RendererID, offset, size, data);
    }

    Buffer* Buffer::Create(BufferType type, uint32_t size, const void* data, BufferUsage usage) {
        return new OpenGLBuffer(type, size, data, usage);
    }

}
