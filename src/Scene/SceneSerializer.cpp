#include "SceneSerializer.h"
#include "Entity.h"
#include "Components.h"
#include "Scene.h"
#include "ComponentSerializerRegistry.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "../Helpers/Logger.h"

namespace lgt {

    void SceneSerializer::Serialize(const std::string& filepath) {
        nlohmann::json sceneJson;
        sceneJson["Version"] = 1;
        sceneJson["EngineVersion"] = "1.0.0";
        sceneJson["Settings"] = nlohmann::json::object();
        sceneJson["Assets"] = nlohmann::json::array();
        
        nlohmann::json entitiesJson = nlohmann::json::array();

        auto view = m_Scene->GetRegistry().view<TagComponent>();
        for (auto entityID : view) {
            Entity entity = { entityID, m_Scene };
            
            nlohmann::json entityJson;
            entityJson["EntityID"] = (uint32_t)entityID;
            
            // Serialize components dynamically using the registry
            ComponentSerializerRegistry::SerializeEntity(entity, entityJson);
            
            entitiesJson.push_back(entityJson);
        }

        sceneJson["Entities"] = entitiesJson;

        std::ofstream out(filepath);
        if (out.is_open()) {
            out << sceneJson.dump(4);
        }
    }

    bool SceneSerializer::Deserialize(const std::string& filepath) {
        std::ifstream in(filepath);
        if (!in.is_open()) return false;

        nlohmann::json sceneJson;
        try {
            in >> sceneJson;
        } catch (const nlohmann::json::parse_error& e) {
            CORE_ERROR("Failed to parse scene JSON: {0}", e.what());
            return false;
        }

        uint32_t version = sceneJson.value("Version", 0);
        if (version < 1) {
            CORE_ERROR("Unsupported scene version: {0}", version);
            return false;
        }

        if (sceneJson.contains("Entities") && sceneJson["Entities"].is_array()) {
            // Stage 1: Read JSON & Create Entities
            // Stage 2: Deserialize Components
            for (const auto& entityJson : sceneJson["Entities"]) {
                uint32_t id = entityJson.value("EntityID", 0); // Need to properly map IDs later
                (void)id;
                
                Entity entity = m_Scene->CreateEntity("Deserialized Entity");
                
                // If it already has a TagComponent from CreateEntity, we let Deserialize overwrite it
                ComponentSerializerRegistry::DeserializeEntity(entity, entityJson);
            }
            
            // Stage 3: Resolve Entity References
            // Stage 4: Resolve Asset Handles
            // Stage 5: Initialize Runtime
            // Stage 6: Bind Scripts
            // Stage 7: OnSceneLoaded
        }

        return true;
    }
}

