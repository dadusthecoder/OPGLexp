#pragma once

#include "../../Scene/Scene.h"
#include "../../Scene/Entity.h"

namespace lgt {

    class EditorPanel {
    public:
        virtual ~EditorPanel() = default;

        virtual void OnInit(Scene* context) { m_Context = context; }
        virtual void OnImGuiRender() = 0;
        virtual void OnUpdate(float ts) {}

        void SetContext(Scene* context) { m_Context = context; }
        Scene* GetContext() const { return m_Context; }

        virtual void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }
        Entity GetSelectedEntity() const { return m_SelectedEntity; }

    protected:
        Scene* m_Context = nullptr;
        Entity m_SelectedEntity = {};
    };

}
