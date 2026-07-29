#include "EditorLayer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include "../Scene/Components.h"
#include "../Core/Input.h"
#include <GLFW/glfw3.h>

namespace lgt {

    void EditorLayer::Init(Scene* scene) {
        m_Scene = scene;
        m_SelectedEntity = {};
        
        m_HierarchyPanel.OnInit(m_Scene);
        m_HierarchyPanel.SetSelectionChangedCallback([this](Entity entity) {
            m_SelectedEntity = entity;
            m_InspectorPanel.SetSelectedEntity(entity);
            m_ViewportPanel.SetSelectedEntity(entity);
            m_AnimationPanel.SetSelectedEntity(entity);
        });

        m_InspectorPanel.OnInit(m_Scene);
        m_ViewportPanel.OnInit(m_Scene);
        m_ConsolePanel.OnInit(m_Scene);
        m_AssetBrowserPanel.OnInit(m_Scene);
        m_StatisticsPanel.OnInit(m_Scene);
        m_RenderGraphPanel.OnInit(m_Scene);
        m_AnimationPanel.OnInit(m_Scene);
        
        SetDarkTheme();
    }

    void EditorLayer::SetDarkTheme() {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
        
        // Headers
        colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        
        // Buttons
        colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Accents
        colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.1f, 0.4f, 0.7f, 1.0f };
        colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.1f, 0.4f, 0.7f, 1.0f };
        colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.1f, 0.4f, 0.7f, 0.5f };
        colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.1f, 0.4f, 0.7f, 0.8f };
        colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.1f, 0.4f, 0.7f, 1.0f };

        auto& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
    }

    void EditorLayer::OnUpdate(float ts) {
        static bool isMovingCamera = false;
        
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) { // Right click
            if (m_ViewportPanel.IsHovered() || m_ViewportPanel.IsFocused()) {
                isMovingCamera = true;
            }
        } else {
            isMovingCamera = false;
        }

        if (isMovingCamera) {
            auto view = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
            for (auto entityID : view) {
                auto& transform = view.get<TransformComponent>(entityID);
                auto& camera = view.get<CameraComponent>(entityID);
                if (camera.primary) {
                    float speed = camera.CameraSpeed * ts;
                    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) speed *= 2.5f;

                    glm::mat4 rotationMatrix = glm::mat4(glm::quat(transform.Rotation));
                    glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
                    glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
                    glm::vec3 up = glm::vec3(rotationMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

                    if (ImGui::IsKeyDown(ImGuiKey_W)) transform.Translation += forward * speed;
                    if (ImGui::IsKeyDown(ImGuiKey_S)) transform.Translation -= forward * speed;
                    if (ImGui::IsKeyDown(ImGuiKey_A)) transform.Translation -= right * speed;
                    if (ImGui::IsKeyDown(ImGuiKey_D)) transform.Translation += right * speed;
                    if (ImGui::IsKeyDown(ImGuiKey_E)) transform.Translation += up * speed;
                    if (ImGui::IsKeyDown(ImGuiKey_Q)) transform.Translation -= up * speed;

                    glm::vec2 mouseDelta = Input::GetMouseDelta();
                    float sensitivity = 0.002f;
                    
                    if (glm::length(mouseDelta) > 0.0f) {
                        transform.Rotation.y -= mouseDelta.x * sensitivity;
                        transform.Rotation.x -= mouseDelta.y * sensitivity;
                        
                        if (transform.Rotation.x > 1.57f) transform.Rotation.x = 1.57f;
                        if (transform.Rotation.x < -1.57f) transform.Rotation.x = -1.57f;
                    }
                    break;
                }
            }
        }
    }

    void EditorLayer::OnImGuiRender() {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Aurora Engine Dockspace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        DrawMenuBar();
        DrawToolbar();

        m_HierarchyPanel.OnImGuiRender();
        m_InspectorPanel.OnImGuiRender();
        m_ViewportPanel.OnImGuiRender();
        m_ConsolePanel.OnImGuiRender();
        m_AssetBrowserPanel.OnImGuiRender();
        if (m_ShowStatistics) m_StatisticsPanel.OnImGuiRender();
        if (m_ShowRenderGraph) m_RenderGraphPanel.OnImGuiRender();
        if (m_ShowAnimationPanel) m_AnimationPanel.OnImGuiRender();

        ImGui::End();
    }

    void EditorLayer::DrawMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {}
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {}
                ImGui::Separator();
                ImGui::MenuItem("Statistics", NULL, &m_ShowStatistics);
                ImGui::MenuItem("Render Graph", NULL, &m_ShowRenderGraph);
                ImGui::MenuItem("Animation Editor", NULL, &m_ShowAnimationPanel);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Scene")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Assets")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Rendering")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void EditorLayer::DrawToolbar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));
        
        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float size = ImGui::GetWindowHeight() - 4.0f;
        
        // Play controls
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 1.5f));
        if (ImGui::Button("Play", ImVec2(size*1.5f, size))) {}
        ImGui::SameLine();
        if (ImGui::Button("Pause", ImVec2(size*1.5f, size))) {}
        ImGui::SameLine();
        if (ImGui::Button("Step", ImVec2(size*1.5f, size))) {}

        // Gizmo controls (left side)
        ImGui::SameLine();
        ImGui::SetCursorPosX(10.0f);
        
        if (ImGui::Button("T", ImVec2(size, size))) m_ViewportPanel.m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::Button("R", ImVec2(size, size))) m_ViewportPanel.m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        ImGui::SameLine();
        if (ImGui::Button("S", ImVec2(size, size))) m_ViewportPanel.m_GizmoType = ImGuizmo::OPERATION::SCALE;
        ImGui::SameLine();
        if (ImGui::Button("None", ImVec2(size*1.5f, size))) m_ViewportPanel.m_GizmoType = -1;
        
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        
        const char* modes[] = { "Local", "World" };
        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("##Mode", &m_ViewportPanel.m_GizmoMode, modes, 2);
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

}
