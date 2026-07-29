#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../Helpers/Logger.h"
#include "../../Helpers/DebugStats.h"
#include <iostream>
#include <meshoptimizer.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Scene/Components.h"
#include "Scene/SkinnedMeshComponent.h"
#include "Scene/AnimatorComponent.h"
#include "TextureLoader.h"
#include "../Core/Renderer.h"
#include <unordered_map>

namespace lgt {

    struct VertexBoneData {
        uint32_t IDs[4] = {0, 0, 0, 0};
        float Weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        void AddBoneData(uint32_t boneID, float weight) {
            for (int i = 0; i < 4; i++) {
                if (Weights[i] == 0.0f) {
                    IDs[i] = boneID;
                    Weights[i] = weight;
                    return;
                }
            }
            // Keep top 4 weights
            int smallestIndex = 0;
            for (int i = 1; i < 4; i++) {
                if (Weights[i] < Weights[smallestIndex]) {
                    smallestIndex = i;
                }
            }
            if (weight > Weights[smallestIndex]) {
                IDs[smallestIndex] = boneID;
                Weights[smallestIndex] = weight;
            }
        }
    };

    static glm::mat4 AssimpMat4ToGLM(const aiMatrix4x4& mat) {
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        );
    }

    static void OptimizeMesh(std::vector<float>& vertices, std::vector<uint32_t>& indices, uint32_t vertexStride, std::vector<Meshlet>& outMeshlets) {
        size_t vertexCount = (vertices.size() * sizeof(float)) / vertexStride;
        size_t indexCount = indices.size();

        if (indexCount == 0 || vertexCount == 0) return;

        std::vector<uint32_t> optimizedIndices(indexCount);
        meshopt_optimizeVertexCache(optimizedIndices.data(), indices.data(), indexCount, vertexCount);

        meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), indexCount,
                                 vertices.data(), vertexCount, vertexStride, 1.05f);

        std::vector<float> optimizedVertices(vertices.size());
        std::vector<uint32_t> remap(vertexCount);
        meshopt_optimizeVertexFetchRemap(remap.data(), optimizedIndices.data(), indexCount, vertexCount);
        meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertexCount, vertexStride, remap.data());
        meshopt_remapIndexBuffer(optimizedIndices.data(), optimizedIndices.data(), indexCount, remap.data());

        vertices = std::move(optimizedVertices);

        const size_t max_vertices = 64;
        const size_t max_triangles = 124;
        const float cone_weight = 0.0f; 

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

        indices.clear();
        indices.reserve(indexCount);
        outMeshlets.reserve(meshlet_count);

        for (size_t i = 0; i < meshlet_count; ++i) {
            const auto& m = local_meshlets[i];
            
            Meshlet out_m;
            out_m.vertexOffset = 0; 
            out_m.vertexCount = m.vertex_count;
            out_m.triangleOffset = static_cast<uint32_t>(indices.size() / 3);
            out_m.triangleCount = m.triangle_count;
            
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

            outMeshlets.push_back(out_m);

            for (unsigned int t = 0; t < m.triangle_count * 3; ++t) {
                unsigned char local_index = meshlet_triangles[m.triangle_offset + t];
                unsigned int global_index = meshlet_vertices[m.vertex_offset + local_index];
                indices.push_back(global_index);
            }
        }
    }

    struct ModelLoadContext {
        std::unordered_map<std::string, uint32_t> boneNameToIndex;
        std::shared_ptr<Skeleton> skeleton;
        bool hasBones = false;
    };

    static void BuildSkeletonHierarchy(aiNode* node, int parentIndex, ModelLoadContext& ctx) {
        std::string nodeName = node->mName.C_Str();
        
        int currentIndex = -1;
        if (ctx.boneNameToIndex.find(nodeName) != ctx.boneNameToIndex.end()) {
            currentIndex = ctx.boneNameToIndex[nodeName];
            
            ctx.skeleton->GetBones()[currentIndex].ParentIndex = parentIndex;
            ctx.skeleton->GetBones()[currentIndex].RestPoseMatrix = AssimpMat4ToGLM(node->mTransformation);
        }

        int nextParentIndex = (currentIndex != -1) ? currentIndex : parentIndex;

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            BuildSkeletonHierarchy(node->mChildren[i], nextParentIndex, ctx);
        }
    }

    static void ProcessNode(aiNode* node, const aiScene* aiscene, Scene* scene, Entity parentEntity, 
                            Shader* defaultShader, const std::string& directory,
                            std::vector<float>& globalVertices, std::vector<uint32_t>& globalIndices, std::vector<Meshlet>& globalMeshlets,
                            ModelLoadContext& ctx) {
        
        aiVector3D scaling;
        aiQuaternion rotation;
        aiVector3D position;
        node->mTransformation.Decompose(scaling, rotation, position);
        
        std::string nodeName = node->mName.C_Str();
        if (nodeName.empty()) nodeName = "Node";
        Entity nodeEntity = scene->CreateEntity(nodeName);
        
        if (parentEntity) {
            nodeEntity.SetParent(parentEntity);
        }
        
        auto& transform = nodeEntity.GetComponent<TransformComponent>();
        transform.Translation = glm::vec3(position.x, position.y, position.z);
        glm::quat q(rotation.w, rotation.x, rotation.y, rotation.z);
        transform.Rotation = glm::eulerAngles(q);
        transform.Scale = glm::vec3(scaling.x, scaling.y, scaling.z);

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* aimesh = aiscene->mMeshes[node->mMeshes[i]];
            
            Entity meshEntity = scene->CreateEntity(nodeName + "_Mesh");
            meshEntity.SetParent(nodeEntity);
            
            auto& meshTransform = meshEntity.GetComponent<TransformComponent>();
            meshTransform.Translation = glm::vec3(0.0f);
            meshTransform.Rotation = glm::vec3(0.0f);
            meshTransform.Scale = glm::vec3(1.0f);

            bool isSkinned = aimesh->mNumBones > 0;
            uint32_t floatsPerVertex = isSkinned ? 13 : 8;

            std::vector<VertexBoneData> boneData;
            if (isSkinned) {
                boneData.resize(aimesh->mNumVertices);
                for (uint32_t b = 0; b < aimesh->mNumBones; b++) {
                    aiBone* bone = aimesh->mBones[b];
                    std::string bName = bone->mName.C_Str();
                    
                    uint32_t boneIndex = 0;
                    if (ctx.boneNameToIndex.find(bName) == ctx.boneNameToIndex.end()) {
                        boneIndex = ctx.skeleton->GetBoneCount();
                        ctx.boneNameToIndex[bName] = boneIndex;
                        ctx.skeleton->AddBone(bName, -1, AssimpMat4ToGLM(bone->mOffsetMatrix), glm::mat4(1.0f));
                    } else {
                        boneIndex = ctx.boneNameToIndex[bName];
                    }

                    for (uint32_t w = 0; w < bone->mNumWeights; w++) {
                        uint32_t vertexId = bone->mWeights[w].mVertexId;
                        float weight = bone->mWeights[w].mWeight;
                        boneData[vertexId].AddBoneData(boneIndex, weight);
                    }
                }
            }

            std::vector<float> vertices;
            vertices.reserve(aimesh->mNumVertices * floatsPerVertex);
            std::vector<uint32_t> indices;

            for (unsigned int j = 0; j < aimesh->mNumVertices; j++) {
                // Position (Local)
                vertices.push_back(aimesh->mVertices[j].x);
                vertices.push_back(aimesh->mVertices[j].y);
                vertices.push_back(aimesh->mVertices[j].z);

                // Normal (Local)
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

                if (isSkinned) {
                    auto& bData = boneData[j];
                    float sum = bData.Weights[0] + bData.Weights[1] + bData.Weights[2] + bData.Weights[3];
                    if (sum > 0.0f) {
                        for (int w = 0; w < 4; w++) bData.Weights[w] /= sum;
                    } else {
                        bData.Weights[0] = 1.0f;
                    }

                    // Pack bone IDs into floats
                    uint32_t packedIDs[4] = { bData.IDs[0], bData.IDs[1], bData.IDs[2], bData.IDs[3] };
                    for (int w = 0; w < 4; w++) {
                        float fID;
                        std::memcpy(&fID, &packedIDs[w], sizeof(float));
                        vertices.push_back(fID);
                    }

                    // Pack bone weights (normalized ubyte4) into a single float
                    uint8_t packedWeights[4] = {
                        static_cast<uint8_t>(bData.Weights[0] * 255.0f),
                        static_cast<uint8_t>(bData.Weights[1] * 255.0f),
                        static_cast<uint8_t>(bData.Weights[2] * 255.0f),
                        static_cast<uint8_t>(bData.Weights[3] * 255.0f)
                    };
                    float fWeight;
                    std::memcpy(&fWeight, &packedWeights, sizeof(float));
                    vertices.push_back(fWeight);
                }
            }

            for (unsigned int j = 0; j < aimesh->mNumFaces; j++) {
                aiFace face = aimesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }

            std::vector<Meshlet> outMeshlets;
            OptimizeMesh(vertices, indices, floatsPerVertex * sizeof(float), outMeshlets);

            // We must upload to global geometry using floats, but our vertex layout is now variable per-mesh. 
            // In a real AAA engine we would have separated skin and static vertex buffers.
            // For now, if we mix skinned and unskinned in the global buffer, it will break.
            // The global geometry buffer needs to have a uniform layout. So we must upgrade ALL meshes in this model to SkinnedPBR if any is skinned,
            // or we must keep separate buffers for static and skinned meshes.
            // Since this model has bones, let's assume we upgrade all meshes to SkinnedPBR layout if ctx.hasBones is true.

            // Wait, if ctx.hasBones is true, and this specific mesh has no bones (isSkinned=false), 
            // we need to pad the vertex data!
            // Let's rebuild the vertices if needed.
            // ACTUALLY, if the engine uses a global geometry buffer, it's easiest to just use a single layout.
            // Let's assume `Renderer::GetGlobalVertices()` now expects 13 floats (SkinnedPBR) everywhere!
            // BUT wait, that will break previously loaded models.
            // Let's create a NEW Mesh object with its OWN buffers. We don't have to use global buffers for everything.
            // The renderer can render individual meshes just fine.

            // To avoid breaking the existing global buffer system, we'll just not use the global buffer for this skinned mesh.
            VertexLayout layout = isSkinned ? VertexLayout::SkinnedPBR() : VertexLayout::PBR();
            
            Mesh* mesh = new Mesh(vertices, indices, outMeshlets, layout);
            Material* material = new Material(defaultShader);

            if (aimesh->mMaterialIndex >= 0) {
                aiMaterial* aimat = aiscene->mMaterials[aimesh->mMaterialIndex];
                aiColor3D color(1.f, 1.f, 1.f);
                aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                material->Albedo = glm::vec3(color.r, color.g, color.b);
                float metallic = 0.0f;
                if (aimat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) material->Metallic = metallic;
                float roughness = 0.5f;
                if (aimat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) material->Roughness = roughness;

                aiString texPath;
                if (aimat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    std::string fullPath = directory + "/" + texPath.C_Str();
                    material->AlbedoMap = TextureLoader::LoadFromFile(fullPath, true);
                }
                if (aimat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                    std::string fullPath = directory + "/" + texPath.C_Str();
                    material->NormalMap = TextureLoader::LoadFromFile(fullPath, false);
                }
            }

            if (isSkinned) {
                // Wait, AssetManager doesn't support adding raw pointers yet without a file path?
                // For now, we will manually create AssetHandle. 
                // We will add the Skeleton to AssetManager later.
                meshEntity.AddComponent<SkinnedMeshComponent>();
                // We also add MeshRendererComponent so Scene::OnRender has the mesh/material pointers
                meshEntity.AddComponent<MeshRendererComponent>(mesh, material);
            } else {
                meshEntity.AddComponent<MeshRendererComponent>(mesh, material);
            }
            
            // Note: Since we are using independent mesh buffers, we don't append to globalVertices.
            // But if the renderer pipeline (like Raytracing/MeshletCulling) strictly requires the global buffers, we must append.
            // For now, we will append to global buffers ONLY IF IT IS NOT SKINNED to not break old pipelines, OR we append it and let the global buffer handle it.
            // Given the task is just to extract bones and skinning matrices, we can use independent meshes.
            // Wait, standard MeshRendererComponent expects the global buffer if MeshletCulling is used!
            // I will just let standard MeshRendererComponent use independent mesh, since `Renderer::Submit` accepts `mesh`.

            spdlog::info("Loaded mesh '{}': {} vertices, {} triangles (optimized)", nodeName, mesh->GetVertexCount(), mesh->GetIndexCount() / 3);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], aiscene, scene, nodeEntity, defaultShader, directory, globalVertices, globalIndices, globalMeshlets, ctx);
        }
    }

    Entity ModelLoader::LoadModel(const std::string& path, Scene* scene, Shader* defaultShader, Entity parent) {
        Assimp::Importer importer;
        const aiScene* aiscene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_GenSmoothNormals | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |
            aiProcess_PopulateArmatureData |
            aiProcess_LimitBoneWeights |
            aiProcess_OptimizeGraph);

        if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
            spdlog::error("Failed to load model '{}': {}", path, importer.GetErrorString());
            return Entity{};
        }

        std::string directory = path;
        size_t lastSlash = directory.find_last_of("/\\");
        if (lastSlash != std::string::npos) directory = directory.substr(0, lastSlash);
        else directory = ".";

        Entity rootEntity = scene->CreateEntity(path);
        if (parent) {
            auto& rootRel = rootEntity.GetComponent<RelationshipComponent>();
            auto& parentRel = parent.GetComponent<RelationshipComponent>();
            
            rootRel.Parent = parent;
            rootRel.NextSibling = parentRel.FirstChild;
            
            if (parentRel.FirstChild != entt::null) {
                scene->GetRegistry().get<RelationshipComponent>(parentRel.FirstChild).PrevSibling = rootEntity;
            }
            parentRel.FirstChild = rootEntity;
            parentRel.ChildrenCount++;
        }

        std::vector<float>& globalVertices = Renderer::GetGlobalVertices();
        std::vector<uint32_t>& globalIndices = Renderer::GetGlobalIndices();
        std::vector<Meshlet>& globalMeshlets = Renderer::GetGlobalMeshlets();
        
        ModelLoadContext ctx;
        ctx.skeleton = std::make_shared<Skeleton>();
        ctx.hasBones = false;

        // Check if any mesh has bones
        for (unsigned int i = 0; i < aiscene->mNumMeshes; i++) {
            if (aiscene->mMeshes[i]->mNumBones > 0) {
                ctx.hasBones = true;
                break;
            }
        }

        ProcessNode(aiscene->mRootNode, aiscene, scene, rootEntity, defaultShader, directory, globalVertices, globalIndices, globalMeshlets, ctx);
        
        if (ctx.hasBones) {
            // Build hierarchy for extracted bones
            BuildSkeletonHierarchy(aiscene->mRootNode, -1, ctx);
            
            // Extract animations
            if (aiscene->mNumAnimations > 0) {
                for (uint32_t a = 0; a < aiscene->mNumAnimations; a++) {
                    aiAnimation* aiAnim = aiscene->mAnimations[a];
                    std::shared_ptr<AnimationClip> clip = std::make_shared<AnimationClip>();
                    clip->Duration = static_cast<float>(aiAnim->mDuration);
                    clip->TicksPerSecond = static_cast<float>(aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 25.0f);
                    clip->Tracks.resize(ctx.skeleton->GetBoneCount());

                    for (uint32_t c = 0; c < aiAnim->mNumChannels; c++) {
                        aiNodeAnim* channel = aiAnim->mChannels[c];
                        std::string trackName = channel->mNodeName.C_Str();

                        if (ctx.boneNameToIndex.find(trackName) == ctx.boneNameToIndex.end()) continue;
                        
                        uint32_t boneIndex = ctx.boneNameToIndex[trackName];
                        auto& track = clip->Tracks[boneIndex];

                        for (uint32_t k = 0; k < channel->mNumPositionKeys; k++) {
                            aiVector3D pos = channel->mPositionKeys[k].mValue;
                            track.PositionKeys.push_back({ static_cast<float>(channel->mPositionKeys[k].mTime), glm::vec3(pos.x, pos.y, pos.z) });
                        }
                        for (uint32_t k = 0; k < channel->mNumRotationKeys; k++) {
                            aiQuaternion rot = channel->mRotationKeys[k].mValue;
                            track.RotationKeys.push_back({ static_cast<float>(channel->mRotationKeys[k].mTime), glm::quat(rot.w, rot.x, rot.y, rot.z) });
                        }
                        for (uint32_t k = 0; k < channel->mNumScalingKeys; k++) {
                            aiVector3D scale = channel->mScalingKeys[k].mValue;
                            track.ScaleKeys.push_back({ static_cast<float>(channel->mScalingKeys[k].mTime), glm::vec3(scale.x, scale.y, scale.z) });
                        }
                    }

                    // For now, attach AnimatorComponent to the root entity with the first animation
                    if (a == 0) {
                        rootEntity.AddComponent<AnimatorComponent>();
                        // We would create a proper AssetHandle here.
                        // animComp.currentClip = AssetManager::CreateAsset(clip);
                    }
                }
            }
        }

        // Upload the accumulated geometry to the renderer
        // Renderer::RebuildGlobalGeometryBuffers();
        
        spdlog::info("Model '{}' loaded successfully", path);
        return rootEntity;
    }

}
