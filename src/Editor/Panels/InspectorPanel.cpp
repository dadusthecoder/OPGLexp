#include "InspectorPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "../../Scene/Components.h"
#include "../../Scene/AnimatorComponent.h"
#include "../../Renderer/Resources/ModelLoader.h"
#include <typeinfo>

namespace lgt {

    // Helper for rendering XYZ drag vectors with color-coded labels
    static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f) {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0]; // Assuming 0 is default/bold

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
    }

    void InspectorPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void InspectorPanel::OnImGuiRender() {
        ImGui::Begin("Inspector");

        if (m_SelectedEntity) {
            DrawComponents(m_SelectedEntity);
        }

        ImGui::End();
    }

    void InspectorPanel::DrawComponents(Entity entity) {
        if (entity.HasComponent<TagComponent>()) {
            auto& tag = entity.GetComponent<TagComponent>().Tag;
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
            if (!entity.HasComponent<TransformComponent>()) {
                if (ImGui::MenuItem("Transform")) {
                    entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!entity.HasComponent<CameraComponent>()) {
                if (ImGui::MenuItem("Camera")) {
                    entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!entity.HasComponent<LightComponent>()) {
                if (ImGui::MenuItem("Light")) {
                    entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!entity.HasComponent<MeshRendererComponent>()) {
                if (ImGui::MenuItem("Mesh Renderer")) {
                    entity.AddComponent<MeshRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity.HasComponent<TransformComponent>()) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            bool open = ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), treeNodeFlags, "Transform");
            ImGui::PopStyleVar();
            if (open) {
                auto& tc = entity.GetComponent<TransformComponent>();
                DrawVec3Control("Translation", tc.Translation);
                glm::vec3 rotationDegrees = glm::degrees(tc.Rotation);
                DrawVec3Control("Rotation", rotationDegrees);
                tc.Rotation = glm::radians(rotationDegrees);
                DrawVec3Control("Scale", tc.Scale, 1.0f);
                ImGui::TreePop();
            }
        }

        if (entity.HasComponent<CameraComponent>()) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            bool open = ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), treeNodeFlags, "Camera");
            ImGui::PopStyleVar();
            if (open) {
                auto& cc = entity.GetComponent<CameraComponent>();
                ImGui::Checkbox("Primary", &cc.primary);
                ImGui::DragFloat("Speed", &cc.CameraSpeed, 0.1f, 0.1f, 100.0f);
                ImGui::TreePop();
            }
        }

        if (entity.HasComponent<LightComponent>()) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            bool open = ImGui::TreeNodeEx((void*)typeid(LightComponent).hash_code(), treeNodeFlags, "Light");
            ImGui::PopStyleVar();
            if (open) {
                auto& lc = entity.GetComponent<LightComponent>();
                const char* lightTypes[] = { "Directional", "Point" };
                ImGui::Combo("Type", &lc.Type, lightTypes, 2);
                ImGui::ColorEdit3("Color", &lc.Color.x);
                ImGui::SliderFloat("Intensity", &lc.Intensity, 0.0f, 50.0f);
                if (lc.Type == 1) {
                    ImGui::DragFloat("Radius", &lc.Radius, 0.1f, 0.0f, 200.0f);
                }
                ImGui::TreePop();
            }
        }

        if (entity.HasComponent<MeshRendererComponent>()) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            bool open = ImGui::TreeNodeEx((void*)typeid(MeshRendererComponent).hash_code(), treeNodeFlags, "Mesh Renderer");
            ImGui::PopStyleVar();
            if (open) {
                auto& mrc = entity.GetComponent<MeshRendererComponent>();
                
                if (!mrc.mesh) {
                    static char meshPath[256] = "res/models/cube/cube.obj";
                    ImGui::InputText("Model Path", meshPath, sizeof(meshPath));
                    if (ImGui::Button("Load Model")) {
                        Shader* geoShader = Shader::Create("res/shaders/geometry.glsl");
                        ModelLoader::LoadModel(meshPath, m_Context, geoShader, entity);
                    }
                } else {
                    ImGui::Text("Mesh Loaded: %d vertices", mrc.mesh->GetVertexCount());
                }

                if (mrc.material) {
                    ImGui::Separator();
                    ImGui::Text("Material Properties");
                    ImGui::ColorEdit3("Albedo", &mrc.material->Albedo.x);
                    ImGui::SliderFloat("Metallic", &mrc.material->Metallic, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", &mrc.material->Roughness, 0.0f, 1.0f);
                    
                    if (mrc.material->AlbedoMap) ImGui::Text("Albedo Map: [Loaded]");
                    if (mrc.material->NormalMap) ImGui::Text("Normal Map: [Loaded]");
                }
                ImGui::TreePop();
            }
        }

        if (entity.HasComponent<AnimatorComponent>()) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            bool open = ImGui::TreeNodeEx((void*)typeid(AnimatorComponent).hash_code(), treeNodeFlags, "Animator");
            ImGui::PopStyleVar();
            if (open) {
                auto& anim = entity.GetComponent<AnimatorComponent>();
                ImGui::Checkbox("Show Skeleton", &anim.ShowSkeleton);
                ImGui::TreePop();
            }
        }
    }

}
