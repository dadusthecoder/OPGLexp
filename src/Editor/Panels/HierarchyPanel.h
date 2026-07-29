#pragma once

#include "EditorPanel.h"
#include <functional>

namespace lgt {

    class HierarchyPanel : public EditorPanel {
    public:
        HierarchyPanel() = default;
        virtual ~HierarchyPanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;

        void SetSelectionChangedCallback(const std::function<void(Entity)>& callback) { m_SelectionCallback = callback; }

    private:
        char m_SearchBuffer[128] = { 0 };
        void DrawEntityNode(Entity entity);

    private:
        std::function<void(Entity)> m_SelectionCallback;
    };

}
