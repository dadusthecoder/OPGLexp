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

#ifdef _WIN32
#define DEBUG_CALLBACK_APIENTRY __stdcall
#else
#define DEBUG_CALLBACK_APIENTRY
#endif

void DEBUG_CALLBACK_APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam) {
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 
    if(type == GL_DEBUG_TYPE_PERFORMANCE) return; // Suppress buffer performance warnings

    std::string sourceStr, typeStr;
    switch (source) {
        case GL_DEBUG_SOURCE_API:             sourceStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other"; break;
    }
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
    }
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         CORE_CRITICAL("OpenGL ({}) [{}]: {}", sourceStr, typeStr, message); __debugbreak(); break;
        case GL_DEBUG_SEVERITY_MEDIUM:       CORE_ERROR("OpenGL ({}) [{}]: {}", sourceStr, typeStr, message); __debugbreak(); break;
        case GL_DEBUG_SEVERITY_LOW:          CORE_WARN("OpenGL ({}) [{}]: {}", sourceStr, typeStr, message); break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: CORE_INFO("OpenGL ({}) [{}]: {}", sourceStr, typeStr, message); break;
    }
}

int main() {
    CORE_LOG_INIT();

    CORE_INFO("main: calling glfwInit");
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef LGT_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    CORE_INFO("main: creating window");
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OPGLexp Engine", NULL, NULL);
    if (window == NULL) {
        CORE_CRITICAL("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    CORE_INFO("main: initializing glad");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        CORE_CRITICAL("Failed to initialize GLAD");
        return -1;
    }

#ifdef LGT_DEBUG
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

    CORE_INFO("main: calling Renderer::Init");
    // --- Engine Init ---
    lgt::Renderer::Init();
    lgt::Renderer::SetViewport(0, 0, 1280, 720);
    lgt::Renderer::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

#ifndef LGT_DIST
    CORE_INFO("main: initializing UIManager");
    lgt::UIManager::Init(window);
#endif
    lgt::Input::Init(window);

    // --- Scene Setup ---
    lgt::Scene scene;
    glfwSetWindowUserPointer(window, &scene);

    CORE_INFO("main: setting up camera");
    // Camera
    lgt::Entity cameraEntity = scene.CreateEntity("MainCamera");
    cameraEntity.AddComponent<lgt::CameraComponent>();
    auto& cameraTransform = cameraEntity.GetComponent<lgt::TransformComponent>();
    cameraTransform.Translation = glm::vec3(0.0f, 1.0f, 5.0f);

    CORE_INFO("main: loading Sponza model");
    // Load Sponza model
    lgt::Shader* geoShader = lgt::Shader::Create("res/shaders/geometry.glsl");
    lgt::Entity rootSponza = lgt::ModelLoader::LoadModel("res/models/sponza/sponza.obj", &scene, geoShader);
    (void)rootSponza;
    
    // Scale Sponza (Now handled directly in ModelLoader::LoadModel)

    // Primary Directional Sun Light
    lgt::Entity lightEntity = scene.CreateEntity("SunLight");
    auto& lightComp = lightEntity.AddComponent<lgt::LightComponent>();
    lightComp.Type = 0;
    lightComp.Color = glm::vec3(1.0f, 0.95f, 0.85f);
    lightComp.Intensity = 8.0f;
    auto& lightTransform = lightEntity.GetComponent<lgt::TransformComponent>();
    lightTransform.Rotation = glm::vec3(glm::radians(-60.0f), glm::radians(30.0f), 0.0f);

    // Secondary Fill Light (Sky bounce from opposite side)
    lgt::Entity fillLightEntity = scene.CreateEntity("FillLight");
    auto& fillLightComp = fillLightEntity.AddComponent<lgt::LightComponent>();
    fillLightComp.Type = 0;
    fillLightComp.Color = glm::vec3(0.6f, 0.75f, 0.95f);
    fillLightComp.Intensity = 3.0f;
    auto& fillLightTransform = fillLightEntity.GetComponent<lgt::TransformComponent>();
    fillLightTransform.Rotation = glm::vec3(glm::radians(-45.0f), glm::radians(-135.0f), 0.0f);

    // --- Editor ---
#ifndef LGT_DIST
    lgt::EditorLayer editorLayer;
    editorLayer.Init(&scene);
#endif

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
        
#ifndef LGT_DIST
        // --- Editor Update ---
        editorLayer.OnUpdate(deltaTime);

        // --- Editor UI ---
        lgt::UIManager::BeginFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        editorLayer.OnImGuiRender();
        lgt::UIManager::EndFrame();
#else
        lgt::Renderer::Present();
#endif

        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
#ifndef LGT_DIST
    lgt::UIManager::Shutdown();
#endif
    lgt::Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
