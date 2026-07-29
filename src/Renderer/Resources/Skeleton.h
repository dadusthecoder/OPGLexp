#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../../Core/AssetManager.h"

namespace lgt {

    constexpr uint32_t MaxBones = 256;

    struct BoneInfo {
        std::string Name;
        int ParentIndex = -1;
        glm::mat4 InverseBindMatrix = glm::mat4(1.0f);
        glm::mat4 RestPoseMatrix = glm::mat4(1.0f);
    };

    class Skeleton : public Asset {
    public:
        Skeleton() = default;
        ~Skeleton() override = default;

        void AddBone(const std::string& name, int parentIndex, const glm::mat4& invBind, const glm::mat4& restPose) {
            BoneInfo info;
            info.Name = name;
            info.ParentIndex = parentIndex;
            info.InverseBindMatrix = invBind;
            info.RestPoseMatrix = restPose;
            m_Bones.push_back(info);
        }

        const std::vector<BoneInfo>& GetBones() const { return m_Bones; }
        std::vector<BoneInfo>& GetBones() { return m_Bones; }
        uint32_t GetBoneCount() const { return static_cast<uint32_t>(m_Bones.size()); }

    private:
        std::vector<BoneInfo> m_Bones;
    };

}
