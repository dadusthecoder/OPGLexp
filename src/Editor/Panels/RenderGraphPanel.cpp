#include "RenderGraphPanel.h"
#include <imgui.h>
#include "../../Helpers/Logger.h"
#include <algorithm>
#include <set>

namespace lgt {

    RenderGraphPanel::RenderGraphPanel() {
        ed::Config config;
        config.SettingsFile = "RenderGraph.json";
        m_EditorContext = ed::CreateEditor(&config);

        // Create some initial dummy nodes to show how it works
        Node geomNode(GetNextId(), "Geometry Pass");
        geomNode.Inputs.emplace_back(GetNextId(), "Models", PinType::Flow);
        geomNode.Outputs.emplace_back(GetNextId(), "G-Buffer", PinType::Framebuffer);
        geomNode.Outputs.emplace_back(GetNextId(), "Depth", PinType::Texture);
        
        for (auto& pin : geomNode.Inputs) { pin.Node = &geomNode; pin.Kind = PinKind::Input; }
        for (auto& pin : geomNode.Outputs) { pin.Node = &geomNode; pin.Kind = PinKind::Output; }
        m_Nodes.push_back(geomNode);

        Node lightNode(GetNextId(), "Lighting Pass");
        lightNode.Inputs.emplace_back(GetNextId(), "G-Buffer", PinType::Framebuffer);
        lightNode.Outputs.emplace_back(GetNextId(), "HDR Color", PinType::Texture);
        
        for (auto& pin : lightNode.Inputs) { pin.Node = &lightNode; pin.Kind = PinKind::Input; }
        for (auto& pin : lightNode.Outputs) { pin.Node = &lightNode; pin.Kind = PinKind::Output; }
        m_Nodes.push_back(lightNode);
    }

    RenderGraphPanel::~RenderGraphPanel() {
        if (m_EditorContext)
            ed::DestroyEditor(m_EditorContext);
    }

    void RenderGraphPanel::OnInit(Scene* context) {
        EditorPanel::OnInit(context);
    }

    void RenderGraphPanel::DrawPinIcon(const Pin& pin, bool connected, int alpha) {
        // Just draw a colored circle or square based on PinType
        ImVec2 size(24, 24);
        if (ImGui::IsRectVisible(size)) {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            ImColor color = ImColor(255, 255, 255);
            if (pin.Type == PinType::Framebuffer) color = ImColor(200, 100, 100);
            if (pin.Type == PinType::Texture) color = ImColor(100, 200, 100);
            if (pin.Type == PinType::Flow) color = ImColor(100, 100, 200);

            if (connected) {
                drawList->AddCircleFilled(ImVec2(cursorPos.x + size.x/2, cursorPos.y + size.y/2), size.x/3.0f, color);
            } else {
                drawList->AddCircle(ImVec2(cursorPos.x + size.x/2, cursorPos.y + size.y/2), size.x/3.0f, color, 0, 2.0f);
            }
        }
        ImGui::Dummy(size);
    }

    void RenderGraphPanel::CompileGraph() {
        m_CompilerOutput = "Compiling Graph...\n";
        
        // Build adjacency list: nodeID -> list of nodeIDs it depends on
        std::unordered_map<int, std::vector<int>> dependencies;
        std::unordered_map<int, int> inDegree;
        
        for (const auto& node : m_Nodes) {
            dependencies[(int)node.ID.Get()] = {};
            inDegree[(int)node.ID.Get()] = 0;
        }

        // Populate dependencies based on links
        for (const auto& link : m_Links) {
            // Find which node owns the start pin (Output) and end pin (Input)
            int outputNodeId = -1;
            int inputNodeId = -1;

            for (const auto& node : m_Nodes) {
                for (const auto& pin : node.Outputs) {
                    if (pin.ID == link.StartPinID) outputNodeId = (int)node.ID.Get();
                }
                for (const auto& pin : node.Inputs) {
                    if (pin.ID == link.EndPinID) inputNodeId = (int)node.ID.Get();
                }
            }

            if (outputNodeId != -1 && inputNodeId != -1) {
                // inputNode depends on outputNode
                dependencies[outputNodeId].push_back(inputNodeId);
                inDegree[inputNodeId]++;
            }
        }

        // Kahn's algorithm for topological sorting
        std::vector<int> sortedOrder;
        std::vector<int> queue;

        // Find nodes with 0 in-degree
        for (const auto& pair : inDegree) {
            if (pair.second == 0) queue.push_back(pair.first);
        }

        while (!queue.empty()) {
            int current = queue.front();
            queue.erase(queue.begin());
            sortedOrder.push_back(current);

            for (int neighbor : dependencies[current]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.push_back(neighbor);
                }
            }
        }

        if (sortedOrder.size() != m_Nodes.size()) {
            m_CompilerOutput += "[ERROR] Cycle detected in Render Graph!\n";
            CORE_ERROR("Cycle detected in Render Graph!");
        } else {
            m_CompilerOutput += "Success: Valid acyclic render graph generated.\nExecution Order:\n";
            for (size_t i = 0; i < sortedOrder.size(); i++) {
                int id = sortedOrder[i];
                auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [id](const Node& n) { return (int)n.ID.Get() == id; });
                if (it != m_Nodes.end()) {
                    m_CompilerOutput += std::to_string(i + 1) + ". " + it->Name + "\n";
                }
            }
            CORE_INFO("RenderGraph successfully compiled with {0} passes.", sortedOrder.size());
        }
    }

    void RenderGraphPanel::OnImGuiRender() {
        ImGui::Begin("Render Graph");

        if (ImGui::Button("Compile")) {
            CompileGraph();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", m_CompilerOutput.c_str());

        ed::SetCurrentEditor(m_EditorContext);
        ed::Begin("My Node Editor");

        for (auto& node : m_Nodes) {
            ed::BeginNode(node.ID);
            ImGui::Text("%s", node.Name.c_str());
            
            ImGui::BeginGroup();
            for (auto& pin : node.Inputs) {
                ed::BeginPin(pin.ID, ed::PinKind::Input);
                ImGui::Text("-> %s", pin.Name.c_str());
                ed::EndPin();
            }
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            for (auto& pin : node.Outputs) {
                ed::BeginPin(pin.ID, ed::PinKind::Output);
                ImGui::Text("%s ->", pin.Name.c_str());
                ed::EndPin();
            }
            ImGui::EndGroup();

            ed::EndNode();
        }

        for (auto& link : m_Links) {
            ed::Link(link.ID, link.StartPinID, link.EndPinID);
        }

        // Handle creation
        if (ed::BeginCreate()) {
            ed::PinId startPinId = 0, endPinId = 0;
            if (ed::QueryNewLink(&startPinId, &endPinId)) {
                if (startPinId && endPinId) {
                    if (ed::AcceptNewItem()) {
                        m_Links.emplace_back(GetNextLinkId(), startPinId, endPinId);
                    }
                }
            }
        }
        ed::EndCreate();

        // Handle deletion
        if (ed::BeginDelete()) {
            ed::LinkId deletedLinkId = 0;
            while (ed::QueryDeletedLink(&deletedLinkId)) {
                if (ed::AcceptDeletedItem()) {
                    m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
                        [deletedLinkId](const Link& link) { return link.ID == deletedLinkId; }),
                        m_Links.end());
                }
            }
        }
        ed::EndDelete();
        
        // Context Menu
        ed::Suspend();
        if (ed::ShowBackgroundContextMenu()) {
            ImGui::OpenPopup("Create Node");
        }
        if (ImGui::BeginPopup("Create Node")) {
            if (ImGui::MenuItem("Geometry Pass")) {
                Node n(GetNextId(), "Geometry Pass");
                n.Outputs.emplace_back(GetNextId(), "G-Buffer", PinType::Framebuffer);
                n.Outputs.emplace_back(GetNextId(), "Depth", PinType::Texture);
                for (auto& pin : n.Outputs) { pin.Node = &n; pin.Kind = PinKind::Output; }
                m_Nodes.push_back(n);
                ed::SetNodePosition(n.ID, ImGui::GetMousePosOnOpeningCurrentPopup());
            }
            if (ImGui::MenuItem("Lighting Pass")) {
                Node n(GetNextId(), "Lighting Pass");
                n.Inputs.emplace_back(GetNextId(), "G-Buffer", PinType::Framebuffer);
                n.Outputs.emplace_back(GetNextId(), "HDR Color", PinType::Texture);
                for (auto& pin : n.Inputs) { pin.Node = &n; pin.Kind = PinKind::Input; }
                for (auto& pin : n.Outputs) { pin.Node = &n; pin.Kind = PinKind::Output; }
                m_Nodes.push_back(n);
                ed::SetNodePosition(n.ID, ImGui::GetMousePosOnOpeningCurrentPopup());
            }
            ImGui::EndPopup();
        }
        ed::Resume();

        ed::End();
        ed::SetCurrentEditor(nullptr);

        ImGui::End();
    }

}
