#include "HierarchyPanel.h"
#include <imgui.h>
#include "../../Scene/Components.h"
#include "../../Scene/SkinnedMeshComponent.h"
#include "../../Helpers/Logger.h"

namespace lgt {

    void HierarchyPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void HierarchyPanel::OnImGuiRender() {
        ImGui::Begin("Hierarchy");
        
        if (m_Context) {
            ImGui::InputText("Search", m_SearchBuffer, sizeof(m_SearchBuffer));
            ImGui::Separator();

            std::string searchString = m_SearchBuffer;
            std::transform(searchString.begin(), searchString.end(), searchString.begin(), ::tolower);

            auto view = m_Context->GetRegistry().view<TagComponent, RelationshipComponent>();
            for (auto entityID : view) {
                auto& rel = view.get<RelationshipComponent>(entityID);
                if (rel.Parent == entt::null) {
                    DrawEntityNode(Entity{ entityID, m_Context });
                }
            }

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
                m_SelectedEntity = {};
                if (m_SelectionCallback) m_SelectionCallback(m_SelectedEntity);
            }

            if (ImGui::BeginPopupContextWindow("##HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    m_Context->CreateEntity("Empty Entity");
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::End();
    }

    void HierarchyPanel::DrawEntityNode(Entity entity) {
        std::string tag = entity.GetComponent<TagComponent>().Tag;
        auto& rel = entity.GetComponent<RelationshipComponent>();

        std::string searchString = m_SearchBuffer;
        std::transform(searchString.begin(), searchString.end(), searchString.begin(), ::tolower);
        
        std::string tagLower = tag;
        std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
        
        // If searching and this doesn't match (and children don't match, ideally), skip
        // Simple search: just hide if not matched and no search string is empty
        if (!searchString.empty() && tagLower.find(searchString) == std::string::npos) {
            // Wait, we still need to draw children that might match
            bool childMatch = false;
            entt::entity curr = rel.FirstChild;
            while(curr != entt::null) {
                auto& childTag = m_Context->GetRegistry().get<TagComponent>(curr).Tag;
                std::string cTagL = childTag;
                std::transform(cTagL.begin(), cTagL.end(), cTagL.begin(), ::tolower);
                if (cTagL.find(searchString) != std::string::npos) {
                    childMatch = true;
                    break;
                }
                curr = m_Context->GetRegistry().get<RelationshipComponent>(curr).NextSibling;
            }
            if (!childMatch) return;
        }

        ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        
        if (rel.FirstChild == entt::null) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        // Determine icon
        std::string icon = "[E]";
        if (entity.HasComponent<CameraComponent>()) icon = "[C]";
        else if (entity.HasComponent<LightComponent>()) icon = "[L]";
        else if (entity.HasComponent<MeshRendererComponent>() || entity.HasComponent<SkinnedMeshComponent>()) icon = "[M]";

        std::string displayString = icon + " " + tag;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", displayString.c_str());
        
        // Drag and Drop implementation
        if (ImGui::BeginDragDropSource()) {
            uint32_t entityID = (uint32_t)entity;
            ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entityID, sizeof(uint32_t));
            ImGui::Text("%s", displayString.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                uint32_t payloadEntityID = *(const uint32_t*)payload->Data;
                Entity payloadEntity = { (entt::entity)payloadEntityID, m_Context };
                
                // Set payloadEntity's parent to this entity
                // Note: Need to implement a proper SetParent method in Scene or RelationshipComponent handling
                // For now, we will just do a simple reparenting warning or basic hook.
                // It requires unlinking from old parent and linking to new parent.
                CORE_INFO("Dropped entity {0} onto {1}", payloadEntityID, (uint32_t)entity);
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked()) {
            m_SelectedEntity = entity;
            if (m_SelectionCallback) m_SelectionCallback(m_SelectedEntity);
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;
            ImGui::EndPopup();
        }

        if (opened && rel.FirstChild != entt::null) {
            entt::entity curr = rel.FirstChild;
            while(curr != entt::null) {
                DrawEntityNode(Entity{curr, m_Context});
                curr = m_Context->GetRegistry().get<RelationshipComponent>(curr).NextSibling;
            }
            ImGui::TreePop();
        }

        if (entityDeleted) {
            m_Context->DestroyEntity(entity);
            if (m_SelectedEntity == entity) {
                m_SelectedEntity = {};
                if (m_SelectionCallback) m_SelectionCallback(m_SelectedEntity);
            }
        }
    }

}
