#include "ComponentSerializerRegistry.h"
#include "Components.h"

namespace lgt {

    std::unordered_map<std::string, ComponentDescriptor> ComponentSerializerRegistry::s_Registry;

    void ComponentSerializerRegistry::Register(const ComponentDescriptor& descriptor) {
        s_Registry[descriptor.Name] = descriptor;
    }

    const std::unordered_map<std::string, ComponentDescriptor>& ComponentSerializerRegistry::GetRegistry() {
        return s_Registry;
    }

    void ComponentSerializerRegistry::SerializeEntity(Entity entity, nlohmann::json& outJson) {
        for (const auto& [name, descriptor] : s_Registry) {
            descriptor.Serialize(entity, outJson);
        }
    }

    void ComponentSerializerRegistry::RegisterAll() {
        Register({
            "TagComponent", 1, 0,
            [](Entity entity, nlohmann::json& outJson) {
                if (entity.HasComponent<TagComponent>()) {
                    outJson["TagComponent"] = { {"Tag", entity.GetComponent<TagComponent>().Tag} };
                }
            },
            [](Entity entity, const nlohmann::json& inJson) {
                auto& tc = entity.HasComponent<TagComponent>() ? entity.GetComponent<TagComponent>() : entity.AddComponent<TagComponent>();
                tc.Tag = inJson.value("Tag", "Entity");
            }
        });

        Register({
            "TransformComponent", 1, 0,
            [](Entity entity, nlohmann::json& outJson) {
                if (entity.HasComponent<TransformComponent>()) {
                    auto& tc = entity.GetComponent<TransformComponent>();
                    outJson["TransformComponent"] = {
                        {"Translation", {tc.Translation.x, tc.Translation.y, tc.Translation.z}},
                        {"Rotation", {tc.Rotation.x, tc.Rotation.y, tc.Rotation.z}},
                        {"Scale", {tc.Scale.x, tc.Scale.y, tc.Scale.z}}
                    };
                }
            },
            [](Entity entity, const nlohmann::json& inJson) {
                auto& tc = entity.AddComponent<TransformComponent>();
                if (inJson.contains("Translation")) {
                    auto trans = inJson["Translation"];
                    tc.Translation = {trans[0], trans[1], trans[2]};
                }
                if (inJson.contains("Rotation")) {
                    auto rot = inJson["Rotation"];
                    tc.Rotation = {rot[0], rot[1], rot[2]};
                }
                if (inJson.contains("Scale")) {
                    auto scale = inJson["Scale"];
                    tc.Scale = {scale[0], scale[1], scale[2]};
                }
            }
        });

        Register({
            "CameraComponent", 1, 0,
            [](Entity entity, nlohmann::json& outJson) {
                if (entity.HasComponent<CameraComponent>()) {
                    auto& cc = entity.GetComponent<CameraComponent>();
                    outJson["CameraComponent"] = {
                        {"Primary", cc.primary}
                    };
                }
            },
            [](Entity entity, const nlohmann::json& inJson) {
                auto& cc = entity.AddComponent<CameraComponent>();
                cc.primary = inJson.value("Primary", true);
            }
        });

        Register({
            "LightComponent", 1, 0,
            [](Entity entity, nlohmann::json& outJson) {
                if (entity.HasComponent<LightComponent>()) {
                    auto& lc = entity.GetComponent<LightComponent>();
                    outJson["LightComponent"] = {
                        {"Color", {lc.Color.x, lc.Color.y, lc.Color.z}},
                        {"Intensity", lc.Intensity},
                        {"Type", lc.Type},
                        {"Radius", lc.Radius}
                    };
                }
            },
            [](Entity entity, const nlohmann::json& inJson) {
                auto& lc = entity.AddComponent<LightComponent>();
                if (inJson.contains("Color")) {
                    auto color = inJson["Color"];
                    lc.Color = {color[0], color[1], color[2]};
                }
                lc.Intensity = inJson.value("Intensity", 1.0f);
                lc.Type = inJson.value("Type", 0);
                lc.Radius = inJson.value("Radius", 10.0f);
            }
        });

        // MeshRendererComponent and others will use AssetManager handles in the new architecture
        // For now, we stub them to not break the build
        Register({
            "MeshRendererComponent", 1, 0,
            [](Entity entity, nlohmann::json& outJson) {
                if (entity.HasComponent<MeshRendererComponent>()) {
                    outJson["MeshRendererComponent"] = {
                        {"Mesh", 0}, // UUID representation
                        {"Material", 0}
                    };
                }
            },
            [](Entity entity, const nlohmann::json& inJson) {
                entity.AddComponent<MeshRendererComponent>();
                // We'll map AssetHandle<Mesh> here once integrated
            }
        });
    }

    void ComponentSerializerRegistry::DeserializeEntity(Entity entity, const nlohmann::json& inJson) {
        for (const auto& [name, descriptor] : s_Registry) {
            if (inJson.contains(name)) {
                descriptor.Deserialize(entity, inJson[name]);
            }
        }
    }

}
