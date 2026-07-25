#pragma once
#include "Vendor/glad.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace lgt {

struct SceneNode;

/// Abstract acceleration structure interface
/// Designed for future TLAS/BLAS extension
class AccelerationStructure {
public:
    virtual ~AccelerationStructure() = default;
    virtual void Build(const std::vector<std::shared_ptr<SceneNode>>& roots) = 0;
    virtual void Update() {}
    virtual void UploadToGPU() = 0;
    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
    virtual bool IsBuilt() const = 0;
    virtual int  GetNodeCount() const = 0;
    virtual int  GetTriangleCount() const = 0;
    virtual GLuint GetNodesSSBO() const = 0;
    virtual GLuint GetVerticesSSBO() const = 0;
    virtual GLuint GetIndicesSSBO() const = 0;
};

} // namespace lgt
