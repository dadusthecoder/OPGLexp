#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "../Core/Buffer.h"

#include "../Core/GPUData.h"

namespace lgt {

    // Describes a single vertex attribute in a vertex layout
    struct VertexAttribute {
        std::string name;
        uint32_t componentCount; // e.g., 3 for vec3, 2 for vec2
        uint32_t offset;         // byte offset within one vertex
        bool normalized = false;
    };

    // Describes the full vertex layout (ordered list of attributes)
    struct VertexLayout {
        std::vector<VertexAttribute> attributes;
        uint32_t stride = 0; // total bytes per vertex

        void Push(const std::string& name, uint32_t count) {
            uint32_t offset = stride;
            attributes.push_back({ name, count, offset, false });
            stride += count * sizeof(float);
        }

        // Standard PBR layout: Position(3) + Normal(3) + TexCoord(2)
        static VertexLayout PBR() {
            VertexLayout layout;
            layout.Push("a_Position", 3);
            layout.Push("a_Normal", 3);
            layout.Push("a_TexCoord", 2);
            return layout;
        }

        // Simple layout: Position(3) only
        static VertexLayout PositionOnly() {
            VertexLayout layout;
            layout.Push("a_Position", 3);
            return layout;
        }
    };

    class Mesh {
    public:
        Mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices, 
             const std::vector<Meshlet>& meshlets,
             const VertexLayout& layout = VertexLayout::PBR());
        ~Mesh();

        void Bind() const;
        void Unbind() const;

        Buffer* GetVertexBuffer() const { return m_VertexBuffer; }
        Buffer* GetIndexBuffer() const { return m_IndexBuffer; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }
        const VertexLayout& GetLayout() const { return m_Layout; }
        const std::vector<Meshlet>& GetMeshlets() const { return m_Meshlets; }
        const std::vector<float>& GetVertices() const { return m_Vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

    private:
        uint32_t m_VAO = 0;
        Buffer* m_VertexBuffer = nullptr;
        Buffer* m_IndexBuffer = nullptr;
        uint32_t m_IndexCount = 0;
        uint32_t m_VertexCount = 0;
        VertexLayout m_Layout;
        std::vector<Meshlet> m_Meshlets;
        std::vector<float> m_Vertices;
        std::vector<uint32_t> m_Indices;

        void SetupVAO();
    };

}
