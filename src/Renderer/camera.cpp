#include "Camera.h"
#include "Helpers/Logger.h"
#include <glm/gtx/string_cast.hpp> // for glm::to_string if needed in logging

namespace lgt {
Camera::Camera(int width, int height, glm::vec3 position)
    : m_position(position),
      m_h(height),
      m_w(width) {
    CORE_INFO("Camera initialized at position: [{}]", glm::to_string(m_position));
}

void Camera::update(GLFWwindow* window, const float& speed, const float& sensitivity, bool canCapture) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (!firstclick) {
            CORE_INFO("Camera control disabled. Cursor released.");
            firstclick = true;
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (firstclick && canCapture) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            double temp_x, temp_y;
            glfwGetCursorPos(window, &temp_x, &temp_y);
            lastx = static_cast<float>(temp_x);
            lasty = static_cast<float>(temp_y);
            CORE_INFO("Camera control enabled. Cursor locked.");
            firstclick = false;
        }
    }

    if (!firstclick) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            m_position += speed * glm::vec3(front.x, 0.0f, front.z);

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            m_position -= speed * glm::vec3(front.x, 0.0f, front.z);

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            m_position -= speed * glm::normalize(glm::cross(front, up));

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            m_position += speed * glm::normalize(glm::cross(front, up));

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            m_position -= speed * up;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            m_position += speed * up;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        yaw   = sensitivity * (static_cast<float>(ypos) - lasty) / 2.0f / m_h;
        pitch = sensitivity * (static_cast<float>(xpos) - lastx) / 2.0f / m_w;
        lastx = static_cast<float>(xpos);
        lasty = static_cast<float>(ypos);

        newfront = glm::rotate(front, glm::radians(-yaw), glm::normalize(glm::cross(front, up)));

        if (!(glm::angle(newfront, up) <= glm::radians(5.0f) || glm::angle(newfront, -up) <= glm::radians(5.0f)))
            front = newfront;
        else
            CORE_WARN("Camera pitch limit reached. Rotation blocked.");

        front = glm::rotate(front, glm::radians(-pitch), up);
    }

    View = glm::lookAt(m_position, m_position + front, up);

    if (m_h != 0 && m_w != 0) {
        Projection = glm::perspective(glm::radians(45.0f), static_cast<float>(m_w) / static_cast<float>(m_h), 0.1f, 100.0f);
    } else {
        Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 300.0f);
        CORE_WARN("Window size invalid. Default aspect ratio used.");
    }

    Campos = m_position;
}

glm::vec3 Camera::getPosition() {
    return m_position;
}
glm::vec3 Camera::getFront() {
    return front;
}

const glm::mat4 Camera::GetViewMatrix() {
    return View;
}
const glm::mat4 Camera::GetProjectionMatrix() {
    return Projection;
}
const glm::vec3 Camera::GetCameraPos() {
    return Campos;
}

const glm::vec3 Camera::GetDirection() {
    return front;
}

float Camera::GetFov() const {
    return 45.0f; // Currently hardcoded to 45 degrees
}

float Camera::GetAspect() const {
    if (m_h != 0 && m_w != 0) {
        return static_cast<float>(m_w) / static_cast<float>(m_h);
    }
    return 1.0f;
}

void Camera::setAspect(int w, int h) {
    m_w = w;
    m_h = h;
}

// shadow camera
// -------------------------------------------------------------------------------------------------------------

ShadowCamera::ShadowCamera(float height, float width, glm::vec3 lightpos, glm::vec3 lightdir)
    : m_h(height),
      m_w(width) {
    CORE_INFO("Light initialized at position: {}", glm::to_string(m_position));
    Update(lightpos, lightdir);
}

void ShadowCamera::Update(glm::vec3 lightpos, glm::vec3 lightdir) {
    m_position  = lightpos;
    m_direction = lightdir;

    float near_plane = 0.1f;
    float far_plane  = 300.0f;
    float orthoSize  = 10.0f; // controls shadow coverage area

    m_projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

    m_view = glm::lookAt(m_position, m_position + m_direction, m_up);
}

const glm::mat4 ShadowCamera::GetViewMatrix() {
    return m_view;
}
const glm::mat4 ShadowCamera::GetProjectionMatrix() {
    return m_projection;
}
const glm::vec3 ShadowCamera::GetCameraPos() {
    return m_position;
}

} // namespace lgt
