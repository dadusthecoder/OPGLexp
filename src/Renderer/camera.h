#pragma once
#include "Renderer.h"

namespace lgt {
class Camera {
private:
    glm::vec3 front = glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 m_position;
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 Campos;
    glm::vec3 newfront;
    glm::mat4 View       = glm::mat4{1.0f};
    glm::mat4 Projection = glm::mat4{1.0f};

    float yaw = 0, pitch = 0, lastx = 0, lasty = 0;
    bool  firstclick = true;
    int   m_h, m_w;

public:
    Camera(int width, int height, glm::vec3 positon);
    void            update(GLFWwindow* window, const float& speed, const float& sensitivity, bool canCapture = false);
    glm::vec3       getPosition();
    glm::vec3       getFront();
    const glm::mat4 GetViewMatrix();
    const glm::mat4 GetProjectionMatrix();
    const glm::vec3 GetCameraPos();
    const glm::vec3 GetDirection();
    float           GetFov() const;
    float           GetAspect() const;
    void            setAspect(int w, int h);
};

class ShadowCamera {
private:
    glm::vec3 m_direction = glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 m_position  = glm::vec3(2.0f, 4.0f, 2.0f);
    glm::vec3 m_up        = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 m_newdirection;
    glm::mat4 m_view       = glm::mat4{1.0f};
    glm::mat4 m_projection = glm::mat4{1.0f};

    float m_h, m_w;

public:
    ShadowCamera(float height, float width, glm::vec3 lightpos, glm::vec3 lightdir);
    void            Update(glm::vec3 lightpos, glm::vec3 lightdir);
    glm::vec3       getPosition();
    glm::vec3       getFront();
    const glm::mat4 GetViewMatrix();
    const glm::mat4 GetProjectionMatrix();
    const glm::vec3 GetCameraPos();
};
} // namespace lgt
