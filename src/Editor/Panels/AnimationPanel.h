#pragma once
#include "EditorPanel.h"

namespace lgt {

    class AnimationPanel : public EditorPanel {
    public:
        AnimationPanel();
        ~AnimationPanel();

        void OnInit(Scene* context) override;
        void OnImGuiRender() override;

    private:
        Scene* m_Context = nullptr;
    };

}
