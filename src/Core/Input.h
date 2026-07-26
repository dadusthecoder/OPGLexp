#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace lgt {

    class Input {
    public:
        static void Init(GLFWwindow* window);
        static void Update();

        static bool IsKeyDown(int key);
        static bool IsMouseButtonDown(int button);
        
        static glm::vec2 GetMousePosition();
        static glm::vec2 GetMouseDelta();

        static void MapAction(const std::string& action, int key);
        static bool IsActionDown(const std::string& action);

    private:
        static GLFWwindow* s_Window;
        static glm::vec2 s_LastMousePos;
        static glm::vec2 s_MouseDelta;
        static std::unordered_map<std::string, int> s_ActionMap;
        static bool s_FirstMouse;
    };

}
