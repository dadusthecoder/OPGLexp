#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "../Core/Buffer.h"

#include "../Core/GPUData.h"

namespace lgt {

    enum class ShaderDataType {
        None = 0,
        Float, Float2, Float3, Float4,
        Int, Int2, Int3, Int4,
        UInt, UInt2, UInt3, UInt4,
        UByte4 // For normalized bone weights
    };

    static uint32_t ShaderDataTypeSize(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::None:     return 0;
            case ShaderDataType::Float:    return 4;
            case ShaderDataType::Float2:   return 4 * 2;
            case ShaderDataType::Float3:   return 4 * 3;
            case ShaderDataType::Float4:   return 4 * 4;
            case ShaderDataType::Int:      return 4;
            case ShaderDataType::Int2:     return 4 * 2;
            case ShaderDataType::Int3:     return 4 * 3;
            case ShaderDataType::Int4:     return 4 * 4;
            case ShaderDataType::UInt:     return 4;
            case ShaderDataType::UInt2:    return 4 * 2;
            case ShaderDataType::UInt3:    return 4 * 3;
            case ShaderDataType::UInt4:    return 4 * 4;
            case ShaderDataType::UByte4:   return 1 * 4;
        }
        return 0;
    }

    // Describes a single vertex attribute in a vertex layout
    struct VertexAttribute {
        std::string name;
        ShaderDataType type;
        uint32_t componentCount; // e.g., 3 for vec3, 2 for vec2
        uint32_t offset;         // byte offset within one vertex
        bool normalized = false;
        
        VertexAttribute(const std::string& name, ShaderDataType type, uint32_t count, uint32_t offset, bool normalized)
            : name(name), type(type), componentCount(count), offset(offset), normalized(normalized) {}
    };

    // Describes the full vertex layout (ordered list of attributes)
    struct VertexLayout {
        std::vector<VertexAttribute> attributes;
        uint32_t stride = 0; // total bytes per vertex

        void Push(const std::string& name, ShaderDataType type, uint32_t count, bool normalized = false) {
            uint32_t offset = stride;
            attributes.push_back({ name, type, count, offset, normalized });
            stride += ShaderDataTypeSize(type);
        }

        // Standard PBR layout: Position(3) + Normal(3) + TexCoord(2)
        static VertexLayout PBR() {
            VertexLayout layout;
            layout.Push("a_Position", ShaderDataType::Float3, 3);
            layout.Push("a_Normal", ShaderDataType::Float3, 3);
            layout.Push("a_TexCoord", ShaderDataType::Float2, 2);
            return layout;
        }

        // Animated PBR layout: Position(3) + Normal(3) + TexCoord(2) + BoneIDs(uvec4) + BoneWeights(normalized ubyte4)
        static VertexLayout SkinnedPBR() {
            VertexLayout layout;
            layout.Push("a_Position", ShaderDataType::Float3, 3);
            layout.Push("a_Normal", ShaderDataType::Float3, 3);
            layout.Push("a_TexCoord", ShaderDataType::Float2, 2);
            layout.Push("a_BoneIDs", ShaderDataType::UInt4, 4);
            layout.Push("a_BoneWeights", ShaderDataType::UByte4, 4, true); // Normalized unsigned byte
            return layout;
        }

        // Simple layout: Position(3) only
        static VertexLayout PositionOnly() {
            VertexLayout layout;
            layout.Push("a_Position", ShaderDataType::Float3, 3);
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
