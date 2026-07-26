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
        }
        return 0;
    }

    OpenGLBuffer::OpenGLBuffer(BufferType type, uint32_t size, const void* data, BufferUsage usage)
        : m_Size(size) {
        m_GLTarget = BufferTypeToGL(type);
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(m_GLTarget, m_RendererID);
        glBufferData(m_GLTarget, size, data, BufferUsageToGL(usage));
    }

    OpenGLBuffer::~OpenGLBuffer() {
        glDeleteBuffers(1, &m_RendererID);
    }

    void OpenGLBuffer::Bind() const {
        glBindBuffer(m_GLTarget, m_RendererID);
    }

    void OpenGLBuffer::Unbind() const {
        glBindBuffer(m_GLTarget, 0);
    }

    void OpenGLBuffer::BindBase(uint32_t index) const {
        glBindBufferBase(m_GLTarget, index, m_RendererID);
    }

    void OpenGLBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        glBindBuffer(m_GLTarget, m_RendererID);
        glBufferSubData(m_GLTarget, offset, size, data);
    }

    Buffer* Buffer::Create(BufferType type, uint32_t size, const void* data, BufferUsage usage) {
        return new OpenGLBuffer(type, size, data, usage);
    }

}
