#include "AnimatorSystem.h"
#include "AnimatorComponent.h"
#include "SkinnedMeshComponent.h"
#include "../Core/AssetManager.h"
#include "../Renderer/Core/DebugRenderer.h"
#include "Entity.h"
#include "Components.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace lgt {

    void AnimatorSystem::Update(Scene* scene, float dt) {
        auto view = scene->GetRegistry().view<AnimatorComponent, SkinnedMeshComponent>();
        
        for (auto entity : view) {
            auto& anim = view.get<AnimatorComponent>(entity);
            auto& skinnedMesh = view.get<SkinnedMeshComponent>(entity);
            
            if (!anim.currentClip.IsValid() || !skinnedMesh.skeletonHandle.IsValid()) continue;
            
            auto clipAsset = AssetManager::GetAsset<AnimationClip>(anim.currentClip.GetUUID());
            auto skeletonAsset = AssetManager::GetAsset<Skeleton>(skinnedMesh.skeletonHandle.GetUUID());
            
            if (!clipAsset || !skeletonAsset) continue;
            
            if (anim.state == PlaybackState::Playing) {
                anim.currentTime += dt * clipAsset->TicksPerSecond * anim.speed;
                
                if (anim.currentTime >= clipAsset->Duration) {
                    if (clipAsset->Looping) {
                        anim.currentTime = std::fmod(anim.currentTime, clipAsset->Duration);
                    } else {
                        anim.currentTime = clipAsset->Duration;
                        anim.state = PlaybackState::Stopped;
                    }
                }
            }
            
            Pose pose;
            EvaluatePose(anim.currentTime, *clipAsset, *skeletonAsset, pose);
            BuildGlobalPose(*skeletonAsset, pose);
            BuildSkinMatrices(*skeletonAsset, pose, anim.SkinMatrices);
            
            anim.GlobalBoneTransforms = pose.GlobalMatrices;

            if (anim.ShowSkeleton) {
                glm::mat4 entityTransform = glm::mat4(1.0f);
                if (scene->GetRegistry().all_of<TransformComponent>(entity)) {
                    entityTransform = scene->GetRegistry().get<TransformComponent>(entity).GlobalTransform;
                }

                const auto& bones = skeletonAsset->GetBones();
                for (size_t i = 0; i < bones.size(); ++i) {
                    if (bones[i].ParentIndex != -1) {
                        glm::vec3 start = glm::vec3(entityTransform * anim.GlobalBoneTransforms[bones[i].ParentIndex] * glm::vec4(0, 0, 0, 1));
                        glm::vec3 end = glm::vec3(entityTransform * anim.GlobalBoneTransforms[i] * glm::vec4(0, 0, 0, 1));
                        DebugRenderer::DrawLine(start, end, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow lines
                    }
                }
            }
        }
    }

    void AnimatorSystem::EvaluatePose(float time, const AnimationClip& clip, const Skeleton& skeleton, Pose& outPose) {
        uint32_t boneCount = skeleton.GetBoneCount();
        outPose.LocalTransforms.resize(boneCount);
        outPose.GlobalMatrices.resize(boneCount, glm::mat4(1.0f));
        
        for (uint32_t i = 0; i < boneCount; ++i) {
            if (i >= clip.Tracks.size()) {
                // If no track for this bone, use rest pose
                const auto& boneInfo = skeleton.GetBones()[i];
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                glm::decompose(boneInfo.RestPoseMatrix, scale, rotation, translation, skew, perspective);
                
                outPose.LocalTransforms[i] = { translation, rotation, scale };
                continue;
            }
            
            const auto& track = clip.Tracks[i];
            BoneTransform bform;
            
            // Translation
            if (track.PositionKeys.empty()) {
                bform.Translation = glm::vec3(0.0f);
            } else if (track.PositionKeys.size() == 1) {
                bform.Translation = track.PositionKeys[0].Value;
            } else {
                // Find keys
                auto p1 = track.PositionKeys.begin();
                auto p2 = p1 + 1;
                while (p2 != track.PositionKeys.end() && time > p2->Time) {
                    p1 = p2;
                    p2++;
                }
                if (p2 == track.PositionKeys.end()) {
                    bform.Translation = p1->Value;
                } else {
                    float factor = (time - p1->Time) / (p2->Time - p1->Time);
                    bform.Translation = glm::mix(p1->Value, p2->Value, factor);
                }
            }
            
            // Rotation
            if (track.RotationKeys.empty()) {
                bform.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            } else if (track.RotationKeys.size() == 1) {
                bform.Rotation = track.RotationKeys[0].Value;
            } else {
                auto r1 = track.RotationKeys.begin();
                auto r2 = r1 + 1;
                while (r2 != track.RotationKeys.end() && time > r2->Time) {
                    r1 = r2;
                    r2++;
                }
                if (r2 == track.RotationKeys.end()) {
                    bform.Rotation = r1->Value;
                } else {
                    float factor = (time - r1->Time) / (r2->Time - r1->Time);
                    bform.Rotation = glm::slerp(r1->Value, r2->Value, factor);
                }
            }
            
            // Scale
            if (track.ScaleKeys.empty()) {
                bform.Scale = glm::vec3(1.0f);
            } else if (track.ScaleKeys.size() == 1) {
                bform.Scale = track.ScaleKeys[0].Value;
            } else {
                auto s1 = track.ScaleKeys.begin();
                auto s2 = s1 + 1;
                while (s2 != track.ScaleKeys.end() && time > s2->Time) {
                    s1 = s2;
                    s2++;
                }
                if (s2 == track.ScaleKeys.end()) {
                    bform.Scale = s1->Value;
                } else {
                    float factor = (time - s1->Time) / (s2->Time - s1->Time);
                    bform.Scale = glm::mix(s1->Value, s2->Value, factor);
                }
            }
            
            outPose.LocalTransforms[i] = bform;
        }
    }
    
    void AnimatorSystem::BuildGlobalPose(const Skeleton& skeleton, Pose& inOutPose) {
        const auto& bones = skeleton.GetBones();
        for (uint32_t i = 0; i < bones.size(); ++i) {
            const auto& bform = inOutPose.LocalTransforms[i];
            glm::mat4 localMat = glm::translate(glm::mat4(1.0f), bform.Translation) * 
                                 glm::mat4_cast(bform.Rotation) * 
                                 glm::scale(glm::mat4(1.0f), bform.Scale);
            
            int parentIndex = bones[i].ParentIndex;
            if (parentIndex == -1) {
                inOutPose.GlobalMatrices[i] = localMat;
            } else {
                inOutPose.GlobalMatrices[i] = inOutPose.GlobalMatrices[parentIndex] * localMat;
            }
        }
    }
    
    void AnimatorSystem::BuildSkinMatrices(const Skeleton& skeleton, const Pose& pose, std::array<glm::mat4, MaxBones>& outMatrices) {
        const auto& bones = skeleton.GetBones();
        for (uint32_t i = 0; i < bones.size(); ++i) {
            outMatrices[i] = pose.GlobalMatrices[i] * bones[i].InverseBindMatrix;
        }
    }

}
