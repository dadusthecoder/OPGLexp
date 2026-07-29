#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../Core/AssetManager.h"

namespace lgt {

    enum class AnimationCompressionType {
        Raw,
        Compressed_ACL, // Placeholder for future Advanced Curve Library integration
        Compressed_Custom
    };

    template <typename T>
    struct Keyframe {
        float Time;
        T Value;
    };

    struct BoneTrack {
        std::vector<Keyframe<glm::vec3>> PositionKeys;
        std::vector<Keyframe<glm::quat>> RotationKeys;
        std::vector<Keyframe<glm::vec3>> ScaleKeys;
    };

    class AnimationClip : public Asset {
    public:
        AnimationClip() = default;
        ~AnimationClip() override = default;

        float Duration = 0.0f;
        float TicksPerSecond = 0.0f;
        bool Looping = true;
        AnimationCompressionType CompressionType = AnimationCompressionType::Raw;

        std::vector<BoneTrack> Tracks; // Index matches Bone Index in Skeleton
    };

}
