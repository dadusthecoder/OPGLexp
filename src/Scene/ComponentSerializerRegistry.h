#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include "Scene/Entity.h"
#include <nlohmann/json.hpp>

namespace lgt {

    struct ComponentDescriptor {
        std::string Name;
        uint32_t Version;
        uint32_t Flags;

        std::function<void(Entity, nlohmann::json&)> Serialize;
        std::function<void(Entity, const nlohmann::json&)> Deserialize;
    };

    class ComponentSerializerRegistry {
    public:
        static void Register(const ComponentDescriptor& descriptor);
        static void RegisterAll();
        static const std::unordered_map<std::string, ComponentDescriptor>& GetRegistry();

        static void SerializeEntity(Entity entity, nlohmann::json& outJson);
        static void DeserializeEntity(Entity entity, const nlohmann::json& inJson);

    private:
        static std::unordered_map<std::string, ComponentDescriptor> s_Registry;
    };

}
