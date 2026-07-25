#include "Renderer/Material.h"
#include "Renderer/Renderer.h"
#include "Renderer/Scene.h"
#include "Renderer/Camera.h"
#include "Renderer/Texture.h"
#include "Editor.h"
#include "Helpers/Logger.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

using namespace lgt;

static lgt::SceneNode* g_SelectedNode = nullptr;
static int g_SelectedLightIndex = -1;
static int g_GizmoOperation = 0; // ImGuizmo::TRANSLATE

namespace Editor {

lgt::SceneNode* GetSelectedNode() { return g_SelectedNode; }
void SetSelectedNode(lgt::SceneNode* node) { 
    g_SelectedNode = node; 
    if (node != nullptr) g_SelectedLightIndex = -1; // Mutual exclusion
}
int GetSelectedLightIndex() { return g_SelectedLightIndex; }
void SetSelectedLightIndex(int index) { 
    g_SelectedLightIndex = index; 
    if (index != -1) g_SelectedNode = nullptr; // Mutual exclusion
}
int GetGizmoOperation() { return g_GizmoOperation; }
void SetGizmoOperation(int op) { g_GizmoOperation = op; }

void ApplyProfessionalTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Unity-style slightly rounded edges
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding       = 2.0f;

    // Clean borders
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;

    ImVec4* colors = style.Colors;

    // Unity dark theme colors
    const ImVec4 bgDark             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Main window bg
    const ImVec4 bgMedium           = ImVec4(0.24f, 0.24f, 0.24f, 1.00f); // Child bg
    const ImVec4 bgLight            = ImVec4(0.28f, 0.28f, 0.28f, 1.00f); // Frame bg
    const ImVec4 bgLighter          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f); // Hover bg
    const ImVec4 bgActive           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Active bg
    const ImVec4 highlightBlue      = ImVec4(0.17f, 0.36f, 0.53f, 1.00f); // Unity selection blue
    const ImVec4 highlightBlueHover = ImVec4(0.20f, 0.42f, 0.62f, 1.00f);

    colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg]  = bgMedium;
    colors[ImGuiCol_PopupBg]  = ImVec4(0.18f, 0.18f, 0.18f, 0.95f);

    colors[ImGuiCol_Border]       = ImVec4(0.10f, 0.10f, 0.10f, 0.80f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]        = bgLight;
    colors[ImGuiCol_FrameBgHovered] = bgLighter;
    colors[ImGuiCol_FrameBgActive]  = bgActive;

    colors[ImGuiCol_TitleBg]          = bgDark;
    colors[ImGuiCol_TitleBgActive]    = bgDark;
    colors[ImGuiCol_TitleBgCollapsed] = bgDark;

    colors[ImGuiCol_MenuBarBg] = bgDark;

    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

    colors[ImGuiCol_CheckMark]        = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab]       = highlightBlue;
    colors[ImGuiCol_SliderGrabActive] = highlightBlueHover;

    colors[ImGuiCol_Button]        = bgLight;
    colors[ImGuiCol_ButtonHovered] = bgLighter;
    colors[ImGuiCol_ButtonActive]  = bgActive;

    colors[ImGuiCol_Header]        = bgMedium;
    colors[ImGuiCol_HeaderHovered] = bgLighter;
    colors[ImGuiCol_HeaderActive]  = highlightBlue;

    colors[ImGuiCol_Separator]        = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = bgLighter;
    colors[ImGuiCol_SeparatorActive]  = highlightBlue;

    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered] = bgLighter;
    colors[ImGuiCol_ResizeGripActive]  = highlightBlue;

    colors[ImGuiCol_Tab]                = bgDark;
    colors[ImGuiCol_TabHovered]         = bgLighter;
    colors[ImGuiCol_TabActive]          = bgMedium;
    colors[ImGuiCol_TabUnfocused]       = bgDark;
    colors[ImGuiCol_TabUnfocusedActive] = bgMedium;

    colors[ImGuiCol_TextSelectedBg] = highlightBlue;
    colors[ImGuiCol_NavHighlight]   = highlightBlue;
}

static SceneNode* s_NodeToDelete = nullptr;

static void DrawSceneNodeRecursive(lgt::SceneNode* node) {
    if (!node)
        return;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node->children.empty() && node->meshes.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string displayName = node->name.empty() ? "Unnamed Node" : node->name;

    if (node == g_SelectedNode) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)node, flags, "%s", displayName.c_str());

    if (ImGui::IsItemClicked()) {
        SetSelectedNode(node);
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            s_NodeToDelete = node;
            if (g_SelectedNode == node)
                g_SelectedNode = nullptr;
        }
        ImGui::EndPopup();
    }

    if (nodeOpen && !(flags & ImGuiTreeNodeFlags_Leaf)) {
        // Render child meshes
        for (size_t i = 0; i < node->meshes.size(); ++i) {
            ImGui::TreeNodeEx((void*)&node->meshes[i],
                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet,
                              "Mesh %zu",
                              i);
        }
        // Render child nodes
        for (auto& child : node->children) {
            DrawSceneNodeRecursive(child.get());
        }
        ImGui::TreePop();
    }
}

void DrawSceneHierarchyPanel(lgt::Scene* scene) {
    if (!scene)
        return;

    ImGui::Begin("Scene Hierarchy");

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("New Light")) {
       //     if (g_SelectedNode == node)
     //           g_SelectedNode = nullptr;
        }
        ImGui::EndPopup();
    }

    s_NodeToDelete = nullptr;
    auto& roots    = scene->getRootNodes();
    for (auto& root : roots) {
        DrawSceneNodeRecursive(root.get());
    }

    if (s_NodeToDelete) {
        scene->RemoveNode(s_NodeToDelete);
        s_NodeToDelete = nullptr;
    }

    ImGui::End();
}

void DrawEnvironmentPanel(lgt::Grid* grid, lgt::Renderer* renderer, lgt::Scene* scene) {
    ImGui::Begin("Environment & Renderer");

    if (ImGui::CollapsingHeader("Renderer Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (renderer) {
            int         currentMode = (int)renderer->getRenderMode();
            const char* items[]     = {"Fill", "Wireframe", "Point"};
            if (ImGui::Combo("Render Mode", &currentMode, items, IM_ARRAYSIZE(items))) {
                renderer->setRenderMode((lgt::RenderMode)currentMode);
            }
            ImGui::Checkbox("Visualize Tiles (Heatmap)", &renderer->visualizeTiles);

            int debugMode = (int)renderer->getDebugMode();
            const char* debugItems[] = {"Final Color", "Base Color", "Normal", "Emissive", "Normals Mapped"};
            if (ImGui::Combo("PBR Debug Mode", &debugMode, debugItems, IM_ARRAYSIZE(debugItems))) {
                renderer->setDebugMode((lgt::DebugMode)debugMode);
            }
            
            ImGui::Separator();
            int gbufDebug = renderer->getRenderContext().gBufferDebugView;
            const char* gbufItems[] = {"Off (Final)", "Albedo", "Normal", "Emissive", "Velocity", "Depth", "SSAO"};
            if (ImGui::Combo("G-Buffer Debug View", &gbufDebug, gbufItems, IM_ARRAYSIZE(gbufItems))) {
                renderer->getRenderContext().gBufferDebugView = gbufDebug;
            }

            ImGui::Separator();
            int shadowDebug = renderer->getRenderContext().shadowDebugMode;
            const char* shadowItems[] = {
                "Off", "World Pos", "View Pos", "Light Clip Pos", 
                "Proj Coords", "Shadow UV", "Stored Depth", "Current Depth", 
                "Cascade Index", "Raw Comparison", "Final Shadow"
            };
            if (ImGui::Combo("Shadow Debug View", &shadowDebug, shadowItems, IM_ARRAYSIZE(shadowItems))) {
                renderer->getRenderContext().shadowDebugMode = shadowDebug;
            }
        }
    }

    if (renderer && ImGui::CollapsingHeader("HDR & Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& ctx = renderer->getRenderContext();
        ImGui::SliderFloat("Exposure", &ctx.hdr.exposure, 0.1f, 10.0f);
        ImGui::SliderFloat("White Point", &ctx.hdr.whitePoint, 0.1f, 10.0f);
        ImGui::SliderFloat("Gamma", &ctx.hdr.gamma, 1.0f, 3.0f);
    }

    if (renderer && ImGui::CollapsingHeader("Bloom (Jimenez 2014)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& ctx = renderer->getRenderContext();
        ImGui::Checkbox("Enable Bloom", &ctx.bloom.enabled);
        if (ctx.bloom.enabled) {
            ImGui::SliderFloat("Threshold", &ctx.bloom.threshold, 0.0f, 10.0f);
            ImGui::SliderFloat("Soft Knee", &ctx.bloom.softKnee, 0.0f, 1.0f);
            ImGui::SliderFloat("Filter Radius", &ctx.bloom.filterRadius, 0.001f, 0.01f, "%.4f");
            ImGui::SliderFloat("Strength", &ctx.bloom.strength, 0.0f, 1.0f);
        }
    }

    if (renderer && ImGui::CollapsingHeader("SSAO (Ambient Occlusion)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& ctx = renderer->getRenderContext();
        ImGui::Checkbox("Enable SSAO", &ctx.ssao.enabled);
        if (ctx.ssao.enabled) {
            ImGui::SliderFloat("Radius", &ctx.ssao.radius, 0.01f, 2.0f);
            ImGui::SliderFloat("Bias", &ctx.ssao.bias, 0.0f, 0.1f);
            ImGui::SliderFloat("Intensity", &ctx.ssao.intensity, 0.0f, 5.0f);
            ImGui::SliderInt("Samples", &ctx.ssao.samples, 8, 64);
        }
    }

    if (ImGui::CollapsingHeader("Grid Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (grid) {
            lgt::GridSettings& settings = grid->getSetting();
            ImGui::Checkbox("Enable Grid", &settings.enableGrid);
            if (settings.enableGrid) {
                ImGui::ColorEdit3("Base Color", &settings.baseColor.x);
                ImGui::Checkbox("Enable Gradient", &settings.enableGradient);
                if (settings.enableGradient) {
                    ImGui::ColorEdit3("Gradient Color", &settings.gradientColor.x);
                }
                ImGui::SliderFloat("Fade Distance", &settings.fadeDistance, 10.0f, 200.0f);
                ImGui::SliderFloat("Grid Intensity", &settings.gridIntensity, 0.0f, 1.0f);

                ImGui::Checkbox("Enable Animation", &settings.enableAnimation);
                if (settings.enableAnimation) {
                    ImGui::SliderFloat("Wave Amplitude", &settings.waveAmplitude, 0.0f, 2.0f);
                    ImGui::SliderFloat("Wave Frequency", &settings.waveFrequency, 0.0f, 2.0f);
                }
            }
        }
    }

    if (scene && ImGui::CollapsingHeader("Light Management", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& lights = scene->getLights();
        
        if (ImGui::Button("Add Light")) {
            lgt::Light newLight;
            newLight.position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Point light default
            newLight.color    = glm::vec4(1.0f, 1.0f, 1.0f, 100.0f);
            newLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 5.0f); // direction, w=radius
            newLight.params = glm::vec4(glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)), 0.0f, 0.0f);
            lights.push_back(newLight);
        }
        
        ImGui::Separator();
        
        if (ImGui::BeginChild("LightsList", ImVec2(0, 200), true)) {
            for (int i = 0; i < lights.size(); i++) {
                ImGui::PushID(i);
                
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (Editor::GetSelectedLightIndex() == i) flags |= ImGuiTreeNodeFlags_Selected;
                
                bool nodeOpen = ImGui::TreeNodeEx(("Light " + std::to_string(i)).c_str(), flags);
                if (ImGui::IsItemClicked()) {
                    Editor::SetSelectedLightIndex(i);
                }
                
                if (nodeOpen) {
                    int lightType = (int)lights[i].position.w;
                    const char* typeItems[] = { "Point", "Directional", "Spot" };
                    if (ImGui::Combo("Type", &lightType, typeItems, IM_ARRAYSIZE(typeItems))) {
                        lights[i].position.w = (float)lightType;
                    }

                    if (lightType != 1 || true) { // Always show position so it can be edited by gizmo
                        ImGui::DragFloat3("Position", &lights[i].position.x, 0.1f);
                    }
                    if (lightType != 0) { // Not point
                        ImGui::DragFloat3("Direction", &lights[i].direction.x, 0.01f, -1.0f, 1.0f);
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            lights[i].direction = glm::vec4(glm::normalize(glm::vec3(lights[i].direction)), lights[i].direction.w);
                        }
                    }
                    
                    ImGui::ColorEdit3("Color", &lights[i].color.r);
                    ImGui::DragFloat("Intensity", &lights[i].color.a, 0.1f, 0.0f, 10000.0f);
                    
                    if (lightType != 1) { // Not directional
                        ImGui::DragFloat("Radius", &lights[i].direction.w, 0.1f, 0.1f, 1000.0f);
                    }
                    
                    if (lightType == 2) { // Spot
                        float inner = glm::degrees(glm::acos(lights[i].params.x));
                        float outer = glm::degrees(glm::acos(lights[i].params.y));
                        if (ImGui::DragFloat("Inner Cutoff", &inner, 0.5f, 0.0f, outer)) {
                            lights[i].params.x = glm::cos(glm::radians(inner));
                        }
                        if (ImGui::DragFloat("Outer Cutoff", &outer, 0.5f, inner, 90.0f)) {
                            lights[i].params.y = glm::cos(glm::radians(outer));
                        }
                    }

                    if (ImGui::Button("Delete Light")) {
                        lights.erase(lights.begin() + i);
                        // Fix selected index after deletion
                        int newSelected = Editor::GetSelectedLightIndex();
                        if (newSelected >= (int)lights.size()) {
                            Editor::SetSelectedLightIndex((int)lights.size() - 1);
                        }
                        i--;
                        ImGui::TreePop();
                        ImGui::PopID();
                        continue;
                    }
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Clear All Lights")) {
            lights.clear();
        }
    }

    if (scene && ImGui::CollapsingHeader("Skybox & IBL", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char skyboxPath[256] = "res/textures/skybox.hdr";
        ImGui::InputText("HDR Path", skyboxPath, sizeof(skyboxPath));
        if (ImGui::Button("Load Skybox")) {
            scene->LoadSkybox(skyboxPath);
        }
    }

    ImGui::End();
}

void DrawCameraPanel(lgt::Camera* camera, float* speed, float* sensitivity) {
    if (!camera)
        return;

    ImGui::Begin("Camera Controls");

    if (ImGui::CollapsingHeader("Movement Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (speed)
            ImGui::SliderFloat("Move Speed", speed, 0.0001f, 0.01f, "%.4f");
        if (sensitivity)
            ImGui::SliderFloat("Sensitivity", sensitivity, 1.0f, 100.0f);
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 pos = camera->GetCameraPos();
        glm::vec3 dir = camera->GetDirection();

        ImGui::Text("Position: X: %.2f  Y: %.2f  Z: %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("Direction: X: %.2f  Y: %.2f  Z: %.2f", dir.x, dir.y, dir.z);
    }

    ImGui::End();
}

void DrawAssetBrowserPanel(lgt::Scene* scene) {
    if (!scene)
        return;

    ImGui::Begin("Asset Browser");

    std::filesystem::path modelsPath = "res/modles"; // Correcting typo locally if needed, but sticking to existing structure
    if (!std::filesystem::exists(modelsPath)) {
        modelsPath = "res/models"; // Fallback to correct spelling just in case
    }

    if (std::filesystem::exists(modelsPath)) {
        static GLuint s_GltfIcon = 0;
        static GLuint s_FbxIcon  = 0;

        if (s_GltfIcon == 0) {
            int            width, height, nrChannels;
            unsigned char* data = stbi_load("res/images/icon_gltf.jpg", &width, &height, &nrChannels, 4);
            if (data) {
                glGenTextures(1, &s_GltfIcon);
                glBindTexture(GL_TEXTURE_2D, s_GltfIcon);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
            }
        }

        if (s_FbxIcon == 0) {
            int            width, height, nrChannels;
            unsigned char* data = stbi_load("res/images/icon_fbx.jpg", &width, &height, &nrChannels, 4);
            if (data) {
                glGenTextures(1, &s_FbxIcon);
                glBindTexture(GL_TEXTURE_2D, s_FbxIcon);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
            }
        }

        static float padding       = 16.0f;
        static float thumbnailSize = 90.0f;
        float        cellSize      = thumbnailSize + padding;

        float panelWidth  = ImGui::GetContentRegionAvail().x;
        int   columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1)
            columnCount = 1;

        if (ImGui::BeginTable("AssetGrid", columnCount)) {
            for (auto& entry : std::filesystem::recursive_directory_iterator(modelsPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".gltf" || ext == ".glb" || ext == ".fbx") {
                        ImGui::TableNextColumn();
                        std::string filename = entry.path().filename().string();

                        std::string filepath = entry.path().string();
                        ImGui::PushID(filepath.c_str());

                        // Push button colors to look more like a Unity thumbnail backing
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                        bool clicked = false;

                        GLuint activeIcon = 0;
                        if (ext == ".gltf" || ext == ".glb")
                            activeIcon = s_GltfIcon;
                        else if (ext == ".fbx")
                            activeIcon = s_FbxIcon;

                        if (activeIcon != 0) {
                            clicked = ImGui::ImageButton(
                                filename.c_str(), (ImTextureID)(intptr_t)activeIcon, ImVec2(thumbnailSize, thumbnailSize));
                        } else {
                            clicked = ImGui::Button("MODEL", ImVec2(thumbnailSize, thumbnailSize));
                        }

                        if (clicked) {
                            scene->LoadGltf(entry.path().string());
                        }
                        ImGui::PopStyleColor(3);

                        // Attempt to center text under the button
                        float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
                        float offset    = (thumbnailSize - textWidth) * 0.5f;
                        if (offset > 0.0f) {
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
                        }
                        ImGui::TextWrapped("%s", filename.c_str());
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndTable();
        }

    } else {
        ImGui::Text("Models directory not found.");
    }

    ImGui::End();
}

void DrawInspectorPanel() {
    ImGui::Begin("Inspector");

    static lgt::SceneNode* s_LastSelectedNode = nullptr;
    static glm::vec3       s_Translation(0.0f);
    static glm::vec3       s_RotationEuler(0.0f);
    static glm::vec3       s_Scale(1.0f);

    if (g_SelectedNode) {
        int op = GetGizmoOperation();
        if (ImGui::RadioButton("Translate", op == 0)) SetGizmoOperation(0);
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", op == 1)) SetGizmoOperation(1);
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", op == 2)) SetGizmoOperation(2);
        ImGui::Separator();
        
        if (g_SelectedNode != s_LastSelectedNode) {
            s_LastSelectedNode = g_SelectedNode;

            // Decompose the localTransform matrix only once upon selection
            glm::quat rotationQuat;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(g_SelectedNode->localTransform, s_Scale, rotationQuat, s_Translation, skew, perspective);

            // Convert quaternion to Euler angles (in degrees for UI)
            s_RotationEuler = glm::degrees(glm::eulerAngles(rotationQuat));
        }

        // 1. Name input
        char nameBuf[256];
        strncpy(nameBuf, g_SelectedNode->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-1); // fill width
        if (ImGui::InputText("##NodeName", nameBuf, sizeof(nameBuf))) {
            g_SelectedNode->name = nameBuf;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Transform Component
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                auto DrawVec3Control = [](const std::string& label, glm::vec3& values, float resetValue = 0.0f) {
                    bool modified = false;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label.c_str());

                    ImGui::TableNextColumn();
                    ImGui::PushID(label.c_str());

                    float  availableWidth = ImGui::GetContentRegionAvail().x;
                    float  itemWidth      = (availableWidth - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
                    float  lineHeight     = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
                    ImVec2 buttonSize     = {lineHeight + 3.0f, lineHeight};

                    auto drawComponent = [&](const char* compLabel, float* v, const ImVec4& color, const ImVec4& hoverColor) {
                        ImGui::PushStyleColor(ImGuiCol_Button, color);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
                        if (ImGui::Button(compLabel, buttonSize)) {
                            *v       = resetValue;
                            modified = true;
                        }
                        ImGui::PopStyleColor(3);

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(itemWidth - buttonSize.x);
                        if (ImGui::DragFloat(("##" + std::string(compLabel)).c_str(), v, 0.1f)) {
                            modified = true;
                        }
                    };

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
                    drawComponent("X", &values.x, ImVec4{0.8f, 0.1f, 0.15f, 1.0f}, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
                    ImGui::PopStyleVar();

                    ImGui::SameLine();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
                    drawComponent("Y", &values.y, ImVec4{0.2f, 0.7f, 0.2f, 1.0f}, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
                    ImGui::PopStyleVar();

                    ImGui::SameLine();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
                    drawComponent("Z", &values.z, ImVec4{0.1f, 0.25f, 0.8f, 1.0f}, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
                    ImGui::PopStyleVar();

                    ImGui::PopID();
                    return modified;
                };

                bool modified = false;
                if (DrawVec3Control("Position", s_Translation))
                    modified = true;
                if (DrawVec3Control("Rotation", s_RotationEuler))
                    modified = true;
                if (DrawVec3Control("Scale", s_Scale, 1.0f))
                    modified = true;

                if (modified) {
                    glm::quat newRot       = glm::quat(glm::radians(s_RotationEuler));
                    glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), s_Translation) * glm::mat4_cast(newRot) *
                                             glm::scale(glm::mat4(1.0f), s_Scale);

                    g_SelectedNode->localTransform = newTransform;
                    g_SelectedNode->UpdateTransformCascades();
                }

                ImGui::EndTable();
            }
        }

        // 3. Mesh Components
        if (!g_SelectedNode->meshes.empty()) {
            for (size_t i = 0; i < g_SelectedNode->meshes.size(); ++i) {
                ImGui::PushID((int)i);
                auto& mesh = g_SelectedNode->meshes[i];

                ImGui::Spacing();
                std::string filterHeader = "Mesh Filter (" + std::to_string(i) + ")";
                if (ImGui::CollapsingHeader(filterHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTable("MeshFilterTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Mesh");

                        ImGui::TableNextColumn();
                        std::string meshName = mesh.name.empty() ? ("Unnamed Mesh") : mesh.name;
                        ImGui::Text("%s (Indices: %zu)", meshName.c_str(), mesh.indexCount);

                        ImGui::EndTable();
                    }
                }

                ImGui::Spacing();
                std::string rendererHeader = "Mesh Renderer (" + std::to_string(i) + ")";
                if (ImGui::CollapsingHeader(rendererHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTable("MeshRendererTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Material");

                        ImGui::TableNextColumn();
                        std::string currentMatName = "Unknown";
                        for (const auto& [name, brdf] : lgt::g_MaterialBRDF) {
                            if (brdf.gpuIndex == mesh.materialIndex) {
                                currentMatName = name.empty() ? "Unnamed Material" : name;
                                break;
                            }
                        }

                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::BeginCombo("##Material", currentMatName.c_str())) {
                            for (const auto& [name, brdf] : lgt::g_MaterialBRDF) {
                                bool        isSelected  = (mesh.materialIndex == brdf.gpuIndex);
                                std::string displayName = name.empty() ? "Unnamed Material" : name;
                                std::string uniqueLabel = displayName + "##mat_" + std::to_string(brdf.gpuIndex);

                                if (ImGui::Selectable(uniqueLabel.c_str(), isSelected)) {
                                    mesh.materialIndex = brdf.gpuIndex;
                                }
                                if (isSelected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::EndTable();
                    }
                }
                ImGui::PopID();
            }
        }

        // 4. Hierarchy Info
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Hierarchy Info")) {
            if (ImGui::BeginTable("HierarchyTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Parent");
                ImGui::TableNextColumn();
                if (g_SelectedNode->parent) {
                    ImGui::Text("%s", g_SelectedNode->parent->name.empty() ? "Unnamed" : g_SelectedNode->parent->name.c_str());
                } else {
                    ImGui::Text("None (Root)");
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Children");
                ImGui::TableNextColumn();
                ImGui::Text("%zu", g_SelectedNode->children.size());

                ImGui::EndTable();
            }
        }
    } else {
        s_LastSelectedNode = nullptr;
        ImGui::Text("No node selected.");
    }

    ImGui::End();
}

void DrawMaterialEditorPanel() {
    ImGui::Begin("Material Editor");

    if (lgt::g_MaterialBRDF.empty()) {
        ImGui::Text("No materials loaded in scene.");
        ImGui::End();
        return;
    }

    static uint32_t selectedGpuIndex = 0xFFFFFFFF;
    static bool     isInitialized    = false;

    if (!isInitialized) {
        selectedGpuIndex = lgt::g_MaterialBRDF.begin()->second.gpuIndex;
        isInitialized    = true;
    }

    std::string selectedMaterialName = "";
    bool        foundActive          = false;
    for (const auto& [name, brdf] : lgt::g_MaterialBRDF) {
        if (brdf.gpuIndex == selectedGpuIndex) {
            selectedMaterialName = name;
            foundActive          = true;
            break;
        }
    }

    if (!foundActive) {
        selectedGpuIndex     = lgt::g_MaterialBRDF.begin()->second.gpuIndex;
        selectedMaterialName = lgt::g_MaterialBRDF.begin()->first;
    }

    std::string comboLabel = selectedMaterialName.empty() ? "Unnamed Material" : selectedMaterialName;
    if (ImGui::BeginCombo("Active Material", comboLabel.c_str())) {
        for (const auto& [name, brdf] : lgt::g_MaterialBRDF) {
            bool        isSelected  = (brdf.gpuIndex == selectedGpuIndex);
            std::string displayName = name.empty() ? "Unnamed Material" : name;
            std::string uniqueLabel = displayName + "##mat_" + std::to_string(brdf.gpuIndex);

            if (ImGui::Selectable(uniqueLabel.c_str(), isSelected)) {
                selectedGpuIndex = brdf.gpuIndex;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    auto&    activeBRDF = lgt::g_MaterialBRDF[selectedMaterialName];
    uint32_t gpuIndex   = activeBRDF.gpuIndex;

    lgt::MaterialGPU currentGPUData = lgt::g_MaterialGPU[gpuIndex];
    bool             trackChanges   = false;

    ImGui::TextDisabled("GPU Buffer Index: %u", gpuIndex);

    if (ImGui::CollapsingHeader("PBR Colors & Factors", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::ColorEdit4("Base Color", &currentGPUData.baseColor.x))
            trackChanges = true;
        if (ImGui::ColorEdit4("Emissive Color", &currentGPUData.emmisiveColor.x))
            trackChanges = true;

        ImGui::Spacing();
        if (ImGui::SliderFloat("Roughness", &currentGPUData.roughness, 0.0f, 1.0f))
            trackChanges = true;
        if (ImGui::SliderFloat("Metallic", &currentGPUData.metallic, 0.0f, 1.0f))
            trackChanges = true;
        if (ImGui::SliderFloat("Emissive Strength", &currentGPUData.emmisiveStrength, 0.0f, 20.0f))
            trackChanges = true;
    }

    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto displayTextureSlot = [](const char* label, GLuint64 handle) {
            ImGui::Text("%s:", label);
            ImGui::SameLine(160);
            if (handle != 0) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "0x%llX (Resident)", handle);
            } else {
                ImGui::TextDisabled("None (Fallback Color)");
            }
        };

        displayTextureSlot("Base Color Map", currentGPUData.diffuseMap);
        displayTextureSlot("Normal Map", currentGPUData.normalMap);
        displayTextureSlot("Emissive Map", currentGPUData.emmisiveMap);
    }

    if (ImGui::CollapsingHeader("Texture Sampler")) {
        for (size_t i = 0; i < activeBRDF.textures.size(); ++i) {
            auto& tex = activeBRDF.textures[i];

            std::string headerName = "Slot [" + std::to_string(i) + "] - ID: " + tex.textureId;
            if (ImGui::TreeNode(headerName.c_str())) {

                ImGui::Text("Texture Handle: %u", tex.textureHandle);

                auto getWrapStr = [](GLenum mode) {
                    if (mode == GL_REPEAT)
                        return "REPEAT";
                    if (mode == GL_CLAMP_TO_EDGE)
                        return "CLAMP_TO_EDGE";
                    if (mode == GL_MIRRORED_REPEAT)
                        return "MIRRORED_REPEAT";
                    return "UNKNOWN";
                };

                ImGui::Text("Wrap S (U Axis): %s", getWrapStr(tex.samplerState.wrapModeS));
                ImGui::Text("Wrap T (V Axis): %s", getWrapStr(tex.samplerState.wrapModeT));

                ImGui::TreePop();
                ImGui::Spacing();
            }
        }
    }

    if (trackChanges) {
        lgt::g_MaterialGPU[gpuIndex] = currentGPUData;
        lgt::UpateMaterial(gpuIndex, currentGPUData);
    }

    ImGui::End();
}

void DrawConsolePanel() {
    ImGui::Begin("Console");

    auto sink = Log::GetConsoleSink();
    if (!sink) {
        ImGui::Text("Console sink not initialized.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) {
        sink->clear();
    }
    ImGui::SameLine();

    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    auto messages = sink->get_messages();

    for (const auto& msg : messages) {
        ImVec4 color;
        bool   hasColor = true;

        switch (msg.level) {
        case spdlog::level::trace:
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            break; // Gray
        case spdlog::level::debug:
            color = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
            break; // Green
        case spdlog::level::info:
            color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
            break; // White
        case spdlog::level::warn:
            color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
            break; // Yellow
        case spdlog::level::err:
            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            break; // Red
        case spdlog::level::critical:
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break; // Bright Red
        default:
            hasColor = false;
            break;
        }

        if (hasColor)
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(msg.text.c_str());
        if (hasColor)
            ImGui::PopStyleColor();
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}

} // namespace Editor