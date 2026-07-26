#pragma once

struct GLFWwindow;

namespace lgt {

    class UIManager {
    public:
        static void Init(GLFWwindow* window);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();
    };

}
