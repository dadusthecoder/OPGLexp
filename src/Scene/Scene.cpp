#include "Scene.h"
#include "Entity.h"
#include "Components.h"

namespace lgt {

    void Scene::UpdateTransformRecursive(entt::entity entity, const glm::mat4& parentTransform) {
        auto& transform = m_Registry.get<TransformComponent>(entity);
        transform.GlobalTransform = parentTransform * transform.GetLocalTransform();

        auto& rel = m_Registry.get<RelationshipComponent>(entity);
        entt::entity curr = rel.FirstChild;
        while(curr != entt::null) {
            UpdateTransformRecursive(curr, transform.GlobalTransform);
            curr = m_Registry.get<RelationshipComponent>(curr).NextSibling;
        }
    }

    Scene::Scene() {
    }

    Scene::~Scene() {
    }

    Entity Scene::CreateEntity(const std::string& name) {
        Entity entity = { m_Registry.create(), this };
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<RelationshipComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity) {
        m_Registry.destroy(entity);
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height) {
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity : view) {
            auto& cameraComp = view.get<CameraComponent>(entity);
            if (cameraComp.camera.GetProjectionType() != SceneCamera::ProjectionType::Orthographic) {
                cameraComp.camera.SetViewportSize(width, height);
            } else {
                cameraComp.camera.SetViewportSize(width, height);
            }
        }
    }

    void Scene::OnUpdate(float deltaTime) {
        // --- NativeScript lifecycle ---
        auto scriptView = m_Registry.view<NativeScriptComponent>();
        for (auto entity : scriptView) {
            auto& scriptComp = scriptView.get<NativeScriptComponent>(entity);
            
            if (!scriptComp.instance && scriptComp.instantiateScript) {
                scriptComp.instance = scriptComp.instantiateScript();
                scriptComp.instance->m_Entity = Entity{ entity, this };
                scriptComp.instance->OnCreate();
                scriptComp.created = true;
            }
            
            if (scriptComp.instance) {
                scriptComp.instance->OnUpdate(deltaTime);
            }
        }

        // Update global transforms based on local transforms recursively
        auto view = m_Registry.view<TransformComponent, RelationshipComponent>();
        for (auto entity : view) {
            auto& rel = view.get<RelationshipComponent>(entity);
            if (rel.Parent == entt::null) { // Root nodes only
                UpdateTransformRecursive(entity, glm::mat4(1.0f));
            }
        }
    }

    void Scene::OnRender() {
        glm::mat4 cameraView = glm::mat4(1.0f);
        glm::mat4 cameraProj = glm::mat4(1.0f);
        glm::vec3 cameraPos = glm::vec3(0.0f);
        
        // Find main camera
        auto cameraGroup = m_Registry.group<CameraComponent>(entt::get<TransformComponent>);
        for (auto entity : cameraGroup) {
            auto [cameraComp, transform] = cameraGroup.get<CameraComponent, TransformComponent>(entity);
            if (cameraComp.primary) {
                // View is inverse of transform
                cameraView = glm::inverse(transform.GlobalTransform);
                cameraProj = cameraComp.camera.GetProjection();
                cameraPos = transform.GlobalTransform[3]; // The translation part
                break;
            }
        }
        
        Renderer::BeginScene(cameraView, cameraProj, cameraPos);
        
        // Submit lights
        auto lightView = m_Registry.view<TransformComponent, LightComponent>();
        for (auto entity : lightView) {
            auto [transform, lightComp] = lightView.get<TransformComponent, LightComponent>(entity);
            
            Renderer::LightData lightData;
            lightData.Position = transform.Translation;
            
            // Assuming Direction is forward vector from rotation
            glm::mat4 rotation = glm::mat4_cast(glm::quat(transform.Rotation));
            lightData.Direction = glm::vec3(rotation * glm::vec4(0, 0, -1, 0));
            
            lightData.Color = lightComp.Color;
            lightData.Intensity = lightComp.Intensity;
            lightData.Type = lightComp.Type;
            lightData.Radius = lightComp.Radius;
            
            Renderer::SubmitLight(lightData);
        }
        
        // Submit all mesh components to renderer
        auto group = m_Registry.group<TransformComponent>(entt::get<MeshRendererComponent>);
        for (auto entity : group) {
            auto [transform, meshComp] = group.get<TransformComponent, MeshRendererComponent>(entity);
            
            if (meshComp.mesh && meshComp.material) {
                RenderCommand cmd;
                cmd.mesh = meshComp.mesh;
                cmd.indexCount = meshComp.mesh->GetIndexCount();
                cmd.transform = transform.GlobalTransform;
                cmd.material = meshComp.material;
                
                Renderer::Submit(cmd);
            }
        }
    }

}
