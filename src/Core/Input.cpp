#include "Input.h"

namespace lgt {

    GLFWwindow* Input::s_Window = nullptr;
    glm::vec2 Input::s_LastMousePos = glm::vec2(0.0f);
    glm::vec2 Input::s_MouseDelta = glm::vec2(0.0f);
    std::unordered_map<std::string, int> Input::s_ActionMap;
    bool Input::s_FirstMouse = true;

    void Input::Init(GLFWwindow* window) {
        s_Window = window;
        s_FirstMouse = true;
        s_MouseDelta = glm::vec2(0.0f);
    }

    void Input::Update() {
        if (!s_Window) return;

        double xpos, ypos;
        glfwGetCursorPos(s_Window, &xpos, &ypos);

        glm::vec2 currentPos = glm::vec2((float)xpos, (float)ypos);

        if (s_FirstMouse) {
            s_LastMousePos = currentPos;
            s_FirstMouse = false;
        }

        s_MouseDelta = currentPos - s_LastMousePos;
        s_LastMousePos = currentPos;
    }

    bool Input::IsKeyDown(int key) {
        if (!s_Window) return false;
        auto state = glfwGetKey(s_Window, key);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonDown(int button) {
        if (!s_Window) return false;
        auto state = glfwGetMouseButton(s_Window, button);
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition() {
        if (!s_Window) return glm::vec2(0.0f);
        double xpos, ypos;
        glfwGetCursorPos(s_Window, &xpos, &ypos);
        return glm::vec2((float)xpos, (float)ypos);
    }

    glm::vec2 Input::GetMouseDelta() {
        return s_MouseDelta;
    }

    void Input::MapAction(const std::string& action, int key) {
        s_ActionMap[action] = key;
    }

    bool Input::IsActionDown(const std::string& action) {
        auto it = s_ActionMap.find(action);
        if (it != s_ActionMap.end()) {
            return IsKeyDown(it->second);
        }
        return false;
    }

}
