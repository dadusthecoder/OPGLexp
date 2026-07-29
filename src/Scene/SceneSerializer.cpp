#include "SceneSerializer.h"
#include "Entity.h"
#include "Components.h"
#include "Scene.h"
#include <fstream>
#include <sstream>

namespace lgt {

    void SceneSerializer::Serialize(const std::string& filepath) {
        std::ofstream out(filepath);
        if (!out.is_open()) return;

        out << "{\n";
        out << "  \"Scene\": \"Untitled\",\n";
        out << "  \"Entities\": [\n";

        bool first = true;
        auto view = m_Scene->GetRegistry().view<TagComponent>();
        for (auto entityID : view) {
            if (!first) {
                out << ",\n";
            }
            first = false;

            Entity entity = { entityID, m_Scene };

            out << "    {\n";
            out << "      \"Entity\": " << (uint32_t)entityID;
            
            if (entity.HasComponent<TagComponent>()) {
                auto& tc = entity.GetComponent<TagComponent>();
                out << ",\n      \"TagComponent\": { \"Tag\": \"" << tc.Tag << "\" }";
            }

            if (entity.HasComponent<TransformComponent>()) {
                auto& tc = entity.GetComponent<TransformComponent>();
                out << ",\n      \"TransformComponent\": {\n";
                out << "        \"Translation\": [" << tc.Translation.x << ", " << tc.Translation.y << ", " << tc.Translation.z << "],\n";
                out << "        \"Rotation\": [" << tc.Rotation.x << ", " << tc.Rotation.y << ", " << tc.Rotation.z << "],\n";
                out << "        \"Scale\": [" << tc.Scale.x << ", " << tc.Scale.y << ", " << tc.Scale.z << "]\n";
                out << "      }";
            }

            if (entity.HasComponent<CameraComponent>()) {
                auto& cc = entity.GetComponent<CameraComponent>();
                out << ",\n      \"CameraComponent\": {\n";
                out << "        \"Primary\": " << (cc.primary ? "true" : "false") << "\n";
                out << "      }";
            }

            if (entity.HasComponent<LightComponent>()) {
                auto& lc = entity.GetComponent<LightComponent>();
                out << ",\n      \"LightComponent\": {\n";
                out << "        \"Color\": [" << lc.Color.x << ", " << lc.Color.y << ", " << lc.Color.z << "],\n";
                out << "        \"Intensity\": " << lc.Intensity << ",\n";
                out << "        \"Type\": " << lc.Type << ",\n";
                out << "        \"Radius\": " << lc.Radius << "\n";
                out << "      }";
            }

            if (entity.HasComponent<MeshRendererComponent>()) {
                out << ",\n      \"MeshRendererComponent\": {\n";
                out << "        \"MeshPath\": \"TODO\",\n";
                out << "        \"MaterialPath\": \"TODO\"\n";
                out << "      }";
            }

            if (entity.HasComponent<NativeScriptComponent>()) {
                auto& nsc = entity.GetComponent<NativeScriptComponent>();
                out << ",\n      \"NativeScriptComponent\": {\n";
                out << "        \"ScriptName\": \"" << nsc.ScriptName << "\"\n";
                out << "      }";
            }

            out << "\n    }";
        }

        out << "\n  ]\n";
        out << "}\n";
    }

    bool SceneSerializer::Deserialize(const std::string& filepath) {
        // TODO: Implement simple state machine or JSON parsing
        return true;
    }
}
