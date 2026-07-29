cmake_minimum_required(VERSION 3.20)
project(OPGLexp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static runtime library for MSVC/clang-cl
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Set output directories based on build configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/MinSizeRel")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo")

# Add source files
set(SOURCES
    src/main.cpp
    # Renderer Core
    src/Renderer/Core/Renderer.cpp
    src/Renderer/Core/RenderCommandQueue.cpp
    src/Renderer/Core/RenderGraph.cpp
    src/Renderer/Core/Shader.cpp
    src/Renderer/Core/UIManager.cpp
    src/Renderer/Core/DebugRenderer.cpp
    # Renderer Resources
    src/Renderer/Resources/Material.cpp
    src/Renderer/Resources/Mesh.cpp
    src/Renderer/Resources/ModelLoader.cpp
    src/Renderer/Resources/TextureLoader.cpp
    # Renderer Utils
    src/Renderer/Utils/BVH.cpp
    # Renderer Passes
    src/Renderer/Core/RayTracingSubsystem.cpp
    src/Renderer/Passes/RadianceCascadesPass.cpp
    src/Renderer/Passes/RTAOPass.cpp
    src/Renderer/Passes/DDGIPass.cpp
    src/Renderer/Passes/IBLPass.cpp
    src/Renderer/Passes/BloomPass.cpp
    src/Renderer/Passes/TAAPass.cpp
    src/Renderer/Passes/RTShadowPass.cpp
    src/Renderer/Passes/CSMPass.cpp
    src/Renderer/Passes/LightCullingPass.cpp
    # Renderer Backend
    src/Renderer/Backend/OpenGLShader.cpp
    src/Renderer/Backend/OpenGLBuffer.cpp
    src/Renderer/Backend/OpenGLTexture.cpp
    src/Renderer/Backend/OpenGLFramebuffer.cpp
    # Scene
    src/Scene/Entity.cpp
    src/Scene/Scene.cpp
    src/Scene/SceneSerializer.cpp
    src/Scene/ComponentSerializerRegistry.cpp
    src/Scene/AnimatorSystem.cpp
    src/Core/Input.cpp
    src/Core/AssetManager.cpp
    src/Game/ScriptRegistry.cpp
    src/Renderer/Resources/ModelLoader.cpp
    # Helpers
    src/Helpers/Logger.cpp
    src/Helpers/DebugStats.cpp
    src/Helpers/GPUTimer.cpp
    # Validation
    src/Renderer/Validation/RendererValidationFramework.cpp
    # Editor
    src/Editor/EditorLayer.cpp
    src/Editor/Panels/HierarchyPanel.cpp
    src/Editor/Panels/InspectorPanel.cpp
    src/Editor/Panels/ViewportPanel.cpp
    src/Editor/Panels/ConsolePanel.cpp
    src/Editor/Panels/AssetBrowserPanel.cpp
    src/Editor/Panels/StatisticsPanel.cpp
    src/Editor/Panels/RenderGraphPanel.cpp
    src/Editor/Panels/AnimationPanel.cpp
    # Vendor
    src/Vendor/glad.c
)

add_executable(OPGLexp ${SOURCES})

# Include directories
target_include_directories(OPGLexp PRIVATE src src/Vendor)

# Set compile definitions based on configuration
target_compile_definitions(OPGLexp PRIVATE 
    $<$<CONFIG:Debug>:LGT_DEBUG>
    $<$<CONFIG:Release>:LGT_DIST>
    $<$<CONFIG:MinSizeRel>:LGT_DIST>
    $<$<CONFIG:RelWithDebInfo>:LGT_RELEASE>
    _CONSOLE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN SPDLOG_NO_FMT_STRING
)

# Compiler flags for Debug build
target_compile_options(OPGLexp PRIVATE 
    $<$<CONFIG:Debug>:-g -O0>
)

# Find and link Vcpkg dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-imgui-node-editor CONFIG REQUIRED)

target_link_libraries(OPGLexp PRIVATE
    glfw
    imgui::imgui
    imguizmo::imguizmo
    glm::glm
    assimp::assimp
    spdlog::spdlog
    fmt::fmt
    EnTT::EnTT
    meshoptimizer::meshoptimizer
    nlohmann_json::nlohmann_json
    unofficial::imgui-node-editor::imgui-node-editor
)

# Post-build task to copy 'res' directory to output directory
add_custom_command(TARGET OPGLexp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:OPGLexp>/res"
    COMMENT "Copying 'res' directory to output directory..."
)

# Compiler specific options (Clang/MSVC compatibility)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(MSVC)
        # clang-cl
        target_compile_options(OPGLexp PRIVATE /W3 /WX- /Od /EHsc /Zc:wchar_t /Zc:forScope /Zc:inline /utf-8)
    else()
        # clang
        target_compile_options(OPGLexp PRIVATE -Wall -g)
    endif()
endif()
