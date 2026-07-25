#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// Include Assimp headers
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Renderer.h"

#include "AccelerationStructure.h"
#include "DDGIVolume.h"

namespace lgt {

enum class LightType {
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2
};

struct Light {
    glm::vec4 position; // xyz = position, w = type (0: point, 1: directional, 2: spot)
    glm::vec4 color;    // rgb = color, a = intensity
    glm::vec4 direction;// xyz = direction (for directional/spot), w = radius/range
    glm::vec4 params;   // x = inner cutoff (cos angle), y = outer cutoff (cos angle), z,w = padding
};

struct SceneNode {
    std::string       name;
    std::vector<Mesh> meshes;

    glm::mat4 localTransform  = glm::mat4(1.0f);
    glm::mat4 globalTransform = glm::mat4(1.0f);

    SceneNode*                              parent = nullptr;
    std::vector<std::shared_ptr<SceneNode>> children; // Quick top-down traversal links and strsge

    void UpdateTransformCascades() {
        if (parent) {
            globalTransform = parent->globalTransform * localTransform;
        } else {
            globalTransform = localTransform;
        }

        for (auto child : children) {
            child->UpdateTransformCascades();
        }
    }
};

class Scene {
public:
    Scene()  = default;
    ~Scene() = default;

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) noexcept            = default;
    Scene& operator=(Scene&&) noexcept = default;

    bool LoadGltf(const std::filesystem::path& path);
    void Update();
    void Clear();
    void RemoveNode(SceneNode* node);

    // Acceleration structure (BVH) for ray tracing — scene-owned data
    void BuildAccelerationStructure();
    AccelerationStructure* GetAccelerationStructure() { return m_accel.get(); }
    const AccelerationStructure* GetAccelerationStructure() const { return m_accel.get(); }
    bool IsAccelDirty() const { return m_accelDirty; }
    void MarkAccelDirty() { m_accelDirty = true; }
    void ClearAccelDirty() { m_accelDirty = false; }
    void CleanUpMaterials();

    const std::vector<std::unique_ptr<DDGIVolume>>& GetProbeVolumes() const { return m_probeVolumes; }
    std::vector<std::unique_ptr<DDGIVolume>>& GetProbeVolumes() { return m_probeVolumes; }

    const std::vector<std::shared_ptr<SceneNode>>& getRootNodes() const { return m_RootNodes; }
    void                                           AddRootNode(std::shared_ptr<SceneNode> node) { m_RootNodes.push_back(node); }
    std::vector<MaterialGPU>&                      getMaterialBuffer() { return m_materialBuffer; };
    std::vector<Light>&                       getLights() { return m_lights; }
    void                                      addLight(const Light& light) { m_lights.push_back(light); }
    
    void                                      LoadSkybox(const std::string& path) { m_skyboxPath = path; m_skyboxDirty = true; }
    std::string                               GetSkyboxPath() const { return m_skyboxPath; }
    bool                                      IsSkyboxDirty() const { return m_skyboxDirty; }
    void                                      ClearSkyboxDirty() { m_skyboxDirty = false; }

private:
    void                       processMaterials(const aiScene* scene, const std::string& dir);
    std::shared_ptr<SceneNode> parseNode(aiNode* node, const aiScene* scene, const std::vector<uint32_t>& materialMap);
    Mesh                       processMesh(aiMesh* mesh, const std::vector<uint32_t>& materialMap);

    std::vector<MaterialGPU>                m_materialBuffer;
    std::vector<Light>                      m_lights;
    std::vector<std::shared_ptr<SceneNode>> m_RootNodes;
    std::string                             m_skyboxPath = "";
    bool                                    m_skyboxDirty = false;

    // Acceleration structure for ray tracing
    std::unique_ptr<AccelerationStructure>  m_accel;
    bool                                    m_accelDirty = true;

    // DDGI Probe Volumes
    std::vector<std::unique_ptr<DDGIVolume>> m_probeVolumes;
};

} // namespace lgt
