#include "StatisticsPanel.h"
#include <imgui.h>
#include "../../Helpers/DebugStats.h"
#include "../../Renderer/Core/Renderer.h"
#include "../../Renderer/Core/Shader.h"

namespace lgt {

    void StatisticsPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void StatisticsPanel::OnImGuiRender() {
        ImGui::Begin("Statistics");
        
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

        ImGui::Separator();
        if (ImGui::Button("Reload Shaders")) {
            Shader::ReloadAll();
        }
#else
        ImGui::Text("Debug Stats disabled in Dist build.");
#endif

        ImGui::End();
    }

}
