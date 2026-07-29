#pragma once
#include <array>
#include <glm/glm.hpp>
#include "../Core/AssetHandle.h"
#include "../Renderer/Resources/AnimationClip.h"
#include "../Renderer/Resources/Skeleton.h"

namespace lgt {

    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };

    struct AnimatorComponent {
        AssetHandle<AnimationClip> currentClip;
        PlaybackState state = PlaybackState::Stopped;
        
        float currentTime = 0.0f;
        float speed = 1.0f;
        
        bool ShowSkeleton = false;
        std::vector<glm::mat4> GlobalBoneTransforms;
        
        alignas(64) std::array<glm::mat4, MaxBones> SkinMatrices;

        AnimatorComponent() {
            SkinMatrices.fill(glm::mat4(1.0f));
        }
        AnimatorComponent(const AnimatorComponent&) = default;
    };

}
