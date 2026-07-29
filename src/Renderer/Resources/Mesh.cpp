#include "Mesh.h"
#include "../../Vendor/glad.h"

namespace lgt {

    Mesh::Mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices, const std::vector<Meshlet>& meshlets, const VertexLayout& layout)
        : m_Layout(layout), m_Meshlets(meshlets), m_Vertices(vertices), m_Indices(indices)
    {
        m_VertexBuffer = Buffer::Create(BufferType::VertexBuffer, vertices.size() * sizeof(float), vertices.data());
        m_IndexBuffer = Buffer::Create(BufferType::IndexBuffer, indices.size() * sizeof(uint32_t), indices.data());
        m_IndexCount = static_cast<uint32_t>(indices.size());
        m_VertexCount = (layout.stride > 0) ? static_cast<uint32_t>((vertices.size() * sizeof(float)) / layout.stride) : 0;

        SetupVAO();
    }

    Mesh::~Mesh() {
        if (m_VAO != 0) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        delete m_VertexBuffer;
        delete m_IndexBuffer;
    }

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::None:     return 0;
            case ShaderDataType::Float:    return GL_FLOAT;
            case ShaderDataType::Float2:   return GL_FLOAT;
            case ShaderDataType::Float3:   return GL_FLOAT;
            case ShaderDataType::Float4:   return GL_FLOAT;
            case ShaderDataType::Int:      return GL_INT;
            case ShaderDataType::Int2:     return GL_INT;
            case ShaderDataType::Int3:     return GL_INT;
            case ShaderDataType::Int4:     return GL_INT;
            case ShaderDataType::UInt:     return GL_UNSIGNED_INT;
            case ShaderDataType::UInt2:    return GL_UNSIGNED_INT;
            case ShaderDataType::UInt3:    return GL_UNSIGNED_INT;
            case ShaderDataType::UInt4:    return GL_UNSIGNED_INT;
            case ShaderDataType::UByte4:   return GL_UNSIGNED_BYTE;
        }
        return 0;
    }

    void Mesh::SetupVAO() {
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        // Bind vertex buffer
        m_VertexBuffer->Bind();

        // Configure each attribute from the layout
        for (uint32_t i = 0; i < m_Layout.attributes.size(); i++) {
            const auto& attr = m_Layout.attributes[i];
            glEnableVertexAttribArray(i);
            
            GLenum glType = ShaderDataTypeToOpenGLBaseType(attr.type);
            
            if (glType == GL_INT || glType == GL_UNSIGNED_INT) {
                glVertexAttribIPointer(
                    i,
                    attr.componentCount,
                    glType,
                    m_Layout.stride,
                    (const void*)(uintptr_t)attr.offset
                );
            } else {
                glVertexAttribPointer(
                    i,                                  
                    attr.componentCount,                
                    glType,                           
                    attr.normalized ? GL_TRUE : GL_FALSE,
                    m_Layout.stride,                    
                    (const void*)(uintptr_t)attr.offset 
                );
            }
        }

        // Bind index buffer
        m_IndexBuffer->Bind();

        // Unbind VAO (leave buffers bound to VAO)
        glBindVertexArray(0);
    }

    void Mesh::Bind() const {
        glBindVertexArray(m_VAO);
    }

    void Mesh::Unbind() const {
        glBindVertexArray(0);
    }

}
