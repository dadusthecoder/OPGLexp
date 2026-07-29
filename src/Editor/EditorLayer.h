#pragma once
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/StatisticsPanel.h"
#include "Panels/RenderGraphPanel.h"
#include "Panels/AnimationPanel.h"

namespace lgt {
    class EditorLayer {
    public:
        void Init(Scene* scene);
        void OnUpdate(float ts);
        void OnImGuiRender();
        
    private:
        void DrawMenuBar();
        void DrawToolbar();
        void SetDarkTheme();

    private:
        Scene* m_Scene = nullptr;
        Entity m_SelectedEntity;
        
        float m_CameraSpeed = 5.0f;
        
        // Panels
        HierarchyPanel m_HierarchyPanel;
        InspectorPanel m_InspectorPanel;
        ViewportPanel m_ViewportPanel;
        ConsolePanel m_ConsolePanel;
        AssetBrowserPanel m_AssetBrowserPanel;
        StatisticsPanel m_StatisticsPanel;
        RenderGraphPanel m_RenderGraphPanel;
        AnimationPanel m_AnimationPanel;
        
        bool m_ShowStatistics = false;
        bool m_ShowRenderGraph = false;
        bool m_ShowAnimationPanel = true;
    };
}
