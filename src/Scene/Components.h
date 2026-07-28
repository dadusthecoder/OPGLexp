#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "SceneCamera.h"
#include <functional>

#include "../Renderer/Resources/Mesh.h"
#include "NativeScript.h"

namespace lgt {

    struct IDComponent {
        uint64_t ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        IDComponent(uint64_t id) : ID(id) {}
    };

    struct TagComponent {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

    struct TransformComponent {
        glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

        // Used by Scene to calculate world transforms
        glm::mat4 GlobalTransform = glm::mat4(1.0f);

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& translation)
            : Translation(translation) {}

        glm::mat4 GetLocalTransform() const {
            glm::mat4 rotation = glm::mat4_cast(glm::quat(Rotation));
            
            return glm::translate(glm::mat4(1.0f), Translation)
                * rotation
                * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct RelationshipComponent {
        entt::entity Parent = entt::null;
        entt::entity FirstChild = entt::null;
        entt::entity PrevSibling = entt::null;
        entt::entity NextSibling = entt::null;
        uint32_t ChildrenCount = 0;

        RelationshipComponent() = default;
        RelationshipComponent(const RelationshipComponent&) = default;
    };

    class Material;

    struct MeshRendererComponent {
        Mesh* mesh = nullptr;
        Material* material = nullptr;
        
        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent&) = default;
        MeshRendererComponent(Mesh* m, Material* mat) : mesh(m), material(mat) {}
    };

    struct LightComponent {
        glm::vec3 Color = glm::vec3(1.0f);
        float Intensity = 1.0f;
        
        // 0 = Directional, 1 = Point
        int Type = 0; 
        
        // Only used for point lights
        float Radius = 10.0f; 

        LightComponent() = default;
        LightComponent(const LightComponent&) = default;
    };

    struct CameraComponent {
        SceneCamera camera;
        bool primary = true;
        
        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    struct NativeScriptComponent {
        NativeScript* instance = nullptr;
        std::function<NativeScript*()> instantiateScript;
        std::function<void(NativeScript*)> destroyScript;
        bool created = false;
        
        template<typename T>
        void Bind() {
            instantiateScript = []() { return static_cast<NativeScript*>(new T()); };
            destroyScript = [](NativeScript* s) { delete s; s = nullptr; };
        }
        
        ~NativeScriptComponent() {
            if (instance && destroyScript) {
                destroyScript(instance);
                instance = nullptr;
            }
        }
    };

}
