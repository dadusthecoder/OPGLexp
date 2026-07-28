#include "BVH.h"
#include <algorithm>
#include <limits>

namespace lgt {

    static glm::vec3 CalculateNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        return glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }

    void BVHBuilder::Build(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
        m_Triangles.clear();
        m_Nodes.clear();

        size_t numTriangles = indices.size() / 3;
        if (numTriangles == 0) return;

        m_Triangles.resize(numTriangles);
        for (size_t i = 0; i < numTriangles; ++i) {
            uint32_t i0 = indices[i * 3 + 0];
            uint32_t i1 = indices[i * 3 + 1];
            uint32_t i2 = indices[i * 3 + 2];

            // Vertex float array stride is 8: pos.xyz, nrm.xyz, uv.xy
            glm::vec3 v0 = glm::vec3(vertices[i0 * 8 + 0], vertices[i0 * 8 + 1], vertices[i0 * 8 + 2]);
            glm::vec3 v1 = glm::vec3(vertices[i1 * 8 + 0], vertices[i1 * 8 + 1], vertices[i1 * 8 + 2]);
            glm::vec3 v2 = glm::vec3(vertices[i2 * 8 + 0], vertices[i2 * 8 + 1], vertices[i2 * 8 + 2]);
            glm::vec3 n = CalculateNormal(v0, v1, v2);

            m_Triangles[i].v0 = glm::vec4(v0, 0.0f);
            m_Triangles[i].v1 = glm::vec4(v1, 0.0f);
            m_Triangles[i].v2 = glm::vec4(v2, 0.0f);
            m_Triangles[i].normal = glm::vec4(n, 0.0f);
        }

        m_Nodes.reserve(numTriangles * 2);
        BVHNode rootNode;
        rootNode.leftFirst = 0;
        rootNode.triCount = (uint32_t)numTriangles;
        m_Nodes.push_back(rootNode);

        UpdateNodeBounds(0);
        Subdivide(0);
    }

    void BVHBuilder::UpdateNodeBounds(uint32_t nodeIdx) {
        BVHNode& node = m_Nodes[nodeIdx];
        node.aabbMin = glm::vec3(1e30f);
        node.aabbMax = glm::vec3(-1e30f);

        for (uint32_t first = node.leftFirst, i = 0; i < node.triCount; i++) {
            const BVHTriangle& leafTri = m_Triangles[first + i];
            
            node.aabbMin = glm::min(node.aabbMin, glm::vec3(leafTri.v0));
            node.aabbMin = glm::min(node.aabbMin, glm::vec3(leafTri.v1));
            node.aabbMin = glm::min(node.aabbMin, glm::vec3(leafTri.v2));
            
            node.aabbMax = glm::max(node.aabbMax, glm::vec3(leafTri.v0));
            node.aabbMax = glm::max(node.aabbMax, glm::vec3(leafTri.v1));
            node.aabbMax = glm::max(node.aabbMax, glm::vec3(leafTri.v2));
        }
    }

    float BVHBuilder::CalculateNodeCost(BVHNode& node) {
        glm::vec3 e = node.aabbMax - node.aabbMin;
        float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
        return node.triCount * surfaceArea;
    }

    float BVHBuilder::FindBestSplitPlane(BVHNode& node, int& axis, float& splitPos) {
        float bestCost = 1e30f;
        const int BINS = 8;
        
        for (int a = 0; a < 3; a++) {
            float boundsMin = 1e30f, boundsMax = -1e30f;
            for (uint32_t i = 0; i < node.triCount; i++) {
                const BVHTriangle& tri = m_Triangles[node.leftFirst + i];
                glm::vec3 centroid = (glm::vec3(tri.v0) + glm::vec3(tri.v1) + glm::vec3(tri.v2)) / 3.0f;
                boundsMin = std::min(boundsMin, centroid[a]);
                boundsMax = std::max(boundsMax, centroid[a]);
            }
            if (boundsMin == boundsMax) continue;

            Bin bins[BINS];
            float scale = BINS / (boundsMax - boundsMin);
            for (uint32_t i = 0; i < node.triCount; i++) {
                const BVHTriangle& tri = m_Triangles[node.leftFirst + i];
                glm::vec3 centroid = (glm::vec3(tri.v0) + glm::vec3(tri.v1) + glm::vec3(tri.v2)) / 3.0f;
                int binIdx = std::min(BINS - 1, (int)((centroid[a] - boundsMin) * scale));
                bins[binIdx].triCount++;
                bins[binIdx].aabbMin = glm::min(bins[binIdx].aabbMin, glm::vec3(tri.v0));
                bins[binIdx].aabbMin = glm::min(bins[binIdx].aabbMin, glm::vec3(tri.v1));
                bins[binIdx].aabbMin = glm::min(bins[binIdx].aabbMin, glm::vec3(tri.v2));
                bins[binIdx].aabbMax = glm::max(bins[binIdx].aabbMax, glm::vec3(tri.v0));
                bins[binIdx].aabbMax = glm::max(bins[binIdx].aabbMax, glm::vec3(tri.v1));
                bins[binIdx].aabbMax = glm::max(bins[binIdx].aabbMax, glm::vec3(tri.v2));
            }

            float leftArea[BINS - 1], rightArea[BINS - 1];
            int leftCount[BINS - 1], rightCount[BINS - 1];
            glm::vec3 leftBoxMin(1e30f), leftBoxMax(-1e30f);
            int leftSum = 0;
            for (int i = 0; i < BINS - 1; i++) {
                leftSum += bins[i].triCount;
                leftCount[i] = leftSum;
                leftBoxMin = glm::min(leftBoxMin, bins[i].aabbMin);
                leftBoxMax = glm::max(leftBoxMax, bins[i].aabbMax);
                glm::vec3 e = leftBoxMax - leftBoxMin;
                leftArea[i] = e.x * e.y + e.y * e.z + e.z * e.x;
            }

            glm::vec3 rightBoxMin(1e30f), rightBoxMax(-1e30f);
            int rightSum = 0;
            for (int i = BINS - 1; i > 0; i--) {
                rightSum += bins[i].triCount;
                rightCount[i - 1] = rightSum;
                rightBoxMin = glm::min(rightBoxMin, bins[i].aabbMin);
                rightBoxMax = glm::max(rightBoxMax, bins[i].aabbMax);
                glm::vec3 e = rightBoxMax - rightBoxMin;
                rightArea[i - 1] = e.x * e.y + e.y * e.z + e.z * e.x;
            }

            scale = (boundsMax - boundsMin) / BINS;
            for (int i = 0; i < BINS - 1; i++) {
                float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
                if (planeCost < bestCost) {
                    axis = a;
                    splitPos = boundsMin + scale * (i + 1);
                    bestCost = planeCost;
                }
            }
        }
        return bestCost;
    }

    void BVHBuilder::Subdivide(uint32_t nodeIdx) {
        BVHNode& node = m_Nodes[nodeIdx];
        if (node.triCount <= 4) return;

        int axis = 0;
        float splitPos = 0;
        float splitCost = FindBestSplitPlane(node, axis, splitPos);
        float nosplitCost = CalculateNodeCost(node);
        if (splitCost >= nosplitCost) return;

        int i = node.leftFirst;
        int j = i + node.triCount - 1;
        while (i <= j) {
            const BVHTriangle& tri = m_Triangles[i];
            glm::vec3 centroid = (glm::vec3(tri.v0) + glm::vec3(tri.v1) + glm::vec3(tri.v2)) / 3.0f;
            if (centroid[axis] < splitPos) {
                i++;
            } else {
                std::swap(m_Triangles[i], m_Triangles[j--]);
            }
        }

        int leftCount = i - node.leftFirst;
        if (leftCount == 0 || leftCount == node.triCount) return;

        uint32_t leftChildIdx = (uint32_t)m_Nodes.size();
        m_Nodes.push_back(BVHNode());
        m_Nodes.push_back(BVHNode());

        // Re-fetch node due to possible push_back reallocation
        m_Nodes[leftChildIdx].leftFirst = m_Nodes[nodeIdx].leftFirst;
        m_Nodes[leftChildIdx].triCount = leftCount;
        m_Nodes[leftChildIdx + 1].leftFirst = i;
        m_Nodes[leftChildIdx + 1].triCount = m_Nodes[nodeIdx].triCount - leftCount;

        m_Nodes[nodeIdx].leftFirst = leftChildIdx;
        m_Nodes[nodeIdx].triCount = 0;

        UpdateNodeBounds(leftChildIdx);
        UpdateNodeBounds(leftChildIdx + 1);

        Subdivide(leftChildIdx);
        Subdivide(leftChildIdx + 1);
    }
}
