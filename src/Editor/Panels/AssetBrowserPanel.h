#pragma once

#include "EditorPanel.h"
#include <filesystem>

namespace lgt {

    class AssetBrowserPanel : public EditorPanel {
    public:
        AssetBrowserPanel();
        virtual ~AssetBrowserPanel() = default;

        virtual void OnInit(Scene* context) override;
        virtual void OnImGuiRender() override;

    private:
        std::filesystem::path m_CurrentDirectory;
    };

}
