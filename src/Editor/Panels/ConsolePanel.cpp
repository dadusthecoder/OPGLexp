#include "ConsolePanel.h"
#include <imgui.h>
#include "../../Helpers/Logger.h"

namespace lgt {

    void ConsolePanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void ConsolePanel::OnImGuiRender() {
        ImGui::Begin("Console");
        
#ifndef LGT_DIST
        // Toolbar for filters
        if (ImGui::Button("Clear")) {
            auto consoleSink = Log::GetConsoleSink();
            if (consoleSink) consoleSink->clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-Scroll", &m_AutoScroll);
        ImGui::SameLine();
        ImGui::Text("| Filters:"); ImGui::SameLine();
        ImGui::Checkbox("Info", &m_ShowInfo); ImGui::SameLine();
        ImGui::Checkbox("Warnings", &m_ShowWarnings); ImGui::SameLine();
        ImGui::Checkbox("Errors", &m_ShowErrors);
        ImGui::Separator();

        auto consoleSink = Log::GetConsoleSink();
        if (consoleSink) {
            auto messages = consoleSink->get_messages();
            for (const auto& msg : messages) {
                bool show = false;
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                
                if (msg.level == spdlog::level::trace || msg.level == spdlog::level::info) {
                    if (m_ShowInfo) show = true;
                    color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                }
                else if (msg.level == spdlog::level::warn) {
                    if (m_ShowWarnings) show = true;
                    color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                }
                else if (msg.level >= spdlog::level::err) {
                    if (m_ShowErrors) show = true;
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                }

                if (show) {
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextWrapped("%s", msg.text.c_str());
                    ImGui::PopStyleColor();
                }
            }
            
            // Auto-scroll to bottom if at the bottom
            // Auto-scroll to bottom if at the bottom
            if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
#else
        ImGui::Text("Console disabled in Dist build.");
#endif

        ImGui::End();
    }

}
