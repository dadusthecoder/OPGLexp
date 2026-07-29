#pragma once
#include "Scene.h"
#include <entt/entt.hpp>
#include "../Renderer/Resources/Pose.h"
#include "../Renderer/Resources/Skeleton.h"
#include "../Renderer/Resources/AnimationClip.h"

namespace lgt {

    class AnimatorSystem {
    public:
        static void Update(Scene* scene, float dt);
        
    private:
        static void EvaluatePose(float time, const AnimationClip& clip, const Skeleton& skeleton, Pose& outPose);
        static void BuildGlobalPose(const Skeleton& skeleton, Pose& inOutPose);
        static void BuildSkinMatrices(const Skeleton& skeleton, const Pose& pose, std::array<glm::mat4, MaxBones>& outMatrices);
    };

}
