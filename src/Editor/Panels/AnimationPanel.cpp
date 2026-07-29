#include "AnimationPanel.h"
#include <imgui.h>
#include "../../Scene/Components.h"
#include "../../Scene/AnimatorComponent.h"
#include "../../Core/AssetManager.h"

namespace lgt {

    AnimationPanel::AnimationPanel() {
    }

    AnimationPanel::~AnimationPanel() {
    }

    void AnimationPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void AnimationPanel::OnImGuiRender() {
        ImGui::Begin("Animation Editor");

        if (!m_Context) {
            ImGui::End();
            return;
        }

        if (m_SelectedEntity) {
            if (m_SelectedEntity.HasComponent<AnimatorComponent>()) {
                auto& anim = m_SelectedEntity.GetComponent<AnimatorComponent>();
                
                ImGui::Text("Animation Controls");
                ImGui::Separator();
                
                // Playback Controls
                if (ImGui::Button(anim.state == PlaybackState::Playing ? "Pause" : "Play")) {
                    if (anim.state == PlaybackState::Playing)
                        anim.state = PlaybackState::Paused;
                    else
                        anim.state = PlaybackState::Playing;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    anim.state = PlaybackState::Stopped;
                    anim.currentTime = 0.0f;
                }
                
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.0f);
                ImGui::SliderFloat("Speed", &anim.speed, 0.1f, 5.0f);
                
                // Timeline Scrubber
                if (anim.currentClip.IsValid()) {
                    auto clipAsset = AssetManager::GetAsset<AnimationClip>(anim.currentClip.GetUUID());
                    if (clipAsset) {
                        float duration = clipAsset->Duration;
                        float time = anim.currentTime;
                        
                        ImGui::Text("Clip: %s", anim.currentClip.GetUUID().str().c_str());
                        
                        if (ImGui::SliderFloat("Timeline", &time, 0.0f, duration, "%.2f s")) {
                            anim.currentTime = time;
                            // If user is scrubbing, we evaluate the pose even if paused.
                            // But AnimatorSystem handles this.
                        }
                    }
                } else {
                    ImGui::Text("No Animation Clip attached.");
                }
            } else {
                ImGui::Text("Selected Entity does not have an AnimatorComponent.");
            }
        } else {
            ImGui::Text("No Entity Selected");
        }

        ImGui::End();
    }

}
