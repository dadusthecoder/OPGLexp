#pragma once
#include "EditorPanel.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <imgui-node-editor/imgui_node_editor.h>

namespace lgt {

    namespace ed = ax::NodeEditor;

    enum class PinKind { Input, Output };
    enum class PinType { Texture, Framebuffer, Flow };

    struct Pin {
        ed::PinId ID;
        struct Node* Node;
        std::string Name;
        PinType Type;
        PinKind Kind;
        
        Pin(int id, const char* name, PinType type)
            : ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input) {}
    };

    struct Node {
        ed::NodeId ID;
        std::string Name;
        std::vector<Pin> Inputs;
        std::vector<Pin> Outputs;

        Node(int id, const char* name)
            : ID(id), Name(name) {}
    };

    struct Link {
        ed::LinkId ID;
        ed::PinId StartPinID;
        ed::PinId EndPinID;

        Link(ed::LinkId id, ed::PinId startId, ed::PinId endId)
            : ID(id), StartPinID(startId), EndPinID(endId) {}
    };

    class RenderGraphPanel : public EditorPanel {
    public:
        RenderGraphPanel();
        ~RenderGraphPanel();

        void OnInit(Scene* context) override;
        void OnImGuiRender() override;

    private:
        void DrawPinIcon(const Pin& pin, bool connected, int alpha);
        void CompileGraph();

    private:
        ed::EditorContext* m_EditorContext = nullptr;

        std::vector<Node> m_Nodes;
        std::vector<Link> m_Links;

        int m_NextId = 1;
        int GetNextId() { return m_NextId++; }

        int m_NextLinkId = 100;
        int GetNextLinkId() { return m_NextLinkId++; }
        
        // Caching for topological sort and output
        std::string m_CompilerOutput;
    };

}
