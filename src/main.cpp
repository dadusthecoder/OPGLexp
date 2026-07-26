#include "Renderer/ErrorReporting.h"
#include "Renderer/GPUResources.h"
#include "Renderer/Scene.h"
#include "Renderer/Material.h"
#include "Renderer/DDGIPass.h"
#include "Renderer/Primitives.h"
#include "Renderer/Camera.h"
#include "Renderer/Renderer.h"
#include "UI/Editor.h"
#include "Helpers/Logger.h"
#include <fstream>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#define MAIN        void main()
#define MAIN_RETURN return

int main() {
    GLFWwindow* window;

    CORE_LOG_INIT();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    int height = 1080, width = 1920;

    window = glfwCreateWindow(width, height, "Lightnig", NULL, NULL);
    assert(window);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        CORE_ERROR("Failed to initialize GLAD");
    }
    enableReportGlErrors();
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f, &fontConfig);
    
    Editor::ApplyProfessionalTheme();

    // Check if imgui.ini exists, if not, load default layout
    if (io.IniFilename) {
        std::ifstream iniFile(io.IniFilename);
        if (!iniFile.good()) {
            const char* default_layout = R"(
[Window][WindowOverViewport_11111111]
Pos=0,0
Size=1920,1055
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Viewport]
Pos=363,0
Size=1210,718
Collapsed=0
DockId=0x00000009,0

[Window][Material Editor]
Pos=1575,0
Size=345,562
Collapsed=0
DockId=0x00000003,0

[Window][Scene Hierarchy]
Pos=0,0
Size=361,549
Collapsed=0
DockId=0x00000007,0

[Window][Environment & Renderer]
Pos=1575,564
Size=345,491
Collapsed=0
DockId=0x00000004,0

[Window][Camera Controls]
Pos=1575,564
Size=345,491
Collapsed=0
DockId=0x00000004,1

[Window][Asset Browser]
Pos=363,720
Size=503,335
Collapsed=0
DockId=0x0000000B,0

[Window][Inspector]
Pos=0,551
Size=361,504
Collapsed=0
DockId=0x00000008,0

[Window][Console]
Pos=868,720
Size=705,335
Collapsed=0
DockId=0x0000000C,0

[Docking][Data]
DockSpace         ID=0x08BD597D Window=0x1BBC0F80 Pos=0,0 Size=1920,1055 Split=X
  DockNode        ID=0x00000005 Parent=0x08BD597D SizeRef=361,1055 Split=Y Selected=0xB8729153
    DockNode      ID=0x00000007 Parent=0x00000005 SizeRef=473,549 Selected=0xB8729153
    DockNode      ID=0x00000008 Parent=0x00000005 SizeRef=473,504 Selected=0x36DC96AB
  DockNode        ID=0x00000006 Parent=0x08BD597D SizeRef=1557,1055 Split=X
    DockNode      ID=0x00000001 Parent=0x00000006 SizeRef=1210,1055 Split=Y Selected=0xC450F867
      DockNode    ID=0x00000009 Parent=0x00000001 SizeRef=1107,718 CentralNode=1 Selected=0xC450F867
      DockNode    ID=0x0000000A Parent=0x00000001 SizeRef=1107,335 Split=X Selected=0x36AF052B
        DockNode  ID=0x0000000B Parent=0x0000000A SizeRef=503,335 Selected=0x36AF052B
        DockNode  ID=0x0000000C Parent=0x0000000A SizeRef=705,335 Selected=0xEA83D666
    DockNode      ID=0x00000002 Parent=0x00000006 SizeRef=345,1055 Split=Y Selected=0x3D0FF072
      DockNode    ID=0x00000003 Parent=0x00000002 SizeRef=336,562 Selected=0x3D0FF072
      DockNode    ID=0x00000004 Parent=0x00000002 SizeRef=336,491 Selected=0xBF434FD5
)";
            ImGui::LoadIniSettingsFromMemory(default_layout);
        }
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("# version 460");

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    CORE_INFO("{}", (char*)glGetString(GL_VENDOR));
    CORE_INFO("{}", (char*)glGetString(GL_RENDERER));

    lgt::Scene  scene;
    lgt::Camera camera((int)width, (int)height, glm::vec3(0.0f, 1.5f, 3.0f));

    // Load Sponza test scene
    // scene.LoadGltf("res/modles/sopnza_palace/Sponza_palace.gltf");
    
    // Create a default 1x1 white texture to prevent shader discard due to handle 0
    GLuint defaultTex;
    glGenTextures(1, &defaultTex);
    glBindTexture(GL_TEXTURE_2D, defaultTex);
    uint32_t whitePixel = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GLuint64 defaultTexHandle = glGetTextureHandleARB(defaultTex);
    glMakeTextureHandleResidentARB(defaultTexHandle);

    // Create test scene materials
    lgt::MaterialGPU matRed = { glm::vec4(1.0f, 0.2f, 0.2f, 1.0f), glm::vec4(0), 1.0f, 0.0f, 0.0f, defaultTexHandle, defaultTexHandle, defaultTexHandle };
    lgt::MaterialGPU matGreen = { glm::vec4(0.2f, 1.0f, 0.2f, 1.0f), glm::vec4(0), 1.0f, 0.0f, 0.0f, defaultTexHandle, defaultTexHandle, defaultTexHandle };
    lgt::MaterialGPU matBlue = { glm::vec4(0.2f, 0.2f, 1.0f, 1.0f), glm::vec4(0), 1.0f, 0.0f, 0.0f, defaultTexHandle, defaultTexHandle, defaultTexHandle };
    lgt::MaterialGPU matWhite = { glm::vec4(0.9f, 0.9f, 0.9f, 1.0f), glm::vec4(0), 1.0f, 0.0f, 0.0f, defaultTexHandle, defaultTexHandle, defaultTexHandle };
    lgt::MaterialGPU matEmissive = { glm::vec4(1.0f), glm::vec4(10.0f, 10.0f, 10.0f, 1.0f), 1.0f, 0.0f, 10.0f, defaultTexHandle, defaultTexHandle, defaultTexHandle };

    uint32_t matRedIdx = lgt::g_MaterialGPU.size(); lgt::g_MaterialGPU.push_back(matRed);
    uint32_t matGreenIdx = lgt::g_MaterialGPU.size(); lgt::g_MaterialGPU.push_back(matGreen);
    uint32_t matBlueIdx = lgt::g_MaterialGPU.size(); lgt::g_MaterialGPU.push_back(matBlue);
    uint32_t matWhiteIdx = lgt::g_MaterialGPU.size(); lgt::g_MaterialGPU.push_back(matWhite);
    uint32_t matEmissiveIdx = lgt::g_MaterialGPU.size(); lgt::g_MaterialGPU.push_back(matEmissive);

    // Register them in the BRDF map for the Material Editor UI
    lgt::g_MaterialBRDF["Red Wall"] = { matRedIdx, matRed.baseColor, matRed.emmisiveColor, matRed.roughness, matRed.metallic, matRed.emmisiveStrength };
    lgt::g_MaterialBRDF["Green Wall"] = { matGreenIdx, matGreen.baseColor, matGreen.emmisiveColor, matGreen.roughness, matGreen.metallic, matGreen.emmisiveStrength };
    lgt::g_MaterialBRDF["Blue"] = { matBlueIdx, matBlue.baseColor, matBlue.emmisiveColor, matBlue.roughness, matBlue.metallic, matBlue.emmisiveStrength };
    lgt::g_MaterialBRDF["White Floor"] = { matWhiteIdx, matWhite.baseColor, matWhite.emmisiveColor, matWhite.roughness, matWhite.metallic, matWhite.emmisiveStrength };
    lgt::g_MaterialBRDF["Emissive Bulb"] = { matEmissiveIdx, matEmissive.baseColor, matEmissive.emmisiveColor, matEmissive.roughness, matEmissive.metallic, matEmissive.emmisiveStrength };

    // Setup Directional Light (Sun)
    lgt::Light sunLight;
    sunLight.position = glm::vec4(0.0f, 20.0f, 0.0f, 1.0f); // w=1 (directional)
    sunLight.color     = glm::vec4(1.0f, 0.95f, 0.85f, 3.0f); // Warm sun + intensity
    sunLight.direction = glm::vec4(0.2f, -1.0f, 0.3f, 1.0f); // Angled down
    sunLight.params    = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
    scene.addLight(sunLight);

    // Setup a couple of point lights
    lgt::Light point1;
    point1.position = glm::vec4(4.0f, 1.0f, 0.0f, 0.0f); // w=0 (point)
    point1.color     = glm::vec4(1.0f, 0.2f, 0.2f, 5.0f); // Red
    point1.direction = glm::vec4(0.0f, 0.0f, 0.0f, 8.0f); // w=radius (8m)
    point1.params    = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
    scene.addLight(point1);

    lgt::Light point2;
    point2.position = glm::vec4(-4.0f, 1.0f, 0.0f, 0.0f); // w=0 (point)
    point2.color     = glm::vec4(0.2f, 0.2f, 1.0f, 5.0f); // Blue
    point2.direction = glm::vec4(0.0f, 0.0f, 0.0f, 8.0f); // w=radius (8m)
    point2.params    = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
    scene.addLight(point2);

    // Load Sponza scene
    scene.LoadGltf("res/modles/sopnza_palace/Sponza_palace.gltf");

    scene.Update();
    scene.BuildAccelerationStructure();

    // Setup DDGI Volume for the test scene
    auto ddgiVolume = std::make_unique<lgt::DDGIVolume>();
    ddgiVolume->Init(glm::vec3(-15.0f, -15.0f, -15.0f), glm::vec3(15.0f, 15.0f, 15.0f));
    scene.GetProbeVolumes().push_back(std::move(ddgiVolume));

    lgt::Renderer renderer(&scene, &camera);
    renderer.init();
    
    lgt::Grid grid;
    lgt::FrameBuffer framebuffer(width, height);
    float camSpeed = 0.05f;
    float camSensitivity = 20.0f;
    float deltaTime = 0.016f; // mock deltaTime for now

    bool rKeyWasPressed = false;
    bool fKeyWasPressed = false;

    // game loopx
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Default background color for the dockspace
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // scene.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // Viewport Panel & 3D Rendering
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        bool viewportHovered = ImGui::IsWindowHovered();
        camera.update(window, camSpeed, camSensitivity, viewportHovered);
        
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
            // Resize FBO and Camera if the viewport changed
            if (framebuffer.GetWidth() != static_cast<int>(viewportSize.x) || framebuffer.GetHeight() != static_cast<int>(viewportSize.y)) {
                framebuffer.Resize(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
                camera.setAspect(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
            }

            // 3D Scene Rendering to FBO
            framebuffer.Use();
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Viewport background color
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            renderer.setViewport(framebuffer.GetWidth(), framebuffer.GetHeight());
            renderer.render(&grid, deltaTime);
            
            framebuffer.Unuse();

            // Display FBO texture in ImGui
            ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
            
            GLuint displayTexture = framebuffer.GetTextureId();
            const auto& ctx = renderer.getRenderContext();
            if (ctx.gBufferDebugView == 1) displayTexture = ctx.gAlbedoMetallic;
            else if (ctx.gBufferDebugView == 2) displayTexture = ctx.gNormalRoughness;
            else if (ctx.gBufferDebugView == 3) displayTexture = ctx.gEmissive;
            else if (ctx.gBufferDebugView == 4) displayTexture = ctx.gVelocity;
            else if (ctx.gBufferDebugView == 5) displayTexture = ctx.gDepth;
            else if (ctx.gBufferDebugView == 6) displayTexture = ctx.aoTexture;
            else if (ctx.gBufferDebugView == 7 && ctx.gpuResources) displayTexture = ctx.gpuResources->ddgiIrradianceAtlas;
            
            ImGui::Image((ImTextureID)(intptr_t)displayTexture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (!ImGuizmo::IsOver()) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    int mouseX = (int)(mousePos.x - cursorScreenPos.x);
                    int mouseY = (int)(mousePos.y - cursorScreenPos.y);
                    renderer.setMouseSelection(mouseX, mouseY);
                }
            }

            // ImGuizmo
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
            
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = camera.GetProjectionMatrix();
            
            if (Editor::GetSelectedNode()) {
                glm::mat4 transform = Editor::GetSelectedNode()->localTransform;
                ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                                     (ImGuizmo::OPERATION)Editor::GetGizmoOperation(), ImGuizmo::LOCAL, glm::value_ptr(transform));
                if (ImGuizmo::IsUsing()) {
                    Editor::GetSelectedNode()->localTransform = transform;
                    // Trigger transform cascade update
                    for (auto root : scene.getRootNodes()) {
                        root->UpdateTransformCascades();
                    }
                }
            } else if (Editor::GetSelectedLightIndex() != -1) {
                int lightIdx = Editor::GetSelectedLightIndex();
                lgt::Light& light = scene.getLights()[lightIdx];
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(light.position));
                
                ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                                     ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(transform));
                if (ImGuizmo::IsUsing()) {
                    light.position.x = transform[3][0];
                    light.position.y = transform[3][1];
                    light.position.z = transform[3][2];
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        Editor::DrawMaterialEditorPanel();
        Editor::DrawSceneHierarchyPanel(&scene);
        Editor::DrawEnvironmentPanel(&grid, &renderer, &scene);
        Editor::DrawCameraPanel(&camera, &camSpeed, &camSensitivity);
        Editor::DrawAssetBrowserPanel(&scene);
        Editor::DrawInspectorPanel();
        Editor::DrawConsolePanel();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyWasPressed) {
            renderer.shutdown();
            renderer.init();
            rKeyWasPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && rKeyWasPressed)
            rKeyWasPressed = false;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyWasPressed) {
            int currentMode = (int)renderer.getDebugMode();
            currentMode = (currentMode + 1) % 5; // 0=Final, 1=Albedo, 2=Normal, 3=Emissive, 4=WorldNormal
            renderer.setDebugMode((lgt::DebugMode)(currentMode));
            fKeyWasPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && fKeyWasPressed)
            fKeyWasPressed = false;

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
