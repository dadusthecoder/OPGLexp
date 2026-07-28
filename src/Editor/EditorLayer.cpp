#include "EditorLayer.h"
#include "../Scene/Components.h"
#include "../Renderer/Core/Renderer.h"
#include "../Renderer/Resources/Material.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include "../Helpers/Logger.h"
#include "../Helpers/DebugStats.h"
#include "../Core/Input.h"
#include <GLFW/glfw3.h>

namespace lgt {

    void EditorLayer::Init(Scene* scene) {
        m_Scene = scene;
        m_SelectedEntity = {};
    }

    void EditorLayer::OnUpdate(float ts) {
        if (!m_ViewportFocused && !m_ViewportHovered) return;

        if (Input::IsMouseButtonDown(1)) { // Right click
            auto view = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
            for (auto entityID : view) {
                auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entityID);
                if (camera.primary) {
                    float speed = 5.0f * ts;
                    if (Input::IsKeyDown(GLFW_KEY_LEFT_SHIFT)) speed *= 2.5f;

                    // Compute forward, right, up
                    glm::vec3 forward = glm::quat(transform.Rotation) * glm::vec3(0.0f, 0.0f, -1.0f);
                    glm::vec3 right = glm::quat(transform.Rotation) * glm::vec3(1.0f, 0.0f, 0.0f);
                    glm::vec3 up = glm::quat(transform.Rotation) * glm::vec3(0.0f, 1.0f, 0.0f);

                    if (Input::IsKeyDown(GLFW_KEY_W)) transform.Translation += forward * speed;
                    if (Input::IsKeyDown(GLFW_KEY_S)) transform.Translation -= forward * speed;
                    if (Input::IsKeyDown(GLFW_KEY_A)) transform.Translation -= right * speed;
                    if (Input::IsKeyDown(GLFW_KEY_D)) transform.Translation += right * speed;
                    if (Input::IsKeyDown(GLFW_KEY_SPACE)) transform.Translation += up * speed;
                    if (Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) transform.Translation -= up * speed;

                    glm::vec2 mouseDelta = Input::GetMouseDelta();
                    float sensitivity = 0.002f;
                    
                    if (glm::length(mouseDelta) > 0.0f) {
                        transform.Rotation.y -= mouseDelta.x * sensitivity; // Yaw
                        transform.Rotation.x -= mouseDelta.y * sensitivity; // Pitch
                        
                        // Clamp pitch
                        if (transform.Rotation.x > 1.57f) transform.Rotation.x = 1.57f;
                        if (transform.Rotation.x < -1.57f) transform.Rotation.x = -1.57f;
                    }
                    break;
                }
            }
        }
    }

    void EditorLayer::OnImGuiRender() {
        DrawHierarchyPanel();
        DrawPropertiesPanel();
        DrawViewportPanel();
        DrawConsolePanel();
        DrawDebugPanel();
        DrawRendererSettingsPanel();
    }

    void EditorLayer::DrawRendererSettingsPanel() {
        ImGui::Begin("Renderer Settings");

        if (ImGui::CollapsingHeader("Environment & Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool rtaoEnabled = Renderer::IsRTAOEnabled();
            if (ImGui::Checkbox("Enable RTAO", &rtaoEnabled)) {
                Renderer::SetRTAOEnabled(rtaoEnabled);
            }

            bool rtShadowsEnabled = Renderer::IsRTShadowsEnabled();
            if (ImGui::Checkbox("Enable RT Shadows", &rtShadowsEnabled)) {
                Renderer::SetRTShadowsEnabled(rtShadowsEnabled);
            }

            bool ddgiEnabled = Renderer::IsDDGIEnabled();
            if (ImGui::Checkbox("Enable DDGI", &ddgiEnabled)) {
                Renderer::SetDDGIEnabled(ddgiEnabled);
            }

            bool meshletCullingEnabled = Renderer::IsMeshletCullingEnabled();
            if (ImGui::Checkbox("Enable Meshlet Culling", &meshletCullingEnabled)) {
                Renderer::SetMeshletCullingEnabled(meshletCullingEnabled);
            }
        }

        if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto view = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
            for (auto entityID : view) {
                auto [transform, cameraComp] = view.get<TransformComponent, CameraComponent>(entityID);
                if (cameraComp.primary) {
                    ImGui::DragFloat3("Position", &transform.Translation.x, 0.1f);
                    
                    glm::vec3 rotationDegrees = glm::degrees(transform.Rotation);
                    if (ImGui::DragFloat3("Rotation", &rotationDegrees.x, 0.5f)) {
                        transform.Rotation = glm::radians(rotationDegrees);
                    }

                    if (cameraComp.camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective) {
                        float fov = glm::degrees(cameraComp.camera.GetPerspectiveVerticalFOV());
                        if (ImGui::DragFloat("FOV", &fov, 0.1f, 1.0f, 179.0f)) {
                            cameraComp.camera.SetPerspective(glm::radians(fov), cameraComp.camera.GetPerspectiveNearClip(), cameraComp.camera.GetPerspectiveFarClip());
                        }
                    }
                    break; // Only show primary camera
                }
            }
        }

        if (ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool taaEnabled = Renderer::IsTAAEnabled();
            if (ImGui::Checkbox("Enable TAA", &taaEnabled)) {
                Renderer::SetTAAEnabled(taaEnabled);
            }
            if (taaEnabled) {
                float blend = Renderer::GetTAABlendFactor();
                if (ImGui::SliderFloat("TAA Blend Factor", &blend, 0.01f, 1.0f)) {
                    Renderer::SetTAABlendFactor(blend);
                }
            }
            
            ImGui::Separator();
            
            bool bloomEnabled = Renderer::IsBloomEnabled();
            if (ImGui::Checkbox("Enable Bloom", &bloomEnabled)) {
                Renderer::SetBloomEnabled(bloomEnabled);
            }
            if (bloomEnabled) {
                float thresh = Renderer::GetBloomThreshold();
                if (ImGui::SliderFloat("Bloom Threshold", &thresh, 0.0f, 10.0f)) {
                    Renderer::SetBloomThreshold(thresh);
                }
                float strength = Renderer::GetBloomStrength();
                if (ImGui::SliderFloat("Bloom Strength", &strength, 0.0f, 1.0f)) {
                    Renderer::SetBloomStrength(strength);
                }
            }
        }

        if (ImGui::CollapsingHeader("Light Management", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add Light")) {
                static int lightCounter = 1;
                Entity lightEntity = m_Scene->CreateEntity("Light " + std::to_string(lightCounter++));
                auto& lightComp = lightEntity.AddComponent<LightComponent>();
                lightComp.Type = 1; // Point light
                lightComp.Color = glm::vec3(1.0f);
                lightComp.Intensity = 5.0f;
                lightComp.Radius = 10.0f;
                auto& tc = lightEntity.GetComponent<TransformComponent>();
                tc.Translation = glm::vec3(0.0f, 5.0f, 0.0f);
            }

            auto view = m_Scene->GetRegistry().view<TagComponent, LightComponent, TransformComponent>();
            int lightIndex = 0;
            Entity entityToDelete = {};
            for (auto entityID : view) {
                auto [tag, lightComp, transform] = view.get<TagComponent, LightComponent, TransformComponent>(entityID);
                Entity entity{ entityID, m_Scene };

                ImGui::PushID((int)entityID);
                if (ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entityID, 0, "%s", tag.Tag.c_str())) {
                    if (ImGui::Button("Delete Light")) {
                        entityToDelete = entity;
                    }
                    
                    const char* lightTypes[] = { "Directional", "Point" };
                    ImGui::Combo("Type", &lightComp.Type, lightTypes, 2);

                    ImGui::ColorEdit3("Color", &lightComp.Color.x);
                    ImGui::SliderFloat("Intensity", &lightComp.Intensity, 0.0f, 50.0f);

                    if (lightComp.Type == 0) {
                        // Directional
                        glm::vec3 rotationDegrees = glm::degrees(transform.Rotation);
                        if (ImGui::DragFloat3("Direction (Rot)", &rotationDegrees.x, 0.5f)) {
                            transform.Rotation = glm::radians(rotationDegrees);
                        }
                    } else {
                        // Point
                        ImGui::DragFloat3("Position", &transform.Translation.x, 0.1f);
                        ImGui::DragFloat("Radius", &lightComp.Radius, 0.1f, 0.0f, 200.0f);
                    }
                    
                    ImGui::TreePop();
                }
                ImGui::PopID();
                lightIndex++;
            }

            if (entityToDelete) {
                m_Scene->DestroyEntity(entityToDelete);
                if (m_SelectedEntity == entityToDelete) m_SelectedEntity = {};
            }
        }

        ImGui::End();
    }

    void EditorLayer::DrawHierarchyPanel() {
        ImGui::Begin("Hierarchy");
        
        auto view = m_Scene->GetRegistry().view<TagComponent>();
        for (auto entityID : view) {
            Entity entity{ entityID, m_Scene };
            std::string tag = entity.GetComponent<TagComponent>().Tag;

            ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
            flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

            bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
            if (ImGui::IsItemClicked()) {
                m_SelectedEntity = entity;
            }

            bool entityDeleted = false;
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete Entity"))
                    entityDeleted = true;
                ImGui::EndPopup();
            }

            if (opened) {
                ImGui::TreePop();
            }

            if (entityDeleted) {
                m_Scene->DestroyEntity(entity);
                if (m_SelectedEntity == entity) m_SelectedEntity = {};
            }
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
            m_SelectedEntity = {};
        }

        if (ImGui::BeginPopupContextWindow("##HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                m_Scene->CreateEntity("Empty Entity");
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void EditorLayer::DrawPropertiesPanel() {
        ImGui::Begin("Properties");
        
        if (m_SelectedEntity) {
            if (m_SelectedEntity.HasComponent<TagComponent>()) {
                auto& tag = m_SelectedEntity.GetComponent<TagComponent>().Tag;
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                snprintf(buffer, sizeof(buffer), "%s", tag.c_str());
                if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
                    tag = std::string(buffer);
                }
            }

            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            if (ImGui::Button("Add Component"))
                ImGui::OpenPopup("AddComponent");

            if (ImGui::BeginPopup("AddComponent")) {
                if (!m_SelectedEntity.HasComponent<TransformComponent>()) {
                    if (ImGui::MenuItem("Transform")) {
                        m_SelectedEntity.AddComponent<TransformComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!m_SelectedEntity.HasComponent<CameraComponent>()) {
                    if (ImGui::MenuItem("Camera")) {
                        m_SelectedEntity.AddComponent<CameraComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!m_SelectedEntity.HasComponent<LightComponent>()) {
                    if (ImGui::MenuItem("Light")) {
                        m_SelectedEntity.AddComponent<LightComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::PopItemWidth();

            if (m_SelectedEntity.HasComponent<TransformComponent>()) {
                if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform")) {
                    auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
                    
                    ImGui::DragFloat3("Translation", &tc.Translation.x, 0.1f);
                    
                    glm::vec3 rotationDegrees = glm::degrees(tc.Rotation);
                    if (ImGui::DragFloat3("Rotation", &rotationDegrees.x, 0.1f)) {
                        tc.Rotation = glm::radians(rotationDegrees);
                    }
                    
                    ImGui::DragFloat3("Scale", &tc.Scale.x, 0.1f);
                    
                    ImGui::TreePop();
                }
            }

            if (m_SelectedEntity.HasComponent<CameraComponent>()) {
                if (ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Camera")) {
                    auto& cc = m_SelectedEntity.GetComponent<CameraComponent>();
                    ImGui::Checkbox("Primary", &cc.primary);
                    ImGui::TreePop();
                }
            }
            
            if (m_SelectedEntity.HasComponent<LightComponent>()) {
                if (ImGui::TreeNodeEx((void*)typeid(LightComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Light")) {
                    auto& lc = m_SelectedEntity.GetComponent<LightComponent>();
                    ImGui::ColorEdit3("Color", &lc.Color.x);
                    ImGui::SliderFloat("Intensity", &lc.Intensity, 0.0f, 20.0f);
                    
                    const char* lightTypes[] = { "Directional", "Point" };
                    ImGui::Combo("Type", &lc.Type, lightTypes, 2);
                    
                    if (lc.Type == 1) {
                        ImGui::DragFloat("Radius", &lc.Radius, 0.1f);
                    }
                    ImGui::TreePop();
                }
            }

            if (m_SelectedEntity.HasComponent<MeshRendererComponent>()) {
                if (ImGui::TreeNodeEx((void*)typeid(MeshRendererComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Mesh Renderer")) {
                    auto& mrc = m_SelectedEntity.GetComponent<MeshRendererComponent>();
                    ImGui::Text("Mesh Path: TODO");
                    if (mrc.material) {
                        ImGui::ColorEdit3("Albedo", &mrc.material->Albedo.x);
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    void EditorLayer::DrawViewportPanel() {
        ImGui::Begin("Scene");
        
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        static ImVec2 lastSize = ImVec2(0, 0);
        
        if (viewportPanelSize.x != lastSize.x || viewportPanelSize.y != lastSize.y) {
            uint32_t width = (uint32_t)viewportPanelSize.x;
            uint32_t height = (uint32_t)viewportPanelSize.y;
            
            if (width > 0 && height > 0) {
                m_Scene->OnViewportResize(width, height);
                Renderer::OnWindowResize(width, height);
            }
            lastSize = viewportPanelSize;
        }

        void* textureID = Renderer::GetFinalColorBufferTextureID();
        if (textureID) {
            ImGui::Image(textureID, ImVec2{viewportPanelSize.x, viewportPanelSize.y}, ImVec2{0, 1}, ImVec2{1, 0});
        }
        ImGui::End();
    }

    void EditorLayer::DrawConsolePanel() {
        ImGui::Begin("Console");
        
#ifndef LGT_DIST
        auto consoleSink = Log::GetConsoleSink();
        if (consoleSink) {
            auto messages = consoleSink->get_messages();
            for (const auto& msg : messages) {
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                if (msg.level == spdlog::level::trace) color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                else if (msg.level == spdlog::level::info) color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                else if (msg.level == spdlog::level::warn) color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                else if (msg.level >= spdlog::level::err) color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextWrapped("%s", msg.text.c_str());
                ImGui::PopStyleColor();
            }
            
            // Auto-scroll to bottom if at the bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
#else
        ImGui::Text("Console disabled in Dist build.");
#endif

        ImGui::End();
    }

    void EditorLayer::DrawDebugPanel() {
        ImGui::Begin("Debug Stats");
        
        bool rtaoEnabled = Renderer::IsRTAOEnabled();
        if (ImGui::Checkbox("Enable RTAO", &rtaoEnabled)) {
            Renderer::SetRTAOEnabled(rtaoEnabled);
        }
        bool ddgiEnabled = Renderer::IsDDGIEnabled();
        if (ImGui::Checkbox("Enable DDGI", &ddgiEnabled)) {
            Renderer::SetDDGIEnabled(ddgiEnabled);
        }
        bool meshletCullingEnabled = Renderer::IsMeshletCullingEnabled();
        if (ImGui::Checkbox("Enable Meshlet Culling", &meshletCullingEnabled)) {
            Renderer::SetMeshletCullingEnabled(meshletCullingEnabled);
        }
        ImGui::Separator();

#ifndef LGT_DIST
        auto stats = DebugStats::GetStats();
        if (ImGui::BeginTable("DebugStatsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("FPS");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f", ImGui::GetIO().Framerate);

            for (const auto& stat : stats) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", stat.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", stat.value.c_str());
            }
            ImGui::EndTable();
        }
#else
        ImGui::Text("Debug Stats disabled in Dist build.");
#endif

        ImGui::End();
    }

}
