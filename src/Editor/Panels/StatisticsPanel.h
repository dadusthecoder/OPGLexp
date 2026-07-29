#pragma once

#include "EditorPanel.h"

namespace lgt {

    class StatisticsPanel : public EditorPanel {
    public:
        StatisticsPanel() = default;
        virtual ~StatisticsPanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;
    };

}
