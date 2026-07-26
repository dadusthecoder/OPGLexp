#pragma once
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

namespace lgt {
    class EditorLayer {
    public:
        void Init(Scene* scene);
        void OnImGuiRender();
        
    private:
        Scene* m_Scene = nullptr;
        Entity m_SelectedEntity;
        
        void DrawHierarchyPanel();
        void DrawPropertiesPanel();
        void DrawViewportPanel();
        void DrawConsolePanel();
        void DrawDebugPanel();
    };
}
