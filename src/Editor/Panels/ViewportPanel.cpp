#include "ViewportPanel.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include "../../Renderer/Core/Renderer.h"
#include "../../Scene/Components.h"
#include "../../Core/Input.h"

namespace lgt {

    void ViewportPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void ViewportPanel::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");
        
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        
        if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y) {
            m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
            uint32_t width = (uint32_t)m_ViewportSize.x;
            uint32_t height = (uint32_t)m_ViewportSize.y;
            
            if (width > 0 && height > 0) {
                m_Context->OnViewportResize(width, height);
                Renderer::OnWindowResize(width, height);
            }
        }

        void* textureID = Renderer::GetFinalColorBufferTextureID();
        if (textureID) {
            ImGui::Image(textureID, ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});
        }

        // Draw Viewport Toolbar
        ImGui::SetCursorPos(ImVec2(10, 30)); // Top left corner inside viewport
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.7f));
        ImGui::BeginChild("##ViewportToolbar", ImVec2(240, 32), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (ImGui::Button("Select")) m_GizmoType = -1;
        ImGui::SameLine();
        if (ImGui::Button("Translate")) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::Button("Rotate")) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        ImGui::SameLine();
        if (ImGui::Button("Scale")) m_GizmoType = ImGuizmo::OPERATION::SCALE;
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Draw ImGuizmo
        if (m_SelectedEntity && m_GizmoType != -1) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

            // Get camera matrices
            auto view = m_Context->GetRegistry().view<TransformComponent, CameraComponent>();
            glm::mat4 cameraProjection;
            glm::mat4 cameraView;
            bool hasCamera = false;
            for (auto entityID : view) {
                auto& transform = view.get<TransformComponent>(entityID);
                auto& camera = view.get<CameraComponent>(entityID);
                if (camera.primary) {
                    cameraProjection = camera.camera.GetProjection();
                    
                    // Simple view matrix construction
                    glm::mat4 t = glm::translate(glm::mat4(1.0f), transform.Translation);
                    glm::mat4 r = glm::mat4(glm::quat(transform.Rotation));
                    cameraView = glm::inverse(t * r);
                    hasCamera = true;
                    break;
                }
            }

            if (hasCamera) {
                auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
                glm::mat4 transform = tc.GlobalTransform; // Assume it's already updated, or compute local
                
                // Construct local transform matrix
                glm::mat4 t = glm::translate(glm::mat4(1.0f), tc.Translation);
                glm::mat4 r = glm::mat4(glm::quat(tc.Rotation));
                glm::mat4 s = glm::scale(glm::mat4(1.0f), tc.Scale);
                glm::mat4 localTransform = t * r * s;

                // Support snapping
                bool snap = Input::IsKeyDown(GLFW_KEY_LEFT_CONTROL);
                float snapValue = 0.5f;
                if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) snapValue = 45.0f;
                float snapValues[3] = { snapValue, snapValue, snapValue };

                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                    (ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)m_GizmoMode, glm::value_ptr(localTransform),
                    nullptr, snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing()) {
                    glm::vec3 translation, rotation, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localTransform), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
                    
                    tc.Translation = translation;
                    tc.Rotation = glm::radians(rotation);
                    tc.Scale = scale;
                }
            }
        }

        // Draw Diagnostics Overlay
        ImGui::SetCursorPos(ImVec2(m_ViewportSize.x - 200, 30));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.7f));
        ImGui::BeginChild("##DiagnosticsOverlay", ImVec2(180, 50), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar();
    }

}
