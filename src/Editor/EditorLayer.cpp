#include "EditorLayer.h"
#include "../Scene/Components.h"
#include "../Renderer/Core/Renderer.h"
#include "../Renderer/Resources/Material.h"
#include <imgui.h>
#include <cstring>

namespace lgt {

    void EditorLayer::Init(Scene* scene) {
        m_Scene = scene;
        m_SelectedEntity = {};
    }

    void EditorLayer::OnImGuiRender() {
        DrawHierarchyPanel();
        DrawPropertiesPanel();
        DrawViewportPanel();
        DrawConsolePanel();
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
        ImGui::Text("Console ready.");
        ImGui::End();
    }

}
