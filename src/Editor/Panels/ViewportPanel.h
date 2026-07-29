#pragma once

#include "EditorPanel.h"
#include <glm/glm.hpp>

namespace lgt {

    class ViewportPanel : public EditorPanel {
    public:
        ViewportPanel() = default;
        virtual ~ViewportPanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;

        bool IsFocused() const { return m_ViewportFocused; }
        bool IsHovered() const { return m_ViewportHovered; }
        
        // Editor Gizmo type toggles
        int m_GizmoType = -1; // -1 = none, 0 = translate, 1 = rotate, 2 = scale
        int m_GizmoMode = 0; // 0 = local, 1 = world

    private:
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
    };

}
