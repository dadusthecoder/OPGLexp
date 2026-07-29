#pragma once

#include "EditorPanel.h"

namespace lgt {

    class ConsolePanel : public EditorPanel {
    public:
        ConsolePanel() = default;
        virtual ~ConsolePanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;

    private:
        bool m_ShowInfo = true;
        bool m_ShowWarnings = true;
        bool m_ShowErrors = true;
        bool m_AutoScroll = true;
    };

}
