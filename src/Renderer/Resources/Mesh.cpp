#include "Mesh.h"
#include "../../Vendor/glad.h"

namespace lgt {

    Mesh::Mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices, const std::vector<Meshlet>& meshlets, const VertexLayout& layout)
        : m_Layout(layout), m_Meshlets(meshlets)
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

    void Mesh::SetupVAO() {
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        // Bind vertex buffer
        m_VertexBuffer->Bind();

        // Configure each attribute from the layout
        for (uint32_t i = 0; i < m_Layout.attributes.size(); i++) {
            const auto& attr = m_Layout.attributes[i];
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(
                i,                                  // location
                attr.componentCount,                // component count (e.g., 3 for vec3)
                GL_FLOAT,                           // type
                attr.normalized ? GL_TRUE : GL_FALSE,
                m_Layout.stride,                    // stride (bytes per vertex)
                (const void*)(uintptr_t)attr.offset // offset within vertex
            );
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
