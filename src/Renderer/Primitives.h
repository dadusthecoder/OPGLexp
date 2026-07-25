#pragma once
#include <vector>
#include <cmath>
#include "renderer.h"

namespace lgt {
namespace Primitives {

inline Mesh CreatePlaneMesh(const std::string& name, float width, float depth, uint32_t matIdx) {
    std::vector<Vertex> vertices = {
        { {-width, 0.0f, -depth}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { {-width, 0.0f,  depth}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { width, 0.0f,  depth}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { width, 0.0f, -depth}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f} }
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
    Mesh mesh;
    mesh.setup(name, vertices, indices, matIdx);
    return mesh;
}

inline Mesh CreateCubeMesh(const std::string& name, float size, uint32_t matIdx, bool insideOut = false) {
    float s = size * 0.5f;
    std::vector<Vertex> vertices = {
        // Front
        { {-s, -s,  s}, {0,0,1}, {0,0}, {1,0,0,1} },
        { { s, -s,  s}, {0,0,1}, {1,0}, {1,0,0,1} },
        { { s,  s,  s}, {0,0,1}, {1,1}, {1,0,0,1} },
        { {-s,  s,  s}, {0,0,1}, {0,1}, {1,0,0,1} },
        // Back
        { {-s, -s, -s}, {0,0,-1}, {0,0}, {-1,0,0,1} },
        { {-s,  s, -s}, {0,0,-1}, {0,1}, {-1,0,0,1} },
        { { s,  s, -s}, {0,0,-1}, {1,1}, {-1,0,0,1} },
        { { s, -s, -s}, {0,0,-1}, {1,0}, {-1,0,0,1} },
        // Left
        { {-s, -s, -s}, {-1,0,0}, {0,0}, {0,0,1,1} },
        { {-s, -s,  s}, {-1,0,0}, {1,0}, {0,0,1,1} },
        { {-s,  s,  s}, {-1,0,0}, {1,1}, {0,0,1,1} },
        { {-s,  s, -s}, {-1,0,0}, {0,1}, {0,0,1,1} },
        // Right
        { { s, -s,  s}, {1,0,0}, {0,0}, {0,0,-1,1} },
        { { s, -s, -s}, {1,0,0}, {1,0}, {0,0,-1,1} },
        { { s,  s, -s}, {1,0,0}, {1,1}, {0,0,-1,1} },
        { { s,  s,  s}, {1,0,0}, {0,1}, {0,0,-1,1} },
        // Top
        { {-s,  s,  s}, {0,1,0}, {0,0}, {1,0,0,1} },
        { { s,  s,  s}, {0,1,0}, {1,0}, {1,0,0,1} },
        { { s,  s, -s}, {0,1,0}, {1,1}, {1,0,0,1} },
        { {-s,  s, -s}, {0,1,0}, {0,1}, {1,0,0,1} },
        // Bottom
        { {-s, -s, -s}, {0,-1,0}, {0,0}, {1,0,0,1} },
        { { s, -s, -s}, {0,-1,0}, {1,0}, {1,0,0,1} },
        { { s, -s,  s}, {0,-1,0}, {1,1}, {1,0,0,1} },
        { {-s, -s,  s}, {0,-1,0}, {0,1}, {1,0,0,1} }
    };
    
    if (insideOut) {
        for (auto& v : vertices) v.normal *= -1.0f;
    }

    std::vector<uint32_t> indices = {
        0,1,2, 2,3,0,       // Front
        4,5,6, 6,7,4,       // Back
        8,9,10, 10,11,8,    // Left
        12,13,14, 14,15,12, // Right
        16,17,18, 18,19,16, // Top
        20,21,22, 22,23,20  // Bottom
    };
    
    if (insideOut) {
        for (size_t i = 0; i < indices.size(); i += 3) {
            std::swap(indices[i+1], indices[i+2]);
        }
    }

    Mesh mesh;
    mesh.setup(name, vertices, indices, matIdx);
    return mesh;
}

inline Mesh CreateSphereMesh(const std::string& name, float radius, uint32_t segments, uint32_t rings, uint32_t matIdx) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (uint32_t y = 0; y <= rings; ++y) {
        for (uint32_t x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)rings;
            
            float xPos = std::cos(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);
            float yPos = std::cos(ySegment * 3.14159f);
            float zPos = std::sin(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);

            Vertex v;
            v.position = glm::vec3(xPos, yPos, zPos) * radius;
            v.normal = glm::vec3(xPos, yPos, zPos);
            v.texCoords = glm::vec2(xSegment, ySegment);
            v.tangent = glm::vec4(-zPos, 0.0f, xPos, 1.0f); // approx
            vertices.push_back(v);
        }
    }

    for (uint32_t y = 0; y < rings; ++y) {
        for (uint32_t x = 0; x < segments; ++x) {
            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);

            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);
            indices.push_back((y + 1) * (segments + 1) + x + 1);
        }
    }

    Mesh mesh;
    mesh.setup(name, vertices, indices, matIdx);
    return mesh;
}

} // namespace Primitives
} // namespace lgt
