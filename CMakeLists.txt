cmake_minimum_required(VERSION 3.20)
project(OPGLexp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static runtime library for MSVC/clang-cl
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.c")

add_executable(OPGLexp ${SRC_FILES})

# Include directories
target_include_directories(OPGLexp PRIVATE src)

# Set compile definitions
target_compile_definitions(OPGLexp PRIVATE LGT_DEBUG _CONSOLE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN SPDLOG_NO_FMT_STRING)

# Find and link Vcpkg dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_link_libraries(OPGLexp PRIVATE
    glfw
    imgui::imgui
    imguizmo::imguizmo
    glm::glm
    assimp::assimp
    spdlog::spdlog
    fmt::fmt
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
