#pragma once

#include "EditorPanel.h"

namespace lgt {

    class InspectorPanel : public EditorPanel {
    public:
        InspectorPanel() = default;
        virtual ~InspectorPanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;

    private:
        void DrawComponents(Entity entity);
    };

}
