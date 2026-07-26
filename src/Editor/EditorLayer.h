#pragma once
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

namespace lgt {
    class EditorLayer {
    public:
        void Init(Scene* scene);
        void OnUpdate(float ts);
        void OnImGuiRender();
        
    private:
        Scene* m_Scene = nullptr;
        Entity m_SelectedEntity;
        
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        
        void DrawHierarchyPanel();
        void DrawPropertiesPanel();
        void DrawViewportPanel();
        void DrawConsolePanel();
        void DrawDebugPanel();
    };
}
