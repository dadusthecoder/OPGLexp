#pragma once

namespace lgt {
    class Entity; // forward declare
    class Scene;  // forward declare
    
    class NativeScript {
    public:
        virtual ~NativeScript() = default;
        
        virtual void OnCreate() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnDestroy() {}
        
        inline Entity GetEntity();
        
        template<typename T>
        inline T& GetComponent();
        
        template<typename T>
        inline bool HasComponent();
        
    protected:
        Entity m_Entity;
        friend class Scene;
    };
}

#include "Entity.h"

namespace lgt {

    inline Entity NativeScript::GetEntity() {
        return m_Entity;
    }
    
    template<typename T>
    inline T& NativeScript::GetComponent() {
        return m_Entity.GetComponent<T>();
    }
    
    template<typename T>
    inline bool NativeScript::HasComponent() {
        return m_Entity.HasComponent<T>();
    }
}
