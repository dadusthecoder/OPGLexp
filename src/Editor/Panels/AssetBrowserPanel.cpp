#include "AssetBrowserPanel.h"
#include <imgui.h>

namespace lgt {

    extern const std::filesystem::path g_AssetPath = "res";

    AssetBrowserPanel::AssetBrowserPanel()
        : m_CurrentDirectory(g_AssetPath)
    {
    }

    void AssetBrowserPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void AssetBrowserPanel::OnImGuiRender() {
        ImGui::Begin("Asset Browser");

        // Split into two panes
        ImGui::Columns(2, "AssetBrowserColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.25f);

        // Left Pane: Folder Tree (just simple hardcoded root for now, or recursive)
        if (ImGui::TreeNodeEx(g_AssetPath.string().c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow)) {
            // Very simple non-recursive for now, just root dirs
            for (auto& directoryEntry : std::filesystem::directory_iterator(g_AssetPath)) {
                if (directoryEntry.is_directory()) {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                    if (m_CurrentDirectory == directoryEntry.path()) flags |= ImGuiTreeNodeFlags_Selected;
                    
                    bool opened = ImGui::TreeNodeEx(directoryEntry.path().filename().string().c_str(), flags);
                    if (ImGui::IsItemClicked()) {
                        m_CurrentDirectory = directoryEntry.path();
                    }
                    if (opened) ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        ImGui::NextColumn();

        // Right Pane: Grid View
        if (m_CurrentDirectory != std::filesystem::path(g_AssetPath)) {
            if (ImGui::Button("<- Back")) {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
            }
            ImGui::SameLine();
        }
        ImGui::Text("%s", m_CurrentDirectory.string().c_str());
        ImGui::Separator();

        static float padding = 16.0f;
        static float thumbnailSize = 64.0f;
        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        if (ImGui::BeginTable("AssetBrowserTable", columnCount)) {
            for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory)) {
                const auto& path = directoryEntry.path();
                auto relativePath = std::filesystem::relative(path, g_AssetPath);
                std::string filenameString = relativePath.filename().string();

                ImGui::TableNextColumn();

                ImGui::PushID(filenameString.c_str());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                
                std::string icon = directoryEntry.is_directory() ? "[Dir]" : "[Asset]";
                if (ImGui::Button(icon.c_str(), { thumbnailSize, thumbnailSize })) {
                    if (directoryEntry.is_directory()) {
                        m_CurrentDirectory /= path.filename();
                    }
                }
                ImGui::PopStyleColor();
                
                ImGui::TextWrapped("%s", filenameString.c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::Columns(1);

        ImGui::End();
    }

}
