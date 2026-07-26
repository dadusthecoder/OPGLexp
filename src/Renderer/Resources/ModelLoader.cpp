#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <meshoptimizer.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Scene/Components.h"
#include "TextureLoader.h"
#include "../Core/Renderer.h"

namespace lgt {

    // Decompose an Assimp matrix into Translation, Rotation (Euler), Scale
    static void DecomposeAssimpMatrix(const aiMatrix4x4& aiMat, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) {
        glm::mat4 m;
        // Assimp is row-major, GLM is column-major
        m[0][0] = aiMat.a1; m[1][0] = aiMat.a2; m[2][0] = aiMat.a3; m[3][0] = aiMat.a4;
        m[0][1] = aiMat.b1; m[1][1] = aiMat.b2; m[2][1] = aiMat.b3; m[3][1] = aiMat.b4;
        m[0][2] = aiMat.c1; m[1][2] = aiMat.c2; m[2][2] = aiMat.c3; m[3][2] = aiMat.c4;
        m[0][3] = aiMat.d1; m[1][3] = aiMat.d2; m[2][3] = aiMat.d3; m[3][3] = aiMat.d4;

        glm::quat rot;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(m, scale, rot, translation, skew, perspective);
        rotation = glm::eulerAngles(rot);
    }

    // Optimize mesh geometry using meshoptimizer
    static void OptimizeMesh(std::vector<float>& vertices, std::vector<uint32_t>& indices, uint32_t vertexStride, std::vector<Meshlet>& outMeshlets) {
        size_t vertexCount = (vertices.size() * sizeof(float)) / vertexStride;
        size_t indexCount = indices.size();

        if (indexCount == 0 || vertexCount == 0) return;

        // 1. Vertex cache optimization — reorder indices for GPU vertex cache
        std::vector<uint32_t> optimizedIndices(indexCount);
        meshopt_optimizeVertexCache(optimizedIndices.data(), indices.data(), indexCount, vertexCount);

        // 2. Overdraw optimization — reduce pixel overdraw 
        meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), indexCount,
                                 vertices.data(), vertexCount, vertexStride, 1.05f);

        // 3. Vertex fetch optimization
        std::vector<float> optimizedVertices(vertices.size());
        std::vector<uint32_t> remap(vertexCount);
        meshopt_optimizeVertexFetchRemap(remap.data(), optimizedIndices.data(), indexCount, vertexCount);
        meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertexCount, vertexStride, remap.data());
        meshopt_remapIndexBuffer(optimizedIndices.data(), optimizedIndices.data(), indexCount, remap.data());

        vertices = std::move(optimizedVertices);

        // 4. Meshlet generation
        const size_t max_vertices = 64;
        const size_t max_triangles = 124;
        const float cone_weight = 0.0f; // Cone culling weight (not used yet)

        size_t max_meshlets = meshopt_buildMeshletsBound(indexCount, max_vertices, max_triangles);
        std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);
        std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
        std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

        size_t meshlet_count = meshopt_buildMeshlets(
            local_meshlets.data(),
            meshlet_vertices.data(),
            meshlet_triangles.data(),
            optimizedIndices.data(),
            indexCount,
            vertices.data(),
            vertexCount,
            vertexStride,
            max_vertices,
            max_triangles,
            cone_weight
        );

        // Build flat index buffer and our Meshlet structures
        indices.clear();
        indices.reserve(indexCount);
        outMeshlets.reserve(meshlet_count);

        for (size_t i = 0; i < meshlet_count; ++i) {
            const auto& m = local_meshlets[i];
            
            Meshlet out_m;
            out_m.vertexOffset = 0; // Not used for this method (indices are global)
            out_m.vertexCount = m.vertex_count;
            out_m.triangleOffset = static_cast<uint32_t>(indices.size() / 3);
            out_m.triangleCount = m.triangle_count;
            
            // Compute bounding sphere
            meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                &meshlet_vertices[m.vertex_offset],
                &meshlet_triangles[m.triangle_offset],
                m.triangle_count,
                vertices.data(),
                vertexCount,
                vertexStride
            );
            
            out_m.bounds.x = bounds.center[0];
            out_m.bounds.y = bounds.center[1];
            out_m.bounds.z = bounds.center[2];
            out_m.bounds.w = bounds.radius;
            
            if (i == 0) {
                std::cout << "Meshlet 0 Bounds: " << out_m.bounds.x << ", " << out_m.bounds.y << ", " << out_m.bounds.z << " R:" << out_m.bounds.w << std::endl;
            }

            outMeshlets.push_back(out_m);

            // Flatten meshlet indices to global indices
            for (unsigned int t = 0; t < m.triangle_count * 3; ++t) {
                unsigned char local_index = meshlet_triangles[m.triangle_offset + t];
                unsigned int global_index = meshlet_vertices[m.vertex_offset + local_index];
                indices.push_back(global_index);
            }
        }
    }

    static void ProcessNode(aiNode* node, const aiScene* aiscene, Scene* scene, Entity parentEntity, 
                            Shader* defaultShader, const std::string& directory) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* aimesh = aiscene->mMeshes[node->mMeshes[i]];
            
            // Create entity
            std::string nodeName = node->mName.C_Str();
            if (nodeName.empty()) nodeName = "Mesh";
            Entity meshEntity = scene->CreateEntity(nodeName);
            
            // Decompose Assimp node transform
            auto& transform = meshEntity.GetComponent<TransformComponent>();
            DecomposeAssimpMatrix(node->mTransformation, transform.Translation, transform.Rotation, transform.Scale);

            // Build interleaved vertex data: Position(3) + Normal(3) + TexCoord(2) = 8 floats
            std::vector<float> vertices;
            vertices.reserve(aimesh->mNumVertices * 8);
            std::vector<uint32_t> indices;

            for (unsigned int j = 0; j < aimesh->mNumVertices; j++) {
                // Position
                vertices.push_back(aimesh->mVertices[j].x);
                vertices.push_back(aimesh->mVertices[j].y);
                vertices.push_back(aimesh->mVertices[j].z);

                // Normal
                if (aimesh->HasNormals()) {
                    vertices.push_back(aimesh->mNormals[j].x);
                    vertices.push_back(aimesh->mNormals[j].y);
                    vertices.push_back(aimesh->mNormals[j].z);
                } else {
                    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
                }

                // TexCoords
                if (aimesh->mTextureCoords[0]) {
                    vertices.push_back(aimesh->mTextureCoords[0][j].x);
                    vertices.push_back(aimesh->mTextureCoords[0][j].y);
                } else {
                    vertices.push_back(0.0f); vertices.push_back(0.0f);
                }
            }

            for (unsigned int j = 0; j < aimesh->mNumFaces; j++) {
                aiFace face = aimesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }

            // --- Automatic geometry optimization ---
            std::vector<Meshlet> outMeshlets;
            OptimizeMesh(vertices, indices, 8 * sizeof(float), outMeshlets);

            // Upload this mesh to the global geometry buffers
            // (Note: In a full engine, we'd append and maintain offsets rather than overwriting)
            Renderer::UploadGlobalGeometry(vertices, indices, outMeshlets);

            Mesh* mesh = new Mesh(vertices, indices, outMeshlets); // Uses default PBR layout
            Material* material = new Material(defaultShader);

            // Extract PBR material properties from Assimp
            if (aimesh->mMaterialIndex >= 0) {
                aiMaterial* aimat = aiscene->mMaterials[aimesh->mMaterialIndex];
                
                // Base color
                aiColor3D color(1.f, 1.f, 1.f);
                aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                material->Albedo = glm::vec3(color.r, color.g, color.b);
                
                // Metallic
                float metallic = 0.0f;
                if (aimat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
                    material->Metallic = metallic;
                }
                
                // Roughness
                float roughness = 0.5f;
                if (aimat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
                    material->Roughness = roughness;
                }

                // Load texture maps if available
                aiString texPath;
                if (aimat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    std::string fullPath = directory + "/" + texPath.C_Str();
                    material->AlbedoMap = TextureLoader::LoadFromFile(fullPath, true).get();
                }
                if (aimat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                    std::string fullPath = directory + "/" + texPath.C_Str();
                    material->NormalMap = TextureLoader::LoadFromFile(fullPath, false).get();
                }
            }

            meshEntity.AddComponent<MeshRendererComponent>(mesh, material);
            
            spdlog::info("Loaded mesh '{}': {} vertices, {} triangles (optimized)", 
                         nodeName, mesh->GetVertexCount(), mesh->GetIndexCount() / 3);
        }

        // Process children recursively
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], aiscene, scene, parentEntity, defaultShader, directory);
        }
    }

    Entity ModelLoader::LoadModel(const std::string& path, Scene* scene, Shader* defaultShader) {
        Assimp::Importer importer;
        const aiScene* aiscene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_GenSmoothNormals | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |      // Assimp-level mesh merging
            aiProcess_OptimizeGraph);        // Assimp-level node optimization

        if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
            spdlog::error("Failed to load model '{}': {}", path, importer.GetErrorString());
            return Entity{};
        }

        // Extract directory for relative texture paths
        std::string directory = path;
        size_t lastSlash = directory.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            directory = directory.substr(0, lastSlash);
        else
            directory = ".";

        Entity rootEntity = scene->CreateEntity(path);
        ProcessNode(aiscene->mRootNode, aiscene, scene, rootEntity, defaultShader, directory);
        
        spdlog::info("Model '{}' loaded successfully", path);
        return rootEntity;
    }

}
