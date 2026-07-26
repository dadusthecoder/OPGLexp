#pragma once

#include <entt/entt.hpp>
#include <string>
#include "../Renderer/Core/Renderer.h"

namespace lgt {

    class Entity;

    class Scene {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(Entity entity);

        void OnUpdate(float deltaTime = 0.0f);
        void OnRender();
        
        void OnViewportResize(uint32_t width, uint32_t height);

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        entt::registry m_Registry;

        friend class Entity;
    };

}
