#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "../Renderer/Resources/ModelLoader.h"
#include "../Renderer/Core/Shader.h"

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
        if (!entity.HasComponent<RelationshipComponent>()) {
            m_Registry.destroy(entity);
            return;
        }

        auto& rel = entity.GetComponent<RelationshipComponent>();
        
        // Unparent from parent
        if (rel.Parent != entt::null) {
            Entity parent{rel.Parent, this};
            auto& parentRel = parent.GetComponent<RelationshipComponent>();
            
            if (parentRel.FirstChild == entity) {
                parentRel.FirstChild = rel.NextSibling;
            }
            if (rel.PrevSibling != entt::null) {
                m_Registry.get<RelationshipComponent>(rel.PrevSibling).NextSibling = rel.NextSibling;
            }
            if (rel.NextSibling != entt::null) {
                m_Registry.get<RelationshipComponent>(rel.NextSibling).PrevSibling = rel.PrevSibling;
            }
            parentRel.ChildrenCount--;
        }

        // Recursively destroy children
        entt::entity curr = rel.FirstChild;
        while (curr != entt::null) {
            entt::entity next = m_Registry.get<RelationshipComponent>(curr).NextSibling;
            
            // Temporarily clear the child's parent so it doesn't try to unparent from us while we are destroying
            m_Registry.get<RelationshipComponent>(curr).Parent = entt::null;
            
            DestroyEntity(Entity{curr, this});
            curr = next;
        }

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
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        
        // Find main camera
        auto cameraGroup = m_Registry.group<CameraComponent>(entt::get<TransformComponent>);
        for (auto entity : cameraGroup) {
            auto [cameraComp, transform] = cameraGroup.get<CameraComponent, TransformComponent>(entity);
            if (cameraComp.primary) {
                // View is inverse of transform
                cameraView = glm::inverse(transform.GlobalTransform);
                cameraProj = cameraComp.camera.GetProjection();
                cameraPos = transform.GlobalTransform[3]; // The translation part
                nearPlane = cameraComp.camera.GetPerspectiveNearClip();
                farPlane = cameraComp.camera.GetPerspectiveFarClip();
                break;
            }
        }
        
        Renderer::BeginScene(cameraView, cameraProj, cameraPos, nearPlane, farPlane);
        
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

    void Scene::CreateValidationScene(Scene* scene, class Shader* geoShader, int type) {
        // Destroy existing entities
        auto view = scene->m_Registry.view<entt::entity>();
        scene->m_Registry.destroy(view.begin(), view.end());

        // Camera
        Entity cameraEntity = scene->CreateEntity("MainCamera");
        cameraEntity.AddComponent<CameraComponent>();
        auto& cameraTransform = cameraEntity.GetComponent<TransformComponent>();
        cameraTransform.Translation = glm::vec3(0.0f, 1.0f, 5.0f);

        if (type == 1) {
            // Scene 1: Geometry (Plane, Sphere, Cube, Directional Light)
            Entity plane = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            plane.GetComponent<TagComponent>().Tag = "Plane";
            plane.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity sphere = ModelLoader::LoadModel("res/models/sphere.obj", scene, geoShader);
            sphere.GetComponent<TagComponent>().Tag = "Sphere";
            sphere.GetComponent<TransformComponent>().Translation = glm::vec3(-2.0f, 1.0f, 0.0f);

            Entity cube = ModelLoader::LoadModel("res/models/cube.obj", scene, geoShader);
            cube.GetComponent<TagComponent>().Tag = "Cube";
            cube.GetComponent<TransformComponent>().Translation = glm::vec3(2.0f, 0.5f, 0.0f);

            Entity lightEntity = scene->CreateEntity("SunLight");
            auto& lightComp = lightEntity.AddComponent<LightComponent>();
            lightComp.Type = 0;
            lightComp.Color = glm::vec3(1.0f, 0.95f, 0.85f);
            lightComp.Intensity = 8.0f;
            lightEntity.GetComponent<TransformComponent>().Rotation = glm::vec3(glm::radians(-60.0f), glm::radians(30.0f), 0.0f);

        } else if (type == 2 || type == 3) {
            // Scene 2 & 3: Cornell Box style + Indirect Testing
            Entity floor = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            floor.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity leftWall = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            leftWall.GetComponent<TransformComponent>().Translation = glm::vec3(-5.0f, 5.0f, 0.0f);
            leftWall.GetComponent<TransformComponent>().Rotation = glm::vec3(0.0f, 0.0f, glm::radians(-90.0f));
            leftWall.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity rightWall = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            rightWall.GetComponent<TransformComponent>().Translation = glm::vec3(5.0f, 5.0f, 0.0f);
            rightWall.GetComponent<TransformComponent>().Rotation = glm::vec3(0.0f, 0.0f, glm::radians(90.0f));
            rightWall.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity backWall = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            backWall.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 5.0f, -5.0f);
            backWall.GetComponent<TransformComponent>().Rotation = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f);
            backWall.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity ceiling = ModelLoader::LoadModel("res/models/plane.obj", scene, geoShader);
            ceiling.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 10.0f, 0.0f);
            ceiling.GetComponent<TransformComponent>().Rotation = glm::vec3(glm::radians(180.0f), 0.0f, 0.0f);
            ceiling.GetComponent<TransformComponent>().Scale = glm::vec3(5.0f);

            Entity centerSphere = ModelLoader::LoadModel("res/models/sphere.obj", scene, geoShader);
            centerSphere.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 2.0f, 0.0f);

            // Add emissive material to sphere in Scene 2
            if (centerSphere.HasComponent<MeshRendererComponent>()) {
                auto& meshComp = centerSphere.GetComponent<MeshRendererComponent>();
                if (meshComp.mesh && meshComp.material) {
                    meshComp.material->Emissive = glm::vec3(2.0f, 1.0f, 0.5f);
                }
            }

            Entity lightEntity = scene->CreateEntity("SunLight");
            auto& lightComp = lightEntity.AddComponent<LightComponent>();
            lightComp.Type = 0;
            lightComp.Color = glm::vec3(0.5f);
            lightComp.Intensity = 2.0f;
            lightEntity.GetComponent<TransformComponent>().Rotation = glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f);
        }
    }

}
