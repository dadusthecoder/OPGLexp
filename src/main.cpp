#include "Vendor/glad.h"
#include <GLFW/glfw3.h>
#include <vector>

#include "Renderer/Core/Renderer.h"
#include "Renderer/Core/Buffer.h"
#include "Renderer/Resources/Mesh.h"
#include "Renderer/Core/Shader.h"
#include "Renderer/Resources/Material.h"
#include "Renderer/Resources/ModelLoader.h"
#include "Renderer/Core/UIManager.h"
#include <imgui.h>
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Editor/EditorLayer.h"
#include "Core/Input.h"
#include "Helpers/Logger.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    lgt::Renderer::OnWindowResize(width, height);
    lgt::Scene* scene = (lgt::Scene*)glfwGetWindowUserPointer(window);
    if (scene) scene->OnViewportResize(width, height);
}

int main() {
    CORE_LOG_INIT();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "OPGLexp Engine", NULL, NULL);
    if (window == NULL) {
        CORE_CRITICAL("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        CORE_CRITICAL("Failed to initialize GLAD");
        return -1;
    }

    // --- Engine Init ---
    lgt::Renderer::Init();
    lgt::Renderer::SetViewport(0, 0, 1280, 720);
    lgt::Renderer::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

    lgt::UIManager::Init(window);
    lgt::Input::Init(window);

    // --- Scene Setup ---
    lgt::Scene scene;
    glfwSetWindowUserPointer(window, &scene);

    // Camera
    lgt::Entity cameraEntity = scene.CreateEntity("MainCamera");
    cameraEntity.AddComponent<lgt::CameraComponent>();
    auto& cameraTransform = cameraEntity.GetComponent<lgt::TransformComponent>();
    cameraTransform.Translation = glm::vec3(0.0f, 1.0f, 5.0f);

    // Load sphere model
    lgt::Shader* geoShader = lgt::Shader::Create("res/shaders/geometry.glsl");
    lgt::Entity rootSphere = lgt::ModelLoader::LoadModel("res/models/sphere.obj", &scene, geoShader);

    // Get the loaded mesh
    lgt::MeshRendererComponent* baseMeshComp = nullptr;
    auto meshView = scene.GetRegistry().view<lgt::MeshRendererComponent>();
    for (auto entityID : meshView) {
        baseMeshComp = &scene.GetRegistry().get<lgt::MeshRendererComponent>(entityID);
        break;
    }

    if (baseMeshComp) {
        // Create a 10x10 grid of spheres
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                lgt::Entity gridSphere = scene.CreateEntity("Sphere_" + std::to_string(i) + "_" + std::to_string(j));
                gridSphere.AddComponent<lgt::MeshRendererComponent>(*baseMeshComp);
                auto& transform = gridSphere.GetComponent<lgt::TransformComponent>();
                transform.Translation = glm::vec3((i - 5) * 2.5f, 0.0f, (j - 5) * 2.5f);
            }
        }
    }

    // Directional Light
    lgt::Entity lightEntity = scene.CreateEntity("SunLight");
    auto& lightComp = lightEntity.AddComponent<lgt::LightComponent>();
    lightComp.Type = 0;
    lightComp.Color = glm::vec3(1.0f, 0.95f, 0.85f);
    lightComp.Intensity = 5.0f;
    auto& lightTransform = lightEntity.GetComponent<lgt::TransformComponent>();
    lightTransform.Rotation = glm::vec3(glm::radians(-45.0f), glm::radians(45.0f), 0.0f);

    // --- Editor ---
    lgt::EditorLayer editorLayer;
    editorLayer.Init(&scene);

    // --- Game Loop ---
    float lastFrameTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        // Delta time
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        glfwPollEvents();
        lgt::Input::Update();

        if (lgt::Input::IsKeyDown(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, true);

        // --- Update ---
        lgt::Renderer::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        lgt::Renderer::Clear();

        lgt::Renderer::BeginFrame();
        scene.OnUpdate(deltaTime);
        scene.OnRender();
        lgt::Renderer::ExecuteQueue();
        lgt::Renderer::EndFrame();
        
        // --- Editor Update ---
        editorLayer.OnUpdate(deltaTime);

        // --- Editor UI ---
        lgt::UIManager::BeginFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        editorLayer.OnImGuiRender();
        lgt::UIManager::EndFrame();

        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
    lgt::UIManager::Shutdown();
    lgt::Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
