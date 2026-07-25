#include "Scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "GltfLoader.h"
#include "StaticBVH.h"
#include "Helpers/Logger.h"

namespace lgt {

// Helper to convert Assimp's 4x4 matrix format directly to GLM row/column
// layouts
inline glm::mat4 ConvertMatrixToGLM(const aiMatrix4x4& from) {
    glm::mat4 to;
    // Assimp is row-major, GLM is column-major
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
}

void Scene::Clear() {
    m_RootNodes.clear();
    m_lights.clear();
}

void Scene::RemoveNode(SceneNode* nodeToRemove) {
    if (!nodeToRemove) return;
    
    if (nodeToRemove->parent) {
        auto& siblings = nodeToRemove->parent->children;
        siblings.erase(std::remove_if(siblings.begin(), siblings.end(),
            [nodeToRemove](const std::shared_ptr<SceneNode>& child) {
                return child.get() == nodeToRemove;
            }), siblings.end());
    } else {
        m_RootNodes.erase(std::remove_if(m_RootNodes.begin(), m_RootNodes.end(),
            [nodeToRemove](const std::shared_ptr<SceneNode>& root) {
                return root.get() == nodeToRemove;
            }), m_RootNodes.end());
    }
    
    CleanUpMaterials();
}

void Scene::CleanUpMaterials() {
    std::unordered_set<uint32_t> usedIndices;
    
    auto collectIndices = [&](auto& self, SceneNode* node) -> void {
        if (!node) return;
        for (auto& mesh : node->meshes) {
            usedIndices.insert(mesh.materialIndex);
        }
        for (auto& child : node->children) {
            self(self, child.get());
        }
    };
    
    for (auto& root : m_RootNodes) {
        collectIndices(collectIndices, root.get());
    }
    
    for (auto it = lgt::g_MaterialBRDF.begin(); it != lgt::g_MaterialBRDF.end(); ) {
        if (usedIndices.find(it->second.gpuIndex) == usedIndices.end()) {
            it = lgt::g_MaterialBRDF.erase(it);
        } else {
            ++it;
        }
    }
    
    std::unordered_map<uint32_t, uint32_t> indexMapping;
    std::vector<MaterialGPU> newMaterialBuffer;
    
    std::vector<uint32_t> sortedUsedIndices(usedIndices.begin(), usedIndices.end());
    std::sort(sortedUsedIndices.begin(), sortedUsedIndices.end());
    
    for (uint32_t oldIdx : sortedUsedIndices) {
        uint32_t newIdx = newMaterialBuffer.size();
        indexMapping[oldIdx] = newIdx;
        if (oldIdx < m_materialBuffer.size()) {
            newMaterialBuffer.push_back(m_materialBuffer[oldIdx]);
        }
    }
    
    auto updateIndices = [&](auto& self, SceneNode* node) -> void {
        if (!node) return;
        for (auto& mesh : node->meshes) {
            if (indexMapping.count(mesh.materialIndex)) {
                mesh.materialIndex = indexMapping[mesh.materialIndex];
            }
        }
        for (auto& child : node->children) {
            self(self, child.get());
        }
    };
    for (auto& root : m_RootNodes) {
        updateIndices(updateIndices, root.get());
    }
    
    for (auto& [name, brdf] : lgt::g_MaterialBRDF) {
        if (indexMapping.count(brdf.gpuIndex)) {
            brdf.gpuIndex = indexMapping[brdf.gpuIndex];
        }
    }
    
    m_materialBuffer = std::move(newMaterialBuffer);
}

void Scene::Update() {
    for (auto root : m_RootNodes) {
        root->UpdateTransformCascades();
    }
}


std::shared_ptr<SceneNode> Scene::parseNode(aiNode* node, const aiScene* scene, const std::vector<uint32_t>& materialMap) {

    auto sceneNode  = std::make_shared<SceneNode>();
    sceneNode->name = node->mName.C_Str();
    CORE_INFO("{}", node->mName.C_Str());

    // Row-major matrix translation from Assimp to GLM
    aiMatrix4x4 t = node->mTransformation;
    sceneNode->localTransform =
        glm::mat4(t.a1, t.b1, t.c1, t.d1, t.a2, t.b2, t.c2, t.d2, t.a3, t.b3, t.c3, t.d3, t.a4, t.b4, t.c4, t.d4);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        sceneNode->meshes.push_back(processMesh(mesh, materialMap));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        sceneNode->children.push_back(parseNode(node->mChildren[i], scene, materialMap));
        sceneNode->children[i]->parent = sceneNode.get();
    }

    return sceneNode;
}

Mesh Scene::processMesh(aiMesh* mesh, const std::vector<uint32_t>& materialMap) {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal =
            mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.f);
        vertex.texCoords =
            mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.f);
        vertex.tangent = mesh->HasTangentsAndBitangents()
                             ? glm::vec4(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f)
                             : glm::vec4(0.f);
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    Mesh glMesh;
    uint32_t globalMaterialIndex = materialMap[mesh->mMaterialIndex];
    std::string meshName = mesh->mName.length > 0 ? mesh->mName.C_Str() : "Unnamed Mesh";
    glMesh.setup(meshName, vertices, indices, globalMaterialIndex);

    return glMesh;
}

bool Scene::LoadGltf(const std::filesystem::path& path) {
    Assimp::Importer importer;

    // Use standard postprocessing configuration options to sanitize incoming
    // graphics structures
    static constexpr unsigned int IMPORT_FLAGS = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                                                 aiProcess_GlobalScale | aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path.string(), IMPORT_FLAGS);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        CORE_ERROR("[Assimp Error]: {}", importer.GetErrorString());
        return false;
    }

    // Clear(); // Removed to support loading multiple models simultaneously
    std::vector<uint32_t> materialMap = loader::assimp::ProcaessMaterials(scene, path.parent_path().string());
    m_RootNodes.push_back(parseNode(scene->mRootNode, scene, materialMap));
    Update();
    m_accelDirty = true;  // Mark BVH for rebuild
    return true;
}

void Scene::BuildAccelerationStructure() {
    if (m_RootNodes.empty()) {
        CORE_WARN("Scene::BuildAccelerationStructure() — no root nodes, skipping");
        return;
    }

    m_accel = std::make_unique<StaticBVH>();
    m_accel->Build(m_RootNodes);
    m_accel->UploadToGPU();
    m_accelDirty = false;

    CORE_INFO("Scene acceleration structure built: {} nodes, {} triangles",
              m_accel->GetNodeCount(), m_accel->GetTriangleCount());
}

} // namespace lgt
