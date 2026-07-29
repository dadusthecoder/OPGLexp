#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace lgt {

    struct BoneTransform {
        glm::vec3 Translation = glm::vec3(0.0f);
        glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f);
    };

    struct Pose {
        std::vector<BoneTransform> LocalTransforms;
        std::vector<glm::mat4> GlobalMatrices;
    };

}
